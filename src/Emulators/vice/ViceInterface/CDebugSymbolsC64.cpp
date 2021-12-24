#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegmentC64.h"

CDebugSymbolsC64::CDebugSymbolsC64(CDebugInterface *debugInterface, CDataAdapter *dataAdapter)
: CDebugSymbols(debugInterface, dataAdapter)
{
	LOGD("CDebugSymbolsC64");
};

CDebugSymbolsSegment *CDebugSymbolsC64::CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum)
{
	LOGD("CDebugSymbolsC64::CreateNewDebugSymbolsSegment");
	CDebugSymbolsSegmentC64 *segment = new CDebugSymbolsSegmentC64(this, name, segmentNum);
	segment->Init();
	
	return segment;
}
