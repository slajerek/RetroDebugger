#ifndef _C64DEBUGGER_PLUGIN_GOATTRACKER_H_
#define _C64DEBUGGER_PLUGIN_GOATTRACKER_H_

#include "CDebuggerEmulatorPluginVice.h"
#include "CDebuggerApi.h"
#include <list>

class CImageData;
class CViewC64GoatTracker;
class CAudioChannelGoatTracker;

class C64DebuggerPluginGoatTracker : public CDebuggerEmulatorPluginVice, CSlrThread
{
public:
	C64DebuggerPluginGoatTracker();
	
	virtual void Init();
	virtual void ThreadRun(void *data);

	virtual void DoFrame();
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);

	CViewC64GoatTracker *view;
	CAudioChannelGoatTracker *audioChannel;
	
	void SetupShadowRegsPlayer();
	
	// assemble
	u16 addrAssemble;
	void Assemble(char *buf);
	void PutDataByte(u8 v);
};

// singleton
extern C64DebuggerPluginGoatTracker *pluginGoatTracker;

void PLUGIN_GoatTrackerInit();

// TODO: add plugin menu
void PLUGIN_GoatTrackerSetVisible(bool isVisible);

#endif
