#include "CViewAtariScreen.h"

CViewAtariScreen::CViewAtariScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
									 CDebugInterface *debugInterface)
: CViewEmulatorScreen(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	
}

bool CViewAtariScreen::IsSkipKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// debugger keys, not used in c64
	if (keyCode == MTKEY_F9 || keyCode == MTKEY_F10 || keyCode == MTKEY_F11 || keyCode == MTKEY_F12 || keyCode == MTKEY_F13 || keyCode == MTKEY_F14 || keyCode == MTKEY_F15 || keyCode == MTKEY_F16)
		return true;
	
	return false;
}

CViewAtariScreen::~CViewAtariScreen()
{
	
}

