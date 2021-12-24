#include "DBG_Log.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsSegmentDrive1541.h"

CDebugSymbolsDrive1541::CDebugSymbolsDrive1541(CDebugInterface *debugInterface, CDataAdapter *dataAdapter)
: CDebugSymbols(debugInterface, dataAdapter)
{
	LOGD("CDebugSymbolsDrive1541");
};

// get currently executed addr (PC) to be checked against breakpoint, note it is virtual to have it different for C64 Drive CPU
int CDebugSymbolsDrive1541::GetCurrentExecuteAddr()
{
	return ((CDebugInterfaceC64 *)debugInterface)->GetDrive1541PC();
}

CDebugSymbolsSegment *CDebugSymbolsDrive1541::CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum)
{
	LOGD("CDebugSymbolsDrive1541::CreateNewDebugSymbolsSegment");
	
	CDebugSymbolSegmentDrive1541 *segment = new CDebugSymbolSegmentDrive1541(this, name, segmentNum);
	segment->Init();
	return segment;
}

