#include "CViewStack.h"
#include "SYS_Main.h"
#include "CGuiMain.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebugDataAdapter.h"
#include "CViewDisassembly.h"
#include "CLayoutParameter.h"
#include "VID_Main.h"

static const char *entryTypeNames[] = {
	"???",       // STACK_ENTRY_UNKNOWN
	"PHA",       // STACK_ENTRY_VALUE
	"PHP",       // STACK_ENTRY_STATUS
	"JSR",       // STACK_ENTRY_JSR_PCH
	"JSR",       // STACK_ENTRY_JSR_PCL
	"BRK",       // STACK_ENTRY_BRK_PCH
	"BRK",       // STACK_ENTRY_BRK_PCL
	"BRK",       // STACK_ENTRY_BRK_STATUS
	"IRQ",       // STACK_ENTRY_IRQ_PCH
	"IRQ",       // STACK_ENTRY_IRQ_PCL
	"IRQ",       // STACK_ENTRY_IRQ_STATUS
	"NMI",       // STACK_ENTRY_NMI_PCH
	"NMI",       // STACK_ENTRY_NMI_PCL
	"NMI",       // STACK_ENTRY_NMI_STATUS
};

// Colors per entry type group: { r, g, b }
static const float typeColors[][3] = {
	{ 0.6f, 0.6f, 0.6f },  // UNKNOWN — grey
	{ 0.0f, 0.9f, 0.0f },  // VALUE (PHA) — green
	{ 0.3f, 0.7f, 1.0f },  // STATUS (PHP) — light blue
	{ 1.0f, 1.0f, 0.3f },  // JSR_PCH — yellow
	{ 1.0f, 1.0f, 0.3f },  // JSR_PCL — yellow
	{ 1.0f, 0.5f, 0.2f },  // BRK_PCH — orange
	{ 1.0f, 0.5f, 0.2f },  // BRK_PCL — orange
	{ 1.0f, 0.5f, 0.2f },  // BRK_STATUS — orange
	{ 1.0f, 0.3f, 0.3f },  // IRQ_PCH — red
	{ 1.0f, 0.3f, 0.3f },  // IRQ_PCL — red
	{ 1.0f, 0.3f, 0.3f },  // IRQ_STATUS — red
	{ 0.9f, 0.3f, 0.9f },  // NMI_PCH — magenta
	{ 0.9f, 0.3f, 0.9f },  // NMI_PCL — magenta
	{ 0.9f, 0.3f, 0.9f },  // NMI_STATUS — magenta
};

CViewStack::CViewStack(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					   CDebugInterface *debugInterface,
					   CStackAnnotationData *stackAnnotation,
					   CDebugDataAdapter *dataAdapter)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	this->stackAnnotation = stackAnnotation;
	this->dataAdapter = dataAdapter;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontBytes = viewC64->fontDisassembly;

	fontSize = 7.0f;
	hasManualFontSize = false;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	readSPFunc = NULL;
	readSPContext = NULL;
	currentSP = 0xFF;

	viewDisassembly = NULL;
	numAddrHitRegions = 0;

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewStack::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	bool sizeChanged = (fabs(sizeX - this->sizeX) > 0.5f || fabs(sizeY - this->sizeY) > 0.5f);

	if (hasManualFontSize && !sizeChanged)
	{
	}
	else
	{
		// Auto-scale: ~28 chars wide
		fontSize = sizeX / 28.0f;

		if (sizeChanged)
			hasManualFontSize = false;
	}

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewStack::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	if (layoutParameter != NULL)
	{
		hasManualFontSize = true;
	}
	else
	{
		float autoFontSize = sizeX / 28.0f;

		if (fabs(fontSize - autoFontSize) > 0.01f)
		{
			hasManualFontSize = true;
		}
	}

	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewStack::DoLogic()
{
}

void CViewStack::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

const char *CViewStack::GetEntryTypeName(u8 entryType)
{
	if (entryType < sizeof(entryTypeNames) / sizeof(entryTypeNames[0]))
		return entryTypeNames[entryType];
	return "???";
}

// Returns how many stack bytes this group consumes (1, 2, or 3)
// and the group name. Walking upward from SP+1 (low addr to high):
//   JSR_PCL at idx → 2 bytes (PCL, PCH)
//   IRQ/BRK/NMI_STATUS at idx → 3 bytes (STATUS, PCL, PCH)
//   PHA/PHP/UNKNOWN → 1 byte
static int GetGroupSize(u8 entryType)
{
	switch (entryType)
	{
		case STACK_ENTRY_JSR_PCL:
			return 2;
		case STACK_ENTRY_IRQ_STATUS:
		case STACK_ENTRY_BRK_STATUS:
		case STACK_ENTRY_NMI_STATUS:
			return 3;
		default:
			return 1;
	}
}

void CViewStack::Render()
{
	if (readSPFunc)
		currentSP = readSPFunc(readSPContext);

	float px = posX;
	float py = posY;

	char buf[64];
	char bytesStr[16];

	u8 sp = currentSP;
	numAddrHitRegions = 0;

	// Header: SP=$xx
	sprintf(buf, "SP=$%02X", sp);
	fontBytes->BlitText(buf, px, py, posZ, fontSize);
	py += fontSize * 1.3f;

	// Show stack from SP+1 to $FF (occupied portion)
	float lineHeight = fontSize;
	float bottomY = posY + sizeY;
	int idx = (u8)(sp + 1);

	// Character width for the monospace font
	float charW = fontSize;

	// The origin PC address starts at character offset 14 in the line:
	// "XX: " (4) + bytesStr (10) = 14 chars before the 4-char hex address
	float addrCharOffset = 14.0f;
	float addrCharWidth = 4.0f;

	// Get mouse position for hover detection
	float mx = guiMain->mousePosX;
	float my = guiMain->mousePosY;

	if (sp == 0xFF)
	{
		//fontBytes->BlitText("(empty)", px, py, posZ, fontSize);
		return;
	}

	while (idx <= 0xFF && py + lineHeight <= bottomY)
	{
		u8 entryType = stackAnnotation->entryTypes[idx];
		int groupSize = GetGroupSize(entryType);

		// Ensure group fits within stack bounds ($FF)
		if (idx + groupSize - 1 > 0xFF)
			groupSize = 0xFF - idx + 1;

		// Read byte values for the group
		u8 vals[3] = { 0, 0, 0 };
		for (int j = 0; j < groupSize; j++)
		{
			if (dataAdapter)
				dataAdapter->AdapterReadByte(0x0100 + idx + j, &vals[j]);
		}

		// Format bytes column (padded to 10 chars)
		if (groupSize == 3)
			sprintf(bytesStr, "%02X %02X %02X  ", vals[0], vals[1], vals[2]);
		else if (groupSize == 2)
			sprintf(bytesStr, "%02X %02X     ", vals[0], vals[1]);
		else
			sprintf(bytesStr, "%02X        ", vals[0]);

		u16 originPC = stackAnnotation->originPC[idx];
		const char *typeName = GetEntryTypeName(entryType);

		// Check for IRQ source on interrupt groups
		u8 irqSource = stackAnnotation->irqSources[idx];
		if (irqSource != IRQ_SOURCE_UNKNOWN && groupSize >= 3)
		{
			const char *srcName = debugInterface->GetIrqSourceName(irqSource);
			sprintf(buf, "%02X: %s%04X %s %s", idx, bytesStr, originPC, typeName, srcName);
		}
		else if (entryType != STACK_ENTRY_UNKNOWN)
		{
			sprintf(buf, "%02X: %s%04X %s", idx, bytesStr, originPC, typeName);
		}
		else
		{
			sprintf(buf, "%02X: %s", idx, bytesStr);
		}

		// Store hit region for the origin PC address (only for known entries)
		if (entryType != STACK_ENTRY_UNKNOWN && numAddrHitRegions < STACK_VIEW_MAX_HIT_REGIONS)
		{
			CStackAddrHitRegion *hr = &addrHitRegions[numAddrHitRegions];
			hr->x = px + addrCharOffset * charW;
			hr->y = py;
			hr->w = addrCharWidth * charW;
			hr->h = lineHeight;
			hr->addr = originPC;
			numAddrHitRegions++;
		}

		// Color based on entry type
		float r = 1.0f, g = 1.0f, b = 1.0f;
		if (entryType < sizeof(typeColors) / sizeof(typeColors[0]))
		{
			r = typeColors[entryType][0];
			g = typeColors[entryType][1];
			b = typeColors[entryType][2];
		}

		fontBytes->BlitTextColor(buf, px, py, posZ, fontSize, r, g, b, 1.0f);

		// Draw hover highlight on the address if mouse is over it
		if (entryType != STACK_ENTRY_UNKNOWN)
		{
			float ax = px + addrCharOffset * charW;
			float aw = addrCharWidth * charW;
			if (mx >= ax && mx <= ax + aw && my >= py && my <= py + lineHeight)
			{
				BlitRectangle(ax, py, posZ, aw, lineHeight, 1.0f, 1.0f, 1.0f, 0.8f, 1.0f);
			}
		}

		py += lineHeight;

		idx += groupSize;
	}

	// Show "..." if there are more entries that don't fit
	if (idx <= 0xFF)
	{
		fontBytes->BlitText("...", px, py, posZ, fontSize);
	}
}

bool CViewStack::DoTap(float x, float y)
{
	for (int i = 0; i < numAddrHitRegions; i++)
	{
		CStackAddrHitRegion *hr = &addrHitRegions[i];
		if (x >= hr->x && x <= hr->x + hr->w && y >= hr->y && y <= hr->y + hr->h)
		{
			if (viewDisassembly)
			{
				viewDisassembly->ScrollToAddress(hr->addr);
			}
			return true;
		}
	}
	return false;
}

// Layout
void CViewStack::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewStack::Deserialize(CByteBuffer *byteBuffer)
{
}
