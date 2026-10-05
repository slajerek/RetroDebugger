#ifndef _CDebugMemoryMapCell_h_
#define _CDebugMemoryMapCell_h_

#include "SYS_Defs.h"
#include "M_Circlebuf.h"
#include <mutex>

// CPU-address markers, not bank-aware physical-memory provenance.
struct DebugMemoryCellAccess
{
	int pc;
	u64 cycle;
	u32 frame;
	int rasterLine;
	int rasterCycle;

	bool IsAvailable() const { return pc >= 0 && pc <= 0xFFFF && cycle != (u64)-1; }
};

struct DebugMemoryCellLastAccess
{
	DebugMemoryCellAccess read;
	DebugMemoryCellAccess write;
};

struct DebugMemoryCellHistoryValue
{
	u64 cycle;
	u32 frame;
	int pc;
	u8 value;
};
// sizeof is 25

struct DebugMemoryCellHistoryEvent
{
	u64 cycle;
	u32 frame;
};
// sizeof is 12

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
	void MarkCellRead(u64 readCycle, u32 readFrame, int readPC, int readRasterLine, int readRasterCycle);
	// Copy both records under the same lock used by their writers and clear path.
	DebugMemoryCellLastAccess GetLastAccess();
	void MarkCellWrite(uint8 value);
	void MarkCellWrite(uint8 value, u64 writeCycle, u32 writeFrame, int writePC, int writeRasterLine, int writeRasterCycle);
	void MarkCellExecuteCode(uint8 opcode, u64 executeCycle, u32 executeFrame);
	void MarkCellExecuteArgument();
	
	void ClearEventsAfterCycle(u64 cycle);
	
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
	
	// history
	
	// previous values: cycle, frame and value
	struct circlebuf valuesHistory;
	
	// previous executes: cycle, frame (i.e. when this address was previously executed)
	struct circlebuf executeHistory;

private:
	// Emulator access hooks run outside CDebugInterface's general mutex.
	// Legacy GUI fields remain public; new remote readers use GetLastAccess().
	std::mutex readWriteAccessMutex;
};


#endif

