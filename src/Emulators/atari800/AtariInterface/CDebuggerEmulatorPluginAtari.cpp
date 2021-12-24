#include "CDebuggerEmulatorPluginAtari.h"
#include "DebuggerDefs.h"
#include "CDebuggerApiNestopia.h"

CDebuggerEmulatorPluginAtari::CDebuggerEmulatorPluginAtari()
: CDebuggerEmulatorPlugin(EMULATOR_TYPE_NESTOPIA)
{
	this->api = (CDebuggerApiVice *)CDebuggerApi::GetDebuggerApi(EMULATOR_TYPE_NESTOPIA);
	this->debugInterfaceNes = (CDebugInterfaceNes *)this->api->debugInterface;
}

CDebuggerEmulatorPluginAtari::~CDebuggerEmulatorPluginAtari()
{
}

void CDebuggerEmulatorPluginAtari::Init()
{
}

void CDebuggerEmulatorPluginAtari::DoFrame()
{
}

u32 CDebuggerEmulatorPluginAtari::KeyDown(u32 keyCode)
{
	return keyCode;
}

u32 CDebuggerEmulatorPluginAtari::KeyUp(u32 keyCode)
{
	return keyCode;
}

bool CDebuggerEmulatorPluginAtari::ScreenMouseDown(float x, float y)
{
	return false;
}

bool CDebuggerEmulatorPluginAtari::ScreenMouseUp(float x, float y)
{
	return false;
}

