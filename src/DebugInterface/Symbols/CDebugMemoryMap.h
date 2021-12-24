#ifndef _CDebugMemoryMap_h_
#define _CDebugMemoryMap_h_

#include "SYS_Defs.h"

class CDebugInterface;
class CDataAdapter;
class CImageData;
class CSlrImage;
class CViewDataDump;
class CDebugInterface;
class CSlrFont;

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

#endif
