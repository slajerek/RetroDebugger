#include "GUI_Main.h"
#include "CMainMenuBar.h"
#include "CViewC64.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "C64SettingsStorage.h"
#include "CViewKeyboardShortcuts.h"
#include "CSlrKeyboardShortcuts.h"
#include "C64KeyboardShortcuts.h"
#include "CSnapshotsManager.h"
#include "CGuiView.h"
#include "SND_SoundEngine.h"
#include "GAM_GamePads.h"
#include "CViewC64Screen.h"
#include "CViewAtariScreen.h"
#include "CViewNesScreen.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugAsmSource.h"
#include "CViewFileD64.h"
#include "CViewDataDump.h"
#include "CViewSnapshots.h"
#include "CLayoutManager.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "C64DebuggerPluginCrtMaker.h"
#include "C64DebuggerPluginDNDK.h"
#include "CViewMemoryMap.h"

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

	// TODO: fixme, enum added, but we need to fix the problem that in code we have a check for gamepads joysticknum -2, do normal generic controller api so keyboard joystick controller is just a pointer

	selectedJoystick1 = 0;
	selectedJoystick2 = 0;

	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->GetSidTypes(&sidTypes);
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
	
	
	//
	kbsResetCpuCycleAndFrameCounters = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Reset emulation counters", MTKEY_BACKSPACE, true, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsResetCpuCycleAndFrameCounters);

	//
	kbsClearMemoryMarkers = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Clear Memory markers", MTKEY_BACKSPACE, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsClearMemoryMarkers);

	//
	
#if defined(RUN_COMMODORE64)
	
	if (viewC64->debugInterfaceC64)
	{
//		kbsInsertD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Device #8", '8', false, false, true, false, this);
//		guiMain->AddKeyboardShortcut(kbsInsertD64);
		
		//
		kbsInsertNextD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert next disk to Device #8", '8', false, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsInsertNextD64);

		kbsStartFromDisk = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Start from Device #8", MTKEY_F3, false, false, false, false, this);
		guiMain->AddKeyboardShortcut(kbsStartFromDisk);
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
	kbsToggleBreakpoint = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLE, "Toggle Breakpoint", '`', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsToggleBreakpoint);

	// code run control
	kbsStepOverInstruction = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step over instruction", MTKEY_F10, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepOverInstruction);

	kbsStepOverJsr = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLE, "Step over JSR", MTKEY_F10, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStepOverJsr);

	kbsStepBackInstruction = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step back instruction", MTKEY_F10, false, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepBackInstruction);

	kbsStepBackMultipleInstructions = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Multiple step back", MTKEY_F10, true, true, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepBackMultipleInstructions);

	kbsStepOneCycle = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Step one cycle", MTKEY_F10, true, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsStepOneCycle);

	kbsRunContinueEmulation = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Run/Continue code", MTKEY_F11, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsRunContinueEmulation);

	//
	
	kbsMakeJmp = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLE, "Make JMP", 'j', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsMakeJmp);

	kbsToggleTrackPC = new CSlrKeyboardShortcut(KBZONE_DISASSEMBLE, "Toggle track PC", ' ', false, false, false, false, this);
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

	// vic editor
	kbsVicEditorCreateNewPicture = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "VIC Editor: New Picture", 'n', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorCreateNewPicture);

	kbsVicEditorPreviewScale = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "VIC Editor: Preview scale", '/', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorPreviewScale);

	kbsVicEditorShowCursor = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Show cursor", '\'', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorShowCursor);

	kbsVicEditorDoUndo = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Undo", 'z', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorDoUndo);

	kbsVicEditorDoRedo = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Redo", 'z', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorDoRedo);

	kbsVicEditorOpenFile = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Open file", 'o', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorOpenFile);

	kbsVicEditorExportFile = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Export screen to file", 'e', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorExportFile);

	kbsVicEditorSaveVCE = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Save as VCE", 's', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorSaveVCE);

	kbsVicEditorLeaveEditor = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Leave VIC Editor", MTKEY_ESC, false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorLeaveEditor);

	kbsVicEditorClearScreen = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Clear screen", MTKEY_BACKSPACE, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorClearScreen);

	kbsVicEditorRectangleBrushSizePlus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Rectangle brush size +", ']', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorRectangleBrushSizePlus);

	kbsVicEditorRectangleBrushSizeMinus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Rectangle brush size -", '[', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorRectangleBrushSizeMinus);

	kbsVicEditorCircleBrushSizePlus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Circle brush size +", ']', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorCircleBrushSizePlus);

	kbsVicEditorCircleBrushSizeMinus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Circle brush size -", '[', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorCircleBrushSizeMinus);

	kbsVicEditorToggleAllWindows = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle all windows", 'f', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleAllWindows);

	kbsVicEditorToggleWindowPreview = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle preview", 'd', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowPreview);

	kbsVicEditorToggleWindowPalette = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle palette", 'p', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowPalette);

	kbsVicEditorToggleWindowLayers = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle layers", 'l', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowLayers);

	kbsVicEditorToggleWindowCharset = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle charset", 'c', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowCharset);

	kbsVicEditorToggleWindowSprite = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle sprite", 's', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowSprite);

	kbsVicEditorToggleSpriteFrames = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle sprite frames", 'g', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleSpriteFrames);

	kbsVicEditorToggleTopBar = NULL; //new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle top bar", 'b', false, false, true, false, this);
//	guiMain->AddKeyboardShortcut(kbsVicEditorToggleTopBar);

	kbsVicEditorSelectNextLayer = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Select next layer", '`', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorSelectNextLayer);

#endif
	

	//
	kbsShowWatch = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Show watch", 'w', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsShowWatch);
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
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Hard Reset", kbsHardReset->cstr))
			{
				viewC64->HardReset();
			}
			if (ImGui::MenuItem("Detach Everything", viewC64->viewC64SettingsMenu->kbsDetachEverything->cstr))
			{
				viewC64->viewC64SettingsMenu->DetachEverything(false, false);
				C64DebuggerStoreSettings();
			}
			
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
			
			if (ImGui::MenuItem("Quit", kbsQuitApplication->cstr))
			{
				SYS_Shutdown();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Code"))
		{
			if (ImGui::MenuItem("Step Over", kbsStepOverInstruction->cstr))
			{
				kbsStepOverInstruction->Run();
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
				if (ImGui::MenuItem("Back 10 frames", kbsScrubEmulationBackMultipleFrames->cstr))
				{
					kbsScrubEmulationBackMultipleFrames->Run();
				}
				if (ImGui::MenuItem("Forward 10 frames", kbsScrubEmulationForwardMultipleFrames->cstr))
				{
					kbsScrubEmulationForwardMultipleFrames->Run();
				}
				ImGui::EndMenu();
			}
			
			bool warpSpeedState = isWarpSpeed;
			if (ImGui::MenuItem("Warp speed", kbsIsWarpSpeed->cstr, &warpSpeedState))
			{
				kbsIsWarpSpeed->Run();
			}
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Load Snapshot", viewC64->viewC64Snapshots->kbsLoadSnapshot->cstr))
			{
				viewC64->viewC64Snapshots->kbsLoadSnapshot->Run();
			}
			if (ImGui::MenuItem("Save Snapshot", viewC64->viewC64Snapshots->kbsSaveSnapshot->cstr))
			{
				viewC64->viewC64Snapshots->kbsSaveSnapshot->Run();
			}
			
			// TODO: move me, generalize me
			if (ImGui::BeginMenu("Quick Snapshots"))
			{
				if (ImGui::MenuItem("Store snapshot #1", viewC64->viewC64Snapshots->kbsStoreSnapshot1->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot1->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #1", viewC64->viewC64Snapshots->kbsRestoreSnapshot1->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot1->Run();
				}

				if (ImGui::MenuItem("Store snapshot #2", viewC64->viewC64Snapshots->kbsStoreSnapshot2->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot2->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #2", viewC64->viewC64Snapshots->kbsRestoreSnapshot2->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot2->Run();
				}

				if (ImGui::MenuItem("Store snapshot #3", viewC64->viewC64Snapshots->kbsStoreSnapshot3->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot3->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #3", viewC64->viewC64Snapshots->kbsRestoreSnapshot3->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot3->Run();
				}

				if (ImGui::MenuItem("Store snapshot #4", viewC64->viewC64Snapshots->kbsStoreSnapshot4->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot4->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #4", viewC64->viewC64Snapshots->kbsRestoreSnapshot4->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot4->Run();
				}

				if (ImGui::MenuItem("Store snapshot #5", viewC64->viewC64Snapshots->kbsStoreSnapshot5->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot5->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #5", viewC64->viewC64Snapshots->kbsRestoreSnapshot5->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot5->Run();
				}

				if (ImGui::MenuItem("Store snapshot #6", viewC64->viewC64Snapshots->kbsStoreSnapshot6->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot6->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #6", viewC64->viewC64Snapshots->kbsRestoreSnapshot6->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot6->Run();
				}

				if (ImGui::MenuItem("Store snapshot #7", viewC64->viewC64Snapshots->kbsStoreSnapshot7->cstr))
				{
					viewC64->viewC64Snapshots->kbsStoreSnapshot7->Run();
				}
				if (ImGui::MenuItem("Restore snapshot #7", viewC64->viewC64Snapshots->kbsRestoreSnapshot7->cstr))
				{
					viewC64->viewC64Snapshots->kbsRestoreSnapshot7->Run();
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
			
			ImGui::Separator();
			
			if (ImGui::MenuItem("Reset emulation counters", kbsResetCpuCycleAndFrameCounters->cstr))
			{
				kbsResetCpuCycleAndFrameCounters->Run();
			}

			if (ImGui::MenuItem("Clear memory markers", kbsClearMemoryMarkers->cstr))
			{
				kbsClearMemoryMarkers->Run();
			}

			
			
//			if (ImGui::MenuItem("Save snapshot", viewC64->viewC64Snapshots->cstr))
//			{
//
//			}
								
			
			ImGui::EndMenu();
		}

		// emulators menus
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			
			if (debugInterface->isRunning == false)
				continue;
			
			const char *name = debugInterface->GetPlatformNameString();
			
			if (ImGui::BeginMenu(name))
			{
				for (std::list<CGuiView *>::iterator it = debugInterface->views.begin(); it != debugInterface->views.end(); it++)
				{
					CGuiView *view = *it;
	
					bool isVisible = view->visible;
					if (ImGui::MenuItem(view->name, "", &isVisible))
					{
						view->SetVisible(isVisible);
						guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
						break;
					}
				}
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

			if (ImGui::MenuItem("Delete Workspace", "", false, guiMain->layoutManager->currentLayout != NULL))
			{
				CLayoutData *layout = guiMain->layoutManager->currentLayout;
				guiMain->layoutManager->RemoveLayout(layout);
				guiMain->layoutManager->currentLayout = NULL;
				guiMain->layoutManager->StoreLayouts();
				delete layout;
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
					if (ImGui::MenuItem("Load first PRG from disk",
									viewC64->viewC64SettingsMenu->kbsAutoJmpFromInsertedDiskFirstPrg->cstr,
									&c64SettingsAutoJmpFromInsertedDiskFirstPrg))
					{
						C64DebuggerStoreSettings();
					}
					
					if (ImGui::MenuItem("Always JMP to loaded addr",
										viewC64->viewC64SettingsMenu->kbsAutoJmpAlwaysToLoadedPRGAddress->cstr,
										&c64SettingsAutoJmpAlwaysToLoadedPRGAddress))
					{
						C64DebuggerStoreSettings();
					}
					
					if (ImGui::MenuItem("Load first PRG on disk insert",
										viewC64->viewC64SettingsMenu->kbsAutoJmpFromInsertedDiskFirstPrg->cstr,
										&c64SettingsAutoJmpFromInsertedDiskFirstPrg))
					{
						C64DebuggerStoreSettings();
					}
					
//					if (ImGui::Combo("Reset C64 before PRG load", &c64SettingsAutoJmpDoReset, "No reset\0Soft reset\0Hard reset\0\0"))
//					{
//						C64DebuggerStoreSettings();
//					}
					

					if (ImGui::BeginMenu("Reset C64 before PRG load"))
					{
						bool b = c64SettingsAutoJmpDoReset == MACHINE_RESET_NONE;
						bool bp = c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD;
						if (ImGui::MenuItem("No reset",
											bp ? viewC64->viewC64SettingsMenu->kbsAutoJmpDoReset->cstr : "", &b))
						{
							c64SettingsAutoJmpDoReset = MACHINE_RESET_NONE;
							C64DebuggerStoreSettings();
						}

						bp = b;
						b = c64SettingsAutoJmpDoReset == MACHINE_RESET_SOFT;

						if (ImGui::MenuItem("Soft reset",
											bp ? viewC64->viewC64SettingsMenu->kbsAutoJmpDoReset->cstr : "", &b))
						{
							c64SettingsAutoJmpDoReset = MACHINE_RESET_SOFT;
							C64DebuggerStoreSettings();
						}

						bp = b;
						b = c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD;

						if (ImGui::MenuItem("Hard reset",
											bp ? viewC64->viewC64SettingsMenu->kbsAutoJmpDoReset->cstr : "", &b))
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
					
					if (ImGui::MenuItem("Fast boot kernal patch", NULL, &c64SettingsFastBootKernalPatch))
					{
						debugInterfaceVice->SetPatchKernalFastBoot(c64SettingsFastBootKernalPatch);
						C64DebuggerStoreSettings();
					}

					ImGui::Separator();
					
					if (ImGui::BeginMenu("REU"))
					{
//
//						ReuSize
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
					
					ImGui::Separator();

					if (ImGui::BeginMenu("Code monitor"))
					{
						bool b = !c64SettingsUseNativeEmulatorMonitor;
						if (ImGui::MenuItem("Default", NULL, &b))
						{
							c64SettingsUseNativeEmulatorMonitor = false;
							C64DebuggerStoreSettings();
						}
						b = c64SettingsUseNativeEmulatorMonitor;
						if (ImGui::MenuItem("VICE", NULL, &b))
						{
							c64SettingsUseNativeEmulatorMonitor = true;
							C64DebuggerStoreSettings();
						}
						ImGui::EndMenu();
					}
					
					if (ImGui::MenuItem("Emulate VSP bug", NULL, &c64SettingsEmulateVSPBug))
					{
						viewC64->debugInterfaceC64->SetVSPBugEmulation(c64SettingsEmulateVSPBug);
						C64DebuggerStoreSettings();
					}

					if (ImGui::MenuItem("Skip drawing sprites", NULL, &c64SettingsVicSkipDrawingSprites))
					{
						viewC64->debugInterfaceC64->SetSkipDrawingSprites(c64SettingsVicSkipDrawingSprites);
						C64DebuggerStoreSettings();
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
				if (ImGui::BeginMenu("UI font"))
				{
					if (ImGui::InputFloat("Font size", &(viewC64->defaultFontSize), 0.5, 1, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue))
					{
						viewC64->defaultFontSize = URANGE(4, viewC64->defaultFontSize, 64);
						viewC64->config->SetFloat("uiDefaultFontSize", &(viewC64->defaultFontSize));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::InputInt("Oversampling", &(viewC64->defaultFontOversampling)))
					{
						viewC64->defaultFontOversampling = URANGE(1, viewC64->defaultFontOversampling, 8);
						viewC64->config->SetInt("uiDefaultFontOversampling", &(viewC64->defaultFontOversampling));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					ImGui::Separator();
					
					if (ImGui::MenuItem("Cousine Regular", "", !strcmp(viewC64->defaultFontPath, "CousineRegular")))
					{
						viewC64->defaultFontPath = "CousineRegular";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Sweet16", "", !strcmp(viewC64->defaultFontPath, "Sweet16")))
					{
						viewC64->defaultFontPath = "Sweet16";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Proggy Clean", "", !strcmp(viewC64->defaultFontPath, "ProggyClean")))
					{
						viewC64->defaultFontPath = "ProggyClean";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("ProFontIIx", "", !strcmp(viewC64->defaultFontPath, "ProFontIIx")))
					{
						viewC64->defaultFontPath = "ProFontIIx";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Droid Sans", "", !strcmp(viewC64->defaultFontPath, "DroidSans")))
					{
						viewC64->defaultFontPath = "DroidSans";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Karla Regular", "", !strcmp(viewC64->defaultFontPath, "KarlaRegular")))
					{
						viewC64->defaultFontPath = "KarlaRegular";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
						viewC64->UpdateDefaultUIFontFromSettings();
					}

					if (ImGui::MenuItem("Roboto Medium", "", !strcmp(viewC64->defaultFontPath, "RobotoMedium")))
					{
						viewC64->defaultFontPath = "RobotoMedium";
						viewC64->config->SetString("uiDefaultFont", &(viewC64->defaultFontPath));
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
				
				if (ImGui::InputFloat("Mouse wheel X scale", &guiMain->mouseScrollWheelScaleX, 0.1, 1))
				{
					viewC64->config->SetFloat("mouseScrollWheelScaleX", &guiMain->mouseScrollWheelScaleX);
				}

				if (ImGui::InputFloat("Mouse wheel Y scale", &guiMain->mouseScrollWheelScaleY, 0.1, 1))
				{
					viewC64->config->SetFloat("mouseScrollWheelScaleY", &guiMain->mouseScrollWheelScaleY);
				}

				ImGui::EndMenu();
			}
						
			if (ImGui::BeginMenu("Rewind history"))
			{
				if (ImGui::MenuItem("Record snapshots history", "", &c64SettingsSnapshotsRecordIsActive))
				{
					C64DebuggerSetSetting("SnapshotsManagerIsActive", &(c64SettingsSnapshotsRecordIsActive));
					C64DebuggerStoreSettings();
				}
				
				int step = 1; int step_fast = 10;
				if (ImGui::InputScalar("Snapshots interval (frames)", ImGuiDataType_::ImGuiDataType_U32, &c64SettingsSnapshotsIntervalNumFrames, &step, &step_fast, "%d",
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
				if (ImGui::InputScalar("Max number of snapshots", ImGuiDataType_::ImGuiDataType_U32, &c64SettingsSnapshotsLimit, &step, &step_fast, "%d", ImGuiInputTextFlags_CharsDecimal)) // | ImGuiInputTextFlags_EnterReturnsTrue))
				{
					if (c64SettingsSnapshotsLimit < 2)
					{
						c64SettingsSnapshotsLimit = 2;
					}
					C64DebuggerSetSetting("SnapshotsManagerLimit", &c64SettingsSnapshotsLimit);
					C64DebuggerStoreSettings();
				}
				
				ImGui::EndMenu();
			}
			
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
			
			if (ImGui::BeginMenu("Audio device"))
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
		static bool show_app_about = false;

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Retro Debugger about", "", &show_retro_debugger_about);
			ImGui::Separator();
			ImGui::MenuItem("ImGui Metrics", "", &show_app_metrics);
			ImGui::MenuItem("ImGui Style Editor", "", &show_app_style_editor);
			ImGui::MenuItem("ImGui Demo", "", &show_app_demo);
			ImGui::MenuItem("ImGui About", "", &show_app_about);
			
			// end Help menu
			ImGui::EndMenu();
		}
		
		if (show_retro_debugger_about)
		{
			ImGui::Begin("Retro Debugger v" RETRODEBUGGER_VERSION_STRING " About", &show_retro_debugger_about);
			ImGui::Text("Retro Debugger is a multiplatform debugger APIs host with simple ImGui implementation.");
			ImGui::Text("(C) 2021 Marcin 'slajerek' Skoczylas, see README for libraries copyright.");
			ImGui::Separator();
			ImGui::Text("");
			ImGui::Text("If you like this tool and you feel that you would like to share with me some beers,");
			ImGui::Text("then you can use this link: http://tinyurl.com/C64Debugger-PayPal");
			ImGui::Text("");
			ImGui::Separator();

			for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
			{
				CDebugInterface *debugInterface = *it;
				ImGui::Text("%11s debug interface is %08x (%srunning)",
						debugInterface->GetPlatformNameString(),
						(debugInterface),
						debugInterface->isRunning ? "" : "not ");
			}
			
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
			
			ImGui::Checkbox("Do not update views positions", &doNotUpdateViewsPosition);

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
					
					if (layoutData->keyShortcut)
					{
						char *buf = SYS_GetCharBuf();
						sprintf(buf, "Workspace %s", layoutName);
						layoutData->keyShortcut->SetName(buf);
						SYS_ReleaseCharBuf(buf);
					}
					
					guiMain->layoutManager->AddLayout(layoutData);
					guiMain->layoutManager->currentLayout = layoutData;
					guiMain->layoutManager->StoreLayouts();
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
	
	if (shortcut == kbsOpenFile)
	{
		viewC64->viewC64MainMenu->OpenDialogOpenFile();
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

	if (viewC64->debugInterfaceC64)
	{
		//
		
//			if (viewC64Snapshots->ProcessKeyboardShortcut(shortcut))
//			{
//				return true;
//			}
		
		if (shortcut == kbsBrowseD64)
		{
			// TODO: viewFileD64
			viewC64->viewFileD64->StartBrowsingD64(0);
			return true;
		}
		else if (shortcut == kbsStartFromDisk)
		{
			viewC64->viewFileD64->StartDiskPRGEntry(0, true);
			return true;
		}
		
		else if (shortcut == kbsInsertD64)
		{
			viewC64->viewC64MainMenu->OpenDialogInsertD64();
			return true;
		}

		else if (shortcut == kbsInsertNextD64)
		{
			viewC64->viewC64MainMenu->InsertNextD64();
			return true;
		}

		else if (shortcut == kbsReloadAndRestart)
		{
			CSlrString *filePath = viewC64->recentlyOpenedFiles->GetMostRecentFilePath();
			if (filePath)
			{
				viewC64->viewC64MainMenu->LoadFile(filePath);
			}
			return true;
		}
		
		
		
		// TODO: move me
//		else if (shortcut == kbsVicEditorExportFile)
//		{
//			viewC64->viewVicEditor->exportMode = VICEDITOR_EXPORT_UNKNOWN;
//			viewC64->viewVicEditor->OpenDialogExportFile();
//			return true;
//		}
		
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

	float scrubMultipleNumSeconds = 10;
	if (shortcut == kbsScrubEmulationBackMultipleFrames)
	{
		LOGD(">>>>>>>>>................ REWIND -%fs", scrubMultipleNumSeconds);
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				float emulationFPS = debugInterface->GetEmulationFPS();
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(-emulationFPS*scrubMultipleNumSeconds);
			}
		}
		guiMain->UnlockMutex();
		return true;
	}
	
	if (shortcut == kbsScrubEmulationForwardMultipleFrames)
	{
		LOGD(">>>>>>>>>................ FORWARD +%fs", scrubMultipleNumSeconds);
		guiMain->LockMutex();
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (debugInterface->snapshotsManager->isPerformingSnapshotRestore == false)
			{
				float emulationFPS = debugInterface->GetEmulationFPS();
				debugInterface->snapshotsManager->RestoreSnapshotByNumFramesOffset(+emulationFPS*scrubMultipleNumSeconds);
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
				guiMain->ShowMessage(buf);
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
	else if (shortcut == kbsStepBackMultipleInstructions)
	{
		// TODO: kbsStepBackMultipleInstructions is not finished yet
		// TODO: scale me by number of cycles per frame
		int numCycles = 666;
		
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

	
	// TODO: VIC EDITOR, MOVE ME
	
//	if (shortcut == mainMenuBar->kbsSaveScreenImageAsPNG)
//	{
//		viewVicEditor->SaveScreenshotAsPNG();
//		return true;
//	}
	
	

	else if (shortcut == kbsShowWatch)
	{
		LOGTODO("bksShowWatch!!!!!");
		
		/*
		// TODO: make generic
		if (viewC64->debugInterfaceC64)
		{
			if (viewC64->viewC64MemoryDataDump->visible == true)
			{
				SetWatchVisible(true);
			}
			else
			{
				SetWatchVisible(false);
			}
		}

		if (viewC64->debugInterfaceAtari)
		{
			if (viewC64->viewAtariMemoryDataDump->visible == true)
			{
				SetWatchVisible(true);
			}
			else
			{
				SetWatchVisible(false);
			}
		}
		 */
		
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
		viewC64->viewC64MemoryMap->ClearExecuteMarkers();
		viewC64->viewDrive1541MemoryMap->ClearExecuteMarkers();
		return;
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->viewAtariMemoryMap->ClearExecuteMarkers();
	}

	if (viewC64->debugInterfaceNes)
	{
		viewC64->viewNesMemoryMap->ClearExecuteMarkers();
	}
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

}

void CMainMenuBar::SystemDialogFileOpenCancelled()
{
}

void CMainMenuBar::SystemDialogFileSaveSelected(CSlrString *path)
{
	
	//^^^^^^^^^^ TODO join these above into one event
	
	switch(systemDialogOperation)
	{
		case SystemDialogOperationExportLabels:
			selectedDebugInterface->symbols->SaveLabelsRetroDebuggerFormat(path);
			break;
		case SystemDialogOperationExportWatches:
			selectedDebugInterface->symbols->SaveWatchesRetroDebuggerFormat(path);
			break;
	}
}

void CMainMenuBar::SystemDialogFileSaveCancelled()
{
}

void CMainMenuBar::SystemDialogPickFolderSelected(CSlrString *path)
{
	if (systemDialogOperation == SystemDialogOperationC64RomsFolder)
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
		
		guiMain->ShowMessageBox("Information", "Atari ROMs folder has been selected. Please restart Retro Debugger to confirm Atari ROMs path.");
		
		/*
		 TODO: someday soon maybe
		guiMain->LockMutex();
		
		viewC64->debugInterfaceAtari->RestartEmulation();
		
		guiMain->UnlockMutex();*/
	}
}

void CMainMenuBar::SystemDialogPickFolderCancelled()
{
}


