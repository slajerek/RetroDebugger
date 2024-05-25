// TODO: it is high time to convert settings into JSON as the parser is finally available and tested
#include "EmulatorsConfig.h"
#include "C64SettingsStorage.h"
#include "SYS_Platform.h"
#include "CViewDataMap.h"
#include "CSlrFileFromOS.h"
#include "CByteBuffer.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CGuiTheme.h"
#include "CColorsTheme.h"
#include "CViewMonitorConsole.h"
#include "SND_SoundEngine.h"
#include "CViewC64Screen.h"
#include "CViewC64ScreenViewfinder.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64VicControl.h"
#include "CViewC64StateSID.h"
#include "CSnapshotsManager.h"
#include "C64Palette.h"
#include "CMainMenuBar.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CViewAtariScreen.h"
#include "CViewNesScreen.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "SYS_DefaultConfig.h"

#include "DebuggerDefs.h"

// this is used for temporarily allow quick switch for loading settings from other path than default
//#define DEBUG_SETTINGS_FILE_PATH	"/Users/mars/Downloads/settings.dat.old.bin"


extern "C" {
#include "sid-resources.h"
}

#define C64DEBUGGER_SETTINGS_FILE_VERSION 0x0002

///
#define C64DEBUGGER_SETTING_BLOCK	0
#define C64DEBUGGER_SETTING_STRING	1
#define C64DEBUGGER_SETTING_U8		2
#define C64DEBUGGER_SETTING_BOOL	3
#define C64DEBUGGER_SETTING_CUSTOM	4
#define C64DEBUGGER_SETTING_U16		5
#define C64DEBUGGER_SETTING_FLOAT	6
#define C64DEBUGGER_SETTING_I32		7

/// blocks
#define C64DEBUGGER_BLOCK_EOF			0

/// settings
int c64SettingsDefaultScreenLayoutId = -1; //SCREEN_LAYOUT_MONITOR_CONSOLE; //SCREEN_LAYOUT_C64_DEBUGGER;
//SCREEN_LAYOUT_C64_DEBUGGER);
//SCREEN_LAYOUT_C64_1541_MEMORY_MAP; //SCREEN_LAYOUT_C64_ONLY //
//SCREEN_LAYOUT_SHOW_STATES; //SCREEN_LAYOUT_C64_DATA_DUMP
//SCREEN_LAYOUT_C64_1541_DEBUGGER

u8 c64SettingsSelectEmulator = EMULATOR_TYPE_UNKNOWN;

bool c64SettingsRunVice = true;
bool c64SettingsRunAtari800 = false;
bool c64SettingsRunNestopia = false;

bool c64SettingsKeepSymbolsLabels = false;
bool c64SettingsKeepSymbolsWatches = true;
bool c64SettingsKeepSymbolsBreakpoints = false;

bool c64SettingsIsInVicEditor = false;

bool c64SettingsSkipConfig = false;
bool c64SettingsPassConfigToRunningInstance = false;

int c64SettingsScreenSupersampleFactor = 4;
bool c64SettingsEmulatorScreenBypassKeyboardShortcuts = true;

bool c64SettingsUsePipeIntegration = true;

bool c64SettingsWindowAlwaysOnTop = false;
CByteBuffer *c64SettingsWindowPosition = NULL;

bool c64SettingsExecuteAwareDisassembly = true;
bool c64SettingsPressCtrlToSetBreakpoint = false;

uint8 c64SettingsMemoryValuesStyle = MEMORY_MAP_VALUES_STYLE_RGB;
uint8 c64SettingsMemoryMarkersStyle = MEMORY_MAP_MARKER_STYLE_DEFAULT;
bool c64SettingsUseMultiTouchInMemoryMap = false;
bool c64SettingsMemoryMapInvertControl = false;
uint8 c64SettingsMemoryMapRefreshRate = 2;
int c64SettingsMemoryMapFadeSpeed = 100;		// percentage

uint8 c64SettingsC64Model = 0;
int c64SettingsEmulationMaximumSpeed = 100;		// percentage
bool c64SettingsFastBootKernalPatch = false;
bool c64SettingsEmulateVSPBug = false;
bool c64SettingsVicSkipDrawingSprites = false;
extern bool c64dSkipBogusPageOffsetReadOnSTA;

// TODO: refactor SID to Sid for better readability

uint8 c64SettingsSIDEngineModel = 0;	// 4=8580 FastSID
uint8 c64SettingsRESIDSamplingMethod = SID_RESID_SAMPLING_RESAMPLING;
bool c64SettingsRESIDEmulateFilters = true;
int c64SettingsRESIDPassBand = 90;
int c64SettingsRESIDFilterBias = 500;

uint8 c64SettingsSIDStereo = 0;					// "SidStereo" 0=none, 1=stereo, 2=triple
uint16 c64SettingsSIDStereoAddress = 0xD420;	// "SidStereoAddressStart"
uint16 c64SettingsSIDTripleAddress = 0xDF00;	// "SidTripleAddressStart"

extern u8 c64SettingsC64SidImportMode = SID_IMPORT_MODE_PSID64;

int c64SettingsDatasetteSpeedTuning = 0;
int c64SettingsDatasetteZeroGapDelay = 20000;
int c64SettingsDatasetteTapeWobble = 10;
bool c64SettingsDatasetteResetWithCPU = false;

//
bool c64SettingsReuEnabled = false;
int c64SettingsReuSize = 16384;

bool c64SettingsMuteSIDOnPause = false;

int c64SettingsViceAudioVolume = 100;				// percentage
bool c64SettingsRunSIDEmulation = true;
uint8 c64SettingsMuteSIDMode = MUTE_SID_MODE_ZERO_VOLUME;

int c64SettingsSidDataHistoryMaxSize = 6000;	// 120 seconds for PAL

uint16 c64SettingsVicPalette = 0;
bool c64SettingsC64ProfilerDoVicProfile = false;

int c64SettingsWaitOnStartup = 0; //500;

bool c64SettingsSnapshotsRecordIsActive = true;
// snapshots interval
int c64SettingsSnapshotsIntervalNumFrames = 10;
// max number of snapshots. TODO: get fps from debuginterface. 50 frames per second * 60 seconds = 3 mins
int c64SettingsSnapshotsLimit = (50 * 60 * 3) / 10;
u8 c64SettingsTimelineSaveZlibCompressionLevel = 1;	// Z_BEST_SPEED

CSlrString *c64SettingsPathToRomsC64 = NULL;
CSlrString *c64SettingsPathToRomC64Kernal = NULL;
CSlrString *c64SettingsPathToRomC64Basic = NULL;
CSlrString *c64SettingsPathToRomC64Chargen = NULL;
CSlrString *c64SettingsPathToRomC64Drive1541 = NULL;
CSlrString *c64SettingsPathToRomC64Drive1541ii = NULL;

CSlrString *c64SettingsPathToD64 = NULL;
CSlrString *c64SettingsDefaultD64Folder = NULL;

CSlrString *c64SettingsPathToPRG = NULL;
CSlrString *c64SettingsDefaultPRGFolder = NULL;

CSlrString *c64SettingsPathToCartridge = NULL;
CSlrString *c64SettingsDefaultCartridgeFolder = NULL;

CSlrString *c64SettingsPathToViceSnapshot = NULL;
CSlrString *c64SettingsPathToAtariSnapshot = NULL;
CSlrString *c64SettingsDefaultSnapshotsFolder = NULL;

CSlrString *c64SettingsPathToTAP = NULL;
CSlrString *c64SettingsDefaultTAPFolder = NULL;

CSlrString *c64SettingsPathToReu = NULL;
CSlrString *c64SettingsDefaultReuFolder = NULL;

CSlrString *c64SettingsDefaultVicEditorFolder = NULL;

bool c64SettingsAtariPokeyStereo = false;

CSlrString *c64SettingsPathToXEX = NULL;
CSlrString *c64SettingsDefaultXEXFolder = NULL;

CSlrString *c64SettingsPathToCAS = NULL;
CSlrString *c64SettingsDefaultCASFolder = NULL;

CSlrString *c64SettingsPathToAtariCartridge = NULL;
CSlrString *c64SettingsDefaultAtariCartridgeFolder = NULL;

CSlrString *c64SettingsPathToATR = NULL;
CSlrString *c64SettingsDefaultATRFolder = NULL;

CSlrString *c64SettingsPathToAtariROMs = NULL;

CSlrString *c64SettingsPathToNES = NULL;
CSlrString *c64SettingsDefaultNESFolder = NULL;

CSlrString *c64SettingsDefaultMemoryDumpFolder = NULL;

CSlrString *c64SettingsPathToC64MemoryMapFile = NULL;
CSlrString *c64SettingsPathToAtari800MemoryMapFile = NULL;

CSlrString *c64SettingsPathToSymbols = NULL;
CSlrString *c64SettingsPathToBreakpoints = NULL;
CSlrString *c64SettingsPathToWatches = NULL;
CSlrString *c64SettingsPathToDebugInfo = NULL;

bool c64SettingsUseNativeEmulatorMonitor = false;

CSlrString *c64SettingsPathToJukeboxPlaylist = NULL;

// priority
bool c64SettingsIsProcessPriorityBoostDisabled = true;
u8 c64SettingsProcessPriority = MT_PRIORITY_ABOVE_NORMAL;

// profiler
CSlrString *c64SettingsC64ProfilerFileOutputPath = NULL;

CSlrString *c64SettingsAudioOutDevice = NULL;
bool c64SettingsRestartAudioOnEmulationReset = false;

bool c64SettingsAlwaysUnpauseEmulationAfterReset = true;


float c64SettingsScreenGridLinesAlpha = 0.3f;
uint8 c64SettingsScreenGridLinesColorScheme = 0;	// 0=red, 1=green, 2=blue, 3=black, 4=dark gray 5=light gray 6=white 7=yellow
float c64SettingsScreenRasterViewfinderScale = 1.5f; //5.0f; //1.5f;	// TODO: remove c64SettingsScreenRasterViewfinderScale and add variable setting in CViewC64Screen

float c64SettingsScreenRasterCrossLinesAlpha = 0.5f;
uint8 c64SettingsScreenRasterCrossLinesColorScheme = 5;	// 0=red, 1=green, 2=blue, 3=black, 4=dark gray 5=light gray 6=white 7=yellow
float c64SettingsScreenRasterCrossAlpha = 0.85f;
uint8 c64SettingsScreenRasterCrossInteriorColorScheme = 6;	// 0=red, 1=green, 2=blue, 3=black, 4=dark gray 5=light gray 6=white 7=yellow
uint8 c64SettingsScreenRasterCrossExteriorColorScheme = 0;	// 0=red, 1=green, 2=blue, 3=black, 4=dark gray 5=light gray 6=white 7=yellow
uint8 c64SettingsScreenRasterCrossTipColorScheme = 3;	// 0=red, 1=green, 2=blue, 3=black, 4=dark gray 5=light gray 6=white 7=yellow

bool c64SettingsShowPositionsInHex = true;

uint8 c64SettingsVicStateRecordingMode = C64D_VICII_RECORD_MODE_EVERY_CYCLE;
bool c64SettingsRenderScreenNearest = true;

int c64SettingsJmpOnStartupAddr = -1;

u8 c64SettingsVicDisplayBorderType = VIC_DISPLAY_SHOW_BORDER_FULL;
bool c64SettingsVicDisplayShowGridLines = true;
bool c64SettingsVicDisplayShowBadLines = false;
bool c64SettingsVicDisplayApplyScroll = false;

bool c64SettingsAutoJmpAlwaysToLoadedPRGAddress = false;	// will jump to loaded address when PRG is loaded from menu
bool c64SettingsAutoJmpFromInsertedDiskFirstPrg = true;	// will load first PRG from attached disk
int c64SettingsAutoJmpDoReset = MACHINE_LOADPRG_RESET_MODE_LOAD_SNAPSHOT_BASIC;
int c64SettingsAutoJmpWaitAfterReset = 1300;				// this is to let c64 drive finish reset

bool c64SettingsForceUnpause = false;						// unpause debugger on jmp if code is stopped

bool c64SettingsResetCountersOnAutoRun = true;

bool c64SettingsRunSIDWhenInWarp = true;

//
u8 c64SettingsSelectedJoystick1 = 0;
u8 c64SettingsSelectedJoystick2 = 0;

//
float c64SettingsPaintGridCharactersColorR = 0.7f;
float c64SettingsPaintGridCharactersColorG = 0.7f;
float c64SettingsPaintGridCharactersColorB = 0.7f;
float c64SettingsPaintGridCharactersColorA = 1.0f;

float c64SettingsPaintGridPixelsColorR = 0.5f;
float c64SettingsPaintGridPixelsColorG = 0.5f;
float c64SettingsPaintGridPixelsColorB = 0.5f;
float c64SettingsPaintGridPixelsColorA = 0.3f;

float c64SettingsFocusBorderLineWidth = 0.7f;

int c64SettingsDisassemblyBackgroundColor = C64D_COLOR_BLACK;
int c64SettingsDisassemblyExecuteColor = C64D_COLOR_WHITE;
int c64SettingsDisassemblyNonExecuteColor = C64D_COLOR_LIGHT_GRAY;
bool c64SettingsDisassemblyUseNearLabels = true;
bool c64SettingsDisassemblyUseNearLabelsForJumps = false;
int c64SettingsDisassemblyNearLabelMaxOffset = 6;

int c64SettingsMenusColorTheme = 0;

bool c64SettingsVicEditorForceReplaceColor = true;
u8 c64SettingsVicEditorDefaultBackgroundColor = 0;

bool c64SettingsUseSystemFileDialogs = true;
bool c64SettingsUseOnlyFirstCPU = true;

int c64SettingsDoubleClickMS = 600;

// automatically load VICE labels if filename matched PRG (*.labels)
bool c64SettingsLoadViceLabels = true;

// automatically load watches if filename matched PRG (*.watch)
bool c64SettingsLoadWatches = true;

// automatically load debug info if filename matched PRG (*.dbg)
bool c64SettingsLoadDebugInfo = true;

bool c64SettingsInterpolationOnDefaultFont = true;

// atari
u8 c64SettingsAtariVideoSystem = ATARI_VIDEO_SYSTEM_PAL;
u8 c64SettingsAtariMachineType = 4;
u8 c64SettingsAtariRamSizeOption = 9;


void storeSettingBlock(CByteBuffer *byteBuffer, u8 value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_BLOCK);
	byteBuffer->PutU8(value);
}

void storeSettingU8(CByteBuffer *byteBuffer, char *name, u8 value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_U8);
	byteBuffer->PutString(name);
	byteBuffer->PutU8(value);
}

void storeSettingU16(CByteBuffer *byteBuffer, char *name, u16 value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_U16);
	byteBuffer->PutString(name);
	byteBuffer->PutU16(value);
}

void storeSettingI32(CByteBuffer *byteBuffer, char *name, i32 value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_I32);
	byteBuffer->PutString(name);
	byteBuffer->PutI32(value);
}

void storeSettingFloat(CByteBuffer *byteBuffer, char *name, float value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_FLOAT);
	byteBuffer->PutString(name);
	byteBuffer->PutFloat(value);
}

void storeSettingBool(CByteBuffer *byteBuffer, char *name, bool value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_BOOL);
	byteBuffer->PutString(name);
	byteBuffer->PutBool(value);
}

void storeSettingString(CByteBuffer *byteBuffer, char *name, CSlrString *value)
{
	byteBuffer->PutU8(C64DEBUGGER_SETTING_STRING);
	byteBuffer->PutString(name);
	byteBuffer->PutSlrString(value);
}

void storeSettingCustom(CByteBuffer *byteBuffer, char *name, CByteBuffer *value)
{
	if (value != NULL)
	{
		byteBuffer->PutU8(C64DEBUGGER_SETTING_CUSTOM);
		byteBuffer->PutString(name);
		byteBuffer->PutByteBuffer(value);
	}
}

void C64DebuggerClearSettings()
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutU16(C64DEBUGGER_SETTINGS_FILE_VERSION);

	storeSettingBlock(byteBuffer, C64DEBUGGER_BLOCK_EOF);
	
#if !defined(DEBUG_SETTINGS_FILE_PATH)
	CSlrString *fileName = new CSlrString(C64D_SETTINGS_FILE_PATH);
	byteBuffer->storeToSettings(fileName);
	delete fileName;
#else
	LOGWarning("C64DebuggerClearSettings: DEBUG_SETTINGS_FILE_PATH is defined as '%s'. NOT CLEARING SETTINGS.", DEBUG_SETTINGS_FILE_PATH);
#endif
	
	delete byteBuffer;
}

void C64DebuggerStoreSettings()
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutU16(C64DEBUGGER_SETTINGS_FILE_VERSION);
	
	storeSettingBlock(byteBuffer, C64DEBUGGER_BLOCK_PRELAUNCH);
	storeSettingString(byteBuffer, "FolderC64Roms", c64SettingsPathToRomsC64);
	storeSettingString(byteBuffer, "FolderC64Kernal", c64SettingsPathToRomC64Kernal);
	storeSettingString(byteBuffer, "FolderC64Basic", c64SettingsPathToRomC64Basic);
	storeSettingString(byteBuffer, "FolderC64Chargen", c64SettingsPathToRomC64Chargen);
	storeSettingString(byteBuffer, "FolderC64Drive1541", c64SettingsPathToRomC64Drive1541);
	storeSettingString(byteBuffer, "FolderC64Drive1541ii", c64SettingsPathToRomC64Drive1541ii);
	storeSettingString(byteBuffer, "FolderD64", c64SettingsDefaultD64Folder);
	storeSettingString(byteBuffer, "FolderPRG", c64SettingsDefaultPRGFolder);
	storeSettingString(byteBuffer, "FolderCRT", c64SettingsDefaultCartridgeFolder);
	storeSettingString(byteBuffer, "FolderTAP", c64SettingsDefaultTAPFolder);
	storeSettingString(byteBuffer, "FolderREU", c64SettingsDefaultReuFolder);
	storeSettingString(byteBuffer, "FolderVicEditor", c64SettingsDefaultVicEditorFolder);
	storeSettingString(byteBuffer, "FolderSnaps", c64SettingsDefaultSnapshotsFolder);
	storeSettingString(byteBuffer, "FolderMemDumps", c64SettingsDefaultMemoryDumpFolder);
	storeSettingString(byteBuffer, "PathD64", c64SettingsPathToD64);
	storeSettingString(byteBuffer, "PathPRG", c64SettingsPathToPRG);
	storeSettingString(byteBuffer, "PathCRT", c64SettingsPathToCartridge);
	storeSettingString(byteBuffer, "PathTAP", c64SettingsPathToTAP);
	storeSettingString(byteBuffer, "PathREU", c64SettingsPathToReu);

	storeSettingString(byteBuffer, "FolderAtariROMs", c64SettingsPathToAtariROMs);
	storeSettingString(byteBuffer, "FolderATR", c64SettingsDefaultATRFolder);
	storeSettingString(byteBuffer, "PathATR", c64SettingsPathToATR);
	storeSettingString(byteBuffer, "FolderXEX", c64SettingsDefaultXEXFolder);
	storeSettingString(byteBuffer, "PathXEX", c64SettingsPathToXEX);
	storeSettingString(byteBuffer, "FolderCAS", c64SettingsDefaultCASFolder);
	storeSettingString(byteBuffer, "PathCAS", c64SettingsPathToCAS);
	storeSettingString(byteBuffer, "FolderAtariCart", c64SettingsDefaultAtariCartridgeFolder);
	storeSettingString(byteBuffer, "PathAtariCart", c64SettingsPathToAtariCartridge);
	storeSettingString(byteBuffer, "FolderNES", c64SettingsDefaultNESFolder);
	storeSettingString(byteBuffer, "PathNES", c64SettingsPathToNES);

	storeSettingBool(byteBuffer, "UsePipeIntegration", c64SettingsUsePipeIntegration);
	
	storeSettingBool(byteBuffer, "AutoJmpAlwaysToLoadedPRGAddress", c64SettingsAutoJmpAlwaysToLoadedPRGAddress);
	storeSettingBool(byteBuffer, "AutoJmpFromInsertedDiskFirstPrg", c64SettingsAutoJmpFromInsertedDiskFirstPrg);

	storeSettingString(byteBuffer, "PathMemMapFile", c64SettingsPathToC64MemoryMapFile);

	storeSettingString(byteBuffer, "AudioOutDevice", c64SettingsAudioOutDevice);
	storeSettingBool(byteBuffer, "RestartAudioOnEmulationReset", c64SettingsRestartAudioOnEmulationReset);

	storeSettingBool(byteBuffer, "ShowPositionsInHex", c64SettingsShowPositionsInHex);

	storeSettingBool(byteBuffer, "FastBootPatch", c64SettingsFastBootKernalPatch);
	storeSettingBool(byteBuffer, "InterpolationOnDefaultFont", c64SettingsInterpolationOnDefaultFont);

	storeSettingU8(byteBuffer, "ScreenLayoutId", c64SettingsDefaultScreenLayoutId);
	storeSettingBool(byteBuffer, "IsInVicEditor", c64SettingsIsInVicEditor);
	
	storeSettingBool(byteBuffer, "DisassemblyExecuteAware", c64SettingsExecuteAwareDisassembly);
		
	storeSettingBool(byteBuffer, "WindowAlwaysOnTop", c64SettingsWindowAlwaysOnTop);

	storeSettingU16(byteBuffer, "ScreenSupersampleFactor", c64SettingsScreenSupersampleFactor);

	storeSettingBool(byteBuffer, "DisableProcessPriorityBoost", c64SettingsIsProcessPriorityBoostDisabled);
	storeSettingU8(byteBuffer, "ProcessPriority", c64SettingsProcessPriority);

	storeSettingBool(byteBuffer, "UseNativeEmulatorMonitor", c64SettingsUseNativeEmulatorMonitor);
	
	storeSettingBool(byteBuffer, "KeepSymbolsLabels", c64SettingsKeepSymbolsLabels);
	storeSettingBool(byteBuffer, "KeepSymbolsWatches", c64SettingsKeepSymbolsWatches);
	storeSettingBool(byteBuffer, "KeepSymbolsBreakpoints", c64SettingsKeepSymbolsBreakpoints);

	storeSettingBool(byteBuffer, "RunViceEmulation", c64SettingsRunVice);
	storeSettingBool(byteBuffer, "RunAtari800Emulation", c64SettingsRunAtari800);
	storeSettingBool(byteBuffer, "RunNestopiaEmulation", c64SettingsRunNestopia);

	
#if !defined(WIN32)
	storeSettingBool(byteBuffer, "UseSystemDialogs", c64SettingsUseSystemFileDialogs);
#endif

#if defined(WIN32)
	storeSettingBool(byteBuffer, "UseOnlyFirstCPU", c64SettingsUseOnlyFirstCPU);
#endif

#if defined(RUN_COMMODORE64)
	storeSettingU8(byteBuffer, "C64Model", c64SettingsC64Model);
	storeSettingString(byteBuffer, "C64ProfilerOutputPath", c64SettingsC64ProfilerFileOutputPath);
	storeSettingBool(byteBuffer, "C64ProfilerDoVic", c64SettingsC64ProfilerDoVicProfile);
#endif
	
	storeSettingCustom(byteBuffer, "WindowPosition", c64SettingsWindowPosition);
	
	storeSettingBlock(byteBuffer, C64DEBUGGER_BLOCK_POSTLAUNCH);
	storeSettingU8(byteBuffer, "MemoryValuesStyle", c64SettingsMemoryValuesStyle);
	storeSettingU8(byteBuffer, "MemoryMarkersStyle", c64SettingsMemoryMarkersStyle);

	storeSettingU16(byteBuffer, "ViceAudioVolume", c64SettingsViceAudioVolume);

#if defined(RUN_COMMODORE64)
	storeSettingU16(byteBuffer, "SIDStereoAddress", c64SettingsSIDStereoAddress);
	storeSettingU16(byteBuffer, "SIDTripleAddress", c64SettingsSIDTripleAddress);
	storeSettingU8(byteBuffer, "SIDStereo", c64SettingsSIDStereo);

	storeSettingU8(byteBuffer, "RESIDSamplingMethod", c64SettingsRESIDSamplingMethod);
	storeSettingBool(byteBuffer, "RESIDEmulateFilters", c64SettingsRESIDEmulateFilters);
	storeSettingI32(byteBuffer, "RESIDPassBand", c64SettingsRESIDPassBand);
	storeSettingI32(byteBuffer, "RESIDFilterBias", c64SettingsRESIDFilterBias);
	
	storeSettingU8(byteBuffer, "SIDEngineModel", c64SettingsSIDEngineModel);

	storeSettingBool(byteBuffer, "MuteSIDOnPause", c64SettingsMuteSIDOnPause);
	storeSettingBool(byteBuffer, "RunSIDWhenWarp", c64SettingsRunSIDWhenInWarp);

	storeSettingBool(byteBuffer, "RunSIDEmulation", c64SettingsRunSIDEmulation);
	storeSettingU8(byteBuffer, "MuteSIDMode", c64SettingsMuteSIDMode);

	storeSettingU8(byteBuffer, "SIDImportMode", c64SettingsC64SidImportMode);
	
	storeSettingI32(byteBuffer, "SIDDataHistoryMaxSize", c64SettingsSidDataHistoryMaxSize);

	storeSettingU8(byteBuffer, "VicStateRecording", c64SettingsVicStateRecordingMode);
	storeSettingU16(byteBuffer, "VicPalette", c64SettingsVicPalette);
	storeSettingBool(byteBuffer, "RenderScreenNearest", c64SettingsRenderScreenNearest);
	
	storeSettingU16(byteBuffer, "EmulationMaximumSpeed", c64SettingsEmulationMaximumSpeed);
	
	storeSettingBool(byteBuffer, "EmulateVSPBug", c64SettingsEmulateVSPBug);
	storeSettingBool(byteBuffer, "VicSkipDrawingSprites", c64SettingsVicSkipDrawingSprites);
	storeSettingBool(byteBuffer, "SkipBogusPageOffsetReadOnSTA", c64dSkipBogusPageOffsetReadOnSTA);
	
	
	storeSettingU8(byteBuffer, "VicDisplayBorder", c64SettingsVicDisplayBorderType);
	storeSettingBool(byteBuffer, "VicDisplayShowGrid", c64SettingsVicDisplayShowGridLines);
	storeSettingBool(byteBuffer, "VicDisplayShowBadLines", c64SettingsVicDisplayShowBadLines);
	storeSettingBool(byteBuffer, "VicDisplayApplyScroll", c64SettingsVicDisplayApplyScroll);
	
	storeSettingBool (byteBuffer, "VicEditorForceReplaceColor", c64SettingsVicEditorForceReplaceColor);
	storeSettingU8(byteBuffer, "VicEditorDefaultBackgroundColor", c64SettingsVicEditorDefaultBackgroundColor);
	
	storeSettingI32(byteBuffer, "DatasetteSpeedTuning", c64SettingsDatasetteSpeedTuning);
	storeSettingI32(byteBuffer, "DatasetteZeroGapDelay", c64SettingsDatasetteZeroGapDelay);
	storeSettingI32(byteBuffer, "DatasetteTapeWobble", c64SettingsDatasetteTapeWobble);
	storeSettingBool(byteBuffer, "DatasetteResetWithCPU", c64SettingsDatasetteResetWithCPU);
	
	storeSettingBool(byteBuffer, "ReuEnabled", c64SettingsReuEnabled);
	storeSettingI32(byteBuffer, "ReuSize", c64SettingsReuSize);
#endif
	
#if defined(RUN_ATARI)
	storeSettingU8(byteBuffer, "AtariVideoSystem", c64SettingsAtariVideoSystem);
	storeSettingU8(byteBuffer, "AtariMachineType", c64SettingsAtariMachineType);
	storeSettingU8(byteBuffer, "AtariRamSizeOption", c64SettingsAtariRamSizeOption);
	storeSettingBool(byteBuffer, "AtariPokeyStereo", c64SettingsAtariPokeyStereo);
#endif

	storeSettingBool(byteBuffer, "MemMapMultiTouch", c64SettingsUseMultiTouchInMemoryMap);
	storeSettingBool(byteBuffer, "MemMapInvert", c64SettingsMemoryMapInvertControl);
	storeSettingU8(byteBuffer, "MemMapRefresh", c64SettingsMemoryMapRefreshRate);
	storeSettingU16(byteBuffer, "MemMapFadeSpeed", c64SettingsMemoryMapFadeSpeed);

	storeSettingFloat(byteBuffer, "GridLinesAlpha", c64SettingsScreenGridLinesAlpha);
	storeSettingU8(byteBuffer, "GridLinesColor", c64SettingsScreenGridLinesColorScheme);
	storeSettingFloat(byteBuffer, "ViewfinderScale", c64SettingsScreenRasterViewfinderScale);
	storeSettingFloat(byteBuffer, "CrossLinesAlpha", c64SettingsScreenRasterCrossLinesAlpha);
	storeSettingU8(byteBuffer, "CrossLinesColor", c64SettingsScreenRasterCrossLinesColorScheme);
	storeSettingFloat(byteBuffer, "CrossAlpha", c64SettingsScreenRasterCrossAlpha);
	storeSettingU8(byteBuffer, "CrossInteriorColor", c64SettingsScreenRasterCrossInteriorColorScheme);
	storeSettingU8(byteBuffer, "CrossExteriorColor", c64SettingsScreenRasterCrossExteriorColorScheme);
	storeSettingU8(byteBuffer, "CrossTipColor", c64SettingsScreenRasterCrossTipColorScheme);
	
	storeSettingFloat(byteBuffer, "FocusBorderWidth", c64SettingsFocusBorderLineWidth);

	//
	storeSettingU8(byteBuffer, "SelectedJoystick1", c64SettingsSelectedJoystick1);
	storeSettingU8(byteBuffer, "SelectedJoystick2", c64SettingsSelectedJoystick2);

	//
	storeSettingU8(byteBuffer, "DisassemblyBackgroundColor", c64SettingsDisassemblyBackgroundColor);
	storeSettingU8(byteBuffer, "DisassemblyExecuteColor", c64SettingsDisassemblyExecuteColor);
	storeSettingU8(byteBuffer, "DisassemblyNonExecuteColor", c64SettingsDisassemblyNonExecuteColor);
	viewC64->config->SetBoolSkipConfigSave("DisassemblyUseNearLabels", &c64SettingsDisassemblyUseNearLabels);
	viewC64->config->SetBoolSkipConfigSave("DisassemblyUseNearLabelsForJumps", &c64SettingsDisassemblyUseNearLabelsForJumps);
	viewC64->config->SetIntSkipConfigSave("DisassemblyNearLabelMaxOffset", &c64SettingsDisassemblyNearLabelMaxOffset);

	storeSettingU8(byteBuffer, "MenusColorTheme", c64SettingsMenusColorTheme);
	
	//
	storeSettingU8(byteBuffer, "AutoJmpDoReset", c64SettingsAutoJmpDoReset);
	storeSettingI32(byteBuffer, "AutoJmpWaitAfterReset", c64SettingsAutoJmpWaitAfterReset);
	
	storeSettingFloat (byteBuffer, "PaintGridCharactersColorR", c64SettingsPaintGridCharactersColorR);
	storeSettingFloat (byteBuffer, "PaintGridCharactersColorG", c64SettingsPaintGridCharactersColorG);
	storeSettingFloat (byteBuffer, "PaintGridCharactersColorB", c64SettingsPaintGridCharactersColorB);
	storeSettingFloat (byteBuffer, "PaintGridCharactersColorA", c64SettingsPaintGridCharactersColorA);
	
	storeSettingFloat (byteBuffer, "PaintGridPixelsColorR", c64SettingsPaintGridPixelsColorR);
	storeSettingFloat (byteBuffer, "PaintGridPixelsColorG", c64SettingsPaintGridPixelsColorG);
	storeSettingFloat (byteBuffer, "PaintGridPixelsColorB", c64SettingsPaintGridPixelsColorB);
	storeSettingFloat (byteBuffer, "PaintGridPixelsColorA", c64SettingsPaintGridPixelsColorA);
		
	storeSettingBool(byteBuffer, "SnapshotsManagerIsActive", c64SettingsSnapshotsRecordIsActive);
	storeSettingI32(byteBuffer, "SnapshotsManagerStoreInterval", c64SettingsSnapshotsIntervalNumFrames);
	storeSettingI32(byteBuffer, "SnapshotsManagerLimit", c64SettingsSnapshotsLimit);
	storeSettingU8(byteBuffer, "SnapshotsManagerSaveZlibCompressionLevel", c64SettingsTimelineSaveZlibCompressionLevel);
	
	storeSettingBlock(byteBuffer, C64DEBUGGER_BLOCK_EOF);

	// new settings
	gApplicationDefaultConfig->SetBoolSkipConfigSave("DisassemblyPressCtrlToSetBreakpoint", &c64SettingsPressCtrlToSetBreakpoint);
	gApplicationDefaultConfig->SetBoolSkipConfigSave("AlwaysUnpauseEmulationAfterReset", &c64SettingsAlwaysUnpauseEmulationAfterReset);
	gApplicationDefaultConfig->SetBoolSkipConfigSave("EmulatorScreenBypassKeyboardShortcuts", &c64SettingsEmulatorScreenBypassKeyboardShortcuts);
	
#if !defined(DEBUG_SETTINGS_FILE_PATH)
	LOGD("C64D_SETTINGS_FILE_PATH is set to=%s", C64D_SETTINGS_FILE_PATH);
	CSlrString *fileName = new CSlrString(C64D_SETTINGS_FILE_PATH);
	fileName->DebugPrint("fileName=");
	byteBuffer->storeToSettings(fileName);
	delete fileName;
#else
	LOGWarning("DEBUG_SETTINGS_FILE_PATH is defined as '%s'. NOT STORING SETTINGS.", DEBUG_SETTINGS_FILE_PATH);
#endif
	
	delete byteBuffer;
	
	gApplicationDefaultConfig->SaveConfig();
}

/// Note: the old style settings are being deprecated, they will be replaced by viewC64->config
/// We have now a mixture of both, although the old settings will be gradually replaced with new config, thus the preferred method of storing and restoring setting now is by viewC64->config object
void C64DebuggerRestoreSettings(uint8 settingsBlockType)
{
	LOGD("C64DebuggerRestoreSettings: settingsBlockType=%d", settingsBlockType);
	
	if (c64SettingsSkipConfig)
	{
		LOGD("... skipping loading config and clearing settings");
		C64DebuggerClearSettings();
		return;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	
#if !defined(DEBUG_SETTINGS_FILE_PATH)
	CSlrString *fileName = new CSlrString(C64D_SETTINGS_FILE_PATH);
	byteBuffer->loadFromSettings(fileName);
	delete fileName;
#else
	LOGWarning("Restoring settings from DEBUG_SETTINGS_FILE_PATH defined as '%s'", DEBUG_SETTINGS_FILE_PATH);
	
	CSlrString *fileName = new CSlrString(DEBUG_SETTINGS_FILE_PATH);
	byteBuffer->readFromFile(fileName);
	delete fileName;
#endif
	
	
	if (byteBuffer->length == 0)
	{
		LOGD("... no stored settings found");
		delete byteBuffer;
		return;
	}
	
	u16 version = byteBuffer->GetU16();
	
	// TODO: remove me after post-launch. check old-style loadFromSettingsWithHeader
	if (version == 0x0000)
	{
		LOGWarning("C64DebuggerReadSettings: old settings, skip header. TODO: remove me");
		// header was 00 00 xx xx, skip next 2 bytes. version should be 00 02
		byteBuffer->GetU16();
		version = byteBuffer->GetU16();
	}
	
	if (version > C64DEBUGGER_SETTINGS_FILE_VERSION)
	{
		LOGError("C64DebuggerReadSettings: incompatible version %04x", version);
		delete byteBuffer;
		return;
	}
	
	C64DebuggerReadSettingsValues(byteBuffer, settingsBlockType);
	
	delete byteBuffer;
	
	// restore new style settings:
	gApplicationDefaultConfig->GetBool("DisassemblyPressCtrlToSetBreakpoint", &c64SettingsPressCtrlToSetBreakpoint, false);
	gApplicationDefaultConfig->GetBool("AlwaysUnpauseEmulationAfterReset", &c64SettingsAlwaysUnpauseEmulationAfterReset, true);
	gApplicationDefaultConfig->GetBool("EmulatorScreenBypassKeyboardShortcuts", &c64SettingsEmulatorScreenBypassKeyboardShortcuts, true);

	gApplicationDefaultConfig->GetBool("DisassemblyUseNearLabels", &c64SettingsDisassemblyUseNearLabels, true);
	gApplicationDefaultConfig->GetBool("DisassemblyUseNearLabelsForJumps", &c64SettingsDisassemblyUseNearLabelsForJumps, false);
	gApplicationDefaultConfig->GetInt("DisassemblyNearLabelMaxOffset", &c64SettingsDisassemblyNearLabelMaxOffset, 6);
}

void C64DebuggerReadSettingsValues(CByteBuffer *byteBuffer, uint8 settingsBlockType)
{
	LOGM("------------- C64DebuggerReadSettingsValues, blockType=%d", settingsBlockType);
	
	u8 blockType = 0xFF;
	
	int valueInt;
	bool valueBool;
	float valueFloat;
	
	// read block
	while (blockType != C64DEBUGGER_BLOCK_EOF)
	{
		u8 dataType = byteBuffer->GetU8();
	
		if (byteBuffer->error)
		{
			LOGError("C64DebuggerReadSettingsValues: Error reading settings");
			return;
		}

		if (dataType == C64DEBUGGER_SETTING_BLOCK)
		{
			blockType = byteBuffer->GetU8();
			continue;
		}
		
		char *name = byteBuffer->GetString();
		
		LOGD("read setting '%s'", name);
		
		void *value = NULL;
		
		if (dataType == C64DEBUGGER_SETTING_U8)
		{
			valueInt = byteBuffer->GetU8();
			value = &valueInt;
		}
		else if (dataType == C64DEBUGGER_SETTING_U16)
		{
			valueInt = byteBuffer->GetU16();
			value = &valueInt;
		}
		else if (dataType == C64DEBUGGER_SETTING_I32)
		{
			valueInt = byteBuffer->GetI32();
			value = &valueInt;
		}
		else if (dataType == C64DEBUGGER_SETTING_FLOAT)
		{
			valueFloat = byteBuffer->GetFloat();
			value = &valueFloat;
		}
		else if (dataType == C64DEBUGGER_SETTING_BOOL)
		{
			valueBool = byteBuffer->GetBool();
			value = &valueBool;
		}
		else if (dataType == C64DEBUGGER_SETTING_STRING)
		{
			value = byteBuffer->GetSlrString();
		}
		else
		{
			LOGError("C64DebuggerReadSettingsValues: unknown dataType=%d", dataType);
		}
		
		if (blockType == settingsBlockType)
		{
			if (dataType == C64DEBUGGER_SETTING_CUSTOM)
			{
				CByteBuffer *byteBufferCustom = byteBuffer->getByteBuffer();
				C64DebuggerReadSettingCustom(name, byteBufferCustom);
				delete byteBufferCustom;
			}
			else
			{
				if (value != NULL)
					C64DebuggerSetSetting(name, value);
			}
		}
		
		STRFREE(name);
		
		if (dataType == C64DEBUGGER_SETTING_STRING)
		{
			delete (CSlrString*)value;
		}
	}
}

void C64DebuggerReadSettingCustom(char *name, CByteBuffer *byteBuffer)
{
	if (!strcmp(name, "WindowPosition"))
	{
		if (byteBuffer->length > 0)
			c64SettingsWindowPosition = new CByteBuffer(byteBuffer);
	}
	
//	if (!strcmp(name, "MonitorHistory"))
//	{
//		int historySize = byteBuffer->GetByte();
//		for (int i = 0; i < historySize; i++)
//		{
//			char *cmd = byteBuffer->GetString();
//			viewC64->viewMonitorConsole->viewConsole->commandLineHistory.push_back(cmd);
//		}
//		viewC64->viewMonitorConsole->viewConsole->commandLineHistoryIt = viewC64->viewMonitorConsole->viewConsole->commandLineHistory.end();
//	}
}


void C64DebuggerSetSetting(const char *name, void *value)
{
	LOGD("C64DebuggerSetSetting: name='%s'", name);
	
	if (!strcmp(name, "RunViceEmulation"))
	{
		bool v = *((bool*)value);
		c64SettingsRunVice = v;
		return;
	}
	else if (!strcmp(name, "RunAtari800Emulation"))
	{
		bool v = *((bool*)value);
		c64SettingsRunAtari800 = v;
		return;
	}
	else if (!strcmp(name, "RunNestopiaEmulation"))
	{
		bool v = *((bool*)value);
		c64SettingsRunNestopia = v;
		return;
	}
	
	//
	else if (!strcmp(name, "KeepSymbolsLabels"))
	{
		bool v = *((bool*)value);
		c64SettingsKeepSymbolsLabels = v;
		return;
	}
	else if (!strcmp(name, "KeepSymbolsWatches"))
	{
		bool v = *((bool*)value);
		c64SettingsKeepSymbolsWatches = v;
		return;
	}
	else if (!strcmp(name, "KeepSymbolsBreakpoints"))
	{
		bool v = *((bool*)value);
		c64SettingsKeepSymbolsBreakpoints = v;
		return;
	}

	// C64
	else if (!strcmp(name, "FolderC64Roms"))
	{
		if (c64SettingsPathToRomsC64 != NULL)
			delete c64SettingsPathToRomsC64;
		
		c64SettingsPathToRomsC64 = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderC64Kernal"))
	{
		if (c64SettingsPathToRomC64Kernal != NULL)
			delete c64SettingsPathToRomC64Kernal;
		
		c64SettingsPathToRomC64Kernal = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderC64Basic"))
	{
		if (c64SettingsPathToRomC64Basic != NULL)
			delete c64SettingsPathToRomC64Basic;
		
		c64SettingsPathToRomC64Basic = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderC64Chargen"))
	{
		if (c64SettingsPathToRomC64Chargen != NULL)
			delete c64SettingsPathToRomC64Chargen;
		
		c64SettingsPathToRomC64Chargen = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderC64Drive1541"))
	{
		if (c64SettingsPathToRomC64Drive1541 != NULL)
			delete c64SettingsPathToRomC64Drive1541;
		
		c64SettingsPathToRomC64Drive1541 = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderC64Drive1541ii"))
	{
		if (c64SettingsPathToRomC64Drive1541ii != NULL)
			delete c64SettingsPathToRomC64Drive1541ii;
		
		c64SettingsPathToRomC64Drive1541ii = new CSlrString((CSlrString*)value);
		return;
	}

	else if (!strcmp(name, "FolderD64"))
	{
		if (c64SettingsDefaultD64Folder != NULL)
			delete c64SettingsDefaultD64Folder;
		
		c64SettingsDefaultD64Folder = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderPRG"))
	{
		if (c64SettingsDefaultPRGFolder != NULL)
			delete c64SettingsDefaultPRGFolder;
		
		c64SettingsDefaultPRGFolder = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderCRT"))
	{
		if (c64SettingsDefaultCartridgeFolder != NULL)
			delete c64SettingsDefaultCartridgeFolder;
		
		c64SettingsDefaultCartridgeFolder = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderTAP"))
	{
		if (c64SettingsDefaultTAPFolder != NULL)
			delete c64SettingsDefaultTAPFolder;
		
		c64SettingsDefaultTAPFolder = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderSnaps"))
	{
		if (c64SettingsDefaultSnapshotsFolder != NULL)
			delete c64SettingsDefaultSnapshotsFolder;
		
		c64SettingsDefaultSnapshotsFolder = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "FolderMemDumps"))
	{
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		
		c64SettingsDefaultMemoryDumpFolder = new CSlrString((CSlrString*)value);
		return;
	}
	
	else if (!strcmp(name, "PathD64"))
	{
		if (c64SettingsPathToD64 != NULL)
			delete c64SettingsPathToD64;
		
		c64SettingsPathToD64 = new CSlrString((CSlrString*)value);
		
		// the setting will be updated later by c64PerformStartupTasksThreaded
		if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
		{
			viewC64->viewC64MainMenu->InsertD64(c64SettingsPathToD64, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
		}
		return;
	}
	else if (!strcmp(name, "PathPRG"))
	{
		if (c64SettingsPathToPRG != NULL)
			delete c64SettingsPathToPRG;
		
		c64SettingsPathToPRG = new CSlrString((CSlrString*)value);
		c64SettingsPathToPRG->DebugPrint("PathPRG=");

		// the setting will be updated later by c64PerformStartupTasksThreaded
		if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
		{
			viewC64->viewC64MainMenu->LoadPRG(c64SettingsPathToPRG, false, false, true, false);
		}
		return;
	}
	else if (!strcmp(name, "PathTAP"))
	{
		if (c64SettingsPathToTAP != NULL)
			delete c64SettingsPathToTAP;
		
		c64SettingsPathToTAP = new CSlrString((CSlrString*)value);
		// the setting will be updated later by c64PerformStartupTasksThreaded
		if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
		{
			viewC64->viewC64MainMenu->LoadTape(c64SettingsPathToTAP, false, false, false);
		}
		return;
	}
	else if (!strcmp(name, "PathCRT"))
	{
		if (c64SettingsPathToCartridge != NULL)
			delete c64SettingsPathToCartridge;
		
		c64SettingsPathToCartridge = new CSlrString((CSlrString*)value);
		
		// the setting will be updated later by c64PerformStartupTasksThreaded
		if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
		{
			viewC64->viewC64MainMenu->InsertCartridge(c64SettingsPathToCartridge, false);
		}
		return;
	}
	else if (!strcmp(name, "ScreenSupersampleFactor"))
	{
		u16 v = *((u16*)value);
		c64SettingsScreenSupersampleFactor = v;
		
		if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
		{
			viewC64->viewC64Screen->SetSupersampleFactor(c64SettingsScreenSupersampleFactor);
		}
		if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
		{
			viewC64->viewAtariScreen->SetSupersampleFactor(c64SettingsScreenSupersampleFactor);
		}
		if (viewC64->debugInterfaceNes && viewC64->debugInterfaceNes->isRunning)
		{
			viewC64->viewNesScreen->SetSupersampleFactor(c64SettingsScreenSupersampleFactor);
		}
		return;
	}
	else if (!strcmp(name, "UsePipeIntegration"))
	{
		bool v = *((bool*)value);
		c64SettingsUsePipeIntegration = v;
		return;
	}
	else if (!strcmp(name, "AutoJmpFromInsertedDiskFirstPrg"))
	{
		bool v = *((bool*)value);
		c64SettingsAutoJmpFromInsertedDiskFirstPrg = v;
		return;
	}
	else if (!strcmp(name, "AutoJmpAlwaysToLoadedPRGAddress"))
	{
		bool v = *((bool*)value);
		c64SettingsAutoJmpAlwaysToLoadedPRGAddress = v;
		return;
	}
	
	else if (!strcmp(name, "SnapshotsManagerIsActive"))
	{
		bool v = *((bool*)value);
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			debugInterface->snapshotsManager->SetRecordingIsActive(v);
		}
		return;
	}

	else if (!strcmp(name, "SnapshotsManagerStoreInterval"))
	{
		i32 v = *((i32*)value);
		
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			debugInterface->snapshotsManager->SetRecordingStoreInterval(v);
		}
		return;
	}
	else if (!strcmp(name, "SnapshotsManagerLimit"))
	{
		i32 v = *((i32*)value);
		
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			debugInterface->snapshotsManager->SetRecordingLimit(v);
		}
		return;
	}
	else if (!strcmp(name, "SnapshotsManagerSaveZlibCompressionLevel"))
	{
		u8 v = *(u8*)value;
		c64SettingsTimelineSaveZlibCompressionLevel = v;
		return;
	}
	
	else if (!strcmp(name, "UseNativeEmulatorMonitor"))
	{
		bool v = *((bool*)value);
		c64SettingsUseNativeEmulatorMonitor = v;
		return;
	}
	
#if defined(WIN32)
	else if (!strcmp(name, "DisableProcessPriorityBoost"))
	{
		bool v = *((bool*)value);
		c64SettingsIsProcessPriorityBoostDisabled = v;
		SYS_SetMainProcessPriorityBoostDisabled(c64SettingsIsProcessPriorityBoostDisabled);
		return;
	}
	else if (!strcmp(name, "ProcessPriority"))
	{
		u8 v = *((u8*)value);
		c64SettingsProcessPriority = v;
		SYS_SetMainProcessPriority(c64SettingsProcessPriority);
		return;
	}
#endif
	
	
	if (!strcmp(name, "InterpolationOnDefaultFont"))
	{
		bool v = *((bool*)value);
		c64SettingsInterpolationOnDefaultFont = v;
		return;
	}

//	if (viewC64->debugInterfaceC64)
#if defined(RUN_COMMODORE64)
	{
		if (!strcmp(name, "FastBootPatch"))
		{
			bool v = *((bool*)value);
			c64SettingsFastBootKernalPatch = v;
			return;
		}
		else if (!strcmp(name, "IsInVicEditor"))
		{
			bool v = *((bool*)value);
			c64SettingsIsInVicEditor = v;
			return;
		}
		else if (!strcmp(name, "C64Model"))
		{
			int v = *((int*)value);
			c64SettingsC64Model = v;
			
			if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
			{
				viewC64->debugInterfaceC64->SetC64ModelType(c64SettingsC64Model);
			}
			return;
		}
		else if (!strcmp(name, "VicStateRecording"))
		{
			int v = *((int*)value);
			c64SettingsVicStateRecordingMode = v;
			viewC64->debugInterfaceC64->SetVicRecordStateMode(v);
			return;
		}
		else if (!strcmp(name, "VicPalette"))
		{
			int v = *((int*)value);
			c64SettingsVicPalette = v;
			C64SetPaletteNum(v);
			return;
		}

		else if (!strcmp(name, "SIDEngineModel"))
		{
			int v = *((int*)value);
			c64SettingsSIDEngineModel = v;
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
			{
				viewC64->debugInterfaceC64->SetSidType(c64SettingsSIDEngineModel);
			}
			return;
		}
		else if (!strcmp(name, "RESIDSamplingMethod"))
		{
			int v = *((int*)value);
			c64SettingsRESIDSamplingMethod = v;
			
			viewC64->debugInterfaceC64->SetSidSamplingMethod(c64SettingsRESIDSamplingMethod);
			return;
		}
		else if (!strcmp(name, "RESIDEmulateFilters"))
		{
			bool v = *((bool*)value);
			
			if (v)
			{
				c64SettingsRESIDEmulateFilters = true;
			}
			else
			{
				c64SettingsRESIDEmulateFilters = false;
			}
			
			viewC64->debugInterfaceC64->SetSidEmulateFilters(c64SettingsRESIDEmulateFilters);
			return;
		}
		else if (!strcmp(name, "RESIDPassBand"))
		{
			int v = *((int*)value);
			c64SettingsRESIDPassBand = v;
			
			viewC64->debugInterfaceC64->SetSidPassBand(c64SettingsRESIDPassBand);
			return;
		}
		else if (!strcmp(name, "RESIDFilterBias"))
		{
			int v = *((int*)value);
			c64SettingsRESIDFilterBias = v;
			
			viewC64->debugInterfaceC64->SetSidFilterBias(c64SettingsRESIDFilterBias);
			return;
		}
		else if (!strcmp(name, "SIDStereo"))
		{
			int v = *((int*)value);
			c64SettingsSIDStereo = v;
			
			viewC64->debugInterfaceC64->SetSidStereo(c64SettingsSIDStereo);
			viewC64->viewC64StateSID->UpdateSidButtonsState();
			return;
		}
		else if (!strcmp(name, "SIDStereoAddress"))
		{
			int v = *((int*)value);
			c64SettingsSIDStereoAddress = v;
			
			viewC64->debugInterfaceC64->SetSidStereoAddress(c64SettingsSIDStereoAddress);
			viewC64->viewC64StateSID->UpdateSidButtonsState();
			return;
		}
		else if (!strcmp(name, "SIDTripleAddress"))
		{
			int v = *((int*)value);
			c64SettingsSIDTripleAddress = v;
			
			viewC64->debugInterfaceC64->SetSidTripleAddress(c64SettingsSIDTripleAddress);
			viewC64->viewC64StateSID->UpdateSidButtonsState();
			return;
		}
		else if (!strcmp(name, "MuteSIDOnPause"))
		{
			bool v = *((bool*)value);
			
			if (v)
			{
				c64SettingsMuteSIDOnPause = true;
			}
			else
			{
				c64SettingsMuteSIDOnPause = false;
			}
			return;
		}
		else if (!strcmp(name, "RunSIDWhenWarp"))
		{
			bool v = *((bool*)value);
			
			if (v)
			{
				c64SettingsRunSIDWhenInWarp = true;
			}
			else
			{
				c64SettingsRunSIDWhenInWarp = false;
			}
			viewC64->debugInterfaceC64->SetRunSIDWhenInWarp(c64SettingsRunSIDWhenInWarp);
			return;
		}
		else if (!strcmp(name, "RunSIDEmulation"))
		{
			bool v = *((bool*)value);
			
			if (v)
			{
				c64SettingsRunSIDEmulation = true;
			}
			else
			{
				c64SettingsRunSIDEmulation = false;
			}
			viewC64->debugInterfaceC64->SetRunSIDEmulation(c64SettingsRunSIDEmulation);
			return;
		}
		else if (!strcmp(name, "MuteSIDMode"))
		{
			u8 v = *((u8*)value);
			c64SettingsMuteSIDMode = v;
			viewC64->UpdateSIDMute();
			return;
		}
		else if (!strcmp(name, "SIDImportMode"))
		{
			u8 v = *((u8*)value);
			c64SettingsC64SidImportMode = v;
			return;
		}
		else if (!strcmp(name, "SIDDataHistoryMaxSize"))
		{
			int v = *((int*)value);
			c64SettingsSidDataHistoryMaxSize = v;
			return;
		}
		
		else if (!strcmp(name, "EmulationMaximumSpeed"))
		{
			int v = *((int*)value);
			c64SettingsEmulationMaximumSpeed = v;
			
			viewC64->debugInterfaceC64->SetEmulationMaximumSpeed(v);
			return;
		}
		else if (!strcmp(name, "EmulateVSPBug"))
		{
			bool v = *((bool*)value);
			c64SettingsEmulateVSPBug = v;
			viewC64->debugInterfaceC64->SetVSPBugEmulation(c64SettingsEmulateVSPBug);
			return;
		}
		else if (!strcmp(name, "VicSkipDrawingSprites"))
		{
			bool v = *((bool*)value);
			c64SettingsVicSkipDrawingSprites = v;
			viewC64->debugInterfaceC64->SetSkipDrawingSprites(c64SettingsVicSkipDrawingSprites);
			return;
		}
		else if (!strcmp(name, "SkipBogusPageOffsetReadOnSTA"))
		{
			bool v = *((bool*)value);
			c64dSkipBogusPageOffsetReadOnSTA = v;
			return;
		}

		else if (!strcmp(name, "AutoJmpDoReset"))
		{
			int v = *((int*)value);
			c64SettingsAutoJmpDoReset = v;
			return;
		}
		else if (!strcmp(name, "AutoJmpWaitAfterReset"))
		{
			int v = *((int*)value);
			c64SettingsAutoJmpWaitAfterReset = v;
			return;
		}
		
		else if (!strcmp(name, "VicDisplayBorder"))
		{
			u8 v = *((u8*)value);
			c64SettingsVicDisplayBorderType = v;
			viewC64->viewC64VicControl->SetBorderType(c64SettingsVicDisplayBorderType);
			return;
		}
		else if (!strcmp(name, "VicDisplayShowGrid"))
		{
			u8 v = *((u8*)value);
			c64SettingsVicDisplayShowGridLines = v;
			viewC64->viewC64VicControl->SetGridLines(c64SettingsVicDisplayShowGridLines);
			return;
		}
		else if (!strcmp(name, "VicDisplayShowBadLines"))
		{
			u8 v = *((u8*)value);
			c64SettingsVicDisplayShowBadLines = v;
			viewC64->viewC64VicControl->ShowBadLines(c64SettingsVicDisplayShowBadLines);
			return;
		}
		else if (!strcmp(name, "VicDisplayApplyScroll"))
		{
			u8 v = *((u8*)value);
			c64SettingsVicDisplayApplyScroll = v;
			viewC64->viewC64VicControl->SetApplyScroll(c64SettingsVicDisplayApplyScroll);
			return;
		}
		
		else if (!strcmp(name, "VicEditorForceReplaceColor"))
		{
			bool v = *((bool*)value);
			c64SettingsVicEditorForceReplaceColor = v;
			return;
		}
		else if (!strcmp(name, "VicEditorDefaultBackgroundColor"))
		{
			u8 v = *((u8*)value);
			c64SettingsVicEditorDefaultBackgroundColor = v;
			return;
		}
		else if (!strcmp(name, "ViceAudioVolume"))
		{
			u16 v = *((u16*)value);
			c64SettingsViceAudioVolume = v;
			viewC64->debugInterfaceC64->SetAudioVolume((float)(c64SettingsViceAudioVolume) / 100.0f);
			return;
		}

		//
		else if (!strcmp(name, "CrossLinesAlpha"))
		{
			float v = *((float*)value);
			c64SettingsScreenRasterCrossLinesAlpha = v;
			
			if (viewC64->debugInterfaceC64)
			{
				viewC64->viewC64Screen->InitRasterColorsFromScheme();
			}
			return;
		}
		else if (!strcmp(name, "CrossLinesColor"))
		{
			u8 v = *((u8*)value);
			c64SettingsScreenRasterCrossLinesColorScheme = v;
			
			if (viewC64->debugInterfaceC64)
			{
				viewC64->viewC64Screen->InitRasterColorsFromScheme();
				
			}
			return;
		}
		else if (!strcmp(name, "CrossAlpha"))
		{
			float v = *((float*)value);
			c64SettingsScreenRasterCrossAlpha = v;
			if (viewC64->debugInterfaceC64)
			{
				viewC64->viewC64Screen->InitRasterColorsFromScheme();
			}
			return;
		}
		else if (!strcmp(name, "CrossInteriorColor"))
		{
			u8 v = *((u8*)value);
			c64SettingsScreenRasterCrossInteriorColorScheme = v;
			
			if (viewC64->debugInterfaceC64)
			{
				viewC64->viewC64Screen->InitRasterColorsFromScheme();
			}
			return;
		}
		else if (!strcmp(name, "CrossExteriorColor"))
		{
			u8 v = *((u8*)value);
			c64SettingsScreenRasterCrossExteriorColorScheme = v;
			
			if (viewC64->debugInterfaceC64)
			{
				viewC64->viewC64Screen->InitRasterColorsFromScheme();
			}
			return;
		}
		else if (!strcmp(name, "CrossTipColor"))
		{
			u8 v = *((u8*)value);
			c64SettingsScreenRasterCrossTipColorScheme = v;
			
			if (viewC64->debugInterfaceC64)
			{
				viewC64->viewC64Screen->InitRasterColorsFromScheme();
			}
			return;
		}
		
		//
		else if (!strcmp(name, "DatasetteSpeedTuning"))
		{
			int v = *((int*)value);
			c64SettingsDatasetteSpeedTuning = v;
			
			viewC64->debugInterfaceC64->DatasetteSetSpeedTuning(v);
			return;
		}
		else if (!strcmp(name, "DatasetteZeroGapDelay"))
		{
			int v = *((int*)value);
			c64SettingsDatasetteZeroGapDelay = v;

			viewC64->debugInterfaceC64->DatasetteSetZeroGapDelay(v);
			return;
		}
		else if (!strcmp(name, "DatasetteTapeWobble"))
		{
			int v = *((int*)value);
			c64SettingsDatasetteTapeWobble = v;

			viewC64->debugInterfaceC64->DatasetteSetTapeWobble(v);
			return;
		}
		else if (!strcmp(name, "DatasetteResetWithCPU"))
		{
			bool v = *((bool*)value);
			c64SettingsDatasetteResetWithCPU = v;

			viewC64->debugInterfaceC64->DatasetteSetResetWithCPU(v);
			return;
		}
		else if (!strcmp(name, "ReuEnabled"))
		{
			bool v = *((bool*)value);
			c64SettingsReuEnabled = v;
			
			LOGD("c64SettingsReuEnabled=%s", STRBOOL(c64SettingsReuEnabled));
			viewC64->debugInterfaceC64->SetReuEnabled(v);
			return;
		}
		else if (!strcmp(name, "ReuSize"))
		{
			int v = *((int*)value);
			c64SettingsReuSize = v;
			viewC64->debugInterfaceC64->SetReuSize(v);
			return;
		}
		else if (!strcmp(name, "C64ProfilerOutputPath"))
		{
			if (c64SettingsC64ProfilerFileOutputPath != NULL)
			{
				delete c64SettingsC64ProfilerFileOutputPath;
			}
			
			c64SettingsC64ProfilerFileOutputPath = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "C64ProfilerDoVic"))
		{
			bool v = *((bool*)value);
			c64SettingsC64ProfilerDoVicProfile = v;
			return;
		}
	}
#endif
	
#if defined(RUN_ATARI)
	{
		/*
		if (!strcmp(name, "AtariPokeyStereo"))
		{
			bool v = *((bool*)value);
			c64SettingsAtariPokeyStereo = v;
			debugInterfaceAtari->SetPokeyStereo(v);
			return;
		}
		else
		 */
		
		
		 if (!strcmp(name, "FolderAtariROMs"))
		{
			if (c64SettingsPathToAtariROMs != NULL)
				delete c64SettingsPathToAtariROMs;
			
			c64SettingsPathToAtariROMs = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "FolderATR"))
		{
			if (c64SettingsDefaultATRFolder != NULL)
				delete c64SettingsDefaultATRFolder;
			
			c64SettingsDefaultATRFolder = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "PathATR"))
		{
			if (c64SettingsPathToATR != NULL)
				delete c64SettingsPathToATR;
			
			c64SettingsPathToATR = new CSlrString((CSlrString*)value);
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->viewC64MainMenu->InsertATR(c64SettingsPathToATR, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
			}
			return;
		}
		else if (!strcmp(name, "FolderXEX"))
		{
			if (c64SettingsDefaultXEXFolder != NULL)
				delete c64SettingsDefaultXEXFolder;
			
			c64SettingsDefaultXEXFolder = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "PathXEX"))
		{
			if (c64SettingsPathToXEX != NULL)
				delete c64SettingsPathToXEX;
			
			c64SettingsPathToXEX = new CSlrString((CSlrString*)value);
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->viewC64MainMenu->LoadXEX(c64SettingsPathToXEX, false, false, true);
			}
			return;
		}
		
		else if (!strcmp(name, "FolderCAS"))
		{
			if (c64SettingsDefaultCASFolder != NULL)
				delete c64SettingsDefaultCASFolder;
			
			c64SettingsDefaultCASFolder = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "PathCAS"))
		{
			if (c64SettingsPathToCAS != NULL)
				delete c64SettingsPathToCAS;
			
			c64SettingsPathToCAS = new CSlrString((CSlrString*)value);
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->viewC64MainMenu->LoadCAS(c64SettingsPathToCAS, false, false, true);
			}
			return;
		}

		else if (!strcmp(name, "FolderAtariCart"))
		{
			if (c64SettingsDefaultAtariCartridgeFolder != NULL)
				delete c64SettingsDefaultAtariCartridgeFolder;
			
			c64SettingsDefaultAtariCartridgeFolder = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "PathAtariCart"))
		{
			if (c64SettingsPathToAtariCartridge != NULL)
				delete c64SettingsPathToAtariCartridge;
			
			c64SettingsPathToAtariCartridge = new CSlrString((CSlrString*)value);
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->viewC64MainMenu->InsertAtariCartridge(c64SettingsPathToAtariCartridge, false, false, true);
			}
			return;
		}

		
		else if (!strcmp(name, "AtariVideoSystem"))
		{
			u8 v = *((u8*)value);
			c64SettingsAtariVideoSystem = v;
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->debugInterfaceAtari->SetVideoSystem(v);
			}
			return;
		}
		else if (!strcmp(name, "AtariMachineType"))
		{
			u8 v = *((u8*)value);
			c64SettingsAtariMachineType = v;
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->debugInterfaceAtari->SetMachineType(v);
			}
			return;
		}

		else if (!strcmp(name, "AtariRamSizeOption"))
		{
			u8 v = *((u8*)value);
			c64SettingsAtariRamSizeOption = v;
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceAtari && viewC64->debugInterfaceAtari->isRunning)
			{
				viewC64->debugInterfaceAtari->SetRamSizeOption(v);
			}
			return;
		}

	}
#endif
	
#if defined(RUN_NES)
	{
		if (!strcmp(name, "FolderNES"))
		{
			if (c64SettingsDefaultNESFolder != NULL)
				delete c64SettingsDefaultNESFolder;
			
			c64SettingsDefaultNESFolder = new CSlrString((CSlrString*)value);
			return;
		}
		else if (!strcmp(name, "PathNES"))
		{
			if (c64SettingsPathToNES != NULL)
				delete c64SettingsPathToNES;
			
			c64SettingsPathToNES = new CSlrString((CSlrString*)value);
			
			// the setting will be updated later by c64PerformStartupTasksThreaded
			if (viewC64->debugInterfaceNes && viewC64->debugInterfaceNes->isRunning)
			{
				viewC64->viewC64MainMenu->LoadNES(c64SettingsPathToNES, false);
			}
			return;
		}

	}
#endif

	if (!strcmp(name, "PathMemMapFile"))
	{
		if (c64SettingsPathToC64MemoryMapFile != NULL)
			delete c64SettingsPathToC64MemoryMapFile;
		
		c64SettingsPathToC64MemoryMapFile = new CSlrString((CSlrString*)value);
		return;
	}
	else if (!strcmp(name, "DisassemblyExecuteAware"))
	{
		bool v = *((bool*)value);
		c64SettingsExecuteAwareDisassembly = v;
		return;
	}
	else if (!strcmp(name, "WindowAlwaysOnTop"))
	{
		bool v = *((bool*)value);
		c64SettingsWindowAlwaysOnTop = v;
		guiMain->SetApplicationWindowAlwaysOnTop(c64SettingsWindowAlwaysOnTop);
		return;
	}
	
#if !defined(WIN32)
	else if (!strcmp(name, "UseSystemDialogs"))
	{
		bool v = *((bool*)value);
		c64SettingsUseSystemFileDialogs = v;
		return;
	}
#endif
	
#if defined(WIN32)
	else if (!strcmp(name, "UseOnlyFirstCPU"))
	{
		bool v = *((bool*)value);
		c64SettingsUseOnlyFirstCPU = v;
		return;
	}
#endif


	
	else if (!strcmp(name, "ScreenLayoutId"))
	{
		int v = *((int*)value);
		c64SettingsDefaultScreenLayoutId = v;
		return;
	}
	
	else if (!strcmp(name, "MemoryValuesStyle"))
	{
		int v = *((int*)value);
		c64SettingsMemoryValuesStyle = v;
		C64DebuggerComputeMemoryMapColorTables(v);
		return;
	}
	else if (!strcmp(name, "MemoryMarkersStyle"))
	{
		int v = *((int*)value);
		c64SettingsMemoryMarkersStyle = v;
		C64DebuggerSetMemoryMapMarkersStyle(v);
		return;
	}
	else if (!strcmp(name, "AudioOutDevice"))
	{
		if (c64SettingsAudioOutDevice != NULL)
			delete c64SettingsAudioOutDevice;

		c64SettingsAudioOutDevice = new CSlrString((CSlrString*)value);
		char *cDeviceName = c64SettingsAudioOutDevice->GetStdASCII();
		gSoundEngine->SetOutputAudioDevice(cDeviceName);
		delete [] cDeviceName;
		return;
	}
	else if (!strcmp(name, "RestartAudioOnEmulationReset"))
	{
		bool v = *((bool*)value);
		c64SettingsRestartAudioOnEmulationReset = v;
		return;
	}
	else if (!strcmp(name, "ShowPositionsInHex"))
	{
		bool v = *((bool*)value);
		c64SettingsShowPositionsInHex = v;
		return;
	}

	
	else if (!strcmp(name, "RenderScreenNearest"))
	{
		bool v = *((bool*)value);
		
		if (v)
		{
			c64SettingsRenderScreenNearest = true;
		}
		else
		{
			c64SettingsRenderScreenNearest = false;
		}
		return;
	}
	
	else if (!strcmp(name, "MemMapMultiTouch"))
	{
#if defined(MACOS)
		bool v = *((bool*)value);
		
		if (v)
		{
			c64SettingsUseMultiTouchInMemoryMap = true;
		}
		else
		{
			c64SettingsUseMultiTouchInMemoryMap = false;
		}
		return;
#endif
	}
	else if (!strcmp(name, "MemMapInvert"))
	{
		bool v = *((bool*)value);
		
		if (v)
		{
			c64SettingsMemoryMapInvertControl = true;
		}
		else
		{
			c64SettingsMemoryMapInvertControl = false;
		}
		return;
	}
	else if (!strcmp(name, "MemMapRefresh"))
	{
		int v = *((int*)value);
		c64SettingsMemoryMapRefreshRate = v;
		return;
	}
	else if (!strcmp(name, "MemMapFadeSpeed"))
	{
		int v = *((int*)value);
		c64SettingsMemoryMapFadeSpeed = v;
		
		float fadeSpeed = v / 100.0f;
		C64DebuggerSetMemoryMapCellsFadeSpeed(fadeSpeed);
		return;
	}
	else if (!strcmp(name, "GridLinesAlpha"))
	{
		float v = *((float*)value);
		c64SettingsScreenGridLinesAlpha = v;
		if (viewC64->debugInterfaceC64)
		{
			viewC64->viewC64Screen->InitRasterColorsFromScheme();
		}
		return;
	}
	else if (!strcmp(name, "GridLinesColor"))
	{
		u8 v = *((u8*)value);
		c64SettingsScreenGridLinesColorScheme = v;
		if (viewC64->debugInterfaceC64)
		{
			viewC64->viewC64Screen->InitRasterColorsFromScheme();
			viewC64->viewC64VicDisplay->InitRasterColorsFromScheme();
		}
		return;
	}
	else if (!strcmp(name, "ViewfinderScale"))
	{
		float v = *((float*)value);
		c64SettingsScreenRasterViewfinderScale = v;
		if (viewC64->debugInterfaceC64)
		{
			viewC64->viewC64ScreenViewfinder->SetZoomedScreenLevel(v);
		}
		return;
	}
	else if (!strcmp(name, "FocusBorderWidth"))
	{
		float v = *((float*)value);
		c64SettingsFocusBorderLineWidth = v;
		guiMain->theme->focusBorderLineWidth = v;
		return;
	}
	else if (!strcmp(name, "SelectedJoystick1"))
	{
		u8 v = *((u8*)value);
		c64SettingsSelectedJoystick1 = v;
		viewC64->mainMenuBar->selectedJoystick1 = v;
		return;
	}
	else if (!strcmp(name, "SelectedJoystick2"))
	{
		u8 v = *((u8*)value);
		c64SettingsSelectedJoystick2 = v;
		viewC64->mainMenuBar->selectedJoystick2 = v;
		return;
	}
	else if (!strcmp(name, "DisassemblyBackgroundColor"))
	{
		u8 v = *((u8*)value);
		c64SettingsDisassemblyBackgroundColor = v;
		return;
	}
	else if (!strcmp(name, "DisassemblyExecuteColor"))
	{
		u8 v = *((u8*)value);
		c64SettingsDisassemblyExecuteColor = v;
		return;
	}
	else if (!strcmp(name, "DisassemblyNonExecuteColor"))
	{
		u8 v = *((u8*)value);
		c64SettingsDisassemblyNonExecuteColor = v;
		return;
	}
	else if (!strcmp(name, "MenusColorTheme"))
	{
		u8 v = *((u8*)value);
		c64SettingsMenusColorTheme = v;
		viewC64->colorsTheme->InitColors(c64SettingsMenusColorTheme);
		return;
	}
	else if (!strcmp(name, "PaintGridCharactersColorR"))
	{
		LOGTODO("C64SettingsStorage: PaintGridCharactersColorR");
//		float v = *((float*)value);
//		c64SettingsPaintGridCharactersColorR = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridCharactersColorG"))
	{
		LOGTODO("C64SettingsStorage: PaintGridCharactersColorG");
//		float v = *((float*)value);
//		c64SettingsPaintGridCharactersColorG = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridCharactersColorB"))
	{
		LOGTODO("C64SettingsStorage: PaintGridCharactersColorB");
//		float v = *((float*)value);
//		c64SettingsPaintGridCharactersColorB = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridCharactersColorA"))
	{
		LOGTODO("C64SettingsStorage: PaintGridCharactersColorA");
//		float v = *((float*)value);
//		c64SettingsPaintGridCharactersColorA = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridPixelsColorR"))
	{
		LOGTODO("C64SettingsStorage: PaintGridPixelsColor");
//		float v = *((float*)value);
//		c64SettingsPaintGridPixelsColorR = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridPixelsColorG"))
	{
		LOGTODO("C64SettingsStorage: PaintGridPixelsColor");
//		float v = *((float*)value);
//		c64SettingsPaintGridPixelsColorG = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridPixelsColorB"))
	{
		LOGTODO("C64SettingsStorage: PaintGridPixelsColor");
//		float v = *((float*)value);
//		c64SettingsPaintGridPixelsColorB = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	else if (!strcmp(name, "PaintGridPixelsColorA"))
	{
		LOGTODO("C64SettingsStorage: PaintGridPixelsColor");
//		float v = *((float*)value);
//		c64SettingsPaintGridPixelsColorA = v;
//		if (viewC64->debugInterfaceC64)
//		{
//			viewC64->screenVicEditor->InitPaintGridColors();
//		}
		return;
	}
	
	LOGError("C64DebuggerSetSetting: unknown setting '%s'", name);
}

//

