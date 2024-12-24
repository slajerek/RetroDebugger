#ifndef _CDebugBreakpointsData_h_
#define _CDebugBreakpointsData_h_

#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointData.h"

class CDebugBreakpointsData : public CDebugBreakpointsAddr
{
public:
	CDebugBreakpointsData(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, const char *addressFormatStr, int minAddr, int maxAddr);
	~CDebugBreakpointsData();

	// factory
	virtual CDebugBreakpoint *CreateEmptyBreakpoint();

	virtual CDebugBreakpointData *EvaluateBreakpoint(int addr, int value, u32 memoryAccess);

	virtual void RenderImGui();
	
	static const char *DataBreakpointComparisonToStr(DataBreakpointComparison comparison);
	static DataBreakpointComparison StrToDataBreakpointComparison(const char *comparisonStr);

protected:
	int addBreakpointPopupValue;
	int addBreakpointPopupComparisonMethod;
};

#endif

