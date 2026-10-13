#pragma once

// Test-only comparisons shared by the real native fixture and portable negative
// controls.  These predicates do not service or simulate an emulator thread.
#include "CTimelineSeekTransaction.h"
#include <array>
#include <tuple>

namespace TimelineCancellationChecks
{
struct Observation
{
	TimelineSeek::Counter counter = 0, frame = 0;
	// PC, A, X, Y, P, SP, instruction cycle, and effective $0001.
	std::array<int, 8> cpu{};
	int rasterLine = 0, rasterCycle = 0, pendingPC = -1, model = -1;
	int border = 0, background = 0, display = 0, sprites = 0;
	std::array<unsigned char, 16> ram{};
	// Read/write records: PC, counter, frame, raster line, and raster cycle.
	std::array<std::uint64_t, 10> access{};
	bool paused = false, parked = false;
	std::string registersJson;
};

inline bool SameMachine(const Observation &a, const Observation &b)
{
	return std::tie(a.counter, a.frame, a.cpu, a.rasterLine, a.rasterCycle,
		a.pendingPC, a.model, a.border, a.background, a.display, a.sprites,
		a.ram, a.access, a.paused, a.parked, a.registersJson)
		== std::tie(b.counter, b.frame, b.cpu, b.rasterLine, b.rasterCycle,
		b.pendingPC, b.model, b.border, b.background, b.display, b.sprites,
		b.ram, b.access, b.paused, b.parked, b.registersJson);
}

inline bool PendingTimeout(const TimelineSeek::Result &r,
	TimelineSeek::Counter previous, bool needsService, bool active)
{
	return r.sequence > previous && r.outcome == "timeout_before_mutation"
		&& r.completed && !r.mutationStarted && !r.destinationResolved
		&& !r.contextAvailable && !r.actualAvailable && !r.sourceAccessAvailable
		&& !needsService && !active;
}

inline bool PositionMatches(const TimelineSeek::Position &p,
	const Observation &b, bool boundary)
{
	return p.counter == b.counter && p.frame == b.frame && p.pc == b.cpu[0]
		&& p.paused && p.instructionBoundary == boundary
		&& p.registersJson == b.registersJson;
}

inline bool Refusal(const TimelineSeek::Result &r, const char *outcome,
	TimelineSeek::Counter previous, const Observation &b, bool boundary)
{
	return r.sequence > previous && r.outcome == outcome && r.completed
		&& !r.mutationStarted && !r.destinationResolved && !r.sourceAccessAvailable
		&& r.contextAvailable && r.actualAvailable
		&& PositionMatches(r.origin, b, boundary) && PositionMatches(r.actual, b, boundary);
}

inline bool IdleResult(const TimelineSeek::Status &s, const TimelineSeek::Result &r,
	bool needsService, bool active)
{
	return !s.busy && !needsService && !active
		&& s.lastRequest.sequence == r.sequence && s.lastRequest.outcome == r.outcome
		&& s.lastRequest.completed && !s.lastRequest.mutationStarted;
}

inline bool Cleared(const TimelineSeek::Status &s)
{
	return !s.bounds.available && s.verifiedCheckpoints == 0
		&& !s.instructionCoverage && s.instructionCount == 0
		&& !s.atInstructionBoundary && !s.busy;
}
}
