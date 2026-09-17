#include "C64DebuggerPluginGoatTracker.h"
// workaround: SDL defines int8_t as different type
#define int8_t blah
extern "C" {
#include "goattrk2.h"
#include "gconsole.h"
#include "greloc.h"
#include "gplay.h"
#include "ginstr.h"
#include "ginstrops.h"
#include "gt2/asm/gt-membuf.h"
}
#include "CByteBuffer.h"
#include "C64Tools.h"
#include "SYS_CommandLine.h"
#include "GFX_Types.h"
#include "CViewC64GoatTracker.h"
#include "CAudioChannelGoatTracker.h"
#include "CViewRenoiseImport.h"
#include "CViewGT2KeyboardSetup.h"
#include "CPianoKeyboardGT2.h"
#include "CGT2RenoiseInput.h"
#include "SND_Main.h"
#include "CDebuggerApiVice.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "GT2ViewCommon.h"
#include "CConfigStorageHjson.h"
#include "CSlrString.h"
#include "CGT2FontAtlas.h"
#include "CGT2AudioMixer.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2OrderList.h"
#include "CViewGT2PatternList.h"
#include "CViewGT2Instrument.h"
#include "CViewGT2InstrumentList.h"
#include "CViewGT2Tables.h"
#include "CViewGT2SongInfo.h"
#include "CViewGT2SongSettings.h"
#include "CViewGT2Status.h"
#include "CViewGT2TitleBar.h"
#include "CViewGT2Mixer.h"
#include "CViewGT2Toolbar.h"
#include "CViewGT2Oscilloscope.h"
#include "CViewGT2StateSID.h"
#include "CGT2VoiceWaveforms.h"
#include "CDebugInterfaceMenuItemFolder.h"
#include "CGT2Favorites.h"
#include "CViewGT2InstrumentsBrowser.h"
#include "CViewGT2InstrumentTableRow.h"
#include "CViewGT2PatternRow.h"
#include "GT2RenderHelper.h"

#include "imgui.h"
#include "CPluginsManager.h"
#include "SYS_KeyCodes.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <map>
#include <list>

#define ASSEMBLE(fmt, ...) sprintf(buf, fmt, ## __VA_ARGS__); this->Assemble(buf);
#define A(fmt, ...) sprintf(buf, fmt, ## __VA_ARGS__); this->Assemble(buf);
#define PUT(v) this->PutDataByte(v);
#define PC addrAssemble

//#define  100
//#define  37

// Defined in RetroDebuggerAppInit.cpp: true for any run driven by a test
// switch. There is no shared header for it, so it is declared where used.
bool C64D_IsAutomatedRunCommandLine();

C64DebuggerPluginGoatTracker *pluginGoatTracker = NULL;

// c64d-only export format: export to SID, then wrap it as a runnable PRG via
// the PSID64 library. Distinct from GT2's FORMAT_SID/PRG/BIN (0/1/2).
static const int kGT2ExportFormatPsid64Prg = 3;

bool gt2RenoiseFollowTrack = true;
bool gt2RenoiseBulkPatternNumberChange = false;
bool gt2MetronomeEnabled = false;

void GT2_SetRenoiseUIScale(float scale)
{
	// Snap to the 12.5% grid, then clamp to [0.5, 3.5].
	scale = roundf(scale / 0.125f) * 0.125f;
	if (scale < 0.5f) scale = 0.5f;
	if (scale > 3.5f) scale = 3.5f;

	if (scale != gt2RenoiseUIScale)
	{
		gt2RenoiseUIScale = scale;
		// Non-layout UI changes do not auto-schedule a layout save; do it here
		// so both the keyboard and menu paths persist identically.
		if (guiMain != NULL)
			guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
	}
}

void GT2_StepRenoiseUIScale(int direction)
{
	GT2_SetRenoiseUIScale(gt2RenoiseUIScale + (float)direction * 0.125f);
}

void C64DebuggerPluginGoatTracker::GlobalLayoutWillDeserialize(CLayoutData *layout)
{
	// Reset zoom to 100% before every layout load. If the loaded layout has a
	// GT2 Patterns view, its layout parameter restores the workspace value.
	(void)layout;
	gt2RenoiseUIScale = 1.0f;
}

void PLUGIN_GoatTrackerInit()
{
	if (pluginGoatTracker == NULL)
	{
		pluginGoatTracker = new C64DebuggerPluginGoatTracker();
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginGoatTracker);
	}
}

bool PLUGIN_GoatTrackerIsVisible()
{
	return (pluginGoatTracker != NULL && pluginGoatTracker->view != NULL && pluginGoatTracker->view->visible);
}

bool C64DebuggerPluginGoatTracker::HandleOpenFileShortcut()
{
	if (!PLUGIN_GoatTrackerIsVisible()) return false;
	if (keypreset != KEY_RENOISE) return false;

	OpenLoadSongDialog();
	return true;
}

void C64DebuggerPluginGoatTracker::AddOpenFileExtensions(std::list<std::string> *extensions, bool isKeyboardShortcut)
{
	if (!PLUGIN_GoatTrackerIsVisible()) return;
	if (isKeyboardShortcut && keypreset != KEY_RENOISE) return;

	extensions->push_back("sng");
}

bool C64DebuggerPluginGoatTracker::OpenFile(CSlrString *path, bool isKeyboardShortcut)
{
	if (!PLUGIN_GoatTrackerIsVisible()) return false;
	if (isKeyboardShortcut && keypreset != KEY_RENOISE) return false;

	CSlrString *ext = path->GetFileExtensionComponentFromPath();
	ext->ConvertToLowerCase();
	bool isSong = ext->CompareWith("sng");
	delete ext;
	if (!isSong) return false;

	char *cPath = path->GetStdASCII();
	LoadSongFromFile(cPath);
	delete[] cPath;
	return true;
}

// ---------------------------------------------------------------------------
// One-time migration: copy a key from the global config to gt2Config when
// it exists in the global config but not yet in gt2Config, then remove it
// from the global config.
static void GT2_MigrateKey(const char *name)
{
	CConfigStorageHjson *g   = viewC64 ? viewC64->config : NULL;
	CConfigStorageHjson *gt2 = pluginGoatTracker ? pluginGoatTracker->gt2Config : NULL;
	if (g == NULL || gt2 == NULL) return;
	if (gt2->E_x_i_s_t_s(name))               // already migrated
	{
		// Drop any stale duplicate left in the global config so the key lives
		// only in gt2Config (the documented end state). Without this, a value
		// left in the global config from before migration — or re-added by a
		// save that fell back to the global config — would linger forever,
		// since this early return previously skipped the erase below.
		if (g->E_x_i_s_t_s(name)) g->hjsonRoot.erase(name);
		return;
	}
	if (!g->E_x_i_s_t_s(name)) return;        // nothing to migrate
	gt2->hjsonRoot[name] = g->hjsonRoot[name];
	g->hjsonRoot.erase(name);
}

static void GT2_MigrateAllSettings()
{
	static const char *scalarKeys[] = {
		"GT2ArpColumns", "GT2KeyboardLayout", "GT2RenoiseEditStep",
		"GT2RenoiseFollowTrack", "GT2RenoiseBulkPatternNumberChange",
		"GT2LoopCurrentPattern", "GT2MetronomeEnabled",
		"GT2ViewPatterns", "GT2ViewOrderList", "GT2ViewInstrument",
		"GT2ViewInstrumentList", "GT2ViewTables", "GT2ViewSongInfo",
		"GT2ViewStatus", "GT2ViewTitleBar", "GT2ViewMixer", "GT2ViewToolbar",
		"GT2ViewKeyboard", "GT2ViewOscilloscope",
		"GT2AutoloadSong", "GoatTrackerSongPath", "GoatTrackerSongFolder", NULL
	};
	for (int i = 0; scalarKeys[i]; i++) GT2_MigrateKey(scalarKeys[i]);
	char key[32];
	for (int i = 0; i < 15; i++) { sprintf(key, "GT2CustomKey1_%d", i); GT2_MigrateKey(key); }
	for (int i = 0; i < 17; i++) { sprintf(key, "GT2CustomKey2_%d", i); GT2_MigrateKey(key); }

	if (viewC64 && viewC64->config) viewC64->config->SaveConfig();
	if (pluginGoatTracker && pluginGoatTracker->gt2Config) pluginGoatTracker->gt2Config->SaveConfig();
}
// ---------------------------------------------------------------------------

void GT2_SaveSubViewVisibility()
{
	if (viewC64 == NULL) return;
	CConfigStorageHjson *cfg = (pluginGoatTracker != NULL && pluginGoatTracker->gt2Config != NULL)
		? pluginGoatTracker->gt2Config
		: viewC64->config;
	if (cfg == NULL) return;

	extern int numarpcolumns;
	int arpCols = numarpcolumns;
	cfg->SetInt("GT2ArpColumns", &arpCols);

	extern unsigned keypreset;
	int kp = (int)keypreset;
	cfg->SetInt("GT2KeyboardLayout", &kp);

	extern int gt2RenoiseEditStep;
	int editStep = gt2RenoiseEditStep;
	cfg->SetInt("GT2RenoiseEditStep", &editStep);
	cfg->SetBool("GT2RenoiseFollowTrack", &gt2RenoiseFollowTrack);
	cfg->SetBool("GT2RenoiseBulkPatternNumberChange", &gt2RenoiseBulkPatternNumberChange);
	bool loopCurrentPattern = gt2LoopCurrentPattern != 0;
	cfg->SetBool("GT2LoopCurrentPattern", &loopCurrentPattern);
	cfg->SetBool("GT2MetronomeEnabled", &gt2MetronomeEnabled);
	cfg->SetBool("GT2KeepPlayingOnStop", &gt2KeepPlayingOnStop);

	int patternDispMode = (int)patterndispmode;
	cfg->SetInt("GT2PatternDispMode", &patternDispMode);

	extern int gt2CommandValueMode;
	cfg->SetInt("GT2CommandValueMode", &gt2CommandValueMode);

	extern int gt2VisibleSustainColumn;
	cfg->SetInt("GT2VisibleSustainColumn", &gt2VisibleSustainColumn);

	extern int gt2PatternCursorCentered;
	cfg->SetInt("GT2PatternCursorCentered", &gt2PatternCursorCentered);

	if (pluginGoatTracker != NULL && pluginGoatTracker->viewInstrumentsBrowser != NULL)
		cfg->SetBool("GT2InstrumentsBrowserPreview",
					 &pluginGoatTracker->viewInstrumentsBrowser->previewOnClick);

	if (pluginGoatTracker != NULL)
		cfg->SetBool("GT2FixInstrumentGatetimerOnLoad",
					 &pluginGoatTracker->fixInstrumentGatetimerOnLoad);

	extern int gt2EchoSustainStep;
	extern int gt2EchoRowStep;
	extern int gt2EchoChannelMask;
	cfg->SetInt("GT2EchoSustainStep", &gt2EchoSustainStep);
	cfg->SetInt("GT2EchoRowStep",     &gt2EchoRowStep);
	cfg->SetInt("GT2EchoChannelMask", &gt2EchoChannelMask);

	extern float gt2OscStrokeThickness;
	cfg->SetFloat("GT2OscStrokeThickness", &gt2OscStrokeThickness);

	if (pluginGoatTracker == NULL) return;

	bool v;
	v = pluginGoatTracker->viewPatterns   && pluginGoatTracker->viewPatterns->visible;   cfg->SetBool("GT2ViewPatterns", &v);
	v = pluginGoatTracker->viewOrderList  && pluginGoatTracker->viewOrderList->visible;  cfg->SetBool("GT2ViewOrderList", &v);
	v = pluginGoatTracker->viewPatternList && pluginGoatTracker->viewPatternList->visible; cfg->SetBool("GT2ViewPatternList", &v);
	v = pluginGoatTracker->viewInstrument && pluginGoatTracker->viewInstrument->visible; cfg->SetBool("GT2ViewInstrument", &v);
	v = pluginGoatTracker->viewInstrumentList && pluginGoatTracker->viewInstrumentList->visible; cfg->SetBool("GT2ViewInstrumentList", &v);
	v = pluginGoatTracker->viewTables     && pluginGoatTracker->viewTables->visible;     cfg->SetBool("GT2ViewTables", &v);
	v = pluginGoatTracker->viewSongInfo   && pluginGoatTracker->viewSongInfo->visible;   cfg->SetBool("GT2ViewSongInfo", &v);
	v = pluginGoatTracker->viewSongSettings && pluginGoatTracker->viewSongSettings->visible; cfg->SetBool("GT2ViewSongSettings", &v);
	v = pluginGoatTracker->viewStatus     && pluginGoatTracker->viewStatus->visible;     cfg->SetBool("GT2ViewStatus", &v);
	v = pluginGoatTracker->viewTitleBar   && pluginGoatTracker->viewTitleBar->visible;   cfg->SetBool("GT2ViewTitleBar", &v);
	v = pluginGoatTracker->viewMixer      && pluginGoatTracker->viewMixer->visible;      cfg->SetBool("GT2ViewMixer", &v);
	v = pluginGoatTracker->viewToolbar    && pluginGoatTracker->viewToolbar->visible;    cfg->SetBool("GT2ViewToolbar", &v);
	v = pluginGoatTracker->viewKeyboard   && pluginGoatTracker->viewKeyboard->visible;   cfg->SetBool("GT2ViewKeyboard", &v);
	v = pluginGoatTracker->viewOscilloscope && pluginGoatTracker->viewOscilloscope->visible; cfg->SetBool("GT2ViewOscilloscope", &v);
	v = pluginGoatTracker->viewStateSID   && pluginGoatTracker->viewStateSID->visible;   cfg->SetBool("GT2ViewStateSID", &v);

	bool autoload = pluginGoatTracker->autoloadSongOnStart;
	cfg->SetBool("GT2AutoloadSong", &autoload);

	int exportFmt = pluginGoatTracker->exportFileFormat;
	cfg->SetInt("GT2ExportFileFormat", &exportFmt);

	// Save custom key tables
	for (int i = 0; i < 15; i++)
	{
		char key[32];
		sprintf(key, "GT2CustomKey1_%d", i);
		int v = customkeytbl1[i];
		cfg->SetInt(key, &v);
	}
	for (int i = 0; i < 17; i++)
	{
		char key[32];
		sprintf(key, "GT2CustomKey2_%d", i);
		int v = customkeytbl2[i];
		cfg->SetInt(key, &v);
	}

	cfg->SaveConfig();
}

void GT2_RestoreSubViewVisibility()
{
	if (viewC64 == NULL) return;
	CConfigStorageHjson *cfg = (pluginGoatTracker != NULL && pluginGoatTracker->gt2Config != NULL)
		? pluginGoatTracker->gt2Config
		: viewC64->config;
	if (cfg == NULL) return;

	extern int numarpcolumns;
	cfg->GetInt("GT2ArpColumns", &numarpcolumns, 0);
	if (numarpcolumns < 0) numarpcolumns = 0;
	if (numarpcolumns > MAX_ARP_COLS) numarpcolumns = MAX_ARP_COLS;

	extern unsigned keypreset;
	int kp = 0;
	cfg->GetInt("GT2KeyboardLayout", &kp, 0);
	if (kp < 0 || kp > 4) kp = 0;
	keypreset = (unsigned)kp;

	extern int gt2RenoiseEditStep;
	cfg->GetInt("GT2RenoiseEditStep", &gt2RenoiseEditStep, 1);
	if (gt2RenoiseEditStep < 0) gt2RenoiseEditStep = 0;
	cfg->GetBool("GT2RenoiseFollowTrack", &gt2RenoiseFollowTrack, true);
	cfg->GetBool("GT2RenoiseBulkPatternNumberChange", &gt2RenoiseBulkPatternNumberChange, false);

	int patternDispMode = 0;
	cfg->GetInt("GT2PatternDispMode", &patternDispMode, 0);
	patterndispmode = (unsigned)patternDispMode;

	extern int gt2CommandValueMode;
	cfg->GetInt("GT2CommandValueMode", &gt2CommandValueMode, 0);

	extern int gt2VisibleSustainColumn;
	cfg->GetInt("GT2VisibleSustainColumn", &gt2VisibleSustainColumn, 0);

	extern int gt2PatternCursorCentered;
	cfg->GetInt("GT2PatternCursorCentered", &gt2PatternCursorCentered, 0);

	if (pluginGoatTracker != NULL && pluginGoatTracker->viewInstrumentsBrowser != NULL)
		cfg->GetBool("GT2InstrumentsBrowserPreview",
					 &pluginGoatTracker->viewInstrumentsBrowser->previewOnClick, false);

	if (pluginGoatTracker != NULL)
		cfg->GetBool("GT2FixInstrumentGatetimerOnLoad",
					 &pluginGoatTracker->fixInstrumentGatetimerOnLoad, true);

	extern int gt2EchoSustainStep;
	extern int gt2EchoRowStep;
	extern int gt2EchoChannelMask;
	cfg->GetInt("GT2EchoSustainStep", &gt2EchoSustainStep, 2);
	cfg->GetInt("GT2EchoRowStep",     &gt2EchoRowStep,     2);
	cfg->GetInt("GT2EchoChannelMask", &gt2EchoChannelMask, 0);
	if (gt2EchoSustainStep < 1)  gt2EchoSustainStep = 1;
	if (gt2EchoSustainStep > 15) gt2EchoSustainStep = 15;
	if (gt2EchoRowStep < 1)  gt2EchoRowStep = 1;
	if (gt2EchoChannelMask < 0 || gt2EchoChannelMask > 7) gt2EchoChannelMask = 0;

	extern float gt2OscStrokeThickness;
	cfg->GetFloat("GT2OscStrokeThickness", &gt2OscStrokeThickness, 3.0f);
	if (gt2OscStrokeThickness < 0.5f) gt2OscStrokeThickness = 0.5f;
	if (gt2OscStrokeThickness > 20.0f) gt2OscStrokeThickness = 20.0f;

	bool loopCurrentPattern = false;
	cfg->GetBool("GT2LoopCurrentPattern", &loopCurrentPattern, false);
	gt2LoopCurrentPattern = loopCurrentPattern ? 1 : 0;
	cfg->GetBool("GT2MetronomeEnabled", &gt2MetronomeEnabled, false);
	cfg->GetBool("GT2KeepPlayingOnStop", &gt2KeepPlayingOnStop, false);

	if (pluginGoatTracker == NULL) return;

	if (pluginGoatTracker->viewPatterns)   pluginGoatTracker->viewPatterns->visible   = cfg->GetBool("GT2ViewPatterns", false);
	if (pluginGoatTracker->viewOrderList)  pluginGoatTracker->viewOrderList->visible  = cfg->GetBool("GT2ViewOrderList", false);
	if (pluginGoatTracker->viewPatternList) pluginGoatTracker->viewPatternList->visible = cfg->GetBool("GT2ViewPatternList", false);
	if (pluginGoatTracker->viewInstrument) pluginGoatTracker->viewInstrument->visible = cfg->GetBool("GT2ViewInstrument", false);
	if (pluginGoatTracker->viewInstrumentList) pluginGoatTracker->viewInstrumentList->visible = cfg->GetBool("GT2ViewInstrumentList", false);
	if (pluginGoatTracker->viewTables)     pluginGoatTracker->viewTables->visible     = cfg->GetBool("GT2ViewTables", false);
	if (pluginGoatTracker->viewSongInfo)   pluginGoatTracker->viewSongInfo->visible   = cfg->GetBool("GT2ViewSongInfo", false);
	if (pluginGoatTracker->viewSongSettings) pluginGoatTracker->viewSongSettings->visible = cfg->GetBool("GT2ViewSongSettings", false);
	if (pluginGoatTracker->viewStatus)     pluginGoatTracker->viewStatus->visible     = cfg->GetBool("GT2ViewStatus", false);
	if (pluginGoatTracker->viewTitleBar)   pluginGoatTracker->viewTitleBar->visible   = cfg->GetBool("GT2ViewTitleBar", false);
	if (pluginGoatTracker->viewMixer)      pluginGoatTracker->viewMixer->visible      = cfg->GetBool("GT2ViewMixer", false);
	if (pluginGoatTracker->viewToolbar)    pluginGoatTracker->viewToolbar->visible    = cfg->GetBool("GT2ViewToolbar", false);
	if (pluginGoatTracker->viewKeyboard)   pluginGoatTracker->viewKeyboard->visible   = cfg->GetBool("GT2ViewKeyboard", false);
	if (pluginGoatTracker->viewOscilloscope) pluginGoatTracker->viewOscilloscope->visible = cfg->GetBool("GT2ViewOscilloscope", false);
	if (pluginGoatTracker->viewStateSID)   pluginGoatTracker->viewStateSID->visible   = cfg->GetBool("GT2ViewStateSID", false);

	pluginGoatTracker->autoloadSongOnStart = cfg->GetBool("GT2AutoloadSong", false);

	cfg->GetInt("GT2ExportFileFormat", &pluginGoatTracker->exportFileFormat, FORMAT_SID);
	if (pluginGoatTracker->exportFileFormat < FORMAT_SID
		|| pluginGoatTracker->exportFileFormat > kGT2ExportFormatPsid64Prg)
		pluginGoatTracker->exportFileFormat = FORMAT_SID;

	// Restore custom key tables
	for (int i = 0; i < 15; i++)
	{
		char key[32];
		sprintf(key, "GT2CustomKey1_%d", i);
		int v = customkeytbl1[i]; // default = current value
		cfg->GetInt(key, &v, (int)customkeytbl1[i]);
		customkeytbl1[i] = (unsigned char)v;
	}
	for (int i = 0; i < 17; i++)
	{
		char key[32];
		sprintf(key, "GT2CustomKey2_%d", i);
		int v = customkeytbl2[i];
		cfg->GetInt(key, &v, (int)customkeytbl2[i]);
		customkeytbl2[i] = (unsigned char)v;
	}
}

void PLUGIN_GoatTrackerSaveSettings()
{
	GT2_SaveSubViewVisibility();
}

void PLUGIN_GoatTrackerRestoreSettings()
{
	GT2_RestoreSubViewVisibility();
}

void PLUGIN_GoatTrackerShutdown()
{
	if (pluginGoatTracker != NULL)
	{
		pluginGoatTracker->Shutdown();
	}
}

void PLUGIN_GoatTrackerSetVisible(bool isVisible)
{
	if (pluginGoatTracker != NULL)
	{
		pluginGoatTracker->view->SetVisible(isVisible);

		if (!isVisible)
		{
			// Save sub-view visibility before closing them
			GT2_SaveSubViewVisibility();

			// Close all GT2 ImGui sub-views
			if (pluginGoatTracker->viewPatterns)   pluginGoatTracker->viewPatterns->visible = false;
			if (pluginGoatTracker->viewOrderList)  pluginGoatTracker->viewOrderList->visible = false;
			if (pluginGoatTracker->viewPatternList) pluginGoatTracker->viewPatternList->visible = false;
			if (pluginGoatTracker->viewInstrument) pluginGoatTracker->viewInstrument->visible = false;
			if (pluginGoatTracker->viewInstrumentList) pluginGoatTracker->viewInstrumentList->visible = false;
			if (pluginGoatTracker->viewTables)     pluginGoatTracker->viewTables->visible = false;
			if (pluginGoatTracker->viewSongInfo)   pluginGoatTracker->viewSongInfo->visible = false;
			// Not in the "show all" block below on purpose: a settings panel
			// is opened when it is wanted, not every time GT2 comes up.
			if (pluginGoatTracker->viewSongSettings) pluginGoatTracker->viewSongSettings->visible = false;
			if (pluginGoatTracker->viewStatus)     pluginGoatTracker->viewStatus->visible = false;
			if (pluginGoatTracker->viewTitleBar)   pluginGoatTracker->viewTitleBar->visible = false;
			if (pluginGoatTracker->viewMixer)      pluginGoatTracker->viewMixer->visible = false;
			if (pluginGoatTracker->viewToolbar)    pluginGoatTracker->viewToolbar->visible = false;
			if (pluginGoatTracker->viewKeyboard)   pluginGoatTracker->viewKeyboard->visible = false;
			if (pluginGoatTracker->viewOscilloscope) pluginGoatTracker->viewOscilloscope->visible = false;
			if (pluginGoatTracker->viewStateSID)   pluginGoatTracker->viewStateSID->visible = false;
		}
		else
		{
			// Restore sub-view settings (arp columns, keyboard layout, key
			// tables, autoload flag). View visibility is overridden below.
			GT2_RestoreSubViewVisibility();

			// Show all GT2 ImGui sub-views when GoatTracker is opened.
			if (pluginGoatTracker->viewPatterns)   pluginGoatTracker->viewPatterns->visible   = true;
			if (pluginGoatTracker->viewOrderList)  pluginGoatTracker->viewOrderList->visible  = true;
			if (pluginGoatTracker->viewPatternList) pluginGoatTracker->viewPatternList->visible = true;
			if (pluginGoatTracker->viewInstrument) pluginGoatTracker->viewInstrument->visible = true;
			if (pluginGoatTracker->viewInstrumentList) pluginGoatTracker->viewInstrumentList->visible = true;
			if (pluginGoatTracker->viewTables)     pluginGoatTracker->viewTables->visible     = true;
			if (pluginGoatTracker->viewSongInfo)   pluginGoatTracker->viewSongInfo->visible   = true;
			if (pluginGoatTracker->viewStatus)     pluginGoatTracker->viewStatus->visible     = true;
			if (pluginGoatTracker->viewTitleBar)   pluginGoatTracker->viewTitleBar->visible   = true;
			if (pluginGoatTracker->viewMixer)      pluginGoatTracker->viewMixer->visible      = true;
			if (pluginGoatTracker->viewToolbar)    pluginGoatTracker->viewToolbar->visible    = true;
			if (pluginGoatTracker->viewKeyboard)   pluginGoatTracker->viewKeyboard->visible   = true;
			if (pluginGoatTracker->viewOscilloscope) pluginGoatTracker->viewOscilloscope->visible = true;

			// Autoload: just set the flag. Actual load happens in DoFrame when engine is ready.
			pluginGoatTracker->autoloadPending = pluginGoatTracker->autoloadSongOnStart;
		}
	}
}


extern "C" {
	// These three are called from the GT2 thread's own main loop (gtmain ->
	// getkey -> win_getspeed -> win_checkmessages), which keeps running while
	// the app tears the plugin down and which a test can outlive. They
	// dereferenced pluginGoatTracker->view->imageDataScreen with no checks at
	// all; a SIGSEGV on a small fault address was seen coming out of
	// win_getspeed on 2026-09-08 (see src/TODO.txt). Nothing here can
	// meaningfully continue without those objects, so bail instead.
	unsigned char *gtGetRgbaPixelsBuffer()
	{
		if (pluginGoatTracker == NULL || pluginGoatTracker->view == NULL
			|| pluginGoatTracker->view->imageDataScreen == NULL)
			return NULL;

		LOGD("pluginGoatTracker->imageDataScreen=%x pluginGoatTracker->imageDataScreen->resultData=%x",
			 pluginGoatTracker->view->imageDataScreen, pluginGoatTracker->view->imageDataScreen->resultData);
		return pluginGoatTracker->view->imageDataScreen->resultData;
	}

	unsigned int gtGetGfxPitch()
	{
		if (pluginGoatTracker == NULL || pluginGoatTracker->view == NULL
			|| pluginGoatTracker->view->imageDataScreen == NULL)
			return 0;
		return pluginGoatTracker->view->imageDataScreen->width * 4;
	}
	
	void gtForwardEvents()
	{
		// Also stop pumping once shutdown has been asked for: ForwardEvents()
		// reads guiMain and the view's event queue, and neither is ours to
		// touch any more at that point.
		if (pluginGoatTracker == NULL || pluginGoatTracker->view == NULL
			|| pluginGoatTracker->shutdownRequested)
			return;
		pluginGoatTracker->view->ForwardEvents();
	}

	// Where GT2's own goattrk2.cfg lives.
	//
	// gtmain() reads and writes that file directly with fopen(), and it used to
	// build the path itself: on macOS ~/Library/Preferences/org.c64.covertbitops.*,
	// on Linux ~/.goattrk/, on Windows next to the executable. Three problems
	// with that, all of them real:
	//
	//   1. The save path used the ORIGINAL GoatTracker 2 filenames, so running
	//      RetroDebugger overwrote the config of a real GT2 install.
	//   2. The load path had already been renamed to *-c64d, so the settings
	//      never round-tripped: every launch read whatever GT2 left behind and
	//      every exit clobbered GT2's file.
	//   3. None of it went through gPathToSettings, so it ignored
	//      MT_SETTINGS_DIR -- meaning a headless test run wrote into the
	//      developer's real home directory, which the isolation added in
	//      bcead24b exists to prevent.
	//
	// It now goes where the rest of GT2's own state goes: the gt2/ subfolder of
	// RetroDebugger's settings folder, next to gt2-settings.hjson and the
	// favourites. Same trailing-slash handling as CGT2Favorites::Save().
	void gt2GetConfigFilePath(char *outPath, int maxLen)
	{
		if (outPath == NULL || maxLen <= 0)
			return;
		outPath[0] = 0;

		std::string base = gPathToSettings ? gPathToSettings : "";
		if (!base.empty() && base.back() != '/' && base.back() != '\\')
			base += "/";

		std::string dir = base + "gt2";
		SYS_CreateFolder(dir.c_str());

		std::string path = dir + "/goattrk2.cfg";
		snprintf(outPath, maxLen, "%s", path.c_str());
	}

	void gt2BeginPatternUndoStep()
	{
		if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
			pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	}

	void gt2CommitPatternUndoStep()
	{
		if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
			pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
	}

	void gt2BeginTableUndoStep()
	{
		if (pluginGoatTracker && pluginGoatTracker->viewTables)
			pluginGoatTracker->viewTables->BeginTableUndoStep();
	}

	void gt2CommitTableUndoStep()
	{
		if (pluginGoatTracker && pluginGoatTracker->viewTables)
			pluginGoatTracker->viewTables->CommitTableUndoStep();
	}

	void gt2ClearPatternUndoHistory()
	{
		if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
			pluginGoatTracker->viewPatterns->ClearPatternUndoHistory();
		if (pluginGoatTracker && pluginGoatTracker->viewTables)
			pluginGoatTracker->viewTables->ClearTableUndoHistory();
	}

	void gt2ClearPatternUndoHistoryIfSongChanged(int cs, int cp, int ci, int ct, int cn)
	{
		if (cs || cp || ci || ct || cn)
			gt2ClearPatternUndoHistory();
	}
}

C64DebuggerPluginGoatTracker::C64DebuggerPluginGoatTracker()
{
	LOGD("C64DebuggerPluginGoatTracker");
	pluginGoatTracker = this;
	LOGD("pluginGoatTracker=%x", pluginGoatTracker);

	view = NULL;
	audioChannel = NULL;
	gt2Config = NULL;
	gt2Favorites = NULL;
	shutdownRequested = false;
	renoiseInput = NULL;
	viewToolbar = NULL;
	viewKeyboard = NULL;
	viewOscilloscope = NULL;
	viewStateSID = NULL;
	viewSongSettings = NULL;
	viewInstrumentsBrowser = NULL;
	viewInstrumentTableRow = NULL;
	viewPatternRow = NULL;
	viewInstrumentList = NULL;
	viewPatternList = NULL;

	showExportDialog = false;
	exportWaitingForSaveDialog = false;
	exportFileFormat = FORMAT_SID;
	exportPlayerAddress = 0x1000;
	exportZeropageAddress = 0xFB;
	exportStatusMessage[0] = 0;
	// Default player options: buffered SID writes ON, rest OFF
	for (int i = 0; i < 7; i++)
		exportPlayerOptions[i] = false;
	exportPlayerOptions[0] = true; // Buffered SID-writes
	fixInstrumentGatetimerOnLoad = true;
	autoloadSongOnStart = false;
	autoloadPending = false;
	instrumentDialogMode = 0;
	recentlyOpenedSongs = NULL;
	songFileExtensions.push_back(new CSlrString("sng"));
}

C64DebuggerPluginGoatTracker::~C64DebuggerPluginGoatTracker()
{
	for (std::list<CSlrString *>::iterator it = songFileExtensions.begin(); it != songFileExtensions.end(); it++)
	{
		delete *it;
	}
	songFileExtensions.clear();
	if (pluginGoatTracker == this)
		pluginGoatTracker = NULL;
}

void C64DebuggerPluginGoatTracker::Shutdown()
{
	shutdownRequested = true;
	if (guiMain != NULL)
		guiMain->RemoveGlobalLayoutCallback(this);
	exitprogram = 1;
	win_quitted = 1;
	gt2_engine_ready = 0;

	if (thread != NULL)
	{
		SDL_WaitThread(thread, NULL);
		thread = NULL;
	}

	if (audioChannel != NULL)
	{
		SND_RemoveChannel(audioChannel);
		delete audioChannel;
		audioChannel = NULL;
	}

	// Free per-voice oscilloscope buffers — gsid.cpp's push function
	// is safe against null pointers, so the order vs. SID teardown
	// doesn't matter here.
	GT2_VoiceWaveforms_Destroy();
}

void C64DebuggerPluginGoatTracker::Init()
{
	LOGD("C64DebuggerPluginGoatTracker::Init");

	// Dedicated GT2 config. isFromSettings=true resolves the path under
	// gPathToSettings. The gt2/ subfolder is created lazily by SaveConfig.
	gt2Config = new CConfigStorageHjson("gt2/gt2-settings.hjson", true);
	gt2Config->ReadConfig();
	GT2_MigrateAllSettings();

	// Recently opened songs list. Own settings key so it stays separate
	// from the File/Recents list kept by CViewC64.
	recentlyOpenedSongs = new CRecentlyOpenedFiles(new CSlrString("recents-gt2songs"), this);

	gt2Favorites = new CGT2Favorites();
	gt2Favorites->Load();

	//
	view = new CViewC64GoatTracker(0, 0, -1, MAX_COLUMNS*8, MAX_ROWS*16);
	gt2MainView = view;  // shared pointer for all GT2 ImGui views to forward events
	viewRenoiseImport = new CViewRenoiseImport();

	// font atlas — Load() is deferred until GT2 initscreen() has populated chardata
	fontAtlas = new CGT2FontAtlas();

	// GT2 exposes the three SID voices as mixer channels.
	audioMixer = new CGT2AudioMixer(MAX_CHN);

	// Per-voice waveform buffers — gsid.cpp pushes per-sample data after
	// each SID clock, this view reads from them.
	GT2_VoiceWaveforms_Create(1024);

	// create all 8 ImGui views
	float posZ = 0.0f;
	viewPatterns   = new CViewGT2Patterns  ("GT2 Patterns",    10,  40, posZ, 400, 500, fontAtlas);
	viewOrderList  = new CViewGT2OrderList ("GT2 Order List", 420,  40, posZ, 350, 100, fontAtlas);
	viewPatternList = new CViewGT2PatternList("GT2 Pattern List", 780, 40, posZ, 200, 400, fontAtlas);
	viewInstrument = new CViewGT2Instrument("GT2 Instrument", 420, 150, posZ, 350, 150, fontAtlas);
	viewInstrumentList = new CViewGT2InstrumentList("GT2 Instruments", 780, 150, posZ, 250, 400, fontAtlas);
	viewTables     = new CViewGT2Tables    ("GT2 Tables",     420, 310, posZ, 350, 250, fontAtlas);
	viewSongInfo   = new CViewGT2SongInfo  ("GT2 Song Info",  420, 570, posZ, 350,  60, fontAtlas);
	// Every song-level knob GT2 has, in one place. Wider than the text-mode
	// views because it is plain ImGui with labelled fields, not a 40x3 grid.
	viewSongSettings = new CViewGT2SongSettings("GT2 Song Settings",
												250, 120, posZ, 520, 620, this);
	viewStatus     = new CViewGT2Status    ("GT2 Status",      10, 550, posZ, 400,  40, fontAtlas);
	viewTitleBar   = new CViewGT2TitleBar  ("GT2 Title Bar",   10,  10, posZ, 760,  20, fontAtlas);
	viewMixer      = new CViewGT2Mixer     ("GT2 Mixer",       10, 600, posZ, 760, 200, audioMixer);
	viewToolbar    = new CViewGT2Toolbar   ("GT2 Toolbar",     10, 810, posZ, 180,  48, this);
	viewKeyboardSetup = new CViewGT2KeyboardSetup("GT2 Keyboard Setup", 200, 100, posZ, 350, 600);
	viewKeyboard   = new CPianoKeyboardGT2 ("GT2 Keyboard View", 200, 810, posZ, 400, 65, this);
	// Triggered SID oscilloscope — binds to the per-voice waveform
	// buffers GT2's audio loop fills directly (CGT2VoiceWaveforms).
	viewOscilloscope = new CViewGT2Oscilloscope("GT2 Oscilloscope",
												610, 810, posZ, 360, 150);
	// The same SID state view the C64 emulator uses, bound to GT2's own SID.
	viewStateSID   = new CViewGT2StateSID  ("GT2 SID",         610, 150, posZ, 250, 270);
	viewInstrumentsBrowser = new CViewGT2InstrumentsBrowser("GT2 Instruments Browser",
															1040, 150, posZ, 320, 420, fontAtlas);
	// Wide by default: the help block is a fixed-width reference table that
	// does not reflow, so a narrow window would only scroll it.
	viewInstrumentTableRow = new CViewGT2InstrumentTableRow("GT2 Instrument Table Row",
															1040, 590, posZ, 560, 340, fontAtlas);
	// Same reasoning about width: the command picker is a fixed 4-column grid
	// of chips and the argument editors are rows of nibble buttons.
	viewPatternRow = new CViewGT2PatternRow("GT2 Pattern Row",
											600, 220, posZ, 560, 420, fontAtlas);

	renoiseInput = new CGT2RenoiseInput(this);

	// Register views with guiMain for rendering and layout serialization
	// (debugInterface->AddView only adds to a list that's consumed at emulator start,
	// but the plugin inits AFTER that, so we must register directly)
	guiMain->LockMutex();
	guiMain->AddViewSkippingLayout(viewPatterns);
	guiMain->AddViewSkippingLayout(viewOrderList);
	guiMain->AddViewSkippingLayout(viewPatternList);
	guiMain->AddViewSkippingLayout(viewInstrument);
	guiMain->AddViewSkippingLayout(viewInstrumentList);
	guiMain->AddViewSkippingLayout(viewTables);
	guiMain->AddViewSkippingLayout(viewSongInfo);
	guiMain->AddViewSkippingLayout(viewSongSettings);
	guiMain->AddViewSkippingLayout(viewStatus);
	guiMain->AddViewSkippingLayout(viewTitleBar);
	guiMain->AddViewSkippingLayout(viewMixer);
	guiMain->AddViewSkippingLayout(viewToolbar);
	guiMain->AddViewSkippingLayout(viewKeyboardSetup);
	guiMain->AddViewSkippingLayout(viewKeyboard);
	guiMain->AddViewSkippingLayout(viewOscilloscope);
	guiMain->AddViewSkippingLayout(viewStateSID);
	guiMain->AddViewSkippingLayout(viewInstrumentsBrowser);
	guiMain->AddViewSkippingLayout(viewInstrumentTableRow);
	guiMain->AddViewSkippingLayout(viewPatternRow);
	// Also register with layout system so visibility persists across sessions
	guiMain->AddViewToLayout(viewPatterns);
	guiMain->AddViewToLayout(viewOrderList);
	guiMain->AddViewToLayout(viewPatternList);
	guiMain->AddViewToLayout(viewInstrument);
	guiMain->AddViewToLayout(viewInstrumentList);
	guiMain->AddViewToLayout(viewTables);
	guiMain->AddViewToLayout(viewSongInfo);
	guiMain->AddViewToLayout(viewSongSettings);
	guiMain->AddViewToLayout(viewStatus);
	guiMain->AddViewToLayout(viewTitleBar);
	guiMain->AddViewToLayout(viewMixer);
	guiMain->AddViewToLayout(viewToolbar);
	guiMain->AddViewToLayout(viewKeyboardSetup);
	guiMain->AddViewToLayout(viewKeyboard);
	guiMain->AddViewToLayout(viewOscilloscope);
	guiMain->AddViewToLayout(viewStateSID);
	guiMain->AddViewToLayout(viewInstrumentsBrowser);
	guiMain->AddViewToLayout(viewInstrumentTableRow);
	guiMain->AddViewToLayout(viewPatternRow);
	guiMain->AddGlobalLayoutCallback(this);
	guiMain->UnlockMutex();

	// Views start hidden — accessible via Plugins → Goat Tracker menu
	viewPatterns->visible   = false;
	viewOrderList->visible  = false;
	viewPatternList->visible = false;
	viewInstrument->visible = false;
	viewInstrumentList->visible = false;
	viewTables->visible     = false;
	viewSongInfo->visible   = false;
	viewSongSettings->visible = false;
	viewStatus->visible     = false;
	viewTitleBar->visible   = false;
	viewMixer->visible      = false;
	viewToolbar->visible    = false;
	viewKeyboardSetup->visible = false;
	viewKeyboard->visible   = false;
	viewOscilloscope->visible = false;
	viewStateSID->visible = false;
	viewInstrumentsBrowser->visible = false;
	viewInstrumentTableRow->visible = false;
	viewPatternRow->visible = false;

	//
	api->StartThread(this);
}

extern "C" {
	int gtmain(int argc, const char **argv);
}

void C64DebuggerPluginGoatTracker::ThreadRun(void *data)
{
	LOGD("TODO: GT commandline");
	
	SYS_Sleep(50);
	if (shutdownRequested) return;
	api->AddView(view);
	if (shutdownRequested) return;

	audioChannel = new CAudioChannelGoatTracker(this);
	audioChannel->Start();
	SND_AddChannel(audioChannel);
	if (shutdownRequested) return;

//	gtmain(SYS_GetArgc(), SYS_GetArgv());
	gtmain(0, SYS_GetArgv());
}

static int doFrameLogCounter = 0;

void C64DebuggerPluginGoatTracker::DoFrame()
{
	if (doFrameLogCounter < 5)
	{
		LOGD("GT2 DoFrame called: autoloadPending=%d gt2_engine_ready=%d", autoloadPending, gt2_engine_ready);
		doFrameLogCounter++;
	}

	// Snapshot the producer side of the rolling per-voice waveform
	// buffers and recompute trigger positions. Cheap; no-op until
	// gsid.cpp has actually pushed samples.
	GT2_VoiceWaveforms_UpdatePerFrame();

	// The player refuses to start a song whose instrument has a gatetimer
	// longer than the tick interval, and stock GT2 stops without a word: play
	// parks on row 0, silent, and it reads as a hang. Say what happened, once
	// per occurrence. The audio thread only sets the values; this is the main
	// thread.
	if (gt2GatetimerStopPending)
	{
		int gatetimer = gt2GatetimerStopGatetimer;
		int tick = gt2GatetimerStopTick;
		int instr = gt2GatetimerStopInstr;
		gt2GatetimerStopPending = 0;

		const char *instrName = (instr >= 0 && instr < MAX_INSTR) ? ginstr[instr].name : "";
		LOGError("GT2: song stopped -- instrument %02X '%s' gatetimer=%d exceeds tick=%d",
				 instr, instrName, gatetimer, tick);
		if (viewC64 != NULL)
		{
			viewC64->ShowMessage("GoatTracker: song stopped. Instrument %02X gatetimer %d "
								 "is longer than the tick (%d). Lower the instrument's "
								 "gatetimer or raise the song tempo.",
								 instr, gatetimer, tick);
		}
	}

	// Autoload song: wait for GT2 engine to be ready, then load
	if (autoloadPending && gt2_engine_ready)
	{
		autoloadPending = false;
		char lastPath[512];
		lastPath[0] = 0;
		if (gt2Config) gt2Config->GetString("GoatTrackerSongPath", lastPath, sizeof(lastPath), "");
		LOGD("GT2 autoload: engine ready, loading '%s'", lastPath);
		if (strlen(lastPath) > 0)
		{
			LoadSongFromFile(lastPath);
		}
	}
}

void C64DebuggerPluginGoatTracker::SetupShadowRegsPlayer()
{
	// setup routine that will copy regs from shadow memory location to SID
	char *buf = SYS_GetCharBuf();

	api->DetachEverything();
	api->Sleep(200);
	
	// prepare RAM
	api->ClearRam(0x0800, 0x10000, 0x00);
	
	// TODO: player copy to d400
	PC = 0x0F00;
	A("SEI");
//	A("INC D020");
	A("JMP %04x", PC);
	
	
//	// load sid
//	u16 fromAddr, toAddr, sidInitAddr, sidPlayAddr;
//	api->LoadSID("music.sid", &fromAddr, &toAddr, &sidInitAddr, &sidPlayAddr);
//
//	//
//	api->CreateNewPicture(C64_PICTURE_MODE_BITMAP_MULTI, 0x00);
//
//	api->Sleep(100);
	
//	imageDataRef = new CImageData("reference.png");
//	api->LoadReferenceImage(imageDataRef);
//	api->SetReferenceImageLayerVisible(true);
//	api->ClearReferenceImage();
//
//	api->ConvertImageToScreen(imageDataRef);
	
	api->ClearScreen();

	api->SetReferenceImageLayerVisible(true);

	api->SetupVicEditorForScreenOnly();

	api->Sleep(100);

	SYS_ReleaseCharBuf(buf);
}

u32 C64DebuggerPluginGoatTracker::KeyDown(u32 keyCode)
{
	if (keyCode == MTKEY_ARROW_UP)
	{
	}
	
	if (keyCode == MTKEY_ARROW_DOWN)
	{
	}
	
	if (keyCode == MTKEY_ARROW_LEFT)
	{
	}
	if (keyCode == MTKEY_ARROW_RIGHT)
	{
	}
	
	if (keyCode == MTKEY_SPACEBAR)
	{
//		api->SaveExomizerPRG(0x1000, 0x3000, 0x0F00, "out.prg");
	}
	
	return keyCode;
}

u32 C64DebuggerPluginGoatTracker::KeyUp(u32 keyCode)
{
	return keyCode;
}

///
void C64DebuggerPluginGoatTracker::Assemble(char *buf)
{
	//	LOGD("Assemble: %04x %s", addrAssemble, buf);
	addrAssemble += api->Assemble(addrAssemble, buf);
}

void C64DebuggerPluginGoatTracker::PutDataByte(u8 v)
{
	//	LOGD("PutDataByte: %04x %02x", addrAssemble, v);
	api->SetByteToRam(addrAssemble, v);
	addrAssemble++;
}

void C64DebuggerPluginGoatTracker::LoadSongFromFile(const char *filePath)
{
	strncpy(songfilename, filePath, MAX_FILENAME - 1);
	songfilename[MAX_FILENAME - 1] = 0;

	loadsong();
	if (viewPatterns)
		viewPatterns->ClearPatternUndoHistory();
	if (viewTables)
		viewTables->ClearTableUndoHistory();

	// Save path to config
	if (gt2Config) gt2Config->SetString("GoatTrackerSongPath", &filePath);

	// Save folder
	CSlrString *slrPath = new CSlrString(filePath);
	CSlrString *folder = slrPath->GetFilePathWithoutFileNameComponentFromPath();
	char *cFolder = folder->GetStdASCII();
	if (gt2Config) gt2Config->SetString("GoatTrackerSongFolder", (const char **)&cFolder);
	delete[] cFolder;
	delete folder;

	if (recentlyOpenedSongs) recentlyOpenedSongs->Add(slrPath);
	delete slrPath;
}

void C64DebuggerPluginGoatTracker::OpenLoadSongDialog()
{
	const char *lastFolder = NULL;
	if (gt2Config) gt2Config->GetString("GoatTrackerSongFolder", &lastFolder, "");
	CSlrString *defaultFolder = (lastFolder && lastFolder[0]) ? new CSlrString(lastFolder) : NULL;

	CSlrString *windowTitle = new CSlrString("Open GoatTracker Song");
	viewC64->ShowDialogOpenFile(this, &songFileExtensions, defaultFolder, windowTitle);

	delete windowTitle;
	if (defaultFolder) delete defaultFolder;
}

void C64DebuggerPluginGoatTracker::SystemDialogFileOpenSelected(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	if (instrumentDialogMode == 1)
	{
		instrumentDialogMode = 0;
		LoadInstrumentFromFile(cPath, einum, false);
	}
	else
	{
		LoadSongFromFile(cPath);
	}
	delete[] cPath;
}

void C64DebuggerPluginGoatTracker::SystemDialogFileOpenCancelled()
{
	instrumentDialogMode = 0;
}

void C64DebuggerPluginGoatTracker::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	char *cPath = filePath->GetStdASCII();
	LoadSongFromFile(cPath);
	delete[] cPath;
}

void C64DebuggerPluginGoatTracker::SaveSongToFile(const char *filePath)
{
	strncpy(songfilename, filePath, MAX_FILENAME - 1);
	songfilename[MAX_FILENAME - 1] = 0;

	savesong();

	// Save path to config
	if (gt2Config) gt2Config->SetString("GoatTrackerSongPath", &filePath);

	// Save folder
	CSlrString *slrPath = new CSlrString(filePath);
	CSlrString *folder = slrPath->GetFilePathWithoutFileNameComponentFromPath();
	char *cFolder = folder->GetStdASCII();
	if (gt2Config) gt2Config->SetString("GoatTrackerSongFolder", (const char **)&cFolder);
	delete[] cFolder;
	delete folder;

	if (recentlyOpenedSongs) recentlyOpenedSongs->Add(slrPath);
	delete slrPath;
}

void C64DebuggerPluginGoatTracker::OpenSaveSongDialog()
{
	std::list<CSlrString *> extensions;
	extensions.push_back(new CSlrString("sng"));

	const char *lastFolder = NULL;
	if (gt2Config) gt2Config->GetString("GoatTrackerSongFolder", &lastFolder, "");
	CSlrString *defaultFolder = (lastFolder && lastFolder[0]) ? new CSlrString(lastFolder) : NULL;

	// Use loaded song filename as default, extract folder from it
	CSlrString *defaultFileName = NULL;
	if (strlen(loadedsongfilename) > 0)
	{
		CSlrString *fullPath = new CSlrString(loadedsongfilename);
		defaultFileName = fullPath->GetFileNameComponentFromPath();
		CSlrString *songFolder = fullPath->GetFilePathWithoutFileNameComponentFromPath();
		if (songFolder && songFolder->GetLength() > 0)
		{
			if (defaultFolder) delete defaultFolder;
			defaultFolder = songFolder;
		}
		else
		{
			if (songFolder) delete songFolder;
		}
		delete fullPath;
	}
	else
	{
		defaultFileName = new CSlrString("untitled.sng");
	}

	CSlrString *windowTitle = new CSlrString("Save GoatTracker Song");
	viewC64->ShowDialogSaveFile(this, &extensions, defaultFileName, defaultFolder, windowTitle);

	delete windowTitle;
	if (defaultFileName) delete defaultFileName;
	if (defaultFolder) delete defaultFolder;
	for (auto *ext : extensions) delete ext;
}

void C64DebuggerPluginGoatTracker::SystemDialogFileSaveSelected(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	if (instrumentDialogMode == 2)
	{
		instrumentDialogMode = 0;
		strncpy(instrfilename, cPath, MAX_FILENAME - 1);
		instrfilename[MAX_FILENAME - 1] = 0;
		saveinstrument();
		RememberInstrumentFolderOf(cPath);
	}
	else if (exportWaitingForSaveDialog)
	{
		exportWaitingForSaveDialog = false;
		DoExport(cPath);
	}
	else
	{
		SaveSongToFile(cPath);
	}
	delete[] cPath;
}

void C64DebuggerPluginGoatTracker::SystemDialogFileSaveCancelled()
{
	exportWaitingForSaveDialog = false;
	instrumentDialogMode = 0;
}

std::string C64DebuggerPluginGoatTracker::GetInstrumentFolder()
{
	const char *folder = NULL;
	if (gt2Config) gt2Config->GetString("GoatTrackerInstrumentFolder", &folder, "");
	if (folder && folder[0]) return std::string(folder);

	// Not set yet: fall back to the song folder, then to Documents. Both are
	// somewhere the user already keeps GT2 material.
	if (gt2Config) gt2Config->GetString("GoatTrackerSongFolder", &folder, "");
	if (folder && folder[0]) return std::string(folder);

	return gCPathToDocuments ? std::string(gCPathToDocuments) : std::string();
}

void C64DebuggerPluginGoatTracker::RememberInstrumentFolderOf(const char *filePath)
{
	if (gt2Config == NULL || filePath == NULL || filePath[0] == 0) return;

	CSlrString *slrPath = new CSlrString(filePath);
	CSlrString *folder = slrPath->GetFilePathWithoutFileNameComponentFromPath();
	char *cFolder = folder->GetStdASCII();
	if (cFolder && cFolder[0])
		gt2Config->SetString("GoatTrackerInstrumentFolder", (const char **)&cFolder);
	delete[] cFolder;
	delete folder;
	delete slrPath;
}

bool C64DebuggerPluginGoatTracker::LoadInstrumentFromFile(const char *filePath, int instrumentNum, bool preview)
{
	if (filePath == NULL || filePath[0] == 0) return false;

	// Instrument 0 is GT2's "no instrument" slot and the last one is the
	// reserved tempo-override slot, so neither is a valid target.
	if (instrumentNum < 1 || instrumentNum > GT2_LAST_INSTR) return false;

	FILE *probe = SYS_OpenFile(filePath, "rb");
	if (probe == NULL) return false;
	fclose(probe);

	// Select first, so the instrument and table views follow the load and so
	// loadinstrument() -- which always writes to einum -- targets this slot.
	einum = instrumentNum;
	showinstrtable();

	strncpy(instrfilename, filePath, MAX_FILENAME - 1);
	instrfilename[MAX_FILENAME - 1] = 0;

	// Audition without cutting the song off: the point of clicking through
	// instruments is to hear the song carry on with each one. Native GT2 stops
	// the song here; the flag is ours and defaults to 0, so text mode keeps its
	// own behaviour. It also has to be off again straight away, including on
	// the paths that return early.
	// One undo step around the load AND the gatetimer clamp below, so undo
	// takes back both together and redo puts back what was actually applied.
	// loadinstrument() opens a nested step of its own; only this outer commit
	// records an entry (CViewGT2Tables' depth counter).
	if (viewTables) viewTables->BeginTableUndoStep();

	gt2KeepPlayingOnInstrumentLoad = 1;
	loadinstrument();
	gt2KeepPlayingOnInstrumentLoad = 0;

	// A .ins holds only the instrument's own slice of each table, plus the
	// absolute pointer it had in the song it was saved from. gsong.c rebases
	// the slice's jump targets on load -- target - optr + start + 1 -- and if
	// the target sat OUTSIDE the slice (GT2 lets instruments share table rows,
	// so a jump into a neighbour's rows is legal in a song) the subtraction
	// goes negative and wraps in the unsigned char into a random high row.
	// The rows it names were never in the file, so the jump cannot be
	// recovered; what it can do is stop the packer from walking off the end of
	// the table ("TABLE EXECUTION OVERFLOWS"). End the table instead.
	// The repairs are collected rather than announced one at a time: an
	// instrument can carry a bad jump in more than one table, and three
	// stacked notifications for one click read as three separate faults.
	char repairReport[1024];
	repairReport[0] = 0;
	int repairedJumps = 0;

	for (int t = 0; t < MAX_TABLES; t++)
	{
		if (t == STBL) continue;                 // speedtable rows are atomic
		int ptr = ginstr[einum].ptr[t];
		if (ptr <= 0) continue;

		int start = ptr - 1;
		int len = gettablepartlen(t, start);
		for (int row = start; row < start + len && row < MAX_TABLELEN; row++)
		{
			if (ltable[t][row] != 0xff) continue;
			int target = rtable[t][row];
			if (target == 0) continue;           // 0 is "stop", always fine
			if (target >= start + 1 && target <= start + len) continue;

			const char *tableName = (t == WTBL) ? "wave" : (t == PTBL ? "pulse" : "filter");

			rtable[t][row] = 0;
			repairedJumps++;
			LOGError("GT2: instrument %02X %s table row %02X jumped to row %02X, "
					 "outside the instrument's own rows %02X-%02X -- ended the table instead",
					 einum, tableName, row + 1, target, start + 1, start + len);

			char line[192];
			snprintf(line, sizeof(line),
					 "  %s table, row $%02X:  jump to row $%02X  (this instrument owns $%02X-$%02X)\n",
					 tableName, row + 1, target, start + 1, start + len);
			strncat(repairReport, line, sizeof(repairReport) - strlen(repairReport) - 1);
		}
	}

	if (repairedJumps > 0)
	{
		// A dialog, not a passing notification: the loaded instrument no longer
		// matches the file on disk, and that is worth stopping for. Without the
		// repair the song simply refuses to export ("TABLE EXECUTION
		// OVERFLOWS"), so silently fixing it would trade one puzzle for another.
		char message[1600];
		snprintf(message, sizeof(message),
				 "Instrument %02X was loaded with %d table jump%s repaired.\n"
				 "\n"
				 "%s"
				 "\n"
				 "A .ins file stores only the instrument's own table rows plus the\n"
				 "pointer it had in the song it was saved from. The jump%s above\n"
				 "left that range, so the rows %s named were never in the file and\n"
				 "cannot be recovered.\n"
				 "\n"
				 "The table%s now end%s there instead. Left alone, the packer walks\n"
				 "off the end of the table and the song refuses to export with\n"
				 "\"TABLE EXECUTION OVERFLOWS\".\n"
				 "\n"
				 "Undo (%s+Z) takes the whole load back if you would rather keep\n"
				 "the instrument that was there before.",
				 einum, repairedJumps, repairedJumps == 1 ? "" : "s",
				 repairReport,
				 repairedJumps == 1 ? "" : "s",
				 repairedJumps == 1 ? "it" : "they",
				 repairedJumps == 1 ? "" : "s",
				 repairedJumps == 1 ? "s" : "",
				 GT2_CmdKey());

		// Not during an automated run: a dialog nobody can dismiss would sit
		// on top of every later UI test in the same process, and the fixtures
		// deliberately include a file that triggers this.
		if (guiMain != NULL && !C64D_IsAutomatedRunCommandLine())
			guiMain->ShowMessageBox("GoatTracker: instrument table repaired", message);
	}

	// A gatetimer longer than the song's tick makes playroutine() refuse to
	// play the song at all -- see the guard in gplay.c and #7a of the
	// instruments-browser notes. Files exported by other
	// tools do carry such values. Clamp it to what the guard will accept,
	// keeping the flag bits (0x80 no-hard-restart, 0x40 legato) intact.
	if (fixInstrumentGatetimerOnLoad)
	{
		int maxSafe = gt2MaxSafeGatetimer();
		int flags = ginstr[einum].gatetimer & 0xc0;
		int timer = ginstr[einum].gatetimer & 0x3f;
		if (maxSafe > 0 && timer > maxSafe)
		{
			ginstr[einum].gatetimer = (unsigned char)(flags | (maxSafe & 0x3f));
			LOGM("GT2: instrument %02X gatetimer %d -> %d (song tick allows at most %d)",
				 einum, timer, maxSafe, maxSafe);
			if (viewC64 != NULL)
			{
				viewC64->ShowMessage("GoatTracker: instrument %02X gatetimer %d was too high "
									 "for this song and was lowered to %d, or the song would "
									 "not play.", einum, timer, maxSafe);
			}
		}
	}

	if (viewTables) viewTables->CommitTableUndoStep();

	RememberInstrumentFolderOf(filePath);

	if (preview)
	{
		// A virtual re-trigger: the note the user last auditioned, on the
		// channel they last auditioned it on. Nothing needs to be under the
		// pattern cursor -- and if nothing has ever been played this session,
		// C-5 (notename[] starts at C-1 == FIRSTNOTE, so C-5 is +4 octaves).
		int note = gt2LastPreviewNote;
		if (note < FIRSTNOTE || note > LASTNOTE)
			note = FIRSTNOTE + 4 * 12;

		int channel = gt2LastPreviewChannel;
		if (channel < 0 || channel >= MAX_CHN)
			channel = epchn;

		playtestnote(note, einum, channel);
	}
	return true;
}

void C64DebuggerPluginGoatTracker::OpenLoadInstrumentDialog()
{
	std::list<CSlrString *> extensions;
	extensions.push_back(new CSlrString("ins"));

	std::string lastFolder = GetInstrumentFolder();
	CSlrString *defaultFolder = lastFolder.empty() ? NULL : new CSlrString(lastFolder.c_str());

	CSlrString *windowTitle = new CSlrString("Load GoatTracker Instrument");
	instrumentDialogMode = 1;
	viewC64->ShowDialogOpenFile(this, &extensions, defaultFolder, windowTitle);

	delete windowTitle;
	if (defaultFolder) delete defaultFolder;
	for (auto *ext : extensions) delete ext;
}

void C64DebuggerPluginGoatTracker::OpenSaveInstrumentDialog()
{
	std::list<CSlrString *> extensions;
	extensions.push_back(new CSlrString("ins"));

	std::string lastFolder = GetInstrumentFolder();
	CSlrString *defaultFolder = lastFolder.empty() ? NULL : new CSlrString(lastFolder.c_str());

	// Default filename from current instrument name
	CSlrString *defaultFileName = NULL;
	if (ginstr[einum].name[0])
	{
		char fname[MAX_INSTRNAMELEN + 8];
		snprintf(fname, sizeof(fname), "%s.ins", ginstr[einum].name);
		defaultFileName = new CSlrString(fname);
	}

	CSlrString *windowTitle = new CSlrString("Save GoatTracker Instrument");
	instrumentDialogMode = 2;
	viewC64->ShowDialogSaveFile(this, &extensions, defaultFileName, defaultFolder, windowTitle);

	delete windowTitle;
	if (defaultFileName) delete defaultFileName;
	if (defaultFolder) delete defaultFolder;
	for (auto *ext : extensions) delete ext;
}

///
void C64DebuggerPluginGoatTracker::OpenExportDialog()
{
	// Sync current GT2 globals into dialog state. The file format is a
	// persisted user preference (gt2Config), not a per-song value, so it is
	// intentionally not synced from GT2's fileformat global here.
	exportPlayerAddress = playeradr;
	exportZeropageAddress = zeropageadr;
	for (int i = 0; i < 7; i++)
		exportPlayerOptions[i] = (playerversion & (PLAYER_BUFFERED << i)) != 0;
	exportStatusMessage[0] = 0;
	showExportDialog = true;
}

// Platform-aware shortcut text. Built via the engine's SYS_KeyCodeToString
// so the command modifier renders as "Cmd" on macOS and "Ctrl" on
// Linux/Windows — never hardcoded.
static std::string GT2_ShortcutText(u32 keyCode, bool isShift, bool isAlt, bool isCommand)
{
	CSlrString *s = SYS_KeyCodeToString(keyCode, isShift, isAlt, isCommand, false);
	std::string out = s->GetStdStringUTF8();
	delete s;
	return out;
}

// The command-modifier word — "Cmd" on macOS, "Ctrl" elsewhere — taken from
// the engine formatter (the prefix of a known command shortcut), not hardcoded.
// Non-static: GT2 views outside this file label their shortcuts with it too.
const char *GT2_CmdKey()
{
	static std::string cmdName;
	if (cmdName.empty())
	{
		std::string s = GT2_ShortcutText((u32)'a', false, false, true);  // e.g. "Cmd+A"
		size_t plus = s.find('+');
		cmdName = (plus != std::string::npos) ? s.substr(0, plus) : "Ctrl";
	}
	return cmdName.c_str();
}

// Generic plugin top-level menu. Shown by CMainMenuBar when this returns a
// non-NULL name — i.e. only while the GoatTracker plugin is active.
const char *C64DebuggerPluginGoatTracker::GetMainMenuName()
{
	if (gPluginsManager != NULL && gPluginsManager->IsActive("GoatTracker"))
		return "GoatTracker";
	return NULL;
}

void C64DebuggerPluginGoatTracker::RenderMainMenuImGui()
{
	extern int numarpcolumns;
	extern unsigned keypreset;
	extern int gt2RenoiseEditStep;

	bool renoise = (keypreset == KEY_RENOISE);

	// Renoise-layout shortcut hints — platform-aware (Cmd on macOS).
	std::string scNew, scOpen, scSave, scSaveAs;
	if (renoise)
	{
		scNew    = GT2_ShortcutText((u32)'n', false, false, true);
		scOpen   = GT2_ShortcutText((u32)'o', false, false, true);
		scSave   = GT2_ShortcutText((u32)'s', false, false, true);
		scSaveAs = GT2_ShortcutText((u32)'s', true,  false, true);
	}

	if (ImGui::MenuItem("New Song", renoise ? scNew.c_str() : NULL))
	{
		stopsong();
		clearsong(1, 1, 1, 1, 1);
		if (viewPatterns) viewPatterns->ClearPatternUndoHistory();
		if (viewTables) viewTables->ClearTableUndoHistory();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Load Song...", renoise ? scOpen.c_str() : NULL))
		OpenLoadSongDialog();
	if (recentlyOpenedSongs)
		recentlyOpenedSongs->RenderImGuiMenu("Open Recent##GoatTracker");
	if (ImGui::MenuItem("Save Song", renoise ? scSave.c_str() : NULL))
	{
		// Quick save to the loaded file; fall back to a dialog if none.
		if (strlen(loadedsongfilename) > 0)
			SaveSongToFile(loadedsongfilename);
		else
			OpenSaveSongDialog();
	}
	if (ImGui::MenuItem("Save Song As...", renoise ? scSaveAs.c_str() : NULL))
		OpenSaveSongDialog();
	
	if (ImGui::MenuItem("Export Song..."))
		OpenExportDialog();

	ImGui::Separator();

	if (ImGui::MenuItem("Import Renoise Song..."))
	{
		if (viewRenoiseImport)
			viewRenoiseImport->Open();
	}

	ImGui::Separator();

	// GT2 ImGui views
	if (viewTitleBar && ImGui::MenuItem("GT2 Title Bar", "", viewTitleBar->visible))
		viewTitleBar->visible = !viewTitleBar->visible;
	if (viewPatterns && ImGui::MenuItem("GT2 Patterns", "", viewPatterns->visible))
		viewPatterns->visible = !viewPatterns->visible;
	if (viewOrderList && ImGui::MenuItem("GT2 Order List", "", viewOrderList->visible))
		viewOrderList->visible = !viewOrderList->visible;
	if (viewPatternList && ImGui::MenuItem("GT2 Pattern List", "", viewPatternList->visible))
		viewPatternList->visible = !viewPatternList->visible;
	if (viewInstrument && ImGui::MenuItem("GT2 Instrument", "", viewInstrument->visible))
		viewInstrument->visible = !viewInstrument->visible;
	if (viewInstrumentList && ImGui::MenuItem("GT2 Instruments", "", viewInstrumentList->visible))
		viewInstrumentList->visible = !viewInstrumentList->visible;
	if (viewTables && ImGui::MenuItem("GT2 Tables", "", viewTables->visible))
		viewTables->visible = !viewTables->visible;
	if (viewSongInfo && ImGui::MenuItem("GT2 Song Info", "", viewSongInfo->visible))
		viewSongInfo->visible = !viewSongInfo->visible;
	if (viewSongSettings && ImGui::MenuItem("GT2 Song Settings", "", viewSongSettings->visible))
		viewSongSettings->visible = !viewSongSettings->visible;
	if (viewStatus && ImGui::MenuItem("GT2 Status", "", viewStatus->visible))
		viewStatus->visible = !viewStatus->visible;
	if (viewMixer && ImGui::MenuItem("GT2 Mixer", "", viewMixer->visible))
		viewMixer->visible = !viewMixer->visible;
	if (viewToolbar && ImGui::MenuItem("GT2 Toolbar", "", viewToolbar->visible))
		viewToolbar->visible = !viewToolbar->visible;
	if (viewKeyboard && ImGui::MenuItem("GT2 Keyboard View", "", viewKeyboard->visible))
		viewKeyboard->visible = !viewKeyboard->visible;
	if (viewOscilloscope && ImGui::MenuItem("GT2 Oscilloscope", "", viewOscilloscope->visible))
		viewOscilloscope->visible = !viewOscilloscope->visible;
	if (viewStateSID && ImGui::MenuItem("GT2 SID", "", viewStateSID->visible))
		viewStateSID->visible = !viewStateSID->visible;
	if (viewInstrumentsBrowser && ImGui::MenuItem("GT2 Instruments Browser", "", viewInstrumentsBrowser->visible))
		viewInstrumentsBrowser->visible = !viewInstrumentsBrowser->visible;
	if (viewInstrumentTableRow && ImGui::MenuItem("GT2 Instrument Table Row", "", viewInstrumentTableRow->visible))
		viewInstrumentTableRow->visible = !viewInstrumentTableRow->visible;
	if (viewPatternRow && ImGui::MenuItem("GT2 Pattern Row", "", viewPatternRow->visible))
		viewPatternRow->visible = !viewPatternRow->visible;

	ImGui::Separator();

	// Arp settings
	{
		bool arpEnabled = (numarpcolumns > 0);
		if (ImGui::MenuItem("Enable Arp Columns", "", arpEnabled))
		{
			numarpcolumns = arpEnabled ? 0 : 4;
			PLUGIN_GoatTrackerSaveSettings();
		}
		if (arpEnabled && ImGui::BeginMenu("Arp Columns"))
		{
			for (int n = 1; n <= 12; n++)
			{
				char label[16];
				sprintf(label, "%d columns", n);
				if (ImGui::MenuItem(label, "", numarpcolumns == n))
				{
					numarpcolumns = n;
					PLUGIN_GoatTrackerSaveSettings();
				}
			}
			ImGui::EndMenu();
		}
	}

	ImGui::Separator();

	// Pattern display options (GT2 patterndispmode bitmask)
	{
		bool hexRows = (patterndispmode & 1) != 0;
		if (ImGui::MenuItem("Hexadecimal Row Numbers", "", hexRows))
		{
			patterndispmode ^= 1;
			PLUGIN_GoatTrackerSaveSettings();
		}
		bool emptyDots = (patterndispmode & 2) != 0;
		if (ImGui::MenuItem("Show Empty Instrument / Command as \"..\"", "", emptyDots))
		{
			patterndispmode ^= 2;
			PLUGIN_GoatTrackerSaveSettings();
		}
		extern int gt2CommandValueMode;
		bool commandValueMode = gt2CommandValueMode != 0;
		if (ImGui::MenuItem("Command Value Mode", "", commandValueMode))
		{
			gt2CommandValueMode ^= 1;
			PLUGIN_GoatTrackerSaveSettings();
		}

		extern int gt2VisibleSustainColumn;
		bool visibleSustain = gt2VisibleSustainColumn != 0;
		if (ImGui::MenuItem("Visible Sustain Column", "", visibleSustain))
		{
			gt2VisibleSustainColumn ^= 1;
			PLUGIN_GoatTrackerSaveSettings();
		}

		extern int gt2PatternCursorCentered;
		bool cursorCentered = gt2PatternCursorCentered != 0;
		if (ImGui::MenuItem("Centre Cursor Row (pattern / pattern list)", "",
			cursorCentered))
		{
			gt2PatternCursorCentered ^= 1;
			PLUGIN_GoatTrackerSaveSettings();
		}
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Autoload Song on Start", "", autoloadSongOnStart))
	{
		autoloadSongOnStart = !autoloadSongOnStart;
		PLUGIN_GoatTrackerSaveSettings();
	}

	if (ImGui::MenuItem("Keep Playing on Stop", "", gt2KeepPlayingOnStop))
	{
		gt2KeepPlayingOnStop = !gt2KeepPlayingOnStop;
		PLUGIN_GoatTrackerSaveSettings();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Off (default): stopping playback cuts the sound immediately, as\n"
						  "stock GT2 does -- every channel's waveform and wavetable are\n"
						  "cleared the moment you press Space.\n"
						  "\n"
						  "On: stopping only stops the sequencer. Whatever is sounding rings\n"
						  "out and decays naturally, the same way a row triggered with Enter\n"
						  "does. Applies to Space and to the toolbar's Pause button; the\n"
						  "toolbar's Stop button always resets the player.");
	}

	if (ImGui::MenuItem("Fix Instrument Gatetimer on Load", "", fixInstrumentGatetimerOnLoad))
	{
		fixInstrumentGatetimerOnLoad = !fixInstrumentGatetimerOnLoad;
		PLUGIN_GoatTrackerSaveSettings();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("An instrument whose gatetimer is longer than the song's tick makes\n"
						  "the player refuse to start the song at all (it jumps to row 0 and\n"
						  "stays silent). With this on, a loaded instrument's gatetimer is\n"
						  "lowered to the highest value this song's tempo allows (now: %d).\n"
						  "Undo takes the load and the correction back together.",
						  gt2MaxSafeGatetimer());
	}

	if (ImGui::BeginMenu("Keyboard Layout"))
	{
		if (ImGui::MenuItem("Tracker", "", keypreset == KEY_TRACKER))
		{ keypreset = KEY_TRACKER; PLUGIN_GoatTrackerSaveSettings(); }
		if (ImGui::MenuItem("DMC", "", keypreset == KEY_DMC))
		{ keypreset = KEY_DMC; PLUGIN_GoatTrackerSaveSettings(); }
		if (ImGui::MenuItem("Janko", "", keypreset == KEY_JANKO))
		{ keypreset = KEY_JANKO; PLUGIN_GoatTrackerSaveSettings(); }
		if (ImGui::MenuItem("Renoise", "", keypreset == KEY_RENOISE))
		{ keypreset = KEY_RENOISE; PLUGIN_GoatTrackerSaveSettings(); }
		if (ImGui::MenuItem("Custom...", "", keypreset == KEY_CUSTOM))
		{ keypreset = KEY_CUSTOM; PLUGIN_GoatTrackerSaveSettings(); }
		if (keypreset == KEY_RENOISE)
		{
			ImGui::Separator();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::InputInt("Renoise Edit Step", &gt2RenoiseEditStep, 0, 0);
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (gt2RenoiseEditStep < 0) gt2RenoiseEditStep = 0;
				PLUGIN_GoatTrackerSaveSettings();
			}
			if (ImGui::MenuItem("Renoise Follow Track", "", gt2RenoiseFollowTrack))
			{
				gt2RenoiseFollowTrack = !gt2RenoiseFollowTrack;
				PLUGIN_GoatTrackerSaveSettings();
			}
			if (ImGui::MenuItem("Bulk Pattern Number Change", "", gt2RenoiseBulkPatternNumberChange))
			{
				gt2RenoiseBulkPatternNumberChange = !gt2RenoiseBulkPatternNumberChange;
				PLUGIN_GoatTrackerSaveSettings();
			}
			ImGui::Separator();
			int zoomPct = (int)(gt2RenoiseUIScale * 100.0f + 0.5f);
			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::SliderInt("UI Zoom", &zoomPct, 50, 350, "%d%%"))
			{
				GT2_SetRenoiseUIScale((float)zoomPct / 100.0f);
			}
			{
				const char *cmd = GT2_CmdKey();
				char zbuf[32];
				snprintf(zbuf, sizeof(zbuf), "%s+=", cmd);
				if (ImGui::MenuItem("Zoom In", zbuf))
					GT2_StepRenoiseUIScale(+1);
				snprintf(zbuf, sizeof(zbuf), "%s+-", cmd);
				if (ImGui::MenuItem("Zoom Out", zbuf))
					GT2_StepRenoiseUIScale(-1);
				if (ImGui::MenuItem("Reset Zoom to 100%"))
					GT2_SetRenoiseUIScale(1.0f);
			}
		}
		ImGui::Separator();
		if (viewKeyboardSetup && ImGui::MenuItem("Edit Custom Layout..."))
			viewKeyboardSetup->visible = true;
		ImGui::EndMenu();
	}

	// Renoise-layout shortcut reference (static, non-clickable submenu).
	if (renoise)
	{
		ImGui::Separator();
		if (ImGui::BeginMenu("Renoise Shortcuts"))
		{
			const char *cmd = GT2_CmdKey();   // "Cmd" / "Ctrl", platform-aware
			char buf[96];

			ImGui::MenuItem("Next / Prev Track",      "Tab / Shift+Tab", false, false);
			ImGui::MenuItem("Trigger Row",            "Enter", false, false);
			ImGui::MenuItem("Toggle Write Mode",      "Esc", false, false);
			ImGui::MenuItem("Play / Stop",            "Space", false, false);
			ImGui::MenuItem("Play From Cursor",       "Shift+Space", false, false);
			snprintf(buf, sizeof(buf), "Num* / Num/ , %s+] / %s+[", cmd, cmd);
			ImGui::MenuItem("Octave Up / Down",       buf, false, false);
			ImGui::MenuItem("Next / Prev Instrument", "Num+ / Num- , Alt+Down / Alt+Up", false, false);
			ImGui::MenuItem("Note Off",               "Caps Lock / A", false, false);
			ImGui::MenuItem("Clear Note",             "Delete", false, false);
			ImGui::MenuItem("Clear Whole Row",        "Alt+Delete", false, false);
			snprintf(buf, sizeof(buf), "\\ / %s+\\", cmd);
			ImGui::MenuItem("Mute / Solo Channel",    buf, false, false);
			snprintf(buf, sizeof(buf), "%s+Right / %s+Left", cmd, cmd);
			ImGui::MenuItem("Pattern Number + / -",   buf, false, false);
			snprintf(buf, sizeof(buf), "%s+0 ... %s+9", cmd, cmd);
			ImGui::MenuItem("Set Edit Step 0-9",      buf, false, false);
			ImGui::MenuItem("Edit Step +1 / -1",      "` / ~", false, false);
			snprintf(buf, sizeof(buf), "%s+= / %s+-", cmd, cmd);
			ImGui::MenuItem("UI Zoom In / Out",       buf, false, false);
			ImGui::MenuItem("Edit Step x2 / /2",      "Alt+= / Alt+-", false, false);
			snprintf(buf, sizeof(buf), "%s+Z / %s+Y", cmd, cmd);
			ImGui::MenuItem("Undo / Redo",            buf, false, false);

			ImGui::SeparatorText("Row Navigation");
			ImGui::MenuItem("Jump to Row 0 / 16",      "F9 / F10", false, false);
			ImGui::MenuItem("Jump to Row 32 / 48",     "F11 / F12", false, false);
			snprintf(buf, sizeof(buf), "Shift+%s+Up/Dn or PgUp/PgDn", cmd);
			ImGui::MenuItem("Prev / Next Row w/ Note", buf, false, false);
			snprintf(buf, sizeof(buf), "%s+Up / %s+Down", cmd, cmd);
			ImGui::MenuItem("Prev / Next Pattern",     buf, false, false);

			ImGui::SeparatorText("Channel Row");
			ImGui::MenuItem("Insert Row",              "Insert", false, false);
			ImGui::MenuItem("Remove Row",              "Shift+Backspace", false, false);

			ImGui::SeparatorText("Song Order");
			snprintf(buf, sizeof(buf), "%s+Insert", cmd);
			ImGui::MenuItem("Insert Pattern",          buf, false, false);
			snprintf(buf, sizeof(buf), "%s+Delete", cmd);
			ImGui::MenuItem("Delete Pattern",          buf, false, false);
			snprintf(buf, sizeof(buf), "%s+K", cmd);
			ImGui::MenuItem("Duplicate Pattern",       buf, false, false);

			ImGui::SeparatorText("Selection / Cell Ops");
			ImGui::MenuItem("Transpose Note +/-##sel",   "Alt+F2 / Alt+F1", false, false);
			ImGui::MenuItem("Transpose Octave +/-##sel", "Alt+F12 / Alt+F11", false, false);
			ImGui::MenuItem("Cut / Copy / Paste##sel",   "Alt+F3 / F4 / F5", false, false);
			ImGui::MenuItem("Clear##sel",                "Shift+Delete", false, false);
			ImGui::MenuItem("Shrink / Expand##sel",      "Alt+F8 / F9", false, false);

			ImGui::SeparatorText("Track Ops (cursor column)");
			ImGui::MenuItem("Transpose Note +/-##trk",   "Shift+F2 / Shift+F1", false, false);
			ImGui::MenuItem("Transpose Octave +/-##trk", "Shift+F12 / Shift+F11", false, false);
			ImGui::MenuItem("Cut / Copy / Paste##trk",   "Shift+F3 / F4 / F5", false, false);
			ImGui::MenuItem("Shrink / Expand##trk",      "Shift+F8 / F9", false, false);

			ImGui::SeparatorText("Phrase Ops (all channels)");
			snprintf(buf, sizeof(buf), "%s+F2 / %s+F1", cmd, cmd);
			ImGui::MenuItem("Transpose Note +/-##phr",   buf, false, false);
			snprintf(buf, sizeof(buf), "%s+F12 / %s+F11", cmd, cmd);
			ImGui::MenuItem("Transpose Octave +/-##phr", buf, false, false);
			snprintf(buf, sizeof(buf), "%s+F3 / F4 / F5", cmd);
			ImGui::MenuItem("Cut / Copy / Paste##phr",   buf, false, false);
			snprintf(buf, sizeof(buf), "%s+F8 / F9", cmd);
			ImGui::MenuItem("Shrink / Expand##phr",      buf, false, false);

			ImGui::EndMenu();
		}
	}
}

void C64DebuggerPluginGoatTracker::RenderExportDialog()
{
	if (!showExportDialog)
		return;

	ImGui::SetNextWindowSize(ImVec2(480, 590), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(450, 560), ImVec2(FLT_MAX, FLT_MAX));
	if (!ImGui::Begin("Export Song", &showExportDialog))
	{
		ImGui::End();
		return;
	}

	static const char *optionNames[] = {
		"Buffered SID-writes",
		"Sound effect support",
		"Volume change support",
		"Store author-info",
		"Use zeropage ghostregs",
		"Disable optimization",
		"Full SID buffering"
	};

	ImGui::SeparatorText("Playroutine Options");
	for (int i = 0; i < 7; i++)
	{
		if (ImGui::Checkbox(optionNames[i], &exportPlayerOptions[i]))
		{
			// Enforce dependencies like GT2 does
			if (i == 0 && !exportPlayerOptions[0])
			{
				// Buffered writes OFF -> disable dependent options
				exportPlayerOptions[1] = false; // Sound effects
				exportPlayerOptions[4] = false; // ZP ghostregs
				exportPlayerOptions[6] = false; // Full buffering
			}
			if ((i == 1 || i == 4 || i == 6) && exportPlayerOptions[i])
			{
				// These require buffered writes
				exportPlayerOptions[0] = true;
			}
		}
	}

	ImGui::SeparatorText("Song Info");
	ImGui::InputText("Title", songname, MAX_STR);
	ImGui::InputText("Author", authorname, MAX_STR);
	ImGui::InputText("Copyright", copyrightname, MAX_STR);

	ImGui::SeparatorText("Addresses");
	ImGui::InputInt("Player address", &exportPlayerAddress, 0x100, 0x400, ImGuiInputTextFlags_CharsHexadecimal);
	exportPlayerAddress &= 0xFF00;
	if (exportPlayerAddress < 0) exportPlayerAddress = 0;
	if (exportPlayerAddress > 0xFF00) exportPlayerAddress = 0xFF00;

	int zpMax = exportPlayerOptions[4] ? 0xE5 : 0xFE;
	ImGui::InputInt("Zeropage address", &exportZeropageAddress, 1, 0x10, ImGuiInputTextFlags_CharsHexadecimal);
	if (exportZeropageAddress < 0x02) exportZeropageAddress = 0x02;
	if (exportZeropageAddress > zpMax) exportZeropageAddress = zpMax;

	ImGui::SeparatorText("File Format");
	bool exportFormatChanged = false;
	if (ImGui::RadioButton("SID - SIDPlay music file", &exportFileFormat, FORMAT_SID))          exportFormatChanged = true;
	if (ImGui::RadioButton("PRG - C64 native format", &exportFileFormat, FORMAT_PRG))           exportFormatChanged = true;
	if (ImGui::RadioButton("BIN - Raw binary (no startaddress)", &exportFileFormat, FORMAT_BIN)) exportFormatChanged = true;
	if (ImGui::RadioButton("PSID64 PRG - SID wrapped as a runnable PRG", &exportFileFormat, kGT2ExportFormatPsid64Prg)) exportFormatChanged = true;
	if (exportFormatChanged)
		PLUGIN_GoatTrackerSaveSettings();

	ImGui::Separator();
	if (ImGui::Button("Export...", ImVec2(120, 0)))
	{
		// Set GT2 globals from dialog state. For PSID64-PRG the song is first
		// exported as SID, so the GT2 fileformat is managed in DoExportPsid64Prg.
		playeradr = exportPlayerAddress;
		zeropageadr = exportZeropageAddress;
		if (exportFileFormat != kGT2ExportFormatPsid64Prg)
			fileformat = exportFileFormat;
		playerversion = 0;
		for (int i = 0; i < 7; i++)
		{
			if (exportPlayerOptions[i])
				playerversion |= (PLAYER_BUFFERED << i);
		}

		// Open save file dialog
		const char *ext;
		switch (exportFileFormat)
		{
			case FORMAT_SID: ext = "sid"; break;
			case FORMAT_BIN: ext = "bin"; break;
			default:         ext = "prg"; break;
		}

		std::list<CSlrString *> extensions;
		extensions.push_back(new CSlrString(ext));

		// Default filename from loaded song
		CSlrString *defaultFileName = NULL;
		CSlrString *defaultFolder = NULL;

		if (strlen(loadedsongfilename) > 0)
		{
			CSlrString *fullPath = new CSlrString(loadedsongfilename);
			CSlrString *nameOnly = fullPath->GetFileNameComponentFromPath();
			defaultFolder = fullPath->GetFilePathWithoutFileNameComponentFromPath();

			// Replace extension
			char *cName = nameOnly->GetStdASCII();
			char exportName[512];
			strncpy(exportName, cName, sizeof(exportName) - 1);
			exportName[sizeof(exportName) - 1] = 0;
			char *dot = strrchr(exportName, '.');
			if (dot) *dot = 0;
			strncat(exportName, ".", sizeof(exportName) - strlen(exportName) - 1);
			strncat(exportName, ext, sizeof(exportName) - strlen(exportName) - 1);
			defaultFileName = new CSlrString(exportName);

			delete[] cName;
			delete nameOnly;
			delete fullPath;
		}

		CSlrString *windowTitle = new CSlrString("Export Song");
		exportWaitingForSaveDialog = true;
		viewC64->ShowDialogSaveFile(this, &extensions, defaultFileName, defaultFolder, windowTitle);

		delete windowTitle;
		if (defaultFileName) delete defaultFileName;
		if (defaultFolder) delete defaultFolder;
		for (auto *e : extensions) delete e;
	}

	if (exportStatusMessage[0])
	{
		ImGui::Spacing();
		ImGui::TextWrapped("%s", exportStatusMessage);
	}

	ImGui::End();
}

void C64DebuggerPluginGoatTracker::DoExport(const char *filePath)
{
	snprintf(exportStatusMessage, sizeof(exportStatusMessage), "Exporting...");

	if (exportFileFormat == kGT2ExportFormatPsid64Prg)
	{
		DoExportPsid64Prg(filePath);
		return;
	}

	char errorMsg[256] = {0};
	int result = relocator_export(filePath, errorMsg, sizeof(errorMsg));

	if (result == 0)
		snprintf(exportStatusMessage, sizeof(exportStatusMessage), "Exported OK");
	else
		snprintf(exportStatusMessage, sizeof(exportStatusMessage), "Error: %s", errorMsg);
}

// PSID64-PRG export: GT2 has no SID-to-membuf path, so the song is exported to
// a temp .sid next to the destination (same folder, so it shares writability),
// fed through the PSID64 library, and the resulting runnable PRG is written to
// filePath. The temp SID is always removed.
void C64DebuggerPluginGoatTracker::DoExportPsid64Prg(const char *filePath)
{
	char tempSidPath[MAX_FILENAME];
	snprintf(tempSidPath, sizeof(tempSidPath), "%s.psid64tmp.sid", filePath);

	int savedFileFormat = fileformat;
	fileformat = FORMAT_SID;
	char errorMsg[256] = {0};
	int result = relocator_export(tempSidPath, errorMsg, sizeof(errorMsg));
	fileformat = savedFileFormat;

	if (result != 0)
	{
		remove(tempSidPath);
		snprintf(exportStatusMessage, sizeof(exportStatusMessage), "Error: %s", errorMsg);
		return;
	}

	CByteBuffer *sidBuffer = new CByteBuffer(tempSidPath, false);
	remove(tempSidPath);
	if (sidBuffer->length == 0)
	{
		delete sidBuffer;
		snprintf(exportStatusMessage, sizeof(exportStatusMessage),
			"Error: SID export produced no data");
		return;
	}

	CByteBuffer *prgBuffer = ConvertSIDtoPRG(sidBuffer);
	delete sidBuffer;
	if (prgBuffer == NULL)
	{
		snprintf(exportStatusMessage, sizeof(exportStatusMessage),
			"Error: PSID64 conversion failed");
		return;
	}

	FILE *f = fopen(filePath, "wb");
	if (f == NULL)
	{
		delete prgBuffer;
		snprintf(exportStatusMessage, sizeof(exportStatusMessage),
			"Error: cannot write %s", filePath);
		return;
	}
	fwrite(prgBuffer->data, 1, prgBuffer->length, f);
	fclose(f);
	delete prgBuffer;

	snprintf(exportStatusMessage, sizeof(exportStatusMessage), "Exported PSID64 PRG OK");
}

// Phase 6 VICE RAM bridge: pack the current song and append the
// FORMAT_PRG bytes (2-byte load address + assembled payload) to `out`.
// Allocates a transient `struct membuf` since the C-side packer speaks
// that type; the bytes are then copied into the caller's CByteBuffer
// and the membuf is freed before returning.
int C64DebuggerPluginGoatTracker::ExportToBuffer(CByteBuffer *out,
                                                 char *errorMsg,
                                                 int errorMsgSize)
{
	if (!out) return -6;

	struct membuf tmp = STATIC_MEMBUF_INIT;
	int result = relocator_export_to_membuf(&tmp, errorMsg, errorMsgSize);
	if (result == 0)
	{
		int n = membuf_memlen(&tmp);
		if (n > 0)
			out->putBytes((u8*)membuf_get(&tmp), (unsigned int)n);
	}
	membuf_free(&tmp);
	return result;
}
