#include "CMCPServer.h"
#include "CMCPBridgeClient.h"
#include "CDebuggerServer.h"
#include "C64D_Version.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebuggerServerApi.h"
#include "C64DShutdown.h"
#include "DBG_Log.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <set>
#include <csignal>

#include "SYS_Main.h"
#include <SDL3/SDL.h>

using namespace std;
using namespace nlohmann;

// --- Base64 encode/decode for MCP binary payloads ---

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static string Base64Encode(const unsigned char *data, size_t len)
{
	string out;
	out.reserve(((len + 2) / 3) * 4);
	for (size_t i = 0; i < len; i += 3)
	{
		unsigned int n = ((unsigned int)data[i]) << 16;
		if (i + 1 < len) n |= ((unsigned int)data[i + 1]) << 8;
		if (i + 2 < len) n |= ((unsigned int)data[i + 2]);
		out += b64_table[(n >> 18) & 0x3F];
		out += b64_table[(n >> 12) & 0x3F];
		out += (i + 1 < len) ? b64_table[(n >> 6) & 0x3F] : '=';
		out += (i + 2 < len) ? b64_table[n & 0x3F] : '=';
	}
	return out;
}

static int B64DecodeChar(char c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

static vector<unsigned char> Base64Decode(const string &input)
{
	vector<unsigned char> out;
	out.reserve((input.size() / 4) * 3);
	unsigned int buf = 0;
	int bits = 0;
	for (char c : input)
	{
		if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;
		int val = B64DecodeChar(c);
		if (val < 0) continue;
		buf = (buf << 6) | val;
		bits += 6;
		if (bits >= 8)
		{
			bits -= 8;
			out.push_back((unsigned char)((buf >> bits) & 0xFF));
		}
	}
	return out;
}

CMCPServer *mcpServer = NULL;

CMCPServer::CMCPServer()
{
	isRunning = false;
	initialized = false;
	shouldStop = false;
	toolsRegistered = false;
	debuggerToolsRegistered = false;
	debuggerServer.store(NULL, std::memory_order_release);
	bridgeClient = NULL;
	isBridgeMode = false;
	debuggerServerPublished = false;
}

CMCPServer::~CMCPServer()
{
}

void CMCPServer::Start()
{
	if (isRunning)
	{
		LOGError("CMCPServer: already running");
		return;
	}
	SYS_StartThread(this);
}

void CMCPServer::Stop()
{
	shouldStop = true;
}

void CMCPServer::SetDebuggerServer(CDebuggerServer *server)
{
	debuggerServer.store(server, std::memory_order_release);
	debuggerServerPublished = (server != NULL);
}

CDebuggerServer *CMCPServer::GetReadyDebuggerServer()
{
	CDebuggerServer *server = debuggerServer.load(std::memory_order_acquire);
	if (server == NULL || !server->IsEndpointRegistryReady())
		return NULL;
	return server;
}

// Read a JSON-RPC message from stdin.
// Supports two formats:
//   1. Content-Length framing (MCP spec): "Content-Length: N\r\n\r\n{...}"
//   2. Bare JSON per line (Claude Code sends this): "{...}\n"
string CMCPServer::ReadMessage()
{
	string line;

	while (getline(cin, line))
	{
		// Remove trailing \r if present
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty())
			continue;

		// Check if this is a Content-Length header (framed mode)
		if (line.substr(0, 16) == "Content-Length: ")
		{
			int contentLength = stoi(line.substr(16));

			// Skip remaining headers until empty line
			string headerLine;
			while (getline(cin, headerLine))
			{
				if (!headerLine.empty() && headerLine.back() == '\r')
					headerLine.pop_back();
				if (headerLine.empty())
					break;
			}

			if (contentLength <= 0)
				return "";

			// Read exactly contentLength bytes of body
			string body(contentLength, '\0');
			cin.read(&body[0], contentLength);
			if (cin.fail())
				return "";
			return body;
		}

		// Bare JSON line (starts with '{')
		if (line[0] == '{')
			return line;
	}

	return "";
}

// Write a JSON-RPC message to stdout (thread-safe)
// Uses bare JSON + newline format (matching Claude Code's client behavior)
void CMCPServer::WriteMessage(const json &msg)
{
	string body = msg.dump();
	lock_guard<mutex> lock(writeMutex);
	fprintf(stdout, "%s\n", body.c_str());
	fflush(stdout);
}

void CMCPServer::SendNotification(const string &method, const json &params)
{
	json notification;
	notification["jsonrpc"] = "2.0";
	notification["method"] = method;
	if (!params.empty())
		notification["params"] = params;
	WriteMessage(notification);
}

void CMCPServer::CheckPlatformStateChanges()
{
	if (isBridgeMode && bridgeClient)
	{
		// Bridge mode: check connection state and remote platforms
		MCPBridgeState bridgeState = bridgeClient->GetState();
		set<string> currentPlatforms;

		if (bridgeState == MCP_BRIDGE_CONNECTED)
		{
			json platforms = bridgeClient->GetRemotePlatformsCopy();
			for (const auto &p : platforms)
			{
				if (p.value("running", false))
					currentPlatforms.insert(p.value("name", ""));
			}
		}

		if (currentPlatforms != lastActivePlatforms)
		{
			lastActivePlatforms = currentPlatforms;
			if (initialized)
			{
				SendNotification("notifications/tools/list_changed");
				SendNotification("notifications/resources/list_changed");
			}
		}
		return;
	}

	// Loading the ready server is the synchronization barrier that makes
	// viewC64 and its completed debugInterfaces vector safe to inspect.
	if (!GetReadyDebuggerServer()) return;

	set<string> currentPlatforms;
	for (auto *di : viewC64->debugInterfaces)
	{
		if (di->isRunning)
			currentPlatforms.insert(di->GetPlatformNameEndpointString());
	}

	if (currentPlatforms != lastActivePlatforms)
	{
		lastActivePlatforms = currentPlatforms;
		if (initialized)
		{
			SendNotification("notifications/tools/list_changed");
			SendNotification("notifications/resources/list_changed");
		}
	}
}

void CMCPServer::ThreadRun(void *passData)
{
	ThreadSetName("MCPServer");
	isRunning = true;

	// When MCP is active, stdout is reserved for JSON-RPC.
	// All debug output must go to stderr or log file only.
	// LOGD/LOGM/LOGError write to the log file, not stdout, so they're safe.
	LOGD("CMCPServer: started (stdout reserved for MCP JSON-RPC)");

	// Tools/resources/prompts are registered lazily on first tools/list request.
	// This allows the MCP thread to start early (in MT_PostInit) and respond to
	// the initialize handshake immediately, before emulators are fully ready.
	toolsRegistered = false;

	// Main JSON-RPC loop: read from stdin, dispatch, write to stdout
	while (!shouldStop)
	{
		// Check for platform state changes after each request
		CheckPlatformStateChanges();

		string msgStr = ReadMessage();
		if (msgStr.empty())
		{
			if (cin.eof())
				break;
			continue;
		}

		try
		{
			json request = json::parse(msgStr);
			json response = HandleRequest(request);

			// Notifications (no id) don't get responses
			if (!response.is_null())
			{
				WriteMessage(response);
			}
		}
		catch (const exception &e)
		{
			LOGError("CMCPServer: parse error: %s", e.what());
			json err;
			err["jsonrpc"] = "2.0";
			err["id"] = nullptr;
			err["error"]["code"] = -32700;
			err["error"]["message"] = string("Parse error: ") + e.what();
			WriteMessage(err);
		}
	}

	isRunning = false;
	LOGD("CMCPServer: stopped");
}

json CMCPServer::HandleRequest(const json &request)
{
	// Check for notification (no id field)
	bool isNotification = !request.contains("id");
	json id = request.contains("id") ? request["id"] : nullptr;

	string method = request.value("method", "");

	// Handle notifications
	if (method == "notifications/initialized")
	{
		initialized = true;
		return json(); // No response for notifications
	}

	if (isNotification)
		return json(); // Other notifications: no response

	// Request methods
	json params = request.value("params", json::object());

	json result;
	if (method == "initialize")
		result = HandleInitialize(params);
	else if (method == "tools/list")
		result = HandleToolsList();
	else if (method == "tools/call")
		result = HandleToolsCall(params);
	else if (method == "resources/list")
		result = HandleResourcesList();
	else if (method == "resources/read")
		result = HandleResourcesRead(params);
	else if (method == "prompts/list")
		result = HandlePromptsList();
	else if (method == "prompts/get")
		result = HandlePromptsGet(params);
	else
	{
		json resp;
		resp["jsonrpc"] = "2.0";
		resp["id"] = id;
		resp["error"]["code"] = -32601;
		resp["error"]["message"] = "Method not found: " + method;
		return resp;
	}

	// Check if result is an error
	if (result.contains("error"))
	{
		json resp;
		resp["jsonrpc"] = "2.0";
		resp["id"] = id;
		resp["error"] = result["error"];
		return resp;
	}

	json resp;
	resp["jsonrpc"] = "2.0";
	resp["id"] = id;
	resp["result"] = result;
	return resp;
}

json CMCPServer::HandleInitialize(const json &params)
{
	json result;
	result["protocolVersion"] = "2024-11-05";
	result["capabilities"]["tools"]["listChanged"] = true;
	result["capabilities"]["resources"]["subscribe"] = false;
	result["capabilities"]["resources"]["listChanged"] = true;
	result["capabilities"]["prompts"]["listChanged"] = false;

	result["serverInfo"]["name"] = "retrodebugger";
	result["serverInfo"]["version"] = RETRODEBUGGER_VERSION_STRING;

	return result;
}

json CMCPServer::HandleToolsList()
{
	// Lazy registration — wait for the server to be ready.
	if (!toolsRegistered)
	{
		if (isBridgeMode)
		{
			// Bridge mode: register bridge-local tools immediately,
			// debugger tools are managed by OnBridgeStateChanged callback
			toolsRegistered = true;
			RegisterBridgeLocalTools();
			RegisterStaticResources();
			RegisterPrompts();

			if (bridgeClient && bridgeClient->GetState() == MCP_BRIDGE_CONNECTED && !debuggerToolsRegistered)
			{
				RegisterDebuggerTools(bridgeClient);
				debuggerToolsRegistered = true;
			}
		}
		else
		{
			// Headless mode: wait until the view and endpoint registry have been
			// published through the synchronized debugger-server readiness gate.
			CDebuggerServer *server = GetReadyDebuggerServer();
			for (int i = 0; i < 100 && server == NULL; i++)
			{
				SYS_Sleep(100);
				server = GetReadyDebuggerServer();
			}
			if (server != NULL)
			{
				toolsRegistered = true;
				RegisterDebuggerTools(server);
				RegisterStaticResources();
				RegisterPrompts();

				for (auto *di : viewC64->debugInterfaces)
				{
					if (di->isRunning)
						lastActivePlatforms.insert(di->GetPlatformNameEndpointString());
				}
			}
		}
	}
	// In bridge mode, debugger tools are managed by OnBridgeStateChanged callback

	json result;
	json toolList = json::array();

	// Build set of running platform names
	std::set<string> activePlatforms;
	if (isBridgeMode && bridgeClient)
	{
		// In bridge mode, use remote platform state (snapshot to avoid data race)
		if (bridgeClient->GetState() == MCP_BRIDGE_CONNECTED)
		{
			json platforms = bridgeClient->GetRemotePlatformsCopy();
			for (const auto &p : platforms)
			{
				if (p.value("running", false))
					activePlatforms.insert(p.value("name", ""));
			}
		}
	}
	else if (GetReadyDebuggerServer())
	{
		for (auto *di : viewC64->debugInterfaces)
		{
			if (di->isRunning)
				activePlatforms.insert(di->GetPlatformNameEndpointString());
		}
	}

	// Snapshot tools under lock to avoid data race with bridge callback thread
	vector<MCPToolDescriptor> toolsSnapshot;
	{
		lock_guard<mutex> lock(toolsMutex);
		toolsSnapshot = tools;
	}

	MCPBridgeState bridgeState = isBridgeMode && bridgeClient ? bridgeClient->GetState() : MCP_BRIDGE_CONNECTED;

	for (const auto &tool : toolsSnapshot)
	{
		// Filter: if tool has a platform requirement, only show if that platform is running
		if (!tool.platform.empty() && activePlatforms.find(tool.platform) == activePlatforms.end())
			continue;

		// In bridge mode, hide debugger-backed tools when disconnected
		// (bridge-local tools have empty platform)
		if (isBridgeMode && bridgeClient && bridgeState != MCP_BRIDGE_CONNECTED)
		{
			// Only show bridge-local tools (retro_transport_diagnostics, retro_reconnect, retro_list_platforms)
			if (tool.name != "retro_transport_diagnostics" &&
				tool.name != "retro_reconnect" &&
				tool.name != "retro_shutdown" &&
				tool.name != "retro_list_platforms")
				continue;
		}

		json t;
		t["name"] = tool.name;
		t["description"] = tool.description;
		t["inputSchema"] = tool.inputSchema;
		toolList.push_back(t);
	}

	result["tools"] = toolList;
	return result;
}

void CMCPServer::EnsureToolsRegistered()
{
	if (!toolsRegistered)
	{
		// Force registration by calling HandleToolsList logic
		HandleToolsList();
	}
}

json CMCPServer::HandleToolsCall(const json &params)
{
	EnsureToolsRegistered();

	string toolName = params.value("name", "");

	// The readiness gate (PR #130, carried): without a ready debugger server
	// nothing may dispatch into it. The shutdown tool is deliberately
	// exempt -- it is the session-ender, must reach the desktop (or the
	// bridge fallback path) even when no ready server exists, and its own
	// handler never touches the gated pointer.
	if (!isBridgeMode && debuggerServerPublished && !GetReadyDebuggerServer()
		&& toolName != "retro_shutdown")
	{
		json result;
		json content = json::array();
		json textContent;
		textContent["type"] = "text";
		textContent["text"] = "Error: Debugger is not ready";
		content.push_back(textContent);
		result["content"] = content;
		result["isError"] = true;
		return result;
	}

	json toolArgs = params.value("arguments", json::object());

	// Snapshot tools under lock to avoid data race with bridge callback thread
	vector<MCPToolDescriptor> toolsSnapshot;
	{
		lock_guard<mutex> lock(toolsMutex);
		toolsSnapshot = tools;
	}

	for (const auto &tool : toolsSnapshot)
	{
		if (tool.name == toolName)
		{
			try
			{
				json toolResult = tool.handler(toolArgs);
				json result;
				json content = json::array();
				json textContent;
				textContent["type"] = "text";
				textContent["text"] = toolResult.dump(2);
				content.push_back(textContent);
				result["content"] = content;
				return result;
			}
			catch (const exception &e)
			{
				json result;
				json content = json::array();
				json textContent;
				textContent["type"] = "text";
				textContent["text"] = string("Error: ") + e.what();
				content.push_back(textContent);
				result["content"] = content;
				result["isError"] = true;
				return result;
			}
		}
	}

	json errResp;
	errResp["error"]["code"] = -32602;
	errResp["error"]["message"] = "Unknown tool: " + toolName;
	return errResp;
}

json CMCPServer::HandleResourcesList()
{
	json result;
	json resList = json::array();

	for (const auto &res : resources)
	{
		json r;
		r["uri"] = res.uri;
		r["name"] = res.name;
		if (!res.description.empty()) r["description"] = res.description;
		if (!res.mimeType.empty()) r["mimeType"] = res.mimeType;
		resList.push_back(r);
	}

	result["resources"] = resList;
	return result;
}

json CMCPServer::HandleResourcesRead(const json &params)
{
	string uri = params.value("uri", "");

	for (const auto &res : resources)
	{
		if (res.uri == uri)
		{
			json result;
			json contents = json::array();
			json content;
			content["uri"] = uri;
			content["mimeType"] = res.mimeType.empty() ? "text/plain" : res.mimeType;
			content["text"] = res.readHandler();
			contents.push_back(content);
			result["contents"] = contents;
			return result;
		}
	}

	json errResp;
	errResp["error"]["code"] = -32602;
	errResp["error"]["message"] = "Unknown resource: " + uri;
	return errResp;
}

json CMCPServer::HandlePromptsList()
{
	json result;
	json promptList = json::array();

	for (const auto &p : prompts)
	{
		json pr;
		pr["name"] = p.name;
		if (!p.description.empty()) pr["description"] = p.description;
		if (!p.arguments.empty()) pr["arguments"] = p.arguments;
		promptList.push_back(pr);
	}

	result["prompts"] = promptList;
	return result;
}

json CMCPServer::HandlePromptsGet(const json &params)
{
	string promptName = params.value("name", "");
	json promptArgs = params.value("arguments", json::object());

	for (const auto &p : prompts)
	{
		if (p.name == promptName)
		{
			return p.getHandler(promptArgs);
		}
	}

	json errResp;
	errResp["error"]["code"] = -32602;
	errResp["error"]["message"] = "Unknown prompt: " + promptName;
	return errResp;
}

void CMCPServer::RegisterTool(const MCPToolDescriptor &tool)
{
	lock_guard<mutex> lock(toolsMutex);
	tools.push_back(tool);
}

void CMCPServer::RegisterResource(const MCPResourceDescriptor &resource)
{
	resources.push_back(resource);
}

void CMCPServer::RegisterPrompt(const MCPPromptDescriptor &prompt)
{
	prompts.push_back(prompt);
}

// Consume the buffer returned by CDebuggerServer::RunEndpointFunction and fail
// loudly when the endpoint did not succeed.
//
// Endpoint results are the JSON envelope built by PrepareResult:
//     {"status": <http code>, "token": ..., "result": {...}}
// while the bridge client answers with a flat
//     {"status": <http code>, "error": ..., "message": ...}
// when no desktop instance is attached.
//
// Command tools used to throw this buffer away and report a hardcoded success
// ("loaded", "paused", ...), so a load of a path that does not exist from the
// Retro Debugger process' point of view -- a Linux-style path handed to a
// Windows build, say -- answered {"status":"loaded"} while the drive reported
// error 21. Throwing here turns the endpoint failure into an MCP isError
// response instead.
//
// Takes ownership of the buffer and returns the envelope's "result" object.
// Only for endpoints without binary output: a binary payload follows the JSON
// after a 0 byte and would not parse.
static json ConsumeEndpointResult(vector<char> *result, const char *toolName)
{
	string action = toolName;

	if (!result || result->empty())
	{
		delete result;
		throw runtime_error(action + " failed: endpoint returned no data");
	}

	string raw(result->data(), result->size());
	delete result;

	json parsed;
	try
	{
		parsed = json::parse(raw);
	}
	catch (const exception &e)
	{
		throw runtime_error(action + " failed: malformed endpoint response: " + e.what());
	}

	int status = parsed.value("status", 0);
	if (status != HTTP_OK)
	{
		// Endpoint errors live under "result", bridge transport errors at the top level
		string message;
		if (parsed.contains("result") && parsed["result"].is_object())
			message = parsed["result"].value("error", string());
		if (message.empty())
		{
			string code = parsed.value("error", string());
			string detail = parsed.value("message", string());
			if (!code.empty() && !detail.empty())
				message = code + ": " + detail;
			else
				message = code.empty() ? detail : code;
		}
		if (message.empty())
			message = "endpoint returned status " + to_string(status);

		throw runtime_error(action + " failed: " + message);
	}

	return parsed.value("result", json::object());
}

// Register all debugger endpoints as MCP tools
void CMCPServer::RegisterDebuggerTools(CDebuggerServer *server)
{
	// In bridge mode retro_shutdown is registered as a bridge-local tool
	// instead, so it survives ClearDebuggerTools() and stays callable while
	// the desktop app is gone -- which is exactly when an orphaned bridge
	// needs shutting down.
	if (!isBridgeMode)
	{
		RegisterShutdownTool(server);
	}

	// Platform listing
	{
		MCPToolDescriptor tool;
		tool.name = "retro_list_platforms";
		tool.description = "List all active emulator platforms and their state";
		tool.inputSchema = {{"type", "object"}, {"properties", json::object()}};
		tool.handler = [this, server](const json &params) -> json
		{
			json result;
			json platforms = json::array();
			if (isBridgeMode && bridgeClient)
			{
				// In bridge mode, use remote platform data
				if (bridgeClient->GetState() == MCP_BRIDGE_CONNECTED)
				{
					result["platforms"] = bridgeClient->remotePlatforms;
				}
				else
				{
					result["platforms"] = json::array();
					result["connectionState"] = bridgeClient->GetStateString();
				}
				return result;
			}
			for (auto *di : viewC64->debugInterfaces)
			{
				json p;
				p["name"] = di->GetPlatformNameEndpointString();
				p["fullName"] = di->GetPlatformNameString();
				p["running"] = di->isRunning;
				platforms.push_back(p);
			}
			result["platforms"] = platforms;
			return result;
		};
		RegisterTool(tool);
	}

	// CPU status
	{
		MCPToolDescriptor tool;
		tool.name = "retro_cpu_status";
		tool.description = "Get CPU registers and status for a platform";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/status";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found or returned empty: " + fn);
			}
			string jsonStr(result->data(), result->size());
			delete result;
			return json::parse(jsonStr);
		};
		RegisterTool(tool);
	}

	// Force PC jump to address
	{
		MCPToolDescriptor tool;
		tool.name = "retro_cpu_jump";
		tool.description = "Force the CPU program counter to jump to an address. "
						   "Returns status \"jumped\" with applied=true once the new PC has been "
						   "committed to the CPU, so a following step executes at the target. "
						   "Returns status \"queued\" with applied=false if the change could not be "
						   "committed (emulation thread not running, or backend unsupported).";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"address",  {{"type", "integer"}, {"description", "Target address"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			json ep;
			ep["address"] = params.at("address");
			json res = ConsumeEndpointResult(server->RunEndpointFunction(platform + "/cpu/makejmp", "", ep, nullptr, 0), "retro_cpu_jump");
			bool applied = res.value("applied", false);
			if (!applied)
			{
				return {{"status", "queued"}, {"requestedPc", params.at("address")}, {"applied", false}};
			}
			return {{"status", "jumped"}, {"address", params.at("address")}, {"applied", true}};
		};
		RegisterTool(tool);
	}

	// Read CPU counters
	{
		MCPToolDescriptor tool;
		tool.name = "retro_cpu_counters";
		tool.description = "Read CPU cycle, instruction, and frame counters";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			vector<char> *result = server->RunEndpointFunction(platform + "/cpu/counters/read", "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("cpu counters read failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Memory read
	{
		MCPToolDescriptor tool;
		tool.name = "retro_memory_read";
		tool.description = "Read a block of memory from emulator (with I/O mapping)";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}},
				{"address", {{"type", "integer"}, {"description", "Start address"}}},
				{"length", {{"type", "integer"}, {"description", "Number of bytes to read"}}}
			}},
			{"required", json::array({"platform", "address", "length"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/memory/readBlock";
			// Accept both "length" and "size" for robustness
			int size = params.contains("length") ? params.at("length").get<int>()
					 : params.contains("size")   ? params.at("size").get<int>()
					 : 1;
			json endpointParams;
			endpointParams["address"] = params.at("address");
			endpointParams["size"] = size;
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			// Result has JSON + null + binary. Parse JSON, encode binary as base64.
			string raw(result->data(), result->size());
			delete result;

			auto nullPos = raw.find('\0');
			if (nullPos != string::npos && nullPos + 1 < raw.size())
			{
				string jsonPart = raw.substr(0, nullPos);
				string binaryPart = raw.substr(nullPos + 1);

				json parsed = json::parse(jsonPart);
				parsed["result"]["data"] = Base64Encode((const unsigned char *)binaryPart.data(), binaryPart.size());
				parsed["result"]["encoding"] = "base64";
				parsed["result"]["byteCount"] = (int)binaryPart.size();
				return parsed;
			}
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Memory write (accepts base64-encoded data)
	{
		MCPToolDescriptor tool;
		tool.name = "retro_memory_write";
		tool.description = "Write a block of memory to emulator (base64-encoded data)";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}},
				{"address", {{"type", "integer"}, {"description", "Start address"}}},
				{"data", {{"type", "string"}, {"description", "Base64-encoded binary data to write"}}}
			}},
			{"required", json::array({"platform", "address", "data"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/memory/writeBlock";
			string b64data = params.at("data").get<string>();
			vector<unsigned char> decoded = Base64Decode(b64data);
			json endpointParams;
			endpointParams["address"] = params.at("address");
			endpointParams["size"] = (int)decoded.size();
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, decoded.data(), (int)decoded.size());
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;
			json parsed = json::parse(raw);
			parsed["result"]["bytesWritten"] = (int)decoded.size();
			return parsed;
		};
		RegisterTool(tool);
	}

	// Pause
	{
		MCPToolDescriptor tool;
		tool.name = "retro_pause";
		tool.description = "Pause emulation";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/pause";
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", json(), nullptr, 0), "retro_pause");
			return {{"status", "paused"}};
		};
		RegisterTool(tool);
	}

	// Continue
	{
		MCPToolDescriptor tool;
		tool.name = "retro_continue";
		tool.description = "Resume emulation";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/continue";
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", json(), nullptr, 0), "retro_continue");
			return {{"status", "running"}};
		};
		RegisterTool(tool);
	}

	// Reset
	{
		MCPToolDescriptor tool;
		tool.name = "retro_reset";
		tool.description = "Reset the emulated machine";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}},
				{"hard", {{"type", "boolean"}, {"description", "Hard reset (true) or soft reset (false)"}, {"default", true}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			bool hard = params.value("hard", true);
			string fn = platform + (hard ? "/reset/hard" : "/reset/soft");
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", json(), nullptr, 0), "retro_reset");
			return {{"status", "reset"}, {"hard", hard}};
		};
		RegisterTool(tool);
	}

	// Warp speed
	{
		MCPToolDescriptor tool;
		tool.name = "retro_warp";
		tool.description = "Enable or disable warp speed (run as fast as possible, no frame-rate limit)";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"enabled",  {{"type", "boolean"}, {"description", "true to enable warp, false to disable"}}}
			}},
			{"required", json::array({"platform", "enabled"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			json ep;
			ep["warp"] = params.at("enabled").get<bool>();
			ConsumeEndpointResult(server->RunEndpointFunction(platform + "/warp/set", "", ep, nullptr, 0), "retro_warp");
			return {{"status", "ok"}, {"warp", params.at("enabled")}};
		};
		RegisterTool(tool);
	}

	// Detach all media
	{
		MCPToolDescriptor tool;
		tool.name = "retro_media_detach";
		tool.description = "Detach all cartridges, disks, and tapes from the emulator";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			ConsumeEndpointResult(server->RunEndpointFunction(platform + "/detachEverything", "", json(), nullptr, 0), "retro_media_detach");
			return {{"status", "detached"}};
		};
		RegisterTool(tool);
	}

	// Detach disk image only (no reset)
	{
		MCPToolDescriptor tool;
		tool.name = "retro_disk_detach";
		tool.description = "Detach the disk image from one drive WITHOUT resetting the machine (the GUI 'Detach Disk Image' action). RAM, registers and execution state are preserved. Use this - not retro_media_detach - when swapping or removing a disk mid-session: retro_media_detach power-cycles the C64.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800)"}}},
				{"device", {{"type", "integer"}, {"description", "Drive/device number: C64 8-11 (default 8), Atari 1-8 (default 1)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			json ep;
			if (params.contains("device"))
			{
				ep["device"] = params.at("device");
			}
			return ConsumeEndpointResult(server->RunEndpointFunction(platform + "/detachDiskImage", "", ep, nullptr, 0), "retro_disk_detach");
		};
		RegisterTool(tool);
	}

	// Start platform
	{
		MCPToolDescriptor tool;
		tool.name = "retro_start_platform";
		tool.description = "Start the emulation thread for a platform (e.g. atari800, nes, c64)";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.at("platform").get<string>();
			string fn = platform + "/emulator/start";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; return {{"status", "error"}}; }
			string raw(result->data(), result->size());
			delete result;
			json parsed = json::parse(raw);
			return parsed.value("result", json::object());
		};
		RegisterTool(tool);
	}

	// Stop platform
	{
		MCPToolDescriptor tool;
		tool.name = "retro_stop_platform";
		tool.description = "Stop the emulation thread for a platform";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.at("platform").get<string>();
			string fn = platform + "/emulator/stop";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; return {{"status", "error"}}; }
			string raw(result->data(), result->size());
			delete result;
			json parsed = json::parse(raw);
			return parsed.value("result", json::object());
		};
		RegisterTool(tool);
	}

	// Breakpoint add
	{
		MCPToolDescriptor tool;
		tool.name = "retro_breakpoint_add";
		tool.description = "Add a CPU breakpoint at an address";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"address", {{"type", "integer"}, {"description", "Address for breakpoint"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/breakpoint/add";
			json ep;
			ep["addr"] = params.at("address");
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", ep, nullptr, 0), "retro_breakpoint_add");
			return {{"status", "breakpoint_added"}, {"address", params.at("address")}};
		};
		RegisterTool(tool);
	}

	// Breakpoint remove
	{
		MCPToolDescriptor tool;
		tool.name = "retro_breakpoint_remove";
		tool.description = "Remove a CPU breakpoint at an address";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"address", {{"type", "integer"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/breakpoint/remove";
			json ep;
			ep["addr"] = params.at("address");
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", ep, nullptr, 0), "retro_breakpoint_remove");
			return {{"status", "breakpoint_removed"}, {"address", params.at("address")}};
		};
		RegisterTool(tool);
	}

	// Memory write/read breakpoint add
	{
		MCPToolDescriptor tool;
		tool.name = "retro_memory_breakpoint_add";
		tool.description = "Add a memory access breakpoint (write or read) at an address. "
			"Pauses emulation when the condition is met. "
			"access: \"write\" (default) or \"read\". "
			"comparison: \"==\", \"!=\", \"<\", \"<=\", \">\", \">=\" (default \">=\"). "
			"value: comparison value 0-255 (default 0). Use comparison \">=\", value 0 to break on any write.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform",   {{"type", "string"}}},
				{"address",    {{"type", "integer"}, {"description", "Memory address to watch"}}},
				{"access",     {{"type", "string"},  {"description", "\"write\" (default) or \"read\""}}},
				{"comparison", {{"type", "string"},  {"description", "\"==\", \"!=\", \"<\", \"<=\", \">\", \">=\" — default \">=\""}}},
				{"value",      {{"type", "integer"}, {"description", "Comparison value 0-255, default 0"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/memory/breakpoint/add";
			json ep;
			ep["addr"]       = params.at("address");
			ep["access"]     = params.value("access", "write");
			ep["comparison"] = params.value("comparison", ">=");
			ep["value"]      = params.value("value", 0);
			vector<char> *result = server->RunEndpointFunction(fn, "", ep, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("memory breakpoint add failed"); }
			string raw(result->data(), result->size());
			delete result;
			json resp = json::parse(raw);
			return resp;
		};
		RegisterTool(tool);
	}

	// Memory breakpoint remove
	{
		MCPToolDescriptor tool;
		tool.name = "retro_memory_breakpoint_remove";
		tool.description = "Remove a memory access breakpoint at an address";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"address",  {{"type", "integer"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/memory/breakpoint/remove";
			json ep;
			ep["addr"] = params.at("address");
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", ep, nullptr, 0), "retro_memory_breakpoint_remove");
			return {{"status", "memory_breakpoint_removed"}, {"address", params.at("address")}};
		};
		RegisterTool(tool);
	}

	// Memory breakpoint list
	{
		MCPToolDescriptor tool;
		tool.name = "retro_memory_breakpoint_list";
		tool.description = "List all memory access (write/read) breakpoints for a platform";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			vector<char> *result = server->RunEndpointFunction(platform + "/cpu/memory/breakpoint/list", "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("memory breakpoint list failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Breakpoint list
	{
		MCPToolDescriptor tool;
		tool.name = "retro_breakpoint_list";
		tool.description = "List all CPU breakpoints for a platform";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/breakpoint/list";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; return json::object(); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Machine state
	{
		MCPToolDescriptor tool;
		tool.name = "retro_machine_state";
		tool.description = "Get machine state for a platform";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/state";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; return json::object(); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Step instruction
	{
		MCPToolDescriptor tool;
		tool.name = "retro_step_instruction";
		tool.description = "Step one CPU instruction";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/step/instruction";
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", json(), nullptr, 0), "retro_step_instruction");
			return {{"status", "stepped"}};
		};
		RegisterTool(tool);
	}

	// Step one CPU cycle
	{
		MCPToolDescriptor tool;
		tool.name = "retro_step_cycle";
		tool.description = "Step one CPU clock cycle";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			ConsumeEndpointResult(server->RunEndpointFunction(platform + "/step/cycle", "", json(), nullptr, 0), "retro_step_cycle");
			return {{"status", "stepped_cycle"}};
		};
		RegisterTool(tool);
	}

	// Step over subroutine
	{
		MCPToolDescriptor tool;
		tool.name = "retro_step_subroutine";
		tool.description = "Step over a subroutine call (JSR) — runs until the called routine returns";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			ConsumeEndpointResult(server->RunEndpointFunction(platform + "/step/subroutine", "", json(), nullptr, 0), "retro_step_subroutine");
			return {{"status", "stepped_subroutine"}};
		};
		RegisterTool(tool);
	}

	// Load file
	{
		MCPToolDescriptor tool;
		tool.name = "retro_load";
		tool.description = "Load a program file (PRG, XEX, NES ROM, D64, CRT, etc.). The path is opened by the Retro Debugger process, so it must be valid on the machine running the debugger (on Windows that means a local path such as C:\\data\\game.d64), not on the MCP client's filesystem. Returns an error if the file does not exist there.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"path", {{"type", "string"}, {"description", "Path to file to load"}}}
			}},
			{"required", json::array({"path"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			json ep;
			ep["path"] = params.at("path");
			ConsumeEndpointResult(server->RunEndpointFunction("load", "", ep, nullptr, 0), "retro_load");
			return {{"status", "loaded"}, {"path", params.at("path")}};
		};
		RegisterTool(tool);
	}

	// Watch list
	{
		MCPToolDescriptor tool;
		tool.name = "retro_watch_list";
		tool.description = "List all watches for a platform";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/watch/list";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; return json::object(); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Watch add
	{
		MCPToolDescriptor tool;
		tool.name = "retro_watch_add";
		tool.description = "Add a watch at a memory address";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"address", {{"type", "integer"}, {"description", "Memory address to watch"}}},
				{"name", {{"type", "string"}, {"description", "Watch label (optional)"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/watch/add";
			json ep;
			ep["address"] = params.at("address");
			if (params.contains("name")) ep["name"] = params["name"];
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", ep, nullptr, 0), "retro_watch_add");
			return {{"status", "watch_added"}, {"address", params.at("address")}};
		};
		RegisterTool(tool);
	}

	// Watch remove
	{
		MCPToolDescriptor tool;
		tool.name = "retro_watch_remove";
		tool.description = "Remove a watch at a memory address";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"address", {{"type", "integer"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/watch/remove";
			json ep;
			ep["address"] = params.at("address");
			ConsumeEndpointResult(server->RunEndpointFunction(fn, "", ep, nullptr, 0), "retro_watch_remove");
			return {{"status", "watch_removed"}, {"address", params.at("address")}};
		};
		RegisterTool(tool);
	}

	// Segment read
	{
		MCPToolDescriptor tool;
		tool.name = "retro_segment_read";
		tool.description = "Get the current debug symbol segment name";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {{"platform", {{"type", "string"}}}}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			vector<char> *result = server->RunEndpointFunction(platform + "/segment/read", "", json(), nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("segment read failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Segment write
	{
		MCPToolDescriptor tool;
		tool.name = "retro_segment_write";
		tool.description = "Set the active debug symbol segment by name";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}}},
				{"segment",  {{"type", "string"}, {"description", "Segment name to activate"}}}
			}},
			{"required", json::array({"platform", "segment"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			json ep;
			ep["segment"] = params.at("segment").get<string>();
			ConsumeEndpointResult(server->RunEndpointFunction(platform + "/segment/write", "", ep, nullptr, 0), "retro_segment_write");
			return {{"status", "ok"}, {"segment", params.at("segment")}};
		};
		RegisterTool(tool);
	}

	// Snapshot save to file
	{
		MCPToolDescriptor tool;
		tool.name = "retro_snapshot_save";
		tool.description = "Save emulator state snapshot to a file. Returns the file path and size.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}},
				{"path", {{"type", "string"}, {"description", "File path to save snapshot to (default: /tmp/retrodebugger_snapshot.snap)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string path = params.value("path", "/tmp/retrodebugger_snapshot.snap");
			string fn = platform + "/snapshot/saveFile";
			json endpointParams;
			endpointParams["path"] = path;
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Snapshot save failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Snapshot load from file
	{
		MCPToolDescriptor tool;
		tool.name = "retro_snapshot_load";
		tool.description = "Load emulator state snapshot from a file.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name"}}},
				{"path", {{"type", "string"}, {"description", "File path to load snapshot from"}}}
			}},
			{"required", json::array({"platform", "path"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string path = params.at("path").get<string>();
			string fn = platform + "/snapshot/loadFile";
			json endpointParams;
			endpointParams["path"] = path;
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Snapshot load failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Quick snapshot save — saves to numbered slot (1-indexed, matches RetroDebugger UI)
	{
		MCPToolDescriptor tool;
		tool.name = "retro_snapshot_quick_save";
		tool.description = "Save emulator state to a quick snapshot slot (slot 1-9, matching the RetroDebugger UI numbering).";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"slot", {{"type", "integer"}, {"description", "Quick snapshot slot number (1-9, matches RetroDebugger UI)"}}}
			}},
			{"required", json::array({"platform", "slot"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/snapshot/quickSave";
			json endpointParams;
			endpointParams["slot"] = params.at("slot");
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Quick snapshot save failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Quick snapshot load — loads from numbered slot (1-indexed, matches RetroDebugger UI)
	{
		MCPToolDescriptor tool;
		tool.name = "retro_snapshot_quick_load";
		tool.description = "Load emulator state from a quick snapshot slot (slot 1-9, matching the RetroDebugger UI numbering).";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"slot", {{"type", "integer"}, {"description", "Quick snapshot slot number (1-9, matches RetroDebugger UI)"}}}
			}},
			{"required", json::array({"platform", "slot"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/snapshot/quickLoad";
			json endpointParams;
			endpointParams["slot"] = params.at("slot");
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Quick snapshot load failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// --- Input Tools ---

	// Joystick press/release
	{
		MCPToolDescriptor tool;
		tool.name = "retro_input_joystick";
		tool.description = "Press or release joystick directions/buttons. "
			"Directions: \"up\", \"down\", \"left\", \"right\", \"fire\", \"fire2\", \"start\", \"select\". "
			"Pass as array of strings or raw bitmask integer (N=1 S=2 W=4 E=8 FIRE=16 FIRE2=32 START=64 SELECT=128). "
			"action \"press\" = JoystickDown, \"release\" = JoystickUp.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform",  {{"type", "string"},  {"description", "Platform name (c64, atari800, nes)"}}},
				{"port",      {{"type", "integer"}, {"description", "Joystick port (0 or 1, default 0)"}}},
				{"axes",      {{"description", "Directions/buttons: array of strings (\"up\",\"down\",\"left\",\"right\",\"fire\",\"fire2\",\"start\",\"select\") or raw bitmask integer"}}},
				{"action",    {{"type", "string"},  {"description", "\"press\" or \"release\" (default \"press\")"}}}
			}},
			{"required", json::array({"platform", "axes"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			int port = params.value("port", 0);
			string action = params.value("action", "press");

			// Resolve axes — accept array of strings OR raw integer bitmask
			uint32_t axesMask = 0;
			const json &axesParam = params.at("axes");
			if (axesParam.is_number())
			{
				axesMask = axesParam.get<uint32_t>();
			}
			else if (axesParam.is_array())
			{
				for (const auto &a : axesParam)
				{
					string name = a.get<string>();
					if      (name == "up"     || name == "north") axesMask |= 0x01; // JOYPAD_N
					else if (name == "down"   || name == "south") axesMask |= 0x02; // JOYPAD_S
					else if (name == "left"   || name == "west")  axesMask |= 0x04; // JOYPAD_W
					else if (name == "right"  || name == "east")  axesMask |= 0x08; // JOYPAD_E
					else if (name == "fire"   || name == "a")     axesMask |= 0x10; // JOYPAD_FIRE
					else if (name == "fire2"  || name == "b")     axesMask |= 0x20; // JOYPAD_FIRE_B
					else if (name == "start")                     axesMask |= 0x40; // JOYPAD_START
					else if (name == "select")                    axesMask |= 0x80; // JOYPAD_SELECT
					else throw runtime_error("Unknown axis name: " + name);
				}
			}
			else if (axesParam.is_string())
			{
				// Single direction as string
				string name = axesParam.get<string>();
				if      (name == "up"     || name == "north") axesMask = 0x01;
				else if (name == "down"   || name == "south") axesMask = 0x02;
				else if (name == "left"   || name == "west")  axesMask = 0x04;
				else if (name == "right"  || name == "east")  axesMask = 0x08;
				else if (name == "fire"   || name == "a")     axesMask = 0x10;
				else if (name == "fire2"  || name == "b")     axesMask = 0x20;
				else if (name == "start")                     axesMask = 0x40;
				else if (name == "select")                    axesMask = 0x80;
				else throw runtime_error("Unknown axis name: " + name);
			}

			string fn = platform + (action == "release" ? "/input/joystickUp" : "/input/joystickDown");
			json endpointParams;
			endpointParams["port"] = port;
			endpointParams["axes"] = axesMask;
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Joystick input failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Read joystick state
	{
		MCPToolDescriptor tool;
		tool.name = "retro_input_joystick_state";
		tool.description = "Read current joystick state for a port. Returns bitmask and named active directions.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"},  {"description", "Platform name (c64, atari800, nes)"}}},
				{"port",     {{"type", "integer"}, {"description", "Joystick port (0 or 1, default 0)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			int port = params.value("port", 0);
			json endpointParams;
			endpointParams["port"] = port;
			vector<char> *result = server->RunEndpointFunction(platform + "/input/joystickState", "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Joystick state read failed"); }
			string raw(result->data(), result->size());
			delete result;
			json resp = json::parse(raw);
			// Annotate with named directions for convenience
			if (resp.contains("result") && resp["result"].contains("state"))
			{
				uint32_t state = resp["result"]["state"].get<uint32_t>();
				json active = json::array();
				if (state & 0x01) active.push_back("up");
				if (state & 0x02) active.push_back("down");
				if (state & 0x04) active.push_back("left");
				if (state & 0x08) active.push_back("right");
				if (state & 0x10) active.push_back("fire");
				if (state & 0x20) active.push_back("fire2");
				if (state & 0x40) active.push_back("start");
				if (state & 0x80) active.push_back("select");
				resp["result"]["active"] = active;
			}
			return resp;
		};
		RegisterTool(tool);
	}

	// Keyboard key press/release
	{
		MCPToolDescriptor tool;
		tool.name = "retro_input_key";
		tool.description = "Press or release a keyboard key. "
			"key accepts: single character string (\"a\", \"0\", \" \"), "
			"special name (\"space\", \"enter\", \"return\", \"backspace\", \"up\", \"down\", \"left\", \"right\", "
			"\"f1\"–\"f8\", \"lshift\", \"rshift\", \"lctrl\", \"rctrl\", \"lalt\", \"ralt\"), "
			"or raw integer SDL keycode. "
			"action: \"press\" or \"release\" (default \"press\").";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"key",      {{"description", "Key: single char string, special name, or integer SDL keycode"}}},
				{"action",   {{"type", "string"}, {"description", "\"press\" or \"release\" (default \"press\")"}}}
			}},
			{"required", json::array({"platform", "key"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string action = params.value("action", "press");

			// Resolve key code
			uint32_t keyCode = 0;
			const json &keyParam = params.at("key");
			if (keyParam.is_number())
			{
				keyCode = keyParam.get<uint32_t>();
			}
			else
			{
				string keyStr = keyParam.get<string>();
				if (keyStr.length() == 1)
				{
					// Single printable character — ASCII code
					keyCode = (uint32_t)(unsigned char)keyStr[0];
				}
				else
				{
					// Special key names → SDL keycodes
					// SDL keycodes for common keys (matches MTKEY_* values)
					if      (keyStr == "space" || keyStr == "spacebar") keyCode = 0x20;     // SDLK_SPACE
					else if (keyStr == "enter" || keyStr == "return")   keyCode = 0x0D;     // SDLK_RETURN (~MTKEY_ENTER)
					else if (keyStr == "backspace")                     keyCode = 0x08;     // SDLK_BACKSPACE
					else if (keyStr == "tab")                           keyCode = 0x09;
					else if (keyStr == "escape" || keyStr == "esc")     keyCode = 0x1B;
					else if (keyStr == "up")    keyCode = SDLK_UP;
					else if (keyStr == "down")  keyCode = SDLK_DOWN;
					else if (keyStr == "left")  keyCode = SDLK_LEFT;
					else if (keyStr == "right") keyCode = SDLK_RIGHT;
					else if (keyStr == "delete" || keyStr == "del") keyCode = SDLK_DELETE;
					else if (keyStr == "insert")    keyCode = SDLK_INSERT;
					else if (keyStr == "home")      keyCode = SDLK_HOME;
					else if (keyStr == "end")       keyCode = SDLK_END;
					else if (keyStr == "pageup")    keyCode = SDLK_PAGEUP;
					else if (keyStr == "pagedown")  keyCode = SDLK_PAGEDOWN;
					else if (keyStr == "f1")  keyCode = SDLK_F1;
					else if (keyStr == "f2")  keyCode = SDLK_F2;
					else if (keyStr == "f3")  keyCode = SDLK_F3;
					else if (keyStr == "f4")  keyCode = SDLK_F4;
					else if (keyStr == "f5")  keyCode = SDLK_F5;
					else if (keyStr == "f6")  keyCode = SDLK_F6;
					else if (keyStr == "f7")  keyCode = SDLK_F7;
					else if (keyStr == "f8")  keyCode = SDLK_F8;
					else if (keyStr == "lshift")  keyCode = SDLK_LSHIFT;
					else if (keyStr == "rshift")  keyCode = SDLK_RSHIFT;
					else if (keyStr == "lalt")    keyCode = SDLK_LALT;
					else if (keyStr == "ralt")    keyCode = SDLK_RALT;
#ifdef __APPLE__
					else if (keyStr == "lctrl")   keyCode = SDLK_LGUI;
					else if (keyStr == "rctrl")   keyCode = SDLK_RGUI;
#else
					else if (keyStr == "lctrl")   keyCode = SDLK_LCTRL;
					else if (keyStr == "rctrl")   keyCode = SDLK_RCTRL;
#endif
					else throw runtime_error("Unknown key name: " + keyStr);
				}
			}

			string fn = platform + (action == "release" ? "/input/keyUp" : "/input/keyDown");
			json endpointParams;
			endpointParams["keyCode"] = keyCode;
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty()) { delete result; throw runtime_error("Key input failed"); }
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// --- 6502 Analysis Tools ---

	// Disassemble
	{
		MCPToolDescriptor tool;
		tool.name = "retro_disassemble";
		tool.description = "Disassemble memory as 6502/6510/2A03 instructions. Returns text-format disassembly with optional hex bytes, labels, and execute-code markers from CPU tracking.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"address", {{"type", "integer"}, {"description", "Start address to disassemble from"}}},
				{"count", {{"type", "integer"}, {"description", "Number of instructions to disassemble (default 64, max 512)"}}},
				{"includeBytes", {{"type", "boolean"}, {"description", "Include raw hex bytes column (default true)"}}},
				{"includeLabels", {{"type", "boolean"}, {"description", "Replace addresses with symbol labels where available (default true)"}}}
			}},
			{"required", json::array({"platform", "address"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/disassemble";
			json endpointParams;
			endpointParams["address"] = params.at("address");
			if (params.contains("count")) endpointParams["count"] = params["count"];
			if (params.contains("includeBytes")) endpointParams["includeBytes"] = params["includeBytes"];
			if (params.contains("includeLabels")) endpointParams["includeLabels"] = params["includeLabels"];
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Assemble
	{
		MCPToolDescriptor tool;
		tool.name = "retro_assemble";
		tool.description = "Assemble 6502 instructions and write them to memory. Accepts multiple instructions separated by newlines. Atomic: if any line fails, nothing is written.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"address", {{"type", "integer"}, {"description", "Start address to write assembled code"}}},
				{"code", {{"type", "string"}, {"description", "Assembly code (multiple instructions separated by newline). Example: \"LDA #$42\\nSTA $D020\\nRTS\""}}}
			}},
			{"required", json::array({"platform", "address", "code"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/assemble";
			json endpointParams;
			endpointParams["address"] = params.at("address");
			endpointParams["code"] = params.at("code");
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Code Map
	{
		MCPToolDescriptor tool;
		tool.name = "retro_code_map";
		tool.description = "Get memory regions identified as executable code by CPU tracking. Shows which address ranges have been executed, helping identify code vs data areas.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"startAddress", {{"type", "integer"}, {"description", "Start of scan range (default 0)"}}},
				{"endAddress", {{"type", "integer"}, {"description", "End of scan range (default 65535)"}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/codemap";
			json endpointParams;
			if (params.contains("startAddress")) endpointParams["startAddress"] = params["startAddress"];
			if (params.contains("endAddress")) endpointParams["endAddress"] = params["endAddress"];
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Memory value search
	{
		MCPToolDescriptor tool;
		tool.name = "retro_memory_search";
		tool.description = "Search RAM for all addresses containing a specific byte value. Use this to find game state variables (e.g. lives counter, score, level). Returns a list of matching addresses.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"value", {{"type", "integer"}, {"description", "Byte value to search for (0-255)"}}},
				{"startAddress", {{"type", "integer"}, {"description", "Start of search range (default 0)"}}},
				{"endAddress", {{"type", "integer"}, {"description", "End of search range (default 65535)"}}}
			}},
			{"required", json::array({"platform", "value"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/memory/search";
			json endpointParams;
			endpointParams["value"] = params.at("value");
			if (params.contains("startAddress")) endpointParams["startAddress"] = params["startAddress"];
			if (params.contains("endAddress")) endpointParams["endAddress"] = params["endAddress"];
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Search Pattern
	{
		MCPToolDescriptor tool;
		tool.name = "retro_search_pattern";
		tool.description = "Search memory for opcode patterns. Use mnemonic names with optional arguments: 'DEC ??' finds all DEC instructions, 'STA $0340' finds stores to specific address, 'LDA #??' finds all immediate loads. By default only searches executed code regions.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"pattern", {{"type", "string"}, {"description", "Opcode pattern: 'DEC ??', 'STA $0340', 'JSR $FFD2', 'LDA #??', etc."}}},
				{"startAddress", {{"type", "integer"}, {"description", "Start of search range (default 0)"}}},
				{"endAddress", {{"type", "integer"}, {"description", "End of search range (default 65535)"}}},
				{"executedOnly", {{"type", "boolean"}, {"description", "Only search in executed code regions (default true)"}}}
			}},
			{"required", json::array({"platform", "pattern"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");
			string fn = platform + "/cpu/search";
			json endpointParams;
			endpointParams["pattern"] = params.at("pattern");
			if (params.contains("startAddress")) endpointParams["startAddress"] = params["startAddress"];
			if (params.contains("endAddress")) endpointParams["endAddress"] = params["endAddress"];
			if (params.contains("executedOnly")) endpointParams["executedOnly"] = params["executedOnly"];
			vector<char> *result = server->RunEndpointFunction(fn, "", endpointParams, nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;
			return json::parse(raw);
		};
		RegisterTool(tool);
	}

	// Screenshot
	{
		MCPToolDescriptor tool;
		tool.name = "retro_screenshot";
		tool.description = "Capture the current emulator screen as a PNG image. If savePath is provided, saves PNG to that file and returns metadata only (no base64). Otherwise returns base64-encoded PNG inline. Use this to observe game state, verify patches, read on-screen text, or confirm the game has started or the player has died.";
		tool.inputSchema = {
			{"type", "object"},
			{"properties", {
				{"platform", {{"type", "string"}, {"description", "Platform name (c64, atari800, nes)"}}},
				{"savePath", {{"type", "string"}, {"description", "Optional: file path to save PNG (e.g. /tmp/screen.png). When set, returns metadata only instead of base64 data."}}}
			}},
			{"required", json::array({"platform"})}
		};
		tool.handler = [server](const json &params) -> json
		{
			string platform = params.value("platform", "c64");

			if (params.contains("savePath"))
			{
				// Save to file — no base64 overhead
				string path = params.at("savePath").get<string>();
				json ep;
				ep["path"] = path;
				vector<char> *result = server->RunEndpointFunction(platform + "/screen/save", "", ep, nullptr, 0);
				if (!result || result->empty()) { delete result; throw runtime_error("screen/save endpoint not found"); }
				string raw(result->data(), result->size());
				delete result;
				return json::parse(raw);
			}

			// Return base64 inline
			string fn = platform + "/screen/snapshot";
			vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
			if (!result || result->empty())
			{
				delete result;
				throw runtime_error("Endpoint not found: " + fn);
			}
			string raw(result->data(), result->size());
			delete result;

			auto nullPos = raw.find('\0');
			if (nullPos != string::npos && nullPos + 1 < raw.size())
			{
				string jsonPart = raw.substr(0, nullPos);
				string pngBinary = raw.substr(nullPos + 1);
				json parsed = json::parse(jsonPart);
				if (parsed.value("status", 0) == 200)
				{
					parsed["result"]["data"] = Base64Encode((const unsigned char *)pngBinary.data(), pngBinary.size());
					parsed["result"]["encoding"] = "base64";
					parsed["result"]["byteCount"] = (int)pngBinary.size();
				}
				return parsed;
			}
			return json::parse(raw);
		};
		RegisterTool(tool);
	}
}

void CMCPServer::RegisterStaticResources()
{
	// Platform capabilities
	{
		MCPResourceDescriptor res;
		res.uri = "retrodebugger://platforms";
		res.name = "Active Platforms";
		res.description = "List of active emulator platforms and their capabilities";
		res.mimeType = "application/json";
		res.readHandler = [this]() -> string
		{
			json result;
			if (isBridgeMode && bridgeClient)
			{
				result["platforms"] = bridgeClient->remotePlatforms;
				result["connectionState"] = bridgeClient->GetStateString();
			}
			else
			{
				json platforms = json::array();
				if (viewC64)
				{
					for (auto *di : viewC64->debugInterfaces)
					{
						json p;
						p["name"] = di->GetPlatformNameEndpointString();
						p["fullName"] = di->GetPlatformNameString();
						p["running"] = di->isRunning;
						platforms.push_back(p);
					}
				}
				result["platforms"] = platforms;
			}
			return result.dump(2);
		};
		RegisterResource(res);
	}

	// C64 memory map reference
	{
		MCPResourceDescriptor res;
		res.uri = "retrodebugger://reference/c64/memory-map";
		res.name = "C64 Memory Map";
		res.description = "Commodore 64 memory map reference";
		res.mimeType = "text/plain";
		res.readHandler = []() -> string
		{
			return
				"C64 Memory Map\n"
				"==============\n"
				"$0000-$00FF  Zero Page\n"
				"$0100-$01FF  Stack\n"
				"$0200-$03FF  OS/KERNAL work area\n"
				"$0400-$07FF  Screen RAM (default)\n"
				"$0800-$9FFF  BASIC program area\n"
				"$A000-$BFFF  BASIC ROM / RAM\n"
				"$C000-$CFFF  Upper RAM\n"
				"$D000-$D3FF  VIC-II registers\n"
				"$D400-$D7FF  SID registers\n"
				"$D800-$DBFF  Color RAM\n"
				"$DC00-$DCFF  CIA 1 (keyboard, joystick)\n"
				"$DD00-$DDFF  CIA 2 (serial, NMI)\n"
				"$E000-$FFFF  KERNAL ROM / RAM\n"
				"\n"
				"SID Registers ($D400-$D418):\n"
				"  $D400/$D401  Voice 1 Frequency\n"
				"  $D402/$D403  Voice 1 Pulse Width\n"
				"  $D404        Voice 1 Control\n"
				"  $D405/$D406  Voice 1 AD/SR\n"
				"  $D407-$D40D  Voice 2 (same layout)\n"
				"  $D40E-$D414  Voice 3 (same layout)\n"
				"  $D415/$D416  Filter Cutoff\n"
				"  $D417        Filter Control\n"
				"  $D418        Volume / Filter Mode\n";
		};
		RegisterResource(res);
	}

	// C64 1541 drive primer
	{
		MCPResourceDescriptor res;
		res.uri = "retrodebugger://reference/c64/drive-1541";
		res.name = "C64 1541 Drive Primer";
		res.description = "Commodore 1541 disk drive memory map and architecture reference";
		res.mimeType = "text/plain";
		res.readHandler = []() -> string
		{
			return
				"Commodore 1541 Disk Drive\n"
				"=========================\n"
				"The 1541 is an intelligent peripheral with its own 6502 CPU, 2KB RAM,\n"
				"16KB ROM, and two VIA 6522 chips. It communicates with the C64 via\n"
				"serial IEC bus.\n"
				"\n"
				"Memory Map:\n"
				"  $0000-$07FF  RAM (2KB)\n"
				"    $0000-$00FF  Zero Page (drive variables)\n"
				"    $0100-$01FF  Stack\n"
				"    $0200-$02FF  Command buffer\n"
				"    $0300-$07FF  Buffer areas (5 x 256-byte buffers)\n"
				"  $1800-$180F  VIA 1 (serial bus communication)\n"
				"  $1C00-$1C0F  VIA 2 (disk controller, read/write head)\n"
				"  $C000-$FFFF  ROM (16KB — DOS, GCR encoding, IEC protocol)\n"
				"\n"
				"VIA 1 Registers ($1800-$180F) — Serial Bus:\n"
				"  $1800  Port B: serial bus data/clock lines\n"
				"  $1801  Port A: serial bus ATN\n"
				"  $1802  DDRB\n"
				"  $1803  DDRA\n"
				"  $1804-$1807  Timer 1\n"
				"  $1808-$180B  Timer 2\n"
				"  $180C  Shift register\n"
				"  $180D  ACR (auxiliary control)\n"
				"  $180E  PCR (peripheral control)\n"
				"  $180F  IFR/IER (interrupt flags/enable)\n"
				"\n"
				"VIA 2 Registers ($1C00-$1C0F) — Disk Controller:\n"
				"  $1C00  Port B: motor control, LED, write protect, sync\n"
				"    Bit 0-1: Stepper motor phase\n"
				"    Bit 2:   Motor on/off\n"
				"    Bit 3:   LED on/off\n"
				"    Bit 4:   Write protect sense\n"
				"    Bit 5:   Density select (bits per track zone)\n"
				"    Bit 6:   Density select\n"
				"    Bit 7:   Sync detected\n"
				"  $1C01  Port A: data byte read/write from disk\n"
				"  $1C04-$1C07  Timer 1 (byte-ready timing)\n"
				"\n"
				"Disk Format (GCR):\n"
				"  35 tracks, 683 sectors (256 bytes each)\n"
				"  Track 18: Directory and BAM (Block Availability Map)\n"
				"  Zones: tracks 1-17 = 21 sectors, 18-24 = 19, 25-30 = 18, 31-35 = 17\n"
				"  Data is GCR-encoded (4 bits -> 5 bits) on disk surface\n"
				"\n"
				"Debugging Tips:\n"
				"  - Drive CPU runs at 1 MHz, same as C64 main CPU\n"
				"  - Use drive1541/ endpoints for drive-side debugging\n"
				"  - drive1541/cpu/status gets drive CPU registers\n"
				"  - drive1541/cpu/memory/readBlock reads drive RAM/ROM\n"
				"  - drive1541/cpu/counters/read for drive cycle counter\n";
		};
		RegisterResource(res);
	}

	// Atari memory map + chip primer
	{
		MCPResourceDescriptor res;
		res.uri = "retrodebugger://reference/atari800/memory-map";
		res.name = "Atari 800/XL/XE Memory Map + Chip Primer";
		res.description = "Atari 8-bit memory map and ANTIC/GTIA/POKEY/PIA register reference";
		res.mimeType = "text/plain";
		res.readHandler = []() -> string
		{
			return
				"Atari 800/XL/XE Memory Map\n"
				"==========================\n"
				"$0000-$00FF  Zero Page\n"
				"$0100-$01FF  Stack\n"
				"$0200-$02FF  OS variables\n"
				"$0300-$04FF  Misc OS areas, device handlers\n"
				"$0500-$057F  OS/User area\n"
				"$0580-$05FF  Floating-point workspace\n"
				"$0600-$06FF  Page 6 (user area, often ML routines)\n"
				"$0700-$9FFF  User RAM (BASIC or ML programs)\n"
				"$A000-$BFFF  BASIC ROM or RAM (bank-switched)\n"
				"$C000-$CFFF  OS ROM or RAM (XL/XE)\n"
				"$D000-$D0FF  GTIA (graphics)\n"
				"$D200-$D2FF  POKEY (sound, keyboard, serial, IRQ)\n"
				"$D300-$D3FF  PIA (joystick ports, memory banking)\n"
				"$D400-$D4FF  ANTIC (display list processor)\n"
				"$D800-$FFFF  OS ROM (float pkg, device handlers, kernel)\n"
				"\n"
				"ANTIC Registers ($D400-$D40F) — Display List Processor:\n"
				"  Write:\n"
				"    $D400 DMACTL  — DMA control (player/missile, display width)\n"
				"    $D401 CHACTL  — Character control (inverse, reflect)\n"
				"    $D402 DLISTL  — Display list pointer low\n"
				"    $D403 DLISTH  — Display list pointer high\n"
				"    $D404 HSCROL  — Horizontal scroll (0-15 color clocks)\n"
				"    $D405 VSCROL  — Vertical scroll (0-15 scan lines)\n"
				"    $D407 PMBASE  — Player/missile base address\n"
				"    $D409 CHBASE  — Character set base address\n"
				"    $D40A WSYNC   — Wait for horizontal sync (write any value)\n"
				"    $D40E NMIEN   — NMI enable (DLI, VBI, Reset)\n"
				"    $D40F NMIRES  — NMI reset\n"
				"  Read:\n"
				"    $D40B VCOUNT  — Vertical line counter (÷2)\n"
				"    $D40C PENH    — Light pen horizontal\n"
				"    $D40D PENV    — Light pen vertical\n"
				"    $D40F NMIST   — NMI status\n"
				"\n"
				"GTIA Registers ($D000-$D01F) — Graphics/Color:\n"
				"  Write:\n"
				"    $D000-$D003 HPOSP0-3  — Player horizontal positions\n"
				"    $D004-$D007 HPOSM0-3  — Missile horizontal positions\n"
				"    $D008-$D00B SIZEP0-3  — Player sizes\n"
				"    $D00C        SIZEM     — Missile sizes\n"
				"    $D00D-$D010 GRAFP0-3  — Player graphics data\n"
				"    $D011        GRAFM     — Missile graphics data\n"
				"    $D012-$D015 COLPM0-3  — Player/missile colors\n"
				"    $D016-$D019 COLPF0-3  — Playfield colors\n"
				"    $D01A        COLBK     — Background color\n"
				"    $D01B        PRIOR     — Priority/GTIA mode select\n"
				"    $D01D        GRACTL    — Graphics control\n"
				"    $D01E        HITCLR    — Collision clear\n"
				"    $D01F        CONSOL    — Console keys (Start/Select/Option)\n"
				"  Read:\n"
				"    $D000-$D003 M0PF-M3PF — Missile-playfield collisions\n"
				"    $D004-$D007 P0PF-P3PF — Player-playfield collisions\n"
				"    $D008-$D00B M0PL-M3PL — Missile-player collisions\n"
				"    $D00C-$D00F P0PL-P3PL — Player-player collisions\n"
				"\n"
				"POKEY Registers ($D200-$D20F) — Sound/IO:\n"
				"  Write:\n"
				"    $D200 AUDF1   — Audio frequency channel 1\n"
				"    $D201 AUDC1   — Audio control channel 1\n"
				"    $D202 AUDF2   — Audio frequency channel 2\n"
				"    $D203 AUDC2   — Audio control channel 2\n"
				"    $D204 AUDF3   — Audio frequency channel 3\n"
				"    $D205 AUDC3   — Audio control channel 3\n"
				"    $D206 AUDF4   — Audio frequency channel 4\n"
				"    $D207 AUDC4   — Audio control channel 4\n"
				"    $D208 AUDCTL  — Audio control (clock, filter, 16-bit)\n"
				"    $D20E IRQEN   — IRQ enable\n"
				"    $D20F SKCTL   — Serial port control\n"
				"  Read:\n"
				"    $D209 KBCODE  — Keyboard code\n"
				"    $D20A RANDOM  — Random number\n"
				"    $D20E IRQST   — IRQ status\n"
				"    $D20F SKSTAT  — Serial port status\n"
				"\n"
				"PIA Registers ($D300-$D303) — Joystick/Banking:\n"
				"  $D300 PORTA   — Joystick ports 1+2 (4 bits each)\n"
				"  $D301 PORTB   — Memory bank select (XL/XE), joystick 3+4 (800)\n"
				"  $D302 PACTL   — Port A control\n"
				"  $D303 PBCTL   — Port B control\n"
				"\n"
				"Debugging Tips:\n"
				"  - Use atari800/antic/read, /gtia/read, /pokey/read, /pia/read\n"
				"    with {\"registers\": [0, 1, 2, ...]} to read chip state\n"
				"  - Use atari800/antic/write etc. to modify registers\n"
				"  - Display list at DLISTL/H controls screen mode\n"
				"  - AUDCTL bit 0: 15kHz clock, bit 4: ch1+2 linked 16-bit\n";
		};
		RegisterResource(res);
	}

	// NES memory map + chip primer
	{
		MCPResourceDescriptor res;
		res.uri = "retrodebugger://reference/nes/memory-map";
		res.name = "NES Memory Map + Chip Primer";
		res.description = "Nintendo Entertainment System CPU/PPU/APU memory map and register reference";
		res.mimeType = "text/plain";
		res.readHandler = []() -> string
		{
			return
				"NES Memory Map + Chip Primer\n"
				"============================\n"
				"\n"
				"CPU Memory Map (64KB address space):\n"
				"  $0000-$07FF  RAM (2KB, mirrored at $0800-$1FFF)\n"
				"    $0000-$00FF  Zero Page\n"
				"    $0100-$01FF  Stack\n"
				"    $0200-$02FF  OAM DMA source (typically sprite data)\n"
				"  $2000-$2007  PPU registers (mirrored every 8 bytes to $3FFF)\n"
				"  $4000-$4017  APU and I/O registers\n"
				"  $4020-$5FFF  Expansion ROM / mapper registers\n"
				"  $6000-$7FFF  SRAM / battery-backed save RAM\n"
				"  $8000-$FFFF  PRG ROM (cartridge program, often bank-switched)\n"
				"    $FFFA-$FFFB  NMI vector\n"
				"    $FFFC-$FFFD  Reset vector\n"
				"    $FFFE-$FFFF  IRQ/BRK vector\n"
				"\n"
				"PPU Registers ($2000-$2007):\n"
				"  $2000 PPUCTRL   — NMI enable, sprite size, BG/sprite pattern\n"
				"    Bit 0-1: Base nametable address (0=$2000,1=$2400,2=$2800,3=$2C00)\n"
				"    Bit 2:   VRAM increment (0=+1 across, 1=+32 down)\n"
				"    Bit 3:   Sprite pattern table (0=$0000, 1=$1000)\n"
				"    Bit 4:   Background pattern table (0=$0000, 1=$1000)\n"
				"    Bit 5:   Sprite size (0=8x8, 1=8x16)\n"
				"    Bit 7:   Generate NMI on VBlank\n"
				"  $2001 PPUMASK   — Color emphasis, sprite/BG enable, clipping\n"
				"    Bit 1: Show BG in leftmost 8 pixels\n"
				"    Bit 2: Show sprites in leftmost 8 pixels\n"
				"    Bit 3: Show background\n"
				"    Bit 4: Show sprites\n"
				"    Bit 5-7: Color emphasis (R/G/B)\n"
				"  $2002 PPUSTATUS — VBlank flag, sprite 0 hit, overflow (read)\n"
				"    Bit 5: Sprite overflow\n"
				"    Bit 6: Sprite 0 hit\n"
				"    Bit 7: VBlank (cleared on read)\n"
				"  $2003 OAMADDR   — OAM address for $2004 access\n"
				"  $2004 OAMDATA   — OAM data read/write\n"
				"  $2005 PPUSCROLL — Scroll position (write x, then y)\n"
				"  $2006 PPUADDR   — PPU address (write high, then low)\n"
				"  $2007 PPUDATA   — PPU data read/write\n"
				"  $4014 OAMDMA    — OAM DMA (write page number to transfer 256 bytes)\n"
				"\n"
				"PPU Memory Map (16KB address space, accessible via $2006/$2007):\n"
				"  $0000-$0FFF  Pattern table 0 (CHR ROM/RAM, 4KB)\n"
				"  $1000-$1FFF  Pattern table 1 (CHR ROM/RAM, 4KB)\n"
				"  $2000-$23FF  Nametable 0 (tile map + attributes, 1KB)\n"
				"  $2400-$27FF  Nametable 1\n"
				"  $2800-$2BFF  Nametable 2 (often mirror)\n"
				"  $2C00-$2FFF  Nametable 3 (often mirror)\n"
				"  $3F00-$3F0F  Background palette (16 entries)\n"
				"  $3F10-$3F1F  Sprite palette (16 entries)\n"
				"\n"
				"APU Registers ($4000-$4017):\n"
				"  Square 1:\n"
				"    $4000  Duty, envelope, volume\n"
				"    $4001  Sweep unit\n"
				"    $4002  Timer low\n"
				"    $4003  Length counter, timer high\n"
				"  Square 2: $4004-$4007 (same layout)\n"
				"  Triangle:\n"
				"    $4008  Linear counter\n"
				"    $400A  Timer low\n"
				"    $400B  Length counter, timer high\n"
				"  Noise:\n"
				"    $400C  Envelope, volume\n"
				"    $400E  Mode, period\n"
				"    $400F  Length counter\n"
				"  DMC:\n"
				"    $4010  Flags, rate\n"
				"    $4011  Direct load\n"
				"    $4012  Sample address\n"
				"    $4013  Sample length\n"
				"  Control:\n"
				"    $4015  Channel enable/status (read: status, write: enable)\n"
				"    $4017  Frame counter (mode, IRQ inhibit)\n"
				"\n"
				"I/O Registers:\n"
				"  $4016  Controller 1 (read: serial data, write bit 0: strobe)\n"
				"  $4017  Controller 2 (read), Frame counter (write)\n"
				"\n"
				"FDS (Famicom Disk System):\n"
				"  Use nes/fds/status to check if loaded ROM is FDS\n"
				"  nes/fds/insert {disk, side} — insert disk\n"
				"  nes/fds/eject — eject disk\n"
				"  nes/fds/changeSide — flip to other side\n"
				"\n"
				"Debugging Tips:\n"
				"  - Use nes/ppu/read with {\"registers\": [0,1,2,...]} for PPU regs\n"
				"  - Use nes/ppu/clocks for scanline position (hClock, vClock)\n"
				"  - Use nes/ppu/nametable/readBlock for VRAM inspection\n"
				"  - Use nes/apu/read for sound register state\n"
				"  - Use nes/apu/mute to isolate individual channels\n"
				"  - NMI fires every VBlank — most game logic runs in NMI handler\n"
				"  - Sprite 0 hit ($2002 bit 6) is commonly used for split-screen\n";
		};
		RegisterResource(res);
	}
}

void CMCPServer::RegisterPrompts()
{
	// Inspect execution state
	{
		MCPPromptDescriptor prompt;
		prompt.name = "inspect_execution";
		prompt.description = "Examine the current CPU state and surrounding code context";
		prompt.arguments = json::array({
			{{"name", "platform"}, {"description", "Platform to inspect (c64, atari800, nes)"}, {"required", true}}
		});
		prompt.getHandler = [](const json &args) -> json
		{
			string platform = args.value("platform", "c64");
			json result;
			json messages = json::array();
			json msg;
			msg["role"] = "user";
			msg["content"]["type"] = "text";
			msg["content"]["text"] =
				"Please inspect the current execution state of the " + platform + " emulator.\n\n"
				"1. Use retro_cpu_status to get the current CPU registers\n"
				"2. Use retro_memory_read to read ~32 bytes around the PC\n"
				"3. Disassemble the code and explain what it's doing\n"
				"4. Check if there are any breakpoints set\n";
			messages.push_back(msg);
			result["messages"] = messages;
			return result;
		};
		RegisterPrompt(prompt);
	}

	// Debug workflow
	{
		MCPPromptDescriptor prompt;
		prompt.name = "debug_workflow";
		prompt.description = "Systematic debugging workflow for finding bugs";
		prompt.arguments = json::array({
			{{"name", "platform"}, {"description", "Platform"}, {"required", true}},
			{{"name", "issue"}, {"description", "Description of the issue"}, {"required", true}}
		});
		prompt.getHandler = [](const json &args) -> json
		{
			string platform = args.value("platform", "c64");
			string issue = args.value("issue", "unknown issue");
			json result;
			json messages = json::array();
			json msg;
			msg["role"] = "user";
			msg["content"]["type"] = "text";
			msg["content"]["text"] =
				"Debug this issue on " + platform + ": " + issue + "\n\n"
				"Steps:\n"
				"1. Use retro_machine_state to check if the emulator is running\n"
				"2. Use retro_cpu_status to examine registers\n"
				"3. Set breakpoints at suspected locations with retro_breakpoint_add\n"
				"4. Use retro_continue to run until breakpoint\n"
				"5. Use retro_memory_read to inspect relevant memory\n"
				"6. Analyze and report findings\n";
			messages.push_back(msg);
			result["messages"] = messages;
			return result;
		};
		RegisterPrompt(prompt);
	}
}

void CMCPServer::SetBridgeMode(CMCPBridgeClient *bridge)
{
	isBridgeMode = true;
	bridgeClient = bridge;

	// Wire state change callback
	bridge->SetStateCallback([this](MCPBridgeState oldState, MCPBridgeState newState)
	{
		OnBridgeStateChanged((int)oldState, (int)newState);
	});

	// Give the bridge a back-reference so it can forward desktop broadcast events
	// as MCP notifications to the LLM client.
	bridge->SetMCPServer(this);
}

void CMCPServer::OnBridgeStateChanged(int oldState, int newState)
{
	LOGD("CMCPServer::OnBridgeStateChanged: %d -> %d", oldState, newState);

	if (newState == MCP_BRIDGE_CONNECTED && oldState != MCP_BRIDGE_CONNECTED)
	{
		// Just connected — register debugger tools if needed
		if (!debuggerToolsRegistered && bridgeClient)
		{
			RegisterDebuggerTools(bridgeClient);
			debuggerToolsRegistered = true;
		}
	}
	else if (newState != MCP_BRIDGE_CONNECTED && oldState == MCP_BRIDGE_CONNECTED)
	{
		// Just disconnected — clear debugger tools so they can be re-registered on reconnect
		ClearDebuggerTools();
		debuggerToolsRegistered = false;
	}

	// Send list_changed notifications on any connected<->disconnected transition
	if (initialized)
	{
		bool wasConnected = (oldState == MCP_BRIDGE_CONNECTED);
		bool isConnected = (newState == MCP_BRIDGE_CONNECTED);
		if (wasConnected != isConnected)
		{
			SendNotification("notifications/tools/list_changed");
			SendNotification("notifications/resources/list_changed");
		}
	}
}

void CMCPServer::ClearDebuggerTools()
{
	lock_guard<mutex> lock(toolsMutex);
	// Remove all tools except bridge-local ones
	vector<MCPToolDescriptor> bridgeLocalTools;
	for (const auto &tool : tools)
	{
		if (tool.name == "retro_transport_diagnostics" ||
			tool.name == "retro_reconnect" ||
			tool.name == "retro_shutdown")
		{
			bridgeLocalTools.push_back(tool);
		}
	}
	tools = bridgeLocalTools;
}

void CMCPServer::RegisterShutdownTool(CDebuggerServer *server)
{
	MCPToolDescriptor tool;
	tool.name = "retro_shutdown";
	tool.description =
		"Shut down RetroDebugger. Quits through the normal File > Quit path, so settings, "
		"window layouts, symbols and plugin state are saved. This ends the session: no further "
		"tool call will succeed afterwards. Only call it when the user asked to close RetroDebugger.";
	tool.inputSchema = {
		{"type", "object"},
		{"properties", {
			{"force", {{"type", "boolean"}, {"description", "Use a shorter watchdog deadline (1500ms instead of 8000ms). Still runs the full graceful path and still saves state. Default false."}}},
			{"timeoutMs", {{"type", "integer"}, {"description", "Watchdog deadline in ms before the process is forced to exit. Overrides the deadline implied by force."}}},
			{"exitBridge", {{"type", "boolean"}, {"description", "In bridge mode (--mcp-live), also exit this local bridge process after the desktop app quits, leaving nothing behind. Default true. Ignored outside bridge mode."}}},
			{"reason", {{"type", "string"}, {"description", "Free-form reason, recorded in the RetroDebugger log"}}}
		}}
	};
	tool.handler = [this, server](const json &params) -> json
	{
		C64DShutdownRequest request = C64DParseShutdownParams(params, "mcp:retro_shutdown");

		bool exitBridge = true;
		if (params.contains("exitBridge") && params["exitBridge"].is_boolean())
			exitBridge = params["exitBridge"].get<bool>();

		json endpointParams;
		endpointParams["force"] = request.force;
		endpointParams["timeoutMs"] = request.timeoutMs;
		endpointParams["graceMs"] = request.graceMs;
		endpointParams["reason"] = request.reason;

		// Ask the application that owns the emulator to quit. Outside bridge
		// mode that is this very process; in bridge mode the call travels over
		// the websocket to the desktop instance.
		//
		// In bridge mode resolve the client at call time rather than trusting
		// the pointer captured at registration: SetBridgeMode() can swap in a
		// new CMCPBridgeClient on reconnect, and this tool outlives that swap
		// because it survives ClearDebuggerTools(). The other bridge-local
		// tools read this->bridgeClient for the same reason.
		CDebuggerServer *target = isBridgeMode ? (CDebuggerServer *)bridgeClient : server;

		json remote;
		bool remoteReachable = false;
		if (target != NULL)
		{
			vector<char> *raw = target->RunEndpointFunction("server/shutdown", "", endpointParams, NULL, 0);
			if (raw != NULL && !raw->empty())
			{
				string rawStr(raw->data(), raw->size());
				// Strip any binary payload separator before parsing.
				size_t nullPos = rawStr.find('\0');
				if (nullPos != string::npos)
					rawStr = rawStr.substr(0, nullPos);
				try
				{
					remote = json::parse(rawStr);
					remoteReachable = (remote.value("status", 0) == HTTP_OK);
				}
				catch (const exception &e)
				{
					remote = {{"error", "malformed_response"}, {"details", e.what()}};
				}
			}
			delete raw;
		}

		json result;
		result["status"] = "shutting_down";
		// A second request against an app that is already quitting is still a
		// success -- it asked for the app to stop and the app is stopping --
		// but say so rather than implying this call is what started it.
		if (remoteReachable && remote.contains("result")
			&& remote["result"].value("status", string()) == "already_shutting_down")
		{
			result["status"] = "already_shutting_down";
		}
		result["force"] = request.force;
		result["timeoutMs"] = request.timeoutMs;
		result["target"] = isBridgeMode ? "desktop" : "application";
		result["remoteReachable"] = remoteReachable;
		if (!remote.is_null())
			result["remote"] = remote;

		bool bridgeExiting = false;
		if (isBridgeMode && exitBridge)
		{
			// Take the bridge down too, so one call leaves no process behind.
			// Deferred like everything else here, so this tool's JSON-RPC
			// response still gets written to stdout before we exit under it.
			// Note this runs even when the desktop was unreachable -- cleaning
			// up an orphaned bridge is precisely that case.
			C64DShutdownRequest bridgeRequest;
			bridgeRequest.force = request.force;
			bridgeRequest.reason = "bridge:retro_shutdown";
			bridgeExiting = C64DRequestShutdown(bridgeRequest);
			if (!bridgeExiting)
			{
				// Already scheduled by an earlier call; it is still exiting.
				bridgeExiting = C64DIsShutdownPending();
			}
		}
		result["bridgeExiting"] = bridgeExiting;

		if (isBridgeMode && !remoteReachable)
		{
			result["status"] = bridgeExiting ? "desktop_unavailable_bridge_exiting" : "desktop_unavailable";
		}

		return result;
	};
	RegisterTool(tool);
}

void CMCPServer::RegisterBridgeLocalTools()
{
	RegisterShutdownTool(bridgeClient);

	// Transport diagnostics — always available
	{
		MCPToolDescriptor tool;
		tool.name = "retro_transport_diagnostics";
		tool.description = "Check bridge connection state and diagnostics for the remote RetroDebugger instance";
		tool.inputSchema = {{"type", "object"}, {"properties", json::object()}};
		tool.handler = [this](const json &params) -> json
		{
			if (bridgeClient)
				return bridgeClient->GetDiagnostics();
			return {{"error", "bridge not configured"}};
		};
		RegisterTool(tool);
	}

	// Force reconnect
	{
		MCPToolDescriptor tool;
		tool.name = "retro_reconnect";
		tool.description = "Force an immediate reconnect attempt to the RetroDebugger desktop instance";
		tool.inputSchema = {{"type", "object"}, {"properties", json::object()}};
		tool.handler = [this](const json &params) -> json
		{
			if (!bridgeClient)
				return {{"error", "bridge not configured"}};
			bool ok = bridgeClient->TryConnect();
			if (ok)
			{
				ok = bridgeClient->RunDiscovery();
			}
			json result;
			result["connected"] = ok;
			result["state"] = bridgeClient->GetStateString();
			if (!ok)
				result["diagnostics"] = bridgeClient->GetDiagnostics();
			return result;
		};
		RegisterTool(tool);
	}
}

static void MCP_SignalHandler(int sig)
{
	if (mcpServer)
		mcpServer->Stop();
	SYS_Shutdown();
}

void MCP_ServerStart()
{
	if (mcpServer == NULL)
	{
		mcpServer = new CMCPServer();
	}

	// MCP can also be started from the GUI after the debugger server exists.
	if (viewC64 && viewC64->debuggerServer)
	{
		mcpServer->SetDebuggerServer(viewC64->debuggerServer);
	}

	// Ignore SIGPIPE so broken stdout pipe doesn't kill the process
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	// Install signal handlers so Ctrl+C cleanly stops the MCP server
	signal(SIGINT, MCP_SignalHandler);
	signal(SIGTERM, MCP_SignalHandler);

	mcpServer->Start();
}

void MCP_ServerStop()
{
	if (mcpServer)
	{
		mcpServer->Stop();
	}
}

void MCP_BridgeStart(const char *host, int port, const char *path)
{
	CMCPBridgeClient *bridge = new CMCPBridgeClient(
		host ? host : "127.0.0.1",
		port > 0 ? port : 0x0DEB,
		path ? path : "/stream"
	);

	if (mcpServer == NULL)
	{
		mcpServer = new CMCPServer();
	}

	mcpServer->SetBridgeMode(bridge);

	// Ignore SIGPIPE so broken stdout/socket pipe doesn't kill the process
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	// Install signal handlers
	signal(SIGINT, MCP_SignalHandler);
	signal(SIGTERM, MCP_SignalHandler);

	// Start the bridge background reconnect thread
	bridge->Start();

	// Start the MCP stdio server
	mcpServer->Start();
}
