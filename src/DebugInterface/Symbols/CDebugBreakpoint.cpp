#include "CDebugBreakpoint.h"
#include "CDebugSymbols.h"
#include "CDebugInterface.h"

u64 CDebugBreakpoint::lastBreakpointId = 0;

CDebugBreakpoint::CDebugBreakpoint(CDebugSymbols *debugSymbols)
: symbols(debugSymbols)
{
	callback = NULL;
	breakpointId = ++lastBreakpointId;
	breakpointType = BREAKPOINT_TYPE_UNKNOWN;
}

CDebugBreakpoint::~CDebugBreakpoint()
{
}

void CDebugBreakpoint::Serialize(Hjson::Value hjsonRoot)
{
}

void CDebugBreakpoint::Deserialize(Hjson::Value hjsonRoot)
{
}

const char *CDebugBreakpoint::GetPlatformNameEndpointString()
{
	return symbols->debugInterface->GetPlatformNameEndpointString();
}

void CDebugBreakpoint::GetDetailsJson(nlohmann::json &j)
{
	j["breakpointId"] = breakpointId;
	j["platform"] = GetPlatformNameEndpointString();
}
