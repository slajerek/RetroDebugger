#ifndef _GUI_MAIN_MENU_BAR_
#define _GUI_MAIN_MENU_BAR_

#include "SYS_Defs.h"
#include "CSlrKeyboardShortcuts.h"
#include "SYS_FileSystem.h"
#include "CGuiMain.h"
#include "CGuiViewProgressBarWindow.h"
#include "CGuiViewSearch.h"
#include "CGlobalKeyboardCallback.h"
#include <vector>

class CByteBuffer;
class CDebugInterface;

enum SelectedJoystick : u8
{
	SelectedJoystickOff = 0,
	SelectedJoystickKeyboard = 1,
	SelectedJoystickGamepad1 = 2,
	SelectedJoystickGamepad2 = 3,
	SelectedJoystickGamepad3 = 4,
	SelectedJoystickGamepad4 = 5
};

enum SystemDialogOperation : u8
{
	SystemDialogOperationC64RomsFolder = 0,
	SystemDialogOperationC64Kernal,
	SystemDialogOperationC64Basic,
	SystemDialogOperationC64Chargen,
	SystemDialogOperationDrive1541,
	SystemDialogOperationDrive1541ii,
	SystemDialogOperationAtari800RomsFolder,
	SystemDialogOperationExportLabels,
	SystemDialogOperationImportLabels,
	SystemDialogOperationExportWatches,
	SystemDialogOperationImportWatches,
	SystemDialogOperationExportBreakpoints,
	SystemDialogOperationImportBreakpoints,
	SystemDialogOperationSaveREU,
	SystemDialogOperationLoadREU,
	SystemDialogOperationDumpC64Memory,
	SystemDialogOperationDumpC64MemoryMarkers,
	SystemDialogOperationDumpDrive1541Memory,
	SystemDialogOperationDumpDrive1541MemoryMarkers,
	SystemDialogOperationMapMemoryToFile,
	SystemDialogOperationC64ProfilerFile,
	SystemDialogOperationSaveTimeline,
	SystemDialogOperationLoadTimeline,
	SystemDialogOperationNESRomsFolder,
	SystemDialogOperationClearSettings
};


class CMainMenuBar : public CSlrKeyboardShortcutCallback, public CSystemFileDialogCallback, CUiMessageBoxCallback, public CGuiViewProgressBarWindowCallback, public CGuiViewSearchCallback, CGlobalKeyboardCallback
{
public:
	CMainMenuBar();
	void RenderImGui();
	
	//
	std::list<CSlrString *> extensionsImportLabels;
	std::list<CSlrString *> extensionsExportLabels;
	std::list<CSlrString *> extensionsWatches;
	std::list<CSlrString *> extensionsBreakpoints;
	std::list<CSlrString *> extensionsREU;
	std::list<CSlrString *> extensionsMemory;
	std::list<CSlrString *> extensionsCSV;
	std::list<CSlrString *> extensionsProfiler;
	std::list<CSlrString *> extensionsTimeline;

	//
	CLayoutData *layoutData;
	char layoutName[128];
	bool doNotUpdateViewsPosition;
	bool waitingForNewLayoutKeyShortcut;
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	// for keyboard shortcut
	virtual void GlobalPreKeyDownCallback(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	std::vector<const char *> sidTypes;
	std::vector<const char *> *c64ModelTypeNames;
	std::vector<int> *c64ModelTypeIds;

	std::list<const char *> *audioDevices;
	std::list<const char *> *gamepads;
	
	void UpdateGamepads();
	
	// TODO: create keyboard as input device/gamepad
	// TODO: move me to settings
	int selectedJoystick1;
	int selectedJoystick2;
	
	virtual bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut);
	
	//
	CSlrKeyboardShortcut *kbsQuitApplication;
	CSlrKeyboardShortcut *kbsCloseWindow;
	
	CSlrKeyboardShortcut *kbsResetCpuCycleAndFrameCounters;
	void ResetMainCpuDebugCycleAndFrameCounters();
	void ResetMainCpuDebugCycleCounter();
	void ResetEmulationFrameCounter();

	CSlrKeyboardShortcut *kbsClearMemoryMarkers;
	void ClearMemoryMarkers();

	CSlrKeyboardShortcut *kbsInsertD64;
	CSlrKeyboardShortcut *kbsBrowseD64;
//	CViewC64MenuItem *menuItemBrowseD64;
	CSlrKeyboardShortcut *kbsInsertNextD64;

	// C64 tape
	CSlrKeyboardShortcut *kbsTapeAttach;
	CSlrKeyboardShortcut *kbsTapeDetach;
	CSlrKeyboardShortcut *kbsTapeStop;
	CSlrKeyboardShortcut *kbsTapePlay;
	CSlrKeyboardShortcut *kbsTapeForward;
	CSlrKeyboardShortcut *kbsTapeRewind;
	CSlrKeyboardShortcut *kbsTapeRecord;

	CSlrKeyboardShortcut *kbsDumpC64Memory;
	CSlrKeyboardShortcut *kbsDumpDrive1541Memory;

	CSlrKeyboardShortcut *kbsInsertATR;

	CSlrKeyboardShortcut *kbsStartFromDisk;
	//	CViewC64MenuItem *menuStartFromDisk;
	
	CSlrKeyboardShortcut *kbsOpenFile;
	CSlrKeyboardShortcut *kbsReloadAndRestart;
	CSlrKeyboardShortcut *kbsResetSoft;
	CSlrKeyboardShortcut *kbsResetHard;
	CSlrKeyboardShortcut *kbsDiskDriveReset;

	CSlrKeyboardShortcut *kbsDetachEverything;
	CSlrKeyboardShortcut *kbsDetachCartridge;
	CSlrKeyboardShortcut *kbsDetachDiskImage;
	CSlrKeyboardShortcut *kbsDetachExecutable;

	CSlrKeyboardShortcut *kbsCartridgeFreezeButton;

	CSlrKeyboardShortcut *kbsIsWarpSpeed;
	bool isWarpSpeed;

	CSlrKeyboardShortcut *kbsSwitchNextMaximumSpeed;
	CSlrKeyboardShortcut *kbsSwitchPrevMaximumSpeed;
	
	CSlrKeyboardShortcut *kbsC64ProfilerStartStop;

	CSlrKeyboardShortcut *kbsSnapshotsC64;

	CSlrKeyboardShortcut *kbsSnapshotsAtari;

	CSlrKeyboardShortcut *kbsSnapshotsNes;

	CSlrKeyboardShortcut *kbsBreakpointsC64;
	CSlrKeyboardShortcut *kbsBreakpointsAtari;
	CSlrKeyboardShortcut *kbsBreakpointsNes;

	CSlrKeyboardShortcut *kbsInsertCartridge;

	CSlrKeyboardShortcut *kbsInsertAtariCartridge;

	CSlrKeyboardShortcut *kbsSettings;

	CSlrKeyboardShortcut *kbsMoveFocusToNextView;
	CSlrKeyboardShortcut *kbsMoveFocusToPreviousView;
	
	//
	// general
	CSlrKeyboardShortcut *kbsStepOverInstruction;
	CSlrKeyboardShortcut *kbsStepBackInstruction;
	CSlrKeyboardShortcut *kbsStepOneCycle;
	CSlrKeyboardShortcut *kbsRunContinueEmulation;

	CSlrKeyboardShortcut *kbsIsDataDirectlyFromRam;
	CSlrKeyboardShortcut *kbsToggleMulticolorImageDump;
	CSlrKeyboardShortcut *kbsShowRasterBeam;
	
	CSlrKeyboardShortcut *kbsSaveScreenImageAsPNG;

	CSlrKeyboardShortcut *kbsCopyToClipboard;
	CSlrKeyboardShortcut *kbsCopyAlternativeToClipboard;
	CSlrKeyboardShortcut *kbsPasteFromClipboard;
	CSlrKeyboardShortcut *kbsPasteAlternativeFromClipboard;
	
	CSlrKeyboardShortcut *kbsNextCodeSegmentSymbols;
	CSlrKeyboardShortcut *kbsPreviousCodeSegmentSymbols;
	
	CSlrKeyboardShortcut *kbsScrubEmulationBackOneFrame;
	CSlrKeyboardShortcut *kbsScrubEmulationForwardOneFrame;
	CSlrKeyboardShortcut *kbsScrubEmulationBackOneSecond;
	CSlrKeyboardShortcut *kbsScrubEmulationForwardOneSecond;
	CSlrKeyboardShortcut *kbsScrubEmulationBackMultipleFrames;
	CSlrKeyboardShortcut *kbsScrubEmulationForwardMultipleFrames;
	CSlrKeyboardShortcut *kbsStepBackNumberOfCycles;
	CSlrKeyboardShortcut *kbsForwardNumberOfCycles;
	CSlrKeyboardShortcut *kbsGoToFrame;
	CSlrKeyboardShortcut *kbsGoToCycle;

	// snapshots
	CSlrKeyboardShortcut *kbsSaveSnapshot;
	CSlrKeyboardShortcut *kbsLoadSnapshot;

	CSlrKeyboardShortcut *kbsStoreSnapshot1;
	CSlrKeyboardShortcut *kbsStoreSnapshot2;
	CSlrKeyboardShortcut *kbsStoreSnapshot3;
	CSlrKeyboardShortcut *kbsStoreSnapshot4;
	CSlrKeyboardShortcut *kbsStoreSnapshot5;
	CSlrKeyboardShortcut *kbsStoreSnapshot6;
	CSlrKeyboardShortcut *kbsStoreSnapshot7;

	CSlrKeyboardShortcut *kbsRestoreSnapshot1;
	CSlrKeyboardShortcut *kbsRestoreSnapshot2;
	CSlrKeyboardShortcut *kbsRestoreSnapshot3;
	CSlrKeyboardShortcut *kbsRestoreSnapshot4;
	CSlrKeyboardShortcut *kbsRestoreSnapshot5;
	CSlrKeyboardShortcut *kbsRestoreSnapshot6;
	CSlrKeyboardShortcut *kbsRestoreSnapshot7;

	// joystick
	CSlrKeyboardShortcut *kbsJoystickUp;
	CSlrKeyboardShortcut *kbsJoystickDown;
	CSlrKeyboardShortcut *kbsJoystickLeft;
	CSlrKeyboardShortcut *kbsJoystickRight;
	CSlrKeyboardShortcut *kbsJoystickFire;
	CSlrKeyboardShortcut *kbsJoystickFireB;
	CSlrKeyboardShortcut *kbsJoystickStart;
	CSlrKeyboardShortcut *kbsJoystickSelect;
	
	// disassembly
	CSlrKeyboardShortcut *kbsToggleBreakpoint;
	CSlrKeyboardShortcut *kbsMakeJmp;
	CSlrKeyboardShortcut *kbsStepOverSubroutine;
	CSlrKeyboardShortcut *kbsToggleTrackPC;
	
	// memory dump & disassembly
	CSlrKeyboardShortcut *kbsGoToAddress;
	
	// c64
	CSlrKeyboardShortcut *kbsAutoJmpFromInsertedDiskFirstPrg;
	CSlrKeyboardShortcut *kbsAutoJmpAlwaysToLoadedPRGAddress;
	CSlrKeyboardShortcut *kbsAutoJmpDoReset;
	
		
	//
	void SwitchNextMaximumSpeed();
	void SwitchPrevMaximumSpeed();
	void SetEmulationMaximumSpeed(int maximumSpeed);

	//
	void DetachEverything(bool showMessage, bool storeSettings);
	void DetachCartridge(bool showMessage);
	void DetachDiskImage(bool showMessage);
	void DetachTape(bool showMessage);
	void DetachC64PRG(bool showMessage);
	void DetachAtariXEX(bool showMessage);

	//
	void CreateSidAddressOptions();
	std::vector<const char *> sidAddressOptions;
	uint16 GetSidAddressFromOptionNum(int optionNum);
	int GetOptionNumFromSidAddress(uint16 sidAddress);
	void UpdateSidSettings();

	//
	void ToggleAutoLoadFromInsertedDisk();
	void ToggleAutoJmpAlwaysToLoadedPRGAddress();
	void ToggleAutoJmpDoReset();
	
	//
	void OpenDialogDumpC64Memory();
	void OpenDialogDumpC64MemoryMarkers();
	void OpenDialogDumpDrive1541Memory();
	void OpenDialogDumpDrive1541MemoryMarkers();

	//
	void OpenDialogSetC64ProfilerFileOutputPath();
	void SetC64ProfilerOutputFile(CSlrString *path);
	void C64ProfilerStartStop(int runForNumCycles, bool pauseCpuWhenFinished);

	//
	void OpenDialogMapC64MemoryToFile();
	void MapC64MemoryToFile(CSlrString *path);
	void UnmapC64MemoryFromFile();

	//
	void OpenDialogLoadTimeline();
	void OpenDialogSaveTimeline(CDebugInterface *debugInterface);
	
	// file dialog callbacks
	int systemDialogOperation;
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	virtual void SystemDialogPickFolderSelected(CSlrString *path);
	virtual void SystemDialogPickFolderCancelled();

	virtual void MessageBoxCallback();

	void ClearRecentlyOpenedFilesLists();
	void ClearSettingsToFactoryDefault();
	
	CDebugInterface *selectedDebugInterface;
	
	// search view
	CGuiViewSearch *viewSearchWindow;
	CSlrKeyboardShortcut *kbsSearchWindow;
	void OpenSearchWindow();
	virtual void GuiViewSearchCompleted(u32 index);
	
	//
	char message[1024];
	
};

#endif //_GUI_MAIN_MENU_BAR_
