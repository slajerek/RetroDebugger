#ifndef _CDebugBreakpointData_h_
#define _CDebugBreakpointData_h_

#include "CDebugBreakpointAddr.h"

// TODO: refactor this to 2 breakpoints (make list?)
class CDebugBreakpointData : public CDebugBreakpointAddr
{
public:
	CDebugBreakpointData(int addr,
					  u32 dataAccess, DataBreakpointComparison comparison, int value);
	CDebugBreakpointData(CDebugSymbols *symbols, int addr,
					  u32 dataAccess, DataBreakpointComparison comparison, int value);
	
	u32 dataAccess;
	int value;
	DataBreakpointComparison comparison;
	
	virtual void Serialize(Hjson::Value hjsonRoot);
	virtual void Deserialize(Hjson::Value hjsonRoot);

	virtual void GetDetailsJson(nlohmann::json &j);
};

#endif
//_CDebugBreakpoint_h_
