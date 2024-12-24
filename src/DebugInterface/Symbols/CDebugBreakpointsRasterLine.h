#ifndef _CDebugBreakpointsRasterLine_h_
#define _CDebugBreakpointsRasterLine_h_

#include "CDebugBreakpointsAddr.h"

class CDebugBreakpointsRasterLine : public CDebugBreakpointsAddr
{
public:
	CDebugBreakpointsRasterLine(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, const char *addressFormatStr, int minAddr, int maxAddr);
	virtual CDebugBreakpoint *CreateEmptyBreakpoint();
};

#endif

