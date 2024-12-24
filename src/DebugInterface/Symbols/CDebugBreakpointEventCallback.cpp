#include "CDebugBreakpointEventCallback.h"

bool CDebugBreakpointEventCallback::DebugBreakpointEvaluateCallback(CDebugBreakpoint *breakpoint)
{
	//	Note, this return allows plugins to set breakpoints that will not cause code to stop.
	//	true = means that the breakpoint is confirmed, so the mechanism will perform an action, such as pausing the code.
	//	false = means that the breakpoint does not need to be processed, and the mechanism will not perform any breakpoint-specific actions.

	return true;
}
