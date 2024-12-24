#ifndef _CDebugBreakpoint_h_
#define _CDebugBreakpoint_h_

#include "SYS_Main.h"
#include "DebuggerDefs.h"
#include "hjson.h"
#include "json.hpp"
#include <map>

#define BREAKPOINT_TYPE_ADDR				0
#define BREAKPOINT_TYPE_DATA				1
#define BREAKPOINT_TYPE_RASTER_LINE			2
#define NUM_DEFAULT_BREAKPOINT_TYPES		3
#define BREAKPOINT_TYPE_UNKNOWN				0xFF

#define ADDR_BREAKPOINT_ACTION_STOP				BV01
#define ADDR_BREAKPOINT_ACTION_SET_BACKGROUND	BV02
#define ADDR_BREAKPOINT_ACTION_STOP_ON_RASTER	BV03

#define MEMORY_BREAKPOINT_ACCESS_WRITE			BV01
#define MEMORY_BREAKPOINT_ACCESS_READ			BV02

class CDebugSymbols;
class CDebugBreakpointEventCallback;

#define UNKNOWN_BREAKPOINT_ID	0

// generic breakpoint
class CDebugBreakpoint
{
public:
	CDebugBreakpoint(CDebugSymbols *debugSymbols);
	virtual ~CDebugBreakpoint();

	virtual void Serialize(Hjson::Value hjsonRoot);
	virtual void Deserialize(Hjson::Value hjsonRoot);

	u64 breakpointId;
	u8 breakpointType;
	
	CDebugSymbols *symbols;

	// return json for web service callback
	virtual void GetDetailsJson(nlohmann::json &j);
	virtual const char *GetPlatformNameEndpointString();
	
	CDebugBreakpointEventCallback *callback;
	
private:
	static u64 lastBreakpointId;
};

#endif
//_CDebugBreakpoint_h_
