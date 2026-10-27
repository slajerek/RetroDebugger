#include "CTestRemoteProtocol.h"
#include "EmulatorsConfig.h"
#include "CDebuggerServer.h"
#include "CDebuggerServerProtocol.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbols.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CTimelineNavigation.h"
#include "CSnapshotsManager.h"
#include "C64DShutdown.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include <string>
#include <limits>
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

// Pause only a running CPU and restore its previous mode on every test exit.
struct CAccessQueryPauseGuard
{
	CDebugInterface *di;
	uint8 mode;
	bool changed;
	CAccessQueryPauseGuard(CDebugInterface *di) : di(di), mode(di->GetDebugMode()),
		changed(di->IsEmulationRunning() && mode != DEBUGGER_MODE_PAUSED)
	{
		if (changed) di->PauseEmulationBlockedWait();
	}
	void Restore()
	{
		if (changed) di->SetDebugMode(mode);
		changed = false;
	}
	~CAccessQueryPauseGuard() { Restore(); }
};

static json ExpectedAccessRecord(const DebugMemoryCellAccess &access)
{
	json result = {{"available", access.IsAvailable()}, {"pc", nullptr}, {"cycle", nullptr},
		{"frame", nullptr}, {"rasterLine", nullptr}, {"rasterCycle", nullptr}};
	if (access.IsAvailable())
	{
		result["pc"] = access.pc;
		result["cycle"] = access.cycle;
		if (access.frame != (u32)-1) result["frame"] = access.frame;
		if (access.rasterLine >= 0) result["rasterLine"] = access.rasterLine;
		if (access.rasterCycle >= 0) result["rasterCycle"] = access.rasterCycle;
	}
	return result;
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

// Exercise both registered native handlers through the real endpoint dispatcher.
static bool TestMemoryReadEndpoints(CDebuggerServer *server)
{
	std::vector<json> invalid = {
		true, 42, 1.5, "invalid", nullptr, json::array(),
		json::object(), {{"address", 0}}, {{"size", 1}},
		{{"address", 65536}, {"size", 1}}, {{"address", 65537}, {"size", 0}},
		{{"address", 65535}, {"size", 2}}, {{"address", 0}, {"size", 65537}},
		{{"address", 2147483647}, {"size", 1}}
	};
	for (const json &value : std::vector<json>{-1, false, true, 1.0, 1.5, "1", nullptr, json::array(), json::object(),
		std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), std::numeric_limits<uint64_t>::max(),
		std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(),
		uint64_t(2147483648), uint64_t(4294967295), uint64_t(4294967296)})
	{
		invalid.push_back({{"address", value}, {"size", 1}});
		invalid.push_back({{"address", 0}, {"size", value}});
	}
	for (const char *fn : {"c64/cpu/memory/readBlock", "c64/ram/readBlock"})
	{
		auto Check = [&](const json &params, int status, int size) -> bool
		{
			auto *result = server->RunEndpointFunction(fn, "read-test", params, nullptr, 0);
			if (!result) return false;
			std::string raw(result->begin(), result->end());
			delete result;
			auto separator = raw.find('\0');
			int binarySize = separator == std::string::npos ? 0 : static_cast<int>(raw.size() - separator - 1);
			json response = json::parse(raw.substr(0, separator), nullptr, false);
			return response.is_object() && response.contains("status") && response.contains("token")
				&& response.at("status") == status && response.at("token") == "read-test" && binarySize == size;
		};
		for (const json &params : invalid)
			if (!Check(params, HTTP_PAYLOAD_TOO_LARGE, 0)) return false;
		for (const json &params : std::vector<json>{
			{{"address", 65536}, {"size", 0}}, {{"address", 65535}, {"size", 1}},
			{{"address", 0}, {"size", 65536}}, {{"address", uint64_t(0)}, {"size", uint64_t(1)}}})
			if (!Check(params, HTTP_OK, params.at("size").get<int>())) return false;
	}
	return true;
}

// Reuse the already-running C64 fixture.  No new test registration or project
// wiring is needed, and all recording settings/mode are restored afterward.
static bool TestTimelineCompletionEndpoints(CDebuggerServer *server, CDebugInterface *di)
{
	CSnapshotsManager *manager = di->snapshotsManager;
	const auto saved = manager->GetFrameNavigationStatus();
	const uint8 savedMode = di->GetDebugMode();
	manager->SetRecordingIsActive(true);
	manager->SetRecordingStoreInterval(1);
	manager->SetRecordingLimit(saved.retention < 32 ? 32 : saved.retention);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(500);
	di->PauseEmulationBlockedWait();
	auto history = manager->GetFrameNavigationStatus();
	bool valid = history.snapshots >= 3 && history.newestFrame > history.oldestFrame + 1;
	if (valid)
	{
		const int target = static_cast<int>(history.newestFrame - 1);
		json response = CallEndpointWithParams(server, "c64/timeline/frame", {{"frame", target}, {"waitMs", 5000}});
		json result = response.value("result", json::object());
		uint64_t requestId = result.value("requestId", uint64_t(0));
		valid = response.value("status", 0) == HTTP_OK && result.value("status", std::string()) == "completed"
			&& requestId != 0 && result.value("completionKnown", false)
			&& result.value("requestedFrame", -1) == target
			&& result["observedFrame"] == di->GetEmulationFrameNumber()
			&& result["observedCycle"] == di->GetMainCpuCycleCounter()
			&& di->GetDebugMode() == DEBUGGER_MODE_PAUSED;
		json followup = CallEndpointWithParams(server, "c64/timeline/status", {{"requestId", requestId}});
		valid = valid && followup.value("status", 0) == HTTP_OK && followup["result"] == result;
		if (valid)
		{
			// Deterministically prevent dequeue, without a timed wait under the
			// manager lock.  Unknown pause/queue state cannot prove completion.
			manager->LockMutex();
			auto admission = manager->RequestFrameNavigation(target, &requestId);
			manager->NotifyFrameNavigationPaused();
			auto pending = manager->WaitForFrameNavigation(requestId, 0);
			manager->CancelRestore();
			manager->UnlockMutex();
			json cancelled = CallEndpointWithParams(server, "c64/timeline/status", {{"requestId", requestId}});
			valid = admission.accepted && std::string(pending.status) == "pending"
				&& cancelled.value("status", 0) == HTTP_OK && cancelled["result"]["status"] == "cancelled"
				&& cancelled["result"]["reason"] == "cancelled" && cancelled["result"]["completionKnown"] == false
				&& cancelled["result"]["observedFrame"].is_null();
		}
	}
	if (!valid) manager->CancelRestore();
	manager->SetRecordingIsActive(saved.recording);
	manager->SetRecordingStoreInterval(saved.intervalFrames);
	manager->SetRecordingLimit(saved.retention);
	di->SetDebugMode(savedMode);
	return valid;
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

		// --- Test 10: native block read validation and compatible block edges ---
		// Run while the existing C64 fixture is active; keep tests 7-9 unchanged.
		if (!TestMemoryReadEndpoints(server))
		{
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, "Test 10 FAIL: native memory read range/status/binary-size mismatch");
			return;
		}
		// --- Test 12: real CPU-thread completion and request-specific cancellation ---
		if (!TestTimelineCompletionEndpoints(server, diC64))
		{
			if (startedByTest) viewC64->StopEmulationThread(diC64);
			FinishTest(false, "Test 12 FAIL: native timeline completion/observation/cancellation mismatch");
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

	// --- Test 9: actual last-access endpoints match records and do not mutate ---
	{
		// Missing interfaces/maps must not silently reduce enabled-platform coverage.
		std::vector<std::string> expectedPlatforms;
#ifdef RUN_COMMODORE64
		expectedPlatforms.push_back("c64");
#endif
#ifdef RUN_ATARI
		expectedPlatforms.push_back("atari800");
#endif
#ifdef RUN_NES
		expectedPlatforms.push_back("nes");
#endif
		if (expectedPlatforms.empty())
		{
			FinishTest(false, "Test 9 FAIL: no CPU platform enabled for last-access coverage");
			return;
		}
		for (const std::string &platform : expectedPlatforms)
		{
			CDebugInterface *di = NULL;
			for (CDebugInterface *candidate : viewC64->debugInterfaces)
			{
				if (candidate && platform == candidate->GetPlatformNameEndpointString())
				{
					di = candidate;
					break;
				}
			}
			if (!di || !di->symbols || !di->symbols->memory || !di->GetDebuggerApi())
			{
				std::string detail = "Test 9 FAIL: enabled CPU marker endpoint unavailable for " + platform;
				FinishTest(false, detail.c_str());
				return;
			}
			CAccessQueryPauseGuard pauseGuard(di);
			CDebugMemoryCell *cell = di->symbols->memory->GetMemoryCell(0x2345);
			if (!cell)
			{
				pauseGuard.Restore();
				FinishTest(false, "Test 9 FAIL: CPU marker map missing test address");
				return;
			}
			DebugMemoryCellLastAccess before = cell->GetLastAccess();
			uint8 mode = di->GetDebugMode();
			std::string endpoint = platform + "/cpu/memory/lastAccess";
			json expected = {{"platform", platform}, {"address", 0x2345}, {"addressSpace", "cpu"},
				{"read", ExpectedAccessRecord(before.read)}, {"write", ExpectedAccessRecord(before.write)}};
			json response = CallEndpointWithParams(server, endpoint.c_str(), {{"address", 0x2345}});
			DebugMemoryCellLastAccess after = cell->GetLastAccess();
			if (response.value("status", 0) != HTTP_OK || response.value("result", json()) != expected
				|| di->GetDebugMode() != mode
				|| ExpectedAccessRecord(after.read) != expected["read"]
				|| ExpectedAccessRecord(after.write) != expected["write"])
			{
				std::string detail = "Test 9 FAIL: last-access payload/state mismatch for " + platform;
				pauseGuard.Restore();
				FinishTest(false, detail.c_str());
				return;
			}
			// Exercise raw endpoint validation, independently of the MCP schema.
			for (const auto &address : json::array({nullptr, true, "0", 1.5, -1, 65536,
				18446744073709551615ULL}))
			{
				response = CallEndpointWithParams(server, endpoint.c_str(), {{"address", address}});
				if (response.value("status", 0) != HTTP_BAD_REQUEST || di->GetDebugMode() != mode)
				{
					pauseGuard.Restore();
					FinishTest(false, "Test 9 FAIL: invalid raw last-access address accepted");
					return;
				}
			}
			response = CallEndpointWithParams(server, endpoint.c_str(), json::object());
			if (response.value("status", 0) != HTTP_BAD_REQUEST)
			{
				pauseGuard.Restore();
				FinishTest(false, "Test 9 FAIL: missing raw last-access address accepted");
				return;
			}
			pauseGuard.Restore();
			std::string detail = "Last-access endpoint verified for " + platform;
			StepCompleted(9, true, detail.c_str());
		}
	}

	// --- Test 11: real read-only frame status and raw validation (no navigation) ---
	{
#ifdef RUN_COMMODORE64
		CDebugInterface *di = NULL;
		for (CDebugInterface *candidate : viewC64->debugInterfaces)
			if (candidate && std::string(candidate->GetPlatformNameEndpointString()) == "c64") di = candidate;
		if (!di)
		{
			FinishTest(false, "Test 11 FAIL: enabled C64 interface missing"); return;
		}
		CAccessQueryPauseGuard pauseGuard(di);
		const uint8 mode = di->GetDebugMode();
		const u64 cycle = di->GetMainCpuCycleCounter();
		json response = CallEndpointWithParams(server, "c64/timeline/status", json::object());
		if (response.value("status", 0) != HTTP_OK || !response.contains("result")
			|| response["result"].value("navigation", std::string()) != "native_frame"
			|| response["result"].value("completionReporting", std::string()) != "acceptance_only"
			|| !response["result"].contains("storedSnapshots")
			|| !response["result"].contains("oldestFrame") || !response["result"].contains("newestFrame")
			|| di->GetDebugMode() != mode || di->GetMainCpuCycleCounter() != cycle)
		{
			pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: timeline status/state mismatch"); return;
		}
		for (const auto &params : json::array({json::object(), {{"frame", -1}}, {{"frame", true}},
			{{"frame", 1.5}}, {{"frame", "1"}}, {{"frame", 18446744073709551615ULL}},
			{{"frame", 1}, {"extra", true}}}))
		{
			response = CallEndpointWithParams(server, "c64/timeline/frame", params);
			if (response.value("status", 0) != HTTP_BAD_REQUEST
				|| di->GetDebugMode() != mode || di->GetMainCpuCycleCounter() != cycle)
			{
				pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: invalid raw frame request accepted/mutated"); return;
			}
		}
		for (const auto &value : json::array({nullptr, true, "1", 1.0, -1, 5001, 18446744073709551615ULL}))
		{
			response = CallEndpointWithParams(server, "c64/timeline/frame", {{"frame", 1}, {"waitMs", value}});
			if (response.value("status", 0) != HTTP_BAD_REQUEST
				|| di->GetDebugMode() != mode || di->GetMainCpuCycleCounter() != cycle)
			{ pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: invalid raw wait accepted/mutated"); return; }
		}
		for (const auto &params : json::array({{{"waitMs", 1}}, {{"requestId", 0}}, {{"requestId", true}},
			{{"requestId", 1.0}}, {{"requestId", "1"}}, {{"requestId", -1}}, {{"requestId", 1}, {"extra", true}}}))
		{
			response = CallEndpointWithParams(server, "c64/timeline/status", params);
			if (response.value("status", 0) != HTTP_BAD_REQUEST)
			{ pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: invalid raw request observation accepted"); return; }
		}
		response = CallEndpointWithParams(server, "c64/timeline/status", {{"requestId", 18446744073709551615ULL}});
		if (response.value("status", 0) != HTTP_OK || response["result"].value("status", std::string()) != "unavailable"
			|| response["result"].value("completionKnown", true) || !response["result"]["observedFrame"].is_null()
			|| di->GetDebugMode() != mode || di->GetMainCpuCycleCounter() != cycle)
		{ pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: request observation mutated/fabricated completion"); return; }
		for (int waitMs : {0, TimelineNavigation::MaxWaitMs})
		{
			// Frame zero is always outside the strict retained range: refusal
			// must not turn into a pending request merely because waitMs exists.
			response = CallEndpointWithParams(server, "c64/timeline/frame", {{"frame", 0}, {"waitMs", waitMs}});
			if (response.value("status", 0) != HTTP_OK || response["result"].value("status", std::string()) != "refused"
				|| response["result"].value("accepted", true) || response["result"].contains("requestId")
				|| di->GetDebugMode() != mode || di->GetMainCpuCycleCounter() != cycle)
			{ pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: refused frame acquired completion ownership"); return; }
		}
		response = CallEndpointWithParams(server, "c64/timeline/status", {{"frame", 1}});
		if (response.value("status", 0) != HTTP_BAD_REQUEST)
		{
			pauseGuard.Restore(); FinishTest(false, "Test 11 FAIL: status accepted frame"); return;
		}
		pauseGuard.Restore();
#endif
		for (CDebugInterface *di : viewC64->debugInterfaces)
		{
			if (!di) continue;
			std::string platform = di->GetPlatformNameEndpointString();
			if (platform != "atari800" && platform != "nes") continue;
			json response = CallEndpointWithParams(server, (platform + "/timeline/status").c_str(), json::object());
			if (response.value("status", 0) != HTTP_BAD_REQUEST || response["result"].value("error", std::string()) != "unsupported_platform")
			{
				FinishTest(false, "Test 11 FAIL: non-C64 timeline not refused"); return;
			}
		}
	}

	FinishTest(true, "All remote protocol tests passed (12/12)");
}

void CTestRemoteProtocol::Cancel()
{
	isRunning = false;
}
