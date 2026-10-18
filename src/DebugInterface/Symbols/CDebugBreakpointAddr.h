#ifndef _CDebugBreakpointAddr_h_
#define _CDebugBreakpointAddr_h_

#include "CDebugBreakpoint.h"
#include "CDebugMemoryMapping.h"
#include <vector>

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

	// RD-025: when not empty, the breakpoint fires only when the byte the CPU
	// fetches at addr comes from one of these image file offsets (a source
	// breakpoint in one bank of a banked cartridge); empty: any bank
	std::vector<u64> imageOffsets;

	// RD-025: when not empty, the breakpoint fires only when the CPU's fetch
	// at addr resolves to one of these physical bytes (device, region,
	// offset): code in one 130XE bank, or in RAM under a ROM; empty: any
	std::vector<CDebugPhysicalAddress> physicalIdentities;
	
	virtual void Serialize(Hjson::Value hjsonRoot);
	virtual void Deserialize(Hjson::Value hjsonRoot);
	
	virtual void GetDetailsJson(nlohmann::json &j);
};


#endif
//_CDebugBreakpoint_h_
