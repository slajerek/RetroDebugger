#include "C64D_InitPlugins.h"
#include "CViewC64.h"

#include "C64DebuggerPluginDummy.h"
#include "C64DebuggerPluginTemplate.h"
#include "C64DebuggerPluginCrtMaker.h"
//#include "C64DebuggerPluginGoatTracker.h"
//#include "C64DebuggerPluginDNDK.h"
//#include "C64DebuggerPluginCommando.h"

void C64D_InitPlugins()
{
//	C64DebuggerPluginDummy *plugin = new C64DebuggerPluginDummy();
//	viewC64->RegisterEmulatorPlugin(plugin);

	if (crtMakerConfigFilePath != NULL)
	{
		C64DebuggerPluginCrtMaker *plugin = new C64DebuggerPluginCrtMaker();
		viewC64->RegisterEmulatorPlugin(plugin);
	}

//	C64DebuggerPluginTemplate *plugin = new C64DebuggerPluginTemplate(300, 30, 500, 400);
//	viewC64->RegisterEmulatorPlugin(plugin);

	// autostart plugin
//	C64DebuggerPluginGoatTracker *plugin = new C64DebuggerPluginGoatTracker();
//	viewC64->RegisterEmulatorPlugin(plugin);

//	C64DebuggerPluginDNDK *plugin = new C64DebuggerPluginDNDK(300, 30, 0, 500, 400);
//	viewC64->RegisterEmulatorPlugin(plugin);

//	C64DebuggerPluginCommando *plugin = new C64DebuggerPluginCommando(300, 30, 0, 500, 400);
//	viewC64->RegisterEmulatorPlugin(plugin);	
}

