#include "CDebugBreakpointData.h"
#include "CDebugBreakpointsData.h"

CDebugBreakpointData::CDebugBreakpointData(int addr,
									u32 dataAccess, DataBreakpointComparison comparison, int value)
: CDebugBreakpointAddr(NULL, addr)
{
	this->breakpointType = BREAKPOINT_TYPE_DATA;
	this->dataAccess = dataAccess;
	this->value = value;
	this->comparison = comparison;
}

CDebugBreakpointData::CDebugBreakpointData(CDebugSymbols *debugSymbols, int addr,
									u32 dataAccess, DataBreakpointComparison comparison, int value)
: CDebugBreakpointAddr(debugSymbols, addr)
{
	this->breakpointType = BREAKPOINT_TYPE_DATA;
	this->dataAccess = dataAccess;
	this->value = value;
	this->comparison = comparison;
}

void CDebugBreakpointData::Serialize(Hjson::Value hjsonRoot)
{
	CDebugBreakpointAddr::Serialize(hjsonRoot);
	
	hjsonRoot["MemoryAccess"] = dataAccess;
	hjsonRoot["Value"] = value;
	hjsonRoot["Comparison"] = (int)comparison;
}

void CDebugBreakpointData::Deserialize(Hjson::Value hjsonRoot)
{
	CDebugBreakpointAddr::Deserialize(hjsonRoot);
	
	dataAccess = hjsonRoot["MemoryAccess"];
	value = hjsonRoot["Value"];
	comparison = (DataBreakpointComparison) ((int)hjsonRoot["Comparison"]);
}

void CDebugBreakpointData::GetDetailsJson(nlohmann::json &j)
{
	CDebugBreakpoint::GetDetailsJson(j);
	j["type"] = "data";
//	j["triggerValue"] = value;
//	j["triggerComparison"] = CDebugBreakpointsData::DataBreakpointComparisonToStr(comparison);
}
