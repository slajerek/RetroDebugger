#include "CDebugBreakpointsRasterLine.h"
#include "CDebugBreakpointRasterLine.h"

CDebugBreakpointsRasterLine::CDebugBreakpointsRasterLine(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, const char *addressFormatStr, int minAddr, int maxAddr)
: CDebugBreakpointsAddr(breakpointType, breakpointTypeStr, segment, addressFormatStr, minAddr, maxAddr)
{
}

CDebugBreakpoint *CDebugBreakpointsRasterLine::CreateEmptyBreakpoint()
{
	return new CDebugBreakpointRasterLine(symbols, 0);
}
