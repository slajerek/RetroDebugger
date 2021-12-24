#ifndef _CDebuggerEmulatorPluginVice_H_
#define _CDebuggerEmulatorPluginVice_H_

#include "SYS_Defs.h"
#include "CDebuggerApiVice.h"
#include "CDebuggerEmulatorPlugin.h"

class CDebugInterfaceVice;
class CDebuggerApiVice;

class CDebuggerEmulatorPluginVice : public CDebuggerEmulatorPlugin
{
public:
	CDebuggerEmulatorPluginVice();
	virtual ~CDebuggerEmulatorPluginVice();
	
	CDebuggerApiVice *api;
	CDebugInterfaceVice *debugInterfaceVice;

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
