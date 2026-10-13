#ifndef _C_TIMELINE_SEEK_TRANSACTION_H_
#define _C_TIMELINE_SEEK_TRANSACTION_H_

#include "CTimelineSeekPlan.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

namespace TimelineSeek
{
enum class Access { None, Read, Write };

struct Request
{
	Mode mode = Mode::Counter;
	Counter value = 0;
	Access access = Access::None;
	std::uint16_t address = 0;
};

// Captured by the emulation thread, never reconstructed by polling a running
// CPU from the server.  Register JSON is opaque here to keep the handshake
// independent of the remote protocol and the emulator implementations.
struct Position
{
	Counter counter = 0, frame = 0;
	bool paused = false, instructionBoundary = false;
	int pc = -1;
	std::string registersJson;
};

struct Result
{
	Counter sequence = 0;
	std::string outcome = "not_submitted";
	bool completed = false;
	bool mutationStarted = false;
	bool destinationResolved = false;
	bool byFrame = false;
	Counter destination = 0;
	// Zero is a valid counter/frame.  Availability must not be inferred from
	// numeric values, especially when the server times out before CPU intake.
	bool contextAvailable = false, actualAvailable = false;
	Position origin, actual;
	bool sourceAccessAvailable = false;
	int sourcePC = -1;
	bool inputRecording = false, inputReplay = false;
};

inline Result UnsupportedPlatformResult()
{
	Result result;
	result.completed = true;
	result.outcome = "unsupported_platform";
	return result;
}

struct Status
{
	Bounds bounds;
	bool seekSupported = false;
	bool recording = false, inputRecording = false, inputReplay = false;
	bool instructionCoverage = false, atInstructionBoundary = false;
	Counter firstInstruction = 0, lastInstruction = 0;
	std::size_t instructionCount = 0, verifiedCheckpoints = 0;
	bool busy = false;
	Result lastRequest;
};

// Value-owned request and result; no caller buffers survive a timeout.  The
// native manager must keep its snapshot lifetimes locked during admission and
// call BeginMutation before queuing a restore.  There is never a manager or
// backend callback while this class's mutex is held.  Lock order when needed:
// manager -> transaction, never transaction -> manager.
class Transaction
{
public:
	bool Submit(const Request &request, Counter &sequence)
	{
		std::lock_guard<std::mutex> lock(mutex);
		// Completed results remain reserved until the waiter consumes them.
		// Otherwise a fast second request could erase the first completion.
		if (state != State::Idle || nextSequence == std::numeric_limits<Counter>::max())
			return false;
		pending = request;
		result = Result();
		result.sequence = ++nextSequence;
		result.outcome = "pending";
		sequence = result.sequence;
		cancelRequested = false;
		abandoned = false;
		state = State::Pending;
		cpuWork.store(true, std::memory_order_release);
		return true;
	}

	bool Take(Request &request, Counter &sequence)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (state != State::Pending) return false;
		request = pending;
		sequence = result.sequence;
		state = State::Admitting;
		result.outcome = "admitting";
		return true;
	}

	// Atomic cancellation/admission boundary.  If the waiter timed out before
	// this transition, this returns false and no native state may be changed.
	bool BeginMutation(Counter sequence, const Result &admission)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (state != State::Admitting || sequence != result.sequence || cancelRequested)
			return false;
		result = admission;
		result.sequence = sequence;
		result.completed = false;
		result.mutationStarted = true;
		result.outcome = "restoring";
		state = State::Executing;
		return true;
	}

	// A settled native pause is required for a successful completion.  A load
	// being dequeued or a restore flag being cleared must not call this as
	// success.  Refusals and explicit post-mutation failures use the same path.
	bool Finish(Counter sequence, const Result &completion)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if ((state != State::Admitting && state != State::Executing) || sequence != result.sequence)
			return false;
		const bool mutated = result.mutationStarted;
		result = completion;
		result.sequence = sequence;
		result.mutationStarted = mutated;
		result.completed = true;
		// An abandoned caller cannot block the next request indefinitely after
		// the CPU has acknowledged cancellation.  Its value-owned result stays
		// readable until a new request is submitted, with its sequence attached.
		state = abandoned ? State::Idle : State::Completed;
		cpuWork.store(false, std::memory_order_release);
		changed.notify_all();
		return true;
	}

	// One waiter per successful Submit, owned by that submitter.
	// Bounded server wait.  A timeout before mutation cancels the request
	// atomically.  Once mutation began, the caller receives an honest pending
	// cancellation result, not a claimed no-op or an invented final CPU state.
	// New requests remain busy until the CPU acknowledges the cancellation.
	Result Wait(Counter sequence, std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (state == State::Idle || result.sequence != sequence)
			return Unmatched(sequence);
		changed.wait_for(lock, timeout, [this, sequence]() {
			return result.sequence != sequence || state == State::Completed;
		});
		if (result.sequence != sequence) return Unmatched(sequence);
		if (state == State::Completed)
		{
			Result completion = result;
			state = State::Idle;
			return completion;
		}
		cancelRequested = true;
		abandoned = true;
		if (state == State::Pending || state == State::Admitting)
		{
			result.outcome = "timeout_before_mutation";
			result.completed = true;
			state = State::Idle;
			cpuWork.store(false, std::memory_order_release);
		}
		else
		{
			result.outcome = "timeout_cancellation_pending";
			// actual is still the last CPU-published observation, not an atomic
			// sample of whatever the emulator is doing on the server's timeout.
		}
		return result;
	}

	bool CancellationRequested(Counter sequence) const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return result.sequence == sequence && cancelRequested;
	}

	// A cheap pause-loop wake hint, not authorization to run or unwind a CPU.
	// Writes occur under the transaction mutex so an older completion cannot
	// erase a newer submitter's wake.  Native mutation still needs BeginMutation.
	bool NeedsCpuService() const { return cpuWork.load(std::memory_order_acquire); }

	bool Busy() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return state != State::Idle;
	}

	Result Inspect() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return result;
	}

private:
	enum class State { Idle, Pending, Admitting, Executing, Completed };
	static Result Unmatched(Counter sequence)
	{
		Result mismatch;
		mismatch.sequence = sequence;
		mismatch.outcome = "request_not_owned";
		return mismatch;
	}
	mutable std::mutex mutex;
	std::atomic<bool> cpuWork{false};
	std::condition_variable changed;
	State state = State::Idle;
	Counter nextSequence = 0;
	bool cancelRequested = false, abandoned = false;
	Request pending;
	Result result;
};
}

#endif
