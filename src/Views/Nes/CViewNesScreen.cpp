#include "CViewNesScreen.h"
#include "CViewC64.h"
#include "CMainMenuBar.h"

CViewNesScreen::CViewNesScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
									 CDebugInterface *debugInterface)
: CViewEmulatorScreen(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	
}

int CViewNesScreen::GetJoystickAxis(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewNesScreen::GetJoystickAxis: keyCode=%d shift=%s alt=%s control=%s super=%s", keyCode, STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl), STRBOOL(isSuper));
	
	if (viewC64->mainMenuBar->kbsJoystickFireB->keyCode == keyCode
		// TODO: this is workaround for my private keyboard that has replaced r-cmd with r-alt using Karabineer, this is temporarily till we get a proper joystick/keyboard shortcuts editor
		|| keyCode == MTKEY_RSUPER)
	{
		return JOYPAD_FIRE_B;
	}
	if (viewC64->mainMenuBar->kbsJoystickStart->keyCode == keyCode)
	{
		return JOYPAD_START;
	}
	if (viewC64->mainMenuBar->kbsJoystickSelect->keyCode == keyCode)
	{
		return JOYPAD_SELECT;
	}
	
	return CViewEmulatorScreen::GetJoystickAxis(keyCode, isShift, isAlt, isControl, isSuper);
}

CViewNesScreen::~CViewNesScreen()
{
	
}

