#ifndef _CDebugMemoryMap_h_
#define _CDebugMemoryMap_h_

#include "SYS_Defs.h"

class CDebugInterface;
class CDataAdapter;
class CImageData;
class CSlrImage;
class CViewDataDump;
class CDebugSymbols;
class CDebugMemoryCell;
class CSlrFont;

// alloc with safe margin to avoid comparison in cells marking (it is quicker)
#define DATA_CELLS_PADDING_LENGTH	0x0200

#define MEMORY_MAP_VALUES_STYLE_RGB			0
#define MEMORY_MAP_VALUES_STYLE_GRAYSCALE	1
#define MEMORY_MAP_VALUES_STYLE_BLACK		2

void C64DebuggerComputeMemoryMapColorTables(uint8 memoryValuesStyle);

#define MEMORY_MAP_MARKER_STYLE_DEFAULT		0
#define MEMORY_MAP_MARKER_STYLE_ICU			1

void C64DebuggerSetMemoryMapMarkersStyle(uint8 memoryMapMarkersStyle);

void C64DebuggerSetMemoryMapCellsFadeSpeed(float fadeSpeed);

extern float colorExecuteCodeR;
extern float colorExecuteCodeG;
extern float colorExecuteCodeB;
extern float colorExecuteCodeA;

extern float colorExecuteArgumentR;
extern float colorExecuteArgumentG;
extern float colorExecuteArgumentB;
extern float colorExecuteArgumentA;

extern float colorExecuteCodeR2;
extern float colorExecuteCodeG2;
extern float colorExecuteCodeB2;
extern float colorExecuteCodeA2;

extern float colorExecuteArgumentR2;
extern float colorExecuteArgumentG2;
extern float colorExecuteArgumentB2;
extern float colorExecuteArgumentA2;

class CDebugMemory
{
public:
	CDebugMemory(CDebugSymbols *debugSymbols);
	
	CDebugInterface *debugInterface;
	CDebugSymbols *debugSymbols;
	CDataAdapter *dataAdapter;
	
	CDebugMemoryCell *GetMemoryCell(int address);

	bool IsExecuteCodeAddress(int address);
	void ClearDebugMarkers();
	void ClearReadWriteDebugMarkers();

	void CellRead(int addr);
	void CellRead(int addr, int pc, int rasterX, int rasterY);
	void CellWrite(int addr, uint8 value);
	void CellWrite(int addr, uint8 value, int pc, int rasterX, int rasterY);
	void CellExecute(int addr, uint8 opcode);

private:
	CDebugMemoryCell **memoryCells;
	int numCells;
};

#endif
