#include "CTestMCPProtocol.h"
#include "CMCPServer.h"
#include "CDebuggerServer.h"
#include "CDebuggerServerProtocol.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "C64DShutdown.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace nlohmann;

static char failureMsg[512];

// --- Shutdown test doubles -------------------------------------------------

static C64DShutdownRequest sRecordedShutdown;
static int sRecordedShutdownCount = 0;

static void RecordShutdown(const C64DShutdownRequest &request)
{
	sRecordedShutdown = request;
	sRecordedShutdownCount++;
}

// Swaps in a recording executor so a shutdown request never actually takes the
// test runner down, and restores the real one on every exit path. Installing an
// executor also clears the pending flag, so this arms a clean slate.
struct CShutdownExecutorGuard
{
	CShutdownExecutorGuard(C64DShutdownExecutorFn executor)
	{
		sRecordedShutdownCount = 0;
		sRecordedShutdown = C64DShutdownRequest();
		C64DSetShutdownExecutor(executor);
	}
	~CShutdownExecutorGuard()
	{
		C64DSetShutdownExecutor(NULL);
	}
};

// The shutdown is scheduled on its own thread, so the executor runs
// asynchronously. Returns true once the expected number of calls landed.
static bool WaitForShutdownCalls(int expected, int timeoutMs)
{
	for (int elapsed = 0; elapsed < timeoutMs; elapsed += 10)
	{
		if (sRecordedShutdownCount >= expected)
			return true;
		SYS_Sleep(10);
	}
	return sRecordedShutdownCount >= expected;
}

// CMCPServer registers its tools lazily, and outside bridge mode it waits for
// viewC64->debuggerServer to show up before doing so. Make sure it is there, or
// every tool request below pays that ten second stall.
//
// Deliberately never stopped again: CDebuggerServerWebSockets does not survive
// a Start() after a Stop(), and later tests in the suite (CTestMCPBridge) need
// a server that is actually reachable on the port.
static void EnsureDebuggerServerPresent()
{
	if (viewC64 != NULL && viewC64->debuggerServer == NULL)
	{
		viewC64->DebuggerServerWebSocketsStart();
	}
}

// Stands in for the debugger server (or the bridge to a remote one) so the
// retro_shutdown tool can be exercised without a live websocket or a live app.
class CRecordingDebuggerServer : public CDebuggerServer
{
public:
	CRecordingDebuggerServer() : callCount(0) {}

	std::string lastFn;
	json lastParams;
	int callCount;

	virtual std::vector<char> *RunEndpointFunction(const std::string &endpointName, const std::string token,
												   json params, u8 *binaryData, int binaryDataSize)
	{
		lastFn = endpointName;
		lastParams = params;
		callCount++;

		json response;
		response["status"] = HTTP_OK;
		response["result"] = C64DShutdownRequestToJson(C64DParseShutdownParams(params, "test"));
		std::string raw = response.dump();
		return new std::vector<char>(raw.begin(), raw.end());
	}
};

// Answers every endpoint with one canned envelope, so the tool layer can be
// driven against endpoint failures without a live emulator behind it.
class CCannedDebuggerServer : public CDebuggerServer
{
public:
	CCannedDebuggerServer(const json &envelope) : envelope(envelope), callCount(0) {}

	json envelope;
	int callCount;

	virtual std::vector<char> *RunEndpointFunction(const std::string &endpointName, const std::string token,
												   json params, u8 *binaryData, int binaryDataSize)
	{
		callCount++;
		std::string raw = envelope.dump();
		return new std::vector<char>(raw.begin(), raw.end());
	}
};

// Drives one tool through the public JSON-RPC entry point.
static json CallTool(CMCPServer &server, int id, const char *toolName, const json &arguments)
{
	json request;
	request["jsonrpc"] = "2.0";
	request["id"] = id;
	request["method"] = "tools/call";
	request["params"]["name"] = toolName;
	request["params"]["arguments"] = arguments;
	return server.HandleRequest(request);
}

// The MCP error text a tool call carried, or an empty string when it succeeded.
static std::string ToolErrorText(const json &response)
{
	if (!response.contains("result") || !response["result"].value("isError", false))
		return std::string();
	return response["result"]["content"][0].value("text", std::string());
}

class CTestMCPDebuggerServer : public CDebuggerServer
{
public:
	CTestMCPDebuggerServer()
	{
		numCalls = 0;
	}

	virtual std::vector<char> *RunEndpointFunction(const std::string& endpointName, const std::string token, nlohmann::json params, u8 *binaryData, int binaryDataSize)
	{
		numCalls++;
		lastEndpointName = endpointName;

		json response;
		response["status"] = 200;
		response["result"]["pc"] = 0x1000;
		std::string responseStr = response.dump();
		return new std::vector<char>(responseStr.begin(), responseStr.end());
	}

	int numCalls;
	std::string lastEndpointName;
};

void CTestMCPProtocol::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	failureMsg[0] = '\0';

	// --- Test 1: RequestContext parsing from v1 request ---
	{
		json v1req;
		v1req["fn"] = "c64/cpu/status";
		v1req["token"] = "abc123";
		v1req["params"] = json::object();

		RequestContext ctx = DebuggerProtocol::ParseRequest(v1req);
		if (ctx.protocolVersion != 1)
		{
			sprintf(failureMsg, "Test 1 FAIL: v1 request parsed as version %d", ctx.protocolVersion);
			TestCompleted(false, failureMsg);
			return;
		}
		if (ctx.token != "abc123")
		{
			sprintf(failureMsg, "Test 1 FAIL: token='%s' expected 'abc123'", ctx.token.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 2: RequestContext parsing from v2 request ---
	{
		json v2req;
		v2req["protocolVersion"] = 2;
		v2req["requestId"] = "req-42";
		v2req["fn"] = "server/hello";
		v2req["params"] = json::object();

		RequestContext ctx = DebuggerProtocol::ParseRequest(v2req);
		if (ctx.protocolVersion != 2)
		{
			sprintf(failureMsg, "Test 2 FAIL: v2 request parsed as version %d", ctx.protocolVersion);
			TestCompleted(false, failureMsg);
			return;
		}
		if (ctx.requestId != "req-42")
		{
			sprintf(failureMsg, "Test 2 FAIL: requestId='%s' expected 'req-42'", ctx.requestId.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 3: MakeError produces valid error object ---
	{
		json err = DebuggerProtocol::MakeError("endpoint_not_found", "Not found");
		if (err["code"] != "endpoint_not_found" || err["message"] != "Not found")
		{
			sprintf(failureMsg, "Test 3 FAIL: error object malformed");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 4: MakeResponse v1 includes token ---
	{
		RequestContext ctx;
		ctx.protocolVersion = 1;
		ctx.token = "tok1";
		json resp = DebuggerProtocol::MakeResponse(ctx, 200, {{"data", "ok"}});
		if (resp["status"] != 200 || resp["token"] != "tok1" || !resp.contains("result"))
		{
			sprintf(failureMsg, "Test 4 FAIL: v1 response malformed");
			TestCompleted(false, failureMsg);
			return;
		}
		if (resp.contains("protocolVersion"))
		{
			sprintf(failureMsg, "Test 4 FAIL: v1 response should not contain protocolVersion");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 5: MakeResponse v2 includes protocolVersion and requestId ---
	{
		RequestContext ctx;
		ctx.protocolVersion = 2;
		ctx.requestId = "req-99";
		json resp = DebuggerProtocol::MakeResponse(ctx, 200, {{"data", "ok"}});
		if (!resp.contains("protocolVersion") || resp["protocolVersion"] != 2)
		{
			sprintf(failureMsg, "Test 5 FAIL: v2 response missing protocolVersion");
			TestCompleted(false, failureMsg);
			return;
		}
		if (resp["requestId"] != "req-99")
		{
			sprintf(failureMsg, "Test 5 FAIL: v2 response requestId mismatch");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 6: EndpointDescriptor serialization ---
	{
		EndpointDescriptor desc;
		desc.fn = "c64/cpu/status";
		desc.platform = "c64";
		desc.category = "cpu";
		desc.description = "Get CPU registers";
		desc.isStubbed = false;

		json j = desc.ToJson();
		if (j["fn"] != "c64/cpu/status" || j["platform"] != "c64" || j["category"] != "cpu")
		{
			sprintf(failureMsg, "Test 6 FAIL: descriptor serialization wrong");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 7: CMCPServer HandleInitialize returns correct protocol version ---
	{
		CMCPServer server;
		json initParams;
		initParams["protocolVersion"] = "2024-11-05";
		initParams["capabilities"] = json::object();
		initParams["clientInfo"]["name"] = "test";

		// Call HandleInitialize directly (it's public enough via HandleRequest)
		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 1;
		request["method"] = "initialize";
		request["params"] = initParams;

		json response = server.HandleRequest(request);
		if (!response.contains("result"))
		{
			sprintf(failureMsg, "Test 7 FAIL: initialize response missing result");
			TestCompleted(false, failureMsg);
			return;
		}
		if (response["result"]["protocolVersion"] != "2024-11-05")
		{
			sprintf(failureMsg, "Test 7 FAIL: protocol version mismatch");
			TestCompleted(false, failureMsg);
			return;
		}
		if (!response["result"].contains("capabilities"))
		{
			sprintf(failureMsg, "Test 7 FAIL: missing capabilities in init response");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 8: CMCPServer unknown method returns error ---
	{
		CMCPServer server;
		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 2;
		request["method"] = "nonexistent/method";
		request["params"] = json::object();

		json response = server.HandleRequest(request);
		if (!response.contains("error"))
		{
			sprintf(failureMsg, "Test 8 FAIL: unknown method should return error");
			TestCompleted(false, failureMsg);
			return;
		}
		if (response["error"]["code"] != -32601)
		{
			sprintf(failureMsg, "Test 8 FAIL: wrong error code %d", (int)response["error"]["code"]);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 9: shutdown request defaults to the graceful watchdog deadline ---
	{
		C64DShutdownRequest request;
		request.Normalize();
		if (request.force != false)
		{
			sprintf(failureMsg, "Test 9 FAIL: default request should not be forced");
			TestCompleted(false, failureMsg);
			return;
		}
		if (request.timeoutMs != C64D_SHUTDOWN_TIMEOUT_DEFAULT_MS)
		{
			sprintf(failureMsg, "Test 9 FAIL: default timeoutMs=%d expected %d",
					request.timeoutMs, C64D_SHUTDOWN_TIMEOUT_DEFAULT_MS);
			TestCompleted(false, failureMsg);
			return;
		}
		if (request.graceMs != C64D_SHUTDOWN_GRACE_DEFAULT_MS)
		{
			sprintf(failureMsg, "Test 9 FAIL: default graceMs=%d expected %d",
					request.graceMs, C64D_SHUTDOWN_GRACE_DEFAULT_MS);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 10: force only shortens the watchdog deadline ---
	{
		C64DShutdownRequest request;
		request.force = true;
		request.Normalize();
		if (request.timeoutMs != C64D_SHUTDOWN_TIMEOUT_FORCED_MS)
		{
			sprintf(failureMsg, "Test 10 FAIL: forced timeoutMs=%d expected %d",
					request.timeoutMs, C64D_SHUTDOWN_TIMEOUT_FORCED_MS);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 11: an explicit timeout survives normalization, forced or not ---
	{
		C64DShutdownRequest request;
		request.timeoutMs = 4321;
		request.Normalize();
		if (request.timeoutMs != 4321)
		{
			sprintf(failureMsg, "Test 11 FAIL: explicit timeoutMs overwritten with %d", request.timeoutMs);
			TestCompleted(false, failureMsg);
			return;
		}

		C64DShutdownRequest forced;
		forced.force = true;
		forced.timeoutMs = 4321;
		forced.Normalize();
		if (forced.timeoutMs != 4321)
		{
			sprintf(failureMsg, "Test 11 FAIL: explicit timeoutMs overwritten by force with %d", forced.timeoutMs);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 12: out-of-range values are clamped, not trusted ---
	{
		C64DShutdownRequest tooSmall;
		tooSmall.timeoutMs = 1;
		tooSmall.graceMs = -500;
		tooSmall.Normalize();
		if (tooSmall.timeoutMs != C64D_SHUTDOWN_TIMEOUT_MIN_MS || tooSmall.graceMs != C64D_SHUTDOWN_GRACE_MIN_MS)
		{
			sprintf(failureMsg, "Test 12 FAIL: low values not clamped (timeoutMs=%d graceMs=%d)",
					tooSmall.timeoutMs, tooSmall.graceMs);
			TestCompleted(false, failureMsg);
			return;
		}

		C64DShutdownRequest tooBig;
		tooBig.timeoutMs = 999999999;
		tooBig.graceMs = 999999999;
		tooBig.Normalize();
		if (tooBig.timeoutMs != C64D_SHUTDOWN_TIMEOUT_MAX_MS || tooBig.graceMs != C64D_SHUTDOWN_GRACE_MAX_MS)
		{
			sprintf(failureMsg, "Test 12 FAIL: high values not clamped (timeoutMs=%d graceMs=%d)",
					tooBig.timeoutMs, tooBig.graceMs);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 13: empty params parse to a normalized graceful request ---
	{
		C64DShutdownRequest request = C64DParseShutdownParams(json::object(), "unit-test");
		if (request.force != false || request.timeoutMs != C64D_SHUTDOWN_TIMEOUT_DEFAULT_MS)
		{
			sprintf(failureMsg, "Test 13 FAIL: empty params gave force=%d timeoutMs=%d",
					(int)request.force, request.timeoutMs);
			TestCompleted(false, failureMsg);
			return;
		}
		if (request.reason != "unit-test")
		{
			sprintf(failureMsg, "Test 13 FAIL: default reason='%s' expected 'unit-test'", request.reason.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 14: params carry force, timeout, grace and reason through ---
	{
		json params;
		params["force"] = true;
		params["timeoutMs"] = 2500;
		params["graceMs"] = 50;
		params["reason"] = "proxy teardown";

		C64DShutdownRequest request = C64DParseShutdownParams(params, "unit-test");
		if (request.force != true || request.timeoutMs != 2500 || request.graceMs != 50)
		{
			sprintf(failureMsg, "Test 14 FAIL: parsed force=%d timeoutMs=%d graceMs=%d",
					(int)request.force, request.timeoutMs, request.graceMs);
			TestCompleted(false, failureMsg);
			return;
		}
		if (request.reason != "proxy teardown")
		{
			sprintf(failureMsg, "Test 14 FAIL: reason='%s'", request.reason.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 15: force alone selects the shorter deadline ---
	{
		json params;
		params["force"] = true;
		C64DShutdownRequest request = C64DParseShutdownParams(params, "unit-test");
		if (request.timeoutMs != C64D_SHUTDOWN_TIMEOUT_FORCED_MS)
		{
			sprintf(failureMsg, "Test 15 FAIL: force gave timeoutMs=%d expected %d",
					request.timeoutMs, C64D_SHUTDOWN_TIMEOUT_FORCED_MS);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 16: wrong-typed params fall back instead of throwing ---
	{
		json params;
		params["force"] = "yes";
		params["timeoutMs"] = "soon";
		params["graceMs"] = json::array();
		params["reason"] = 42;

		C64DShutdownRequest request = C64DParseShutdownParams(params, "unit-test");
		if (request.force != false || request.timeoutMs != C64D_SHUTDOWN_TIMEOUT_DEFAULT_MS
			|| request.graceMs != C64D_SHUTDOWN_GRACE_DEFAULT_MS || request.reason != "unit-test")
		{
			sprintf(failureMsg, "Test 16 FAIL: wrong-typed params not ignored (force=%d timeoutMs=%d graceMs=%d reason='%s')",
					(int)request.force, request.timeoutMs, request.graceMs, request.reason.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 17: the reply payload describes the scheduled shutdown ---
	{
		json params;
		params["force"] = true;
		params["graceMs"] = 25;
		json payload = C64DShutdownRequestToJson(C64DParseShutdownParams(params, "unit-test"));

		if (payload.value("status", std::string()) != "shutting_down")
		{
			sprintf(failureMsg, "Test 17 FAIL: payload status='%s'", payload.value("status", std::string()).c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (payload.value("force", false) != true
			|| payload.value("timeoutMs", 0) != C64D_SHUTDOWN_TIMEOUT_FORCED_MS
			|| payload.value("graceMs", 0) != 25)
		{
			sprintf(failureMsg, "Test 17 FAIL: payload does not describe the request: %s", payload.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 18: a shutdown request reaches the executor with a normalized request ---
	{
		CShutdownExecutorGuard guard(RecordShutdown);

		C64DShutdownRequest request;
		request.force = true;
		request.graceMs = 0;
		request.reason = "test-18";

		if (!C64DRequestShutdown(request))
		{
			sprintf(failureMsg, "Test 18 FAIL: first shutdown request was refused");
			TestCompleted(false, failureMsg);
			return;
		}
		if (!WaitForShutdownCalls(1, 2000))
		{
			sprintf(failureMsg, "Test 18 FAIL: executor never ran (%d calls)", sRecordedShutdownCount);
			TestCompleted(false, failureMsg);
			return;
		}
		if (sRecordedShutdown.force != true || sRecordedShutdown.timeoutMs != C64D_SHUTDOWN_TIMEOUT_FORCED_MS)
		{
			sprintf(failureMsg, "Test 18 FAIL: executor got force=%d timeoutMs=%d",
					(int)sRecordedShutdown.force, sRecordedShutdown.timeoutMs);
			TestCompleted(false, failureMsg);
			return;
		}
		if (sRecordedShutdown.reason != "test-18")
		{
			sprintf(failureMsg, "Test 18 FAIL: executor got reason='%s'", sRecordedShutdown.reason.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (!C64DIsShutdownPending())
		{
			sprintf(failureMsg, "Test 18 FAIL: shutdown should be pending after a request");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 19: a second request while one is pending is refused, not doubled ---
	{
		CShutdownExecutorGuard guard(RecordShutdown);

		C64DShutdownRequest request;
		request.graceMs = 0;
		request.reason = "test-19-first";
		if (!C64DRequestShutdown(request))
		{
			sprintf(failureMsg, "Test 19 FAIL: first shutdown request was refused");
			TestCompleted(false, failureMsg);
			return;
		}
		if (!WaitForShutdownCalls(1, 2000))
		{
			sprintf(failureMsg, "Test 19 FAIL: executor never ran");
			TestCompleted(false, failureMsg);
			return;
		}

		C64DShutdownRequest second;
		second.graceMs = 0;
		second.reason = "test-19-second";
		if (C64DRequestShutdown(second))
		{
			sprintf(failureMsg, "Test 19 FAIL: second shutdown request should have been refused");
			TestCompleted(false, failureMsg);
			return;
		}

		SYS_Sleep(200);
		if (sRecordedShutdownCount != 1)
		{
			sprintf(failureMsg, "Test 19 FAIL: executor ran %d times, expected 1", sRecordedShutdownCount);
			TestCompleted(false, failureMsg);
			return;
		}
		if (sRecordedShutdown.reason != "test-19-first")
		{
			sprintf(failureMsg, "Test 19 FAIL: refused request still reached the executor ('%s')",
					sRecordedShutdown.reason.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// Tests 20 and 21 both drive CMCPServer tool dispatch.
	{
	EnsureDebuggerServerPresent();

	// --- Test 20: retro_shutdown forwards to the server/shutdown endpoint ---
	{
		// Guard the real executor for the whole block: tool registration also
		// binds a second retro_shutdown to the live debugger server, and nothing
		// in this test may be able to take the test runner down.
		CShutdownExecutorGuard guard(RecordShutdown);

		CRecordingDebuggerServer recordingServer;
		CMCPServer server;
		server.RegisterDebuggerTools(&recordingServer);

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 20;
		request["method"] = "tools/call";
		request["params"]["name"] = "retro_shutdown";
		request["params"]["arguments"]["force"] = true;
		request["params"]["arguments"]["reason"] = "test-20";

		json response = server.HandleRequest(request);
		if (!response.contains("result") || response["result"].value("isError", false))
		{
			sprintf(failureMsg, "Test 20 FAIL: retro_shutdown call failed: %s", response.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (recordingServer.callCount != 1)
		{
			sprintf(failureMsg, "Test 20 FAIL: endpoint called %d times, expected 1", recordingServer.callCount);
			TestCompleted(false, failureMsg);
			return;
		}
		if (recordingServer.lastFn != "server/shutdown")
		{
			sprintf(failureMsg, "Test 20 FAIL: forwarded to '%s', expected 'server/shutdown'",
					recordingServer.lastFn.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (recordingServer.lastParams.value("force", false) != true
			|| recordingServer.lastParams.value("reason", std::string()) != "test-20")
		{
			sprintf(failureMsg, "Test 20 FAIL: forwarded params wrong: %s",
					recordingServer.lastParams.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}

		// Outside bridge mode nothing local is scheduled -- the target app is
		// the one hosting this MCP server, and it quits via the endpoint.
		std::string text = response["result"]["content"][0]["text"].get<std::string>();
		json toolResult = json::parse(text);
		if (toolResult.value("target", std::string()) != "application")
		{
			sprintf(failureMsg, "Test 20 FAIL: target='%s' expected 'application'",
					toolResult.value("target", std::string()).c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (toolResult.value("bridgeExiting", true) != false)
		{
			sprintf(failureMsg, "Test 20 FAIL: bridgeExiting should be false outside bridge mode");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 21: retro_shutdown is listed with force, timeoutMs and exitBridge ---
	{
		CShutdownExecutorGuard guard(RecordShutdown);

		CRecordingDebuggerServer recordingServer;
		CMCPServer server;
		server.RegisterDebuggerTools(&recordingServer);

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 21;
		request["method"] = "tools/list";
		request["params"] = json::object();

		json response = server.HandleRequest(request);
		const json *shutdownTool = NULL;
		if (response.contains("result") && response["result"].contains("tools"))
		{
			for (const auto &tool : response["result"]["tools"])
			{
				if (tool.value("name", std::string()) == "retro_shutdown")
				{
					shutdownTool = &tool;
					break;
				}
			}
		}
		if (shutdownTool == NULL)
		{
			sprintf(failureMsg, "Test 21 FAIL: retro_shutdown missing from tools/list");
			TestCompleted(false, failureMsg);
			return;
		}
		const json &properties = (*shutdownTool)["inputSchema"]["properties"];
		if (!properties.contains("force") || !properties.contains("timeoutMs")
			|| !properties.contains("exitBridge") || !properties.contains("reason"))
		{
			sprintf(failureMsg, "Test 21 FAIL: retro_shutdown schema incomplete: %s", properties.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 22: retro_disk_detach forwards to <platform>/detachDiskImage ---
	{
		CRecordingDebuggerServer recordingServer;
		CMCPServer server;
		server.RegisterDebuggerTools(&recordingServer);

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 22;
		request["method"] = "tools/call";
		request["params"]["name"] = "retro_disk_detach";
		request["params"]["arguments"]["platform"] = "c64";
		request["params"]["arguments"]["device"] = 9;

		json response = server.HandleRequest(request);
		if (!response.contains("result") || response["result"].value("isError", false))
		{
			sprintf(failureMsg, "Test 22 FAIL: retro_disk_detach call failed: %s", response.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (recordingServer.lastFn != "c64/detachDiskImage")
		{
			sprintf(failureMsg, "Test 22 FAIL: forwarded to '%s', expected 'c64/detachDiskImage'",
					recordingServer.lastFn.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (recordingServer.lastParams.value("device", 0) != 9)
		{
			sprintf(failureMsg, "Test 22 FAIL: forwarded params wrong: %s",
					recordingServer.lastParams.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}

		// Without an explicit device the tool must stay silent about it, so the
		// endpoint picks the platform default (C64: unit 8, Atari: D1:)
		json requestNoDevice;
		requestNoDevice["jsonrpc"] = "2.0";
		requestNoDevice["id"] = 22;
		requestNoDevice["method"] = "tools/call";
		requestNoDevice["params"]["name"] = "retro_disk_detach";
		requestNoDevice["params"]["arguments"]["platform"] = "atari800";

		json responseNoDevice = server.HandleRequest(requestNoDevice);
		if (!responseNoDevice.contains("result") || responseNoDevice["result"].value("isError", false))
		{
			sprintf(failureMsg, "Test 22 FAIL: retro_disk_detach without device failed: %s",
					responseNoDevice.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (recordingServer.lastFn != "atari800/detachDiskImage" || recordingServer.lastParams.contains("device"))
		{
			sprintf(failureMsg, "Test 22 FAIL: fn='%s' params=%s, expected 'atari800/detachDiskImage' with no device",
					recordingServer.lastFn.c_str(), recordingServer.lastParams.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 23: retro_disk_detach is listed and distinct from retro_media_detach ---
	{
		CRecordingDebuggerServer recordingServer;
		CMCPServer server;
		server.RegisterDebuggerTools(&recordingServer);

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 23;
		request["method"] = "tools/list";
		request["params"] = json::object();

		json response = server.HandleRequest(request);
		const json *diskDetachTool = NULL;
		bool hasMediaDetach = false;
		if (response.contains("result") && response["result"].contains("tools"))
		{
			for (const auto &tool : response["result"]["tools"])
			{
				const std::string name = tool.value("name", std::string());
				if (name == "retro_disk_detach")
					diskDetachTool = &tool;
				else if (name == "retro_media_detach")
					hasMediaDetach = true;
			}
		}
		if (diskDetachTool == NULL || !hasMediaDetach)
		{
			sprintf(failureMsg, "Test 23 FAIL: tools/list missing retro_disk_detach (%d) or retro_media_detach (%d)",
					diskDetachTool != NULL ? 1 : 0, hasMediaDetach ? 1 : 0);
			TestCompleted(false, failureMsg);
			return;
		}
		const json &properties = (*diskDetachTool)["inputSchema"]["properties"];
		if (!properties.contains("platform") || !properties.contains("device"))
		{
			sprintf(failureMsg, "Test 23 FAIL: retro_disk_detach schema incomplete: %s", properties.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}


	// --- Test 24: a failing endpoint makes retro_load an MCP error ---
	//
	// The handler used to throw the endpoint result away and answer
	// {"status":"loaded"} regardless, so a path the RetroDebugger process
	// cannot open -- a Linux-style path handed to a Windows build -- looked
	// like a successful load while the drive reported error 21.
	{
		json envelope;
		envelope["status"] = HTTP_NOT_FOUND;
		envelope["result"]["error"] = "File not found: /docker-share/c64TestData/test.d64";

		CCannedDebuggerServer cannedServer(envelope);
		CMCPServer server;
		server.RegisterDebuggerTools(&cannedServer);

		json response = CallTool(server, 24, "retro_load", {{"path", "/docker-share/c64TestData/test.d64"}});
		std::string error = ToolErrorText(response);
		if (error.empty())
		{
			sprintf(failureMsg, "Test 24 FAIL: failing load reported success: %s", response.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (error.find("retro_load") == std::string::npos
			|| error.find("File not found") == std::string::npos)
		{
			sprintf(failureMsg, "Test 24 FAIL: error text lost the endpoint message: %s", error.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		if (cannedServer.callCount != 1)
		{
			sprintf(failureMsg, "Test 24 FAIL: endpoint called %d times, expected 1", cannedServer.callCount);
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 25: a successful endpoint still reports the load ---
	{
		json envelope;
		envelope["status"] = HTTP_OK;
		envelope["result"]["status"] = "queued";
		envelope["result"]["path"] = "/tmp/game.prg";

		CCannedDebuggerServer cannedServer(envelope);
		CMCPServer server;
		server.RegisterDebuggerTools(&cannedServer);

		json response = CallTool(server, 25, "retro_load", {{"path", "/tmp/game.prg"}});
		std::string error = ToolErrorText(response);
		if (!error.empty())
		{
			sprintf(failureMsg, "Test 25 FAIL: successful load reported an error: %s", error.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
		json toolResult = json::parse(response["result"]["content"][0]["text"].get<std::string>());
		if (toolResult.value("status", std::string()) != "loaded"
			|| toolResult.value("path", std::string()) != "/tmp/game.prg")
		{
			sprintf(failureMsg, "Test 25 FAIL: unexpected tool result: %s", toolResult.dump().c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 26: the bridge's flat transport error survives too ---
	//
	// CMCPBridgeClient answers with {"status", "error", "message"} at the top
	// level instead of the PrepareResult envelope when no desktop app is
	// attached, which must not read as a successful command either.
	{
		json envelope;
		envelope["status"] = HTTP_SERVICE_UNAVAILABLE;
		envelope["error"] = "desktop_unavailable";
		envelope["message"] = "No RetroDebugger desktop instance is currently attached";

		CCannedDebuggerServer cannedServer(envelope);
		CMCPServer server;
		server.RegisterDebuggerTools(&cannedServer);

		json response = CallTool(server, 26, "retro_load", {{"path", "/tmp/game.prg"}});
		std::string error = ToolErrorText(response);
		if (error.find("desktop_unavailable") == std::string::npos
			|| error.find("currently attached") == std::string::npos)
		{
			sprintf(failureMsg, "Test 26 FAIL: bridge transport error not propagated: '%s'", error.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 27: every command tool propagates, not just retro_load ---
	{
		json envelope;
		envelope["status"] = HTTP_NOT_ACCEPTABLE;
		envelope["result"]["error"] = "endpoint refused";

		const char *toolNames[] = {
			"retro_pause", "retro_continue", "retro_reset", "retro_step_instruction",
			"retro_cpu_jump", "retro_breakpoint_add", "retro_breakpoint_remove",
			"retro_watch_add", "retro_watch_remove", "retro_segment_write",
			"retro_media_detach", "retro_disk_detach", "retro_warp"
		};

		for (int i = 0; i < (int)(sizeof(toolNames) / sizeof(toolNames[0])); i++)
		{
			CCannedDebuggerServer cannedServer(envelope);
			CMCPServer server;
			server.RegisterDebuggerTools(&cannedServer);

			json arguments;
			arguments["platform"] = "c64";
			arguments["address"] = 0x1000;
			arguments["segment"] = "Default";
			arguments["enabled"] = true;

			json response = CallTool(server, 27, toolNames[i], arguments);
			std::string error = ToolErrorText(response);
			if (error.find("endpoint refused") == std::string::npos)
			{
				sprintf(failureMsg, "Test 27 FAIL: %s swallowed the endpoint failure: %s",
						toolNames[i], response.dump().c_str());
				TestCompleted(false, failureMsg);
				return;
			}
		}
	}

	}


	// --- Test 28: MCP cannot acquire a server before endpoint registration ---
	// (PR #130's Test 9, carried into the private tree's numbering)
	{
		CTestMCPDebuggerServer debuggerServer;
		CMCPServer server;
		server.SetDebuggerServer(&debuggerServer);

		if (server.GetReadyDebuggerServer() != NULL)
		{
			sprintf(failureMsg, "Test 28 FAIL: unready debugger server was published");
			TestCompleted(false, failureMsg);
			return;
		}

		debuggerServer.SetEndpointRegistryReady(true);
		if (server.GetReadyDebuggerServer() != &debuggerServer)
		{
			sprintf(failureMsg, "Test 28 FAIL: ready debugger server was not acquired");
			TestCompleted(false, failureMsg);
			return;
		}

		debuggerServer.SetEndpointRegistryReady(false);
		if (server.GetReadyDebuggerServer() != NULL)
		{
			sprintf(failureMsg, "Test 28 FAIL: stopped debugger server remained ready");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 29: debugger tools capture the server acquired after readiness ---
	// (PR #130's Test 10, carried into the private tree's numbering)
	{
		CTestMCPDebuggerServer debuggerServer;
		debuggerServer.SetEndpointRegistryReady(true);

		CMCPServer server;
		server.SetDebuggerServer(&debuggerServer);

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 3;
		request["method"] = "tools/call";
		request["params"]["name"] = "retro_cpu_status";
		request["params"]["arguments"]["platform"] = "c64";

		json response = server.HandleRequest(request);
		if (!response.contains("result") || debuggerServer.numCalls != 1)
		{
			sprintf(failureMsg, "Test 29 FAIL: debugger tool did not call the ready server");
			TestCompleted(false, failureMsg);
			return;
		}
		if (debuggerServer.lastEndpointName != "c64/cpu/status")
		{
			sprintf(failureMsg, "Test 29 FAIL: endpoint='%s'", debuggerServer.lastEndpointName.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	TestCompleted(true, "All MCP protocol tests passed (29/29)");
}

void CTestMCPProtocol::Cancel()
{
	isRunning = false;
}
