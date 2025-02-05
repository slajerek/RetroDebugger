#include "C64D_InitPlugins.h"
#include "CViewC64.h"

#include "C64DebuggerPluginDummy.h"
#include "C64DebuggerPluginTemplate.h"
#include "C64DebuggerPluginCrtMaker.h"
//#include "C64DebuggerPluginGoatTracker.h"

void C64D_InitPlugins()
{
//	C64DebuggerPluginDummy *plugin = new C64DebuggerPluginDummy();
//	viewC64->RegisterEmulatorPlugin(plugin);

	if (crtMakerConfigFilePath != NULL)
	{
		C64DebuggerPluginCrtMaker *plugin = new C64DebuggerPluginCrtMaker();
		viewC64->RegisterEmulatorPlugin(plugin);
	}
}

