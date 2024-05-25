#ifndef _C64SETTINGS_STORAGE_H_
#define _C64SETTINGS_STORAGE_H_

#include "SYS_Defs.h"
#include "SYS_Types.h"
#include "CSlrString.h"
#include "EmulatorsConfig.h"

// TODO: make generic. move settings.dat to settings.hjson (in progress...)
#define C64D_SETTINGS_FILE_PATH				"settings.dat"
#define C64D_KEYBOARD_SHORTCUTS_FILE_PATH	"shortcuts.dat"
#define C64D_KEYMAP_FILE_PATH				"keymap.dat"


// settings that need to be initialized pre-launch
#define C64DEBUGGER_BLOCK_PRELAUNCH		1

// settings that need to be set when emulation is initialized
#define C64DEBUGGER_BLOCK_POSTLAUNCH	2

enum loadPRGResetMode : u8
{
	MACHINE_LOADPRG_RESET_MODE_NONE = 0,
	MACHINE_LOADPRG_RESET_MODE_SOFT = 1,
	MACHINE_LOADPRG_RESET_MODE_HARD = 2,
	MACHINE_LOADPRG_RESET_MODE_LOAD_SNAPSHOT_BASIC = 3,
	MACHINE_LOADPRG_RESET_MODE_END_OF_ENUM = 4,
	MACHINE_LOADPRG_RESET_MODE_LOAD_SNAPSHOT_CUSTOM = 5
};

enum muteSIDMode : u8
{
	MUTE_SID_MODE_ZERO_VOLUME		= 0,
	MUTE_SID_MODE_SKIP_EMULATION	= 1
};

enum c64SIDImportMode : u8
{
	SID_IMPORT_MODE_DIRECT_COPY		= 0,
	SID_IMPORT_MODE_PSID64			= 1
};

enum atariVideoSystem : u8
{
	ATARI_VIDEO_SYSTEM_PAL = 0,
	ATARI_VIDEO_SYSTEM_NTSC = 1
};

// startup data
extern u8 c64SettingsSelectEmulator;

// settings
extern bool c64SettingsSkipConfig;
extern bool c64SettingsPassConfigToRunningInstance;

extern bool c64SettingsKeepSymbolsLabels;
extern bool c64SettingsKeepSymbolsWatches;
extern bool c64SettingsKeepSymbolsBreakpoints;

extern int c64SettingsDefaultScreenLayoutId;
extern bool c64SettingsIsInVicEditor;

extern int c64SettingsScreenSupersampleFactor;
// emu screens should consume key shortcuts? Emulator Screen Input: Bypass App Shortcuts
extern bool c64SettingsEmulatorScreenBypassKeyboardShortcuts;

extern bool c64SettingsUsePipeIntegration;

extern uint8 c64SettingsMemoryValuesStyle;
extern uint8 c64SettingsMemoryMarkersStyle;
extern bool c64SettingsUseMultiTouchInMemoryMap;
extern bool c64SettingsMemoryMapInvertControl;
extern uint8 c64SettingsMemoryMapRefreshRate;
extern int c64SettingsMemoryMapFadeSpeed;

extern uint8 c64SettingsC64Model;
extern int c64SettingsEmulationMaximumSpeed;
extern bool c64SettingsFastBootKernalPatch;

extern bool c64SettingsReuEnabled;
extern int c64SettingsReuSize;

extern uint8 c64SettingsSIDEngineModel;
extern uint8 c64SettingsRESIDSamplingMethod;
extern bool c64SettingsRESIDEmulateFilters;
extern int c64SettingsRESIDPassBand;
extern int c64SettingsRESIDFilterBias;

extern uint8 c64SettingsSIDStereo;			// "SidStereo" 0=none, 1=stereo, 2=triple
extern uint16 c64SettingsSIDStereoAddress;	// "SidStereoAddressStart"
extern uint16 c64SettingsSIDTripleAddress;	// "SidTripleAddressStart"

extern bool c64SettingsMuteSIDOnPause;

extern int c64SettingsViceAudioVolume;
extern bool c64SettingsRunSIDEmulation;
extern uint8 c64SettingsMuteSIDMode;

extern int c64SettingsSidDataHistoryMaxSize;

extern bool c64SettingsEmulateVSPBug;
extern bool c64SettingsVicSkipDrawingSprites;

extern uint8 c64SettingsVicStateRecordingMode;
extern uint16 c64SettingsVicPalette;
extern bool c64SettingsRenderScreenNearest;

extern bool c64SettingsExecuteAwareDisassembly;
extern bool c64SettingsPressCtrlToSetBreakpoint;

extern bool c64SettingsWindowAlwaysOnTop;
extern CByteBuffer *c64SettingsWindowPosition;

extern float c64SettingsScreenGridLinesAlpha;
extern uint8 c64SettingsScreenGridLinesColorScheme;
extern float c64SettingsScreenRasterViewfinderScale;
extern float c64SettingsScreenRasterCrossLinesAlpha;
extern uint8 c64SettingsScreenRasterCrossLinesColorScheme;
extern float c64SettingsScreenRasterCrossAlpha;
extern uint8 c64SettingsScreenRasterCrossExteriorColorScheme;
extern uint8 c64SettingsScreenRasterCrossInteriorColorScheme;
extern uint8 c64SettingsScreenRasterCrossTipColorScheme;

extern bool c64SettingsShowPositionsInHex;

//
extern u8 c64SettingsSelectedJoystick1;
extern u8 c64SettingsSelectedJoystick2;

// startup
extern int c64SettingsWaitOnStartup;
extern CSlrString *c64SettingsPathToRomsC64;
extern CSlrString *c64SettingsPathToRomC64Kernal;
extern CSlrString *c64SettingsPathToRomC64Basic;
extern CSlrString *c64SettingsPathToRomC64Chargen;
extern CSlrString *c64SettingsPathToRomC64Drive1541;
extern CSlrString *c64SettingsPathToRomC64Drive1541ii;
extern CSlrString *c64SettingsPathToD64;
extern CSlrString *c64SettingsDefaultD64Folder;
extern CSlrString *c64SettingsPathToPRG;
extern CSlrString *c64SettingsDefaultPRGFolder;
extern CSlrString *c64SettingsPathToCartridge;
extern CSlrString *c64SettingsDefaultCartridgeFolder;
extern CSlrString *c64SettingsPathToViceSnapshot;
extern CSlrString *c64SettingsPathToAtariSnapshot;
extern CSlrString *c64SettingsDefaultSnapshotsFolder;
extern CSlrString *c64SettingsDefaultMemoryDumpFolder;
extern CSlrString *c64SettingsPathToC64MemoryMapFile;

extern CSlrString *c64SettingsPathToTAP;
extern CSlrString *c64SettingsDefaultTAPFolder;

extern CSlrString *c64SettingsPathToReu;
extern CSlrString *c64SettingsDefaultReuFolder;

extern CSlrString *c64SettingsDefaultVicEditorFolder;

extern CSlrString *c64SettingsPathToAtariROMs;

extern bool c64SettingsAtariPokeyStereo;
extern CSlrString *c64SettingsPathToATR;
extern CSlrString *c64SettingsDefaultATRFolder;
extern CSlrString *c64SettingsPathToXEX;
extern CSlrString *c64SettingsDefaultXEXFolder;
extern CSlrString *c64SettingsPathToCAS;
extern CSlrString *c64SettingsDefaultCASFolder;
extern CSlrString *c64SettingsPathToAtariCartridge;
extern CSlrString *c64SettingsDefaultAtariCartridgeFolder;

extern CSlrString *c64SettingsPathToNES;
extern CSlrString *c64SettingsDefaultNESFolder;

extern CSlrString *c64SettingsPathToSymbols;
extern CSlrString *c64SettingsPathToWatches;
extern CSlrString *c64SettingsPathToBreakpoints;
extern CSlrString *c64SettingsPathToDebugInfo;

extern bool c64SettingsUseNativeEmulatorMonitor;

extern CSlrString *c64SettingsPathToJukeboxPlaylist;

// priority
extern bool c64SettingsIsProcessPriorityBoostDisabled;
extern u8 c64SettingsProcessPriority;

// profiler
extern CSlrString *c64SettingsC64ProfilerFileOutputPath;
extern bool c64SettingsC64ProfilerDoVicProfile;

// snapshots recorder
extern bool c64SettingsSnapshotsRecordIsActive;

// storing interval
extern int c64SettingsSnapshotsIntervalNumFrames;
// max number of snapshots
extern int c64SettingsSnapshotsLimit;
// compression level for save
extern u8 c64SettingsTimelineSaveZlibCompressionLevel;

// sid import
extern u8 c64SettingsC64SidImportMode;

extern CSlrString *c64SettingsAudioOutDevice;
extern bool c64SettingsRestartAudioOnEmulationReset;

extern bool c64SettingsAlwaysUnpauseEmulationAfterReset;

extern int c64SettingsJmpOnStartupAddr;

extern bool c64SettingsAutoJmpAlwaysToLoadedPRGAddress;
extern bool c64SettingsAutoJmpFromInsertedDiskFirstPrg;
extern int c64SettingsAutoJmpDoReset;
extern int c64SettingsAutoJmpWaitAfterReset;
extern bool c64SettingsForceUnpause;

//
extern int c64SettingsDatasetteSpeedTuning;
extern int c64SettingsDatasetteZeroGapDelay;
extern int c64SettingsDatasetteTapeWobble;
extern bool c64SettingsDatasetteResetWithCPU;

extern bool c64SettingsResetCountersOnAutoRun;

extern bool c64SettingsRunSIDWhenInWarp;

extern u8 c64SettingsVicDisplayBorderType;
extern bool c64SettingsVicDisplayShowGridLines;
extern bool c64SettingsVicDisplayApplyScroll;
extern bool c64SettingsVicDisplayShowBadLines;

extern float c64SettingsPaintGridCharactersColorR;
extern float c64SettingsPaintGridCharactersColorG;
extern float c64SettingsPaintGridCharactersColorB;
extern float c64SettingsPaintGridCharactersColorA;

extern float c64SettingsPaintGridPixelsColorR;
extern float c64SettingsPaintGridPixelsColorG;
extern float c64SettingsPaintGridPixelsColorB;
extern float c64SettingsPaintGridPixelsColorA;

extern float c64SettingsFocusBorderLineWidth;

extern bool c64SettingsVicEditorForceReplaceColor;
extern u8 c64SettingsVicEditorDefaultBackgroundColor;

extern int c64SettingsDisassemblyBackgroundColor;
extern int c64SettingsDisassemblyExecuteColor;
extern int c64SettingsDisassemblyNonExecuteColor;
extern bool c64SettingsDisassemblyUseNearLabels;
extern bool c64SettingsDisassemblyUseNearLabelsForJumps;
extern int c64SettingsDisassemblyNearLabelMaxOffset;

extern int c64SettingsMenusColorTheme;

extern bool c64SettingsUseSystemFileDialogs;
extern bool c64SettingsUseOnlyFirstCPU;

extern int c64SettingsDoubleClickMS;

extern bool c64SettingsLoadViceLabels;
extern bool c64SettingsLoadWatches;
extern bool c64SettingsLoadDebugInfo;

extern bool c64SettingsInterpolationOnDefaultFont;

// atari
extern u8 c64SettingsAtariVideoSystem;
extern u8 c64SettingsAtariMachineType;
extern u8 c64SettingsAtariRamSizeOption;

extern bool c64SettingsRunVice;
extern bool c64SettingsRunAtari800;
extern bool c64SettingsRunNestopia;

// set setting
void C64DebuggerSetSetting(const char *name, void *value);
void C64DebuggerSetSettingInt(const char *settingName, int param);
void C64DebuggerSetSettingString(const char *settingName, CSlrString *param);

void C64DebuggerClearSettings();
void C64DebuggerStoreSettings();
void C64DebuggerRestoreSettings(uint8 settingsBlockType);
void C64DebuggerReadSettingsValues(CByteBuffer *byteBuffer, uint8 settingsBlockType);

void C64DebuggerReadSettingCustom(char *name, CByteBuffer *byteBuffer);


#endif
