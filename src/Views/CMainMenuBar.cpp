#include "GUI_Main.h"
#include "SYS_DefaultConfig.h"
#include "EmulatorsConfig.h"
#include "CGuiViewDebugLog.h"
#include "VID_Fonts.h"
#include "CViewC64.h"
#include "CMainMenuBar.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "C64SettingsStorage.h"
#include "CViewKeyboardShortcuts.h"
#include "CSlrKeyboardShortcuts.h"
#include "C64KeyboardShortcuts.h"
#include "CSnapshotsManager.h"
#include "CDebugEventsHistory.h"
#include "CGuiView.h"
#include "SND_SoundEngine.h"
#include "GAM_GamePads.h"
#include "CViewC64Screen.h"
#include "CViewAtariScreen.h"
#include "CViewNesScreen.h"
#include "CViewC64Palette.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugAsmSource.h"
#include "CViewDrive1541Browser.h"
#include "C64Palette.h"
#include "CViewC64StateSID.h"
#include "CViewDataDump.h"
#include "CViewSnapshots.h"
#include "CViewTimeline.h"
#include "CViewAudioMixer.h"
#include "CLayoutManager.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "C64DebuggerPluginCrtMaker.h"
#include "C64DebuggerPluginDNDK.h"
#include "CViewMemoryMap.h"
#include "CViewMonitorConsole.h"
#include "CGuiViewMessages.h"
#include "CGuiViewUiDebug.h"
#include "CMidiInKeyboard.h"
#include "CDebugInterfaceMenuItem.h"
#include "CViewTimeline.h"
#include "CViewC64Charset.h"
#include "CViewC64VicEditor.h"
#include "CViewC64KeyMap.h"
#include "CConfigStorageHjson.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"
#include "implot_internal.h"

extern bool c64dSkipBogusPageOffsetReadOnSTA;

#include <map>

void PLUGIN_GoatTrackerSetVisible(bool isVisible);

CMainMenuBar::CMainMenuBar()
{
	selectedDebugInterface = NULL;
	layoutData = NULL;
	
	audioDevices = NULL;
	gamepads = NULL;
	waitingForNewLayoutKeyShortcut = false;

	extensionsImportLabels.push_back(new CSlrString("vs"));
	extensionsImportLabels.push_back(new CSlrString("dbg"));
	extensionsImportLabels.push_back(new CSlrString("lbl"));
	extensionsImportLabels.push_back(new CSlrString("labels"));
	extensionsExportLabels.push_back(new CSlrString("labels"));
	extensionsREU.push_back(new CSlrString("reu"));

	extensionsWatches.push_back(new CSlrString("watches"));
	extensionsBreakpoints.push_back(new CSlrString("breakpoints"));

	extensionsMemory.push_back(new CSlrString("bin"));
	extensionsCSV.push_back(new CSlrString("csv"));
	extensionsProfiler.push_back(new CSlrString("pd"));
	extensionsTimeline.push_back(new CSlrString("rtdl"));
	extensionsTimeline.push_back(new CSlrString("rdtl"));

	// this is Search view
	viewSearchWindow = new CGuiViewSearch("Open Window", 200, 150, -1, 400, 200, this);
	viewSearchWindow->hideWindowOnFocusLost = true;
	viewSearchWindow->SetVisible(false);
	
	// move me to c64 tools/interface
	CreateSidAddressOptions();
	
	// TODO: fixme, enum added, but we need to fix the problem that in code we have a check for gamepads joysticknum -2, do normal generic controller api so keyboard joystick controller is just a pointer
	selectedJoystick1 = 0;
	selectedJoystick2 = 0;
	
	c64ModelTypeNames = new std::vector<const char *>();
	c64ModelTypeIds = new std::vector<int>();
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->GetSidTypes(&sidTypes);
		viewC64->debugInterfaceC64->GetC64ModelTypes(c64ModelTypeNames, c64ModelTypeIds);
	}
	
	// setup keyboard shortcuts
	
#if defined(MACOS)
	kbsQuitApplication = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Quit application", 'q', false, false, true, false, this);
#elif defined(LINUX)
	kbsQuitApplication = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Quit application", MTKEY_F4, false, true, false, false, this);
#elif defined(WIN32)
	kbsQuitApplication = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Quit application", MTKEY_F4, false, true, false, false, this);
#endif
	guiMain->AddKeyboardShortcut(kbsQuitApplication);
	
	kbsCloseWindow = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Close window", 'w', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsCloseWindow);

	//
	kbsSearchWindow = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Open Window", 'o', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSearchWindow);

	//
	kbsResetCpuCycleAndFrameCounters = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Reset emulation counters", MTKEY_BACKSPACE, true, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsResetCpuCycleAndFrameCounters);

	//
	kbsClearMemoryMarkers = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Clear Memory markers", MTKEY_BACKSPACE, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsClearMemoryMarkers);

	//
	kbsDetachEverything = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Everything", 'd', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachEverything);
	
	kbsDetachDiskImage = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Disk Image", '8', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachDiskImage);
	
	kbsDetachCartridge = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Cartridge", '0', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachCartridge);

	kbsDetachExecutable = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Executable", '-', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachExecutable);


#if defined(RUN_COMMODORE64)
	
	if (viewC64->debugInterfaceC64)
	{
		kbsAutoJmpFromInsertedDiskFirstPrg = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Auto load first PRG from D64", 'a', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsAutoJmpFromInsertedDiskFirstPrg);

		kbsAutoJmpAlwaysToLoadedPRGAddress = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Always JMP to loaded addr", 'j', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsAutoJmpAlwaysToLoadedPRGAddress);

		kbsAutoJmpDoReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Reset C64 before PRG load", 'h', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsAutoJmpDoReset);


//		kbsInsertD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Device #8", '8', false, false, true, false, this);
//		guiMain->AddKeyboardShortcut(kbsInsertD64);
		
		// disk
		kbsInsertNextD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert next disk to Device #8", '8', false, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsInsertNextD64);

		kbsStartFromDisk = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Start from Device #8", MTKEY_F3, false, false, false, false, this);
		guiMain->AddKeyboardShortcut(kbsStartFromDisk);
		
		kbsBrowseD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Browse Device #8", MTKEY_F7, false, false, false, false, this);
		guiMain->AddKeyboardShortcut(kbsBrowseD64);
		
		// cartridge
		kbsCartridgeFreezeButton = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Cartridge freeze", 'f', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsCartridgeFreezeButton);

		// tape
		kbsTapeAttach = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Attach", 't', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeAttach);
		
		kbsTapeDetach = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Detach", 't', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeDetach);
		
		kbsTapeStop = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Stop", 's', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeStop);

		kbsTapePlay = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Play", 'p', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapePlay);

		kbsTapeForward = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Forward", 'f', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeForward);

		kbsTapeRewind = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Rewind", 'r', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeRewind);

		kbsTapeRecord = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Record", 'y', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeRecord);

		//
		kbsSwitchNextMaximumSpeed = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Next maximum speed", ']', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsSwitchNextMaximumSpeed);
		kbsSwitchPrevMaximumSpeed = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Previous maximum speed", '[', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsSwitchPrevMaximumSpeed);
		
		kbsDumpC64Memory = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Dump C64 memory", 'u', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsDumpC64Memory);
		
		kbsDumpDrive1541Memory = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Dump Drive 1541 memory", 'u', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsDumpDrive1541Memory);

		kbsC64ProfilerStartStop = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Start/Stop profiling C64", 'i', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsC64ProfilerStartStop);
	}
#endif

#if defined(RUN_ATARI)
//	if (viewC64->debugInterfaceAtari)
//	{
//		kbsInsertATR = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Disk", '8', false, false, true, false, this);
//		guiMain->AddKeyboardShortcut(kbsInsertATR);
//	}
#endif

	//
	
	kbsOpenFile = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Load", 'o', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsOpenFile);
		
	kbsReloadAndRestart = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Open most recent", 'l', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsReloadAndRestart);
	
	kbsSoftReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Soft Reset", 'r', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSoftReset);

	kbsHardReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Hard Reset", 'r', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsHardReset);

	//
	kbsIsWarpSpeed = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Warp speed", 'p', false, false, true, false, this);
	isWarpSpeed = false;
	guiMain->AddKeyboardShortcut(kbsIsWarpSpeed);

	if (viewC64->debugInterfaceC64)
	{
		kbsDiskDriveReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Disk Drive Reset", 'r', false, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsDiskDriveReset);

//		kbsInsertCartridge = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Cartridge", '0', false, false, false, true);
//		guiMain->AddKeyboardShortcut(kbsInsertCartridge);
	}
	
	if (viewC64->debugInterfaceAtari)
	{
//		kbsInsertAtariCartridge = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Cartridge", '0', false, false, false, true);
//		guiMain->AddKeyboardShortcut();
	}
	
	kbsCopyToClipboard  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Copy to clipboard", 'c', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsCopyToClipboard);

	kbsCopyAlternativeToClipboard  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Copy alternative to clipboard", 'c', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsCopyAlternativeToClipboard);

	kbsPasteFromClipboard  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Paste from clipboard", 'v', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsPasteFromClipboard);

	kbsPasteAlternativeFromClipboard  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Paste alternative from clipboard", 'v', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsPasteAlternativeFromClipboard);

	// code segments symbols
	kbsNextCodeSegmentSymbols  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Next code symbols segment", ';', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsNextCodeSegmentSymbols);

	kbsPreviousCodeSegmentSymbols  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Previous code symbols segment", '\'', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsPreviousCodeSegmentSymbols);

	// emulation rewind
	kbsScrubEmulationBackOneFrame  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Rewind emulation back one frame",
															  MTKEY_ARROW_LEFT, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsScrubEmulationBackOneFrame);

	kbsScrubEmulationForwardOneFrame  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Forward emulation one frame",
																 MTKEY_ARROW_RIGHT, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsScrubEmulationForwardOneFrame);

	kbsScrubEmulationBackOneSecond  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Rewind emulation back 1s",
															   MTKEY_ARROW_LEFT, false, true, true, false, this);
	guiMain->AddKeyboardShortcut(kbsScrubEmulationBackOneSecond);

	kbsScrubEmulationForwardOneSecond  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Forward emulation 1s",
															   MTKEY_ARROW_RIGHT, false, true, true, false, this);
	guiMain->AddKeyboardShortcut(kbsScrubEmulationForwardOneSecond);

	kbsScrubEmulationBackMultipleFrames  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Rewind emulation back 10s",
																	MTKEY_ARROW_LEFT, true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsScrubEmulationBackMultipleFrames);

	kbsScrubEmulationForwardMultipleFrames  = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Forward emulation 10s",
																	   MTKEY_ARROW_RIGHT, true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsScrubEmulationForwardMultipleFrames);

	// snapshots
	kbsSaveSnapshot = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Save snapshot", 's', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSaveSnapshot);
		
	kbsLoadSnapshot = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Load snapshot", 'd', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsLoadSnapshot);


	// ctrl+shift+1,2,3... store snapshot
	kbsStoreSnapshot1 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #1", '1', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot1);
	kbsStoreSnapshot2 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #2", '2', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot2);
	kbsStoreSnapshot3 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #3", '3', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot3);
	kbsStoreSnapshot4 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #4", '4', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot4);
	kbsStoreSnapshot5 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #5", '5', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot5);
	kbsStoreSnapshot6 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #6", '6', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot6);
	kbsStoreSnapshot7 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #7", '7', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot7);

	// ctrl+1,2,3,... restore snapshot
	kbsRestoreSnapshot1 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #1", '1', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot1);
	kbsRestoreSnapshot2 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #2", '2', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot2);
	kbsRestoreSnapshot3 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #3", '3', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot3);
	kbsRestoreSnapshot4 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #4", '4', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot4);
	kbsRestoreSnapshot5 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #5", '5', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot5);
	kbsRestoreSnapshot6 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #6", '6', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot6);
	kbsRestoreSnapshot7 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #7", '7', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot7);
	
	// joystick
	kbsJoystickUp = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick UP", MTKEY_ARROW_UP, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickUp);
	kbsJoystickDown = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick DOWN", MTKEY_ARROW_DOWN, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickDown);
	kbsJoystickLeft = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick LEFT", MTKEY_ARROW_LEFT, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickLeft);
	kbsJoystickRight = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick RIGHT", MTKEY_ARROW_RIGHT, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickRight);
	kbsJoystickFire = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick FIRE", MTKEY_RALT, false, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickFire);

#if defined(RUN_NES)
	kbsJoystickFireB = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick FIRE B", MTKEY_RCONTROL, false, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickFireB);
	kbsJoystickStart = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick START", MTKEY_F1, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickStart);
	kbsJoystickSelect = new CSlrKeyboardShortcut(KBZONE_SCREEN, "Joystick SELECT", MTKEY_F2, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsJoystickSelect);
#endif
	
	//
	kbsToggleBreakpoint = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLY, "Toggle Breakpoint", '`', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsToggleBreakpoint);

	// code run control
	kbsStepOverInstruction = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step over instruction", MTKEY_F10, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepOverInstruction);

	kbsStepOverJsr = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLY, "Step over JSR", MTKEY_F10, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStepOverJsr);

	kbsStepBackInstruction = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step back instruction", MTKEY_F10, false, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepBackInstruction);

	kbsForwardNumberOfCycles = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Forward number of cycles", MTKEY_F10, true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsForwardNumberOfCycles);

	kbsStepBackNumberOfCycles = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step back number of cycles", MTKEY_F10, true, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepBackNumberOfCycles);

	kbsStepOneCycle = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step one cycle", MTKEY_F10, true, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepOneCycle);

	kbsRunContinueEmulation = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Run/Continue code", MTKEY_F11, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsRunContinueEmulation);

	kbsGoToCycle = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Go to cycle", MTKEY_F11, true, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsGoToCycle);

	kbsGoToFrame = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Go to frame", MTKEY_F11, true, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsGoToFrame);

	//
	
	kbsMakeJmp = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLY, "Make JMP", 'j', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsMakeJmp);

	kbsToggleTrackPC = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLY, "Toggle track PC", ' ', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsToggleTrackPC);

	kbsGoToAddress = new CSlrKeyboardShortcut(KBZONE_MEMORY, "Go to address", 'g', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsGoToAddress);

	//
#if defined(RUN_COMMODORE64)
	kbsIsDataDirectlyFromRam = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Show data from RAM", 'm', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsIsDataDirectlyFromRam);

	kbsToggleMulticolorImageDump = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Show multicolor data", 'k', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsToggleMulticolorImageDump);

	kbsShowRasterBeam = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Show Raster Beam", 'e', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsShowRasterBeam);

	kbsSaveScreenImageAsPNG = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Save screenshot as PNG", 'p', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSaveScreenImageAsPNG);

#endif
}

void CMainMenuBar::UpdateGamepads()
{
	if (gamepads == NULL)
	{
		char *buf = SYS_GetCharBuf();
		int numGamepads;
		CGamePad **gamepadDevices = GAM_EnumerateGamepads(&numGamepads);
		gamepads = new std::list<const char *>();
		
		for (int i = 0; i < numGamepads; i++)
		{
			char *name = gamepadDevices[i]->name;
			
			if (gamepadDevices[i]->isActive)
			{
				sprintf(buf, "Gamepad #%d: %s", (i+1), name);
			}
			else
			{
				sprintf(buf, "Gamepad #%d", (i+1));
			}
			gamepads->push_back(STRALLOC(buf));
		}
		SYS_ReleaseCharBuf(buf);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// THIS IS MAIN MENU BAR ///////////////////////////////////////////////////////////////




void CMainMenuBar::RenderImGui()
{
	static bool openPopupImGuiWorkaround = false;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", kbsOpenFile->cstr))
			{
				// TODO: move me
				
				// SystemDialogFileOpenSelected
//				LOGM("OpenDialogOpenFile");
//				openDialogFunction = VIEWC64SETTINGS_OPEN_FILE;
//
//				CSlrString *windowTitle = new CSlrString("Open file");
//				viewC64->ShowDialogOpenFile(this, &openFileExtensions, c64SettingsDefaultPRGFolder, windowTitle);
//				delete windowTitle;
				
				viewC64->viewC64MainMenu->OpenDialogOpenFile();
			}
			
			bool mostRecentIsAvailable = viewC64->recentlyOpenedFiles->IsMostRecentFilePathAvailable();
			if (ImGui::MenuItem("Open Most Recent", kbsReloadAndRestart->cstr, false, mostRecentIsAvailable))
			{
				kbsReloadAndRestart->Run();
			}
			
			viewC64->recentlyOpenedFiles->RenderImGuiMenu("Open Recent");
			
			// TODO: should this menu item be here?
			if (viewC64->debugInterfaceC64 && viewC64->debugInterfaceC64->isRunning)
			{
				if (ImGui::MenuItem("Insert next C64 disk", kbsInsertNextD64->cstr))
				{
					kbsInsertNextD64->Run();
				}
			}
			
			if (ImGui::MenuItem("File browser", "", &viewC64->viewFileBrowser->visible))
			{
				viewC64->viewFileBrowser->SetVisible(true);
				guiMain->SetFocus(viewC64->viewFileBrowser);
				guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
			}
			
			ImGui::Separator();
			if (ImGui::MenuItem("Soft Reset", kbsSoftReset->cstr))
			{
				kbsSoftReset->Run();
			}
			if (ImGui::MenuItem("Hard Reset", kbsHardReset->cstr))
			{
				kbsHardReset->Run();
			}

			if (viewC64->debugInterfaceC64->isRunning)
			{
				if (ImGui::MenuItem("Cartridge Freeze", kbsCartridgeFreezeButton->cstr))
				{
					kbsCartridgeFreezeButton->Run();
				}
			}
			
			ImGui::Separator();
			if (ImGui::MenuItem("Detach Everything", kbsDetachEverything->cstr))
			{
				kbsDetachEverything->Run();
			}

			if (ImGui::MenuItem("Detach Disk Image", kbsDetachDiskImage->cstr))
			{
				kbsDetachDiskImage->Run();
			}
			
			if (ImGui::MenuItem("Detach Cartridge", kbsDetachCartridge->cstr))
			{
				kbsDetachCartridge->Run();
			}
			
			if (viewC64->debugInterfaceC64->isRunning
				|| viewC64->debugInterfaceAtari->isRunning)
			{
				if (ImGui::MenuItem("Detach Executable", kbsDetachExecutable->cstr))
				{
					kbsDetachExecutable->Run();
				}
			}
			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// EMULATORS VIEWS

			// Emulators
			ImGui::Separator();
			for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
			{
				CDebugInterface *debugInterface = *it;
			
				bool isRunning = debugInterface->isRunning;
				const char *name = debugInterface->GetPlatformNameString();
				
				if (ImGui::MenuItem(name, "", &isRunning))
				{
					if (debugInterface->isRunning == false)
					{
						viewC64->StartEmulationThread(debugInterface);
					}
					else
					{
						viewC64->StopEmulationThread(debugInterface);
					}
				}
			}
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Open Window", kbsSearchWindow->cstr))
			{
				kbsSearchWindow->Run();
			}
			if (ImGui::MenuItem("Close Window", kbsCloseWindow->cstr))
			{
				kbsCloseWindow->Run();
			}
			if (ImGui::MenuItem("Quit", kbsQuitApplication->cstr))
			{
				SYS_Shutdown();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Code"))
		{
			char *t = "Step Instruction";
			bool isPaused = true;

			for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
			{
				CDebugInterface *debugInterface = *it;
				if (debugInterface->GetDebugMode() == DEBUGGER_MODE_RUNNING)
				{
					// if at least one emulator is not paused then change text to Pause
					t = "Pause";
					isPaused = false;
					break;
				}
			}

			if (ImGui::MenuItem(t, kbsStepOverInstruction->cstr))
			{
				kbsStepOverInstruction->Run();
			}
			
//			if (isPaused)
			{
				if (ImGui::MenuItem("Step Over JSR", kbsStepOverJsr->cstr))
				{
					kbsStepOverJsr->Run();
				}
			}
			
			if (ImGui::MenuItem("Backstep", kbsStepBackInstruction->cstr))
			{
				kbsStepBackInstruction->Run();
			}

			if (ImGui::MenuItem("Continue", kbsRunContinueEmulation->cstr))
			{
				kbsRunContinueEmulation->Run();
			}
			
			if (ImGui::BeginMenu("Rewind"))
			{
				if (ImGui::MenuItem("Back one frame", kbsScrubEmulationBackOneFrame->cstr))
				{
					kbsScrubEmulationBackOneFrame->Run();
				}
				if (ImGui::MenuItem("Forward one frame", kbsScrubEmulationForwardOneFrame->cstr))
				{
					kbsScrubEmulationForwardOneFrame->Run();
				}
				if (ImGui::MenuItem("Back one second", kbsScrubEmulationBackOneSecond->cstr))
				{
					kbsScrubEmulationBackOneSecond->Run();
				}
				if (ImGui::MenuItem("Forward one second", kbsScrubEmulationForwardOneSecond->cstr))
				{
					kbsScrubEmulationForwardOneSecond->Run();
				}

				ImGui::Separator();
				//
				int stepNumberOfFrames = 10;
				gApplicationDefaultConfig->GetInt("StepNumberOfFrames", &stepNumberOfFrames, 10);

				if (ImGui::InputInt("Number of Frames", &stepNumberOfFrames, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					gApplicationDefaultConfig->SetInt("StepNumberOfFrames", &stepNumberOfFrames);
				}
				
				char *buf = SYS_GetCharBuf();
				sprintf(buf, "Back %d frames", stepNumberOfFrames);
				if (ImGui::MenuItem(buf, kbsScrubEmulationBackMultipleFrames->cstr))
				{
					kbsScrubEmulationBackMultipleFrames->Run();
				}
				sprintf(buf, "Forward %d frames", stepNumberOfFrames);
				if (ImGui::MenuItem(buf, kbsScrubEmulationForwardMultipleFrames->cstr))
				{
					kbsScrubEmulationForwardMultipleFrames->Run();
				}

				//
				int stepNumberOfCycles = 10;
				gApplicationDefaultConfig->GetInt("StepNumberOfCycles", &stepNumberOfCycles, 10);

				if (ImGui::InputInt("Number of Cycles", &stepNumberOfCycles, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					gApplicationDefaultConfig->SetInt("StepNumberOfCycles", &stepNumberOfCycles);
				}

				sprintf(buf, "Back %d cycles", stepNumberOfCycles);
				if (ImGui::MenuItem(buf, kbsStepBackNumberOfCycles->cstr))
				{
					kbsStepBackNumberOfCycles->Run();
				}
				sprintf(buf, "Forward %d cycles", stepNumberOfCycles);
				if (ImGui::MenuItem(buf, kbsForwardNumberOfCycles->cstr))
				{
					kbsForwardNumberOfCycles->Run();
				}

				if (ImGui::MenuItem("Step 1 cycle", kbsStepOneCycle->cstr))
				{
					kbsStepOneCycle->Run();
				}

				//
				int numRunningInterfaces = 0;
				for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
				{
					CDebugInterface *debugInterface = *it;
					if (debugInterface->isRunning)
						numRunningInterfaces++;
				}
				bool showEmulatorName = numRunningInterfaces > 1 ? true : false;

//				for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
				{
//					CDebugInterface *debugInterface = *it;
					CViewTimeline *viewTimeline;

					//	TODO:				CViewTimeline *viewTimeline = debugInterface->snapshotsManager->GetViewTimeline();
					viewTimeline = viewC64->viewC64Timeline;
					if (viewTimeline->debugInterface->isRunning)
					{
						ImGui::Separator();
						viewTimeline->RenderMenuGoTo(showEmulatorName);
					}

					viewTimeline = viewC64->viewAtariTimeline;
					if (viewTimeline->debugInterface->isRunning)
					{
						ImGui::Separator();
						viewTimeline->RenderMenuGoTo(showEmulatorName);
					}

					viewTimeline = viewC64->viewNesTimeline;
					if (viewTimeline->debugInterface->isRunning)
					{
						ImGui::Separator();
						viewTimeline->RenderMenuGoTo(showEmulatorName);
					}
				}
				
				ImGui::EndMenu();
			}
			
			bool warpSpeedState = isWarpSpeed;
			if (ImGui::MenuItem("Warp speed", kbsIsWarpSpeed->cstr, &warpSpeedState))
			{
				kbsIsWarpSpeed->Run();
			}
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Load Snapshot", kbsLoadSnapshot->cstr))
			{
				kbsLoadSnapshot->Run();
			}
			if (ImGui::MenuItem("Save Snapshot", kbsSaveSnapshot->cstr))
			{
				kbsSaveSnapshot->Run();
			}
			
			// TODO: move me, generalize me
			if (ImGui::BeginMenu("Quick Snapshots"))
			{
				if (ImGui::MenuItem("Store snapshot #1", kbsStoreSnapshot1->cstr))
				{
					kbsStoreSnapshot1->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #1", kbsRestoreSnapshot1->cstr))
				{
					kbsRestoreSnapshot1->Run();
				}

				if (ImGui::MenuItem("Store snapshot #2", kbsStoreSnapshot2->cstr))
				{
					kbsStoreSnapshot2->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #2", kbsRestoreSnapshot2->cstr))
				{
					kbsRestoreSnapshot2->Run();
				}

				if (ImGui::MenuItem("Store snapshot #3", kbsStoreSnapshot3->cstr))
				{
					kbsStoreSnapshot3->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #3", kbsRestoreSnapshot3->cstr))
				{
					kbsRestoreSnapshot3->Run();
				}

				if (ImGui::MenuItem("Store snapshot #4", kbsStoreSnapshot4->cstr))
				{
					kbsStoreSnapshot4->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #4", kbsRestoreSnapshot4->cstr))
				{
					kbsRestoreSnapshot4->Run();
				}

				if (ImGui::MenuItem("Store snapshot #5", kbsStoreSnapshot5->cstr))
				{
					kbsStoreSnapshot5->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #5", kbsRestoreSnapshot5->cstr))
				{
					kbsRestoreSnapshot5->Run();
				}

				if (ImGui::MenuItem("Store snapshot #6", kbsStoreSnapshot6->cstr))
				{
					kbsStoreSnapshot6->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #6", kbsRestoreSnapshot6->cstr))
				{
					kbsRestoreSnapshot6->Run();
				}

				if (ImGui::MenuItem("Store snapshot #7", kbsStoreSnapshot7->cstr))
				{
					kbsStoreSnapshot7->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #7", kbsRestoreSnapshot7->cstr))
				{
					kbsRestoreSnapshot7->Run();
				}

				ImGui::EndMenu();
			}
			
			ImGui::Separator();
			
			// segments
			if (ImGui::MenuItem("Next code segment", kbsNextCodeSegmentSymbols->cstr))
			{
				kbsNextCodeSegmentSymbols->Run();
			}
			if (ImGui::MenuItem("Previous code segment", kbsPreviousCodeSegmentSymbols->cstr))
			{
				kbsPreviousCodeSegmentSymbols->Run();
			}
			
			/////// note: below we check if we have only one emulator active (I guess most of times will be that), thus then we default control of segments to that one emulator only
			int emulatorNum = 0;
			for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
			{
				CDebugInterface *debugInterface = *it;
				if (debugInterface->isRunning)
				{
					char *buf = SYS_GetCharBuf();
					
					sprintf(buf, "%s Segments", debugInterface->GetPlatformNameString());
					if (debugInterface->symbols && !debugInterface->symbols->segments.empty())
					{
						if (ImGui::BeginMenu(buf, true))
						{
							CDebugSymbolsSegment *segmentToActivate = NULL;
							for (std::vector<CDebugSymbolsSegment *>::iterator it = debugInterface->symbols->segments.begin(); it != debugInterface->symbols->segments.end(); it++)
							{
								CDebugSymbolsSegment *segment = *it;
								bool selected = (segment == debugInterface->symbols->currentSegment);
								char *name = segment->name->GetStdASCII();
								if (name[0] == 0)
									name = STRALLOC("<empty>");
								if (ImGui::MenuItem(name, "", selected, true))
								{
									// segment selected
									segmentToActivate = segment;
								}
								STRFREE(name);
							}
							if (segmentToActivate)
							{
								debugInterface->symbols->ActivateSegment(segmentToActivate);
							}
							ImGui::EndMenu();
						}
					}
					else
					{
						// not enabled, can't select segments
						if (ImGui::BeginMenu(buf, false))
						{
							ImGui::EndMenu();
						}
					}
					
					sprintf(buf, "%s Labels", debugInterface->GetPlatformNameString());
					if (debugInterface->symbols && debugInterface->symbols->currentSegment)
					{
						if (ImGui::BeginMenu(buf, true))
						{
							if (ImGui::MenuItem("Clear labels"))
							{
								debugInterface->symbols->currentSegment->DeleteCodeLabels();
							}
							if (ImGui::MenuItem("Import labels"))
							{
								systemDialogOperation = SystemDialogOperationImportLabels;
								selectedDebugInterface = debugInterface;
								CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
								SYS_DialogOpenFile(this, &extensionsImportLabels, defaultFolder, NULL);
								delete defaultFolder;
							}
							if (ImGui::MenuItem("Export labels"))
							{
								systemDialogOperation = SystemDialogOperationExportLabels;
								selectedDebugInterface = debugInterface;
								CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
								SYS_DialogSaveFile(this, &extensionsExportLabels, NULL, defaultFolder, NULL);
								delete defaultFolder;
							}
							ImGui::EndMenu();
						}
						
						sprintf(buf, "%s Watches", debugInterface->GetPlatformNameString());
						if (debugInterface->symbols && debugInterface->symbols->currentSegment)
						{
							if (ImGui::BeginMenu(buf, true))
							{
								if (ImGui::MenuItem("Clear watches"))
								{
									debugInterface->symbols->currentSegment->DeleteAllWatches();
								}
								if (ImGui::MenuItem("Import watches"))
								{
									systemDialogOperation = SystemDialogOperationImportWatches;
									selectedDebugInterface = debugInterface;
									CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
									SYS_DialogOpenFile(this, &extensionsWatches, defaultFolder, NULL);
									delete defaultFolder;
								}
								if (ImGui::MenuItem("Export watches"))
								{
									systemDialogOperation = SystemDialogOperationExportWatches;
									selectedDebugInterface = debugInterface;
									CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
									SYS_DialogSaveFile(this, &extensionsWatches, NULL, defaultFolder, NULL);
									delete defaultFolder;
								}
								ImGui::EndMenu();
							}
						}
						
						sprintf(buf, "%s Breakpoints", debugInterface->GetPlatformNameString());
						if (debugInterface->symbols && debugInterface->symbols->currentSegment)
						{
							if (ImGui::BeginMenu(buf, true))
							{
								if (ImGui::MenuItem("Clear breakpoints"))
								{
									debugInterface->symbols->currentSegment->ClearBreakpoints();
									debugInterface->symbols->UpdateRenderBreakpoints();
								}
								if (ImGui::MenuItem("Import breakpoints"))
								{
									systemDialogOperation = SystemDialogOperationImportBreakpoints;
									selectedDebugInterface = debugInterface;
									CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
									SYS_DialogOpenFile(this, &extensionsBreakpoints, defaultFolder, NULL);
									delete defaultFolder;
								}
								if (ImGui::MenuItem("Export breakpoints"))
								{
									systemDialogOperation = SystemDialogOperationExportBreakpoints;
									selectedDebugInterface = debugInterface;
									CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
									SYS_DialogSaveFile(this, &extensionsBreakpoints, NULL, defaultFolder, NULL);
									delete defaultFolder;
								}
								ImGui::EndMenu();
							}
						}
												
						sprintf(buf, "%s Timeline", debugInterface->GetPlatformNameString());
						if (ImGui::BeginMenu(buf, c64SettingsSnapshotsRecordIsActive))
						{
							if (ImGui::MenuItem("Save timeline"))
							{
								OpenDialogSaveTimeline(debugInterface);
							}
							ImGui::Separator();
							if (ImGui::MenuItem("Load timeline"))
							{
								OpenDialogLoadTimeline();
							}
							if (ImGui::MenuItem("Clear timeline"))
							{
								debugInterface->snapshotsManager->ClearSnapshotsHistory();
								debugInterface->symbols->debugEventsHistory->DeleteAllEvents();
							}
							ImGui::EndMenu();
						}
						
					}
					else
					{
						// not enabled
						if (ImGui::BeginMenu(buf, false))
						{
							ImGui::EndMenu();
						}
					}

					/*
					// snapshots
					sprintf(buf, "%s Timeline", debugInterface->GetPlatformNameString());
//					if (debugInterface->symbols && debugInterface->symbols->currentSegment)
					{
						if (ImGui::BeginMenu(buf, true))
						{
//							if (ImGui::MenuItem("Save snapshot", emulatorNum == 0 ? kbsSna))
							ImGui::EndMenu();
						}
					}
//					else
//					{
//						// not enabled
//						if (ImGui::BeginMenu(buf, false))
//						{
//							ImGui::EndMenu();
//						}
//					}
					*/
					
					SYS_ReleaseCharBuf(buf);
				}
				
				emulatorNum++;
			}
			
			if (ImGui::BeginMenu("Keep symbols"))
			{
				bool all = c64SettingsKeepSymbolsLabels && c64SettingsKeepSymbolsWatches && c64SettingsKeepSymbolsBreakpoints;
				
				if (ImGui::MenuItem("All symbols", "", &all))
				{
					c64SettingsKeepSymbolsLabels = all;
					c64SettingsKeepSymbolsWatches = all;
					c64SettingsKeepSymbolsBreakpoints = all;
					C64DebuggerStoreSettings();
					
					viewC64->DefaultSymbolsStore();
				}
				
				if (ImGui::MenuItem("Labels", "", &c64SettingsKeepSymbolsLabels))
				{
					C64DebuggerStoreSettings();
				}
				
				if (ImGui::MenuItem("Watches", "", &c64SettingsKeepSymbolsWatches))
				{
					C64DebuggerStoreSettings();
				}

				if (ImGui::MenuItem("Breakpoints", "", &c64SettingsKeepSymbolsBreakpoints))
				{
					C64DebuggerStoreSettings();
				}
				
				ImGui::EndMenu();
			}
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Reset emulation counters", kbsResetCpuCycleAndFrameCounters->cstr))
			{
				kbsResetCpuCycleAndFrameCounters->Run();
			}

			if (ImGui::MenuItem("Clear memory markers", kbsClearMemoryMarkers->cstr))
			{
				kbsClearMemoryMarkers->Run();
			}
			
			ImGui::EndMenu();
		}
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// EMULATORS MENUS

		// emulators menus
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			
			if (debugInterface->isRunning == false)
				continue;
			
			const char *name = debugInterface->GetPlatformNameString();
			
			if (ImGui::BeginMenu(name))
			{
				for (std::list<CDebugInterfaceMenuItem *>::iterator it = debugInterface->menuItems.begin(); it != debugInterface->menuItems.end(); it++)
				{
					CDebugInterfaceMenuItem *menuItem = *it;
					menuItem->RenderImGui();
				}

				ImGui::EndMenu();
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//		if (viewC64->viewVicEditor->IsVisible())
		if (viewC64->debugInterfaceC64->isRunning)
		{
			bool isVisible = viewC64->viewVicEditor->IsVisible();
			if (ImGui::BeginMenu("VIC Editor##MainMenuBar"))
			{
				if (ImGui::MenuItem("Show VIC Editor", "", &isVisible))
				{
					viewC64->viewVicEditor->SetVisible(true);
				}
				ImGui::Separator();
				
				viewC64->viewVicEditor->RenderContextMenuItems();
				ImGui::EndMenu();
			}
		}
		
		if (ImGui::BeginMenu("Plugins"))
		{
			// TODO: generalize me
			// TODO: add GetName to Plugin
			if (ImGui::MenuItem("Goat Tracker"))
			{
				PLUGIN_GoatTrackerInit();
				PLUGIN_GoatTrackerSetVisible(true);
			}
			
			if (ImGui::MenuItem("DNDK Trainer"))
			{
				PLUGIN_DdnkInit();
				PLUGIN_DdnkSetVisible(true);
			}

			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Workspace"))
		{
			for (std::list<CLayoutData *>::iterator it = guiMain->layoutManager->layouts.begin();
				 it != guiMain->layoutManager->layouts.end(); it++)
			{
				CLayoutData *layoutData = *it;
				
				bool isSelected = (layoutData == guiMain->layoutManager->currentLayout);

				char *buf = SYS_GetCharBuf();

				// color on
				if (layoutData->doNotUpdateViewsPositions)
				{
//					ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Menu bar background color
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 32.0f)); // Menu bar padding				}

					sprintf(buf, "*%s##workspacemenu", layoutData->layoutName);
				}
				else
				{
					sprintf(buf, " %s##workspacemenu", layoutData->layoutName);
				}
				
				const char *keyShortcutName = layoutData->keyShortcut ? layoutData->keyShortcut->cstr : "";
				if (ImGui::MenuItem(buf, keyShortcutName, &isSelected))
				{
					guiMain->layoutManager->SetLayoutAsync(layoutData, true);
				}
				
				// color off
				if (layoutData->doNotUpdateViewsPositions)
				{
//					ImGui::PopStyleColor();
					ImGui::PopStyleVar(1);
					ImGui::PopStyleColor(1);
				}
			}
			
			ImGui::Separator();
			if (ImGui::MenuItem("New Workspace..."))
			{
				layoutData = new CLayoutData();
				guiMain->layoutManager->SerializeLayoutAsync(layoutData);
				
				// does not work ImGui::OpenPopup("Store Layout as...");
				openPopupImGuiWorkaround = true;
			}

			if (ImGui::MenuItem("Delete This Workspace", "", false, guiMain->layoutManager->currentLayout != NULL))
			{
				CLayoutData *layout = guiMain->layoutManager->currentLayout;
				guiMain->layoutManager->RemoveAndDeleteLayout(layout);
				guiMain->layoutManager->currentLayout = NULL;
				guiMain->layoutManager->StoreLayouts();
				guiMain->ShowNotification("Information", "Workspace deleted");
			}
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Save Workspaces"))
			{
				guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
				guiMain->ShowNotification("Information", "Workspaces saved");
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			// imgui example:
//					if (ImGui::BeginMenu("Options"))
//					{
//						static bool enabled = true;
//						ImGui::MenuItem("Enabled", "", &enabled);
//						ImGui::BeginChild("child", ImVec2(0, 60), true);
//						for (int i = 0; i < 10; i++)
//							ImGui::Text("Scrolling Text %d", i);
//						ImGui::EndChild();
//						static float f = 0.5f;
//						static int n = 0;
//						ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
//						ImGui::InputFloat("Input", &f, 0.1f);
//						ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
//						ImGui::EndMenu();
//					}

			
			if (viewC64->debugInterfaceC64)
			{
				if (ImGui::BeginMenu("C64##Settings"))
				{
					if (ImGui::MenuItem("Load first PRG on disk insert",
										kbsAutoJmpFromInsertedDiskFirstPrg->cstr,
										&c64SettingsAutoJmpFromInsertedDiskFirstPrg))
					{
						C64DebuggerStoreSettings();
					}
					
					if (ImGui::MenuItem("Always JMP to loaded addr",
										kbsAutoJmpAlwaysToLoadedPRGAddress->cstr,
										&c64SettingsAutoJmpAlwaysToLoadedPRGAddress))
					{
						C64DebuggerStoreSettings();
					}

					// alternative:
//					if (ImGui::Combo("Reset C64 before PRG load", &c64SettingsAutoJmpDoReset, "No reset\0Soft reset\0Hard reset\0\0"))
//					{
//						C64DebuggerStoreSettings();
//					}
					if (ImGui::BeginMenu("Reset C64 before PRG load"))
					{
						bool b = c64SettingsAutoJmpDoReset == MACHINE_RESET_NONE;
						bool bp = c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD;
						if (ImGui::MenuItem("No reset",
											bp ? kbsAutoJmpDoReset->cstr : "", &b))
						{
							c64SettingsAutoJmpDoReset = MACHINE_RESET_NONE;
							C64DebuggerStoreSettings();
						}

						bp = b;
						b = c64SettingsAutoJmpDoReset == MACHINE_RESET_SOFT;

						if (ImGui::MenuItem("Soft reset",
											bp ? kbsAutoJmpDoReset->cstr : "", &b))
						{
							c64SettingsAutoJmpDoReset = MACHINE_RESET_SOFT;
							C64DebuggerStoreSettings();
						}

						bp = b;
						b = c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD;

						if (ImGui::MenuItem("Hard reset",
											bp ? kbsAutoJmpDoReset->cstr : "", &b))
						{
							c64SettingsAutoJmpDoReset = MACHINE_RESET_HARD;
							C64DebuggerStoreSettings();
						}

						ImGui::EndMenu();
					}
					
					int step = 10; int step_fast = 100;
					if (ImGui::InputScalar("Wait after Reset", ImGuiDataType_::ImGuiDataType_U32, &c64SettingsAutoJmpWaitAfterReset, &step, &step_fast, "%d", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						C64DebuggerStoreSettings();
					}
					
					ImGui::Separator();

					if (ImGui::BeginMenu("Memory"))
					{
						if (ImGui::MenuItem("Dump C64 memory", kbsDumpC64Memory->cstr))
						{
							OpenDialogDumpC64Memory();
						}
						if (ImGui::MenuItem("Dump C64 memory markers"))
						{
							OpenDialogDumpC64MemoryMarkers();
						}
						if (ImGui::MenuItem("Dump Disk 1541 memory", kbsDumpDrive1541Memory->cstr))
						{
							OpenDialogDumpDrive1541Memory();
						}
						if (ImGui::MenuItem("Dump Disk 1541 memory markers"))
						{
							OpenDialogDumpDrive1541MemoryMarkers();
						}
						ImGui::Separator();
						
						if (c64SettingsPathToC64MemoryMapFile == NULL)
						{
							if (ImGui::MenuItem("Map C64 memory to file"))
							{
								OpenDialogMapC64MemoryToFile();
							}
						}
						else
						{
							char *buf = SYS_GetCharBuf();
							char *asciiPath = c64SettingsPathToC64MemoryMapFile->GetStdASCII();
							
							// display file name in menu
							char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
							sprintf(buf, "Unmap C64 memory from file: %s", fname);
							if (ImGui::MenuItem(buf))
							{
								UnmapC64MemoryFromFile();
							}
							SYS_ReleaseCharBuf(buf);
						}

						ImGui::EndMenu();
					}
					
					if (ImGui::BeginMenu("Profiler"))
					{
						if (c64SettingsC64ProfilerFileOutputPath != NULL)
						{
							char *buf = SYS_GetCharBuf();
							char *asciiPath = c64SettingsC64ProfilerFileOutputPath->GetStdASCII();
							
							// display file name in menu
							char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
							sprintf(buf, "Set Profiler file: %s", fname);
							if (ImGui::MenuItem(buf))
							{
								OpenDialogSetC64ProfilerFileOutputPath();
							}
							
							delete [] fname;
							delete [] asciiPath;
						}
						else
						{
							if (ImGui::MenuItem("Set Profiler file"))
							{
								OpenDialogSetC64ProfilerFileOutputPath();
							}
						}
						
						if (ImGui::MenuItem("Perform VIC profiling", NULL, &c64SettingsC64ProfilerDoVicProfile))
						{
							C64DebuggerSetSetting("C64ProfilerDoVic", &c64SettingsC64ProfilerDoVicProfile);
							C64DebuggerStoreSettings();
						}
						
						bool isProfilingC64 = viewC64->debugInterfaceC64->IsProfilerActive();
						if (ImGui::MenuItem(isProfilingC64 ? "Stop Profiler" : "Start Profiler", kbsC64ProfilerStartStop->cstr))
						{
							// TODO: add option in menu to run profiler for selected number of cycles (funct already implemented)
							// -1 means run endlessly
							// also in 		else if (shortcut == kbsC64ProfilerStartStop)
							C64ProfilerStartStop(-1, false);	//int runForNumCycles, bool pauseCpuWhenFinished
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Code monitor"))
					{
						bool b = !c64SettingsUseNativeEmulatorMonitor;
						if (ImGui::MenuItem("Default", NULL, &b))
						{
							c64SettingsUseNativeEmulatorMonitor = false;
							viewC64->viewC64MonitorConsole->PrintInitPrompt();
							C64DebuggerStoreSettings();
						}
						b = c64SettingsUseNativeEmulatorMonitor;
						if (ImGui::MenuItem("VICE", NULL, &b))
						{
							c64SettingsUseNativeEmulatorMonitor = true;
							viewC64->viewC64MonitorConsole->PrintInitPrompt();
							C64DebuggerStoreSettings();
						}
						ImGui::EndMenu();
					}
					ImGui::Separator();
					
					/////
					ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);

					if (ImGui::BeginMenu("VIC"))
					{
						if (ImGui::BeginMenu("VIC Palette"))
						{
							viewC64->viewC64Palette->RenderMenuItemsAvailablePalettes();
							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("VIC Border mode"))
						{
							std::vector<const char *> borderModes;
							borderModes.push_back("Normal");		// 0
							borderModes.push_back("Full");			// 1
							borderModes.push_back("Debug");			// 2
							borderModes.push_back("No borders");	// 3
							
							int currentBorderMode = 0;
							viewC64->config->GetInt("viceViciiBorderMode", &currentBorderMode, 0);
							
							for (int i = 0; i < borderModes.size(); i++)
							{
								if (ImGui::MenuItem(borderModes[i], NULL, (i == currentBorderMode)))
								{
									currentBorderMode = i;
									viewC64->config->SetInt("viceViciiBorderMode", &currentBorderMode);
									debugInterfaceVice->SetViciiBorderMode(currentBorderMode);
								}
							}
							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("VIC Display recording"))
						{
							bool b = (c64SettingsVicStateRecordingMode == C64D_VICII_RECORD_MODE_NONE);
							if (ImGui::MenuItem("None", NULL, &b))
							{
								c64SettingsVicStateRecordingMode = C64D_VICII_RECORD_MODE_NONE;
								viewC64->debugInterfaceC64->SetVicRecordStateMode(c64SettingsVicStateRecordingMode);
								C64DebuggerStoreSettings();
							}
							
							b = (c64SettingsVicStateRecordingMode == C64D_VICII_RECORD_MODE_EVERY_LINE);
							if (ImGui::MenuItem("Each raster line", NULL, &b))
							{
								c64SettingsVicStateRecordingMode = C64D_VICII_RECORD_MODE_EVERY_LINE;
								viewC64->debugInterfaceC64->SetVicRecordStateMode(c64SettingsVicStateRecordingMode);
								C64DebuggerStoreSettings();
							}
							
							b = (c64SettingsVicStateRecordingMode == C64D_VICII_RECORD_MODE_EVERY_CYCLE);
							if (ImGui::MenuItem("Each VIC cycle", NULL, &b))
							{
								c64SettingsVicStateRecordingMode = C64D_VICII_RECORD_MODE_EVERY_CYCLE;
								viewC64->debugInterfaceC64->SetVicRecordStateMode(c64SettingsVicStateRecordingMode);
								C64DebuggerStoreSettings();
							}

							ImGui::EndMenu();
						}
						
						if (ImGui::MenuItem("Skip drawing sprites", NULL, &c64SettingsVicSkipDrawingSprites))
						{
							viewC64->debugInterfaceC64->SetSkipDrawingSprites(c64SettingsVicSkipDrawingSprites);
							C64DebuggerStoreSettings();
						}
						
						ImGui::EndMenu();
					}
					
					if (ImGui::BeginMenu("SID"))
					{
						if (ImGui::SliderInt("VICE audio volume", &c64SettingsViceAudioVolume, 0, 100))
						{
							u16 v = c64SettingsViceAudioVolume;
							C64DebuggerSetSetting("ViceAudioVolume", &v);
							C64DebuggerStoreSettings();
						}
						
						if (ImGui::MenuItem("Mute SID on pause", NULL, &c64SettingsMuteSIDOnPause))
						{
							C64DebuggerSetSetting("MuteSIDOnPause", &c64SettingsMuteSIDOnPause);
							C64DebuggerStoreSettings();
						}
						
						if (ImGui::BeginMenu("SID file import mode"))
						{
							std::vector<const char *> options;
							options.push_back("Copy to RAM");
							options.push_back("PSID64");

							int val = 0;
							for (std::vector<const char *>::iterator itName = options.begin(); itName != options.end(); itName++)
							{
								const char *optStr = *itName;
								bool selected = (val == c64SettingsC64SidImportMode);
								if (ImGui::MenuItem(optStr, NULL, &selected))
								{
									C64DebuggerSetSetting("SIDImportMode", &val);
									C64DebuggerStoreSettings();
								}
								val++;
							}
							ImGui::EndMenu();
						}
						
						if (ImGui::SliderInt("SID Tracker history length", &c64SettingsSidDataHistoryMaxSize, 50, 120000))
						{
							C64DebuggerSetSetting("SIDDataHistoryMaxSize", &c64SettingsSidDataHistoryMaxSize);
							C64DebuggerStoreSettings();
						}
						
						ImGui::Separator();
						if (ImGui::BeginMenu("SID stereo"))
						{
							std::vector<const char *> options;
							options.push_back("No");
							options.push_back("Two SIDs");
							options.push_back("Three SIDs");

							int val = 0;
							for (std::vector<const char *>::iterator itName = options.begin(); itName != options.end(); itName++)
							{
								const char *optStr = *itName;
								bool selected = (val == c64SettingsSIDStereo);
								if (ImGui::MenuItem(optStr, NULL, &selected))
								{
									C64DebuggerSetSetting("SIDStereo", &val);
									C64DebuggerStoreSettings();
								}
								val++;
							}
							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("SID #2 address"))
						{
							int optNum = GetOptionNumFromSidAddress(c64SettingsSIDStereoAddress);
							int n = 0;
							for (std::vector<const char *>::iterator it = sidAddressOptions.begin(); it != sidAddressOptions.end(); it++)
							{
								bool selected = (optNum == n);
								if (ImGui::MenuItem(*it, NULL, selected))
								{
									u16 sidAddr = GetSidAddressFromOptionNum(n);
									C64DebuggerSetSetting("SIDStereoAddress", &sidAddr);
								}
								n++;
							}
							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("SID #3 address"))
						{
							int optNum = GetOptionNumFromSidAddress(c64SettingsSIDTripleAddress);
							int n = 0;
							for (std::vector<const char *>::iterator it = sidAddressOptions.begin(); it != sidAddressOptions.end(); it++)
							{
								bool selected = (optNum == n);
								if (ImGui::MenuItem(*it, NULL, selected))
								{
									u16 sidAddr = GetSidAddressFromOptionNum(n);
									C64DebuggerSetSetting("SIDTripleAddress", &sidAddr);
								}
								n++;
							}
							ImGui::EndMenu();
						}

						ImGui::Separator();
						
						if (ImGui::BeginMenu("SID engine"))
						{
							int i = 0;
							for (std::vector<const char *>::iterator it = sidTypes.begin(); it != sidTypes.end(); it++)
							{
								const char *sidModel = *it;
								bool selected = (c64SettingsSIDEngineModel == i);
								if (ImGui::MenuItem(sidModel, "", &selected))
								{
									C64DebuggerSetSetting("SIDEngineModel", &i);
									C64DebuggerStoreSettings();
								}
								i++;
							}
							ImGui::EndMenu();
						}

						if (ImGui::BeginMenu("RESID Sampling method"))
						{
							std::vector<const char *> options;
							options.push_back("Fast");
							options.push_back("Interpolating");
							options.push_back("Resampling");
							options.push_back("Fast Resampling");

							int val = 0;
							for (std::vector<const char *>::iterator itName = options.begin(); itName != options.end(); itName++)
							{
								const char *optStr = *itName;
								bool selected = (val == c64SettingsRESIDSamplingMethod);
								if (ImGui::MenuItem(optStr, NULL, &selected))
								{
									C64DebuggerSetSetting("RESIDSamplingMethod", &val);
									C64DebuggerStoreSettings();
								}
								val++;
							}
							ImGui::EndMenu();
						}
						
						if (ImGui::MenuItem("RESID Emulate filters", NULL, c64SettingsRESIDEmulateFilters))
						{
							C64DebuggerSetSetting("RESIDEmulateFilters", &c64SettingsRESIDEmulateFilters);
							C64DebuggerStoreSettings();
						}
						
						if (ImGui::SliderInt("RESID Pass Band", &c64SettingsRESIDPassBand, 0, 90))
						{
							C64DebuggerSetSetting("RESIDPassBand", &c64SettingsRESIDPassBand);
							C64DebuggerStoreSettings();
						}
						
						if (ImGui::SliderInt("RESID Filter Bias", &c64SettingsRESIDFilterBias, -500, 500))
						{
							C64DebuggerSetSetting("RESIDFilterBias", &c64SettingsRESIDPassBand);
							C64DebuggerStoreSettings();
						}
						
						ImGui::EndMenu();
					}
				
					/* TODO: check if SID mute works correctly
					//
					options = new std::vector<CSlrString *>();
					options->push_back(new CSlrString("Zero volume"));
					options->push_back(new CSlrString("Stop SID emulation"));
					
					menuItemMuteSIDMode = new CViewC64MenuItemOption(fontHeight, new CSlrString("Mute SID mode: "),
																	 NULL, tr, tg, tb, options, font, fontScale);
					menuItemMuteSIDMode->SetSelectedOption(c64SettingsMuteSIDMode, false);
					menuItemSubMenuAudio->AddMenuItem(menuItemMuteSIDMode);
					
					//
					menuItemRunSIDEmulation = new CViewC64MenuItemOption(fontHeight, new CSlrString("Run SID emulation: "),
																		 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
					menuItemRunSIDEmulation->SetSelectedOption(c64SettingsRunSIDEmulation ? 1 : 0, false);
					menuItemSubMenuAudio->AddMenuItem(menuItemRunSIDEmulation);
					
					//
					
					menuItemRunSIDWhenInWarp = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Run SID emulation in warp: "),
																		  NULL, tr, tg, tb, optionsYesNo, font, fontScale);
					menuItemRunSIDWhenInWarp->SetSelectedOption(c64SettingsRunSIDWhenInWarp ? 1 : 0, false);
					menuItemSubMenuAudio->AddMenuItem(menuItemRunSIDWhenInWarp);
					

					*/
					
					if (ImGui::BeginMenu("REU"))
					{
						if (ImGui::MenuItem("Enabled", "", &c64SettingsReuEnabled))
						{
							C64DebuggerSetSetting("ReuEnabled", &c64SettingsReuEnabled);
							C64DebuggerStoreSettings();
						}

						const int settingsReuSizes[8] = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384 };
						
						int selectedReuSize = 0;
						for (int i = 0; i < 8; i++)
						{
							if (settingsReuSizes[i] == c64SettingsReuSize)
							{
								selectedReuSize = i;
								break;
							}
						}
						
						if (ImGui::Combo("REU Size", &selectedReuSize, " 128\0 256\0 512\0 1024\0 2048\0 4096\0 8192\0 16384\0\0"))
						{
							int reuSize = settingsReuSizes[selectedReuSize];
							C64DebuggerSetSetting("ReuSize", &reuSize);
							C64DebuggerStoreSettings();
						}

						if (ImGui::MenuItem("Attach REU"))
						{
							systemDialogOperation = SystemDialogOperationLoadREU;
							selectedDebugInterface = viewC64->debugInterfaceC64;
							CSlrString *defaultFolder = c64SettingsDefaultReuFolder;
							SYS_DialogOpenFile(this, &extensionsREU, defaultFolder, NULL);
						}
						
						if (ImGui::MenuItem("Save REU"))
						{
							systemDialogOperation = SystemDialogOperationSaveREU;
							selectedDebugInterface = viewC64->debugInterfaceC64;
							CSlrString *defaultFolder = c64SettingsDefaultReuFolder;
							SYS_DialogSaveFile(this, &extensionsREU, NULL, defaultFolder, NULL);
						}

						ImGui::EndMenu();
					}

					///
					if (ImGui::BeginMenu("Datasette##c64"))
					{
						if (ImGui::MenuItem("Insert Tape##c64Datasette", kbsTapeAttach->cstr))
						{
							kbsTapeAttach->Run();
						}
						if (ImGui::MenuItem("Remove Tape##c64Datasette", kbsTapeDetach->cstr))
						{
							kbsTapeDetach->Run();
						}
						ImGui::Separator();
//						if (ImGui::MenuItem("Create Tape##c64"))
//						{
//							TODO
//						}
						if (ImGui::MenuItem("Play##c64Datasette", kbsTapePlay->cstr))
						{
							kbsTapePlay->Run();
						}
						if (ImGui::MenuItem("Stop##c64Datasette", kbsTapeStop->cstr))
						{
							kbsTapeStop->Run();
						}
						if (ImGui::MenuItem("Forward##c64Datasette", kbsTapeForward->cstr))
						{
							kbsTapeForward->Run();
						}
						if (ImGui::MenuItem("Rewind##c64Datasette", kbsTapeRewind->cstr))
						{
							kbsTapeRewind->Run();
						}
						if (ImGui::MenuItem("Record##c64Datasette", kbsTapeRecord->cstr))
						{
							kbsTapeRecord->Run();
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Reset Datasette##c64Datasette"))
						{
							viewC64->debugInterfaceC64->DatasetteReset();
							viewC64->ShowMessage("Datasette RESET");
						}
						ImGui::Separator();
						
						if (ImGui::InputInt("Speed tuning##c64Datasette", &c64SettingsDatasetteSpeedTuning))
						{
							if (c64SettingsDatasetteSpeedTuning < 0)
								c64SettingsDatasetteSpeedTuning = 0;
							if (c64SettingsDatasetteSpeedTuning > 100)
								c64SettingsDatasetteSpeedTuning = 100;
							viewC64->debugInterfaceC64->DatasetteSetSpeedTuning(c64SettingsDatasetteSpeedTuning);
							C64DebuggerStoreSettings();
						}
						if (ImGui::InputInt("Zero-gap delay##c64Datasette", &c64SettingsDatasetteZeroGapDelay))
						{
							if (c64SettingsDatasetteZeroGapDelay < 0)
								c64SettingsDatasetteZeroGapDelay = 0;
							if (c64SettingsDatasetteZeroGapDelay > 50000)
								c64SettingsDatasetteZeroGapDelay = 50000;
							viewC64->debugInterfaceC64->DatasetteSetZeroGapDelay(c64SettingsDatasetteZeroGapDelay);
							C64DebuggerStoreSettings();
						}
						if (ImGui::InputInt("Tape wobble##c64Datasette", &c64SettingsDatasetteTapeWobble))
						{
							if (c64SettingsDatasetteTapeWobble < 0)
								c64SettingsDatasetteTapeWobble = 0;
							if (c64SettingsDatasetteTapeWobble > 100)
								c64SettingsDatasetteTapeWobble = 100;
							viewC64->debugInterfaceC64->DatasetteSetTapeWobble(c64SettingsDatasetteTapeWobble);
							C64DebuggerStoreSettings();
						}
						if (ImGui::MenuItem("Reset with CPU##c64Datasette", NULL, &c64SettingsDatasetteResetWithCPU))
						{
							viewC64->debugInterfaceC64->DatasetteSetResetWithCPU(c64SettingsDatasetteResetWithCPU);
							C64DebuggerStoreSettings();
						}

						ImGui::EndMenu();
					}

					ImGui::Separator();
					
					///
					if (ImGui::BeginMenu("Machine model"))
					{
						std::vector<const char *>::iterator itMachineModelName = c64ModelTypeNames->begin();
						std::vector<int>::iterator itMachineModelId = c64ModelTypeIds->begin();

						while(itMachineModelName != c64ModelTypeNames->end())
						{
							int modelId = *itMachineModelId;
							const char *modelName = *itMachineModelName;
							
							bool selected = (c64SettingsC64Model == modelId);
							
							if (ImGui::MenuItem(modelName, NULL, &selected))
							{
								C64DebuggerSetSetting("C64Model", &modelId);
								C64DebuggerStoreSettings();
							}
							
							itMachineModelName++;
							itMachineModelId++;
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Emulation speed"))
					{
						if (ImGui::MenuItem("Next maximum speed", kbsSwitchNextMaximumSpeed->cstr))
						{
							SwitchNextMaximumSpeed();
						}
						if (ImGui::MenuItem("Previous maximum speed", kbsSwitchPrevMaximumSpeed->cstr))
						{
							SwitchPrevMaximumSpeed();
						}
						ImGui::Separator();
						std::vector<const char *> optionsNames;
						std::vector<int> optionsValues;
						optionsNames.push_back("10%");
						optionsValues.push_back(10);
						optionsNames.push_back("20%");
						optionsValues.push_back(20);
						optionsNames.push_back("50%");
						optionsValues.push_back(50);
						optionsNames.push_back("100%");
						optionsValues.push_back(100);
						optionsNames.push_back("200%");
						optionsValues.push_back(200);
						optionsNames.push_back("300%");
						optionsValues.push_back(300);
						optionsNames.push_back("400%");
						optionsValues.push_back(400);

						std::vector<int>::iterator itValue = optionsValues.begin();
						for (std::vector<const char *>::iterator itName = optionsNames.begin(); itName != optionsNames.end(); itName++)
						{
							const char *optStr = *itName;
							int val = *itValue;
							bool selected = (val == c64SettingsEmulationMaximumSpeed);
							if (ImGui::MenuItem(optStr, NULL, &selected))
							{
								SetEmulationMaximumSpeed(val);
							}
							itValue++;
						}
						ImGui::EndMenu();
					}
					
//					was above ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
					ImGui::PopItemFlag();
					
					if (ImGui::MenuItem("Fast boot kernal patch", NULL, &c64SettingsFastBootKernalPatch))
					{
						debugInterfaceVice->SetPatchKernalFastBoot(c64SettingsFastBootKernalPatch);
						C64DebuggerStoreSettings();
					}

					if (ImGui::MenuItem("Emulate VSP bug", NULL, &c64SettingsEmulateVSPBug))
					{
						viewC64->debugInterfaceC64->SetVSPBugEmulation(c64SettingsEmulateVSPBug);
						C64DebuggerStoreSettings();
					}
					
					if (ImGui::MenuItem("Skip dummy read on STA", NULL, &c64dSkipBogusPageOffsetReadOnSTA))
					{
						C64DebuggerStoreSettings();
					}
					
					ImGui::Separator();
					
					bool isVisible = viewC64->viewC64KeyMap->visible;
					if (ImGui::MenuItem("Keyboard mapping##c64", "", &isVisible))
					{
						viewC64->viewC64KeyMap->SetFocus();
						guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
					}

					if (ImGui::MenuItem("Select C64 ROMs folder"))
					{
						systemDialogOperation = SystemDialogOperationC64RomsFolder;
						SYS_DialogPickFolder(this, c64SettingsPathToC64Roms);
					}
	
					/*
					if (ImGui::BeginMenu("Select C64 ROMs"))
					{
						if (ImGui::MenuItem("Select C64 ROMs folder"))
						{
							systemDialogOperation = SystemDialogOperationC64RomsFolder;
							SYS_DialogPickFolder(this, c64SettingsPathToC64Roms);
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Kernal"))
						{
//							systemDialogOperation = SystemDialogOperationC64Kernal;
//							SYS_DialogPickFolder(this, c64SettingsPathToAtariROMs);
						}
						
						ImGui::EndMenu();
					}
					 */
					
					ImGui::EndMenu();
				}
			}
			if (viewC64->debugInterfaceAtari)
			{
				if (ImGui::BeginMenu("Atari 800##Settings"))
				{
					if (ImGui::BeginMenu("Video system"))
					{
						bool selected = c64SettingsAtariVideoSystem == ATARI_VIDEO_SYSTEM_PAL;
						if (ImGui::MenuItem("PAL", "", &selected))
						{
							u8 set = ATARI_VIDEO_SYSTEM_PAL;
							C64DebuggerSetSetting("AtariVideoSystem", &set);
							C64DebuggerStoreSettings();
						}
						selected = c64SettingsAtariVideoSystem == ATARI_VIDEO_SYSTEM_NTSC;
						if (ImGui::MenuItem("NTSC", "", &selected))
						{
							u8 set = ATARI_VIDEO_SYSTEM_NTSC;
							C64DebuggerSetSetting("AtariVideoSystem", &set);
							C64DebuggerStoreSettings();
						}
						ImGui::EndMenu();
					}
					
					if (ImGui::BeginMenu("Machine type"))
					{
						std::vector<const char *> optionsAtariMachineTypes;
						optionsAtariMachineTypes.push_back("Atari 400 (16 KB)");
						optionsAtariMachineTypes.push_back("Atari 800 (48 KB)");
						optionsAtariMachineTypes.push_back("Atari 1200XL (64 KB)");
						optionsAtariMachineTypes.push_back("Atari 600XL (16 KB)");
						optionsAtariMachineTypes.push_back("Atari 800XL (64 KB)");
						optionsAtariMachineTypes.push_back("Atari 130XE (128 KB)");
						optionsAtariMachineTypes.push_back("Atari XEGS (64 KB)");
						optionsAtariMachineTypes.push_back("Atari 5200 (16 KB)");
						
						int i = 0;
						for (std::vector<const char *>::iterator it = optionsAtariMachineTypes.begin(); it != optionsAtariMachineTypes.end(); it++)
						{
							const char *machineType = *it;
							bool selected = (c64SettingsAtariMachineType == i);
							if (ImGui::MenuItem(machineType, "", &selected))
							{
								C64DebuggerSetSetting("AtariMachineType", &i);
								C64DebuggerStoreSettings();
							}
							i++;
						}
						ImGui::EndMenu();
					}
					
					if (ImGui::BeginMenu("Ram size"))
					{
						std::vector<const char *> optionsAtariRamSize;
						if (c64SettingsAtariMachineType == 0 || c64SettingsAtariMachineType == 1)
						{
							optionsAtariRamSize.push_back("8 KB");
							optionsAtariRamSize.push_back("16 KB");
							optionsAtariRamSize.push_back("24 KB");
							optionsAtariRamSize.push_back("32 KB");
							optionsAtariRamSize.push_back("40 KB");
							optionsAtariRamSize.push_back("48 KB");
							optionsAtariRamSize.push_back("52 KB");

						}
						else if (c64SettingsAtariMachineType >= 2 && c64SettingsAtariMachineType <= 6)
						{
							optionsAtariRamSize.push_back("16 KB");
							optionsAtariRamSize.push_back("32 KB");
							optionsAtariRamSize.push_back("48 KB");
							optionsAtariRamSize.push_back("64 KB");
							optionsAtariRamSize.push_back("128 KB");
							optionsAtariRamSize.push_back("192 KB");
							optionsAtariRamSize.push_back("320 KB (Rambo)");
							optionsAtariRamSize.push_back("320 KB (Compy-Shop)");
							optionsAtariRamSize.push_back("576 KB");
							optionsAtariRamSize.push_back("1088 KB");
						}
						else if (c64SettingsAtariMachineType == 7)
						{
							optionsAtariRamSize.push_back("16 kB");
						}
						
						int i = 0;
						for (std::vector<const char *>::iterator it = optionsAtariRamSize.begin(); it != optionsAtariRamSize.end(); it++)
						{
							const char *ramSizeOption = *it;
							bool selected = (c64SettingsAtariRamSizeOption == i);
							if (ImGui::MenuItem(ramSizeOption, "", &selected))
							{
								C64DebuggerSetSetting("AtariRamSizeOption", &i);
								C64DebuggerStoreSettings();
							}
							i++;
						}

						ImGui::EndMenu();
					}
					
					bool isPokeyStereo = viewC64->debugInterfaceAtari->IsPokeyStereo();
					if (ImGui::MenuItem("POKEY Stereo", NULL, &isPokeyStereo))
					{
						viewC64->debugInterfaceAtari->SetPokeyStereo(isPokeyStereo);
					}
					
					if (ImGui::MenuItem("Select Atari ROMs folder"))
					{
						// "Set Atari ROMs folder");
						systemDialogOperation = SystemDialogOperationAtari800RomsFolder;
						SYS_DialogPickFolder(this, c64SettingsPathToAtariROMs);
					}
					
					ImGui::EndMenu();
				}
			}
			
			ImGui::Separator();

			if (ImGui::BeginMenu("UI"))
			{
				ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
								
				if (ImGui::BeginMenu("Memory map"))
				{
					if (ImGui::BeginMenu("Memory map values color"))
					{
						std::vector<const char *> options;
						options.push_back("RGB");
						options.push_back("Gray");
						options.push_back("None");
						
						int n = 0;
						for (std::vector<const char *>::iterator it = options.begin(); it != options.end(); it++)
						{
							bool selected = (c64SettingsMemoryValuesStyle == n);
							if (ImGui::MenuItem(*it, NULL, &selected))
							{
								C64DebuggerSetSetting("MemoryValuesStyle", &n);
								C64DebuggerStoreSettings();
							}
							n++;
						}
						ImGui::EndMenu();
					}
					
					if (ImGui::BeginMenu("Memory map markers color"))
					{
						std::vector<const char *> options;
						options.push_back("Default");
						options.push_back("ICU");
						
						int n = 0;
						for (std::vector<const char *>::iterator it = options.begin(); it != options.end(); it++)
						{
							bool selected = (c64SettingsMemoryMarkersStyle == n);
							if (ImGui::MenuItem(*it, NULL, &selected))
							{
								C64DebuggerSetSetting("MemoryMarkersStyle", &n);
								C64DebuggerStoreSettings();
							}
							n++;
						}
						ImGui::EndMenu();
					}
					
					if (ImGui::MenuItem("Invert memory map zoom", NULL, &c64SettingsMemoryMapInvertControl))
					{
						C64DebuggerSetSetting("MemMapInvert", &c64SettingsMemoryMapInvertControl);
						C64DebuggerStoreSettings();
					}

					int step = 1; int step_fast = 10;
					if (ImGui::InputScalar("Markers fade out speed",
										   ImGuiDataType_::ImGuiDataType_S32, &c64SettingsMemoryMapFadeSpeed, &step, &step_fast, "%d",
										   ImGuiInputTextFlags_CharsDecimal))
					{
						if (c64SettingsMemoryMapFadeSpeed < 0)
						{
							c64SettingsMemoryMapFadeSpeed = 1;
						}
						C64DebuggerSetSetting("MemMapFadeSpeed", &c64SettingsMemoryMapFadeSpeed);
						C64DebuggerStoreSettings();
					}

					step = 1; step_fast = 5;
					if (ImGui::InputScalar("Memory map refresh rate",
										   ImGuiDataType_::ImGuiDataType_U8, &c64SettingsMemoryMapRefreshRate, &step, &step_fast, "%d",
										   ImGuiInputTextFlags_CharsDecimal)) // | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (c64SettingsMemoryMapRefreshRate < 0)
						{
							c64SettingsMemoryMapRefreshRate = 1;
						}
						C64DebuggerSetSetting("MemMapRefresh", &c64SettingsMemoryMapRefreshRate);
						C64DebuggerStoreSettings();
					}
					
					/* TODO: Multi-touch map control
					if (ImGui::MenuItem("Multi-touch map control", NULL, &c64SettingsUseMultiTouchInMemoryMap))
					{
						C64DebuggerSetSetting("MemMapMultiTouch", &c64SettingsUseMultiTouchInMemoryMap);
						C64DebuggerStoreSettings();
					}*/
					ImGui::EndMenu();
				}
				
				if (ImGui::BeginMenu("Graphics position format"))
				{
					bool showHex = c64SettingsShowPositionsInHex;
					bool showDecimal = !c64SettingsShowPositionsInHex;
					if (ImGui::MenuItem("Hexadecimal", NULL, &showHex))
					{
						c64SettingsShowPositionsInHex = true;
						C64DebuggerStoreSettings();
					}
					if (ImGui::MenuItem("Decimal", NULL, &showDecimal))
					{
						c64SettingsShowPositionsInHex = false;
						C64DebuggerStoreSettings();
					}
					ImGui::EndMenu();
				}
				
				if (ImGui::BeginMenu("Disassembly"))
				{
					if (ImGui::MenuItem("Execute-aware disassembly", NULL, &c64SettingsExecuteAwareDisassembly))
					{
						C64DebuggerStoreSettings();
					}
#if defined(MACOS)
					if (ImGui::MenuItem("Press CMD to set breakpoint", NULL, &c64SettingsPressCtrlToSetBreakpoint))
#else
					if (ImGui::MenuItem("Press CTRL to set breakpoint", NULL, &c64SettingsPressCtrlToSetBreakpoint))
#endif
					{
						C64DebuggerStoreSettings();
					}
					
					if (ImGui::MenuItem("Use near labels", NULL, &c64SettingsDisassemblyUseNearLabels))
					{
						viewC64->config->SetBool("DisassemblyUseNearLabels", &c64SettingsDisassemblyUseNearLabels);
					}
					if (ImGui::MenuItem("Use near labels also for jumps", NULL, &c64SettingsDisassemblyUseNearLabelsForJumps))
					{
						viewC64->config->SetBool("DisassemblyUseNearLabelsForJumps", &c64SettingsDisassemblyUseNearLabelsForJumps);
					}
					if (ImGui::InputInt("Near label max offset", &c64SettingsDisassemblyNearLabelMaxOffset))
					{
						if (c64SettingsDisassemblyNearLabelMaxOffset < 0)
							c64SettingsDisassemblyNearLabelMaxOffset = 0;
						viewC64->config->SetInt("DisassemblyNearLabelMaxOffset", &c64SettingsDisassemblyNearLabelMaxOffset);
					}
					
					ImGui::EndMenu();
				}
				
				if (ImGui::BeginMenu("Emulator Screen"))
				{
					if (ImGui::MenuItem("Bypass keyboard shortcuts", NULL, &c64SettingsEmulatorScreenBypassKeyboardShortcuts))
					{
						C64DebuggerStoreSettings();
					}
					ImGui::EndMenu();
				}
				
				ImGui::Separator();

				if (ImGui::BeginMenu("Theme style"))
				{
					int currentThemeStyle = VID_GetDefaultImGuiStyle();
					std::vector<const char *> themeStyleNames;
					themeStyleNames.push_back("Dark Alternative");
					themeStyleNames.push_back("Dark");
					themeStyleNames.push_back("Light");
					themeStyleNames.push_back("Classic");
					themeStyleNames.push_back("IntelliJ");
					themeStyleNames.push_back("Photoshop");
					themeStyleNames.push_back("Corporate Grey");
					themeStyleNames.push_back("Corporate Grey 3D");
					themeStyleNames.push_back("Nice");
					
					int i = 0;
					for (std::vector<const char *>::iterator it = themeStyleNames.begin(); it != themeStyleNames.end(); it++)
					{
						const char *name = *it;
						bool selected = (i == currentThemeStyle);
						if (ImGui::MenuItem(name, NULL, &selected))
						{
							VID_SetDefaultImGuiStyle((ImGuiStyleType)i);
						}
						i++;
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Default font"))
				{
					if (ImGui::InputFloat("Font size", &(gDefaultFontSize), 0.5, 1, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue))
					{
						gDefaultFontSize = URANGE(4, gDefaultFontSize, 64);
						viewC64->config->SetFloat("uiDefaultFontSize", &(gDefaultFontSize));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::InputInt("Oversampling", &(gDefaultFontOversampling)))
					{
						gDefaultFontOversampling = URANGE(1, gDefaultFontOversampling, 8);
						viewC64->config->SetInt("uiDefaultFontOversampling", &(gDefaultFontOversampling));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					ImGui::Separator();
					
					// TODO: refactor and generalize font names to allow easy fonts adding
					if (ImGui::MenuItem("Cousine Regular", "", !strcmp(gDefaultFontPath, "CousineRegular")))
					{
						gDefaultFontPath = "CousineRegular";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Sweet16", "", !strcmp(gDefaultFontPath, "Sweet16")))
					{
						gDefaultFontPath = "Sweet16";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Proggy Clean", "", !strcmp(gDefaultFontPath, "ProggyClean")))
					{
						gDefaultFontPath = "ProggyClean";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("ProFontIIx", "", !strcmp(gDefaultFontPath, "ProFontIIx")))
					{
						gDefaultFontPath = "ProFontIIx";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Droid Sans", "", !strcmp(gDefaultFontPath, "DroidSans")))
					{
						gDefaultFontPath = "DroidSans";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Karla Regular", "", !strcmp(gDefaultFontPath, "KarlaRegular")))
					{
						gDefaultFontPath = "KarlaRegular";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Roboto Medium", "", !strcmp(gDefaultFontPath, "RobotoMedium")))
					{
						gDefaultFontPath = "RobotoMedium";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Unifont", "", !strcmp(gDefaultFontPath, "Unifont")))
					{
						gDefaultFontPath = "Unifont";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("PT Mono", "", !strcmp(gDefaultFontPath, "PTMono")))
					{
						gDefaultFontPath = "PTMono";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Plex Sans", "", !strcmp(gDefaultFontPath, "PlexSans")))
					{
						gDefaultFontPath = "PlexSans";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Plex Mono", "", !strcmp(gDefaultFontPath, "PlexMono")))
					{
						gDefaultFontPath = "PlexMono";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Monoki", "", !strcmp(gDefaultFontPath, "Monoki")))
					{
						gDefaultFontPath = "Monoki";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Liberation Sans", "", !strcmp(gDefaultFontPath, "LiberationSans")))
					{
						gDefaultFontPath = "LiberationSans";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Exo Medium", "", !strcmp(gDefaultFontPath, "ExoMedium")))
					{
						gDefaultFontPath = "ExoMedium";
						viewC64->config->SetString("uiDefaultFont", &(gDefaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}
					
					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Interpolation on code font", "", &c64SettingsInterpolationOnDefaultFont))
				{
					guiMain->fntConsole->image->SetLinearScaling(c64SettingsInterpolationOnDefaultFont);
					guiMain->fntConsoleInverted->image->SetLinearScaling(c64SettingsInterpolationOnDefaultFont);
					C64DebuggerStoreSettings();
				}
				
				ImGui::Separator();
				
				bool mainWindowAlwaysOnTop = VID_IsMainWindowAlwaysOnTop();
				if (ImGui::MenuItem("Main window always on top", NULL, &mainWindowAlwaysOnTop))
				{
					guiMain->SetApplicationWindowAlwaysOnTop(mainWindowAlwaysOnTop);
				}
				
				bool viewportsEnable = VID_IsViewportsEnable();
				if (ImGui::MenuItem("Enable floating windows", NULL, &viewportsEnable))
				{
					viewC64->SetViewportsEnable(viewportsEnable);
				}
				bool uiDockingWithShift = true;
				viewC64->config->GetBool("uiDockingWithShift", &uiDockingWithShift, true);
				if (ImGui::MenuItem("Docking with shift", NULL, &uiDockingWithShift))
				{
					viewC64->config->SetBool("uiDockingWithShift", &uiDockingWithShift);
					ImGuiIO& io = ImGui::GetIO();
					io.ConfigDockingWithShift = uiDockingWithShift;
				}
				
				ImGui::Separator();

				if (ImGui::InputFloat("Mouse wheel X scale", &guiMain->mouseScrollWheelScaleX, 0.1, 1))
				{
					viewC64->config->SetFloat("mouseScrollWheelScaleX", &guiMain->mouseScrollWheelScaleX);
				}

				if (ImGui::InputFloat("Mouse wheel Y scale", &guiMain->mouseScrollWheelScaleY, 0.1, 1))
				{
					viewC64->config->SetFloat("mouseScrollWheelScaleY", &guiMain->mouseScrollWheelScaleY);
				}
				ImGui::Separator();
				
				bool uiRaiseWindowOnPass = true;
				viewC64->config->GetBool("uiRaiseWindowOnPass", &uiRaiseWindowOnPass, true);
				if (ImGui::MenuItem("Raise on command line pass", NULL, &uiRaiseWindowOnPass))
				{
					viewC64->config->SetBool("uiRaiseWindowOnPass", &uiRaiseWindowOnPass);
				}
				
				ImGui::Separator();
				if (ImGui::MenuItem("Clear recently opened files lists"))
				{
					ClearRecentlyOpenedFilesLists();
				}
				if (ImGui::MenuItem("Clear Settings to factory default"))
				{
					ClearSettingsToFactoryDefault();
				}
				
				ImGui::PopItemFlag();
				
				ImGui::EndMenu();
			}
									
			ImGui::Separator();
			
			if (ImGui::BeginMenu("Emulation"))
			{
				if (ImGui::MenuItem("Always unpause after reset", "", &c64SettingsAlwaysUnpauseEmulationAfterReset))
				{
					C64DebuggerStoreSettings();
				}
				
				if (ImGui::BeginMenu("Rewind history"))
				{
					if (ImGui::MenuItem("Record snapshots history", "", &c64SettingsSnapshotsRecordIsActive))
					{
						C64DebuggerSetSetting("SnapshotsManagerIsActive", &(c64SettingsSnapshotsRecordIsActive));
						C64DebuggerStoreSettings();
					}
					
					int step = 1; int step_fast = 10;
					if (ImGui::InputScalar("Snapshots interval (frames)",
										   ImGuiDataType_::ImGuiDataType_U32, &c64SettingsSnapshotsIntervalNumFrames, &step, &step_fast, "%d",
										   ImGuiInputTextFlags_CharsDecimal)) // | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (c64SettingsSnapshotsIntervalNumFrames < 1)
						{
							c64SettingsSnapshotsIntervalNumFrames = 1;
						}
						C64DebuggerSetSetting("SnapshotsManagerStoreInterval", &c64SettingsSnapshotsIntervalNumFrames);
						C64DebuggerStoreSettings();
					}
					
					step = 10; step_fast = 100;
					if (ImGui::InputScalar("Max number of snapshots",
										   ImGuiDataType_::ImGuiDataType_U32, &c64SettingsSnapshotsLimit, &step, &step_fast, "%d",
										   ImGuiInputTextFlags_CharsDecimal)) // | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (c64SettingsSnapshotsLimit < 2)
						{
							c64SettingsSnapshotsLimit = 2;
						}
						C64DebuggerSetSetting("SnapshotsManagerLimit", &c64SettingsSnapshotsLimit);
						C64DebuggerStoreSettings();
					}
					int level = (int)c64SettingsTimelineSaveZlibCompressionLevel;
					if (ImGui::InputInt("Save timeline compression level", &level))
					{
						c64SettingsTimelineSaveZlibCompressionLevel = URANGE(1, level, 9);
						C64DebuggerStoreSettings();
					}
					
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();

			// TODO: fix me, create func to do menu %d
			if (ImGui::BeginMenu("Joystick #1"))
			{
				UpdateGamepads();

				bool saveSettings = false;
				bool selected = false;
				int i = 0;
				if (selectedJoystick1 == SelectedJoystick::SelectedJoystickOff)
				{
					selected = true;
				}
				
				if (ImGui::MenuItem("Off", "", &selected))
				{
					selectedJoystick1 = SelectedJoystick::SelectedJoystickOff;
					saveSettings = true;
				}
				
				i++;
				selected = false;
				
				if (selectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard)
				{
					selected = true;
				}
				
				if (ImGui::MenuItem("Keyboard", "", &selected))
				{
					selectedJoystick1 = SelectedJoystick::SelectedJoystickKeyboard;
					saveSettings = true;
				}
				
				i++;
				
				for (std::list<const char *>::iterator it = gamepads->begin(); it != gamepads->end(); it++)
				{
					const char *gamepadName = *it;
					
					bool selected = (selectedJoystick1 == i);
					if (ImGui::MenuItem(gamepadName, "", &selected))
					{
						selectedJoystick1 = i;
						saveSettings = true;
					}
					i++;
				}
				
				if (saveSettings)
				{
					c64SettingsSelectedJoystick1 = selectedJoystick1;
					C64DebuggerStoreSettings();
				}
								
				ImGui::EndMenu();
			}
			// generalize me
			
			if (ImGui::BeginMenu("Joystick #2"))
			{
				UpdateGamepads();

				bool saveSettings = false;
				bool selected = false;
				int i = 0;
				if (selectedJoystick2 == SelectedJoystick::SelectedJoystickOff)
				{
					selected = true;
				}
				
				if (ImGui::MenuItem("Off", "", &selected))
				{
					selectedJoystick2 = SelectedJoystick::SelectedJoystickOff;
					saveSettings = true;
				}
				
				i++;
				selected = false;
				
				if (selectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
				{
					selected = true;
				}
				
				if (ImGui::MenuItem("Keyboard", "", &selected))
				{
					selectedJoystick2 = SelectedJoystick::SelectedJoystickKeyboard;
					saveSettings = true;
				}
				
				i++;
				
				for (std::list<const char *>::iterator it = gamepads->begin(); it != gamepads->end(); it++)
				{
					const char *gamepadName = *it;
					
					bool selected = (selectedJoystick2 == i);
					if (ImGui::MenuItem(gamepadName, "", &selected))
					{
						selectedJoystick2 = i;
						saveSettings = true;
					}
					i++;
				}
				
				if (saveSettings)
				{
					c64SettingsSelectedJoystick2 = selectedJoystick2;
					C64DebuggerStoreSettings();
				}
								
				ImGui::EndMenu();
			}
			
			// TODO: copypasted code ^^^^^^
			
			ImGui::Separator();
			
			if (ImGui::BeginMenu("Audio"))
			{
				bool isVisibleAudioMixer = viewC64->viewAudioMixer->visible;
				if (ImGui::MenuItem(viewC64->viewAudioMixer->name, "", &isVisibleAudioMixer))
				{
					viewC64->viewAudioMixer->SetFocus();
					guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
				}

				if (ImGui::BeginMenu("Audio Device"))
				{
					if (audioDevices == NULL)
					{
						audioDevices = gSoundEngine->EnumerateAvailableOutputDevices();
					}
					
					char *cSelectedDevice = NULL;
					if (c64SettingsAudioOutDevice == NULL)
					{
						cSelectedDevice = STRALLOC("Default");
					}
					else
					{
						cSelectedDevice = c64SettingsAudioOutDevice->GetStdASCII();
					}
					
					int i = 0;
					for (std::list<const char *>::iterator it = audioDevices->begin(); it != audioDevices->end(); it++)
					{
						const char *audioDevice = *it;
						bool selected = !strcmp(cSelectedDevice, audioDevice);
						if (ImGui::MenuItem(audioDevice, "", &selected))
						{
							CSlrString *deviceName = new CSlrString(audioDevice);
							C64DebuggerSetSetting("AudioOutDevice", deviceName);
							C64DebuggerStoreSettings();
							delete deviceName;
						}
						i++;
					}
					STRFREE(cSelectedDevice);

					ImGui::EndMenu();
				}
				
				int bufferNumSamples = 512;
				const char* bufferSizeSamplesStr[] = { "16", "32", "64", "128", "256", "512", "1024", "2048", "4096" };
				const int   bufferSizeSamples[]    = {  16,   32,   64,   128,   256,   512,   1024,   2048,   4096  };
				gApplicationDefaultConfig->GetInt("AudioBufferNumSamples", &bufferNumSamples, 512);
				
				int currentItem = 5;
				for (int i = 0; i < 9; i++)
				{
					if (bufferSizeSamples[i] == bufferNumSamples)
					{
						currentItem = i;
						break;
					}
				}

				if (ImGui::BeginCombo("Buffer samples", bufferSizeSamplesStr[currentItem]))
				{
					for (int i = 0; i < 9; i++)
					{
						bool isSelected = (currentItem == i);
						if (ImGui::Selectable(bufferSizeSamplesStr[i], isSelected))
						{
							bufferNumSamples = bufferSizeSamples[i];
							gApplicationDefaultConfig->SetInt("AudioBufferNumSamples", &bufferNumSamples);
							gSoundEngine->RestartAudioDevice();
						}
						
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				
				ImGui::EndMenu();
			}
			
			ImGui::EndMenu();
		}
		else
		{
			// immediatelly delete audioDevices list to refresh it each time user clicks on Settings
			if (audioDevices != NULL)
			{
				while(!audioDevices->empty())
				{
					const char *deviceName = audioDevices->back();
					audioDevices->pop_back();
					STRFREE(deviceName);
				}
				audioDevices = NULL;
			}
		}
		
		// the same for gamepads
		if (gamepads != NULL)
		{
			while(!gamepads->empty())
			{
				const char *gamepadName = gamepads->back();
				gamepads->pop_back();
				STRFREE(gamepadName);
			}
			gamepads = NULL;
		}
		
		
		//		if (ImGui::BeginMenu("Edit"))
		//		{
		//			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
		//			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
		//			ImGui::Separator();
		//			if (ImGui::MenuItem("Cut", "CTRL+X")) {}
		//			if (ImGui::MenuItem("Copy", "CTRL+C")) {}
		//			if (ImGui::MenuItem("Paste", "CTRL+V")) {}
		//			ImGui::EndMenu();
		//		}

		static bool show_retro_debugger_about = false;
		static bool show_app_metrics = false;
		static bool show_app_style_editor = false;
		static bool show_app_demo = false;
		static bool show_implot_demo = false;
		static bool show_app_about = false;

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Retro Debugger about", "", &show_retro_debugger_about);
			ImGui::Separator();
			ImGui::MenuItem("ImGui Metrics", "", &show_app_metrics);
			ImGui::MenuItem("ImGui Style Editor", "", &show_app_style_editor);
			ImGui::MenuItem("ImGui Demo", "", &show_app_demo);
			ImGui::MenuItem("ImGui Plot demo", "", &show_implot_demo);
			ImGui::MenuItem("ImGui About", "", &show_app_about);
			ImGui::Separator();
			if (ImGui::MenuItem("MTEngineSDL test", "", &(viewC64->viewUiDebug->visible)))
			{
				guiMain->AddViewSkippingLayout(viewC64->viewUiDebug);
				viewC64->viewUiDebug->SetFocus();
			}

			bool isVisible = viewC64->viewMessages->visible;
			if (ImGui::MenuItem("App Messages", "", &isVisible))
			{
				viewC64->viewMessages->SetFocus();
				guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
			}

#if !defined(GLOBAL_DEBUG_OFF)
			isVisible = guiViewDebugLog->visible;
			if (ImGui::MenuItem("Debug Log", "", &isVisible))
			{
				guiViewDebugLog->SetFocus();
				guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
			}
#endif
			// end Help menu
			ImGui::EndMenu();
		}
		
		if (show_retro_debugger_about)
		{
			ImGui::Begin("Retro Debugger v" RETRODEBUGGER_VERSION_STRING " About", &show_retro_debugger_about);
			ImGui::Text("Retro Debugger is a multiplatform debugger APIs host with ImGui implementation.");
			ImGui::Text("(C) 2016-2023 Marcin 'slajerek' Skoczylas, see README for libraries copyright.");
			ImGui::Separator();
			ImGui::Text("");
			ImGui::Text("If you like this tool and you feel that you would like to share with me some beers,");
			ImGui::Text("then you can use this link: http://tinyurl.com/C64Debugger-PayPal");
			ImGui::Text("");
			ImGui::Separator();

			for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
			{
				CDebugInterface *debugInterface = *it;
				CSlrString *versionString = debugInterface->GetEmulatorVersionString();
				char *cVersionString = versionString->GetStdASCII();
				ImGui::Text("%34s is %08x (%srunning)",
						cVersionString,
						(debugInterface),
						debugInterface->isRunning ? "" : "not ");
				STRFREE(cVersionString);
				delete versionString;
			}

//			ImGui::Separator();

			ImGui::End();
		}
		if (show_app_metrics)       { ImGui::ShowMetricsWindow(&show_app_metrics); }
		if (show_app_style_editor)
		{
			ImGui::Begin("Dear ImGui Style Editor", &show_app_style_editor);
			ImGui::ShowStyleEditor();
			ImGui::End();
		}
		if (show_app_demo)         { ImGui::ShowDemoWindow(&show_app_demo); }
		if (show_implot_demo)		{ ImPlot::ShowDemoWindow(); }
		if (show_app_about)         { ImGui::ShowAboutWindow(&show_app_about); }

		// done Help Menu
		
		
		ImGui::EndMainMenuBar();
	}

//	LOGD("layout=%x", layout);
	if (layoutData && openPopupImGuiWorkaround)
	{
		layoutName[0] = 0x00;
		doNotUpdateViewsPosition = false;
		ImGui::OpenPopup("New Workspace");
	}
	
	if (ImGui::BeginPopupModal("New Workspace", NULL, waitingForNewLayoutKeyShortcut ? ImGuiWindowFlags_NoResize : ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (openPopupImGuiWorkaround)
			ImGui::SetKeyboardFocusHere();

		if (waitingForNewLayoutKeyShortcut == false)
		{
			bool saveLayout = false;
			if (ImGui::InputText("Name##NewWorkspacePopup", layoutName, 127, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				saveLayout = true;
			}
			
			ImGui::Checkbox("Preserve views layout", &doNotUpdateViewsPosition);

			if (layoutData->keyShortcut != NULL)
			{
				ImGui::Text("Key shortcut: %s", layoutData->keyShortcut->cstr);
			}

			if (ImGui::Button("Assign keyboard shortcut"))
			{
				waitingForNewLayoutKeyShortcut = true;
			}

			if (ImGui::Button("Cancel"))
			{
				delete layoutData;
				layoutData = NULL;
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Create"))
			{
				saveLayout = true;
			}
			
			if (saveLayout)
			{
				if (layoutName[0] != 0x00)
				{
					layoutData->layoutName = STRALLOC(layoutName);
					layoutData->doNotUpdateViewsPositions = doNotUpdateViewsPosition;
					
					char *buf = SYS_GetCharBuf();
					if (layoutData->keyShortcut)
					{
						sprintf(buf, "Workspace %s", layoutName);
						layoutData->keyShortcut->SetName(buf);
					}
					
					guiMain->layoutManager->AddLayout(layoutData);
					guiMain->layoutManager->currentLayout = layoutData;
					guiMain->layoutManager->StoreLayouts();
					
					sprintf(buf, "Created new workspace: %s", layoutName);
					guiMain->ShowNotification("Information", buf);
					SYS_ReleaseCharBuf(buf);
				}
				else
				{
					delete layoutData;
				}
				layoutData = NULL;
				
				ImGui::CloseCurrentPopup();
			}
		}
		else
		{
			// waiting for key shortcut
			
			ImGui::Text("Hover here your cursor");
			ImGui::Text ("and press a new shortcut key");
		}
		
		openPopupImGuiWorkaround = false;

		ImGui::EndPopup();
	}

}

bool CMainMenuBar::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CMainMenuBar::KeyDown: keyCode=%d %s %s %s %s\n", keyCode, STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl), STRBOOL(isSuper));
	if (keyCode == MTKEY_ESC && guiMain->IsViewFullScreen())
	{
		guiMain->SetViewFullScreen(SetFullScreenMode::ViewLeaveFullScreen, NULL);
		return true;
	}
	
	if (waitingForNewLayoutKeyShortcut)
	{
		if (SYS_IsKeyCodeSpecial(keyCode))
			return false;
		
		if (layoutData->keyShortcut)
		{
			guiMain->RemoveKeyboardShortcut(layoutData->keyShortcut);
			delete layoutData->keyShortcut;
			layoutData->keyShortcut = NULL;
		}
		
		CSlrKeyboardShortcut *findShortcut = guiMain->keyboardShortcuts->FindShortcut(KBZONE_GLOBAL, keyCode, isShift, isAlt, isControl, isSuper);
		if (findShortcut != NULL)
		{
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "Keyboard shortcut %s is already assigned to %s", findShortcut->cstr, findShortcut->name);
			guiMain->ShowMessageBox("Please revise", buf);
			SYS_ReleaseCharBuf(buf);
			waitingForNewLayoutKeyShortcut = false;
			return true;
		}

		// keyboard shortcut name will be updated on save
		layoutData->keyShortcut = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "", keyCode, isShift, isAlt, isControl, guiMain->isSuperPressed, guiMain->layoutManager);
		
		waitingForNewLayoutKeyShortcut = false;
		return true;
	}
	
	if (guiMain->CheckKeyboardShortcut(keyCode))
		return true;
	
	return false;
}

bool CMainMenuBar::ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *shortcut)
{
	LOGD("CMainMenuBar::ProcessKeyboardShortcut");
	shortcut->DebugPrint();
	
	// TODO: GENERALIZE THIS make a list of avaliable interfaces and iterate
	if (viewC64->debugInterfaceC64)
	{
		viewC64->viewC64Screen->KeyUpModifierKeys(shortcut->isShift, shortcut->isAlt, shortcut->isControl);
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->viewAtariScreen->KeyUpModifierKeys(shortcut->isShift, shortcut->isAlt, shortcut->isControl);
	}

	if (viewC64->debugInterfaceNes)
	{
		viewC64->viewNesScreen->KeyUpModifierKeys(shortcut->isShift, shortcut->isAlt, shortcut->isControl);
	}
	
	if (shortcut == kbsCloseWindow)
	{
		guiMain->CloseCurrentImGuiWindow();
		return true;
	}
	
	if (shortcut == kbsSearchWindow)
	{
		OpenSearchWindow();
		return true;
	}
	
	if (shortcut == kbsOpenFile)
	{
		viewC64->viewC64MainMenu->OpenDialogOpenFile();
		return true;
	}
	
	if (shortcut == kbsDetachEverything)
	{
		DetachEverything(true, true);
		return true;
	}
	if (shortcut == kbsDetachCartridge)
	{
		DetachCartridge(true);
		return true;
	}
	if (shortcut == kbsDetachDiskImage)
	{
		DetachDiskImage(true);
		return true;
	}
	if (shortcut == kbsDetachExecutable)
	{
		if (viewC64->debugInterfaceC64->isRunning)
		{
			DetachC64PRG(true);
		}
		if (viewC64->debugInterfaceAtari->isRunning)
		{
			DetachAtariXEX(true);
		}
		return true;
	}

	if (shortcut == kbsCartridgeFreezeButton)
	{
		viewC64->debugInterfaceC64->CartridgeFreezeButtonPressed();
		return true;
	}

	if (shortcut == kbsIsWarpSpeed)
	{
		isWarpSpeed = !isWarpSpeed;
		
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			debugInterface->SetSettingIsWarpSpeed(isWarpSpeed);
		}
		return true;
	}

	if (shortcut == kbsResetCpuCycleAndFrameCounters)
	{
		ResetMainCpuDebugCycleAndFrameCounters();
		return true;
	}

	if (shortcut == kbsClearMemoryMarkers)
	{
		ClearMemoryMarkers();
		return true;
	}

	// snapshots
	if (shortcut == kbsSaveSnapshot)
	{
		viewC64->viewC64Snapshots->OpenDialogSaveSnapshot();
	}
	else if (shortcut == kbsLoadSnapshot)
	{
		viewC64->viewC64Snapshots->OpenDialogLoadSnapshot();
	}
	else if (shortcut == kbsStoreSnapshot1)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(0);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot1)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(0);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot2)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(1);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot2)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(1);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot3)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(2);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot3)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(2);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot4)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(3);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot4)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(3);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot5)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(4);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot5)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(4);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot6)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(5);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot6)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(5);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot7)
	{
		viewC64->viewC64Snapshots->QuickStoreFullSnapshot(6);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot7)
	{
		viewC64->viewC64Snapshots->QuickRestoreFullSnapshot(6);
		return true;
	}
	
	if (viewC64->debugInterfaceC64)
	{
		//
		
//			if (viewC64Snapshots->ProcessKeyboardShortcut(shortcut))
//			{
//				return true;
//			}
		
		if (shortcut == kbsBrowseD64)
		{
			// TODO: workaround for broken scroll to selected item, fix me in viewMenuItem
			bool wasVisible = viewC64->viewDrive1541FileD64->IsVisible();
			
			viewC64->viewDrive1541FileD64->StartBrowsingD64(0);
			if (!viewC64->viewC64Screen->HasFocus())
			{
				guiMain->SetFocus(viewC64->viewDrive1541FileD64);
			}
			
			return true;
		}
		
		if (shortcut == kbsStartFromDisk)
		{
			viewC64->viewDrive1541FileD64->StartDiskPRGEntry(0, true);
			return true;
		}
		
		if (shortcut == kbsInsertD64)
		{
			viewC64->viewC64MainMenu->OpenDialogInsertD64();
			return true;
		}

		if (shortcut == kbsInsertNextD64)
		{
			viewC64->viewC64MainMenu->InsertNextD64();
			return true;
		}
		if (shortcut == kbsAutoJmpFromInsertedDiskFirstPrg)
		{
			viewC64->mainMenuBar->ToggleAutoLoadFromInsertedDisk();
			return true;
		}
		if (shortcut == kbsAutoJmpAlwaysToLoadedPRGAddress)
		{
			ToggleAutoJmpAlwaysToLoadedPRGAddress();
			return true;
		}
		if (shortcut == kbsAutoJmpDoReset)
		{
			ToggleAutoJmpDoReset();
			return true;
		}
		if (shortcut == kbsReloadAndRestart)
		{
			CSlrString *filePath = viewC64->recentlyOpenedFiles->GetMostRecentFilePath();
			if (filePath)
			{
				viewC64->viewC64MainMenu->LoadFile(filePath);
			}
			return true;
		}
		
		// datasette
		if (shortcut == kbsTapeAttach)
		{
			viewC64->viewC64MainMenu->OpenDialogInsertTape();
			return true;
		}
		else if (shortcut == kbsTapeDetach)
		{
			viewC64->debugInterfaceC64->DetachTape();
			viewC64->ShowMessageInfo("Tape detached");
			return true;
		}
		else if (shortcut == kbsTapeStop)
		{
			viewC64->debugInterfaceC64->DatasetteStop();
			viewC64->ShowMessageInfo("Datasette STOP");
			return true;
		}
		else if (shortcut == kbsTapePlay)
		{
			viewC64->debugInterfaceC64->DatasettePlay();
			viewC64->ShowMessageInfo("Datasette PLAY");
			return true;
		}
		else if (shortcut == kbsTapeForward)
		{
			viewC64->debugInterfaceC64->DatasetteForward();
			viewC64->ShowMessageInfo("Datasette FORWARD");
			return true;
		}
		else if (shortcut == kbsTapeRewind)
		{
			viewC64->debugInterfaceC64->DatasetteRewind();
			viewC64->ShowMessageInfo("Datasette REWIND");
			return true;
		}
		
		// TODO: move me
//		if (shortcut == kbsVicEditorExportFile)
//		{
//			viewC64->viewVicEditor->exportMode = VICEDITOR_EXPORT_UNKNOWN;
//			viewC64->viewVicEditor->OpenDialogExportFile();
//			return true;
//		}
		
		if (shortcut == kbsSwitchNextMaximumSpeed)
		{
			SwitchNextMaximumSpeed();
			return true;
		}
		if (shortcut == kbsSwitchPrevMaximumSpeed)
		{
			SwitchPrevMaximumSpeed();
			return true;
		}

		if (shortcut == kbsDumpC64Memory)
		{
			OpenDialogDumpC64Memory();
			return true;
		}
		if (shortcut == kbsDumpDrive1541Memory)
		{
			OpenDialogDumpDrive1541Memory();
			return true;
		}

		if (shortcut == kbsC64ProfilerStartStop)
		{
			C64ProfilerStartStop(-1, false);
			return true;
		}

	}

	if (debugInterfaceAtari)
	{
		if (shortcut == kbsInsertATR)
		{
			viewC64->viewC64MainMenu->OpenDialogInsertATR();
			return true;
		}
	}
	
	if (debugInterfaceNes)
	{
		//
	}
	
	///
	// check emulation scrubbing
	if (shortcut == kbsScrubEmulationBackOneFrame)
	{
		LOGD(">>>>>>>>>................ REWIND -1");
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(-1);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}
	if (shortcut == kbsScrubEmulationForwardOneFrame)
	{
		LOGD(">>>>>>>>>................ FORWARD +1");
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;

			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(+1);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}

	if (shortcut == kbsScrubEmulationBackOneSecond)
	{
		LOGD(">>>>>>>>>................ REWIND -1s");
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;

			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				float emulationFPS = debugInterface->GetEmulationFPS();
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(-emulationFPS);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}
	if (shortcut == kbsScrubEmulationForwardOneSecond)
	{
		LOGD(">>>>>>>>>................ FORWARD +1s");
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				float emulationFPS = debugInterface->GetEmulationFPS();
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(+emulationFPS);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}

	if (shortcut == kbsScrubEmulationBackMultipleFrames)
	{
		int scrubNumberOfFrames = 10;
		gApplicationDefaultConfig->GetInt("StepNumberOfFrames", &scrubNumberOfFrames, 10);
		LOGD(">>>>>>>>>................ REWIND -%d frames", scrubNumberOfFrames);
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(-scrubNumberOfFrames);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}
	
	if (shortcut == kbsScrubEmulationForwardMultipleFrames)
	{
		int scrubNumberOfFrames = 10;
		gApplicationDefaultConfig->GetInt("StepNumberOfFrames", &scrubNumberOfFrames, 10);
		LOGD(">>>>>>>>>................ FORWARD +%d frames", scrubNumberOfFrames);
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				float emulationFPS = debugInterface->GetEmulationFPS();
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(+scrubNumberOfFrames);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}
	
	///
	
	if (shortcut == kbsInsertCartridge)
	{
		viewC64->viewC64MainMenu->OpenDialogInsertCartridge();
		return true;
	}
	else if (shortcut == kbsInsertAtariCartridge)
	{
		viewC64->viewC64MainMenu->OpenDialogInsertAtariCartridge();
		return true;
	}
	
	else if (shortcut == kbsNextCodeSegmentSymbols
			 || shortcut == kbsPreviousCodeSegmentSymbols)
	{
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->symbols)
			{
				if (shortcut == kbsNextCodeSegmentSymbols)
				{
					debugInterface->symbols->SelectNextSegment();
				}
				else
				{
					debugInterface->symbols->SelectPreviousSegment();
				}
				
				/* TODO: display new segment somewhere (create new view for emu segments?)
				char *buf = SYS_GetCharBuf();
				char *buf2 = debugInterface->symbols->currentSegment->name->GetStdASCII();
				sprintf(buf, "Segment: %s", buf2);
				delete [] buf2;
				viewC64->ShowMessageInfo(buf);
				SYS_ReleaseCharBuf(buf);
				 */
			}

		}
	}
	
	// tape
	

	else if (shortcut == kbsDiskDriveReset)
	{
		viewC64->debugInterfaceC64->DiskDriveReset();
		return true;
	}
	else if (shortcut == kbsSoftReset)
	{
		viewC64->SoftReset();
		return true;
	}
	else if (shortcut == kbsHardReset)
	{
		viewC64->HardReset();
		return true;
	}
	else if (shortcut == kbsStepOverInstruction)
	{
		viewC64->StepOverInstruction();
		return true;
	}
	else if (shortcut == kbsStepBackInstruction)
	{
		viewC64->StepBackInstruction();
		return true;
	}
	else if (shortcut == kbsForwardNumberOfCycles)
	{
		int numCycles = 10;
		gApplicationDefaultConfig->GetInt("StepNumberOfCycles", &numCycles, 10);
		
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->GetDebugMode() == DEBUGGER_MODE_RUNNING
				&& !debugInterface->snapshotsManager->IsPerformingSnapshotRestore())
			{
				debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
			}

			u64 currentCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
			debugInterface->snapshotsManager->RestoreSnapshotByCycle(currentCycle + numCycles);

		}
		guiMain->UnlockMutex();
		return true;
	}
	else if (shortcut == kbsStepBackNumberOfCycles)
	{
		int numCycles = 10;
		gApplicationDefaultConfig->GetInt("StepNumberOfCycles", &numCycles, 10);
		
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->GetDebugMode() == DEBUGGER_MODE_RUNNING
				&& !debugInterface->snapshotsManager->IsPerformingSnapshotRestore())
			{
				debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
			}

			u64 currentCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
			debugInterface->snapshotsManager->RestoreSnapshotByCycle(currentCycle - numCycles);

		}
		guiMain->UnlockMutex();
		return true;
	}
	
	else if (shortcut == kbsStepOneCycle)
	{
		viewC64->StepOneCycle();
		return true;
	}
	else if (shortcut == kbsRunContinueEmulation)
	{
		viewC64->RunContinueEmulation();
		return true;
	}
	else if (shortcut == kbsGoToFrame)
	{
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			CViewTimeline *viewTimeline = debugInterface->GetViewTimeline();
			int enteredGoToFrameNum = viewTimeline->enteredGoToFrameNum;
			debugInterface->snapshotsManager->RestoreSnapshotByFrame(enteredGoToFrameNum);
		}
	}
	else if (shortcut == kbsGoToCycle)
	{
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			CViewTimeline *viewTimeline = debugInterface->GetViewTimeline();
			int enteredGoToCycleNum = viewTimeline->enteredGoToCycleNum;
			debugInterface->snapshotsManager->RestoreSnapshotByCycle(enteredGoToCycleNum);
		}
	}

	else if (shortcut == kbsIsDataDirectlyFromRam)
	{
		viewC64->SwitchIsDataDirectlyFromRam();
		return true;
	}
	else if (shortcut == kbsToggleMulticolorImageDump)
	{
		viewC64->SwitchIsMulticolorDataDump();
		return true;
	}
	else if (shortcut == kbsShowRasterBeam)
	{
		viewC64->SwitchIsShowRasterBeam();
		return true;
	}
	
	else if (shortcut == kbsMoveFocusToNextView)
	{
		viewC64->MoveFocusToNextView();
		return true;
	}
	else if (shortcut == kbsMoveFocusToPreviousView)
	{
		viewC64->MoveFocusToPrevView();
		return true;
	}
	else if (shortcut == kbsSaveScreenImageAsPNG)
	{
		viewC64->viewVicEditor->SaveScreenshotAsPNG();
		return true;
	}

	return false;
}

void CMainMenuBar::ResetMainCpuDebugCycleAndFrameCounters()
{
	ResetMainCpuDebugCycleCounter();
	ResetEmulationFrameCounter();
}

void CMainMenuBar::ResetMainCpuDebugCycleCounter()
{
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->ResetMainCpuDebugCycleCounter();
	}
}

void CMainMenuBar::ResetEmulationFrameCounter()
{
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->ResetEmulationFrameCounter();
	}
}

void CMainMenuBar::ClearMemoryMarkers()
{
	// TODO: generalize me. move markers to debuginterface
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->ClearDebugMarkers();
		return;
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->ClearDebugMarkers();
	}

	if (viewC64->debugInterfaceNes)
	{
		viewC64->debugInterfaceNes->ClearDebugMarkers();
	}
}

// TODO: refactor and move below to C64 interface/tools
void CMainMenuBar::CreateSidAddressOptions()
{
	char *buf = SYS_GetCharBuf();

	for (uint16 j = 0x0020; j < 0x0100; j += 0x0020)
	{
		uint16 addr = 0xD400 + j;
		sprintf(buf, "$%04X", addr);
		sidAddressOptions.push_back(STRALLOC(buf));
	}

	for (uint16 i = 0xD500; i < 0xD800; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			sprintf(buf, "$%04X", addr);
			sidAddressOptions.push_back(STRALLOC(buf));
		}
	}

	for (uint16 i = 0xDE00; i < 0xE000; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			sprintf(buf, "$%04X", addr);
			sidAddressOptions.push_back(STRALLOC(buf));
		}
	}
	
	SYS_ReleaseCharBuf(buf);
}

uint16 CMainMenuBar::GetSidAddressFromOptionNum(int optionNum)
{
	int o = 0;
	
	for (uint16 j = 0x0020; j < 0x0100; j += 0x0020)
	{
		uint16 addr = 0xD400 + j;
		
		if (o == optionNum)
			return addr;
		
		o++;
	}
	
	for (uint16 i = 0xD500; i < 0xD800; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			if (o == optionNum)
				return addr;
			
			o++;
		}
	}
	
	for (uint16 i = 0xDE00; i < 0xE000; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			if (o == optionNum)
				return addr;
			
			o++;
		}
	}
	
	LOGError("CMainMenuBar::GetSidAddressFromOptionNum: sid address not correct, option num=%d", optionNum);
	return 0xD420;
}

int CMainMenuBar::GetOptionNumFromSidAddress(uint16 sidAddress)
{
	int o = 0;
	
	for (uint16 j = 0x0020; j < 0x0100; j += 0x0020)
	{
		uint16 addr = 0xD400 + j;
		if (sidAddress == addr)
			return o;
		
		o++;
	}
	
	for (uint16 i = 0xD500; i < 0xD800; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			if (sidAddress == addr)
				return o;
			
			o++;
		}
	}
	
	for (uint16 i = 0xDE00; i < 0xE000; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			if (sidAddress == addr)
				return o;
			
			o++;
		}
	}
	
	LOGError("CMainMenuBar::GetSidAddressFromOptionNum: sid address not correct, sidAddress=%04x", sidAddress);
	return 0;
}

void CMainMenuBar::UpdateSidSettings()
{
	viewC64->viewC64StateSID->UpdateSidButtonsState();
}

void CMainMenuBar::ToggleAutoLoadFromInsertedDisk()
{
	if (c64SettingsAutoJmpFromInsertedDiskFirstPrg)
	{
		viewC64->ShowMessageInfo("Auto load from disk is OFF");
	}
	else
	{
		viewC64->ShowMessageInfo("Auto load from disk is ON");
	}
}

void CMainMenuBar::ToggleAutoJmpAlwaysToLoadedPRGAddress()
{
	if (c64SettingsAutoJmpAlwaysToLoadedPRGAddress)
	{
		viewC64->ShowMessageInfo("Auto JMP to loaded address is OFF");
	}
	else
	{
		viewC64->ShowMessageInfo("Auto JMP to loaded address is ON");
	}
}

void CMainMenuBar::ToggleAutoJmpDoReset()
{
	if (c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD)
	{
		viewC64->ShowMessageInfo("Do not Reset before PRG load");
	}
	else if (c64SettingsAutoJmpDoReset == MACHINE_RESET_NONE)
	{
		viewC64->ShowMessageInfo("Soft Reset before PRG load");
	}
	else if (c64SettingsAutoJmpDoReset == MACHINE_RESET_SOFT)
	{
		viewC64->ShowMessageInfo("Hard Reset before PRG load");
	}
}

//
void CMainMenuBar::SwitchNextMaximumSpeed()
{
	std::vector<int> speeds = { 10, 20, 50, 100, 200, 300, 400, 10 };
	for (int i = 0; i < speeds.size(); i++)
	{
		if (speeds[i] == c64SettingsEmulationMaximumSpeed)
		{
			SetEmulationMaximumSpeed(speeds[i+1]);
			break;
		}
	}
}

void CMainMenuBar::SwitchPrevMaximumSpeed()
{
	std::vector<int> speeds = { 400, 300, 200, 100, 50, 20, 10, 400 };
	for (int i = 0; i < speeds.size(); i++)
	{
		if (speeds[i] == c64SettingsEmulationMaximumSpeed)
		{
			SetEmulationMaximumSpeed(speeds[i+1]);
			break;
		}
	}
}

void CMainMenuBar::SetEmulationMaximumSpeed(int maximumSpeed)
{
	C64DebuggerSetSetting("EmulationMaximumSpeed", &maximumSpeed);
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "Emulation speed set to %d", maximumSpeed);
	viewC64->ShowMessageInfo(buf);
	SYS_ReleaseCharBuf(buf);
}

//
void CMainMenuBar::DetachEverything(bool showMessage, bool storeSettings)
{
	// detach drive, cartridge & tape
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->DetachEverything();
		
		guiMain->LockMutex();
		
		if (c64SettingsPathToD64)
		{
			delete c64SettingsPathToD64;
			c64SettingsPathToD64 = NULL;
		}
		
		if (c64SettingsPathToCartridge)
		{
			delete c64SettingsPathToCartridge;
			c64SettingsPathToCartridge = NULL;
		}
		
		if (c64SettingsPathToPRG)
		{
			delete c64SettingsPathToPRG;
			c64SettingsPathToPRG = NULL;
		}

		if (c64SettingsPathToTAP)
		{
			delete c64SettingsPathToTAP;
			c64SettingsPathToTAP = NULL;
		}

		viewC64->debugInterfaceC64->ClearDebugMarkers();

		guiMain->UnlockMutex();
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->DetachEverything();
		
		guiMain->LockMutex();
		
		if (c64SettingsPathToATR)
		{
			delete c64SettingsPathToATR;
			c64SettingsPathToATR = NULL;
		}
		
		if (c64SettingsPathToAtariCartridge)
		{
			delete c64SettingsPathToAtariCartridge;
			c64SettingsPathToAtariCartridge = NULL;
		}
		
		if (c64SettingsPathToXEX)
		{
			delete c64SettingsPathToXEX;
			c64SettingsPathToXEX = NULL;
		}
		
		if (c64SettingsPathToCAS)
		{
			delete c64SettingsPathToCAS;
			c64SettingsPathToCAS = NULL;
		}

		viewC64->debugInterfaceAtari->ClearDebugMarkers();
		
		guiMain->UnlockMutex();

	}
	
//	if (viewC64->debugInterfaceNes)
//	{
//		viewC64->debugInterfaceNes->DetachEverything();
//
//		guiMain->LockMutex();
//
////		if (viewC64->viewC64MainMenu->menuItemInsertATR->str2 != NULL)
////			delete viewC64->viewC64MainMenu->menuItemInsertATR->str2;
////		viewC64->viewC64MainMenu->menuItemInsertATR->str2 = NULL;
//
//		delete c64SettingsPathToATR;
//		c64SettingsPathToATR = NULL;
//
////		if (viewC64->viewC64MainMenu->menuItemInsertAtariCartridge->str2 != NULL)
////			delete viewC64->viewC64MainMenu->menuItemInsertAtariCartridge->str2;
////		viewC64->viewC64MainMenu->menuItemInsertAtariCartridge->str2 = NULL;
//
//		delete c64SettingsPathToAtariCartridge;
//		c64SettingsPathToAtariCartridge = NULL;
//
////		if (viewC64->viewC64MainMenu->menuItemOpenFile->str2 != NULL)
////			delete viewC64->viewC64MainMenu->menuItemOpenFile->str2;
////		viewC64->viewC64MainMenu->menuItemOpenFile->str2 = NULL;
//
//		delete c64SettingsPathToXEX;
//		c64SettingsPathToXEX = NULL;
//
//		delete c64SettingsPathToCAS;
//		c64SettingsPathToCAS = NULL;
//
//		viewC64->debugInterfaceAtari->ClearDebugMarkers();
//
//		guiMain->UnlockMutex();
//
//	}
	
	if (storeSettings)
	{
		C64DebuggerStoreSettings();
	}
	
	if (showMessage)
	{
		viewC64->ShowMessageInfo("Detached everything");
	}
}

void CMainMenuBar::DetachDiskImage(bool showMessage)
{
	if (viewC64->debugInterfaceC64)
	{
		// detach drive
		viewC64->debugInterfaceC64->DetachDriveDisk();
		
		guiMain->LockMutex();
		
		if (c64SettingsPathToD64)
		{
			delete c64SettingsPathToD64;
			c64SettingsPathToD64 = NULL;
		}
		
		guiMain->UnlockMutex();
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		// detach drive
		viewC64->debugInterfaceAtari->DetachDriveDisk();
		
		guiMain->LockMutex();
		
		if (c64SettingsPathToATR)
		{
			delete c64SettingsPathToATR;
			c64SettingsPathToATR = NULL;
		}
		
		guiMain->UnlockMutex();
	}
	
	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		viewC64->ShowMessageInfo("Drive image detached");
	}
}

void CMainMenuBar::DetachCartridge(bool showMessage)
{
	// detach cartridge
	guiMain->LockMutex();
	
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->DetachCartridge();
		if (c64SettingsPathToCartridge)
		{
			delete c64SettingsPathToCartridge;
			c64SettingsPathToCartridge = NULL;
		}
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->DetachCartridge();
		if (c64SettingsPathToAtariCartridge)
		{
			delete c64SettingsPathToAtariCartridge;
			c64SettingsPathToAtariCartridge = NULL;
		}
	}
	
	guiMain->UnlockMutex();
	
	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		viewC64->ShowMessageInfo("Cartridge detached");
	}
}

void CMainMenuBar::DetachTape(bool showMessage)
{
	// detach tape
	viewC64->debugInterfaceC64->DetachTape();
	
	guiMain->LockMutex();
	
	if (c64SettingsPathToTAP)
	{
		delete c64SettingsPathToTAP;
		c64SettingsPathToTAP = NULL;
	}
	
	guiMain->UnlockMutex();
	
	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		viewC64->ShowMessageInfo("Tape detached");
	}
}

void CMainMenuBar::DetachC64PRG(bool showMessage)
{
	// detach prg
	guiMain->LockMutex();
	
	if (c64SettingsPathToPRG)
	{
		delete c64SettingsPathToPRG;
		c64SettingsPathToPRG = NULL;
	}

	viewC64->debugInterfaceC64->ClearDebugMarkers();
	viewC64->debugInterfaceC64->HardReset();

	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		viewC64->ShowMessageInfo("C64 PRG detached");
	}
}

void CMainMenuBar::DetachAtariXEX(bool showMessage)
{
	// detach xex
	guiMain->LockMutex();
	
	if (c64SettingsPathToXEX)
	{
		delete c64SettingsPathToXEX;
		c64SettingsPathToXEX = NULL;
	}

	viewC64->debugInterfaceAtari->ClearDebugMarkers();
	viewC64->debugInterfaceAtari->HardReset();

	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		viewC64->ShowMessageInfo("Atari XEX detached");
	}
}
// profiler
void CMainMenuBar::SetC64ProfilerOutputFile(CSlrString *path)
{
	path->DebugPrint("CMainMenuBar::SetC64ProfilerOutputFile, path=");
	
	if (c64SettingsC64ProfilerFileOutputPath != path)
	{
		if (c64SettingsC64ProfilerFileOutputPath != NULL)
			delete c64SettingsC64ProfilerFileOutputPath;
		c64SettingsC64ProfilerFileOutputPath = new CSlrString(path);
	}
		
	C64DebuggerStoreSettings();
}

void CMainMenuBar::C64ProfilerStartStop(int runForNumCycles, bool pauseCpuWhenFinished)
{
	if (viewC64->debugInterfaceC64->IsProfilerActive())
	{
		viewC64->debugInterfaceC64->ProfilerDeactivate();
		viewC64->ShowMessageInfo("C64 Profiler stopped");
		return;
	}
	
	if (c64SettingsC64ProfilerFileOutputPath == NULL)
	{
		viewC64->ShowMessageError("Please set the C64 profiler output file before proceeding.");
		return;
	}
	
	char *path = c64SettingsC64ProfilerFileOutputPath->GetStdASCII();
	FILE *fp = fopen(path, "wb");
	if (fp == NULL)
	{
		viewC64->ShowMessageError("C64 Profiler cannot write to the selected file. Ensure the file isn't in use and you have the necessary permissions.");
		return;
	}
	fclose(fp);
	
	viewC64->debugInterfaceC64->ProfilerActivate(path, runForNumCycles, pauseCpuWhenFinished);
	viewC64->ShowMessageInfo("C64 Profiler started");
}

void CMainMenuBar::OpenDialogSetC64ProfilerFileOutputPath()
{
	systemDialogOperation = SystemDialogOperationC64ProfilerFile;
	
	CSlrString *defaultFileName = new CSlrString("c64profile");
	CSlrString *windowTitle = new CSlrString("Set C64 profiler output file");
	viewC64->ShowDialogSaveFile(this, &extensionsProfiler, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

//
void CMainMenuBar::OpenDialogDumpC64Memory()
{
	systemDialogOperation = SystemDialogOperationDumpC64Memory;
	
	CSlrString *defaultFileName = new CSlrString("c64memory");
	CSlrString *windowTitle = new CSlrString("Dump C64 memory");
	viewC64->ShowDialogSaveFile(this, &extensionsMemory, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CMainMenuBar::OpenDialogDumpC64MemoryMarkers()
{
	systemDialogOperation = SystemDialogOperationDumpC64MemoryMarkers;
	
	CSlrString *defaultFileName = new CSlrString("c64markers");
	CSlrString *windowTitle = new CSlrString("Dump C64 memory markers");
	viewC64->ShowDialogSaveFile(this, &extensionsCSV, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CMainMenuBar::OpenDialogDumpDrive1541Memory()
{
	systemDialogOperation = SystemDialogOperationDumpDrive1541Memory;
	
	CSlrString *defaultFileName = new CSlrString("1541memory");
	CSlrString *windowTitle = new CSlrString("Dump Disk 1541 memory");
	viewC64->ShowDialogSaveFile(this, &extensionsMemory, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CMainMenuBar::OpenDialogDumpDrive1541MemoryMarkers()
{
	systemDialogOperation = SystemDialogOperationDumpDrive1541MemoryMarkers;

	CSlrString *defaultFileName = new CSlrString("1541markers");
	CSlrString *windowTitle = new CSlrString("Dump Disk 1541 memory markers");
	viewC64->ShowDialogSaveFile(this, &extensionsCSV, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CMainMenuBar::OpenDialogMapC64MemoryToFile()
{
	systemDialogOperation = SystemDialogOperationMapMemoryToFile;

	CSlrString *defaultFileName = new CSlrString("c64memory");
	
	CSlrString *windowTitle = new CSlrString("Map C64 memory to file");
	viewC64->ShowDialogSaveFile(this, &extensionsMemory, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CMainMenuBar::MapC64MemoryToFile(CSlrString *path)
{
	//path->DebugPrint("CMainMenuBar::MapC64MemoryToFile, path=");
	
	if (c64SettingsPathToC64MemoryMapFile != path)
	{
		if (c64SettingsPathToC64MemoryMapFile != NULL)
			delete c64SettingsPathToC64MemoryMapFile;
		c64SettingsPathToC64MemoryMapFile = new CSlrString(path);
	}
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	
	C64DebuggerStoreSettings();
	guiMain->ShowMessageBox("Information", "Please restart Retro Debugger in order to map memory.", this);
}

void CMainMenuBar::UnmapC64MemoryFromFile()
{
	if (c64SettingsPathToC64MemoryMapFile != NULL)
	{
		systemDialogOperation = SystemDialogOperationMapMemoryToFile;

		delete c64SettingsPathToC64MemoryMapFile;
		c64SettingsPathToC64MemoryMapFile = NULL;
		
		C64DebuggerStoreSettings();
		guiMain->ShowMessageBox("Information", "Please restart Retro Debugger in order to unmap memory from file.", this);
	}
}

//
void CMainMenuBar::OpenDialogSaveTimeline(CDebugInterface *debugInterface)
{
	systemDialogOperation = SystemDialogOperationSaveTimeline;
	selectedDebugInterface = debugInterface;
	CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
	SYS_DialogSaveFile(this, &extensionsTimeline, NULL, defaultFolder, NULL);
	delete defaultFolder;
}

void CMainMenuBar::OpenDialogLoadTimeline()
{
	systemDialogOperation = SystemDialogOperationLoadTimeline;
	// note this does not matter as loading timeline always selects proper debug interface based on the timeline settings stored in the file
	CSlrString *defaultFolder = viewC64->recentlyOpenedFiles->GetCurrentOpenedFolder();
	SYS_DialogOpenFile(this, &extensionsTimeline, defaultFolder, NULL);
	delete defaultFolder;
}

//
// file dialog callbacks
void CMainMenuBar::SystemDialogFileOpenSelected(CSlrString *path)
{
	if (systemDialogOperation == SystemDialogOperationImportLabels)
	{
		CSlrString *ext = path->GetFileExtensionComponentFromPath();
		if (ext->CompareWith("vs"))
		{
			selectedDebugInterface->symbols->ParseSymbols(path);
		}
		else if (ext->CompareWith("dbg"))
		{
			selectedDebugInterface->symbols->ParseSourceDebugInfo(path);
		}
		else if (ext->CompareWith("lbl"))
		{
			LOGTODO("Atari LBL format not supported yet");
		}
		else if (ext->CompareWith("labels"))
		{
			selectedDebugInterface->symbols->DeleteAllSymbols();
			selectedDebugInterface->symbols->LoadLabelsRetroDebuggerFormat(path);
		}
	}
	else if (systemDialogOperation == SystemDialogOperationImportWatches)
	{
		if (selectedDebugInterface->symbols->currentSegment)
		{
			selectedDebugInterface->symbols->currentSegment->DeleteAllWatches();
			selectedDebugInterface->symbols->LoadWatchesRetroDebuggerFormat(path);
		}
	}
	else if (systemDialogOperation == SystemDialogOperationImportBreakpoints)
	{
		if (selectedDebugInterface->symbols->currentSegment)
		{
			selectedDebugInterface->symbols->currentSegment->ClearBreakpoints();
			selectedDebugInterface->symbols->LoadBreakpointsRetroDebuggerFormat(path);
		}
	}

	else if (systemDialogOperation == SystemDialogOperationLoadREU)
	{
		viewC64->viewC64MainMenu->AttachReu(path, true, true);
		C64DebuggerStoreSettings();
	}

	else if (systemDialogOperation == SystemDialogOperationSaveREU)
	{
		viewC64->viewC64MainMenu->SaveReu(path, true, true);
		C64DebuggerStoreSettings();
	}
	
	else if (systemDialogOperation == SystemDialogOperationDumpC64Memory)
	{
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();

		viewC64->debugInterfaceC64->DumpC64Memory(path);
		C64DebuggerStoreSettings();
	}
	else if (systemDialogOperation == SystemDialogOperationDumpC64MemoryMarkers)
	{
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();

		viewC64->debugInterfaceC64->DumpC64MemoryMarkers(path);
		C64DebuggerStoreSettings();
	}
	else if (systemDialogOperation == SystemDialogOperationDumpDrive1541Memory)
	{
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();

		viewC64->debugInterfaceC64->DumpDisk1541Memory(path);
		C64DebuggerStoreSettings();
	}
	else if (systemDialogOperation == SystemDialogOperationDumpDrive1541MemoryMarkers)
	{
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();

		viewC64->debugInterfaceC64->DumpDisk1541MemoryMarkers(path);
		C64DebuggerStoreSettings();
	}
	else if (systemDialogOperation == SystemDialogOperationC64ProfilerFile)
	{
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();

		SetC64ProfilerOutputFile(path);
		C64DebuggerStoreSettings();
	}
	else if (systemDialogOperation == SystemDialogOperationLoadTimeline)
	{
		viewC64->recentlyOpenedFiles->Add(path);
		CViewTimeline::LoadTimeline(path);
	}
	// save
	else if (systemDialogOperation == SystemDialogOperationExportLabels)
	{
		selectedDebugInterface->symbols->SaveLabelsRetroDebuggerFormat(path);
	}
	else if (systemDialogOperation == SystemDialogOperationExportWatches)
	{
		selectedDebugInterface->symbols->SaveWatchesRetroDebuggerFormat(path);
	}
	else if (systemDialogOperation == SystemDialogOperationExportBreakpoints)
	{
		selectedDebugInterface->symbols->SaveBreakpointsRetroDebuggerFormat(path);
	}
	else if (systemDialogOperation == SystemDialogOperationMapMemoryToFile)
	{
		MapC64MemoryToFile(path);
	}
	else if (systemDialogOperation == SystemDialogOperationSaveTimeline)
	{
		CViewTimeline::SaveTimeline(path, selectedDebugInterface);
	}
	
	// pick folder
	else if (systemDialogOperation == SystemDialogOperationC64RomsFolder)
	{
		if (c64SettingsPathToC64Roms)
			delete c64SettingsPathToC64Roms;
		c64SettingsPathToC64Roms = new CSlrString(path);
		
		CDebugInterfaceVice *vice = (CDebugInterfaceVice*)viewC64->debugInterfaceC64;
		char *p = path->GetStdASCII();
		vice->ScanFolderForRoms(p);
		STRFREE(p);
		
		C64DebuggerStoreSettings();
	}
	else if (systemDialogOperation == SystemDialogOperationAtari800RomsFolder)
	{
		if (c64SettingsPathToAtariROMs != NULL)
		{
			delete c64SettingsPathToAtariROMs;
		}
		
		c64SettingsPathToAtariROMs = path;
		C64DebuggerStoreSettings();
		
		guiMain->ShowMessageBox("Information", "Atari ROMs folder selected. Please restart Retro Debugger to finalize and confirm the Atari ROMs path.", this);
		
		/*
		 TODO: someday soon maybe
		guiMain->LockMutex();
		
		viewC64->debugInterfaceAtari->RestartEmulation();
		
		guiMain->UnlockMutex();*/
	}
}

void CMainMenuBar::SystemDialogFileOpenCancelled()
{
}

void CMainMenuBar::SystemDialogFileSaveSelected(CSlrString *path)
{
	SystemDialogFileOpenSelected(path);
}

void CMainMenuBar::SystemDialogFileSaveCancelled()
{
}

void CMainMenuBar::SystemDialogPickFolderSelected(CSlrString *path)
{
	SystemDialogFileOpenSelected(path);
}

void CMainMenuBar::SystemDialogPickFolderCancelled()
{
}

void CMainMenuBar::ClearRecentlyOpenedFilesLists()
{
	viewC64->recentlyOpenedFiles->Clear();
	viewC64->recentlyOpenedFiles->StoreToSettings();

	viewC64->viewC64Charset->recentlyOpened->Clear();
	viewC64->viewC64Charset->recentlyOpened->StoreToSettings();

	viewC64->viewC64StateSID->recentlyOpened->Clear();
	viewC64->viewC64StateSID->recentlyOpened->StoreToSettings();

	viewC64->viewVicEditor->recentlyOpened->Clear();
	viewC64->viewVicEditor->recentlyOpened->StoreToSettings();

}

void CMainMenuBar::ClearSettingsToFactoryDefault()
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	CSlrString *fileName = new CSlrString(C64D_SETTINGS_FILE_PATH);
	byteBuffer->storeToSettings(fileName);
	
	fileName->Set(C64D_KEYBOARD_SHORTCUTS_FILE_PATH);
	byteBuffer->storeToSettings(fileName);
	
	fileName->Set(C64D_KEYMAP_FILE_PATH);
	byteBuffer->storeToSettings(fileName);

	fileName->Set(C64D_LAYOUTS_FILE_NAME);
	byteBuffer->storeToSettings(fileName);
	
	fileName->Set(C64D_RECENTS_FILE_NAME);
	byteBuffer->storeToSettings(fileName);
	
	fileName->Set(APPLICATION_DEFAULT_CONFIG_HJSON_FILE_PATH);
	byteBuffer->storeToSettings(fileName);
	
	delete fileName;
	delete byteBuffer;
		
	guiMain->ShowMessageBox("Information", "Settings have been cleared. For changes to take effect, please restart Retro Debugger.", this);

}

void CMainMenuBar::MessageBoxCallback()
{
	if (systemDialogOperation == SystemDialogOperationAtari800RomsFolder)
	{
		LOGD("CMainMenuBar::MessageBoxCallback: SystemDialogOperationAtari800RomsFolder selected, shutdown app");
		SYS_RestartApplication();
	}

	if (systemDialogOperation == SystemDialogOperationMapMemoryToFile)
	{
		LOGD("CMainMenuBar::MessageBoxCallback: SystemDialogOperationMapMemoryToFile selected, shutdown app");
		SYS_RestartApplication();
	}

	if (systemDialogOperation == SystemDialogOperationClearSettings)
	{
		LOGD("CMainMenuBar::MessageBoxCallback: SystemDialogOperationClearSettings selected, shutdown app");
		SYS_RestartApplication();
	}
}

//
void CMainMenuBar::OpenSearchWindow()
{
	guiMain->LockMutex();
	
	viewSearchWindow->ClearItems();
	for (std::list<CGuiView *>::iterator it = guiMain->views.begin(); it != guiMain->views.end(); it++)
	{
		CGuiView *view = *it;
		if (view != viewSearchWindow)
			viewSearchWindow->items.push_back(view->name);
	}

	if (!viewSearchWindow->IsVisible())
	{
		guiMain->AddViewSkippingLayout(viewSearchWindow);
	}
	
	viewSearchWindow->SetVisible(true);
	viewSearchWindow->ActivateView();
	
	guiMain->UnlockMutex();
}

void CMainMenuBar::GuiViewSearchCompleted(u32 index)
{
	LOGD("CMainMenuBar::GuiViewSearchCompleted, index=%d");
	
	guiMain->LockMutex();
	int i = 0;
	for (std::list<CGuiView *>::iterator it = guiMain->views.begin(); it != guiMain->views.end(); it++)
	{
		if (i == index)
		{
			CGuiView *view = *it;
			view->SetFocus();
			break;
		}
		i++;
	}
	viewSearchWindow->SetVisible(false);
	guiMain->UnlockMutex();
}

