#include "RetroDebuggerAppInit.h"
#include "DBG_Log.h"
#include "CSlrImage.h"
#include "RES_ResourceManager.h"
#include "GUI_Main.h"
#include "MT_API.h"
#include "C64CommandLine.h"
#include "CViewC64.h"
#include "SYS_Defs.h"
#include "RetroDebuggerEmbeddedData.h"
#include "CTestSuiteRetroDebugger.h"
#include "CTestRunner.h"
#include "VID_Main.h"
#include "SYS_CommandLine.h"
#include "SYS_Main.h"
#include "CMCPServer.h"
#include "C64SettingsStorage.h"
#include "C64DUiScale.h"
#include <cstring>
#include <cstdio>


#ifdef __APPLE__
extern "C" void RD_HideDockIcon();
#endif

#ifdef WIN32
extern "C" void archdep_init_stdio(void);
#endif

#if MT_ENABLE_IMGUI_TEST_ENGINE
#include "CImGuiTestEngine.h"
#include "imgui_te_engine.h"
extern void RegisterRetroDebuggerTests(ImGuiTestEngine *engine);
static bool sRunTests = false;
static const char *sImGuiTestFilter = NULL;
static int sWarmupFrames = 0;
static bool sTestsQueued = false;
static const char *sImGuiResultsFilePath = "tests/results/last_run.txt";

static void WriteImGuiTestResults(const char *failureLabel, const char *failureSummary)
{
	FILE *f = fopen(sImGuiResultsFilePath, "w");
	if (!f)
	{
		LOGError("WriteImGuiTestResults: Failed to open %s", sImGuiResultsFilePath);
		return;
	}

	int passed = 0;
	int total = 0;

	if (failureLabel != NULL)
	{
		fprintf(f, "[%s] FAIL: %s\n", failureLabel, failureSummary);
		total = 1;
	}
	else
	{
		ImGuiTestEngine *engine = CImGuiTestEngine::GetEngine();
		if (engine != NULL)
		{
			ImVector<ImGuiTest *> tests;
			ImGuiTestEngine_GetTestList(engine, &tests);
			for (int i = 0; i < tests.Size; i++)
			{
				ImGuiTest *test = tests[i];
				ImGuiTestStatus status = test->Output.Status;
				if (status == ImGuiTestStatus_Unknown || status == ImGuiTestStatus_Queued || status == ImGuiTestStatus_Running)
					continue;

				bool success = (status == ImGuiTestStatus_Success);
				const char *summary = success ? "ImGui test passed" : "ImGui test failed";
				ImGuiTextBuffer failureLog;
				if (!success && !test->Output.Log.IsEmpty())
				{
					test->Output.Log.ExtractLinesForVerboseLevels(ImGuiTestVerboseLevel_Error, ImGuiTestVerboseLevel_Trace, &failureLog);
					if (!failureLog.empty())
						summary = failureLog.c_str();
				}
				fprintf(f, "[%s/%s] %s: %s\n",
						test->Category ? test->Category : "imgui",
						test->Name ? test->Name : "unnamed",
						success ? "PASS" : "FAIL",
						summary);
				total++;
				if (success)
					passed++;
			}
		}
	}

	fprintf(f, "---\n");
	fprintf(f, "RESULT: %d/%d passed\n", passed, total);
	fclose(f);

	LOGM("ImGui test results written to %s", sImGuiResultsFilePath);
}
#endif

// CTestSuite CLI flags
static bool sRunSuiteTest = false;
static bool sRunSuiteAll = false;
static bool sListTests = false;
static bool sExitAfterTests = false;
static bool sStartMCPServer = false;
static bool sStartMCPBridge = false;
static const char *sMCPBridgeHost = "127.0.0.1";
static int sMCPBridgePort = 0x0DEB;
static const char *sMCPBridgePath = "/stream";
static const char *sSuiteTestName = NULL;
static bool sSuiteTestScheduled = false;
static int sSuiteWarmupFrames = 0;

static void WriteCliFailureResult(const char *label, const char *summary)
{
	FILE *f = fopen("tests/results/last_run.txt", "w");
	if (!f)
	{
		LOGError("WriteCliFailureResult: Failed to open tests/results/last_run.txt");
		return;
	}
	fprintf(f, "[%s] FAIL: %s\n", label, summary);
	fprintf(f, "---\n");
	fprintf(f, "RESULT: 0/1 passed\n");
	fclose(f);
}

const char *MT_GetMainWindowTitle()
{
#if !MT_DEBUG_LOGS
	return "Retro Debugger v" RETRODEBUGGER_VERSION_STRING;
#else
	return "Retro Debugger v" RETRODEBUGGER_VERSION_STRING " (compiled on " __DATE__ " " __TIME__ ")";
#endif
}

// Defined below, next to MT_PreInit's audio guard -- both answer the same
// question: is this an automated run that must not touch this machine?
bool C64D_IsAutomatedRunCommandLine();

const char *MT_GetSettingsFolderName()
{
	// A test run must never write into the user's settings folder. The app
	// rewrites layouts.dat, imgui.ini and settings.dat on every shutdown, so a
	// run pointed at the real folder silently replaces a workspace the user
	// spent time building -- which is what happened on 2026-09-08, from
	// invoking the binary directly instead of through tests/run_test.sh.
	//
	// MT_SETTINGS_DIR (honoured by the engine's SYS_InitFileSystem) is the way
	// to redirect it, and tests/run_test.sh sets it. This is the backstop for
	// when it is not set: a command line carrying a test switch gets its own
	// folder name, so even a direct invocation cannot reach the real one.
	if (C64D_IsAutomatedRunCommandLine())
		return "RetroDebugger-tests";

	return "RetroDebugger";
}

void MT_GetDefaultWindowPositionAndSize(int *defaultWindowPosX, int *defaultWindowPosY, int *defaultWindowWidth, int *defaultWindowHeight, bool *maximized)
{
	// First run only -- once the window has been moved or resized these are
	// replaced by the stored MainWindow* keys. The engine asks for this during
	// VID_Init, before there is a window and before C64D_UiScaleInitEarly has
	// run, so the scale comes from the primary display rather than from the
	// resolved setting.
	float scale = MT_DetectDisplayUiScale();

	*defaultWindowPosX = (int)(50 * scale); //SDL_WINDOWPOS_CENTERED;
	*defaultWindowPosY = (int)(125 * scale); //SDL_WINDOWPOS_CENTERED;
	*defaultWindowWidth = (int)(510 * scale);
	*defaultWindowHeight = (int)(510 * 9 / 16 * scale);
	*maximized = true;
}

// Headless runs (the CLI test suites) must never touch the machine's real
// audio output. c64d is sound-heavy -- SID emulation drives the audio
// callback continuously -- and a CI/VM box's sound device is not something a
// test run should be opening at all. SDL's "dummy" driver is a fake device
// that consumes silently and paces itself, so no loopback device is needed.
// This mirrors the private apps' pre-init audio-driver guard exactly,
// including the overwrite=0 semantics (an explicitly exported
// SDL_AUDIODRIVER still wins) and running in MT_PreInit, before the engine's
// SDL_Init(SDL_INIT_AUDIO).
//
// THIS IS HYGIENE, NOT AN OOM FIX. It was added during the first Linux run
// while a large-RSS OOM was being investigated, and the obvious theory --
// an unthrottled audio callback pulling frames as fast as it can, running
// the emulator far past 1x -- was tested directly and DISPROVED: RSS was
// identical with and without SDL_AUDIODRIVER forced to dummy, because SDL3's
// dummy driver self-paces via SDL_Delay (SDL_dummyaudio.c) and the real ALSA
// device was not racing either. The actual memory cost is CDebugMemory's
// eager per-byte CDebugMemoryCell allocation across every emulator's full
// address space, each cell reserving two history ring buffers up front --
// >1GB before any test runs. See the Linux-SDL3 findings notes, items 9 and
// 10. Do not "restore" an audio-throughput explanation here; it was measured.
static bool C64D_IsHeadlessCommandLine()
{
	for (int i = 0; i < (int)sysCommandLineArguments.size(); i++)
	{
		const char *arg = sysCommandLineArguments[i];
		if (strcmp(arg, "--headless") == 0 || strcmp(arg, "--mcp-headless") == 0)
			return true;
	}
	return false;
}

// Any run driven by a test switch, headless or not.
//
// An automated run must not touch the machine it runs on: it must not play
// audio out of the speakers, and it must not write into the user's settings
// folder. Both used to key off --headless alone, so `--run-suite` (which shows
// a window) went to the real audio device and the real ~/Library/RetroDebugger
// -- audible, and it overwrote layouts.dat/imgui.ini on shutdown.
bool C64D_IsAutomatedRunCommandLine()
{
	if (C64D_IsHeadlessCommandLine())
		return true;

	for (int i = 0; i < (int)sysCommandLineArguments.size(); i++)
	{
		const char *arg = sysCommandLineArguments[i];
		if (strcmp(arg, "--run-suite") == 0 || strcmp(arg, "--run-test") == 0
			|| strcmp(arg, "--run-tests") == 0 || strcmp(arg, "--run-imgui-test") == 0
			|| strcmp(arg, "--exit-after-tests") == 0)
			return true;
	}
	return false;
}

void MT_PreInit()
{
#ifdef WIN32
	// Configure stdio before MCP can block reading stdin; VICE reuses this setup.
	archdep_init_stdio();
#endif

	if (C64D_IsAutomatedRunCommandLine())
		// SDL3: SDL_setenv is gone. SDL_setenv_unsafe is the direct
		// replacement and keeps the overwrite=0 semantics we rely on.
		// ("unsafe" is about thread safety against concurrent getenv, not
		// about correctness -- this runs before any thread exists.)
		SDL_setenv_unsafe("SDL_AUDIODRIVER", "dummy", 0);

	C64DebuggerInitStartupTasks();
	C64DebuggerParseCommandLine0();

	// Override the MTEngineSDL default (follow OS) — retrodebugger defaults to dark theme
	VID_SetAppDefaultImGuiStyle(IMGUI_STYLE_DARK);

	// Start MCP server as early as possible — the JSON-RPC thread responds
	// to the initialize handshake immediately. Tool registration is deferred
	// to the first tools/list call (when viewC64 is ready).
	// Parse MCP flags here since MT_PostInit hasn't run yet.
	for (int i = 0; i < (int)sysCommandLineArguments.size(); i++)
	{
		const char *arg = sysCommandLineArguments[i];
		if (strcmp(arg, "--mcp-server") == 0 || strcmp(arg, "--mcp-headless") == 0)
		{
			sStartMCPServer = true;
		}
		else if (strcmp(arg, "--mcp-live") == 0)
		{
			sStartMCPBridge = true;
		}
		else if (strcmp(arg, "--host") == 0 && i + 1 < (int)sysCommandLineArguments.size())
		{
			sMCPBridgeHost = sysCommandLineArguments[++i];
		}
		else if (strcmp(arg, "--port") == 0 && i + 1 < (int)sysCommandLineArguments.size())
		{
			sMCPBridgePort = atoi(sysCommandLineArguments[++i]);
		}
		else if (strcmp(arg, "--path") == 0 && i + 1 < (int)sysCommandLineArguments.size())
		{
			sMCPBridgePath = sysCommandLineArguments[++i];
		}
	}
	if (sStartMCPBridge)
	{
		// Set headless before returning to SYS_Startup — this ensures the
		// SDL_HINT_MAC_BACKGROUND_APP hint is set and no window is created.
		gHeadlessMode = true;
#ifdef __APPLE__
		// SDL_HINT_MAC_BACKGROUND_APP is only applied when SDL_INIT_VIDEO runs,
		// which is skipped in headless mode. Call setActivationPolicy directly
		// so the bridge process has no Dock icon.
		RD_HideDockIcon();
#endif
		MCP_BridgeStart(sMCPBridgeHost, sMCPBridgePort, sMCPBridgePath);
	}
	else if (sStartMCPServer)
	{
		MCP_ServerStart();
	}
}

void MT_GuiPreInit()
{
}

void MT_PostInit()
{
	LOGD("MT_PostInit");

	// Parse CLI flags early (before view creation)
	for (int i = 0; i < (int)sysCommandLineArguments.size(); i++)
	{
		const char *arg = sysCommandLineArguments[i];
		if (strcmp(arg, "--run-test") == 0)
		{
			if (i + 1 >= (int)sysCommandLineArguments.size())
			{
				const char *failureSummary = "--run-test requires a test name";
				LOGError("%s", failureSummary);
				WriteCliFailureResult("cli", failureSummary);
				SYS_CleanExit();
			}
			sRunSuiteTest = true;
			sSuiteTestName = sysCommandLineArguments[i + 1];
			i++;
		}
		else if (strcmp(arg, "--run-suite") == 0)
		{
			sRunSuiteAll = true;
		}
		else if (strcmp(arg, "--list-tests") == 0)
		{
			sListTests = true;
			gHeadlessMode = true;
		}
		else if (strcmp(arg, "--exit-after-tests") == 0)
		{
			sExitAfterTests = true;
		}
		else if (strcmp(arg, "--headless") == 0)
		{
			gHeadlessMode = true;
		}
		else if (strcmp(arg, "--mcp-server") == 0)
		{
			sStartMCPServer = true;
		}
		else if (strcmp(arg, "--mcp-headless") == 0)
		{
			sStartMCPServer = true;
			gHeadlessMode = true;
		}
		else if (strcmp(arg, "--mcp-live") == 0)
		{
			sStartMCPBridge = true;
			gHeadlessMode = true;
		}
#if MT_ENABLE_IMGUI_TEST_ENGINE
		else if (strcmp(arg, "--run-tests") == 0)
		{
			sRunTests = true;
		}
		else if (strcmp(arg, "--run-imgui-test") == 0)
		{
			if (i + 1 >= (int)sysCommandLineArguments.size())
			{
				const char *failureSummary = "--run-imgui-test requires a filter";
				LOGError("%s", failureSummary);
				WriteCliFailureResult("cli", failureSummary);
				SYS_CleanExit();
			}
			sRunTests = true;
			sImGuiTestFilter = sysCommandLineArguments[i + 1];
			i++;
		}
#endif
	}

	// Disable ImGui ini saving in headless mode to avoid overwriting user's layout
	#if MT_ENABLE_IMGUI_TEST_ENGINE
	if (sRunTests && (sRunSuiteTest || sRunSuiteAll))
	{
		const char *failureSummary = "Cannot combine CTestSuite and ImGui test CLI flags";
		LOGError("%s", failureSummary);
		WriteImGuiTestResults("cli", failureSummary);
		SYS_CleanExit();
	}
	#endif

	if (gHeadlessMode)
	{
		ImGui::GetIO().IniFilename = NULL;
	}

	// Set CLI mode flags before view creation
	if (sRunSuiteTest || sRunSuiteAll
#if MT_ENABLE_IMGUI_TEST_ENGINE
		|| sRunTests
#endif
	)
	{
		CTestSuite::isCLIModeActive = true;
		CTestRunner::isTestPending = true;
	}

	RetroDebuggerEmbeddedAddData();

	// BEFORE the views exist: resolves the HiDPI UI scale and applies it to the
	// ImGui style, so every view constructor's MT_UiScaled() default is
	// already right. See the HiDPI UI scaling design notes.
	C64D_UiScaleInitEarly();

	CViewC64 *viewC64 = new CViewC64(0, 0, -1, SCREEN_WIDTH, SCREEN_HEIGHT);
	guiMain->SetView(viewC64);

	// AFTER every view exists (CViewC64's constructor builds them all, plugins
	// included): upgrades layouts.dat and imgui.ini when they were written at a
	// different scale. Needs the live views to know each layout parameter's
	// type, and runs before the first frame so ImGui reads the upgraded ini.
	C64D_UiScaleMigratePersistedGeometry();

	if (sListTests)
	{
		// Enumerate registered tests and exit (used by the subprocess-per-test
		// orchestrator). Done after view creation so test construction has the
		// same app context it gets in a normal run.
		CTestSuite::ListTestsFromCLI(new CTestSuiteRetroDebugger());
	}

	VID_SetFPS(5);

#if MT_ENABLE_IMGUI_TEST_ENGINE
	// ONLY when ImGui tests were actually asked for.
	//
	// This used to be unconditional, so every ordinary launch created a test
	// engine, registered the whole ImGui test suite against it and left it
	// hooked into the frame loop -- ImGuiTestEngine_PostSwap() every frame, an
	// input path that can substitute simulated input, and a crash handler that
	// replaces the app's own. None of that belongs in a debugging session, and
	// on Windows nobody had noticed because MT_ENABLE_IMGUI_TEST_ENGINE only
	// started being defined here when the capability manifest turned
	// MT_CAP_TEST_ENGINE on -- a build that did not compile until the
	// windows.h Yield() collision in CImGuiTests.cpp was fixed.
	//
	// Safe to skip: every CImGuiTestEngine entry point returns harmlessly on a
	// NULL engine, both engine-side users (VID_ForwardTestEngineInputToGuiMain
	// and the input-suppression check in VID_ProcessEvents) test for NULL
	// first, Shutdown() is guarded by its own "initialized" flag, and nothing
	// in this app opens the test-engine UI interactively. The CTestSuite
	// flags (--run-suite / --run-test) do not use this engine at all.
	if (sRunTests)
	{
		CImGuiTestEngine::Init();
		RegisterRetroDebuggerTests(CImGuiTestEngine::GetEngine());
	}
#endif
}

void MT_Render()
{
	// CTestSuite CLI scheduling
	// Note: scheduled here (not in MT_PostRenderEndFrame) because Metal's
	// nextDrawable blocks on hidden windows in headless mode, preventing
	// MT_PostRenderEndFrame from ever being called. Runs on first frame
	// since ImGui state is valid by this point (guiMain->RenderImGui()
	// has already executed in the same frame).
	if ((sRunSuiteTest || sRunSuiteAll) && !sSuiteTestScheduled)
	{
		sSuiteWarmupFrames++;
		if (sSuiteWarmupFrames >= 1)
		{
			sSuiteTestScheduled = true;
			if (viewC64->testRunner == NULL)
			{
				viewC64->testRunner = new CTestRunner();
			}
			CTestSuiteRetroDebugger *suite = new CTestSuiteRetroDebugger();
			if (sRunSuiteTest)
			{
				CTestSuite::RunFromCLI(suite, sSuiteTestName);
			}
			else
			{
				CTestSuite::RunFromCLI(suite, NULL);
			}
		}
	}

	// MCP server auto-start from saved settings (when user toggled it on
	// via Settings > Remote menu). Only start once, on the first render frame
	// so that viewC64 and all debug interfaces are fully initialized.
	static bool sMCPAutoStartChecked = false;
	if (!sMCPAutoStartChecked && !sStartMCPServer && c64SettingsRunMCPServer && mcpServer == NULL)
	{
		sMCPAutoStartChecked = true;
		MCP_ServerStart();
		// MCP tools dispatch through the WebSocket server's endpoint map
		if (viewC64->debuggerServer == NULL)
		{
			viewC64->DebuggerServerWebSocketsStart();
		}
	}
}

void MT_PostRenderEndFrame()
{
	#if MT_ENABLE_IMGUI_TEST_ENGINE
	CImGuiTestEngine::PostSwap();

	if (sRunTests && !sTestsQueued)
	{
		sWarmupFrames++;
		if (sWarmupFrames >= 10)
		{
			ImGuiTestEngine *engine = CImGuiTestEngine::GetEngine();
			if (engine != NULL)
			{
				ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests, sImGuiTestFilter, ImGuiTestRunFlags_RunFromCommandLine);
			}
			sTestsQueued = true;

			ImVector<ImGuiTestRunTask> queuedTests;
			if (engine != NULL)
			{
				ImGuiTestEngine_GetTestQueue(engine, &queuedTests);
			}
			if (queuedTests.Size == 0)
			{
				const char *failureLabel = (sImGuiTestFilter != NULL) ? sImGuiTestFilter : "imgui";
				const char *failureSummary = (sImGuiTestFilter != NULL) ? "No ImGui tests matched filter" : "No ImGui tests were queued";
				LOGError("ImGui CLI: %s", failureSummary);
				WriteImGuiTestResults(failureLabel, failureSummary);
				CTestRunner::isTestPending = false;
				if (sExitAfterTests)
				{
					SYS_Shutdown();
					return;
				}
			}
		}
	}
	if (sExitAfterTests && sTestsQueued && CImGuiTestEngine::IsTestQueueEmpty())
	{
		int tested = 0, success = 0;
		CImGuiTestEngine::GetResultSummary(&tested, &success);
		WriteImGuiTestResults(NULL, NULL);
		CTestRunner::isTestPending = false;
		LOGM("TEST RESULTS: %d/%d passed", success, tested);
		// Write the same results file the CTestSuite path writes, so the
		// runner and CI parse one format for both suites instead of grepping
		// log text for a number.
		CImGuiTestEngine::WriteResults();
		if (success == tested) {
			LOGM("ALL TESTS PASSED");
		} else {
			LOGM("SOME TESTS FAILED (%d failures)", tested - success);
		}
		SYS_Shutdown();
	}
#endif
}

void MT_Shutdown()
{
#if MT_ENABLE_IMGUI_TEST_ENGINE
	CImGuiTestEngine::Shutdown();
#endif
}
