#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegmentC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbolsDrive1541.h"

CDebugSymbolsC64::CDebugSymbolsC64(CDebugInterface *debugInterface)
: CDebugSymbols(debugInterface, true)
{
	LOGD("CDebugSymbolsC64");
};

CDebugSymbolsSegment *CDebugSymbolsC64::CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum)
{
	LOGD("CDebugSymbolsC64::CreateNewDebugSymbolsSegment");
	name->DebugPrint();
	CDebugSymbolsSegmentC64 *segment = new CDebugSymbolsSegmentC64(this, name, segmentNum);
	segment->Init();
	
	return segment;
}

CDebugSymbols *CDebugSymbolsC64::GetSymbolsForDevice(int deviceId)
{
	if (deviceId == C64_SYMBOL_DEVICE_DRIVE1541)
	{
		// symbolsDrive1541 is a CDebugInterfaceC64 member (NULL on backends without a 1541)
		return ((CDebugInterfaceC64 *)debugInterface)->symbolsDrive1541;
	}
	return CDebugSymbols::GetSymbolsForDevice(deviceId);
}
