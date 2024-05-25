#ifndef _CDebugMemoryMapCell_h_
#define _CDebugMemoryMapCell_h_

#include "SYS_Defs.h"

class CDebugMemoryCell
{
public:
	CDebugMemoryCell(int addr);
	virtual ~CDebugMemoryCell();
	
	// cell address (for virtual maps, like Drive 1541 map)
	int addr;
	
	// read/write colors
	float sr, sg, sb, sa;
	
	// value color
	float vr, vg, vb, va;
	
	// render color (s+v)
	float rr, rg, rb, ra;

	//volatile
	bool isExecuteCode;
	//volatile
	bool isExecuteArgument;
	
	bool isRead;
	bool isWrite;
	
	void MarkCellRead();
	void MarkCellWrite(uint8 value);
	void MarkCellExecuteCode(uint8 opcode);
	void MarkCellExecuteArgument();
	
	void ClearDebugMarkers();
	void ClearReadWriteDebugMarkers();
	
	void UpdateCellColors(u8 v, bool showExecutePC, int pc);
	
	// last write PC & raster (where was PC & raster when cell was written)
	int writePC;
	int writeRasterLine, writeRasterCycle;
	u64 writeCycle;
	u32 writeFrame;

	// last read PC & raster (where was PC & raster when cell was read)
	int readPC;
	int readRasterLine, readRasterCycle;
	u64 readCycle;
	u32 readFrame;

	// last execute cycle
	u64 executeCycle;
	u32 executeFrame;
};


#endif

