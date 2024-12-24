#ifndef _CDebugBreakpointEventCallback_h_
#define _CDebugBreakpointEventCallback_h_

#include "CDebugBreakpoint.h"

class CDebugBreakpoint;

class CDebugBreakpointEventCallback
{
public:
	// returns if we should skip evaluation of breakpoint (i.e. stop code running)
	virtual bool DebugBreakpointEvaluateCallback(CDebugBreakpoint *breakpoint);
};

#endif
