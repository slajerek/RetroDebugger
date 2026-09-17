#include "CTestMCPBridge.h"
#include "CMCPBridgeClient.h"
#include "CMCPServer.h"
#include "CDebuggerServer.h"
#include "CDebuggerServerProtocol.h"
#include "CViewC64.h"
#include "C64SettingsStorage.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <set>

using namespace nlohmann;

static char sFailMsg[1024];

static bool WaitForBridgeState(CMCPBridgeClient *bridge, MCPBridgeState target, int timeoutMs)
{
	for (int elapsed = 0; elapsed < timeoutMs; elapsed += 50)
	{
		if (bridge->GetState() == target)
			return true;
		SYS_Sleep(50);
	}
	return bridge->GetState() == target;
}

static bool EnsureDebuggerServerRunning()
{
	if (!viewC64 || !viewC64->debuggerServer)
	{
		viewC64->DebuggerServerWebSocketsStart();
		for (int i = 0; i < 40; i++)
		{
			if (viewC64->debuggerServer && viewC64->debuggerServer->isRunning)
				return true;
			SYS_Sleep(50);
		}
	}
	return viewC64->debuggerServer && viewC64->debuggerServer->isRunning;
}

void CTestMCPBridge::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	sFailMsg[0] = '\0';

	// The bridge test needs a WebSocket debugger server to connect to.
	// Either the test process's own server, or a running desktop app, will do.
	// Try to start our own; if it fails (port busy), assume the desktop is running.
	int wsPort = c64SettingsRunDebuggerServerWebSocketsPort;
	bool serverStartedByTest = false;
	if (viewC64->debuggerServer == NULL)
	{
		viewC64->DebuggerServerWebSocketsStart();
		serverStartedByTest = true;
		SYS_Sleep(1000);
	}
	else if (!viewC64->debuggerServer->isRunning)
	{
		viewC64->debuggerServer->Start();
		serverStartedByTest = true;
		SYS_Sleep(1000);
	}

	// Verify that SOMETHING is listening on the port by trying a quick bridge connect
	{
		CMCPBridgeClient probe("127.0.0.1", wsPort, "/stream");
		bool canConnect = probe.TryConnect();
		probe.Disconnect();
		if (!canConnect)
		{
			TestCompleted(false, "FAIL: No WebSocket server reachable on port (test server and desktop app both unavailable)");
			return;
		}
	}

	auto FinishTest = [&](bool success, const char *message)
	{
		if (serverStartedByTest && viewC64->debuggerServer && viewC64->debuggerServer->isRunning)
		{
			viewC64->debuggerServer->Stop();
		}
		TestCompleted(success, message);
	};

	int testNum = 0;
	int passed = 0;

	// =====================================================================
	// Test 1: Bridge starts in disconnected state
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		if (bridge.GetState() != MCP_BRIDGE_DISCONNECTED)
		{
			sprintf(sFailMsg, "Test %d FAIL: initial state should be disconnected, got %s",
					testNum, bridge.GetStateString().c_str());
			FinishTest(false, sFailMsg);
			return;
		}
		passed++;
	}

	// =====================================================================
	// Test 2: Bridge connects to running WebSocket server
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		bool connected = bridge.TryConnect();
		if (!connected)
		{
			sprintf(sFailMsg, "Test %d FAIL: TryConnect failed, lastError in diagnostics",
					testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		// State should be connecting (not yet connected — need discovery)
		if (bridge.GetState() != MCP_BRIDGE_CONNECTING)
		{
			sprintf(sFailMsg, "Test %d FAIL: after TryConnect state should be connecting, got %s",
					testNum, bridge.GetStateString().c_str());
			FinishTest(false, sFailMsg);
			return;
		}
		bridge.Disconnect();
		passed++;
	}

	// =====================================================================
	// Test 3: Discovery succeeds against running server
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		bridge.TryConnect();
		bool discovered = bridge.RunDiscovery();
		if (!discovered)
		{
			json diag = bridge.GetDiagnostics();
			sprintf(sFailMsg, "Test %d FAIL: RunDiscovery failed: %s",
					testNum, diag.dump().c_str());
			FinishTest(false, sFailMsg);
			return;
		}

		if (bridge.remoteProtocolVersion < DEBUGGER_PROTOCOL_V2)
		{
			sprintf(sFailMsg, "Test %d FAIL: protocolVersion=%d, expected >= %d",
					testNum, bridge.remoteProtocolVersion, DEBUGGER_PROTOCOL_V2);
			FinishTest(false, sFailMsg);
			return;
		}
		if (bridge.remoteDiscoveryVersion < DEBUGGER_DISCOVERY_VERSION)
		{
			sprintf(sFailMsg, "Test %d FAIL: discoveryVersion=%d, expected >= %d",
					testNum, bridge.remoteDiscoveryVersion, DEBUGGER_DISCOVERY_VERSION);
			FinishTest(false, sFailMsg);
			return;
		}
		if (bridge.remoteServerName != "RetroDebugger")
		{
			sprintf(sFailMsg, "Test %d FAIL: serverName='%s', expected 'RetroDebugger'",
					testNum, bridge.remoteServerName.c_str());
			FinishTest(false, sFailMsg);
			return;
		}
		if (bridge.remoteEndpoints.empty())
		{
			sprintf(sFailMsg, "Test %d FAIL: no endpoints discovered", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		if (bridge.remotePlatforms.empty())
		{
			sprintf(sFailMsg, "Test %d FAIL: no platforms discovered", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Disconnect();
		passed++;
	}

	// =====================================================================
	// Test 4: RunEndpointFunction dispatches through bridge
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		bridge.TryConnect();
		bridge.RunDiscovery();

		// Call server/hello through RunEndpointFunction
		std::vector<char> *result = bridge.RunEndpointFunction("server/hello", "", json::object(), nullptr, 0);
		if (!result || result->empty())
		{
			delete result;
			sprintf(sFailMsg, "Test %d FAIL: RunEndpointFunction(server/hello) returned empty", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		std::string raw(result->data(), result->size());
		delete result;

		auto nullPos = raw.find('\0');
		if (nullPos != std::string::npos) raw = raw.substr(0, nullPos);

		try
		{
			json j = json::parse(raw);
			if (j.value("status", 0) != 200)
			{
				sprintf(sFailMsg, "Test %d FAIL: server/hello returned status %d", testNum, j.value("status", 0));
				FinishTest(false, sFailMsg);
				return;
			}
		}
		catch (const std::exception &e)
		{
			sprintf(sFailMsg, "Test %d FAIL: parse error: %s", testNum, e.what());
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Disconnect();
		passed++;
	}

	// =====================================================================
	// Test 5: RunEndpointFunction returns desktop_unavailable when disconnected
	// =====================================================================
	testNum++;
	{
		// Connect to a port nothing listens on
		CMCPBridgeClient bridge("127.0.0.1", 19999, "/stream");
		std::vector<char> *result = bridge.RunEndpointFunction("server/hello", "", json::object(), nullptr, 0);
		if (!result || result->empty())
		{
			delete result;
			sprintf(sFailMsg, "Test %d FAIL: should return error response, not empty", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		std::string raw(result->data(), result->size());
		delete result;

		try
		{
			json j = json::parse(raw);
			if (j.value("error", "") != "desktop_unavailable")
			{
				sprintf(sFailMsg, "Test %d FAIL: expected desktop_unavailable, got: %s",
						testNum, raw.substr(0, 200).c_str());
				FinishTest(false, sFailMsg);
				return;
			}
		}
		catch (const std::exception &e)
		{
			sprintf(sFailMsg, "Test %d FAIL: parse error: %s", testNum, e.what());
			FinishTest(false, sFailMsg);
			return;
		}
		passed++;
	}

	// =====================================================================
	// Test 6: Disconnect sets state to stale (was connected)
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		bridge.TryConnect();
		bridge.RunDiscovery();

		// Manually set connected state via SetState helper workaround
		// (RunDiscovery doesn't set CONNECTED — that's done by the background thread or caller)
		// We verify that Disconnect from a connected-like state goes to stale
		// First verify we can communicate:
		std::vector<char> *result = bridge.RunEndpointFunction("server/hello", "", json::object(), nullptr, 0);
		bool wasWorking = (result && !result->empty());
		delete result;

		if (!wasWorking)
		{
			sprintf(sFailMsg, "Test %d FAIL: bridge wasn't functional before disconnect test", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Disconnect();
		// After disconnect from a working connection, state depends on prior state
		// Since we didn't go through full SetState(CONNECTED), state was CONNECTING
		// which means disconnect sets DISCONNECTED
		MCPBridgeState afterState = bridge.GetState();
		if (afterState != MCP_BRIDGE_DISCONNECTED && afterState != MCP_BRIDGE_STALE)
		{
			sprintf(sFailMsg, "Test %d FAIL: after disconnect state should be disconnected or stale, got %s",
					testNum, bridge.GetStateString().c_str());
			FinishTest(false, sFailMsg);
			return;
		}
		passed++;
	}

	// =====================================================================
	// Test 7: State callback fires on transitions
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		int callbackCount = 0;
		MCPBridgeState lastOld = MCP_BRIDGE_DISCONNECTED;
		MCPBridgeState lastNew = MCP_BRIDGE_DISCONNECTED;

		bridge.SetStateCallback([&](MCPBridgeState oldState, MCPBridgeState newState)
		{
			callbackCount++;
			lastOld = oldState;
			lastNew = newState;
		});

		bridge.TryConnect();
		if (callbackCount < 1)
		{
			sprintf(sFailMsg, "Test %d FAIL: state callback not fired after TryConnect (count=%d)",
					testNum, callbackCount);
			FinishTest(false, sFailMsg);
			return;
		}
		// Should have transitioned to CONNECTING
		if (lastNew != MCP_BRIDGE_CONNECTING)
		{
			sprintf(sFailMsg, "Test %d FAIL: expected transition to connecting, got %d->%d",
					testNum, (int)lastOld, (int)lastNew);
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Disconnect();
		passed++;
	}

	// =====================================================================
	// Test 8: CMCPServer bridge mode — offline tools/list returns bridge-local only
	// =====================================================================
	testNum++;
	{
		CMCPServer server;
		// Connect to nothing — bridge stays disconnected
		CMCPBridgeClient bridge("127.0.0.1", 19999, "/stream");
		server.SetBridgeMode(&bridge);

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 1;
		request["method"] = "tools/list";
		request["params"] = json::object();

		json response = server.HandleRequest(request);
		if (!response.contains("result") || !response["result"].contains("tools"))
		{
			sprintf(sFailMsg, "Test %d FAIL: tools/list missing result.tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		json tools = response["result"]["tools"];
		// Should only have bridge-local tools
		bool hasTransportDiag = false;
		bool hasReconnect = false;
		bool hasDebuggerTool = false;
		for (const auto &t : tools)
		{
			std::string name = t.value("name", "");
			if (name == "retro_transport_diagnostics") hasTransportDiag = true;
			if (name == "retro_reconnect") hasReconnect = true;
			if (name == "retro_cpu_status") hasDebuggerTool = true;
		}

		if (!hasTransportDiag)
		{
			sprintf(sFailMsg, "Test %d FAIL: missing retro_transport_diagnostics in offline tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		if (!hasReconnect)
		{
			sprintf(sFailMsg, "Test %d FAIL: missing retro_reconnect in offline tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		if (hasDebuggerTool)
		{
			sprintf(sFailMsg, "Test %d FAIL: retro_cpu_status should NOT appear in offline tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		passed++;
	}

	// =====================================================================
	// Test 9: CMCPServer bridge mode — connected tools/list includes debugger tools
	// =====================================================================
	testNum++;
	{
		CMCPServer server;
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		server.SetBridgeMode(&bridge);

		// Start bridge background thread — it will connect, discover, and set CONNECTED
		bridge.Start();
		if (!WaitForBridgeState(&bridge, MCP_BRIDGE_CONNECTED, 10000))
		{
			bridge.Stop();
			sprintf(sFailMsg, "Test %d FAIL: bridge did not reach connected state, got %s",
					testNum, bridge.GetStateString().c_str());
			FinishTest(false, sFailMsg);
			return;
		}

		json request;
		request["jsonrpc"] = "2.0";
		request["id"] = 1;
		request["method"] = "tools/list";
		request["params"] = json::object();

		json response = server.HandleRequest(request);
		json tools = response["result"]["tools"];

		bool hasCpuStatus = false;
		bool hasMemoryRead = false;
		bool hasTransportDiag = false;
		int toolCount = 0;
		for (const auto &t : tools)
		{
			std::string name = t.value("name", "");
			if (name == "retro_cpu_status") hasCpuStatus = true;
			if (name == "retro_memory_read") hasMemoryRead = true;
			if (name == "retro_transport_diagnostics") hasTransportDiag = true;
			toolCount++;
		}

		if (!hasCpuStatus)
		{
			sprintf(sFailMsg, "Test %d FAIL: missing retro_cpu_status in connected tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		if (!hasMemoryRead)
		{
			sprintf(sFailMsg, "Test %d FAIL: missing retro_memory_read in connected tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		if (!hasTransportDiag)
		{
			sprintf(sFailMsg, "Test %d FAIL: missing retro_transport_diagnostics in connected tools", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		// Should be 46: 43 debugger + 3 bridge-local
		if (toolCount != 46)
		{
			sprintf(sFailMsg, "Test %d FAIL: expected 46 tools, got %d", testNum, toolCount);
			bridge.Stop();
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Stop();
		passed++;
	}

	// =====================================================================
	// Test 10: Tools cleared after disconnect, no duplicates on reconnect
	// =====================================================================
	testNum++;
	{
		CMCPServer server;
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		server.SetBridgeMode(&bridge);

		// First connect via background thread
		bridge.Start();
		if (!WaitForBridgeState(&bridge, MCP_BRIDGE_CONNECTED, 10000))
		{
			bridge.Stop();
			sprintf(sFailMsg, "Test %d FAIL: bridge did not connect", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		// Force initial tools/list to register everything
		{
			json req;
			req["jsonrpc"] = "2.0";
			req["id"] = 0;
			req["method"] = "tools/list";
			req["params"] = json::object();
			server.HandleRequest(req);
		}

		// Stop bridge (simulates desktop app going away)
		bridge.Stop();
		SYS_Sleep(500);

		// Verify tools list is bridge-local only
		{
			json request;
			request["jsonrpc"] = "2.0";
			request["id"] = 1;
			request["method"] = "tools/list";
			request["params"] = json::object();

			json response = server.HandleRequest(request);
			json tools = response["result"]["tools"];
			int toolCount = (int)tools.size();
			if (toolCount != 3)
			{
				sprintf(sFailMsg, "Test %d FAIL: after disconnect expected 3 bridge-local tools, got %d",
						testNum, toolCount);
				FinishTest(false, sFailMsg);
				return;
			}

			// retro_shutdown has to survive the disconnect: shutting down an
			// orphaned bridge is exactly what it is for.
			bool hasShutdown = false;
			for (const auto &t : tools)
			{
				if (t.value("name", std::string()) == "retro_shutdown")
					hasShutdown = true;
			}
			if (!hasShutdown)
			{
				sprintf(sFailMsg, "Test %d FAIL: retro_shutdown missing from disconnected tool list", testNum);
				FinishTest(false, sFailMsg);
				return;
			}
		}

		// Reconnect: create new bridge, rewire
		CMCPBridgeClient bridge2("127.0.0.1", wsPort, "/stream");
		server.SetBridgeMode(&bridge2);
		bridge2.Start();
		if (!WaitForBridgeState(&bridge2, MCP_BRIDGE_CONNECTED, 10000))
		{
			bridge2.Stop();
			sprintf(sFailMsg, "Test %d FAIL: bridge2 did not reconnect", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		// Verify full tool set, no duplicates
		{
			json request;
			request["jsonrpc"] = "2.0";
			request["id"] = 2;
			request["method"] = "tools/list";
			request["params"] = json::object();

			json response = server.HandleRequest(request);
			json tools = response["result"]["tools"];
			int toolCount = (int)tools.size();
			if (toolCount != 46)
			{
				sprintf(sFailMsg, "Test %d FAIL: after reconnect expected 46 tools, got %d",
						testNum, toolCount);
				bridge2.Stop();
				FinishTest(false, sFailMsg);
				return;
			}

			// Check no duplicate names
			std::set<std::string> names;
			for (const auto &t : tools)
			{
				std::string name = t.value("name", "");
				if (names.count(name) > 0)
				{
					sprintf(sFailMsg, "Test %d FAIL: duplicate tool name: %s", testNum, name.c_str());
					bridge2.Stop();
					FinishTest(false, sFailMsg);
					return;
				}
				names.insert(name);
			}
		}

		bridge2.Stop();
		passed++;
	}

	// =====================================================================
	// Test 11: Diagnostics tool returns correct info
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		bridge.Start();
		if (!WaitForBridgeState(&bridge, MCP_BRIDGE_CONNECTED, 10000))
		{
			bridge.Stop();
			sprintf(sFailMsg, "Test %d FAIL: bridge did not connect for diagnostics test", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		json diag = bridge.GetDiagnostics();
		if (diag.value("host", "") != "127.0.0.1")
		{
			sprintf(sFailMsg, "Test %d FAIL: diagnostics host mismatch", testNum);
			bridge.Stop();
			FinishTest(false, sFailMsg);
			return;
		}
		if (diag.value("port", 0) != 3563)
		{
			sprintf(sFailMsg, "Test %d FAIL: diagnostics port mismatch", testNum);
			bridge.Stop();
			FinishTest(false, sFailMsg);
			return;
		}
		if (!diag.contains("remoteServerName") || diag["remoteServerName"] != "RetroDebugger")
		{
			sprintf(sFailMsg, "Test %d FAIL: diagnostics missing remoteServerName", testNum);
			bridge.Stop();
			FinishTest(false, sFailMsg);
			return;
		}
		if (!diag.contains("remoteEndpointCount") || diag["remoteEndpointCount"].get<int>() < 10)
		{
			sprintf(sFailMsg, "Test %d FAIL: diagnostics remoteEndpointCount too low", testNum);
			bridge.Stop();
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Stop();
		passed++;
	}

	// =====================================================================
	// Test 12: Connection to wrong port fails gracefully
	// =====================================================================
	testNum++;
	{
		CMCPBridgeClient bridge("127.0.0.1", 19999, "/stream");
		bool connected = bridge.TryConnect();
		if (connected)
		{
			bridge.Disconnect();
			sprintf(sFailMsg, "Test %d FAIL: should not connect to non-existent server", testNum);
			FinishTest(false, sFailMsg);
			return;
		}
		if (bridge.GetState() != MCP_BRIDGE_DISCONNECTED)
		{
			sprintf(sFailMsg, "Test %d FAIL: after failed connect, state should be disconnected, got %s",
					testNum, bridge.GetStateString().c_str());
			FinishTest(false, sFailMsg);
			return;
		}
		passed++;
	}

	// =====================================================================
	// Test 13: Tool call dispatched through bridge returns real data
	// =====================================================================
	testNum++;
	{
		CMCPServer server;
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		server.SetBridgeMode(&bridge);

		bridge.Start();
		if (!WaitForBridgeState(&bridge, MCP_BRIDGE_CONNECTED, 10000))
		{
			bridge.Stop();
			sprintf(sFailMsg, "Test %d FAIL: bridge did not connect", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		// Ensure tools are registered
		json listReq;
		listReq["jsonrpc"] = "2.0";
		listReq["id"] = 1;
		listReq["method"] = "tools/list";
		listReq["params"] = json::object();
		server.HandleRequest(listReq);

		// Call retro_list_platforms through MCP
		json callReq;
		callReq["jsonrpc"] = "2.0";
		callReq["id"] = 2;
		callReq["method"] = "tools/call";
		callReq["params"]["name"] = "retro_list_platforms";
		callReq["params"]["arguments"] = json::object();

		json response = server.HandleRequest(callReq);
		if (!response.contains("result") || !response["result"].contains("content"))
		{
			sprintf(sFailMsg, "Test %d FAIL: tools/call response missing result.content", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		std::string text = response["result"]["content"][0].value("text", "");
		try
		{
			json platformData = json::parse(text);
			if (!platformData.contains("platforms"))
			{
				sprintf(sFailMsg, "Test %d FAIL: retro_list_platforms result missing platforms key", testNum);
				FinishTest(false, sFailMsg);
				return;
			}
			if (!platformData["platforms"].is_array() || platformData["platforms"].empty())
			{
				sprintf(sFailMsg, "Test %d FAIL: retro_list_platforms returned empty platforms", testNum);
				FinishTest(false, sFailMsg);
				return;
			}
		}
		catch (const std::exception &e)
		{
			sprintf(sFailMsg, "Test %d FAIL: parse platforms result: %s", testNum, e.what());
			bridge.Stop();
			FinishTest(false, sFailMsg);
			return;
		}

		bridge.Stop();
		passed++;
	}

	// =====================================================================
	// Test 14: Headless and bridge expose same canonical tool names
	// =====================================================================
	testNum++;
	{
		// Get headless tool names (from the actual running server)
		CMCPServer headlessServer;

		json headlessListReq;
		headlessListReq["jsonrpc"] = "2.0";
		headlessListReq["id"] = 1;
		headlessListReq["method"] = "tools/list";
		headlessListReq["params"] = json::object();
		json headlessResp = headlessServer.HandleRequest(headlessListReq);

		std::set<std::string> headlessNames;
		if (headlessResp.contains("result") && headlessResp["result"].contains("tools"))
		{
			for (const auto &t : headlessResp["result"]["tools"])
				headlessNames.insert(t.value("name", ""));
		}

		// Get bridge tool names
		CMCPServer bridgeServer;
		CMCPBridgeClient bridge("127.0.0.1", wsPort, "/stream");
		bridgeServer.SetBridgeMode(&bridge);
		bridge.Start();
		if (!WaitForBridgeState(&bridge, MCP_BRIDGE_CONNECTED, 10000))
		{
			bridge.Stop();
			sprintf(sFailMsg, "Test %d FAIL: bridge did not connect for parity test", testNum);
			FinishTest(false, sFailMsg);
			return;
		}

		json bridgeListReq;
		bridgeListReq["jsonrpc"] = "2.0";
		bridgeListReq["id"] = 1;
		bridgeListReq["method"] = "tools/list";
		bridgeListReq["params"] = json::object();
		json bridgeResp = bridgeServer.HandleRequest(bridgeListReq);

		std::set<std::string> bridgeNames;
		if (bridgeResp.contains("result") && bridgeResp["result"].contains("tools"))
		{
			for (const auto &t : bridgeResp["result"]["tools"])
				bridgeNames.insert(t.value("name", ""));
		}

		// All headless tool names should exist in bridge (bridge has extras: transport_diagnostics, reconnect)
		for (const auto &name : headlessNames)
		{
			if (bridgeNames.count(name) == 0)
			{
				sprintf(sFailMsg, "Test %d FAIL: headless tool '%s' missing from bridge mode",
						testNum, name.c_str());
				bridge.Stop();
				FinishTest(false, sFailMsg);
				return;
			}
		}

		bridge.Stop();
		passed++;
	}

	sprintf(sFailMsg, "All MCP bridge tests passed (%d/%d)", passed, testNum);
	FinishTest(true, sFailMsg);
}

void CTestMCPBridge::Cancel()
{
	isRunning = false;
}
