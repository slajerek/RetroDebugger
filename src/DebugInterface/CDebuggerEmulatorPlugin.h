#ifndef _CDebuggerEmulatorPlugin_H_
#define _CDebuggerEmulatorPlugin_H_

#include "SYS_Defs.h"
#include "CDebuggerApi.h"

// this plugin abstract class is intended for additional work done with emulator
// example: start vice emulator and do something with it, like own function for painting

class CDebugInterface;

class CDebuggerEmulatorPlugin
{
public:
	CDebuggerEmulatorPlugin();
	CDebuggerEmulatorPlugin(u8 emulatorType);
	virtual ~CDebuggerEmulatorPlugin();
	
	u8 emulatorType;
	virtual void SetEmulatorType(u8 emulatorType);
	virtual CDebugInterface *GetDebugInterface();
	
	virtual void Init();
	virtual void DoFrame();
	
	// returns key
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);
	
	// returns is consumed
	virtual bool ScreenMouseDown(float x, float y);
	virtual bool ScreenMouseUp(float x, float y);

	static void RegisterPlugin(CDebuggerEmulatorPlugin *plugin);
};

#endif
