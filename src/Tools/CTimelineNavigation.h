#ifndef _CTimelineNavigation_h_
#define _CTimelineNavigation_h_

#include "json.hpp"
#include <climits>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <cstring>
#include <string>

// Wire contract for existing frame navigation, not precise timeline seeking.
namespace TimelineNavigation
{
struct Status
{
	bool supported = false;
	bool running = false;
	bool paused = false;
	bool busy = false;
	bool recording = false;
	int intervalFrames = 0;
	int retention = 0;
	uint64_t snapshots = 0;
	uint32_t oldestFrame = 0;
	uint32_t newestFrame = 0;
};

struct Result
{
	bool accepted = false;
	const char *reason = "unavailable";
};

static const int MaxWaitMs = 5000;

struct Arguments
{
	bool statusOnly = false;
	int frame = 0;
	int waitMs = -1; // Omitted: preserve the legacy queued response.
	uint64_t requestId = 0;
};

inline bool UnsignedInteger(const nlohmann::json &value, uint64_t maximum, uint64_t &number)
{
	if (!value.is_number_integer()) return false;
	if (!value.is_number_unsigned() && value.get<int64_t>() < 0) return false;
	number = value.get<uint64_t>();
	return number <= maximum;
}

// Shared strict validation for direct endpoints and the MCP adapter.
inline bool ParseEndpoint(const nlohmann::json &params, bool statusOnly, Arguments &args)
{
	args = Arguments();
	args.statusOnly = statusOnly;
	if (statusOnly && params.is_null()) return true;
	if (!params.is_object()) return false;
	uint64_t value = 0;
	if (statusOnly)
	{
		if (params.empty()) return true;
		return params.size() == 1 && params.contains("requestId")
			&& UnsignedInteger(params.at("requestId"), UINT64_MAX, args.requestId) && args.requestId != 0;
	}
	if (!params.contains("frame") || !UnsignedInteger(params.at("frame"), INT_MAX, value)) return false;
	args.frame = static_cast<int>(value);
	if (params.contains("waitMs"))
	{
		if (!UnsignedInteger(params.at("waitMs"), MaxWaitMs, value)) return false;
		args.waitMs = static_cast<int>(value);
	}
	return params.size() == (args.waitMs < 0 ? 1 : 2);
}

inline bool ParseTool(const nlohmann::json &params, Arguments &args)
{
	if (!params.is_object() || !params.contains("platform") || params.at("platform") != "c64"
		|| !params.contains("action") || !params.at("action").is_string()) return false;
	if (params.at("action") != "status" && params.at("action") != "frame") return false;
	nlohmann::json endpointParams = params;
	endpointParams.erase("platform");
	endpointParams.erase("action");
	return ParseEndpoint(endpointParams, params.at("action") == "status", args);
}

struct Observation
{
	uint64_t requestId = 0;
	int requestedFrame = 0;
	const char *status = "unavailable";
	const char *reason = "request_not_retained";
	uint32_t observedFrame = 0;
	uint64_t observedCycle = 0;
};

// One retained request, not a job/history service.  Emulator callers take the
// manager lock before this signal lock; waiters take ONLY this lock, never an
// emulator lock.  No method here calls back into the emulator.
class Completion
{
public:
	uint64_t Begin(int frame)
	{
		std::lock_guard<std::mutex> lock(mutex);
		observation = Observation();
		observation.requestId = ++sequence;
		observation.requestedFrame = frame;
		observation.status = "pending";
		observation.reason = "in_progress";
		restored = targetReached = false;
		awaitingStablePause.store(false, std::memory_order_release);
		cv.notify_all();
		return sequence;
	}

	void Cancel(const char *reason)
	{
		std::lock_guard<std::mutex> lock(mutex);
		CancelLocked(reason);
	}

	void Restored(bool success)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (!Pending()) return;
		if (!success) CancelLocked("restore_failed");
		else restored = true;
	}

	void TargetReached(uint32_t frame)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (Pending() && restored && frame == static_cast<uint32_t>(observation.requestedFrame))
		{
			targetReached = true;
			awaitingStablePause.store(true, std::memory_order_release);
		}
	}

	// Keep an interrupt/drive cycle from parking before the main CPU can export
	// its registers at the stable pause point.  Read-only, with no hot-loop lock.
	bool AwaitingStablePause() const { return awaitingStablePause.load(std::memory_order_acquire); }

	// Called only at the main CPU's restore-capable, post-import pause point.
	void Paused(bool busy, uint32_t frame, uint64_t cycle)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (!Pending() || busy) return;
		if (!restored || !targetReached)
		{
			CancelLocked("interrupted");
			return;
		}
		awaitingStablePause.store(false, std::memory_order_release);
		observation.status = "completed";
		observation.reason = nullptr;
		observation.observedFrame = frame;
		observation.observedCycle = cycle;
		cv.notify_all();
	}

	Observation Wait(uint64_t requestId, int waitMs)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (waitMs > 0)
			cv.wait_for(lock, std::chrono::milliseconds(waitMs > MaxWaitMs ? MaxWaitMs : waitMs),
				[this, requestId]{ return observation.requestId != requestId || !Pending(); });
		if (observation.requestId != requestId || requestId == 0)
		{
			Observation unavailable;
			unavailable.requestId = requestId;
			return unavailable;
		}
		Observation result = observation;
		if (Pending() && waitMs > 0) result.reason = "wait_timeout";
		return result;
	}

private:
	bool Pending() const { return std::strcmp(observation.status, "pending") == 0; }
	void CancelLocked(const char *reason)
	{
		if (!Pending()) return;
		observation.status = "cancelled";
		observation.reason = reason;
		targetReached = false;
		awaitingStablePause.store(false, std::memory_order_release);
		cv.notify_all();
	}
	std::mutex mutex;
	std::condition_variable cv;
	uint64_t sequence = 0;
	Observation observation;
	bool restored = false;
	bool targetReached = false;
	std::atomic<bool> awaitingStablePause{false};
};

inline nlohmann::json ObservationJson(const Observation &o)
{
	bool completed = std::string(o.status) == "completed";
	return {{"platform", "c64"}, {"navigation", "native_frame"}, {"requestId", o.requestId},
		{"requestedFrame", std::string(o.status) == "unavailable" ? nlohmann::json(nullptr) : nlohmann::json(o.requestedFrame)},
		{"accepted", std::string(o.status) != "unavailable"}, {"status", o.status}, {"completionKnown", completed},
		{"reason", o.reason ? nlohmann::json(o.reason) : nlohmann::json(nullptr)},
		{"observedFrame", completed ? nlohmann::json(o.observedFrame) : nlohmann::json(nullptr)},
		{"observedCycle", completed ? nlohmann::json(o.observedCycle) : nlohmann::json(nullptr)}};
}

// Reject, rather than silently taking the legacy lower-bound adjustment or
// narrowing an unrepresentable retained frame range to the backend's int API.
inline const char *FrameRefusal(const Status &s, int frame)
{
	if (!s.supported) return "unsupported_platform";
	if (!s.running) return "emulator_stopped";
	if (s.busy) return "busy";
	if (!s.paused) return "pause_required";
	if (frame < 0) return "invalid_frame";
	if (!s.snapshots) return "no_history";
	if (s.newestFrame > INT_MAX) return "frame_range_unrepresentable";
	if (static_cast<uint32_t>(frame) <= s.oldestFrame || static_cast<uint32_t>(frame) > s.newestFrame)
		return "frame_out_of_range";
	return nullptr;
}

inline nlohmann::json StatusJson(const Status &s)
{
	nlohmann::json result = {{"platform", "c64"}, {"supported", s.supported},
		{"navigation", "native_frame"}, {"completionReporting", "acceptance_only"},
		{"running", s.running}, {"paused", s.paused}, {"busy", s.busy},
		{"recording", s.recording}, {"intervalFrames", s.intervalFrames}, {"retention", s.retention},
		{"storedSnapshots", s.snapshots}, {"oldestFrame", nullptr}, {"newestFrame", nullptr},
		{"oldestFrameExclusive", true}};
	if (s.snapshots)
	{
		result["oldestFrame"] = s.oldestFrame;
		result["newestFrame"] = s.newestFrame;
	}
	return result;
}

inline nlohmann::json ResultJson(const Result &result, int frame)
{
	return {{"platform", "c64"}, {"navigation", "native_frame"},
		{"status", result.accepted ? "queued" : "refused"}, {"accepted", result.accepted},
		{"requestedFrame", frame}, {"completionKnown", false},
		{"reason", result.accepted ? nlohmann::json(nullptr) : nlohmann::json(result.reason)}};
}
}
#endif
