#include "CDebuggerEmulatorPluginNestopia.h"
#include "DebuggerDefs.h"
#include "CDebuggerApiNestopia.h"

CDebuggerEmulatorPluginNestopia::CDebuggerEmulatorPluginNestopia()
: CDebuggerEmulatorPlugin(EMULATOR_TYPE_NESTOPIA)
{
	this->api = (CDebuggerApiVice *)CDebuggerApi::GetDebuggerApi(EMULATOR_TYPE_NESTOPIA);
	this->debugInterfaceNes = (CDebugInterfaceNes *)this->api->debugInterface;
}

CDebuggerEmulatorPluginNestopia::~CDebuggerEmulatorPluginNestopia()
{
}

void CDebuggerEmulatorPluginNestopia::Init()
{
}

void CDebuggerEmulatorPluginNestopia::DoFrame()
{
}

u32 CDebuggerEmulatorPluginNestopia::KeyDown(u32 keyCode)
{
	return keyCode;
}

u32 CDebuggerEmulatorPluginNestopia::KeyUp(u32 keyCode)
{
	return keyCode;
}

bool CDebuggerEmulatorPluginNestopia::ScreenMouseDown(float x, float y)
{
	return false;
}

bool CDebuggerEmulatorPluginNestopia::ScreenMouseUp(float x, float y)
{
	return false;
}

