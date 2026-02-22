#include "CViewC64MemoryBank.h"
#include "SYS_Main.h"
#include "CGuiMain.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64StateVIC.h"
#include "CLayoutParameter.h"

// Memory region contents
enum MemMapEntry
{
	RAM,
	BASIC_ROM,
	KERNAL_ROM,
	CHAR_ROM,
	IO,
	CART_LO,
	CART_HI,
	OPEN		// Ultimax: unmapped, directly accesses the bus
};

static const char *memMapEntryNames[] = {
	"RAM",
	"BASIC ROM",
	"KERNAL ROM",
	"CHAR ROM",
	"I/O",
	"CART ROM LO",
	"CART ROM HI",
	"OPEN"
};

// Region address labels
static const char *memMapRegionNames[] = {
	"$8000-$9FFF",		// 0: 8K at $8000
	"$A000-$BFFF",		// 1: 8K at $A000
	"$C000-$CFFF",		// 2: 4K at $C000
	"$D000-$DFFF",		// 3: 4K at $D000
	"$E000-$FFFF"		// 4: 8K at $E000
};

// 32-mode lookup table: mode = (exrom << 4) | (game << 3) | (port & 7)
// Each entry: { $8000, $A000, $C000, $D000, $E000 }
// Based on https://www.c64-wiki.com/wiki/Bank_Switching
static const MemMapEntry memoryModes[32][5] = {
	// EXROM=0, GAME=0 (modes 0-7)
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  //  0: $x0
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  //  1: $x1
	{ RAM,     RAM,      RAM,     CHAR_ROM, KERNAL_ROM }, //  2: $x2
	{ CART_LO, CART_HI,  RAM,     CHAR_ROM, KERNAL_ROM }, //  3: $x3
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  //  4: $x4
	{ RAM,     RAM,      RAM,     IO,       RAM      },  //  5: $x5
	{ RAM,     RAM,      RAM,     IO,       KERNAL_ROM }, //  6: $x6
	{ CART_LO, CART_HI,  RAM,     IO,       KERNAL_ROM }, //  7: $x7

	// EXROM=0, GAME=1 (modes 8-15)
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  //  8: $x0
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  //  9: $x1
	{ RAM,     RAM,      RAM,     CHAR_ROM, KERNAL_ROM }, // 10: $x2
	{ CART_LO, BASIC_ROM,RAM,     CHAR_ROM, KERNAL_ROM }, // 11: $x3
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  // 12: $x4
	{ RAM,     RAM,      RAM,     IO,       RAM      },  // 13: $x5
	{ RAM,     RAM,      RAM,     IO,       KERNAL_ROM }, // 14: $x6
	{ CART_LO, BASIC_ROM,RAM,     IO,       KERNAL_ROM }, // 15: $x7

	// EXROM=1, GAME=0 — Ultimax mode (modes 16-23)
	{ OPEN,    OPEN,     OPEN,    OPEN,     CART_HI  },  // 16: $x0
	{ OPEN,    OPEN,     OPEN,    OPEN,     CART_HI  },  // 17: $x1
	{ CART_LO, OPEN,     OPEN,    CHAR_ROM, CART_HI  },  // 18: $x2
	{ CART_LO, OPEN,     OPEN,    CHAR_ROM, CART_HI  },  // 19: $x3
	{ OPEN,    OPEN,     OPEN,    OPEN,     CART_HI  },  // 20: $x4
	{ CART_LO, OPEN,     OPEN,    IO,       CART_HI  },  // 21: $x5
	{ CART_LO, OPEN,     OPEN,    IO,       CART_HI  },  // 22: $x6
	{ CART_LO, OPEN,     OPEN,    IO,       CART_HI  },  // 23: $x7

	// EXROM=1, GAME=1 — normal, no cartridge (modes 24-31)
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  // 24: $x0
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  // 25: $x1
	{ RAM,     RAM,      RAM,     CHAR_ROM, KERNAL_ROM }, // 26: $x2
	{ RAM,     BASIC_ROM,RAM,     CHAR_ROM, KERNAL_ROM }, // 27: $x3
	{ RAM,     RAM,      RAM,     RAM,      RAM      },  // 28: $x4
	{ RAM,     RAM,      RAM,     IO,       RAM      },  // 29: $x5
	{ RAM,     RAM,      RAM,     IO,       KERNAL_ROM }, // 30: $x6
	{ RAM,     BASIC_ROM,RAM,     IO,       KERNAL_ROM }, // 31: $x7 (default)
};

// Ultimax mode also affects $1000-$7FFF (unmapped)
static bool IsUltimaxMode(int mode)
{
	return (mode >= 16 && mode <= 23);
}

CViewC64MemoryBank::CViewC64MemoryBank(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontBytes = viewC64->fontDisassembly;

	fontSize = 7.0f;
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

// Color per entry type: { r, g, b }
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

	int mode = (exrom << 4) | (game << 3) | (port & 0x07);

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
	const MemMapEntry *regions = memoryModes[mode];

	if (IsUltimaxMode(mode))
	{
		sprintf(buf, "$1000-$7FFF  %s", memMapEntryNames[OPEN]);
		const float *c = memMapEntryColors[OPEN];
		fontBytes->BlitTextColor(buf, px, py, posZ, fontSize, c[0], c[1], c[2], 1.0f);
		py += fontSize;
	}

	for (int r = 0; r < 5; r++)
	{
		MemMapEntry entry = regions[r];
		sprintf(buf, "%s  %s", memMapRegionNames[r], memMapEntryNames[entry]);
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
