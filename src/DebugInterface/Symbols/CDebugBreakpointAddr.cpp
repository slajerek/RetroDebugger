#include "CDebugBreakpointAddr.h"

CDebugBreakpointAddr::CDebugBreakpointAddr(int addr)
: CDebugBreakpoint(NULL)
{
	this->breakpointType = BREAKPOINT_TYPE_ADDR;
	this->isActive = true;
	this->addr = addr;
	this->actions = ADDR_BREAKPOINT_ACTION_STOP;
	this->data = 0x00;
}

CDebugBreakpointAddr::CDebugBreakpointAddr(CDebugSymbols *debugSymbols, int addr)
: CDebugBreakpoint(debugSymbols)
{
	this->breakpointType = BREAKPOINT_TYPE_ADDR;
	this->isActive = true;
	this->addr = addr;
	this->actions = ADDR_BREAKPOINT_ACTION_STOP;
	this->data = 0x00;
}

void CDebugBreakpointAddr::Serialize(Hjson::Value hjsonRoot)
{
	hjsonRoot["IsActive"] = isActive;
	hjsonRoot["Addr"] = addr;
	hjsonRoot["Actions"] = actions;
	hjsonRoot["Data"] = data;
}

void CDebugBreakpointAddr::Deserialize(Hjson::Value hjsonRoot)
{
	isActive = (bool)hjsonRoot["IsActive"];
	addr = hjsonRoot["Addr"];
	actions = hjsonRoot["Actions"];
	data = hjsonRoot["Data"];
}

void CDebugBreakpointAddr::GetDetailsJson(nlohmann::json &j)
{
	CDebugBreakpoint::GetDetailsJson(j);
	j["type"] = "addr";
//	j["triggerAddr"] = addr;
}

CDebugBreakpointAddr::~CDebugBreakpointAddr()
{
}

