#ifndef _C64DEBUGGER_PLUGIN_GOATTRACKER_H_
#define _C64DEBUGGER_PLUGIN_GOATTRACKER_H_

#include "CDebuggerEmulatorPluginVice.h"
#include "CDebuggerApi.h"
#include "CGlobalLayoutCallback.h"
#include "CSystemFileDialogCallback.h"
#include "CRecentlyOpenedFiles.h"
#include <list>
#include <string>

class CByteBuffer;

class CImageData;
class CViewC64GoatTracker;
class CAudioChannelGoatTracker;
class CViewRenoiseImport;

class CGT2FontAtlas;
class CConfigStorageHjson;
class CGT2AudioMixer;
class CViewGT2Patterns;
class CViewGT2OrderList;
class CViewGT2PatternList;
class CViewGT2Instrument;
class CViewGT2Tables;
class CViewGT2InstrumentList;
class CViewGT2SongInfo;
class CViewGT2SongSettings;
class CViewGT2Status;
class CViewGT2TitleBar;
class CViewGT2Mixer;
class CViewGT2Toolbar;
class CViewGT2KeyboardSetup;
class CPianoKeyboardGT2;
class CViewGT2Oscilloscope;
class CViewGT2StateSID;
class CViewGT2InstrumentsBrowser;
class CViewGT2InstrumentTableRow;
class CViewGT2PatternRow;
class CGT2RenoiseInput;
class CGT2Favorites;

class C64DebuggerPluginGoatTracker : public CDebuggerEmulatorPluginVice, CSlrThread, CSystemFileDialogCallback, public CGlobalLayoutCallback,
									 public CRecentlyOpenedFilesCallback
{
public:
	C64DebuggerPluginGoatTracker();
	virtual ~C64DebuggerPluginGoatTracker();

	virtual void Init();
	virtual void ThreadRun(void *data);
	void Shutdown();

	virtual void DoFrame();
	virtual bool HandleOpenFileShortcut();
	virtual void AddOpenFileExtensions(std::list<std::string> *extensions, bool isKeyboardShortcut);
	virtual bool OpenFile(CSlrString *path, bool isKeyboardShortcut);
	virtual void GlobalLayoutWillDeserialize(CLayoutData *layout);
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);

	// Generic plugin top-level menu (see CDebuggerEmulatorPlugin).
	virtual const char *GetMainMenuName();
	virtual void RenderMainMenuImGui();

	CViewC64GoatTracker *view;
	CAudioChannelGoatTracker *audioChannel;
	CViewRenoiseImport *viewRenoiseImport;

	CGT2FontAtlas *fontAtlas;
	// Dedicated GT2 config: gPathToSettings/gt2/gt2-settings.hjson
	CConfigStorageHjson *gt2Config;
	CGT2AudioMixer *audioMixer;
	CViewGT2Patterns *viewPatterns;
	CViewGT2OrderList *viewOrderList;
	CViewGT2PatternList *viewPatternList;
	CViewGT2Instrument *viewInstrument;
	CViewGT2InstrumentList *viewInstrumentList;
	CViewGT2Tables *viewTables;
	CViewGT2SongInfo *viewSongInfo;
	CViewGT2SongSettings *viewSongSettings;
	CViewGT2Status *viewStatus;
	CViewGT2TitleBar *viewTitleBar;
	CViewGT2Mixer *viewMixer;
	CViewGT2Toolbar *viewToolbar;
	CViewGT2KeyboardSetup *viewKeyboardSetup;
	CPianoKeyboardGT2 *viewKeyboard;
	CViewGT2Oscilloscope *viewOscilloscope;
	CViewGT2StateSID *viewStateSID;
	CViewGT2InstrumentsBrowser *viewInstrumentsBrowser;
	CViewGT2InstrumentTableRow *viewInstrumentTableRow;
	CViewGT2PatternRow *viewPatternRow;

	CGT2Favorites *gt2Favorites;

	// Renoise keyboard-layout dispatcher. Each GT2 view's KeyDown delegates
	// here so Renoise rebindings work regardless of which view has focus.
	// See CGT2RenoiseInput.h.
	CGT2RenoiseInput *renoiseInput;
	volatile bool shutdownRequested;

	void SetupShadowRegsPlayer();
	void LoadSongFromFile(const char *filePath);
	std::list<CSlrString *> songFileExtensions;
	void OpenLoadSongDialog();

	// Recently opened GT2 songs, kept in its own settings file
	// ("recents-gt2songs") so it does not mix with the File/Recents list.
	CRecentlyOpenedFiles *recentlyOpenedSongs;
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();

	void SaveSongToFile(const char *filePath);
	void OpenSaveSongDialog();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();

	// Export dialog state
	bool showExportDialog;
	bool exportPlayerOptions[7]; // matches GT2 MAX_OPTIONS
	int exportFileFormat;        // FORMAT_SID=0, FORMAT_PRG=1, FORMAT_BIN=2, PSID64-PRG=3 (c64d-only)
	int exportPlayerAddress;
	int exportZeropageAddress;
	char exportStatusMessage[256];
	bool exportWaitingForSaveDialog;

	void OpenExportDialog();
	void RenderExportDialog();
	void DoExport(const char *filePath);
	// Export the song to a temp SID, then run it through the PSID64 library
	// to produce a runnable PRG saved at filePath.
	void DoExportPsid64Prg(const char *filePath);

	// Phase 6 VICE RAM bridge: pack & assemble the current song and
	// append the FORMAT_PRG bytes (2-byte load address + assembled
	// payload) to `out`. Returns 0 on success, negative on failure with
	// a human-readable message in `errorMsg`. Designed to be paired with
	// CDebuggerApiVice::LoadPRG(CByteBuffer*, autoStart, forceFastReset)
	// for parity testing or direct-to-VICE playback without disk I/O.
	int ExportToBuffer(CByteBuffer *out, char *errorMsg, int errorMsgSize);

	// Loads a *.ins into `instrumentNum` (1..GT2_LAST_INSTR), selecting that
	// instrument first so the editor follows. Undoable: gsong.c
	// loadinstrument() records a step on the table undo stack, so Ctrl+Z and
	// the toolbar Undo revert it. `preview` auditions the result with the note
	// last played in the editor. Returns false when the file cannot be read.
	bool LoadInstrumentFromFile(const char *filePath, int instrumentNum, bool preview);
	// Folder the instrument browser and the load-instrument dialog share,
	// persisted in gt2Config as GoatTrackerInstrumentFolder.
	void RememberInstrumentFolderOf(const char *filePath);
	std::string GetInstrumentFolder();

	// Load/Save instrument dialog routing
	// 0 = none (song or export), 1 = load instrument, 2 = save instrument
	int instrumentDialogMode;
	void OpenLoadInstrumentDialog();
	void OpenSaveInstrumentDialog();

	// Clamp an instrument's gatetimer to gt2MaxSafeGatetimer() as it is loaded,
	// so a file exported with a gatetimer longer than the song's tick cannot
	// leave the song refusing to play. On by default; only the ImGui-side
	// loaders apply it, native GT2 text mode is untouched.
	bool fixInstrumentGatetimerOnLoad;

	// Autoload song on startup
	bool autoloadSongOnStart;
	bool autoloadPending;

	// assemble
	u16 addrAssemble;
	void Assemble(char *buf);
	void PutDataByte(u8 v);
};

// singleton
extern C64DebuggerPluginGoatTracker *pluginGoatTracker;

void PLUGIN_GoatTrackerInit();

// The command-modifier word for shortcut labels: "Cmd" on macOS, "Ctrl"
// elsewhere. Taken from the engine's key formatter, never hardcoded.
const char *GT2_CmdKey();
void PLUGIN_GoatTrackerSetVisible(bool isVisible);
bool PLUGIN_GoatTrackerIsVisible();
void PLUGIN_GoatTrackerSaveSettings();
void PLUGIN_GoatTrackerRestoreSettings();
void PLUGIN_GoatTrackerShutdown();

// Renoise-layout UI zoom. The only sanctioned way to change gt2RenoiseUIScale.
// Clamps to [0.5, 3.5], snaps to the 12.5% grid, and marks the layout dirty
// when the value actually changes so the new zoom persists per workspace.
void GT2_SetRenoiseUIScale(float scale);
void GT2_StepRenoiseUIScale(int direction);  // direction = +1 / -1, one 12.5% step

#endif
