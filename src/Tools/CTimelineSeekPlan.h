#ifndef _C_TIMELINE_SEEK_PLAN_H_
#define _C_TIMELINE_SEEK_PLAN_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace TimelineSeek
{
using Counter = std::uint64_t;

// These are different coordinate systems, even when an older API calls all
// three "cycles".  Only a verified elapsed CPU clock supports time conversion.
enum class ClockDomain { CpuClock, InstructionIndex, InstructionDebugClock };
enum class Mode { Counter, Frame, BackInstructions, BackVideoFrames, BackSeconds };
enum class Error
{
	None, InvalidCount, NoHistory, FutureTarget, HistoryUnavailable,
	NotAtInstructionBoundary, InstructionHistoryUnavailable,
	UnsupportedTimebase, Overflow
};

inline const char *ErrorName(Error error)
{
	switch (error)
	{
		case Error::None: return "ready";
		case Error::InvalidCount: return "invalid_count";
		case Error::NoHistory: return "no_history";
		case Error::FutureTarget: return "future_target";
		case Error::HistoryUnavailable: return "history_unavailable";
		case Error::NotAtInstructionBoundary: return "not_at_instruction_boundary";
		case Error::InstructionHistoryUnavailable: return "instruction_history_unavailable";
		case Error::UnsupportedTimebase: return "unsupported_timebase";
		case Error::Overflow: return "overflow";
	}
	return "unknown_error";
}

// Owned and synchronized by the snapshot manager.  The producer must observe
// every selected-main-CPU dispatch boundary while coverage is active, including
// the current pre-dispatch pause.  Sparse execution markers cannot populate it.
// Clear on a recording gap/reset; trim before replay; never count device CPUs.
class InstructionHistory
{
public:
	static constexpr std::size_t DefaultCapacity = 262144;
	explicit InstructionHistory(std::size_t capacity = DefaultCapacity)
		: values(capacity), begin(0), size(0), revision(0) {}

	void Clear()
	{
		begin = size = 0;
		++revision;
	}

	// A clock regression without a prior explicit trim is a new coverage epoch.
	// Reobserving an unchanged paused boundary does not count an instruction.
	void Observe(Counter cycle)
	{
		if (values.empty()) return;
		if (size && cycle < At(size - 1)) Clear();
		if (size && cycle == At(size - 1)) return;
		if (size == values.size())
		{
			begin = (begin + 1) % values.size();
			--size;
		}
		values[(begin + size) % values.size()] = cycle;
		++size;
		++revision;
	}

	void TrimAtOrAfter(Counter cycle)
	{
		size = LowerBound(cycle);
		++revision;
	}

	Error Back(Counter origin, Counter count, bool atInstructionBoundary, Counter &target) const
	{
		if (count == 0) return Error::InvalidCount;
		// NES, for example, can pause after an opcode but before its index retires.
		// A numerically matching index alone is not a pre-instruction boundary.
		if (!atInstructionBoundary) return Error::NotAtInstructionBoundary;
		const std::size_t index = LowerBound(origin);
		if (index == size || At(index) != origin || count > index)
			return Error::InstructionHistoryUnavailable;
		target = At(index - static_cast<std::size_t>(count));
		return Error::None;
	}

	std::size_t Size() const { return size; }
	Counter Revision() const { return revision; }
	bool Range(Counter &first, Counter &last) const
	{
		if (!size) return false;
		first = At(0);
		last = At(size - 1);
		return true;
	}

private:
	Counter At(std::size_t index) const { return values[(begin + index) % values.size()]; }
	std::size_t LowerBound(Counter cycle) const
	{
		std::size_t first = 0, last = size;
		while (first < last)
		{
			const std::size_t middle = first + (last - first) / 2;
			if (At(middle) < cycle) first = middle + 1;
			else last = middle;
		}
		return first;
	}

	std::vector<Counter> values;
	std::size_t begin, size;
	Counter revision;
};

struct Bounds
{
	bool available = false;
	Counter firstCounter = 0, lastCounter = 0;
	Counter firstFrame = 0, lastFrame = 0;
	Counter originCounter = 0, originFrame = 0;
	bool originAtInstructionBoundary = false;
	ClockDomain domain = ClockDomain::CpuClock;
	Counter clocksPerSecond = 0;
	// Must be established for the covered interval, not inferred from current FPS.
	bool constantTimebase = false;
};

struct Plan
{
	Error error = Error::NoHistory;
	bool byFrame = false;
	Counter target = 0;
};

// Pure admission/resolution only.  A ready plan neither selects a raw snapshot
// pointer nor proves that a backend can restore/acknowledge the destination.
// Revalidate it in the serialized native transaction before any mutation.
inline Plan Resolve(const Bounds &bounds, const InstructionHistory &history, Mode mode, Counter value)
{
	Plan plan;
	if (!bounds.available || bounds.firstCounter > bounds.lastCounter || bounds.firstFrame > bounds.lastFrame)
		return plan;
	Counter target = value;
	const bool relative = mode == Mode::BackInstructions || mode == Mode::BackVideoFrames || mode == Mode::BackSeconds;
	if (relative && (value == 0 || value > 1000000))
	{
		plan.error = Error::InvalidCount;
		return plan;
	}
	plan.byFrame = mode == Mode::Frame || mode == Mode::BackVideoFrames;
	if (mode == Mode::BackInstructions)
	{
		plan.error = history.Back(bounds.originCounter, value, bounds.originAtInstructionBoundary, target);
		if (plan.error != Error::None) return plan;
	}
	else if (mode == Mode::BackVideoFrames)
	{
		if (value > bounds.originFrame)
		{
			plan.error = Error::HistoryUnavailable;
			return plan;
		}
		target = bounds.originFrame - value;
	}
	else if (mode == Mode::BackSeconds)
	{
		if (bounds.domain != ClockDomain::CpuClock || !bounds.constantTimebase || bounds.clocksPerSecond == 0)
		{
			plan.error = Error::UnsupportedTimebase;
			return plan;
		}
		if (value > std::numeric_limits<Counter>::max() / bounds.clocksPerSecond)
		{
			plan.error = Error::Overflow;
			return plan;
		}
		const Counter delta = value * bounds.clocksPerSecond;
		if (delta > bounds.originCounter)
		{
			plan.error = Error::HistoryUnavailable;
			return plan;
		}
		target = bounds.originCounter - delta;
	}
	const Counter origin = plan.byFrame ? bounds.originFrame : bounds.originCounter;
	const Counter first = plan.byFrame ? bounds.firstFrame : bounds.firstCounter;
	const Counter last = plan.byFrame ? bounds.lastFrame : bounds.lastCounter;
	if (target > origin) plan.error = Error::FutureTarget;
	else if (target < first || target > last) plan.error = Error::HistoryUnavailable;
	else
	{
		plan.error = Error::None;
		plan.target = target;
	}
	return plan;
}
}

#endif
