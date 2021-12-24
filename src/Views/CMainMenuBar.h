#ifndef _GUI_MAIN_MENU_BAR_
#define _GUI_MAIN_MENU_BAR_

#include "SYS_Defs.h"
#include "CSlrKeyboardShortcuts.h"
#include "SYS_FileSystem.h"
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
	SystemDialogOperationSaveREU,
	SystemDialogOperationLoadREU
};


class CMainMenuBar : public CSlrKeyboardShortcutCallback, CSystemFileDialogCallback
{
public:
	CMainMenuBar();
	void RenderImGui();
	
	//
	std::list<CSlrString *> extensionsImportLabels;
	std::list<CSlrString *> extensionsExportLabels;
	std::list<CSlrString *> extensionsWatches;
	std::list<CSlrString *> extensionsREU;

	//
	CLayoutData *layoutData;
	char layoutName[128];
	bool doNotUpdateViewsPosition;
	
	std::vector<const char *> sidTypes;
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
	CSlrKeyboardShortcut *kbsMainMenuScreen;
	CSlrKeyboardShortcut *kbsVicEditorScreen;
	
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

	CSlrKeyboardShortcut *kbsInsertATR;

	CSlrKeyboardShortcut *kbsStartFromDisk;
	//	CViewC64MenuItem *menuStartFromDisk;
	
	CSlrKeyboardShortcut *kbsOpenFile;
	CSlrKeyboardShortcut *kbsReloadAndRestart;
	CSlrKeyboardShortcut *kbsSoftReset;
	CSlrKeyboardShortcut *kbsHardReset;
	CSlrKeyboardShortcut *kbsDiskDriveReset;

	CSlrKeyboardShortcut *kbsIsWarpSpeed;
	bool isWarpSpeed;

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
	CSlrKeyboardShortcut *kbsStepBackMultipleInstructions;
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
	
	// joystick
	CSlrKeyboardShortcut *kbsJoystickUp;
	CSlrKeyboardShortcut *kbsJoystickDown;
	CSlrKeyboardShortcut *kbsJoystickLeft;
	CSlrKeyboardShortcut *kbsJoystickRight;
	CSlrKeyboardShortcut *kbsJoystickFire;
	CSlrKeyboardShortcut *kbsJoystickFireB;
	CSlrKeyboardShortcut *kbsJoystickStart;
	CSlrKeyboardShortcut *kbsJoystickSelect;
	
	// disassemble
	CSlrKeyboardShortcut *kbsToggleBreakpoint;
	CSlrKeyboardShortcut *kbsMakeJmp;
	CSlrKeyboardShortcut *kbsStepOverJsr;
	CSlrKeyboardShortcut *kbsToggleTrackPC;
	
	// memory dump & disassemble
	CSlrKeyboardShortcut *kbsGoToAddress;
	
	// vic editor
	CSlrKeyboardShortcut *kbsVicEditorCreateNewPicture;
	CSlrKeyboardShortcut *kbsVicEditorPreviewScale;
	CSlrKeyboardShortcut *kbsVicEditorShowCursor;
	CSlrKeyboardShortcut *kbsVicEditorDoUndo;
	CSlrKeyboardShortcut *kbsVicEditorDoRedo;
	CSlrKeyboardShortcut *kbsVicEditorOpenFile;
	CSlrKeyboardShortcut *kbsVicEditorExportFile;
	CSlrKeyboardShortcut *kbsVicEditorSaveVCE;
	CSlrKeyboardShortcut *kbsVicEditorLeaveEditor;
	CSlrKeyboardShortcut *kbsVicEditorClearScreen;
	CSlrKeyboardShortcut *kbsVicEditorRectangleBrushSizePlus;
	CSlrKeyboardShortcut *kbsVicEditorRectangleBrushSizeMinus;
	CSlrKeyboardShortcut *kbsVicEditorCircleBrushSizePlus;
	CSlrKeyboardShortcut *kbsVicEditorCircleBrushSizeMinus;
	CSlrKeyboardShortcut *kbsVicEditorToggleAllWindows;
	
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowPreview;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowPalette;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowLayers;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowCharset;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowSprite;
	CSlrKeyboardShortcut *kbsVicEditorToggleSpriteFrames;
	CSlrKeyboardShortcut *kbsVicEditorToggleTopBar;
	CSlrKeyboardShortcut *kbsVicEditorToggleToolBox;

	CSlrKeyboardShortcut *kbsVicEditorSelectNextLayer;
	
	//
	CSlrKeyboardShortcut *kbsShowWatch;
	
	// file dialog callbacks
	int systemDialogOperation;
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	virtual void SystemDialogPickFolderSelected(CSlrString *path);
	virtual void SystemDialogPickFolderCancelled();

	//
	bool waitingForKeyShortcut;
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	CDebugInterface *selectedDebugInterface;
};

#endif //_GUI_MAIN_MENU_BAR_
