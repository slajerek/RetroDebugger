#include "CTestMCPProtocol.h"
#include "CMCPServer.h"
#include "CDebuggerServer.h"
#include "CDebuggerServerProtocol.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>

using namespace nlohmann;

static char failureMsg[512];

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

	// --- Test 9: MCP cannot acquire a server before endpoint registration ---
	{
		CTestMCPDebuggerServer debuggerServer;
		CMCPServer server;
		server.SetDebuggerServer(&debuggerServer);

		if (server.GetReadyDebuggerServer() != NULL)
		{
			sprintf(failureMsg, "Test 9 FAIL: unready debugger server was published");
			TestCompleted(false, failureMsg);
			return;
		}

		debuggerServer.SetEndpointRegistryReady(true);
		if (server.GetReadyDebuggerServer() != &debuggerServer)
		{
			sprintf(failureMsg, "Test 9 FAIL: ready debugger server was not acquired");
			TestCompleted(false, failureMsg);
			return;
		}

		debuggerServer.SetEndpointRegistryReady(false);
		if (server.GetReadyDebuggerServer() != NULL)
		{
			sprintf(failureMsg, "Test 9 FAIL: stopped debugger server remained ready");
			TestCompleted(false, failureMsg);
			return;
		}
	}

	// --- Test 10: debugger tools capture the server acquired after readiness ---
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
			sprintf(failureMsg, "Test 10 FAIL: debugger tool did not call the ready server");
			TestCompleted(false, failureMsg);
			return;
		}
		if (debuggerServer.lastEndpointName != "c64/cpu/status")
		{
			sprintf(failureMsg, "Test 10 FAIL: endpoint='%s'", debuggerServer.lastEndpointName.c_str());
			TestCompleted(false, failureMsg);
			return;
		}
	}

	TestCompleted(true, "All MCP protocol tests passed (10/10)");
}

void CTestMCPProtocol::Cancel()
{
	isRunning = false;
}
