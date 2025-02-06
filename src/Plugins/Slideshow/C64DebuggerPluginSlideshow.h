#ifndef _C64DebuggerPluginSlideshow_H_
#define _C64DebuggerPluginSlideshow_H_

#include "C64DebuggerPluginCrtMaker.h"
#include "CDebuggerApi.h"
#include <list>

class CImageData;
class CViewC64CrtMaker;

class C64DebuggerPluginSlideshow : public C64DebuggerPluginCrtMaker
{
public:
	C64DebuggerPluginSlideshow();
	
	virtual void Init();
	virtual void ThreadRun(void *data);

	virtual void DoFrame();
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);
	
	char *assembleTextBuf;
};

extern C64DebuggerPluginSlideshow *pluginGalaxy;

void PLUGIN_GalaxyInit();
void PLUGIN_GalaxySetVisible(bool isVisible);


#endif
