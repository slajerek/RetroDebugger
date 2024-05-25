#include "CDebugMemoryCell.h"
#include "CDebugMemory.h"

const float alphaSplit = 0.55f;
const float colorSplit = 0.5f;

#if defined(RUN_ATARI)

// TODO FIXME: colorExecuteCodeR

float colorExecuteCodeR = 0.8f;
float colorExecuteCodeG = 1.0f;
float colorExecuteCodeB = 1.0f;
float colorExecuteCodeA = 0.9f;
#else
float colorExecuteCodeR = 0.0f;
float colorExecuteCodeG = 1.0f;
float colorExecuteCodeB = 1.0f;
float colorExecuteCodeA = 0.7f;
#endif

float colorExecuteArgumentR = 0.0f;
float colorExecuteArgumentG = 0.0f;
float colorExecuteArgumentB = 1.0f;
float colorExecuteArgumentA = 0.7f;

float colorExecuteCodeR2 = colorExecuteCodeR;
float colorExecuteCodeG2 = colorExecuteCodeG;
float colorExecuteCodeB2 = colorExecuteCodeB;
float colorExecuteCodeA2 = 1.0f; //colorExecuteCodeA * fce;

float colorExecuteArgumentR2 = colorExecuteArgumentR;
float colorExecuteArgumentG2 = colorExecuteArgumentG;
float colorExecuteArgumentB2 = colorExecuteArgumentB;
float colorExecuteArgumentA2 = 1.0f; //colorExecuteArgumentA * fce;

// rgb color is represented as <0.0, 1.0>
// returned rgba is later clamped after adding read/write markers
void ComputeColorFromValueStyleRGB(u8 v, float *r, float *g, float *b)
{
	float vc = (float)v / 255.0f;
	
	if (vc < 0.333333f)
	{
		*r = vc * colorSplit;
		*g = 0.0f;
		*b = 0.0f;
	}
	else if (vc < 0.666666f)
	{
		*r = 0.0f;
		*g = vc * colorSplit;
		*b = 0.0f;
	}
	else
	{
		*r = 0.0f;
		*g = 0.0f;
		*b = vc * colorSplit;
	}
	
	float vc2 = vc * 0.33f;
	*r += vc2;
	*g += vc2;
	*b += vc2;
}

void ComputeColorFromValueStyleGrayscale(u8 v, float *r, float *g, float *b)
{
	float vc = (float)v / 255.0f;
	
	*r = vc * colorSplit;
	*g = vc * colorSplit;
	*b = vc * colorSplit;
	
	float vc2 = vc * 0.33f;
	*r += vc2;
	*g += vc2;
	*b += vc2;
}

static float colorForValueR[256];
static float colorForValueG[256];
static float colorForValueB[256];

void C64DebuggerComputeMemoryMapColorTables(uint8 memoryValuesStyle)
{
	if (memoryValuesStyle == MEMORY_MAP_VALUES_STYLE_RGB)
	{
		for (int v = 0; v < 256; v++)
		{
			ComputeColorFromValueStyleRGB(v, &(colorForValueR[v]), &(colorForValueG[v]), &(colorForValueB[v]));
		}
	}
	else if (memoryValuesStyle == MEMORY_MAP_VALUES_STYLE_BLACK)
	{
		for (int v = 0; v < 256; v++)
		{
			colorForValueR[v] = 0; colorForValueG[v] = 0; colorForValueB[v] = 0;
		}
	}
	else //MEMORY_MAP_VALUES_STYLE_GRAYSCALE
	{
		for (int v = 0; v < 256; v++)
		{
			ComputeColorFromValueStyleGrayscale(v, &(colorForValueR[v]), &(colorForValueG[v]), &(colorForValueB[v]));
		}
	}
}


inline void ColorFromValue(u8 v, float *r, float *g, float *b, float *a)
{
	*a = alphaSplit;
	*r = colorForValueR[v];
	*g = colorForValueG[v];
	*b = colorForValueB[v];
}


///
void _markCellReadStyleDebugger(CDebugMemoryCell *cell)
{
	cell->sg = 0.3f;
	cell->sb = 1.0f;
	cell->sa = 1.0f;
}

void _markCellWriteStyleDebugger(CDebugMemoryCell *cell)
{
	cell->sr = 1.0f;
	cell->sa = 1.0f;
}

void _markCellExecuteCodeStyleDebugger(CDebugMemoryCell *cell)
{
	cell->sr = 0.0f;
	cell->sg = 1.0f;
	cell->sb = 1.0f;
	cell->sa = 1.0f;
}

void _markCellExecuteArgumentStyleDebugger(CDebugMemoryCell *cell)
{
	cell->sr = 0.0f;
	cell->sg = 0.3f;
	cell->sb = 1.0f;
	cell->sa = 1.0f;
}

void _markCellReadStyleICU(CDebugMemoryCell *cell)
{
	cell->sg = 1.0f;
	cell->sa = 1.0f;
}

void _markCellWriteStyleICU(CDebugMemoryCell *cell)
{
	cell->sr = 1.0f;
	cell->sa = 1.0f;
}

void _markCellExecuteCodeStyleICU(CDebugMemoryCell *cell)
{
	cell->sr = 0.0f;
	cell->sg = 1.0f;
	cell->sb = 1.0f;
	cell->sa = 1.0f;
}

void _markCellExecuteArgumentStyleICU(CDebugMemoryCell *cell)
{
	cell->sr = 0.0f;
	cell->sg = 1.0f;
	cell->sb = 0.0f;
	cell->sa = 1.0f;
}

typedef void (*MarkCellReadFunc)(CDebugMemoryCell *cell);
typedef void (*MarkCellWriteFunc)(CDebugMemoryCell *cell);
typedef void (*MarkCellExecuteCodeFunc)(CDebugMemoryCell *cell);
typedef void (*MarkCellExecuteArgumentFunc)(CDebugMemoryCell *cell);

MarkCellReadFunc markCellRead = _markCellReadStyleDebugger;
MarkCellWriteFunc markCellWrite = _markCellWriteStyleDebugger;
MarkCellExecuteCodeFunc markCellExecuteCode = _markCellExecuteCodeStyleDebugger;
MarkCellExecuteArgumentFunc markCellExecuteArgument = _markCellExecuteArgumentStyleDebugger;

void C64DebuggerSetMemoryMapMarkersStyle(uint8 memoryMapMarkersStyle)
{
	float darken = 0.5f;
	if (memoryMapMarkersStyle == MEMORY_MAP_MARKER_STYLE_DEFAULT)
	{
		markCellRead = _markCellReadStyleDebugger;
		markCellWrite = _markCellWriteStyleDebugger;
		markCellExecuteCode = _markCellExecuteCodeStyleDebugger;
		markCellExecuteArgument = _markCellExecuteArgumentStyleDebugger;
		
		colorExecuteCodeR = 0.0f * darken;
		colorExecuteCodeG = 1.0f * darken;
		colorExecuteCodeB = 1.0f * darken;
		colorExecuteCodeA = 1.0f;
		
		colorExecuteArgumentR = 0.0f * darken;
		colorExecuteArgumentG = 0.0f * darken;
		colorExecuteArgumentB = 1.0f * darken;
		colorExecuteArgumentA = 1.0f;

	}
	else if (memoryMapMarkersStyle == MEMORY_MAP_MARKER_STYLE_ICU)
	{
		markCellRead = _markCellReadStyleICU;
		markCellWrite = _markCellWriteStyleICU;
		markCellExecuteCode = _markCellExecuteCodeStyleICU;
		markCellExecuteArgument = _markCellExecuteArgumentStyleICU;

		colorExecuteCodeR = 0.0f * darken;
		colorExecuteCodeG = 1.0f * darken;
		colorExecuteCodeB = 1.0f * darken;
		colorExecuteCodeA = 1.0f;
		
		colorExecuteArgumentR = 0.0f * darken;
		colorExecuteArgumentG = 1.0f * darken;
		colorExecuteArgumentB = 0.0f * darken;
		colorExecuteArgumentA = 1.0f;
	}
	
	float fce = 0.65f;
	colorExecuteCodeR2 = colorExecuteCodeR * fce;
	colorExecuteCodeG2 = colorExecuteCodeG * fce;
	colorExecuteCodeB2 = colorExecuteCodeB * fce;
	colorExecuteCodeA2 = 1.0f; //colorExecuteCodeA * fce;
	
	colorExecuteArgumentR2 = colorExecuteArgumentR * fce;
	colorExecuteArgumentG2 = colorExecuteArgumentG * fce;
	colorExecuteArgumentB2 = colorExecuteArgumentB * fce;
	colorExecuteArgumentA2 = 1.0f; //colorExecuteArgumentA * fce;

}


CDebugMemoryCell::CDebugMemoryCell(int addr)
{
	this->addr = addr;
	
	sr = 0.0f; vr = 0.0f; rr = 0.0f;
	sg = 0.0f; vg = 0.0f; rg = 0.0f;
	sb = 0.0f; vb = 0.0f; rb = 0.0f;
	sa = 0.0f; va = 0.0f; ra = 0.0f;
	
	isExecuteCode = false;
	isExecuteArgument = false;
	
	isRead = false;
	isWrite = false;

	writePC = writeRasterCycle = writeRasterLine = -1;
	readPC  = readRasterCycle  = readRasterLine = -1;
	writeCycle = readCycle = executeCycle = -1;
	writeFrame = readFrame = executeFrame = -1;
}

CDebugMemoryCell::~CDebugMemoryCell()
{
}

void CDebugMemoryCell::ClearDebugMarkers()
{
	isExecuteCode = false;
	isExecuteArgument = false;
	isRead = false;
	isWrite = false;

	sr = 0.0f; vr = 0.0f; rr = 0.0f;
	sg = 0.0f; vg = 0.0f; rg = 0.0f;
	sb = 0.0f; vb = 0.0f; rb = 0.0f;
	sa = 0.0f; va = 0.0f; ra = 0.0f;
}

void CDebugMemoryCell::ClearReadWriteDebugMarkers()
{
	isRead = false;
	isWrite = false;

	sr = 0.0f; vr = 0.0f; rr = 0.0f;
	sg = 0.0f; vg = 0.0f; rg = 0.0f;
	sb = 0.0f; vb = 0.0f; rb = 0.0f;
	sa = 0.0f; va = 0.0f; ra = 0.0f;
	
	writePC = writeRasterCycle = writeRasterLine = -1;
	readPC  = readRasterCycle  = readRasterLine = -1;
	writeCycle = readCycle = executeCycle = -1;
}

void CDebugMemoryCell::MarkCellRead()
{
	markCellRead(this);
	isRead = true;
}

void CDebugMemoryCell::MarkCellWrite(uint8 value)
{
	//LOGTODO("remove argument marker based on previous code length");
	
	isExecuteCode = false;
	isExecuteArgument = false;
	isWrite = true;
	markCellWrite(this);
}

void CDebugMemoryCell::MarkCellExecuteCode(uint8 opcode)
{
	isExecuteCode = true;
	markCellExecuteCode(this);
}

void CDebugMemoryCell::MarkCellExecuteArgument()
{
	isExecuteArgument = true;
	markCellExecuteArgument(this);
}

void CDebugMemoryCell::UpdateCellColors(u8 v, bool showExecutePC, int pc)
{
	ColorFromValue(v, &vr, &vg, &vb, &va);

	if (showExecutePC && addr == pc)
	{
		sr = 1.0f;
		sg = 1.0f;
		sb = 1.0f;
		sa = 1.0f;
	}
	
	if (isExecuteCode)
	{
		rr = sr + colorExecuteCodeR2;// + vr;
		rg = sg + colorExecuteCodeG2;// + vg;
		rb = sb + colorExecuteCodeB2;// + vb;
		ra = sa + colorExecuteCodeA2;// + va;
	}
	else if (isExecuteArgument)
	{
		rr = sr + colorExecuteArgumentR2;// + vr;
		rg = sg + colorExecuteArgumentG2;// + vg;
		rb = sb + colorExecuteArgumentB2;// + vb;
		ra = sa + colorExecuteArgumentA2;// + va;
	}
	else
	{
		rr = vr + sr;
		rg = vg + sg;
		rb = vb + sb;
		ra = va + sa;
	}
	
	if (rr > 1.0) rr = 1.0;
	if (rg > 1.0) rg = 1.0;
	if (rb > 1.0) rb = 1.0;
	if (ra > 1.0) ra = 1.0;
}
