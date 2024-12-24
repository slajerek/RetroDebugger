#include "CDebugBreakpointRasterLine.h"

CDebugBreakpointRasterLine::CDebugBreakpointRasterLine(int rasterLine)
: CDebugBreakpointAddr(rasterLine)
{
	this->breakpointType = BREAKPOINT_TYPE_RASTER_LINE;
}

CDebugBreakpointRasterLine::CDebugBreakpointRasterLine(CDebugSymbols *debugSymbols, int rasterLine)
: CDebugBreakpointAddr(debugSymbols, rasterLine)
{
	this->breakpointType = BREAKPOINT_TYPE_RASTER_LINE;
}

void CDebugBreakpointRasterLine::GetDetailsJson(nlohmann::json &j)
{
	CDebugBreakpoint::GetDetailsJson(j);
	j["type"] = "rasterLine";
//	j["triggerRasterLine"] = addr;
}

