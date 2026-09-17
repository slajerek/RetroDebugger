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
	virtual ~C64DebuggerPluginGalaxy();

	virtual void Init();
	virtual void ThreadRun(void *data);

	virtual void DoFrame();
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);
	
	char *assembleTextBuf;
	
	int BACK_SCREEN_ADDR;
	int BACK_COLOR_ADDR;
	int BACK_BITMAP_ADDR;
	
	bool forceRebuildImages;
	
	CByteBuffer *actionBuffer;
	
	char *hiresPngPath;
	char *hiresPrgPath;
	char *hiresPrgName;
	char *nufliPrgPath;
	char *nufliPrgName;

	
	void AddActions();
	
	void AddActionFinish();
	void AddActionLoadNextFile();
	void AddActionShowBitmap();
	void AddActionShowNufli();
	void AddActionHideNufli();
	void AddActionHideScreen();
	void AddActionDelay(int numDelay);
	void AddActionSongFadeout();
	
	void AddFileBitmap(int bitmapID);
	void AddFileNufli(int nufliID);
};

extern C64DebuggerPluginGalaxy *pluginGalaxy;

void PLUGIN_GalaxyInit();
void PLUGIN_GalaxySetVisible(bool isVisible);


#endif
