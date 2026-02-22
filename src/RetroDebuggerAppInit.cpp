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
#include "CTestSuite.h"
#include "CTestRunner.h"
#include "VID_Main.h"
#include "SYS_CommandLine.h"
#include "SYS_Main.h"
#include <cstring>

#ifdef ENABLE_IMGUI_TEST_ENGINE
#include "CImGuiTestEngine.h"
#include "imgui_te_engine.h"
extern void RegisterRetroDebuggerTests(ImGuiTestEngine *engine);
static bool sRunTests = false;
static int sWarmupFrames = 0;
static bool sTestsQueued = false;
#endif

// CTestSuite CLI flags
static bool sRunSuiteTest = false;
static bool sRunSuiteAll = false;
static bool sExitAfterTests = false;
static const char *sSuiteTestName = NULL;
static bool sSuiteTestScheduled = false;
static int sSuiteWarmupFrames = 0;

const char *MT_GetMainWindowTitle()
{
#if defined(GLOBAL_DEBUG_OFF)
	return "Retro Debugger v" RETRODEBUGGER_VERSION_STRING;
#else
	return "Retro Debugger v" RETRODEBUGGER_VERSION_STRING " (compiled on " __DATE__ " " __TIME__ ")";
#endif
}

const char *MT_GetSettingsFolderName()
{
	return "RetroDebugger";
}

void MT_GetDefaultWindowPositionAndSize(int *defaultWindowPosX, int *defaultWindowPosY, int *defaultWindowWidth, int *defaultWindowHeight, bool *maximized)
{
	*defaultWindowPosX = 50; //SDL_WINDOWPOS_CENTERED;
	*defaultWindowPosY = 125; //SDL_WINDOWPOS_CENTERED;
	*defaultWindowWidth = 510;
	*defaultWindowHeight = 510*9/16;
	*maximized = true;
}

void MT_PreInit()
{
	C64DebuggerInitStartupTasks();
	C64DebuggerParseCommandLine0();
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
		if (strcmp(arg, "--run-test") == 0 && i + 1 < (int)sysCommandLineArguments.size())
		{
			sRunSuiteTest = true;
			sSuiteTestName = sysCommandLineArguments[i + 1];
			i++;
		}
		else if (strcmp(arg, "--run-suite") == 0)
		{
			sRunSuiteAll = true;
		}
		else if (strcmp(arg, "--exit-after-tests") == 0)
		{
			sExitAfterTests = true;
		}
		else if (strcmp(arg, "--headless") == 0)
		{
			gHeadlessMode = true;
		}
#ifdef ENABLE_IMGUI_TEST_ENGINE
		else if (strcmp(arg, "--run-tests") == 0)
		{
			sRunTests = true;
		}
#endif
	}

	// Disable ImGui ini saving in headless mode to avoid overwriting user's layout
	if (gHeadlessMode)
	{
		ImGui::GetIO().IniFilename = NULL;
	}

	// Set CLI mode flags before view creation
	if (sRunSuiteTest || sRunSuiteAll)
	{
		CTestSuite::isCLIModeActive = true;
		CTestRunner::isTestPending = true;
	}

	RetroDebuggerEmbeddedAddData();

	CViewC64 *viewC64 = new CViewC64(0, 0, -1, SCREEN_WIDTH, SCREEN_HEIGHT);
	guiMain->SetView(viewC64);

	VID_SetFPS(5);

#ifdef ENABLE_IMGUI_TEST_ENGINE
	CImGuiTestEngine::Init();
	RegisterRetroDebuggerTests(CImGuiTestEngine::GetEngine());
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
			if (sRunSuiteTest)
			{
				CTestSuite::RunFromCLI(sSuiteTestName);
			}
			else
			{
				CTestSuite::RunFromCLI(NULL);
			}
		}
	}
}

void MT_PostRenderEndFrame()
{
#ifdef ENABLE_IMGUI_TEST_ENGINE
	CImGuiTestEngine::PostSwap();

	if (sRunTests && !sTestsQueued)
	{
		sWarmupFrames++;
		if (sWarmupFrames >= 10)
		{
			CImGuiTestEngine::QueueAllTests();
			sTestsQueued = true;
		}
	}
	if (sExitAfterTests && sTestsQueued && CImGuiTestEngine::IsTestQueueEmpty())
	{
		int tested = 0, success = 0;
		CImGuiTestEngine::GetResultSummary(&tested, &success);
		LOGM("TEST RESULTS: %d/%d passed", success, tested);
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
#ifdef ENABLE_IMGUI_TEST_ENGINE
	CImGuiTestEngine::Shutdown();
#endif
}
