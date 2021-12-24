#include "CDebuggerEmulatorPluginVice.h"
#include "DebuggerDefs.h"
#include "CDebuggerApiVice.h"

CDebuggerEmulatorPluginVice::CDebuggerEmulatorPluginVice()
: CDebuggerEmulatorPlugin(EMULATOR_TYPE_C64_VICE)
{
	this->api = (CDebuggerApiVice *)CDebuggerApi::GetDebuggerApi(EMULATOR_TYPE_C64_VICE);
	this->debugInterfaceVice = (CDebugInterfaceVice *)this->api->debugInterface;
}

CDebuggerEmulatorPluginVice::~CDebuggerEmulatorPluginVice()
{
}

void CDebuggerEmulatorPluginVice::Init()
{
}

void CDebuggerEmulatorPluginVice::DoFrame()
{
}

u32 CDebuggerEmulatorPluginVice::KeyDown(u32 keyCode)
{
	return keyCode;
}

u32 CDebuggerEmulatorPluginVice::KeyUp(u32 keyCode)
{
	return keyCode;
}

bool CDebuggerEmulatorPluginVice::ScreenMouseDown(float x, float y)
{
	return false;
}

bool CDebuggerEmulatorPluginVice::ScreenMouseUp(float x, float y)
{
	return false;
}

