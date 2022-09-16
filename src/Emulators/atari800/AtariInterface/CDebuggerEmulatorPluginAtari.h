#ifndef _CDebuggerEmulatorPluginAtari_H_
#define _CDebuggerEmulatorPluginAtari_H_

#include "SYS_Defs.h"
#include "CDebuggerApiAtari.h"
#include "CDebuggerEmulatorPlugin.h"

class CDebuggerApiAtari;
class CDebugInterfaceAtari;

class CDebuggerEmulatorPluginAtari : public CDebuggerEmulatorPlugin
{
public:
	CDebuggerEmulatorPluginAtari();
	virtual ~CDebuggerEmulatorPluginAtari();
	
	CDebuggerApiAtari *api;
	CDebugInterfaceAtari *debugInterfaceAtari;

	virtual void Init();
	virtual void DoFrame();
	
	// returns key
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);
	
	// returns is consumed
	virtual bool ScreenMouseDown(float x, float y);
	virtual bool ScreenMouseUp(float x, float y);
};

#endif
