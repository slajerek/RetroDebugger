#ifndef _CDebugVariableResolver_h_
#define _CDebugVariableResolver_h_

#include "SYS_Defs.h"
#include "CDebugDatabase.h"
#include <string>
#include <vector>

class CDebugInterface;

// RD-022 (RESEARCH-c-symbols.md #6): a C variable at a PC -- where it is, how
// much may be claimed about it, and its bytes. Only the innermost frame's
// locals are resolved: no producer records an outer frame's stack pointer.
struct CDebugVariableResolution
{
	const CDebugVariable *variable = NULL;
	std::string name;
	std::string function;			// "" for a global
	std::string type;				// "u8", "i16", "u8*", "u8[4]", "?"
	bool hasAddress = false;
	u32 address = 0;
	u32 size = 0;
	DebugMappingQuality quality = DebugMappingQuality::Missing;
	std::string reason;				// why it is not Exact; "" when it is
	std::vector<u8> bytes;			// read through the CPU view, up to 64
};

const char *DebugVariableLocationName(DebugVariableLocation location);
std::string DebugTypeName(const CDebugDatabase *database, u32 typeId);

// NAME at PC: the PC's function's local first, then a global. Missing with
// a reason when there is no such variable.
CDebugVariableResolution DebugResolveVariable(const CDebugDatabase *database, CDebugInterface *debugInterface,
											  u32 regionId, const std::string &name, u16 pc);

// Every variable visible at PC: the PC's function's locals, then the globals.
std::vector<CDebugVariableResolution> DebugVariablesInScope(const CDebugDatabase *database, CDebugInterface *debugInterface,
															u32 regionId, u16 pc);

#endif
