#ifndef _CDebugEventsHistory_h_
#define _CDebugEventsHistory_h_

#include "SYS_Defs.h"
#include <map>

class CDebugSymbols;
class CDebugSymbolsSegment;
class CDebugBreakpoint;
class CSlrMutex;

enum
{
	DEBUG_EVENT_TYPE_NONE,
	DEBUG_EVENT_TYPE_BREAKPOINT,
	DEBUG_EVENT_TYPE_MANUAL
};

#define DEBUG_EVENTS_HISTORY_MAX_COMMENT_LENGTH	512

class CDebugEvent
{
public:
	CDebugEvent(u8 eventType, CDebugSymbolsSegment *segment, u64 cycle, u32 frame);
	virtual ~CDebugEvent();
	
	u8 eventType;
	CDebugSymbolsSegment *segment;
	u64 cycle;
	u32 frame;
	char comment[DEBUG_EVENTS_HISTORY_MAX_COMMENT_LENGTH];
};

class CDebugEventBreakpoint : public CDebugEvent
{
public:
	CDebugEventBreakpoint(CDebugBreakpoint *breakpoint, u32 condition, CDebugSymbolsSegment *segment, u64 cycle, u32 frame);

	CDebugBreakpoint *breakpoint;
	u32 condition;	// additional condition (if available), eg. MEMORY_BREAKPOINT_ACCESS_READ
};

class CDebugEventManual : public CDebugEvent
{
public:
	CDebugEventManual(CDebugSymbolsSegment *segment, u64 cycle, u32 frame);
};

class CDebugEventsHistory
{
public:
	CDebugEventsHistory(CDebugSymbols *debugSymbols);
	~CDebugEventsHistory();
	
	CSlrMutex *mutex;
	CDebugSymbols *debugSymbols;
	
	// cycle, CDebugEvent
	std::map<u64, CDebugEvent *> events;
	
	CDebugEventBreakpoint *CreateEventBreakpoint(CDebugBreakpoint *breakpoint, u32 condition, CDebugSymbolsSegment *segment);
	CDebugEventManual *CreateEventManual();
	void DeleteEvent(CDebugEventBreakpoint *event);
	
	void DeleteAllEvents();
};


#endif
