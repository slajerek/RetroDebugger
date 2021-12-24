#include "CDebuggerEmulatorPlugin.h"
#include "DebuggerDefs.h"
#include "CViewC64.h"

void CDebuggerEmulatorPlugin::RegisterPlugin(CDebuggerEmulatorPlugin *plugin)
{
	viewC64->RegisterEmulatorPlugin(plugin);
	plugin->Init();
}

CDebuggerEmulatorPlugin::CDebuggerEmulatorPlugin()
{
	SetEmulatorType(EMULATOR_TYPE_UNKNOWN);
}

CDebuggerEmulatorPlugin::CDebuggerEmulatorPlugin(u8 emulatorType)
{
	SetEmulatorType(emulatorType);
}

CDebuggerEmulatorPlugin::~CDebuggerEmulatorPlugin()
{
}

void CDebuggerEmulatorPlugin::SetEmulatorType(u8 emulatorType)
{
	this->emulatorType = emulatorType;
}

CDebugInterface *CDebuggerEmulatorPlugin::GetDebugInterface()
{
	return viewC64->GetDebugInterface(this->emulatorType);
}

void CDebuggerEmulatorPlugin::Init()
{
}

void CDebuggerEmulatorPlugin::DoFrame()
{
}

u32 CDebuggerEmulatorPlugin::KeyDown(u32 keyCode)
{
	return keyCode;
}

u32 CDebuggerEmulatorPlugin::KeyUp(u32 keyCode)
{
	return keyCode;
}

bool CDebuggerEmulatorPlugin::ScreenMouseDown(float x, float y)
{
	return false;
}

bool CDebuggerEmulatorPlugin::ScreenMouseUp(float x, float y)
{
	return false;
}

