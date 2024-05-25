#ifndef _C64DebuggerPluginGalaxy_H_
#define _C64DebuggerPluginGalaxy_H_

#include "C64DebuggerPluginCrtMaker.h"
#include "CDebuggerApi.h"
#include <list>

class CImageData;
class CViewC64CrtMaker;

class C64DebuggerPluginGalaxy : public C64DebuggerPluginCrtMaker
{
public:
	C64DebuggerPluginGalaxy();
	
	virtual void Init();
	virtual void ThreadRun(void *data);

	virtual void DoFrame();
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);
	
	char *assembleTextBuf;
};

extern C64DebuggerPluginGalaxy *pluginGalaxy;

void PLUGIN_GalaxyInit();
void PLUGIN_GalaxySetVisible(bool isVisible);


#endif
