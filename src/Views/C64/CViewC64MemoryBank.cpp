#include "CViewC64MemoryBank.h"
#include "SYS_Main.h"
#include "CGuiMain.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64StateVIC.h"
#include "CLayoutParameter.h"
#include "MT_UiScale.h"
#include "C64BankingModes.h"
#include "CDebugMemoryMapping.h"

CViewC64MemoryBank::CViewC64MemoryBank(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontBytes = viewC64->fontDisassembly;

	fontSize = MT_UiScaled(7.0f);
	hasManualFontSize = false;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64MemoryBank::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	bool sizeChanged = (fabs(sizeX - this->sizeX) > 0.5f || fabs(sizeY - this->sizeY) > 0.5f);

	if (hasManualFontSize && !sizeChanged)
	{
		// Size unchanged (startup/layout restore): keep manually set font size
	}
	else
	{
		// Auto-scale: ~30 chars wide, ~14 rows tall
		fontSize = fmin(sizeX / 30.0f, sizeY / 12.0f);

		if (sizeChanged)
			hasManualFontSize = false;
	}

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64MemoryBank::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	if (layoutParameter != NULL)
	{
		hasManualFontSize = true;
	}
	else
	{
		float autoFontSize = fmin(sizeX / 30.0f, sizeY / 12.0f);

		if (fabs(fontSize - autoFontSize) > 0.01f)
		{
			hasManualFontSize = true;
		}
	}

	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64MemoryBank::DoLogic()
{
}

void CViewC64MemoryBank::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

// Color per C64BankEntry: { r, g, b }
static const float memMapEntryColors[][3] = {
	{ 0.0f, 0.8f, 0.0f },		// RAM        — green
	{ 0.9f, 0.5f, 0.1f },		// BASIC_ROM  — orange
	{ 0.9f, 0.5f, 0.1f },		// KERNAL_ROM — orange
	{ 0.3f, 0.6f, 1.0f },		// CHAR_ROM   — blue
	{ 1.0f, 1.0f, 0.2f },		// IO         — yellow
	{ 0.9f, 0.3f, 0.9f },		// CART_LO    — magenta
	{ 0.9f, 0.3f, 0.9f },		// CART_HI    — magenta
	{ 0.5f, 0.5f, 0.5f },		// OPEN       — grey
};

void CViewC64MemoryBank::Render()
{
	float px = posX;
	float py = posY;

	// Locked-state background (historical frame)
	if (viewC64->viewC64StateVIC->GetIsLockedState())
	{
		BlitFilledRectangle(posX, posY, -1, sizeX, sizeY, 0.35f, 0.0f, 0.0f, 1.0f);
	}

	// Read state
	u8 port = viewC64->viciiStateToShow.memory0001;
	u8 exrom = viewC64->viciiStateToShow.exrom;
	u8 game = viewC64->viciiStateToShow.game;

	int mode = C64BankingMode(exrom, game, port);

	char buf[64];

	// Header: $01=XX  MODE NN
	sprintf(buf, "$01=%02x  MODE %d", port, mode);
	fontBytes->BlitText(buf, px, py, posZ, fontSize);
	py += fontSize * 1.5f;

	// Bit header row
	float col1 = px;
	float colExrom = px + fontSize * 19.0f;

	sprintf(buf, " 7 6 5 4 3 2 1 0");
	fontBytes->BlitText(buf, col1, py, posZ, fontSize);
	fontBytes->BlitText("EXROM GAME", colExrom, py, posZ, fontSize);
	py += fontSize;

	// Bit values row
	sprintf(buf, " %c %c %d %d %d %d %d %d",
			'.', '.',
			(port >> 5) & 1, (port >> 4) & 1, (port >> 3) & 1,
			(port >> 2) & 1, (port >> 1) & 1, port & 1);
	fontBytes->BlitText(buf, col1, py, posZ, fontSize);

	sprintf(buf, "  %d     %d", exrom, game);
	fontBytes->BlitText(buf, colExrom, py, posZ, fontSize);
	py += fontSize;

	py += fontSize * 0.5f;

	// Memory map: all regions, color-coded
	if (C64BankingIsUltimax(mode))
	{
		sprintf(buf, "$1000-$7FFF  %s", C64BankEntryName(C64BANK_OPEN));
		const float *c = memMapEntryColors[C64BANK_OPEN];
		fontBytes->BlitTextColor(buf, px, py, posZ, fontSize, c[0], c[1], c[2], 1.0f);
		py += fontSize;
	}

	// the live machine (not a locked VIC-II state): the cartridge bank in
	// each window, from the mapping resolver (RD-028)
	CDebugMemoryMappingResolver *resolver = viewC64->viewC64StateVIC->GetIsLockedState() ? NULL
		: debugInterface->GetMemoryMappingResolver();
	static const u16 regionAddress[C64BANK_NUM_REGIONS] = { 0x8000, 0xA000, 0xC000, 0xD000, 0xE000 };

	for (int r = 0; r < C64BANK_NUM_REGIONS; r++)
	{
		C64BankEntry entry = C64BankingEntry(mode, r);
		sprintf(buf, "%s  %s", C64BankRegionName(r), C64BankEntryName(entry));
		if (resolver && (entry == C64BANK_CART_LO || entry == C64BANK_CART_HI))
		{
			CDebugMappedAddress mapped = resolver->ResolveRead(regionAddress[r]);
			if (mapped.physical.device == "c64-cart")
				sprintf(buf + strlen(buf), " BANK %u", mapped.physical.offset >> 13);
		}
		const float *c = memMapEntryColors[entry];
		fontBytes->BlitTextColor(buf, px, py, posZ, fontSize, c[0], c[1], c[2], 1.0f);
		py += fontSize;
	}

	py += fontSize * 0.5f;

	// Bit labels with values, green when set
	float gR = 0.0f, gG = 0.8f, gB = 0.0f;	// green for "on"
	float dR = 1.0f, dG = 1.0f, dB = 1.0f;	// default white for "off"

	// Row 1: 0 LORAM=x  1 HIRAM=x  2 CHAREN=x
	struct { int bit; const char *name; } portBits[] = {
		{ 0, "LORAM" }, { 1, "HIRAM" }, { 2, "CHAREN" },
		{ 3, "CDATA" }, { 4, "SENSE" }, { 5, "MOTOR" }
	};

	float cx = px;
	for (int i = 0; i < 3; i++)
	{
		int v = (port >> portBits[i].bit) & 1;
		sprintf(buf, "%d %s=%d", portBits[i].bit, portBits[i].name, v);
		float r = v ? gR : dR, g = v ? gG : dG, b = v ? gB : dB;
		fontBytes->BlitTextColor(buf, cx, py, posZ, fontSize, r, g, b, 1.0f);
		cx += fontSize * 10.0f;
	}
	py += fontSize;

	// Row 2: 3 CDATA=x  4 SENSE=x  5 MOTOR=x
	cx = px;
	for (int i = 3; i < 6; i++)
	{
		int v = (port >> portBits[i].bit) & 1;
		sprintf(buf, "%d %s=%d", portBits[i].bit, portBits[i].name, v);
		float r = v ? gR : dR, g = v ? gG : dG, b = v ? gB : dB;
		fontBytes->BlitTextColor(buf, cx, py, posZ, fontSize, r, g, b, 1.0f);
		cx += fontSize * 10.0f;
	}
}

// Layout
void CViewC64MemoryBank::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64MemoryBank::Deserialize(CByteBuffer *byteBuffer)
{
}
