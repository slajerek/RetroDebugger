#ifndef _CDebuggerServerProtocol_h_
#define _CDebuggerServerProtocol_h_

#include "SYS_Defs.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

// Protocol versions
#define DEBUGGER_PROTOCOL_V1 1
#define DEBUGGER_PROTOCOL_V2 2
#define DEBUGGER_PROTOCOL_CURRENT DEBUGGER_PROTOCOL_V2
#define DEBUGGER_DISCOVERY_VERSION 1

// Request context — carries version-specific correlation info
struct RequestContext
{
	int protocolVersion;     // 1 or 2 (0 = internal/MCP)
	std::string token;       // v1 correlation field
	std::string requestId;   // v2 correlation field

	RequestContext() : protocolVersion(1) {}
};

// Endpoint descriptor — metadata for discovery and MCP tool generation
struct EndpointDescriptor
{
	std::string fn;                      // endpoint name (e.g. "c64/cpu/status")
	std::string platform;                // "c64", "atari800", "nes", "" for server-level
	std::string category;                // "control", "cpu", "memory", "breakpoints", "chips", "media", "server"
	std::string description;             // human-readable description
	nlohmann::json paramsSchema;         // JSON Schema for params (can be empty object)
	nlohmann::json resultSchema;         // JSON Schema for result (can be empty object)
	bool supportsBinaryInput;            // endpoint accepts binary payload
	bool supportsBinaryOutput;           // endpoint returns binary payload
	bool isStubbed;                      // backend is TODO/no-op

	EndpointDescriptor()
		: supportsBinaryInput(false), supportsBinaryOutput(false), isStubbed(false)
	{
		paramsSchema = nlohmann::json::object();
		resultSchema = nlohmann::json::object();
	}

	nlohmann::json ToJson() const
	{
		nlohmann::json j;
		j["fn"] = fn;
		if (!platform.empty()) j["platform"] = platform;
		j["category"] = category;
		if (!description.empty()) j["description"] = description;
		j["supportsBinaryInput"] = supportsBinaryInput;
		j["supportsBinaryOutput"] = supportsBinaryOutput;
		j["isStubbed"] = isStubbed;
		if (!paramsSchema.empty()) j["paramsSchema"] = paramsSchema;
		if (!resultSchema.empty()) j["resultSchema"] = resultSchema;
		return j;
	}
};

// Endpoint handler function type
typedef std::function<std::vector<char> *(const RequestContext &ctx, const nlohmann::json params, u8 *binaryData, int binaryDataSize)> EndpointHandlerV2;

// Legacy handler type (v1 compat — existing lambdas use this)
typedef std::function<std::vector<char> *(const std::string token, const nlohmann::json params, u8 *binaryData, int binaryDataSize)> EndpointHandlerV1;

// Protocol helper functions
namespace DebuggerProtocol
{
	// Validate JSON integers before narrowing.  Subtraction avoids overflowing address + size.
	// An empty read at the exact end remains compatible with the legacy block endpoints.
	inline bool ValidateMemoryReadRange(const nlohmann::json &addressJson, const nlohmann::json &sizeJson,
									   int dataLength, int &address, int &size)
	{
		if (dataLength < 0 || !addressJson.is_number_integer() || !sizeJson.is_number_integer())
			return false;
		if ((!addressJson.is_number_unsigned() && addressJson.get<int64_t>() < 0)
			|| (!sizeJson.is_number_unsigned() && sizeJson.get<int64_t>() < 0))
			return false;

		uint64_t start = addressJson.get<uint64_t>();
		uint64_t count = sizeJson.get<uint64_t>();
		uint64_t length = static_cast<uint64_t>(dataLength);
		if (start > length || count > length - start)
			return false;

		address = static_cast<int>(start);
		size = static_cast<int>(count);
		return true;
	}

	// Parse incoming request, populate context
	RequestContext ParseRequest(const nlohmann::json &requestJson);

	// Create structured error response
	nlohmann::json MakeError(const std::string &code, const std::string &message,
							 const nlohmann::json &details = nlohmann::json::object());

	// Prepare v2 response JSON
	nlohmann::json MakeResponse(const RequestContext &ctx, int status,
								const nlohmann::json &result = nlohmann::json::object(),
								const nlohmann::json &error = nlohmann::json());
}

namespace GuiControl
{
	// A deliberately closed surface, shared by discovery, MCP, and direct calls.
	inline nlohmann::json Schema()
	{
		using nlohmann::json;
		return {{"type", "object"}, {"additionalProperties", false},
			{"properties", {
				{"action", {{"type", "string"}, {"enum", json::array({"capabilities", "status", "show", "open", "set_address", "submit", "cancel", "capture"})}}},
				{"target", {{"type", "string"}, {"enum", json::array({"c64.memory_breakpoints", "c64.memory_breakpoints.editor"})}}},
				{"text", {{"type", "string"}, {"minLength", 1}, {"maxLength", 64}, {"not", {{"pattern", "[^ -~]"}}}}},
				{"requestId", {{"type", "string"}, {"minLength", 1}, {"maxLength", 20}, {"not", {{"pattern", "[^0-9]"}}}}}
			}}, {"required", json::array({"action"})},
			{"oneOf", json::array({
				{{"properties", {{"action", {{"enum", json::array({"capabilities"})}}}}}, {"maxProperties", 1}},
				{{"properties", {{"action", {{"enum", json::array({"status"})}}}}}, {"required", json::array({"requestId"})}, {"maxProperties", 2}},
				{{"properties", {{"action", {{"enum", json::array({"show", "open"})}}}, {"target", {{"enum", json::array({"c64.memory_breakpoints"})}}}}}, {"required", json::array({"target"})}, {"maxProperties", 2}},
				{{"properties", {{"action", {{"enum", json::array({"set_address"})}}}, {"target", {{"enum", json::array({"c64.memory_breakpoints.editor"})}}}}}, {"required", json::array({"target", "text"})}, {"maxProperties", 3}},
				{{"properties", {{"action", {{"enum", json::array({"submit", "cancel"})}}}, {"target", {{"enum", json::array({"c64.memory_breakpoints.editor"})}}}}}, {"required", json::array({"target"})}, {"maxProperties", 2}},
				{{"properties", {{"action", {{"enum", json::array({"capture"})}}}}}, {"required", json::array({"target"})}, {"maxProperties", 2}}
			})}};
	}

	inline bool Valid(const nlohmann::json &args)
	{
		if (!args.is_object() || !args.contains("action") || !args["action"].is_string()) return false;
		std::string action = args["action"];
		if (action == "capabilities") return args.size() == 1;
		if (action == "status")
		{
			if (args.size() != 2 || !args.contains("requestId") || !args["requestId"].is_string()) return false;
			std::string id = args["requestId"];
			return !id.empty() && id.size() <= 20 && id.find_first_not_of("0123456789") == std::string::npos;
		}
		if (!args.contains("target") || !args["target"].is_string()) return false;
		std::string target = args["target"];
		bool panel = target == "c64.memory_breakpoints";
		bool editor = target == "c64.memory_breakpoints.editor";
		if (action == "set_address")
		{
			if (!editor || args.size() != 3 || !args.contains("text") || !args["text"].is_string()) return false;
			std::string text = args["text"];
			if (text.empty() || text.size() > 64) return false;
			for (unsigned char c : text) if (c < 32 || c > 126) return false;
			return true;
		}
		if (args.size() != 2) return false;
		return ((action == "show" || action == "open") && panel)
			|| ((action == "submit" || action == "cancel") && editor)
			|| (action == "capture" && (panel || editor));
	}
}

#endif
