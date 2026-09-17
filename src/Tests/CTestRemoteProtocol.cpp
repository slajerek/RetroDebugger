#include "CTestRemoteProtocol.h"
#include "CDebuggerServer.h"
#include "CDebuggerServerProtocol.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "C64DShutdown.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include <string>
#include <vector>

using namespace nlohmann;

// --- server/shutdown test double -------------------------------------------

static C64DShutdownRequest sRecordedShutdown;
static int sRecordedShutdownCount = 0;

static void RecordShutdown(const C64DShutdownRequest &request)
{
	sRecordedShutdown = request;
	sRecordedShutdownCount++;
}

// Swaps in a recording executor so hitting server/shutdown for real does not
// take the test runner down, and restores the real one on every exit path.
struct CShutdownExecutorGuard
{
	CShutdownExecutorGuard()
	{
		sRecordedShutdownCount = 0;
		sRecordedShutdown = C64DShutdownRequest();
		C64DSetShutdownExecutor(RecordShutdown);
	}
	~CShutdownExecutorGuard()
	{
		C64DSetShutdownExecutor(NULL);
	}
};

static bool WaitForServerRunning(CViewC64 *viewC64, int timeoutMs)
{
	for (int elapsed = 0; elapsed < timeoutMs; elapsed += 50)
	{
		if (viewC64->debuggerServer != NULL && viewC64->debuggerServer->isRunning)
		{
			return true;
		}
		SYS_Sleep(50);
	}

	return viewC64->debuggerServer != NULL && viewC64->debuggerServer->isRunning;
}

// Helper: call an endpoint on the debugger server, parse JSON result
static json CallEndpoint(CDebuggerServer *server, const char *fn)
{
	std::vector<char> *result = server->RunEndpointFunction(fn, "", json(), nullptr, 0);
	if (!result || result->empty())
	{
		delete result;
		return json();
	}
	std::string raw(result->data(), result->size());
	delete result;

	// Strip binary portion if present (null-byte separated)
	auto nullPos = raw.find('\0');
	if (nullPos != std::string::npos)
		raw = raw.substr(0, nullPos);

	try
	{
		return json::parse(raw);
	}
	catch (const json::exception &e)
	{
		return {
			{"status", HTTP_INTERNAL_SERVER_ERROR},
			{"error", "Malformed JSON response"},
			{"details", e.what()}
		};
	}
}

// Helper: call an endpoint with params, parse JSON result
static json CallEndpointWithParams(CDebuggerServer *server, const char *fn, const json &params)
{
	std::vector<char> *result = server->RunEndpointFunction(fn, "", params, nullptr, 0);
	if (!result || result->empty())
	{
		delete result;
		return json();
	}
	std::string raw(result->data(), result->size());
	delete result;

	auto nullPos = raw.find('\0');
	if (nullPos != std::string::npos)
		raw = raw.substr(0, nullPos);

	try
	{
		return json::parse(raw);
	}
	catch (const json::exception &e)
	{
		return {
			{"status", HTTP_INTERNAL_SERVER_ERROR},
			{"error", "Malformed JSON response"},
			{"details", e.what()}
		};
	}
}

static bool JsonObjectContainsFields(const json &object, const json &expectedFields)
{
	if (!object.is_object() || !expectedFields.is_object())
		return false;

	for (auto it = expectedFields.begin(); it != expectedFields.end(); ++it)
	{
		if (!object.contains(it.key()) || object[it.key()] != it.value())
			return false;
	}

	return true;
	}

static const json *FindJsonArrayObjectWithFields(const json &array, const json &expectedFields)
{
	if (!array.is_array())
		return NULL;

	for (const auto &entry : array)
	{
		if (JsonObjectContainsFields(entry, expectedFields))
			return &entry;
	}

	return NULL;
}

static bool JsonArrayContainsString(const json &array, const char *value)
{
	if (!array.is_array())
		return false;

	for (const auto &entry : array)
	{
		if (entry.is_string() && entry.get<std::string>() == value)
			return true;
	}

	return false;
}

void CTestRemoteProtocol::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;

	// Start the debugger server if not already running
	bool serverStartedByTest = false;
	if (viewC64->debuggerServer == NULL)
	{
		viewC64->DebuggerServerWebSocketsStart();
		serverStartedByTest = true;
		if (!WaitForServerRunning(viewC64, 2000))
		{
			TestCompleted(false, "FAIL: Debugger server did not reach running state");
			return;
		}
	}

	auto FinishTest = [&](bool success, const char *message)
	{
		if (serverStartedByTest && viewC64->debuggerServer != NULL && viewC64->debuggerServer->isRunning)
		{
			viewC64->debuggerServer->Stop();
		}

		TestCompleted(success, message);
	};

	CDebuggerServer *server = viewC64->debuggerServer;
	if (!server)
	{
		FinishTest(false, "FAIL: Could not start debugger server");
		return;
	}

	// --- Test 1: discovery payloads expose protocol metadata ---
	{
		json resp = CallEndpoint(server, "server/hello");
		if (resp.empty() || !resp.contains("result"))
		{
			FinishTest(false, "Test 1 FAIL: server/hello returned empty");
			return;
		}
		if (resp["result"].value("protocolVersion", 0) != 2)
		{
			FinishTest(false, "Test 1 FAIL: server/hello protocolVersion != 2");
			return;
		}
		if (resp["result"].value("serverName", std::string()) != "RetroDebugger")
		{
			FinishTest(false, "Test 1 FAIL: server/hello serverName != RetroDebugger");
			return;
		}
		if (!resp["result"].contains("serverVersion") || !resp["result"]["serverVersion"].is_string()
			|| resp["result"]["serverVersion"].get<std::string>().empty())
		{
			FinishTest(false, "Test 1 FAIL: server/hello missing non-empty serverVersion");
			return;
		}

		resp = CallEndpoint(server, "server/capabilities");
		if (resp.empty() || !resp.contains("result"))
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities returned empty");
			return;
		}
		if (resp["result"].value("protocolVersion", 0) != 2)
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities protocolVersion != 2");
			return;
		}
		if (resp["result"].value("discoveryVersion", 0) != 1)
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities discoveryVersion != 1");
			return;
		}
		if (!resp["result"].contains("platforms") || !resp["result"]["platforms"].is_array())
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities missing platforms array");
			return;
		}
		if (!resp["result"].contains("features") || !resp["result"]["features"].is_array())
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities missing features array");
			return;
		}
		if (!JsonArrayContainsString(resp["result"]["features"], "platforms"))
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities missing platforms feature");
			return;
		}
		if (!JsonArrayContainsString(resp["result"]["features"], "endpointDescriptors"))
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities missing endpointDescriptors feature");
			return;
		}
		if (!JsonArrayContainsString(resp["result"]["features"], "binaryFraming"))
		{
			FinishTest(false, "Test 1 FAIL: server/capabilities missing binaryFraming feature");
			return;
		}
	}

	// --- Test 2: server/endpoints lists endpoints with fn field ---
	{
		json resp = CallEndpoint(server, "server/endpoints");
		if (resp.empty() || !resp.contains("result"))
		{
			FinishTest(false, "Test 2 FAIL: server/endpoints returned empty");
			return;
		}
		if (!resp["result"].contains("endpoints") || !resp["result"]["endpoints"].is_array())
		{
			FinishTest(false, "Test 2 FAIL: server/endpoints missing endpoints array");
			return;
		}
		const json &endpoints = resp["result"]["endpoints"];
		// Check that endpoints have fn field
		for (const auto &ep : endpoints)
		{
			if (!ep.contains("fn"))
			{
				FinishTest(false, "Test 2 FAIL: endpoint entry missing fn field");
				return;
			}
		}

		if (FindJsonArrayObjectWithFields(endpoints, {
			{"fn", "server/hello"}
		}) == NULL)
		{
			FinishTest(false, "Test 2 FAIL: missing server/hello endpoint descriptor");
			return;
		}

		if (FindJsonArrayObjectWithFields(endpoints, {
			{"fn", "server/capabilities"}
		}) == NULL)
		{
			FinishTest(false, "Test 2 FAIL: missing server/capabilities endpoint descriptor");
			return;
		}

		const json *nesReadBlockDescriptor = FindJsonArrayObjectWithFields(endpoints, {
			{"fn", "nes/ppu/nametable/readBlock"},
			{"platform", "nes"},
			{"category", "chips"},
			{"supportsBinaryOutput", true}
		});
		if (nesReadBlockDescriptor == NULL || !nesReadBlockDescriptor->contains("description")
			|| !(*nesReadBlockDescriptor)["description"].is_string()
			|| (*nesReadBlockDescriptor)["description"].get<std::string>().empty())
		{
			FinishTest(false, "Test 2 FAIL: missing complete descriptor for nes/ppu/nametable/readBlock");
			return;
		}

		const json *nesWriteBlockDescriptor = FindJsonArrayObjectWithFields(endpoints, {
			{"fn", "nes/ppu/nametable/writeBlock"},
			{"platform", "nes"},
			{"category", "chips"},
			{"supportsBinaryInput", true}
		});
		if (nesWriteBlockDescriptor == NULL || !nesWriteBlockDescriptor->contains("description")
			|| !(*nesWriteBlockDescriptor)["description"].is_string()
			|| (*nesWriteBlockDescriptor)["description"].get<std::string>().empty())
		{
			FinishTest(false, "Test 2 FAIL: missing complete descriptor for nes/ppu/nametable/writeBlock");
			return;
		}
	}

	// --- Test 3: server/events lists event types ---
	{
		json resp = CallEndpoint(server, "server/events");
		if (resp.empty() || !resp.contains("result"))
		{
			FinishTest(false, "Test 3 FAIL: server/events returned empty");
			return;
		}
		if (!resp["result"].contains("events") || !resp["result"]["events"].is_array())
		{
			FinishTest(false, "Test 3 FAIL: server/events missing events array");
			return;
		}
		if (resp["result"]["events"].size() < 3)
		{
			std::string failure = "Test 3 FAIL: expected 3+ events, got " + std::to_string(resp["result"]["events"].size());
			FinishTest(false, failure.c_str());
			return;
		}
	}

	// --- Test 4: unknown endpoint returns error (not HTTP_OK) ---
	{
		json resp = CallEndpoint(server, "nonexistent/endpoint");
		// Should return a non-OK status (404) or empty — must NOT look like success
		if (resp.contains("status") && resp["status"].get<int>() == HTTP_OK)
		{
			FinishTest(false, "Test 4 FAIL: unknown endpoint should not return HTTP_OK");
			return;
		}
	}

	// --- Test 5: server/platforms reports running state correctly ---
	{
		// Ensure C64 emulator is running
		CDebugInterface *diC64 = viewC64->debugInterfaceC64;
		bool startedByTest = false;
		if (diC64 != NULL && !diC64->isRunning)
		{
			viewC64->StartEmulationThread(diC64);
			SYS_Sleep(2000);
			startedByTest = true;
		}

		if (diC64 == NULL || !diC64->isRunning)
		{
			FinishTest(false, "Test 5 FAIL: could not start C64 emulator");
			return;
		}

		json resp = CallEndpoint(server, "server/platforms");
		if (resp.empty() || !resp.contains("result"))
		{
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, "Test 5 FAIL: server/platforms returned empty");
			return;
		}

		json platforms = resp["result"].value("platforms", json::array());
		bool foundC64Running = false;
		for (const auto &p : platforms)
		{
			if (p.value("name", "") == "c64" && p.value("running", false) == true)
			{
				foundC64Running = true;
				break;
			}
		}

		if (!foundC64Running)
		{
			std::string detail = "Test 5 FAIL: server/platforms does not report c64 as running. Response: " + resp.dump();
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, detail.c_str());
			return;
		}

		// --- Test 6: c64/state reports isRunning and non-zero PC ---
		resp = CallEndpoint(server, "c64/state");
		if (resp.empty() || !resp.contains("result"))
		{
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, "Test 6 FAIL: c64/state returned empty");
			return;
		}

		bool stateIsRunning = resp["result"].value("isRunning", false);
		int cpuPC = resp["result"].value("cpuPC", 0);

		if (!stateIsRunning)
		{
			std::string detail = "Test 6 FAIL: c64/state isRunning is false. di->isRunning=" + std::to_string(diC64->isRunning) + " Response: " + resp.dump();
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, detail.c_str());
			return;
		}

		if (cpuPC == 0)
		{
			std::string detail = "Test 6 FAIL: c64/state cpuPC is 0 (expected non-zero for running C64). Response: " + resp.dump();
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, detail.c_str());
			return;
		}

		if (startedByTest) viewC64->StopEmulationThread(diC64);
	}

	// --- Test 7: server/shutdown is advertised as a server-level endpoint ---
	{
		json resp = CallEndpoint(server, "server/endpoints");
		if (resp.empty() || !resp.contains("result"))
		{
			FinishTest(false, "Test 7 FAIL: server/endpoints returned empty");
			return;
		}

		const json *shutdownDesc = FindJsonArrayObjectWithFields(resp["result"]["endpoints"],
																 {{"fn", "server/shutdown"}});
		if (shutdownDesc == NULL)
		{
			FinishTest(false, "Test 7 FAIL: server/shutdown missing from server/endpoints");
			return;
		}
		if (shutdownDesc->value("category", std::string()) != "server")
		{
			FinishTest(false, "Test 7 FAIL: server/shutdown category is not 'server'");
			return;
		}
		if (!shutdownDesc->contains("paramsSchema")
			|| !(*shutdownDesc)["paramsSchema"]["properties"].contains("force"))
		{
			std::string detail = "Test 7 FAIL: server/shutdown params schema missing 'force': " + shutdownDesc->dump();
			FinishTest(false, detail.c_str());
			return;
		}
	}

	// --- Test 8: server/shutdown replies before the application quits ---
	{
		CShutdownExecutorGuard shutdownGuard;

		json params;
		params["force"] = true;
		params["graceMs"] = 0;
		params["reason"] = "remote-protocol-test";

		json resp = CallEndpointWithParams(server, "server/shutdown", params);
		if (resp.empty() || !resp.contains("result"))
		{
			FinishTest(false, "Test 8 FAIL: server/shutdown returned empty");
			return;
		}
		if (resp.value("status", 0) != HTTP_OK)
		{
			std::string detail = "Test 8 FAIL: server/shutdown status != 200: " + resp.dump();
			FinishTest(false, detail.c_str());
			return;
		}
		if (resp["result"].value("status", std::string()) != "shutting_down")
		{
			std::string detail = "Test 8 FAIL: unexpected result payload: " + resp["result"].dump();
			FinishTest(false, detail.c_str());
			return;
		}
		if (resp["result"].value("force", false) != true
			|| resp["result"].value("timeoutMs", 0) != C64D_SHUTDOWN_TIMEOUT_FORCED_MS)
		{
			std::string detail = "Test 8 FAIL: reply does not describe the request: " + resp["result"].dump();
			FinishTest(false, detail.c_str());
			return;
		}

		// The reply came back first; the shutdown itself lands on its own thread.
		bool executed = false;
		for (int elapsed = 0; elapsed < 2000 && !executed; elapsed += 10)
		{
			executed = (sRecordedShutdownCount >= 1);
			if (!executed)
				SYS_Sleep(10);
		}
		if (!executed)
		{
			FinishTest(false, "Test 8 FAIL: server/shutdown never reached the shutdown executor");
			return;
		}
		if (sRecordedShutdown.force != true
			|| sRecordedShutdown.timeoutMs != C64D_SHUTDOWN_TIMEOUT_FORCED_MS
			|| sRecordedShutdown.reason != "remote-protocol-test")
		{
			std::string detail = "Test 8 FAIL: executor got force=" + std::to_string((int)sRecordedShutdown.force)
				+ " timeoutMs=" + std::to_string(sRecordedShutdown.timeoutMs)
				+ " reason='" + sRecordedShutdown.reason + "'";
			FinishTest(false, detail.c_str());
			return;
		}
	}

	FinishTest(true, "All remote protocol tests passed (8/8)");
}

void CTestRemoteProtocol::Cancel()
{
	isRunning = false;
}
