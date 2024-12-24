#ifndef _CDebugBreakpointAddr_h_
#define _CDebugBreakpointAddr_h_

#include "CDebugBreakpoint.h"

class CDebugBreakpointAddr : public CDebugBreakpoint
{
public:
	CDebugBreakpointAddr(int addr);
	CDebugBreakpointAddr(CDebugSymbols *debugSymbols, int addr);
	virtual ~CDebugBreakpointAddr();
	
	bool isActive;

	int addr;
	u32 actions;
	u8 data;
	
	virtual void Serialize(Hjson::Value hjsonRoot);
	virtual void Deserialize(Hjson::Value hjsonRoot);
	
	virtual void GetDetailsJson(nlohmann::json &j);
};


#endif
//_CDebugBreakpoint_h_
