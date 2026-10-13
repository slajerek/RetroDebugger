#ifndef _C_TIMELINE_SEEK_JSON_H_
#define _C_TIMELINE_SEEK_JSON_H_

#include "CTimelineSeekTransaction.h"
#include "json.hpp"
#include <initializer_list>

namespace TimelineSeek
{
inline bool UnsignedInteger(const nlohmann::json &value, Counter &number)
{
	if (value.is_number_unsigned()) { number = value.get<Counter>(); return true; }
	if (!value.is_number_integer()) return false;
	const std::int64_t signedNumber = value.get<std::int64_t>();
	if (signedNumber < 0) return false;
	number = static_cast<Counter>(signedNumber);
	return true;
}

inline bool OnlyKeys(const nlohmann::json &params, std::initializer_list<const char *> allowed)
{
	for (auto it = params.begin(); it != params.end(); ++it)
	{
		bool found = false;
		for (const char *key : allowed) if (it.key() == key) found = true;
		if (!found) return false;
	}
	return true;
}

// Shared native/MCP validation.  Platform is consumed by the MCP adapter first;
// the native endpoint's platform comes from its registered path.
inline const char *ParseRequest(const nlohmann::json &params, Request &request, bool &status, std::uint32_t &timeoutMs)
{
	request = Request();
	status = false;
	timeoutMs = 5000;
	if (!params.is_object() || !params.contains("mode") || !params.at("mode").is_string())
		return "mode must be status, last_read, last_write, cycle, frame or back";
	const std::string mode = params.at("mode").get<std::string>();
	if (mode == "status")
	{
		status = true;
		return OnlyKeys(params, {"mode"}) ? nullptr : "status accepts no destination or timeout parameters";
	}
	if (params.contains("timeoutMs"))
	{
		Counter timeout = 0;
		if (!UnsignedInteger(params.at("timeoutMs"), timeout) || timeout < 1 || timeout > 60000)
			return "timeoutMs must be an integer from 1 to 60000";
		timeoutMs = static_cast<std::uint32_t>(timeout);
	}
	if (mode == "last_read" || mode == "last_write")
	{
		if (!OnlyKeys(params, {"mode", "address", "timeoutMs"})) return "conflicting destination parameters";
		Counter address = 0;
		if (!params.contains("address") || !UnsignedInteger(params.at("address"), address) || address > 65535)
			return "address must be an integer from 0 to 65535";
		request.address = static_cast<std::uint16_t>(address);
		request.access = mode == "last_read" ? Access::Read : Access::Write;
		return nullptr;
	}
	if (mode == "cycle" || mode == "frame")
	{
		const char *key = mode == "cycle" ? "cycle" : "frame";
		if (!OnlyKeys(params, {"mode", key, "timeoutMs"})) return "conflicting destination parameters";
		if (!params.contains(key) || !UnsignedInteger(params.at(key), request.value))
			return "cycle/frame must be an exact nonnegative integer";
		request.mode = mode == "cycle" ? Mode::Counter : Mode::Frame;
		return nullptr;
	}
	if (mode == "back")
	{
		if (!OnlyKeys(params, {"mode", "unit", "count", "timeoutMs"})) return "conflicting destination parameters";
		if (!params.contains("unit") || !params.at("unit").is_string()) return "back requires an explicit unit";
		const std::string unit = params.at("unit").get<std::string>();
		if (unit == "instructions") request.mode = Mode::BackInstructions;
		else if (unit == "video_frames") request.mode = Mode::BackVideoFrames;
		else if (unit == "emulated_seconds") request.mode = Mode::BackSeconds;
		else return "unit must be instructions, video_frames or emulated_seconds";
		request.value = 1;
		if (params.contains("count") && !UnsignedInteger(params.at("count"), request.value))
			return "count must be an integer from 1 to 1000000";
		if (request.value < 1 || request.value > 1000000) return "count must be an integer from 1 to 1000000";
		return nullptr;
	}
	return "mode must be status, last_read, last_write, cycle, frame or back";
}

inline const char *DomainName(ClockDomain domain)
{
	switch (domain)
	{
		case ClockDomain::CpuClock: return "cpu_clock";
		case ClockDomain::InstructionIndex: return "instruction_index";
		case ClockDomain::InstructionDebugClock: return "instruction_debug_clock";
	}
	return "unsupported";
}

inline nlohmann::json PositionJson(const Position &position)
{
	return {{"counter", position.counter}, {"frame", position.frame}, {"paused", position.paused},
		{"instructionBoundary", position.instructionBoundary},
		{"registers", position.registersJson.empty() ? nlohmann::json(nullptr) : nlohmann::json::parse(position.registersJson)}};
}

inline nlohmann::json ResultJson(const Result &result)
{
	using nlohmann::json;
	return {{"sequence", result.sequence}, {"outcome", result.outcome},
		{"success", result.completed && result.outcome == "restored"}, {"completed", result.completed},
		{"mutationStarted", result.mutationStarted},
		{"destination", result.destinationResolved ? json{{"unit", result.byFrame ? "video_frame" : "recorded_counter"}, {"value", result.destination}} : json(nullptr)},
		{"origin", result.contextAvailable ? PositionJson(result.origin) : json(nullptr)},
		{"actual", result.actualAvailable ? PositionJson(result.actual) : json(nullptr)},
		{"actualStateIsFinal", result.completed && result.actualAvailable},
		{"sourcePC", result.sourceAccessAvailable ? json(result.sourcePC) : json(nullptr)},
		{"inputRecording", result.inputRecording}, {"inputReplay", result.inputReplay}};
}

// Use identical, observation-free refusals in native and MCP adapters.  The
// manager also rejects unsupported backends, so direct callers cannot bypass
// this admission policy.  No transaction/sequence is allocated for a refusal.
inline nlohmann::json UnsupportedPlatformJson(const std::string &platform, const nlohmann::json &params)
{
	nlohmann::json result = ResultJson(UnsupportedPlatformResult());
	result["platform"] = platform;
	result["requested"] = params;
	result["seekSupported"] = false;
	result["supportedPlatforms"] = nlohmann::json::array({"c64"});
	return result;
}

inline nlohmann::json StatusJson(const Status &status)
{
	using nlohmann::json;
	const Bounds &b = status.bounds;
	const bool supported = status.seekSupported;
	return {{"counterDomain", DomainName(b.domain)}, {"recording", status.recording},
		{"inputRecording", status.inputRecording}, {"inputReplay", status.inputReplay},
		{"busy", status.busy}, {"capturedCheckpoints", status.verifiedCheckpoints},
		{"seekSupported", supported}, {"supportedPlatforms", json::array({"c64"})},
		{"unsupportedReason", supported ? json(nullptr) : json("unsupported_platform")},
		{"supportedModes", supported ? json::array({"status", "last_read", "last_write", "cycle", "frame", "back"}) : json::array({"status"})},
		{"relativeUnits", json{{"instructions", supported ? "requires contiguous index and pre-dispatch origin" : "unsupported"},
			{"video_frames", supported ? "backend video-frame boundary" : "unsupported"},
			{"emulated_seconds", supported && b.domain == ClockDomain::CpuClock ? "requires verified constant timebase" : "unsupported"}}},
		{"checkpointRange", supported && b.available ? json{{"firstCounter", b.firstCounter}, {"lastCounter", b.lastCounter},
			{"firstFrame", b.firstFrame}, {"lastFrame", b.lastFrame}} : json(nullptr)},
		{"atInstructionBoundary", supported && status.atInstructionBoundary},
		{"instructionRange", supported && status.instructionCoverage ? json{{"first", status.firstInstruction},
			{"last", status.lastInstruction}, {"count", status.instructionCount}} : json(nullptr)},
		{"emulatedSeconds", supported && b.domain == ClockDomain::CpuClock && b.constantTimebase},
		{"clocksPerSecond", supported && b.constantTimebase ? json(b.clocksPerSecond) : json(nullptr)},
		{"lastRequest", ResultJson(status.lastRequest)}};
}
}
#endif
