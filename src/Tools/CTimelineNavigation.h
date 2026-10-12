#ifndef _CTimelineNavigation_h_
#define _CTimelineNavigation_h_

#include "json.hpp"
#include <climits>
#include <cstdint>

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

inline bool ParseFrame(const nlohmann::json &params, int &frame)
{
	if (!params.is_object() || params.size() != 1 || !params.contains("frame"))
		return false;
	const auto &value = params.at("frame");
	if (!value.is_number_integer()) return false;
	if (value.is_number_unsigned())
	{
		if (value.get<uint64_t>() > INT_MAX) return false;
	}
	else if (value.get<int64_t>() < 0 || value.get<int64_t>() > INT_MAX)
		return false;
	frame = value.get<int>();
	return true;
}

inline bool ParseTool(const nlohmann::json &params, bool &status, int &frame)
{
	if (!params.is_object() || !params.contains("platform") || params.at("platform") != "c64"
		|| !params.contains("action") || !params.at("action").is_string()) return false;
	status = params.at("action") == "status";
	if (status) return params.size() == 2;
	if (params.at("action") != "frame" || params.size() != 3 || !params.contains("frame")) return false;
	return ParseFrame({{"frame", params.at("frame")}}, frame);
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
