#ifndef _CViewAtariScreen_h_
#define _CViewAtariScreen_h_

#include "CViewEmulatorScreen.h"

class CViewAtariScreen : public CViewEmulatorScreen
{
public:
	CViewAtariScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewAtariScreen();
	
	virtual bool IsSkipKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
};

#endif
