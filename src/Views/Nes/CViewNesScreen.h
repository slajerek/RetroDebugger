#ifndef _CViewNesScreen_h_
#define _CViewNesScreen_h_

#include "CViewEmulatorScreen.h"

class CViewNesScreen : public CViewEmulatorScreen
{
public:
	CViewNesScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewNesScreen();
	
	int GetJoystickAxis(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
};

#endif
