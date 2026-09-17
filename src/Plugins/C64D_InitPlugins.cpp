//
// PUBLIC-TREE OVERRIDE of src/Plugins/C64D_InitPlugins.cpp.
//
// This is the exported copy, not the working one. The private tree registers
// demo-effect plugins that are deliberately not published; this copy carries
// the same structure with those registrations removed and MoonshineDragons
// (the one allowed addition) enabled. Keep it in step with the private file:
// any registration that lands in the private file must be re-judged here --
// prohibited never, shareable yes.
//
// Applied verbatim over the cut tree by the export script (from the private
// workspace's public-tree-overrides directory). Changing this file has no
// effect on the working build.
//
#include "C64D_InitPlugins.h"
#include "CViewC64.h"
#include "CPluginsManager.h"
#include "CDebuggerEmulatorPlugin.h"

#include "C64DebuggerPluginDummy.h"
#include "C64DebuggerPluginTemplate.h"
#include "C64DebuggerPluginCrtMaker.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "MoonshineDragons/C64DebuggerPluginMoonshineDragons.h"
#include "C64DebuggerPluginDNDK.h"
//#include "C64DebuggerPluginCommando.h"

static void GoatTrackerActivate()
{
	if (pluginGoatTracker == NULL)
	{
		PLUGIN_GoatTrackerInit();
	}
	else
	{
		viewC64->RegisterEmulatorPlugin(pluginGoatTracker);
	}
	PLUGIN_GoatTrackerSetVisible(true);
}

static void GoatTrackerDeactivate()
{
	if (pluginGoatTracker != NULL)
	{
		PLUGIN_GoatTrackerSetVisible(false);
		CDebuggerEmulatorPlugin::UnregisterPlugin(pluginGoatTracker);
	}
}

static void MoonshineDragonsActivate()
{
	if (pluginMoonshineDragons == NULL)
	{
		PLUGIN_MoonshineDragonsInit();
	}
	else
	{
		viewC64->RegisterEmulatorPlugin(pluginMoonshineDragons);
	}
	PLUGIN_MoonshineDragonsSetVisible(true);
}

static void MoonshineDragonsDeactivate()
{
	if (pluginMoonshineDragons != NULL)
	{
		PLUGIN_MoonshineDragonsSetVisible(false);
		CDebuggerEmulatorPlugin::UnregisterPlugin(pluginMoonshineDragons);
	}
}

static void DdnkActivate()
{
	if (pluginDDNK == NULL)
	{
		PLUGIN_DdnkInit();
	}
	else
	{
		viewC64->RegisterEmulatorPlugin(pluginDDNK);
	}
	PLUGIN_DdnkSetVisible(true);
}

static void DdnkDeactivate()
{
	if (pluginDDNK != NULL)
	{
		PLUGIN_DdnkSetVisible(false);
		CDebuggerEmulatorPlugin::UnregisterPlugin(pluginDDNK);
	}
}

void C64D_InitPlugins()
{
	gPluginsManager = new CPluginsManager();

	// GoatTracker shows a generic open/close toggle in the Plugins menu; its
	// own commands live in a dedicated top-level menu contributed via the
	// plugin menu API (CDebuggerEmulatorPlugin::GetMainMenuName).
	gPluginsManager->DeclarePlugin("GoatTracker", "Goat Tracker 2",   GoatTrackerActivate, GoatTrackerDeactivate, true);
	PLUGIN_GoatTrackerRestoreSettings();
	gPluginsManager->DeclarePlugin("DNDK",        "DNDK Trainer",     DdnkActivate,        DdnkDeactivate);
	gPluginsManager->DeclarePlugin("MoonshineDragons",   "Moonshine Dragons",  MoonshineDragonsActivate,   MoonshineDragonsDeactivate);

	if (crtMakerConfigFilePath != NULL)
	{
		C64DebuggerPluginCrtMaker *plugin = new C64DebuggerPluginCrtMaker();
		viewC64->RegisterEmulatorPlugin(plugin);
	}
}
