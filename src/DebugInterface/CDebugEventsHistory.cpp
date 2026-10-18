#include "CDebugEventsHistory.h"
#include "CDebugSymbols.h"
#include "CDebugInterface.h"
#include "SYS_Threading.h"
#include <cstring>

CDebugEventsHistory::CDebugEventsHistory(CDebugSymbols *debugSymbols)
{
	this->debugSymbols = debugSymbols;
	mutex = new CSlrMutex("CDebugEventsHistory");
}

CDebugEventBreakpoint *CDebugEventsHistory::CreateEventBreakpoint(CDebugBreakpoint *breakpoint, u32 condition, CDebugSymbolsSegment *segment)
{
	u64 currentCycle = debugSymbols->debugInterface->GetMainCpuCycleCounter();

	// check if already exists
	std::map<u64, CDebugEvent *>::iterator itEvent = events.find(currentCycle);
	if (itEvent != events.end())
	{
		CDebugEvent *event = itEvent->second;
		if (event->eventType == DEBUG_EVENT_TYPE_BREAKPOINT)
		{
			return (CDebugEventBreakpoint*)event;
		}
		return NULL;
	}
	
	u32 currentFrame = debugSymbols->debugInterface->GetEmulationFrameNumber();
	
	CDebugEventBreakpoint *event = new CDebugEventBreakpoint(breakpoint, condition, segment, currentCycle, currentFrame);

	mutex->Lock();
	events[currentCycle] = event;
	mutex->Unlock();
	
	return event;
}

CDebugEventManual *CDebugEventsHistory::CreateEventManual()
{
	u64 currentCycle = debugSymbols->debugInterface->GetMainCpuCycleCounter();

	// check if already exists
	std::map<u64, CDebugEvent *>::iterator itEvent = events.find(currentCycle);
	if (itEvent != events.end())
	{
		CDebugEvent *event = itEvent->second;
		if (event->eventType == DEBUG_EVENT_TYPE_MANUAL)
		{
			return (CDebugEventManual*)event;
		}
		return NULL;
	}
	
	u32 currentFrame = debugSymbols->debugInterface->GetEmulationFrameNumber();
	
	CDebugEventManual *event = new CDebugEventManual(debugSymbols->currentSegment, currentCycle, currentFrame);

	mutex->Lock();
	events[currentCycle] = event;
	mutex->Unlock();
	
	return event;
}

CDebugEventStep *CDebugEventsHistory::CreateEventStep(const char *description)
{
	u64 currentCycle = debugSymbols->debugInterface->GetMainCpuCycleCounter();
	u32 currentFrame = debugSymbols->debugInterface->GetEmulationFrameNumber();

	mutex->Lock();
	CDebugEventStep *event = NULL;
	if (events.find(currentCycle) == events.end())
	{
		event = new CDebugEventStep(description, debugSymbols->currentSegment, currentCycle, currentFrame);
		events[currentCycle] = event;
	}
	mutex->Unlock();

	return event;
}

void CDebugEventsHistory::ForgetSegment(CDebugSymbolsSegment *segment)
{
	mutex->Lock();
	for (auto &it : events)
	{
		CDebugEvent *event = it.second;
		if (event->segment != segment)
			continue;
		event->segment = NULL;
		if (event->eventType == DEBUG_EVENT_TYPE_BREAKPOINT)
			((CDebugEventBreakpoint *)event)->breakpoint = NULL;
	}
	mutex->Unlock();
}

void CDebugEventsHistory::DeleteEvent(CDebugEventBreakpoint *event)
{
	mutex->Lock();
	events.erase(event->cycle);
	delete event;
	mutex->Unlock();
}

CDebugEvent::CDebugEvent(u8 eventType, CDebugSymbolsSegment *segment, u64 cycle, u32 frame)
{
	this->eventType = eventType;
	this->segment = segment;
	this->cycle = cycle;
	this->frame = frame;
	this->comment[0] = 0;
}

CDebugEvent::~CDebugEvent()
{
}
	
CDebugEventBreakpoint::CDebugEventBreakpoint(CDebugBreakpoint *breakpoint, u32 condition, CDebugSymbolsSegment *segment, u64 cycle, u32 frame)
: CDebugEvent(DEBUG_EVENT_TYPE_BREAKPOINT, segment, cycle, frame)
{
	this->breakpoint = breakpoint;
	this->condition = condition;
}

CDebugEventManual::CDebugEventManual(CDebugSymbolsSegment *segment, u64 cycle, u32 frame)
: CDebugEvent(DEBUG_EVENT_TYPE_MANUAL, segment, cycle, frame)
{
}

CDebugEventStep::CDebugEventStep(const char *description, CDebugSymbolsSegment *segment, u64 cycle, u32 frame)
: CDebugEvent(DEBUG_EVENT_TYPE_STEP, segment, cycle, frame)
{
	strncpy(this->description, description, DEBUG_EVENTS_HISTORY_MAX_DESCRIPTION_LENGTH - 1);
	this->description[DEBUG_EVENTS_HISTORY_MAX_DESCRIPTION_LENGTH - 1] = 0;
}

void CDebugEventsHistory::DeleteAllEvents()
{
	mutex->Lock();
	while(!events.empty())
	{
		CDebugEvent *event = events.begin()->second;
		events.erase(event->cycle);
		delete event;
	}
	mutex->Unlock();
}

CDebugEventsHistory::~CDebugEventsHistory()
{
	DeleteAllEvents();
}

