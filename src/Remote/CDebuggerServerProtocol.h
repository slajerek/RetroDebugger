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

#endif
