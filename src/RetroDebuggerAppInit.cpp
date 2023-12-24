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
	
	RetroDebuggerEmbeddedAddData();
	
	CViewC64 *viewC64 = new CViewC64(0, 0, -1, SCREEN_WIDTH, SCREEN_HEIGHT);
	guiMain->SetView(viewC64);
	
	VID_SetFPS(5);
}

void MT_Render()
{
}

void MT_PostRenderEndFrame()
{
}
