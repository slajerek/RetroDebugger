#ifndef _CDebuggerEmulatorPluginNestopia_H_
#define _CDebuggerEmulatorPluginNestopia_H_

#include "SYS_Defs.h"
#include "CDebuggerApiVice.h"
#include "CDebuggerEmulatorPlugin.h"

class CDebuggerApiVice;
class CDebugInterfaceNes;

class CDebuggerEmulatorPluginNestopia : public CDebuggerEmulatorPlugin
{
public:
	CDebuggerEmulatorPluginNestopia();
	virtual ~CDebuggerEmulatorPluginNestopia();
	
	CDebuggerApiVice *api;
	CDebugInterfaceNes *debugInterfaceNes;

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
