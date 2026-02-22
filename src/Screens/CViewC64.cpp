//
// C64 Debugger (C) Marcin Skoczylas, slajerek@gmail.com
//
// created on 2016-02-22

// define also in CGuiMain
//#define DO_NOT_USE_AUDIO_QUEUE

#define LOG_KEYBOARD_PRESS_KEY_NAME

// TODO: move me ..
#define MAX_BUFFER 2048

extern "C"{
#include "c64mem.h"
}

#include "FontProFontIIx.h"

#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "SYS_DefaultConfig.h"
#include "VID_Main.h"
#include "VID_Fonts.h"
#include "SND_Main.h"
#include "CGuiMain.h"
#include "CLayoutManager.h"
#include "RES_ResourceManager.h"
#include "CSlrFontProportional.h"
#include "VID_ImageBinding.h"
#include "CGuiViewDebugLog.h"
#include "GAM_GamePads.h"
#include "CByteBuffer.h"
#include "CSlrKeyboardShortcuts.h"
#include "SYS_KeyCodes.h"
#include "C64SettingsStorage.h"
#include "SYS_PIPE.h"
#include "SND_SoundEngine.h"
#include "CAudioChannelVice.h"
#include "CAudioChannelAtari.h"
#include "CAudioChannelNes.h"
#include "RetroDebuggerEmbeddedData.h"
#include "CGuiViewMessages.h"
#include "CViewFileBrowser.h"

#include "CDebugDataAdapter.h"
#include "imgui_freetype.h"

#include "SYS_KeyCodes.h"
#include "CViewDataDump.h"
#include "CViewDataMonitor.h"
#include "CViewDataPlot.h"
#include "CViewDataWatch.h"
#include "CViewDataMap.h"
#include "CViewDisassembly.h"
#include "CViewC64Disassembly.h"
#include "CViewSourceCode.h"

#include "CViewC64Screen.h"
#include "CViewC64ScreenViewfinder.h"
#include "CViewC64StateCIA.h"
#include "CViewC64StateREU.h"
#include "CViewC64MemoryBank.h"
#include "CViewStack.h"
#include "CViewEmulationCounters.h"
#include "CViewC64StateSID.h"
#include "CViewC64StateVIC.h"
#include "CViewC64BreakpointsIrq.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64VicControl.h"
#include "CViewC64ColorRamScreen.h"
#include "CViewDrive1541StateVIA.h"
#include "CViewDrive1541Led.h"
#include "CViewDrive1541BreakpointsIrq.h"

#include "CViewC64SidPianoKeyboard.h"
#include "CViewDrive1541Browser.h"
#include "CViewC64StateCPU.h"
#include "CViewInputEvents.h"
#include "CViewBreakpoints.h"
#include "CViewDebugEventsHistory.h"
#include "CViewTimeline.h"
#include "CViewDrive1541StateCPU.h"

#include "CViewAtariScreen.h"
#include "CViewAtariStateCPU.h"
#include "CViewAtariStateANTIC.h"
#include "CViewAtariStatePIA.h"
#include "CViewAtariStateGTIA.h"
#include "CViewAtariStatePOKEY.h"

#include "CViewNesScreen.h"
#include "CViewNesStateCPU.h"
#include "CViewNesStateAPU.h"
#include "CViewNesPianoKeyboard.h"
#include "CViewNesStatePPU.h"
#include "CViewNesPpuPatterns.h"
#include "CViewNesPpuNametables.h"
#include "CViewNesPpuAttributes.h"
#include "CViewNesPpuOam.h"
#include "CViewNesPpuPalette.h"

#include "CViewAudioMixer.h"
#include "CGuiViewProgressBarWindow.h"
#include "CDebugInterfaceMenuItemView.h"
#include "CDebugInterfaceMenuItemFolder.h"

#include "CViewEmulationState.h"

#include "CMainMenuHelper.h"
#include "CViewSettingsMenu.h"

#include "CViewDrive1541Browser.h"
#include "CViewDrive1541DiskData.h"
#include "CViewC64KeyMap.h"
#include "CViewC64AllGraphicsBitmaps.h"
#include "CViewC64AllGraphicsBitmapsControl.h"
#include "CViewC64AllGraphicsScreens.h"
#include "CViewC64AllGraphicsScreensControl.h"
#include "CViewC64AllGraphicsSprites.h"
#include "CViewC64AllGraphicsSpritesControl.h"
#include "CViewC64AllGraphicsCharsets.h"
#include "CViewC64AllGraphicsCharsetsControl.h"
#include "CViewC64SidTrackerHistory.h"
#include "CViewKeyboardShortcuts.h"
#include "CViewMonitorConsole.h"
#include "CViewSnapshots.h"
#include "CViewColodore.h"

#include "CViewC64VicEditor.h"
#include "CViewC64VicEditorPreview.h"
#include "CViewC64VicEditorLayers.h"
#include "CViewC64IoAccess.h"
#include "CViewC64MemoryAccess.h"
#include "CViewC64Charset.h"
#include "CViewC64Palette.h"
#include "CViewC64Sprite.h"

#include "CViewAbout.h"
#include "CViewJukeboxPlaylist.h"
#include "CMainMenuBar.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"

#include "CJukeboxPlaylist.h"
#include "C64FileDataAdapter.h"
#include "C64KeyboardShortcuts.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "CDebugSymbols.h"
#include "C64Palette.h"
#include "C64KeyMap.h"
#include "C64CommandLine.h"
#include "C64SharedMemory.h"
#include "C64SIDFrequencies.h"
#include "CWaveformData.h"
#include "SND_SoundEngine.h"
#include "CSlrFileFromOS.h"
#include "CColorsTheme.h"
#include "SYS_Threading.h"
#include "CDebugAsmSource.h"
#include "CDebuggerEmulatorPlugin.h"
#include "CSnapshotsManager.h"

#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegmentC64.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugInterfaceMenuItemFolder.h"
#include "CDebugInterfaceMenuItemView.h"

#include "C64D_InitPlugins.h"

#include "CPipeProtocolDebuggerCallback.h"
#include "CDebuggerServer.h"

#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"


#include "CSlrFileZlib.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>

CViewC64 *viewC64 = NULL;

unsigned long c64dStartupTime = 0;

#define TEXT_ADDR	0x0400
#define COLOR_ADDR	0xD800

// remove me:
void TEST_Editor();
void TEST_Editor_Render();

const ImGuiInputTextFlags defaultHexInputFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;

// workaround: external API due to stupid MS Windows winsock2 clashes
CDebuggerServer *REMOTE_CreateDebuggerServerWebSockets(int port);
void REMOTE_DebuggerServerWebSocketsSetPort(CDebuggerServer *debuggerServer, int port);

// TODO: refactor this. after transition to ImGui the CViewC64 is no longer a view, it is just a application holder of other views.
CViewC64::CViewC64(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView("CViewC64", posX, posY, posZ, sizeX, sizeY)
{
	LOGM("CViewC64::CViewC64 starting init");
	
	isInitialized = false;
	
	this->name = "CViewC64";
	viewC64 = this;
	
	mutexShowMessage = new CSlrMutex("CViewC64::ShowMessage");
	
	c64dStartupTime = SYS_GetCurrentTimeInMillis();
	
	this->config = gApplicationDefaultConfig;

	gDefaultFontPath = NULL;
	gDefaultFontSize = 13.0f;
	
	// here comes windows shit:
	this->config->GetFloat("mouseScrollWheelScaleX", &guiMain->mouseScrollWheelScaleX, 1.0f);
	
#if defined(WIN32)
	this->config->GetFloat("mouseScrollWheelScaleY", &guiMain->mouseScrollWheelScaleY, 5.0f);
#else
	this->config->GetFloat("mouseScrollWheelScaleY", &guiMain->mouseScrollWheelScaleY, 1.0f);
#endif
	
	C64InitPalette();
	
	this->debugInterfaceC64 = NULL;
	this->emulationThreadC64 = NULL;

	this->debugInterfaceAtari = NULL;
	this->emulationThreadAtari = NULL;

	this->debugInterfaceNes = NULL;
	this->emulationThreadNes = NULL;

	this->testRunner = NULL;

	this->isDataDirectlyFromRAM = false;
		
	memset(&viciiStateToShow, 0, sizeof(vicii_cycle_state_t));
	memset(&currentViciiState, 0, sizeof(vicii_cycle_state_t));

	C64DebuggerInitSharedMemory();
	SYS_SharedMemoryRegisterCallback(viewC64);

	C64DebuggerParseCommandLine1();

	// restore pre-launch settings (paths to D64, PRG, CRT)
	C64DebuggerRestoreSettings(C64DEBUGGER_BLOCK_PRELAUNCH);
	
	
	LOGM("sound engine startup");
	
	LOGTODO("	gSoundEngine->StartAudioUnit(true, false, 0)");
//#ifndef DO_NOT_USE_AUDIO_QUEUE
//	gSoundEngine->StartAudioUnit(true, false, 0);
//#endif
	
	SID_FrequenciesInit();

	mappedC64Memory = NULL;
	mappedC64MemoryDescriptor = NULL;
	
	isSoundMuted = false;
	
	keyboardShortcuts = new C64KeyboardShortcuts();
	
	// init default key map
	if (c64SettingsSkipConfig == false)
	{
		C64KeyMapLoadFromSettings();
	}
	else
	{
		C64KeyMapCreateDefault();
	}
	
	this->colorsTheme = new CColorsTheme(0);

	// default memory map colors
	C64DebuggerComputeMemoryMapColorTables(0);
	C64DebuggerSetMemoryMapMarkersStyle(0);

	// init the Commodore 64 object
	this->InitViceC64();



	// crude hack for now, we needed c64 only for fonts
#ifndef RUN_COMMODORE64
	delete this->debugInterfaceC64;
	this->debugInterfaceC64 = NULL;
#endif
	
	
#ifdef RUN_ATARI
	// init the Atari 800 object
	this->InitAtari800();
	
	// TODO: create Atari fonts from kernel data
//	this->CreateFonts();
#endif
	
#if defined(RUN_NES)
	
	this->InitNestopia();
	
#endif

	int borderMode = 0;
	config->GetInt("viceViciiBorderMode", &borderMode, 0);

	debugInterfaceVice->SetViciiBorderMode(borderMode);
	
	// create fonts from Commodore 64/Atari800 kerne/al data
	this->CreateFonts();

	userFontsNeedRecreation = false;
	guiRenderFrameCounter = 0;
	isShowingRasterCross = config->GetBool("RasterCrossVisible", false);

	
	///
	///
	
	this->InitViews();
	
	//
	viewAudioMixer = new CViewAudioMixer("Audio Mixer", 200, 100, -1, 200, 250, gMainMixerAudioChannels);
	viewAudioMixer->visible = false;
	guiMain->AddView(viewAudioMixer);

	//
	viewProgressBarWindow = new CGuiViewProgressBarWindow("Progress", 100, 100, -1, 150, 150, NULL);

	//
	//
	// THIS BELOW IS NOT USED ANYMORE, STORED HERE FOR HISTORICAL PURPOSES:
	
	/*
	// TODO: move me to CGuiMain
	// loop of views for TAB & shift+TAB
	if (debugInterfaceC64 != NULL)
	{
		traversalOfViews.push_back(viewC64ScreenWrapper);
	}
	
	if (debugInterfaceC64 != NULL)
	{
		traversalOfViews.push_back(viewC64Disassembly);
		traversalOfViews.push_back(viewC64Disassembly2);
		traversalOfViews.push_back(viewC64MemoryDataDump);
		traversalOfViews.push_back(viewDrive1541MemoryDataDump);
		traversalOfViews.push_back(viewDrive1541Disassembly);
		traversalOfViews.push_back(viewDrive1541Disassembly2);
		traversalOfViews.push_back(viewC64MemoryDataDump2);
		traversalOfViews.push_back(viewDrive1541MemoryDataDump2);
		traversalOfViews.push_back(viewC64MemoryDataDump3);
		traversalOfViews.push_back(viewDrive1541MemoryDataDump3);
		traversalOfViews.push_back(viewC64MemoryMap);
		traversalOfViews.push_back(viewDrive1541MemoryMap);
		traversalOfViews.push_back(viewC64MonitorConsole);
	}
	
	if (debugInterfaceC64 != NULL)
	{
		traversalOfViews.push_back(viewC64VicDisplay);
	}
	
	if (debugInterfaceAtari != NULL)
	{
		traversalOfViews.push_back(viewAtariScreen);
		traversalOfViews.push_back(viewAtariDisassembly);
		traversalOfViews.push_back(viewAtariMemoryDataDump);
		traversalOfViews.push_back(viewAtariMemoryMap);
		traversalOfViews.push_back(viewAtariMonitorConsole);
	}

	if (debugInterfaceNes != NULL)
	{
		traversalOfViews.push_back(viewNesScreen);
		traversalOfViews.push_back(viewNesDisassembly);
		traversalOfViews.push_back(viewNesMemoryDataDump);
		traversalOfViews.push_back(viewNesMemoryMap);
		traversalOfViews.push_back(viewNesPpuNametableMemoryDataDump);
		traversalOfViews.push_back(viewNesPpuNametableMemoryMap);
		traversalOfViews.push_back(viewNesMonitorConsole);
	}
	
	*/
	

	// add screens
//	DO NOT ADD YOUSELF YOU'VE BEEN ADDED ALREADY: guiMain->AddGuiElement(this);

	// other screens. These screens are obsolete:
	mainMenuHelper = new CMainMenuHelper(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewC64MainMenu);
	
	viewC64SettingsMenu = new CViewSettingsMenu(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewC64SettingsMenu);
	
	if (this->debugInterfaceC64 != NULL)
	{
//		viewFileD64 = new CViewDrive1541FileD64(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewFileD64);
		
//		viewC64BreakpointsPC = new CViewBreakpoints(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->breakpointsPC);
//		guiMain->AddGuiElement(viewC64BreakpointsPC);
		
		viewC64Snapshots = new CViewSnapshots(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewC64Snapshots);
		
		viewC64KeyMap = new CViewC64KeyMap(150, 50, -3.0, 600, 400);
		this->viewC64KeyMap->visible = false;
		guiMain->AddView(this->viewC64KeyMap);

//		viewColodore = new CViewColodore(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewColodore);
	}
	else
	{
		viewDrive1541Browser = NULL;
		viewC64BreakpointsPC = NULL;
		viewC64Snapshots = NULL;
		viewC64KeyMap = NULL;
	}
	
	if (this->debugInterfaceAtari != NULL)
	{
		viewAtariSnapshots = new CViewSnapshots(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewAtariSnapshots);
	}

	if (this->debugInterfaceNes != NULL)
	{
		viewNesSnapshots = new CViewSnapshots(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewNesSnapshots);
	}

	viewKeyboardShortcuts = new CViewKeyboardShortcuts(100, 100, -3.0, 400, 300);
//	guiMain->AddGuiElement(viewKeyboardShortcuts);

	viewAbout = new CViewAbout(100, 100, -3.0, 400, 300);
//	guiMain->AddGuiElement(viewAbout);
	
	// open/save file dialogs replacement
	viewSelectFile = new CGuiViewSelectFile(100, 100, -3.0, 400, 300, false, this);
	viewSelectFile->SetFont(fontDefaultCBMShifted, 2.0f);
//	guiMain->AddGuiElement(viewSelectFile);

	viewSaveFile = new CGuiViewSaveFile(100, 100, -3.0, 400, 300, this);
	viewSaveFile->SetFont(fontDefaultCBMShifted, 2.0f);
//	guiMain->AddGuiElement(viewSaveFile);
		
	SYS_AddApplicationPauseResumeListener(this);
	
	
	//
	// Init menu bar
	this->mainMenuBar = new CMainMenuBar();
	
	// Log messages
	this->viewMessages = new CGuiViewMessages("Log messages", 155, 200, -1, 500, 400);
	this->viewMessages->visible = false;
	guiMain->AddView(this->viewMessages);
	
	// File browser
	set<string> supportedExtensions = GetSupportedFileExtensions();
	viewFileBrowser = new CViewFileBrowser("File browser", 200, 100, -1, 500, 400, supportedExtensions, this);
	viewFileBrowser->visible = true;
	guiMain->AddView(viewFileBrowser);
	
	//
	// RESTORE settings that need to be set when emulation is initialized
	//
	C64DebuggerRestoreSettings(C64DEBUGGER_BLOCK_POSTLAUNCH);
	
	LOGD("... settings restored");
	
	// do additional parsing
	C64DebuggerParseCommandLine2();

	// memory map colors
	C64DebuggerComputeMemoryMapColorTables(c64SettingsMemoryValuesStyle);
	C64DebuggerSetMemoryMapMarkersStyle(c64SettingsMemoryMarkersStyle);

	C64DebuggerSetMemoryMapCellsFadeSpeed((float)c64SettingsMemoryMapFadeSpeed / 100.0f);

	bool isInVicEditor = c64SettingsIsInVicEditor;
	
//	LOGD("... after parsing c64SettingsDefaultScreenLayoutId=%d", c64SettingsDefaultScreenLayoutId);
//	if (c64SettingsDefaultScreenLayoutId >= SCREEN_LAYOUT_MAX)
//	{
//		LOGD("... c64SettingsDefaultScreenLayoutId=%d >= SCREEN_LAYOUT_MAX=%d", c64SettingsDefaultScreenLayoutId, SCREEN_LAYOUT_MAX);
//
//		c64SettingsDefaultScreenLayoutId = SCREEN_LAYOUT_C64_DEBUGGER;
//		LOGD("... corrected c64SettingsDefaultScreenLayoutId=%d", c64SettingsDefaultScreenLayoutId);
//	}
	
//	// TODO: temporary hack to run both emulators
//#if defined(RUN_COMMODORE64) && defined(RUN_ATARI)
//	if (debugInterfaceC64 && debugInterfaceAtari)
//	{
//		this->SwitchToScreenLayout(SCREEN_LAYOUT_C64_AND_ATARI);
//	}
//#endif
	
	//
	//
	// Create plugins
	this->CreateEmulatorPlugins();

	
	//////////////////////
	this->viewJukeboxPlaylist = NULL;

	if (c64SettingsPathToJukeboxPlaylist != NULL)
	{
		this->InitJukebox(c64SettingsPathToJukeboxPlaylist);
	}
	
	// finished starting up
	RES_SetStateIdle();
	VID_SetFPS(FRAMES_PER_SECOND);

	//
	// Start PIPE integration
	if (c64SettingsUsePipeIntegration)
	{
		PIPE_Init("retrodebugger");
		pipeProtocolCallback = new CPipeProtocolDebuggerCallback();
		PIPE_AddCallback(pipeProtocolCallback);
	}
	
	//
	// Start emulation threads (emulation should be already initialized, just run the processor)
	//
	
	if (c64SettingsRunVice)
	{
		StartEmulationThread(debugInterfaceC64);
	}
	
	if (c64SettingsRunAtari800)
	{
		StartEmulationThread(debugInterfaceAtari);
	}
	
	if (c64SettingsRunNestopia)
	{
		StartEmulationThread(debugInterfaceNes);
	}
	
	//

	if (c64SettingsSkipConfig == false)
	{
		viewKeyboardShortcuts->RestoreKeyboardShortcuts();
	}
	
	viewKeyboardShortcuts->UpdateQuitShortcut();

	//
	C64SetPaletteNum(c64SettingsVicPalette);

#if defined(WIN32)
	// set process priority
	SYS_SetMainProcessPriorityBoostDisabled(c64SettingsIsProcessPriorityBoostDisabled);
	SYS_SetMainProcessPriority(c64SettingsProcessPriority);
#endif
	
	//
	mouseCursorNumFramesToHideCursor = 60;
	mouseCursorVisibilityCounter = 0;
	
	// start
	ShowMainScreen();
	
	// TODO: generalize me, init plugins
	if (debugInterfaceC64)
	{
		debugInterfaceC64->InitPlugins();
	}

	if (debugInterfaceAtari)
	{
		debugInterfaceAtari->InitPlugins();
	}

	if (debugInterfaceNes)
	{
		debugInterfaceNes->InitPlugins();
	}
	
	DefaultSymbolsRestore();
	firstStoreDefaultSymbols = true;
	
	isInitialized = true;
	
	// attach disks, cartridges etc
	C64DebuggerPerformStartupTasks();
	
	// and settings
	guiMain->fntConsole->image->SetLinearScaling(c64SettingsInterpolationOnDefaultFont);
	guiMain->fntConsoleInverted->image->SetLinearScaling(c64SettingsInterpolationOnDefaultFont);

	// register for dropfile callback
	guiMain->AddGlobalDropFileCallback(this);
	
	// restore open recent menu items
	recentlyOpenedFiles = new CRecentlyOpenedFiles(new CSlrString(C64D_RECENTS_FILE_NAME), this);

	// start post-init ui tasks
	SYS_StartThread(this);
	
	// start remote debugger server
	debuggerServer = NULL;
	if (c64SettingsRunDebuggerServerWebSockets)
	{
		DebuggerServerWebSocketsStart();
	}
	
//	TEST_Editor();
}

void CViewC64::ThreadRun(void *passData)
{
	ThreadSetName("CViewC64");
	SYS_Sleep(500);
	
	// set focus to emulator screen
	for (CDebugInterface *debugInterface : debugInterfaces)
	{
		if (debugInterface->isRunning)
		{
			CViewEmulatorScreen *view = debugInterface->GetViewScreen();
			if (view && view->IsVisible())
			{
				CUiThreadTaskSetViewFocus *task = new CUiThreadTaskSetViewFocus(view);
				guiMain->AddUiThreadTask(task);
				return;
			}
		}
	}
}

void CViewC64::ShowMainScreen()
{
	// Note, this is not used anymore because we do not have a main screen as in old C64 Debugger.
	// But considering that we may need to have similar behavior we are keeping this for future reasons.
	return;

//	CheckMouseCursorVisibility();
}

CViewC64::~CViewC64()
{
}

void CViewC64::RegisterEmulatorPlugin(CDebuggerEmulatorPlugin *emuPlugin)
{
	CDebugInterface *debugInterface = emuPlugin->GetDebugInterface();
	debugInterface->RegisterPlugin(emuPlugin);
}

void CViewC64::InitJukebox(CSlrString *jukeboxJsonFilePath)
{
	guiMain->LockMutex();
	
#if defined(MACOS) || defined(LINUX)
	// set current folder to jukebox path
	
	CSlrString *path = jukeboxJsonFilePath->GetFilePathWithoutFileNameComponentFromPath();
	char *cPath = path->GetStdASCII();
	
	LOGD("CViewC64::InitJukebox: chroot to %s", cPath);

	chdir(cPath);
	
	delete [] cPath;
	delete path;
	
#endif
	
	if (this->viewJukeboxPlaylist == NULL)
	{
		this->viewJukeboxPlaylist = new CViewJukeboxPlaylist(-10, -10, -3.0, 0.1, 0.1); //SCREEN_WIDTH, SCREEN_HEIGHT);
		this->AddGuiElement(this->viewJukeboxPlaylist);
	}
	
	this->viewJukeboxPlaylist->DeletePlaylist();
	
	this->viewJukeboxPlaylist->visible = true;

	// start with black screen
	this->viewJukeboxPlaylist->fadeState = JUKEBOX_PLAYLIST_FADE_STATE_FADE_OUT;
	this->viewJukeboxPlaylist->fadeValue = 1.0f;
	this->viewJukeboxPlaylist->fadeStep = 0.0f;
	
	char *str = jukeboxJsonFilePath->GetStdASCII();
	
	this->viewJukeboxPlaylist->InitFromFile(str);
	
	delete [] str;
	
	if ((this->debugInterfaceC64 && this->debugInterfaceC64->isRunning)
		|| (this->debugInterfaceAtari && this->debugInterfaceAtari->isRunning)
		|| (this->debugInterfaceNes && this->debugInterfaceNes->isRunning))
	{
		this->viewJukeboxPlaylist->StartPlaylist();
	}
	else
	{
		/*
		// jukebox will be started by c64PerformStartupTasksThreaded()
		if (this->viewJukeboxPlaylist->playlist->setLayoutViewNumber >= 0
			&& this->viewJukeboxPlaylist->playlist->setLayoutViewNumber < SCREEN_LAYOUT_MAX)
		{
//			viewC64->SwitchToScreenLayout(this->viewJukeboxPlaylist->playlist->setLayoutViewNumber);
		}
		 */
	}
	
	guiMain->UnlockMutex();
}

void CViewC64::InitViceC64()
{
	LOGM("CViewC64::InitViceC64");
	
	if (c64SettingsPathToC64MemoryMapFile)
	{
		// Create debug interface and init Vice
		char *asciiPath = c64SettingsPathToC64MemoryMapFile->GetStdASCII();

		this->MapC64MemoryToFile(asciiPath);

		LOGD(".. mapped C64 memory to file '%s'", asciiPath);
		
		delete [] asciiPath;
		
	}
	else
	{
		this->mappedC64Memory = (uint8 *)malloc(C64_RAM_SIZE);
	}
	
	this->debugInterfaceC64 = new CDebugInterfaceVice(this, this->mappedC64Memory, c64SettingsFastBootKernalPatch);
	
	this->debugInterfaces.push_back(this->debugInterfaceC64);
	
	LOGM("CViewC64::InitViceC64: done");

}

void CViewC64::InitAtari800()
{
	LOGM("CViewC64::InitAtari800");
	
	if (debugInterfaceAtari != NULL)
	{
		delete debugInterfaceAtari;
		debugInterfaceAtari = NULL;
	}
	
//	if (c64SettingsPathToC64MemoryMapFile)
//	{
//		// Create debug interface and init Vice
//		char *asciiPath = c64SettingsPathToAtari800MemoryMapFile->GetStdASCII();
//		
//		this->MapC64MemoryToFile(asciiPath);
//		
//		LOGD(".. mapped Atari800 memory to file '%s'", asciiPath);
//		
//		delete [] asciiPath;
//		
//	}
//	else
//	{
//		this->mappedC64Memory = (uint8 *)malloc(C64_RAM_SIZE);
//	}
	
	this->debugInterfaceAtari = new CDebugInterfaceAtari(this); //, this->mappedC64Memory, c64SettingsFastBootKernalPatch);
	
	this->debugInterfaces.push_back(this->debugInterfaceAtari);

	LOGM("CViewC64::InitAtari800: done");
}

void CViewC64::InitNestopia()
{
	LOGM("CViewC64::InitNestopia");

	this->debugInterfaceNes = new CDebugInterfaceNes(this);

	this->debugInterfaces.push_back(this->debugInterfaceNes);

	LOGM("CViewC64::InitNestopia: done");
}

// SP reader callbacks for CViewStack
static u8 ReadC64MainSP(void *context)
{
	return viewC64->viciiStateToShow.sp;
}

static u8 ReadDrive1541SP(void *context)
{
	C64StateCPU state;
	((CDebugInterfaceC64*)context)->GetDrive1541CpuState(&state);
	return state.sp;
}

static u8 ReadAtariSP(void *context)
{
	u16 pc; u8 a, x, y, flags, sp, irq;
	((CDebugInterfaceAtari*)context)->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);
	return sp;
}

static u8 ReadNesSP(void *context)
{
	u16 pc; u8 a, x, y, flags, sp, irq;
	((CDebugInterfaceNes*)context)->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);
	return sp;
}

void CViewC64::InitViews()
{
	// set mouse cursor outside at startup
	mouseCursorX = -SCREEN_WIDTH;
	mouseCursorY = -SCREEN_HEIGHT;
		
	// TODO: generalize and move the views initialization to CDebugInterface
	InitViceViews();
	InitAtari800Views();
	InitNestopiaViews();
	
	// add views to layout to serialize/deserialize them with layout
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		
		for (std::list<CGuiView *>::iterator it = debugInterface->views.begin(); it != debugInterface->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddViewToLayout(view);
		}
	}
}

void CViewC64::InitViceViews()
{
	// create views
	float minPosX = 10.0f;
	float minPosY = 30.0f;
	
	// note: views are created first to have dependencies fulfilled. They are added later to have them sorted.
	
	// TODO: refactor: move to CreateViews and AddViews in emulators ui (to debugInterface or ->ui). Note, it is desired to shorten path, thus why viewC64->viewNesMem vs masterView->nes->viewNesMem...  global viewC64 is left for a purpose here as it is a shortcut when coding new functionalities and letter c is near shift, anyway - we can leave it for now here
	
	// this is regular c64 screen
	viewC64Screen = new CViewC64Screen("C64 Screen", 510, 40, posZ, 540, 402, debugInterfaceC64);

	viewC64ScreenViewfinder = new CViewC64ScreenViewfinder("C64 Viewfinder", 310, 140, posZ, 270, 201, viewC64Screen, debugInterfaceC64);

	viewC64StateCPU = new CViewC64StateCPU("C64 CPU", 510, 5, posZ, 350, 35, debugInterfaceC64);

	
	// views
	viewC64MemoryMap = new CViewDataMap("C64 Memory map", 190, 20, posZ, 320, 280,
										  debugInterfaceC64->dataAdapterC64,
										  256, 256, true);	// 256x256 = 64kB
	debugInterfaceC64->AddViewMemoryMap(viewC64MemoryMap);

	viewDrive1541MemoryMap = new CViewDataMap("1541 Memory map", 120, 80, posZ, 200, 200,
												debugInterfaceC64->dataAdapterDrive1541,
												256, 256, true);	//34
	debugInterfaceC64->AddViewMemoryMap(viewDrive1541MemoryMap);

	viewDrive1541MemoryMapMinimalRam = new CViewDataMap("1541 Memory map (RAM only)", 120, 80, posZ, 200, 200,
												debugInterfaceC64->dataAdapterDrive1541MinimalRam,
												64, 32, true);	//34
	debugInterfaceC64->AddViewMemoryMap(viewDrive1541MemoryMapMinimalRam);

	
	viewC64Disassembly = new CViewC64Disassembly("C64 Disassembly", 0, 20, posZ, 190, 420,
											  debugInterfaceC64->symbols, NULL);
	
	viewC64Disassembly2 = new CViewC64Disassembly("C64 Disassembly 2", 100, 100, posZ, 200, 300,
												 debugInterfaceC64->symbols, NULL);

	viewC64MemoryDataDump = new CViewDataDump("C64 Memory", 190, 300, posZ, 320, 140,
											  debugInterfaceC64->symbols, viewC64MemoryMap, viewC64Disassembly);
	viewC64Disassembly->SetViewDataDump(viewC64MemoryDataDump);

	viewC64MemoryMonitor = new CViewDataMonitor("C64 Memory Monitor", 290, 350, posZ, 400, 200,
												debugInterfaceC64->symbols, debugInterfaceC64->dataAdapterC64DirectRam,
												viewC64MemoryMap, viewC64Disassembly);

	viewC64MemoryPlot = new CViewDataPlot("C64 Memory Plot", 230, 275, posZ, 400, 200,
												debugInterfaceC64->symbols, debugInterfaceC64->dataAdapterC64);

	viewC64MemoryDataWatch = new CViewDataWatch("C64 Data watch", 140, 140, posZ, 300, 300,
												debugInterfaceC64->symbols, viewC64MemoryMap);

	//
	viewC64MemoryDataDump2 = new CViewDataDump("C64 Memory 2", 10, 30, posZ, 300, 300,
											   debugInterfaceC64->symbols, viewC64MemoryMap, viewC64Disassembly);

	viewC64Disassembly2->SetViewDataDump(viewC64MemoryDataDump2);

	viewC64MemoryDataDump3 = new CViewDataDump("C64 Memory 3", 30, 40, posZ, 300, 300,
											   debugInterfaceC64->symbols, viewC64MemoryMap, viewC64Disassembly);

	// set first data dump as main to be controlled by memory map
	viewC64MemoryMap->SetDataDumpView(viewC64MemoryDataDump);
	
	//
	viewC64BreakpointsPC = new CViewBreakpoints("C64 PC Breakpoints", 10, 40, posZ, 400, 300, this->debugInterfaceC64->symbols, BREAKPOINT_TYPE_ADDR);

	viewC64BreakpointsMemory = new CViewBreakpoints("C64 Memory Breakpoints", 30, 50, posZ, 400, 300, this->debugInterfaceC64->symbols, BREAKPOINT_TYPE_DATA);

	viewC64BreakpointsRaster = new CViewBreakpoints("C64 Raster Breakpoints", 40, 40, posZ, 400, 300, this->debugInterfaceC64->symbols, BREAKPOINT_TYPE_RASTER_LINE);
	
	viewC64BreakpointsIrq = new CViewC64BreakpointsIrq("C64 IRQ Breakpoints", 50, 50, posZ, 400, 300, this->debugInterfaceC64->symbolsC64);

	viewC64DebugEventsHistory = new CViewDebugEventsHistory("C64 Debug Events History", 180, 60, posZ, 400, 200, this->debugInterfaceC64);

	viewDrive1541BreakpointsPC = new CViewBreakpoints("1541 PC Breakpoints", 60, 60, posZ, 400, 300, this->debugInterfaceC64->symbolsDrive1541, BREAKPOINT_TYPE_ADDR);
	
	viewDrive1541BreakpointsMemory = new CViewBreakpoints("1541 Memory Breakpoints", 70, 70, posZ, 400, 300, this->debugInterfaceC64->symbolsDrive1541, BREAKPOINT_TYPE_DATA);

	viewDrive1541BreakpointsIrq = new CViewDrive1541BreakpointsIrq("1541 IRQ Breakpoints", 80, 80, posZ, 400, 300, this->debugInterfaceC64->symbolsDrive1541);

	//
	viewC64CartridgeMemoryMap = new CViewDataMap("C64 Cartridge memory map", 40, 70, posZ, 400, 300,
												   debugInterfaceC64->dataAdapterCartridgeC64,
										  512, 1024, true);	// 512*1024 = 512kB
	viewC64CartridgeMemoryMap->updateMapNumberOfFps = 1.0f;
	viewC64CartridgeMemoryMap->updateMapIsAnimateEvents = false;
	viewC64CartridgeMemoryMap->showCurrentExecutePC = false;
	
	viewC64CartridgeMemoryDataDump = new CViewDataDump("C64 Cartridge memory", 50, 50, posZ, 400, 300,
											  debugInterfaceC64->symbolsCartridgeC64, viewC64CartridgeMemoryMap, viewC64Disassembly);

	//
	viewC64SourceCode = new CViewSourceCode("C64 Source code", 40, 70, posZ, 500, 350,
											debugInterfaceC64, debugInterfaceC64->symbols, viewC64Disassembly);
	//
	viewC64StateCIA = new CViewC64StateCIA("C64 CIA", 10, 40, posZ, 400, 200, debugInterfaceC64);

	viewC64StateSID = new CViewC64StateSID("C64 SID", 10, 40, posZ, 250, 270, debugInterfaceC64);

	viewC64StateVIC = new CViewC64StateVIC("C64 VIC", 10, 40, posZ, 300, 200, debugInterfaceC64);
	
	viewC64StateREU = new CViewC64StateREU("C64 REU", 10, 40, posZ, 300, 200, debugInterfaceC64);

	viewC64MemoryBank = new CViewC64MemoryBank("C64 Memory Bank", 10, 40, posZ, 300, 200, debugInterfaceC64);

	viewC64Stack = new CViewStack("C64 Stack", 10, 40, posZ, 200, 200,
								  debugInterfaceC64, &debugInterfaceC64->mainCpuStack,
								  debugInterfaceC64->dataAdapterC64DirectRam);
	viewC64Stack->readSPFunc = ReadC64MainSP;
	viewC64Stack->readSPContext = debugInterfaceC64;
	viewC64Stack->viewDisassembly = viewC64Disassembly;

	viewC64EmulationCounters = new CViewEmulationCounters("C64 Counters", 860, -1.25, posZ, 130, 43, debugInterfaceC64);

	viewEmulationState = new CViewEmulationState("C64 Emulation", 10, 40, posZ, 350, 10, debugInterfaceC64);

	viewC64VicDisplay = new CViewC64VicDisplay("C64 VIC Display", 50, 50, posZ, 400, 500, debugInterfaceC64);
	viewC64VicDisplay->isZoomPanEnabled = true;

	viewC64VicControl = new CViewC64VicControl("C64 VIC Control", 10, 50, posZ, 100, 360, viewC64VicDisplay);

	viewC64MonitorConsole = new CViewMonitorConsole("C64 Monitor console", 40, 70, posZ, 500, 300, debugInterfaceC64);

	//
	viewDrive1541StateCPU = new CViewDrive1541StateCPU("1541 CPU", 20, 50, posZ, 300, 35, debugInterfaceC64);
	
	viewDrive1541Disassembly = new CViewC64Disassembly("1541 Disassembly", 110, 110, posZ, 200, 300,
													debugInterfaceC64->symbolsDrive1541, NULL);
	
	viewDrive1541Disassembly2 = new CViewC64Disassembly("1541 Disassembly 2", 120, 120, posZ, 200, 300,
													debugInterfaceC64->symbolsDrive1541, NULL);

	viewDrive1541StateVIA = new CViewDrive1541StateVIA("1541 VIA", 10, 40, posZ, 300, 200, debugInterfaceC64);
	
	viewDrive1541Led = new CViewDrive1541Led("1541 Drive Led", 10, 100, posZ, 50, 50, debugInterfaceC64);
	
	viewDrive1541MemoryDataDump = new CViewDataDump("1541 Memory", 20, 30, posZ, 300, 300,
													debugInterfaceC64->symbolsDrive1541,
													viewDrive1541MemoryMap, viewDrive1541Disassembly);
	viewDrive1541Disassembly->SetViewDataDump(viewDrive1541MemoryDataDump);

	//
	viewDrive1541MemoryDataDump2 = new CViewDataDump("1541 Memory 2", 10, 10, posZ, 300, 300,
													 debugInterfaceC64->symbolsDrive1541, viewDrive1541MemoryMap, viewDrive1541Disassembly);
	viewDrive1541Disassembly2->SetViewDataDump(viewDrive1541MemoryDataDump2);

	//
	viewDrive1541MemoryDataDump3 = new CViewDataDump("1541 Memory 3", 10, 10, posZ, 300, 300,
													 debugInterfaceC64->symbolsDrive1541, viewDrive1541MemoryMap, viewDrive1541Disassembly);

	viewDrive1541MemoryMonitor = new CViewDataMonitor("1541 Memory Monitor", 120, 50, posZ, 400, 200,
												debugInterfaceC64->symbolsDrive1541, debugInterfaceC64->dataAdapterDrive1541DirectRam,
													  viewDrive1541MemoryMap, viewDrive1541Disassembly);

	viewDrive1541MemoryDataWatch = new CViewDataWatch("1541 Data watch", 40, 40, posZ, 300, 300,
													  debugInterfaceC64->symbolsDrive1541,
													  viewDrive1541MemoryMap);

	viewDrive1541Stack = new CViewStack("1541 Stack", 10, 40, posZ, 200, 200,
									   debugInterfaceC64, &debugInterfaceC64->driveStack,
									   debugInterfaceC64->dataAdapterDrive1541DirectRam);
	viewDrive1541Stack->readSPFunc = ReadDrive1541SP;
	viewDrive1541Stack->readSPContext = debugInterfaceC64;
	viewDrive1541Stack->viewDisassembly = viewDrive1541Disassembly;

	// set first drive data dump as main to be controlled by drive memory map
	viewDrive1541MemoryMap->SetDataDumpView(viewDrive1541MemoryDataDump);
	viewDrive1541MemoryMapMinimalRam->SetDataDumpView(viewDrive1541MemoryDataDump);

	//
	viewDrive1541Browser = new CViewDrive1541Browser("1541 Disk directory", 120, 120, posZ, 400, 300);
	viewDrive1541DiskData = new CViewDrive1541DiskData("1541 Disk data", 200, 200, posZ, 350, 350);
	viewDrive1541DiskContentsDataDump = new CViewDataDump("1541 Disk contents", 10, 10, posZ, 300, 300,
														  debugInterfaceC64->symbolsDrive1541DiskContents, NULL, NULL);

	//
	viewC64AllGraphicsBitmapsControl = new CViewC64AllGraphicsBitmapsControl("C64 All Bitmaps Control", 200, 100, posZ, 400, 400, debugInterfaceC64);
	viewC64AllGraphicsBitmaps = new CViewC64AllGraphicsBitmaps("C64 All Bitmaps", 100, 100, posZ, 400, 400, debugInterfaceC64, viewC64AllGraphicsBitmapsControl);
	viewC64AllGraphicsScreensControl = new CViewC64AllGraphicsScreensControl("C64 All Screens Control", 220, 120, posZ, 400, 400, debugInterfaceC64);
	viewC64AllGraphicsScreens = new CViewC64AllGraphicsScreens("C64 All Screens", 120, 120, posZ, 400, 400, debugInterfaceC64, viewC64AllGraphicsScreensControl);
	viewC64AllGraphicsCharsetsControl = new CViewC64AllGraphicsCharsetsControl("C64 All Charsets Control", 240, 140, posZ, 400, 400, debugInterfaceC64);
	viewC64AllGraphicsCharsets = new CViewC64AllGraphicsCharsets("C64 All Charsets", 140, 140, posZ, 400, 400, debugInterfaceC64);
	viewC64AllGraphicsSpritesControl = new CViewC64AllGraphicsSpritesControl("C64 All Sprites Control", 250, 150, posZ, 400, 400, debugInterfaceC64);
	viewC64AllGraphicsSprites = new CViewC64AllGraphicsSprites("C64 All Sprites", 150, 150, posZ, 400, 400, debugInterfaceC64);

	//
	viewC64SidTrackerHistory = new CViewC64SidTrackerHistory("C64 SID Tracker history", 150, 40, posZ, 600, 400, (CDebugInterfaceVice*)debugInterfaceC64);

	viewC64SidPianoKeyboard = new CViewC64SidPianoKeyboard("C64 SID Piano keyboard", 50, 100, posZ, 400, 65, viewC64SidTrackerHistory);

	float timelineHeight = 40;
	viewC64Timeline = new CViewTimeline("C64 Timeline", 0, 440, posZ, 700, timelineHeight, debugInterfaceC64);

	viewVicEditor = new CViewC64VicEditor("C64 VIC Editor", 100, 150, posZ, 480, 300);
	viewVicEditorPreview = new CViewC64VicEditorPreview("C64 VIC Editor Preview", 300, 200, posZ, 480/4, 300/4, viewVicEditor);
	viewVicEditorLayers = new CViewC64VicEditorLayers("C64 VIC Editor Layers", 470, 180, posZ, 80, 57, viewVicEditor);
	viewC64Charset = new CViewC64Charset("C64 Charset", 150, 200, posZ, 200, 50, viewVicEditor);
	viewC64Palette = new CViewC64Palette("C64 Palette", 420, 310, posZ, 150*0.85f, 30*0.85f, viewVicEditor);
	viewC64Sprite = new CViewC64Sprite("C64 Sprite", 320, 180, posZ, 100, 87, viewVicEditor);
	viewVicEditor->SetHelperViews(viewC64VicControl, viewVicEditorLayers, viewC64Charset, viewC64Palette, viewC64Sprite);

	viewC64IoAccess = new CViewC64IoAccess("C64 I/O Access", 200, 150, posZ, 450, 400);
	viewC64IoAccess->viewDisassembly = viewC64Disassembly;

	viewC64MemoryAccess = new CViewC64MemoryAccess("C64 Memory Access", 200, 150, posZ, 450, 400);
	viewC64MemoryAccess->viewDisassembly = viewC64Disassembly;

	viewC64ColorRamScreen = new CViewC64ColorRamScreen("C64 Color RAM", 350, 200, posZ, 320, 200);
	
	// add sorted views
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64Screen);
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64ScreenViewfinder);
	viewC64ScreenViewfinder->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64StateCPU);
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64Disassembly);
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64Disassembly2);
	viewC64Disassembly2->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64SourceCode);
	viewC64SourceCode->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryDataDump);
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryDataDump2);
	viewC64MemoryDataDump2->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryDataDump3);
	viewC64MemoryDataDump3->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryMonitor);
	viewC64MemoryMonitor->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryPlot);
	viewC64MemoryPlot->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryDataWatch);
	viewC64MemoryDataWatch->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryMap);

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryBank);
	viewC64MemoryBank->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64Stack);
	viewC64Stack->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64BreakpointsPC);
	viewC64BreakpointsPC->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64BreakpointsMemory);
	viewC64BreakpointsMemory->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64BreakpointsRaster);
	viewC64BreakpointsRaster->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64BreakpointsIrq);
	viewC64BreakpointsIrq->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64DebugEventsHistory);
	viewC64DebugEventsHistory->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64CartridgeMemoryDataDump);
	viewC64CartridgeMemoryDataDump->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64CartridgeMemoryMap);
	viewC64CartridgeMemoryMap->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64StateCIA);
	viewC64StateCIA->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64IoAccess);
	viewC64IoAccess->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MemoryAccess);
	viewC64MemoryAccess->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64StateREU);
	viewC64StateREU->visible = false;

	CDebugInterfaceMenuItemFolder *folderSID = AddEmulatorMenuItemFolder(debugInterfaceC64, "C64 SID##folder");
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderSID, viewC64StateSID);
	viewC64StateSID->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderSID, viewC64SidTrackerHistory);
	viewC64SidTrackerHistory->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderSID, viewC64SidPianoKeyboard);
	viewC64SidPianoKeyboard->visible = false;
	
	CDebugInterfaceMenuItemFolder *folderVIC = AddEmulatorMenuItemFolder(debugInterfaceC64, "C64 VIC##folder");
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64StateVIC);
	viewC64StateVIC->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64VicDisplay);
	viewC64VicDisplay->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64VicControl);
	viewC64VicControl->visible = false;
	//	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphics);
	//	viewC64AllGraphics->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewVicEditor);
	viewVicEditor->visible = false;
//	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewVicEditorPreview);
//	viewVicEditorPreview->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewVicEditorLayers);
	viewVicEditorLayers->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64Charset);
	viewC64Charset->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64Palette);
	viewC64Palette->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64Sprite);
	viewC64Sprite->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64ColorRamScreen);
	viewC64ColorRamScreen->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsBitmaps);
	viewC64AllGraphicsBitmaps->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsBitmapsControl);
	viewC64AllGraphicsBitmapsControl->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsScreens);
	viewC64AllGraphicsScreens->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsScreensControl);
	viewC64AllGraphicsScreensControl->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsCharsets);
	viewC64AllGraphicsCharsets->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsCharsetsControl);
	viewC64AllGraphicsCharsetsControl->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsSprites);
	viewC64AllGraphicsSprites->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderVIC, viewC64AllGraphicsSpritesControl);
	viewC64AllGraphicsSpritesControl->visible = false;

	CDebugInterfaceMenuItemFolder *folderDrive1541 = AddEmulatorMenuItemFolder(debugInterfaceC64, "1541 Drive##folder");
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541StateCPU);
	viewDrive1541StateCPU->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541Disassembly);
	viewDrive1541Disassembly->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541Disassembly2);
	viewDrive1541Disassembly2->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryDataDump);
	viewDrive1541MemoryDataDump->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryDataDump2);
	viewDrive1541MemoryDataDump2->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryDataDump3);
	viewDrive1541MemoryDataDump3->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryMonitor);
	viewDrive1541MemoryMonitor->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryDataWatch);
	viewDrive1541MemoryDataWatch->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryMap);
	viewDrive1541MemoryMap->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541MemoryMapMinimalRam);
	viewDrive1541MemoryMapMinimalRam->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541Stack);
	viewDrive1541Stack->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541BreakpointsPC);
	viewDrive1541BreakpointsPC->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541BreakpointsMemory);
	viewDrive1541BreakpointsMemory->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541BreakpointsIrq);
	viewDrive1541BreakpointsIrq->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541StateVIA);
	viewDrive1541StateVIA->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541Led);
	viewDrive1541Led->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541Browser);
	viewDrive1541Browser->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541DiskContentsDataDump);
	viewDrive1541DiskContentsDataDump->visible = false;
	AddEmulatorMenuItemViewToFolder(debugInterfaceC64, folderDrive1541, viewDrive1541DiskData);
	viewDrive1541DiskData->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewC64MonitorConsole);
	viewC64MonitorConsole->visible = false;

	AddEmulatorMenuItemView(debugInterfaceC64, viewEmulationState);
	viewEmulationState->visible = false;
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64EmulationCounters);
	viewC64EmulationCounters->visible = true;
	
	AddEmulatorMenuItemView(debugInterfaceC64, viewC64Timeline);

	//
	debugInterfaceC64->viewScreen = viewC64Screen;
	debugInterfaceC64->viewDisassembly = viewC64Disassembly;
	
	// add timeline view to snapshots manager
	debugInterfaceC64->snapshotsManager->viewTimeline = viewC64Timeline;

}

// ATARI800 views
void CViewC64::InitAtari800Views()
{
	///
	viewAtariScreen = new CViewAtariScreen("Atari Screen", 510, 40, posZ, 612, 402, debugInterfaceAtari);

	viewAtariStateCPU = new CViewAtariStateCPU("Atari CPU", 510, 5, posZ, 350, 35, debugInterfaceAtari);

	//
	
	viewAtariMemoryMap = new CViewDataMap("Atari Memory map", 190, 20, posZ, 320, 280,
											debugInterfaceAtari->dataAdapter,
											256, 256, true);	// 256x256 = 64kB
	debugInterfaceAtari->AddViewMemoryMap(viewAtariMemoryMap);

	viewAtariDisassembly = new CViewDisassembly("Atari Disassembly", 0, 20, posZ, 190, 420,
												debugInterfaceAtari->symbols, NULL);

	//
	viewAtariBreakpointsPC = new CViewBreakpoints("Atari PC Breakpoints", 70, 70, posZ, 200, 300, this->debugInterfaceAtari->symbols, BREAKPOINT_TYPE_ADDR);

	viewAtariBreakpointsMemory = new CViewBreakpoints("Atari Memory Breakpoints", 90, 70, posZ, 200, 300, this->debugInterfaceAtari->symbols, BREAKPOINT_TYPE_DATA);

	//
	viewAtariSourceCode = new CViewSourceCode("Atari Assembler source", 40, 70, posZ, 500, 350,
											  debugInterfaceAtari, debugInterfaceAtari->symbols, viewAtariDisassembly);

	viewAtariMemoryDataDump = new CViewDataDump("Atari Memory", 190, 300, posZ, 320, 140,
												debugInterfaceAtari->symbols, viewAtariMemoryMap, viewAtariDisassembly);
	viewAtariMemoryDataDump->selectedCharset = 2;

	viewAtariMemoryMonitor = new CViewDataMonitor("Atari Memory Monitor", 290, 350, posZ, 400, 200,
												  debugInterfaceAtari->symbols, debugInterfaceAtari->dataAdapter,
												  viewAtariMemoryMap, viewAtariDisassembly);

	viewAtariMemoryPlot = new CViewDataPlot("Atari Memory Plot", 230, 275, posZ, 400, 200,
												debugInterfaceAtari->symbols, debugInterfaceAtari->dataAdapter);

	//
	viewAtariDisassembly->SetViewDataDump(viewAtariMemoryDataDump);

	//
	viewAtariMemoryDataWatch = new CViewDataWatch("Atari Watches", 140, 140, posZ, 300, 300,
												  debugInterfaceAtari->symbols, viewAtariMemoryMap);

	//

	viewAtariStateANTIC = new CViewAtariStateANTIC("Atari ANTIC", 10, 40, posZ, 400, 200, debugInterfaceAtari);

	viewAtariStateGTIA = new CViewAtariStateGTIA("Atari GTIA", 10, 40, posZ, 400, 100, debugInterfaceAtari);

	viewAtariStatePIA = new CViewAtariStatePIA("Atari PIA", 10, 40, posZ, 200, 60, debugInterfaceAtari);

	viewAtariStatePOKEY = new CViewAtariStatePOKEY("Atari POKEY", 10, 40, posZ, 530, 100, debugInterfaceAtari);

	viewAtariMonitorConsole = new CViewMonitorConsole("Atari Monitor console", 40, 70, posZ, 500, 300, debugInterfaceAtari);

	viewAtariEmulationCounters = new CViewEmulationCounters("Atari Counters", 860, -1.25, posZ, 140, 43, debugInterfaceAtari);

	float timelineHeight = 40;
	viewAtariTimeline = new CViewTimeline("Atari Timeline", 0, 440, posZ, 700, timelineHeight, debugInterfaceAtari);

	viewAtariStack = new CViewStack("Atari Stack", 10, 40, posZ, 200, 200,
									debugInterfaceAtari, &debugInterfaceAtari->mainCpuStack,
									debugInterfaceAtari->dataAdapter);
	viewAtariStack->readSPFunc = ReadAtariSP;
	viewAtariStack->readSPContext = debugInterfaceAtari;
	viewAtariStack->viewDisassembly = viewAtariDisassembly;

	// add sorted views
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariScreen);
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariStateCPU);
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariDisassembly);
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariSourceCode);
	viewAtariSourceCode->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariMemoryDataDump);
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariMemoryMonitor);
	viewAtariMemoryMonitor->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariMemoryPlot);
	viewAtariMemoryPlot->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariMemoryDataWatch);
	viewAtariMemoryDataWatch->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariMemoryMap);
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariStack);
	viewAtariStack->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariBreakpointsPC);
	viewAtariBreakpointsPC->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariBreakpointsMemory);
	viewAtariBreakpointsMemory->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariStateANTIC);
	viewAtariStateANTIC->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariStateGTIA);
	viewAtariStateGTIA->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariStatePIA);
	viewAtariStatePIA->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariStatePOKEY);
	viewAtariStatePOKEY->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariMonitorConsole);
	viewAtariMonitorConsole->visible = false;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariEmulationCounters);
	viewAtariEmulationCounters->visible = true;
	AddEmulatorMenuItemView(debugInterfaceAtari, viewAtariTimeline);

	//
	debugInterfaceAtari->viewScreen = viewAtariScreen;
	debugInterfaceAtari->viewDisassembly = viewAtariDisassembly;

	// add timeline view to snapshots manager
	debugInterfaceAtari->snapshotsManager->viewTimeline = viewAtariTimeline;
}

void CViewC64::InitNestopiaViews()
{
	///
	viewNesScreen = new CViewNesScreen("NES Screen", 510, 40, posZ, 408, 402, debugInterfaceNes);

	viewNesStateCPU = new CViewNesStateCPU("NES CPU", 510, 5, posZ, 290, 35, debugInterfaceNes);

	viewNesMemoryMap = new CViewDataMap("NES Memory map", 190, 20, posZ, 320, 280,
										  debugInterfaceNes->dataAdapter,
										  256, 256, true);	// 256x256 = 64kB
	debugInterfaceNes->AddViewMemoryMap(viewNesMemoryMap);

	viewNesDisassembly = new CViewDisassembly("NES Disassembly", 0, 20, posZ, 190, 424,
												debugInterfaceNes->symbols, NULL);
		
	//
	viewNesBreakpointsPC = new CViewBreakpoints("NES PC Breakpoints", 70, 70, posZ, 200, 300, this->debugInterfaceNes->symbols, BREAKPOINT_TYPE_ADDR);

	viewNesBreakpointsMemory = new CViewBreakpoints("NES Memory Breakpoints", 90, 70, posZ, 200, 300, this->debugInterfaceNes->symbols, BREAKPOINT_TYPE_DATA);

	//
	viewNesSourceCode = new CViewSourceCode("NES Assembler source", 40, 70, posZ, 500, 350,
											  debugInterfaceNes, debugInterfaceNes->symbols, viewNesDisassembly);


	viewNesMemoryDataDump = new CViewDataDump("NES Memory", 190, 300, posZ, 320, 144,
											  debugInterfaceNes->symbols, viewNesMemoryMap, viewNesDisassembly);
	viewNesDisassembly->SetViewDataDump(viewNesMemoryDataDump);

	viewNesMemoryMonitor = new CViewDataMonitor("NES Memory Monitor", 290, 350, posZ, 400, 200,
												debugInterfaceNes->symbols, debugInterfaceNes->dataAdapter,
												viewNesMemoryMap, viewNesDisassembly);

	viewNesMemoryPlot = new CViewDataPlot("NES Memory Plot", 230, 275, posZ, 400, 200,
												debugInterfaceNes->symbols, debugInterfaceNes->dataAdapter);

	viewNesMemoryDataWatch = new CViewDataWatch("NES Watches", 140, 140, posZ, 300, 300,
												  debugInterfaceNes->symbols, viewNesMemoryMap);

	viewNesPpuNametables = new CViewNesPpuNametables("NES Nametables", 10, 40, posZ, 400, 200, debugInterfaceNes);

	viewNesPpuNametableMemoryMap = new CViewDataMap("NES Nametables Memory map", 140, 140, posZ, 300, 300, 
													  debugInterfaceNes->dataAdapterPpuNmt,
													  64, 64, false);

	viewNesPpuNametableMemoryDataDump = new CViewDataDump("NES Nametables Memory", 140, 250, posZ, 300, 150,
													debugInterfaceNes->symbolsPpuNmt, viewNesPpuNametableMemoryMap, viewNesDisassembly);
//	viewNesDisassembly->SetViewDataDump(viewNesMemoryDataDumpPpuNmt);

	
	viewNesStateAPU = new CViewNesStateAPU("NES APU State", 10, 40, posZ, 820, 200, debugInterfaceNes);
	viewNesPianoKeyboard = new CViewNesPianoKeyboard("NES APU Piano Keyboard", 10, 122, posZ, 393, 50, NULL);

	viewNesStatePPU = new CViewNesStatePPU("NES PPU State", 10, 40, posZ, 200, 80, debugInterfaceNes);

	viewNesPpuPatterns = new CViewNesPpuPatterns("NES PPU Patterns", 10, 40, posZ, 400, 200, debugInterfaceNes);

	viewNesPpuAttributes = new CViewNesPpuAttributes("NES PPU Attributes", 10, 40, posZ, 400, 200, debugInterfaceNes);

	viewNesPpuOam = new CViewNesPpuOam("NES PPU Oam", 10, 40, posZ, 400, 200, debugInterfaceNes);

	viewNesPpuPalette = new CViewNesPpuPalette("NES PPU Palette", 700, 443, posZ, 220, 60, debugInterfaceNes);

	viewNesInputEvents = new CViewInputEvents("NES Joysticks", 10, 40, posZ, 150, 310, debugInterfaceNes);

	viewNesMonitorConsole = new CViewMonitorConsole("NES Monitor console", 40, 70, posZ, 500, 300, debugInterfaceNes);

	viewNesEmulationCounters = new CViewEmulationCounters("NES Counters", 800, -1.25, posZ, 120, 43, debugInterfaceNes);

	float timelineHeight = 60;
	viewNesTimeline = new CViewTimeline("NES Timeline", 0, 443, posZ, 700, timelineHeight, debugInterfaceNes);

	viewNesStack = new CViewStack("NES Stack", 10, 40, posZ, 200, 200,
								  debugInterfaceNes, &debugInterfaceNes->mainCpuStack,
								  debugInterfaceNes->dataAdapter);
	viewNesStack->readSPFunc = ReadNesSP;
	viewNesStack->readSPContext = debugInterfaceNes;
	viewNesStack->viewDisassembly = viewNesDisassembly;

	// add sorted views
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesScreen);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesStateCPU);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesDisassembly);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesSourceCode);
	viewNesSourceCode->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesMemoryDataDump);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesMemoryMonitor);
	viewNesMemoryMonitor->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesMemoryPlot);
	viewNesMemoryPlot->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesMemoryDataWatch);
	viewNesMemoryDataWatch->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesMemoryMap);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesStack);
	viewNesStack->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesBreakpointsPC);
	viewNesBreakpointsPC->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesBreakpointsMemory);
	viewNesBreakpointsMemory->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesStateAPU);
	viewNesStateAPU->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPianoKeyboard);
	viewNesPianoKeyboard->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesStatePPU);
	viewNesStatePPU->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuPalette);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuNametables);
	viewNesPpuNametables->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuNametableMemoryDataDump);
	viewNesPpuNametableMemoryDataDump->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuNametableMemoryMap);
	viewNesPpuNametableMemoryMap->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuPatterns);
	viewNesPpuPatterns->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuAttributes);
	viewNesPpuAttributes->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesPpuOam);
	viewNesPpuOam->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesInputEvents);
	viewNesInputEvents->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesMonitorConsole);
	viewNesMonitorConsole->visible = false;
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesEmulationCounters);
	AddEmulatorMenuItemView(debugInterfaceNes, viewNesTimeline);
	
	//
	debugInterfaceNes->viewScreen = viewNesScreen;
	debugInterfaceNes->viewDisassembly = viewNesDisassembly;

	// add timeline view to snapshots manager
	debugInterfaceNes->snapshotsManager->viewTimeline = viewNesTimeline;
}


CDebugInterfaceMenuItemView *CViewC64::AddEmulatorMenuItemView(CDebugInterface *debugInterface, CGuiView *view)
{
	debugInterface->AddView(view);
	
	CDebugInterfaceMenuItemView *menuItemView = new CDebugInterfaceMenuItemView(view, view->name);
	debugInterface->AddMenuItem(menuItemView);
	
	return menuItemView;
}

CDebugInterfaceMenuItemFolder *CViewC64::AddEmulatorMenuItemFolder(CDebugInterface *debugInterface, const char *folderName)
{
	CDebugInterfaceMenuItemFolder *menuItemFolder = new CDebugInterfaceMenuItemFolder(folderName);
	debugInterface->AddMenuItem(menuItemFolder);
	
	return menuItemFolder;
}

CDebugInterfaceMenuItemView *CViewC64::AddEmulatorMenuItemViewToFolder(CDebugInterface *debugInterface, CDebugInterfaceMenuItemFolder *folder, CGuiView *view)
{
	debugInterface->AddView(view);
	
	CDebugInterfaceMenuItemView *menuItemView = new CDebugInterfaceMenuItemView(view, view->name);
	folder->AddMenuItem(menuItemView);
	
	return menuItemView;
}

// TODO: generalize me
void CViewC64::StartEmulationThread(CDebugInterface *debugInterface)
{
	if (debugInterface == debugInterfaceC64)
	{
		c64SettingsRunVice = true;
		StartViceC64EmulationThread();
	}
	else if (debugInterface == debugInterfaceAtari)
	{
		c64SettingsRunAtari800 = true;
		StartAtari800EmulationThread();
	}
	else if (debugInterface == debugInterfaceNes)
	{
		c64SettingsRunNestopia = true;
		StartNestopiaEmulationThread();
	}
	
	C64DebuggerStoreSettings();
}

void CViewC64::StartViceC64EmulationThread()
{
	LOGM("CViewC64::StartViceC64EmulationThread");
	if (debugInterfaceC64 != NULL && debugInterfaceC64->IsEmulationRunning() == false)
	{
		debugInterfaceC64->CheckLoadedRoms();
		
		guiMain->LockMutex();
		
		if (emulationThreadC64 == NULL)
		{
			emulationThreadC64 = new CEmulationThreadC64();
			SYS_StartThread(emulationThreadC64, NULL);
		}
		else
		{
			// TODO: move me to debuginterface
			// restart emulation
			debugInterfaceC64->isRunning = true;
//			debugInterfaceC64->ClearHistory();
//			debugInterfaceC64->ResetHard();
			debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
			debugInterfaceC64->RestartAudio();
		}

		// add views
		for (std::list<CGuiView *>::iterator it = debugInterfaceVice->views.begin(); it != debugInterfaceVice->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddViewSkippingLayout(view);
		}
		guiMain->UnlockMutex();
	}
	
	viewC64Screen->SetVisible(true);
//	viewC64ScreenWrapper->SetVisible(true);
}

void CViewC64::StartAtari800EmulationThread()
{
	LOGM("CViewC64::StartAtari800EmulationThread");
	if (debugInterfaceAtari != NULL && debugInterfaceAtari->IsEmulationRunning() == false)
	{
		guiMain->LockMutex();
		if (emulationThreadAtari == NULL)
		{
			emulationThreadAtari = new CEmulationThreadAtari();
			SYS_StartThread(emulationThreadAtari, NULL);
		}
		else
		{
			// TODO: move me to debuginterface
			// restart emulation
			debugInterfaceAtari->isRunning = true;
//			debugInterfaceAtari->ClearHistory();
//			debugInterfaceAtari->ResetHard();
			debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_RUNNING);
			debugInterfaceAtari->RestartAudio();
		}
		
		// add views
		for (std::list<CGuiView *>::iterator it = debugInterfaceAtari->views.begin(); it != debugInterfaceAtari->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddViewSkippingLayout(view);
		}
		guiMain->UnlockMutex();
	}

	viewAtariScreen->SetVisible(true);
}

void CViewC64::StartNestopiaEmulationThread()
{
	LOGM("CViewC64::StartNestopiaEmulationThread");
	if (debugInterfaceNes != NULL && debugInterfaceNes->IsEmulationRunning() == false)
	{
		guiMain->LockMutex();
		
		if (emulationThreadNes == NULL)
		{
			emulationThreadNes = new CEmulationThreadNes();
			SYS_StartThread(emulationThreadNes, NULL);
		}
		else
		{
			// TODO: move me to debuginterface
			// restart emulation
			debugInterfaceNes->isRunning = true;
//			debugInterfaceNes->ClearHistory();
//			debugInterfaceNes->ResetHard();
			debugInterfaceNes->SetDebugMode(DEBUGGER_MODE_RUNNING);
			debugInterfaceNes->audioChannel->Start();
		}
		
		// add views
		for (std::list<CGuiView *>::iterator it = debugInterfaceNes->views.begin(); it != debugInterfaceNes->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddViewSkippingLayout(view);
		}
		guiMain->UnlockMutex();
	}

	viewNesScreen->SetVisible(true);
}

// TODO: generalize me
void CViewC64::StopEmulationThread(CDebugInterface *debugInterface)
{
	if (debugInterface == debugInterfaceC64)
	{
		c64SettingsRunVice = false;
		StopViceC64EmulationThread();
	}
	else if (debugInterface == debugInterfaceAtari)
	{
		c64SettingsRunAtari800 = false;
		StopAtari800EmulationThread();
	}
	else if (debugInterface == debugInterfaceNes)
	{
		c64SettingsRunNestopia = false;
		StopNestopiaEmulationThread();
	}
	
	C64DebuggerStoreSettings();
}

void CViewC64::StopViceC64EmulationThread()
{
	guiMain->LockMutex();
	debugInterfaceC64->LockMutex();
	debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_PAUSED);
//	debugInterfaceC64->ClearHistory();
	debugInterfaceC64->isRunning = false;
	((CDebugInterfaceVice *)debugInterfaceC64)->audioChannel->Stop();

	// remove views
	for (std::list<CGuiView *>::iterator it = debugInterfaceC64->views.begin(); it != debugInterfaceC64->views.end(); it++)
	{
		CGuiView *view = *it;
		guiMain->RemoveViewSkippingLayout(view);
	}
	debugInterfaceC64->UnlockMutex();
	guiMain->UnlockMutex();
}

void CViewC64::StopAtari800EmulationThread()
{
	guiMain->LockMutex();
	debugInterfaceAtari->LockMutex();
	debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_PAUSED);
//	debugInterfaceAtari->ClearHistory();
	debugInterfaceAtari->isRunning = false;
	debugInterfaceAtari->audioChannel->Stop();

	// remove views
	for (std::list<CGuiView *>::iterator it = debugInterfaceAtari->views.begin(); it != debugInterfaceAtari->views.end(); it++)
	{
		CGuiView *view = *it;
		guiMain->RemoveViewSkippingLayout(view);
	}
	debugInterfaceAtari->UnlockMutex();
	guiMain->UnlockMutex();
}

void CViewC64::StopNestopiaEmulationThread()
{
	guiMain->LockMutex();
	debugInterfaceNes->LockMutex();
	debugInterfaceNes->SetDebugMode(DEBUGGER_MODE_PAUSED);
//	debugInterfaceNes->ClearHistory();
	debugInterfaceNes->isRunning = false;
	debugInterfaceNes->audioChannel->Stop();

	// remove views
	for (std::list<CGuiView *>::iterator it = debugInterfaceNes->views.begin(); it != debugInterfaceNes->views.end(); it++)
	{
		CGuiView *view = *it;
		guiMain->RemoveViewSkippingLayout(view);
	}
	debugInterfaceNes->UnlockMutex();
	guiMain->UnlockMutex();
}

void CEmulationThreadC64::ThreadRun(void *data)
{
	ThreadSetName("C64");
	LOGD("CEmulationThreadC64::ThreadRun");
		
	viewC64->viewDrive1541Browser->SetDiskImage(0);
	viewC64->debugInterfaceC64->RunEmulationThread();
	
	LOGD("CEmulationThreadC64::ThreadRun: finished");
}

void CEmulationThreadAtari::ThreadRun(void *data)
{
	ThreadSetName("ATARI");
	viewC64->debugInterfaceAtari->SetMachineType(c64SettingsAtariMachineType);
	viewC64->debugInterfaceAtari->SetRamSizeOption(c64SettingsAtariRamSizeOption);
	viewC64->debugInterfaceAtari->SetVideoSystem(c64SettingsAtariVideoSystem);

	LOGD("CEmulationThreadAtari::ThreadRun");
	
	viewC64->debugInterfaceAtari->RunEmulationThread();
	
	LOGD("CEmulationThreadAtari::ThreadRun: finished");
}

void CEmulationThreadNes::ThreadRun(void *data)
{
	ThreadSetName("NES");
	LOGD("CEmulationThreadNes::ThreadRun");
	
	viewC64->debugInterfaceNes->RunEmulationThread();
	
	LOGD("CEmulationThreadNes::ThreadRun: finished");
}


void CViewC64::DoLogic()
{
//	CGuiView::DoLogic();

}

void CViewC64::Render()
{
	
//	guiMain->fntConsole->BlitText("CViewC64", 0, 0, 0, 31, 1.0);
//	Blit(guiMain->imgConsoleFonts, 50, 50, -1, 200, 200);
//	BlitRectangle(50, 50, -1, 200, 200, 1, 0, 0, 1);

//	// TODO: generalize this, we need a separate entity to run CellsAnimationLogic
//#if defined(RUN_COMMODORE64)
//	viewC64MemoryMap->CellsAnimationLogic();
//	viewDrive1541MemoryMap->CellsAnimationLogic();
//
//	// workaround
//	if (viewDrive1541MemoryDataDump->visible)
//	{
//		// TODO: this is a workaround, we need to store memory cells state in different object than a memory map view!
//		// and run it once per frame if needed!!
//		// workaround: run logic for cells in drive ROM area because it is not done by memory map view because it is not being displayed
//		viewC64->viewDrive1541MemoryMap->DriveROMCellsAnimationLogic();
//	}
//#endif
//
//#if defined(RUN_ATARI)
//
//	viewAtariMemoryMap->CellsAnimationLogic();
//
//#endif
//
//#if defined(RUN_NES)
//
//	viewNesMemoryMap->CellsAnimationLogic();
//	viewNesPpuNametableMemoryMap->CellsAnimationLogic();
//
//#endif
	
	guiRenderFrameCounter++;

	// TODO: generalize image screen refresh
//	for (CDebugInterface *debugInterface : debugInterfaces)
//	{
//		debugInterface->screen->RefreshImage();
//	}
	
//	if (frameCounter % 2 == 0)
	{
		//if (viewC64ScreenWrapper->visible)   always do this anyway
		
		if (debugInterfaceC64 && debugInterfaceC64->isRunning && viewC64Screen)
		{
			viewC64StateVIC->UpdateSpritesImages();
			viewC64Screen->RefreshImage();
		}

		if (debugInterfaceAtari && debugInterfaceAtari->isRunning && viewAtariScreen)
		{
			viewAtariScreen->RefreshImage();
		}

		if (debugInterfaceNes && debugInterfaceNes->isRunning && viewNesScreen)
		{
			viewNesScreen->RefreshImage();
		}
	}
	
	
	//////////

#ifdef RUN_COMMODORE64
	debugInterfaceC64->snapshotsManager->LockMutex();
	
	// copy current state of VIC
	c64d_vicii_copy_state(&(this->currentViciiState));

	viewC64VicDisplay->UpdateViciiState();

	this->UpdateViciiColors();
		
	//////////
	
	if (viewC64VicDisplay->canScrollDisassembly)
	{
		viewC64Disassembly->SetCurrentPC(viciiStateToShow.lastValidPC);
		viewC64Disassembly2->SetCurrentPC(viciiStateToShow.lastValidPC);
	}
	else
	{
		viewC64Disassembly->SetCurrentPC(this->currentViciiState.lastValidPC);
		viewC64Disassembly2->SetCurrentPC(this->currentViciiState.lastValidPC);
	}
	
	/// 1541 CPU
	C64StateCPU diskCpuState;
	debugInterfaceC64->GetDrive1541CpuState(&diskCpuState);
	
	viewDrive1541Disassembly->SetCurrentPC(diskCpuState.lastValidPC);
	viewDrive1541Disassembly2->SetCurrentPC(diskCpuState.lastValidPC);
	
	// update SID waveforms
	int numSids = debugInterfaceC64->GetNumSids();
	if (viewC64StateSID->IsVisible())
	{
		for (int sidNum = 0; sidNum < numSids; sidNum++)
		{
			debugInterfaceC64->SetSIDReceiveChannelsData(sidNum, true);
		}
		
		debugInterfaceC64->UpdateWaveforms();
	}
	else
	{
		for (int sidNum = 0; sidNum < numSids; sidNum++)
		{
			debugInterfaceC64->SetSIDReceiveChannelsData(sidNum, false);
		}
	}
	
	debugInterfaceC64->snapshotsManager->UnlockMutex();
	
#endif
	
	///
#ifdef RUN_ATARI
	viewAtariDisassembly->SetCurrentPC(debugInterfaceAtari->GetCpuPC());
	
	if (viewAtariStatePOKEY->IsVisible())
	{
		debugInterfaceAtari->UpdateWaveforms();
	}
	
	
#endif

#ifdef RUN_NES
	viewNesDisassembly->SetCurrentPC(debugInterfaceNes->GetCpuPC());
	
	if (viewNesStateAPU->IsVisible())
	{
		debugInterfaceNes->UpdateWaveforms();
	}
	
#endif

	//
	
	//
	// now render all visible views
	//
	//
	
//	CGuiView::Render();
	CGuiView::RenderImGui();

	//
	// and stuff what is left to render:
	//
	
#ifdef RUN_COMMODORE64
//	if (viewC64->viewC64ScreenWrapper->visible)
//	{
//		viewC64ScreenWrapper->RenderRaster(rasterToShowX, rasterToShowY);
//	}
	
//	if (viewC64Screen->showZoomedScreen)
//	{
//		viewC64Screen->RenderZoomedScreen(c64RasterPosToShowX, c64RasterPosToShowY);
//	}
#endif

	// check if we need to hide or display cursor
	CheckMouseCursorVisibility();

//	// debug render fps
//	char buf[128];
//	sprintf(buf, "%-6.2f %-6.2f", debugInterface->emulationSpeed, debugInterface->emulationFrameRate);
//	
//	guiMain->fntConsole->BlitText(buf, 0, 0, -1, 15);

	
}

void CViewC64::RenderImGui()
{
	if (userFontsNeedRecreation)
	{
		RecreateUserFonts();
	}

	/// SKIP FONT:
	ImGui::SetCurrentFont(imFontDefault, ImGui::GetStyle().FontSizeBase, 0.0f);

//	ImGui::SetCurrentFont(imFontPro);
//	ImGui::SetCurrentFont(imFontSweet16);
	
	
//	LOGD("guiRenderFrameCounter=%d", guiRenderFrameCounter);
//	if (guiRenderFrameCounter == 70)
//	{
//		viewC64MainMenu->LoadPRG(new CSlrString("/Volumes/Macintosh\ HD/Users/mars/develop/MTEngine/_RUNTIME_/Documents/c64/disks/PartyDog.prg"),
//								 true, false, true, true);
//	}
	
	if (guiMain->IsViewFullScreen() == false)
	{
		mainMenuBar->RenderImGui();
	}
	
	this->Render();
//	viewC64Screen->RefreshScreen();
//	viewC64Screen->Render();
//	viewC64Screen->RenderImGui();
	
//	TEST_Editor_Render();
}

///////////////

// TODO: refactor UpdateViciiColors and move to C64DebugInterfaceVice
void CViewC64::UpdateViciiColors()
{
	int rasterX = viciiStateToShow.raster_cycle*8;
	int rasterY = viciiStateToShow.raster_line;
	
//	LOGD("rasterX=%x rasterY=%x badline=%d", rasterX, rasterY, viciiStateToShow.bad_line);
	
	// update current colors for rendering states
	
	this->c64RasterPosToShowX = rasterX;
	this->c64RasterPosToShowY = rasterY;
	this->c64RasterPosCharToShowX = (viewC64->viciiStateToShow.raster_cycle - 0x11);
	this->c64RasterPosCharToShowY = (viewC64->viciiStateToShow.raster_line - 0x33) / 8;

	//	LOGD("       |   c64RasterPosToShowX=%3d c64RasterPosToShowY=%3d", c64RasterPosToShowX, c64RasterPosToShowY);
	//	LOGD("       |   rasterCharToShowX=%3d rasterCharToShowY=%3d", c64RasterPosCharToShowX, c64RasterPosCharToShowY);

	if (c64RasterPosCharToShowX < 0)
	{
		c64RasterPosCharToShowX = 0;
	}
	else if (c64RasterPosCharToShowX > 39)
	{
		c64RasterPosCharToShowX = 39;
	}
	
	if (c64RasterPosCharToShowY < 0)
	{
		c64RasterPosCharToShowY = 0;
	}
	else if (c64RasterPosCharToShowY > 24)
	{
		c64RasterPosCharToShowY = 24;
	}

	// get current VIC State's pointers and colors
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	
	viewC64->viewC64VicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
												 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr,
												 this->colorsToShow);
	
	if (viewC64StateVIC->forceColorD800 == -1)
	{
		this->colorToShowD800 = color_ram_ptr[ c64RasterPosCharToShowY * 40 + c64RasterPosCharToShowX ] & 0x0F;
	}
	else
	{
		this->colorToShowD800 = viewC64StateVIC->forceColorD800;
	}
	
	// force D020-D02E colors?
	for (int i = 0; i < 0x0F; i++)
	{
		if (viewC64StateVIC->forceColors[i] != -1)
		{
			colorsToShow[i] = viewC64StateVIC->forceColors[i];
			viewC64->viciiStateToShow.regs[0x20 + i] = viewC64StateVIC->forceColors[i];
		}
	}
}


void CViewC64::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewC64::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewC64::ButtonPressed(CGuiButton *button)
{
	/*
	 if (button == btnDone)
	 {
		guiMain->SetView((CGuiView*)guiMain->viewMainEditor);
		GUI_SetPressConsumed(true);
		return true;
	 }
	 */
	return false;
}

bool CViewC64::ProcessKeyboardShortcut(u32 keyCode, u8 actionType, CSlrKeyboardShortcut *shortcut)
{
	if (shortcut != NULL)
	{
		
	}
	
	return false;
}

// TODO: refactor local viewC64MemoryDataDump->renderDataWithColors to global settings variable
void CViewC64::SwitchIsMulticolorDataDump()
{
	viewC64MemoryDataDump->renderDataWithColors = !viewC64MemoryDataDump->renderDataWithColors;
	viewC64AllGraphicsSprites->UpdateRenderDataWithColors();
	
	LOGTODO("viewC64AllGraphics->UpdateRenderDataWithColors()");
}

void CViewC64::SetIsMulticolorDataDump(bool isMultiColor)
{
	viewC64MemoryDataDump->renderDataWithColors = isMultiColor;
	viewC64AllGraphicsSprites->UpdateRenderDataWithColors();

	LOGTODO("viewC64AllGraphics->UpdateRenderDataWithColors()");
}


void CViewC64::SwitchIsShowRasterBeam()
{
	isShowingRasterCross = !isShowingRasterCross;
	bool s = isShowingRasterCross;
	config->SetBool("RasterCrossVisible", &s);
}

void CViewC64::StepOverInstruction()
{
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->StepOverInstruction();
	}
}

void CViewC64::StepOverSubroutine()
{
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->StepOverSubroutine();
	}
}

void CViewC64::StepOneCycle()
{
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->StepOneCycle();
	}
}

void CViewC64::StepBackInstruction()
{
	guiMain->LockMutex();
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		
		if (debugInterface->isRunning)
		{
			if (debugInterface->GetDebugMode() == DEBUGGER_MODE_RUNNING
				&& !debugInterface->snapshotsManager->IsPerformingSnapshotRestore())
			{
				debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
			}
			debugInterface->snapshotsManager->RestoreSnapshotBackstepInstruction();
		}
		
	}
	guiMain->UnlockMutex();
}

void CViewC64::RunContinueEmulation()
{
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->RunContinueEmulation();
	}
}

void CViewC64::ResetHard()
{
	if (c64SettingsRestartAudioOnEmulationReset)
	{
		// TODO: gSoundEngine->RestartAudioUnit();
	}
	
	if (c64SettingsAlwaysUnpauseEmulationAfterReset)
	{
		for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin();
			 it != debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
	}

	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin();
		 it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->ResetHard();
		debugInterface->ClearDebugMarkers();
	}
}

void CViewC64::ResetSoft()
{
	if (c64SettingsRestartAudioOnEmulationReset)
	{
		// TODO: gSoundEngine->RestartAudioUnit();
	}
	
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin();
		 it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->ResetSoft();
		debugInterface->ClearDebugMarkers();
	}

	if (c64SettingsIsInVicEditor)
	{
		viewC64->viewC64VicControl->UnlockAll();
	}
}

CViewDisassembly *CViewC64::GetActiveDisassemblyView()
{
	if (debugInterfaceC64)
	{
		if (viewC64Disassembly->visible)
			return viewC64Disassembly;
		
		if (viewDrive1541Disassembly->visible)
			return viewDrive1541Disassembly;

		if (viewC64Disassembly2->visible)
			return viewC64Disassembly;
		
		if (viewDrive1541Disassembly2->visible)
			return viewDrive1541Disassembly;
	}
	
	if (debugInterfaceAtari)
	{
		if (viewAtariDisassembly->visible)
			return viewAtariDisassembly;
	}
	
	if (debugInterfaceNes)
	{
		if (viewNesDisassembly)
			return viewNesDisassembly;
	}
	
	return NULL;
}

// TODO: proper banking
void CViewC64::SwitchIsDataDirectlyFromRam()
{
	LOGTODO("CViewC64::SwitchIsDataDirectlyFromRam(): make generic");
	
	if (this->isDataDirectlyFromRAM == false)
	{
		SwitchIsDataDirectlyFromRam(true);
	}
	else
	{
		SwitchIsDataDirectlyFromRam(false);
	}
	
}

void CViewC64::SwitchIsDataDirectlyFromRam(bool setIsDirectlyFromRam)
{
	LOGD("CViewC64::SwitchIsDataDirectlyFromRam: setIsDirectlyFromRam=%s", STRBOOL(setIsDirectlyFromRam));
	LOGTODO("CViewC64::SwitchIsDataDirectlyFromRam(): make generic");
	
	this->isDataDirectlyFromRAM = setIsDirectlyFromRam;
	
	if (viewC64->debugInterfaceC64)
	{
		if (setIsDirectlyFromRam == true)
		{
			viewC64MemoryMap->SetDataAdapter(debugInterfaceC64->dataAdapterC64DirectRam);
			viewC64MemoryDataDump->SetDataAdapter(debugInterfaceC64->dataAdapterC64DirectRam);
			viewDrive1541MemoryMap->SetDataAdapter(debugInterfaceC64->dataAdapterDrive1541DirectRam);
			viewDrive1541MemoryDataDump->SetDataAdapter(debugInterfaceC64->dataAdapterDrive1541DirectRam);
			
			//		viewAtariMemoryMap->isDataDirectlyFromRAM = true;
			//		viewAtariMemoryDataDump->SetDataAdapter(debugInterfaceAtari->data)
		}
		else
		{
			viewC64MemoryMap->SetDataAdapter(debugInterfaceC64->dataAdapterC64);
			viewC64MemoryDataDump->SetDataAdapter(debugInterfaceC64->dataAdapterC64);
			viewDrive1541MemoryMap->SetDataAdapter(debugInterfaceC64->dataAdapterDrive1541);
			viewDrive1541MemoryDataDump->SetDataAdapter(debugInterfaceC64->dataAdapterDrive1541);
		}
		
		LOGTODO("viewC64->viewC64AllGraphics->UpdateShowIOButton - update in control");
//		viewC64->viewC64AllGraphics->UpdateShowIOButton();
	}
	else
	{
		LOGTODO("CViewC64::SwitchIsDataDirectlyFromRam is not supported for this emulator");
	}
}


bool CViewC64::CanSelectView(CGuiView *view)
{
	if (view->visible && view != viewC64MemoryMap && view != viewDrive1541MemoryMap)
		return true;
	
	return false;
}

// TODO: move this to CGuiView
void CViewC64::MoveFocusToNextView()
{
	LOGTODO("CViewC64::MoveFocusToNextView");
	/*
	if (focusElement == NULL)
	{
		SetFocusElement(traversalOfViews[0]);
		return;
	}
	
	int selectedViewNum = -1;
	
	for (int i = 0; i < traversalOfViews.size(); i++)
	{
		CGuiView *view = traversalOfViews[i];
		if (view == focusElement)
		{
			selectedViewNum = i;
			break;
		}
	}
	
	CGuiView *newView = NULL;
	for (int z = 0; z < traversalOfViews.size(); z++)
	{
		selectedViewNum++;
		if (selectedViewNum == traversalOfViews.size())
		{
			selectedViewNum = 0;
		}

		newView = traversalOfViews[selectedViewNum];
		if (CanSelectView(newView))
			break;
	}
	
	if (CanSelectView(newView))
	{
		SetFocus(traversalOfViews[selectedViewNum]);
	}
	else
	{
		LOGError("CViewC64::MoveFocusToNextView: no visible views");
	}*/
	
}

void CViewC64::MoveFocusToPrevView()
{
	LOGTODO("CViewC64::MoveFocusToPrevView");
	/*
	if (focusElement == NULL)
	{
		SetFocus(traversalOfViews[0]);
		return;
	}
	
	int selectedViewNum = -1;
	
	for (int i = 0; i < traversalOfViews.size(); i++)
	{
		CGuiView *view = traversalOfViews[i];
		if (view == focusElement)
		{
			selectedViewNum = i;
			break;
		}
	}
	
	if (selectedViewNum == -1)
	{
		LOGError("CViewC64::MoveFocusToPrevView: selected view not found");
		return;
	}
	
	CGuiView *newView = NULL;
	for (int z = 0; z < traversalOfViews.size(); z++)
	{
		selectedViewNum--;
		if (selectedViewNum == -1)
		{
			selectedViewNum = traversalOfViews.size()-1;
		}
		
		newView = traversalOfViews[selectedViewNum];
		if (CanSelectView(newView))
			break;
	}
	
	if (CanSelectView(newView))
	{
		SetFocus(traversalOfViews[selectedViewNum]);
	}
	else
	{
		LOGError("CViewC64::MoveFocusToNextView: no visible views");
	}*/
}

//////
extern "C" {
	void machine_drive_flush(void);
}

bool CViewC64::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64::KeyDownRepeat, keyCode=%4.4x (%d) %c", keyCode, keyCode, keyCode);
	
#if defined(LOG_KEYBOARD_PRESS_KEY_NAME)
	CSlrString *keyCodeStr = SYS_KeyCodeToString(keyCode);
	char *str = keyCodeStr->GetStdASCII();
	LOGI("                   KeyDown=%s %s%s%s%s", str, isShift ? "shift ":"", isAlt ? "alt ":"", isControl ? "ctrl ":"", isSuper ? "super ":"");
	delete [] str;
	delete keyCodeStr;
#endif
	
	// if emulator screen has focus then it takes precedence
	for (CDebugInterface *debugInterface : debugInterfaces)
	{
		CViewEmulatorScreen *view = debugInterface->GetViewScreen();
		if (view && view->HasFocus())
		{
			view->KeyDownRepeat(keyCode, isShift, isAlt, isControl, isSuper);
			return true;
		}
	}

	// then key shortcuts
//	if (mainMenuBar->KeyDownRepeat(keyCode, isShift, isAlt, isControl, isSuper))
//		return true;
	
	return false;
}

bool CViewC64::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64::KeyDown, keyCode=%4.4x (%d) %c", keyCode, keyCode, keyCode);

	// debug
//	if (keyCode == MTKEY_SPACEBAR)
//	{
////		debugInterfaceC64->snapshotsManager->StoreTimelineToFile(new CSlrString("/Users/mars/Downloads2/test.rtdl"));
//		
//		ImGuiIO& io = ImGui::GetIO();
//		io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;         // Disable Multi-Viewport / Platform Windows
//	}
//	if (keyCode == MTKEY_ENTER)
//	{
//		debugInterfaceC64->snapshotsManager->RestoreTimelineFromFile(new CSlrString("/Users/mars/Downloads2/test.rtdl"));
//	}
	
#if defined(LOG_KEYBOARD_PRESS_KEY_NAME)
	CSlrString *keyCodeStr = SYS_KeyCodeToString(keyCode);
	char *str = keyCodeStr->GetStdASCII();
	LOGI("                   KeyDown=%s %s%s%s%s", str, isShift ? "shift ":"", isAlt ? "alt ":"", isControl ? "ctrl ":"", isSuper ? "super ":"");
	delete [] str;
	delete keyCodeStr;
#endif
	
	if (keyCode == MTKEY_F10)
	{
		LOGD("dupa");
	}
	
	// if emulator screen has focus then it takes precedence
	for (CDebugInterface *debugInterface : debugInterfaces)
	{
		CViewEmulatorScreen *view = debugInterface->GetViewScreen();
		if (view && view->HasFocus())
		{
			if (view->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
				return true;
		}
	}
	
	// then menu bar
	if (mainMenuBar->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
		
	// and keyboard shortcuts
	if (guiMain->CheckKeyboardShortcut(keyCode))
		return true;
		
	return false;
	
	
	
	
	
//#if defined(DEBUG_TEST_CODE)
//	if (keyCode == 'a' && isControl)
//	{
//		VID_TestMenu();
//	}
//#endif
	
	
//	if (keyCode == MTKEY_F2)
//	{
//		LOGD("@");
//		((CDebugInterfaceVice *)viewC64->debugInterface)->MakeBasicRunC64();
////		((CDebugInterfaceVice *)viewC64->debugInterface)->SetStackPointerC64(0xFF);
////		((CDebugInterfaceVice *)viewC64->debugInterface)->SetRegisterA1541(0xFF);
//	}
	
//	// debug only
//	if (keyCode == MTKEY_F7 && isShift)
//	{
//		debugInterface->MakeJmpC64(0x2000);
//		return true;
//	}
//	
//	if (keyCode == MTKEY_F8 && isShift)
//	{
//		AddDebugCode();
//		return true;
//	}
//
//	
//	if (keyCode == MTKEY_F8 && isShift)
//	{
////		debugInterface->SetC64ModelType(4);
//		
//		MapC64MemoryToFile ("/Users/mars/memorymap");
//		viewC64->ShowMessage("mapped");
//		return true;
//	}

	
	//
	// these are very nasty UX workarounds just for now only
	//
//	if (this->currentScreenLayoutId == SCREEN_LAYOUT_C64_VIC_DISPLAY)
//	{
//		if (this->focusElement == NULL)
//		{
//			if (viewC64VicDisplay->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
//				return true;
//		}
//	}
//
//	if (this->currentScreenLayoutId == SCREEN_LAYOUT_C64_ALL_SIDS)
//	{
//		if (this->focusElement == NULL)
//		{
//			if (viewC64AllSids->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
//				return true;
//		}
//	}
	


	/*
	// another UX workaround that can be fixed reviewing again the ui events paths
	// but this will be anyway scrapped for the new imgui version
	if (debugInterfaceC64 && currentScreenLayoutId == SCREEN_LAYOUT_C64_ALL_SIDS)
	{
		// make spacebar reset tracker views
		if (focusElement != viewC64->viewC64Disassembly
			&& focusElement != viewC64->viewC64MemoryDataDump
			&& focusElement != viewC64->viewC64StateSID
			&& focusElement != viewC64->viewC64Screen
			&& focusElement != viewC64->viewC64ScreenWrapper)
		{
			if (keyCode == MTKEY_SPACEBAR || keyCode == MTKEY_ARROW_UP || keyCode == MTKEY_ARROW_DOWN
				|| keyCode == MTKEY_PAGE_UP || keyCode == MTKEY_PAGE_DOWN)
			{
				return viewC64->viewC64AllSids->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
			}
		}
	}
	
	if (keyCode >= MTKEY_F1 && keyCode <= MTKEY_F8 && !isControl)
	{
		if (debugInterfaceC64 && viewC64ScreenWrapper->HasFocus())
		{
			return viewC64ScreenWrapper->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
		
		if (debugInterfaceAtari && viewAtariScreen->HasFocus())
		{
			return viewAtariScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
		
		if (debugInterfaceNes && viewNesScreen->HasFocus())
		{
			return viewNesScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
	}
	*/
	
	// TODO: is this below needed?
	
	/*
	// TODO: this is a temporary UX workaround for step over jsr
	CSlrKeyboardShortcut *shortcut = guiMain->keyboardShortcuts->FindShortcut(KBZONE_DISASSEMBLY, keyCode, isShift, isAlt, isControl, isSuper);

	if (shortcut == mainMenuBar->kbsStepOverJsr)
	{
		if (this->debugInterfaceC64)
		{
			if (focusElement != viewDrive1541Disassembly && viewC64Disassembly->visible)
			{
				viewC64Disassembly->StepOverJsr();
				return true;
			}
			if (focusElement != viewC64Disassembly && viewDrive1541Disassembly->visible)
			{
				viewDrive1541Disassembly->StepOverJsr();
				return true;
			}
		}
		
		if (this->debugInterfaceAtari)
		{
			viewAtariDisassembly->StepOverJsr();
			return true;
		}
		
		if (this->debugInterfaceNes)
		{
			viewNesDisassembly->StepOverJsr();
			return true;
		}
	}
	*/
	
	//
	// end of UX workarounds
	//
	
//	if (focusElement != NULL)
//	{
//		if (focusElement->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
//		{
//			keyDownCodes.push_back(keyCode);
//			return true;
//		}
//	}

	//
	// UX workarounds
	//

//	// if in vic display layout key was not consumed key by focused view, pass it to vic display
//	if (this->currentScreenLayoutId == SCREEN_LAYOUT_C64_VIC_DISPLAY)
//	{
//		if (viewC64VicDisplay->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
//			return true;
//	}
	
//	if (debugInterfaceC64 && viewC64ScreenWrapper->HasFocus())
//	{
//		return viewC64ScreenWrapper->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
//
//	if (debugInterfaceAtari && viewAtariScreen->HasFocus())
//	{
//		return viewAtariScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
//	
//	if (debugInterfaceNes && viewNesScreen->HasFocus())
//	{
//		return viewNesScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
	
	return false; //CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64::PostKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// not consumed key send to screen
	for (CDebugInterface *debugInterface : debugInterfaces)
	{
		CViewEmulatorScreen *view = debugInterface->GetViewScreen();
		if (view->IsVisible())
		{
			if (view->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
				return true;
		}
	}
	return false;
}

bool CViewC64::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64::KeyUp, keyCode=%d isShift=%d isAlt=%d isControl=%d", keyCode, isShift, isAlt, isControl);
	
#if defined(LOG_KEYBOARD_PRESS_KEY_NAME)
	CSlrString *keyCodeStr = SYS_KeyCodeToString(keyCode);
	char *str = keyCodeStr->GetStdASCII();
	LOGI("                   KeyDown=%s %s%s%s%s", str, isShift ? "shift ":"", isAlt ? "alt ":"", isControl ? "ctrl ":"", isSuper ? "super ":"");
	delete [] str;
	delete keyCodeStr;
#endif
	
	if (keyCode == MTKEY_SPACEBAR && !isShift && !isAlt && !isControl)
	{
		for (std::vector<CDebugInterface *>::iterator itDebugInterface = debugInterfaces.begin(); itDebugInterface != debugInterfaces.end(); itDebugInterface++)
		{
			CDebugInterface *debugInterface = *itDebugInterface;
			if (!debugInterface->isRunning)
			{
				continue;
			}
			
			for (std::vector<CViewDataMap *>::iterator itViewMap = debugInterface->viewsMemoryMap.begin(); itViewMap != debugInterface->viewsMemoryMap.end(); itViewMap++)
			{
				CViewDataMap *viewMemoryMap = *itViewMap;
				
				if (viewMemoryMap->visible
					&& viewMemoryMap->IsInside(guiMain->mousePosX, guiMain->mousePosY))
				{
	//				guiMain->SetFocus(viewC64MemoryMap);
					if (viewMemoryMap->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
						return true;
				}
			}
		}
	}
	
//	// check if shortcut
//	std::list<u32> zones;
//	zones.push_back(KBZONE_GLOBAL);
//	
//	CSlrKeyboardShortcut *shortcut = guiMain->keyboardShortcuts->FindShortcut(zones, keyCode, isShift, isAlt, isControl, isSuper);
//	if (shortcut != NULL)
//		return true;
//
//	if (focusElement != NULL)
//	{
//		if (focusElement->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
//		{
//			keyDownCodes.remove(keyCode);
//			return true;
//		}
//	}
//
//	for (std::list<u32>::iterator it = keyDownCodes.begin(); it != keyDownCodes.end(); it++)
//	{
//		if (keyCode == *it)
//		{
//			keyDownCodes.remove(keyCode);
//			return true;
//		}
//	}

	// key was not consumed, send to emulator screen that has focus first
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (!debugInterface->isRunning)
			continue;
		
		CViewEmulatorScreen *viewScreen = debugInterface->GetViewScreen();
		if (!viewScreen)
			continue;
		
		if (viewScreen->HasFocus())
		{
			return viewScreen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
		}
	}

	// and if no screen has focus send the event anyway to all screens, but do not consume
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (!debugInterface->isRunning)
			continue;
		
		CViewEmulatorScreen *viewScreen = debugInterface->GetViewScreen();
		if (!viewScreen)
			continue;
		
		// send event anyway
		viewScreen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	}

	// key not cosumed
	return false;
}

//@returns is consumed
bool CViewC64::DoTap(float x, float y)
{
	LOGG("CViewC64::DoTap:  x=%f y=%f", x, y);

	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->MouseDown(x, y);
	}

	// TODO: is this ok?
	return false;
	
	for (std::map<float, CGuiElement *, compareZupwards>::iterator enumGuiElems = guiElementsUpwards.begin();
		 enumGuiElems != guiElementsUpwards.end(); enumGuiElems++)
	{
		CGuiElement *guiElement = (*enumGuiElems).second;

		LOGG("check inside=%s", guiElement->name);
		
		if (guiElement->visible == false)
			continue;
		
		if (guiElement->IsInside(x, y))
		{
			LOGG("... is inside=%s", guiElement->name);

			// let view decide what to do even if it does not have focus
			guiElement->DoTap(x, y);

			if (guiElement->IsFocusableElement() && ((focusElement != guiElement) || (guiElement->HasFocus() == false)))
			{
				SetFocusElement((CGuiView *)guiElement);
			}
		}
	}

	return true;
	
	//return CGuiView::DoTap(x, y);
}

bool CViewC64::DoRightClick(float x, float y)
{
	return false;
}

bool CViewC64::DoFinishRightClick(float x, float y)
{
	return false;
}


// scroll only where cursor is moving
bool CViewC64::DoNotTouchedMove(float x, float y)
{
	LOGG("CViewC64::DoNotTouchedMove, mouseCursor=%f %f", mouseCursorX, mouseCursorY);

	mouseCursorX = x;
	mouseCursorY = y;

	if (c64SettingsEmulatedMouseC64Enabled && c64SettingsEmulatedMouseCursorAutoHide)
	{
		// do not show mouse cursor on mouse move when emulated mouse is autohide or grabbed
		return false;
	}
	
	if (guiMain->IsMouseCursorVisible() == false)
	{
		guiMain->SetMouseCursorVisible(true);
		mouseCursorVisibilityCounter = 0;
	}
	
	return false;
}

bool CViewC64::DoScrollWheel(float deltaX, float deltaY)
{
	LOGG("CViewC64::DoScrollWheel, mouseCursor=%f %f", mouseCursorX, mouseCursorY);

//	// TODO: ugly hack, fix me
//#if defined(RUN_COMMODORE64)
//	if (viewC64Screen->showZoomedScreen)
//	{
//		if (viewC64Screen->IsInsideZoomedScreen(mouseCursorX, mouseCursorY))
//		{
//			viewC64Screen->DoScrollWheel(deltaX, deltaY);
//			return true;
//		}
//	}
//#endif
	
	return false;
}


bool CViewC64::DoFinishTap(float x, float y)
{
	LOGG("CViewC64::DoFinishTap: %f %f", x, y);

	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->MouseUp(x, y);
	}

	return false;
	
//	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64::DoDoubleTap:  x=%f y=%f", x, y);
	return false;

//	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64::DoFinishTap: %f %f", x, y);
	return false;

//	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
//	LOGD("CViewC64::DoMove");
	return false;

	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return false;
//	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64::InitZoom()
{
	return false;

	return CGuiView::InitZoom();
}

bool CViewC64::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return false;
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return false;
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return false;
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return false;
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewC64::FinishTouches()
{
	return;
//	return CGuiView::FinishTouches();
}

bool CViewC64::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;

	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64::ActivateView()
{
	LOGG("CViewC64::ActivateView()");
}

void CViewC64::DeactivateView()
{
	LOGG("CViewC64::DeactivateView()");
}

void CViewC64::KeyUpModifierKeys(bool isShift, bool isAlt, bool isControl)
{
	for (CDebugInterface *debugInterface : debugInterfaces)
	{
		debugInterface->KeyUpModifierKeys(isShift, isAlt, isControl);
	}
}

void CViewC64::ApplicationEnteredBackground()
{
	LOGG("CViewC64::ApplicationEnteredBackground");
	
	// workaround for alt+tab
	KeyUpModifierKeys(true, true, true);
}

void CViewC64::ApplicationEnteredForeground()
{
	LOGG("CViewC64::ApplicationEnteredForeground");

	// workaround for alt+tab
	KeyUpModifierKeys(true, true, true);
}

#if defined(RUN_ATARI)
extern "C" {
	void MEMORY_GetCharsetScreenCodes(u8 *cs);
}
#endif

ImFont *AddSweet16MonoFont();

void CViewC64::CreateFonts()
{
	fontDisassembly = guiMain->fntConsole;
	fontDisassemblyInverted = guiMain->fntConsoleInverted;

	CreateDefaultUIFont();
	Create8BitFonts();
}

void CViewC64::Create8BitFonts()
{
//	u64 t1 = SYS_GetCurrentTimeInMillis();

	// Default fonts (always from embedded chargen ROM, readable even with custom ROM loaded)
	uint8 *defaultCharRom = debugInterfaceC64->GetDefaultCharRom();
	fontDefaultCBM1 = ProcessFonts(defaultCharRom, true);
	fontDefaultCBM2 = ProcessFonts(defaultCharRom + 0x0800, true);
	fontDefaultCBMShifted = ProcessFonts(defaultCharRom + 0x0800, false);

	// User fonts (from potentially custom chargen ROM)
	uint8 *charRom = debugInterfaceC64->GetCharRom();
	fontCBM1 = ProcessFonts(charRom, true);
	fontCBM2 = ProcessFonts(charRom + 0x0800, true);
	fontCBMShifted = ProcessFonts(charRom + 0x0800, false);

	hasCustomChargenRom = (defaultCharRom != NULL && memcmp(charRom, defaultCharRom, 0x1000) != 0);

	fontAtari = NULL;

#if defined(RUN_ATARI)
	u8 charRomAtari[0x0800];
	MEMORY_GetCharsetScreenCodes(charRomAtari);
	fontAtari = ProcessFonts(charRomAtari, true);
#endif

//	u64 t2 = SYS_GetCurrentTimeInMillis();
//	LOGD("time=%u", t2-t1);
}

void CViewC64::RecreateUserFonts()
{
	if (debugInterfaceC64 == NULL)
		return;

	// Delete old user fonts
	if (fontCBM1) { delete fontCBM1; fontCBM1 = NULL; }
	if (fontCBM2) { delete fontCBM2; fontCBM2 = NULL; }
	if (fontCBMShifted) { delete fontCBMShifted; fontCBMShifted = NULL; }

	// Recreate from current chargen ROM data
	uint8 *charRom = debugInterfaceC64->GetCharRom();
	fontCBM1 = ProcessFonts(charRom, true);
	fontCBM2 = ProcessFonts(charRom + 0x0800, true);
	fontCBMShifted = ProcessFonts(charRom + 0x0800, false);

	uint8 *defaultCharRom = debugInterfaceC64->GetDefaultCharRom();
	hasCustomChargenRom = (defaultCharRom != NULL && memcmp(charRom, defaultCharRom, 0x1000) != 0);

	userFontsNeedRecreation = false;
}

void CViewC64::CreateDefaultUIFont()
{
	/// SKIP FONT
//	return;
	
	ImGuiIO& io = ImGui::GetIO();
		
#if defined(MACOS)
	config->GetString("uiDefaultFont", &gDefaultFontPath, "CousineRegular");
	config->GetFloat("uiDefaultFontSize", &gDefaultFontSize, 16.0f);
	config->GetInt("uiDefaultFontOversampling", &gDefaultFontOversampling, 8);
#else
	config->GetString("uiDefaultFont", &gDefaultFontPath, "Sweet16");
	config->GetFloat("uiDefaultFontSize", &gDefaultFontSize, 16.0f);
	config->GetInt("uiDefaultFontOversampling", &gDefaultFontOversampling, 4);
#endif
	
	gDefaultFontSize = URANGE(4, gDefaultFontSize, 64);
	gDefaultFontOversampling = URANGE(1, gDefaultFontOversampling, 8);

	// TODO: refactor and generalize font names to allow easy fonts adding
	if (!strcmp(gDefaultFontPath, "ProFontIIx"))
	{
		imFontDefault = AddProFontIIx(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "Sweet16"))
	{
		imFontDefault = AddSweet16MonoFont(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "CousineRegular"))
	{
		imFontDefault = AddCousineRegularFont(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "DroidSans"))
	{
		imFontDefault = AddDroidSansFont(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "KarlaRegular"))
	{
		imFontDefault = AddKarlaRegularFont(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "Unifont"))
	{
		imFontDefault = AddUnifont(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "PTMono"))
	{
		imFontDefault = AddPTMono(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "PlexSans"))
	{
		imFontDefault = AddPlexSans(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "PlexMono"))
	{
		imFontDefault = AddPlexMono(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "Monoki"))
	{
		imFontDefault = AddMonoki(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "LiberationSans"))
	{
		imFontDefault = AddLiberationSans(gDefaultFontSize, gDefaultFontOversampling);
	}
	else if (!strcmp(gDefaultFontPath, "ExoMedium"))
	{
		imFontDefault = AddExoMedium(gDefaultFontSize, gDefaultFontOversampling);
	}

	else
	{
		ImFontConfig fontConfig = ImFontConfig();
		fontConfig.SizePixels = gDefaultFontSize;
		fontConfig.OversampleH = gDefaultFontOversampling;
		fontConfig.OversampleV = gDefaultFontOversampling;
		imFontDefault = io.Fonts->AddFontDefault();
	}
//	else if (!strcmp(defaultFontPath, "Custom"))
//	{
//
//	}
		
	guiMain->MergeIconsWithLatestFont(gDefaultFontSize);
}

void CViewC64::UpdateDefaultUIFontFromSettings()
{
	CUiThreadTaskSetDefaultUiFont *task = new CUiThreadTaskSetDefaultUiFont();
	guiMain->AddUiThreadTask(task);
}

void CUiThreadTaskSetDefaultUiFont::RunUIThreadTask()
{
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	
	viewC64->CreateDefaultUIFont();
	
	guiMain->CreateUiFontsTexture(gDefaultFontSize);
}

//
void CViewC64::SetViewportsEnable(bool viewportsEnable)
{
	LOGM("CViewC64::SetViewportsEnable: %s", STRBOOL(viewportsEnable));
	CUiThreadTaskSetViewportsEnable *task = new CUiThreadTaskSetViewportsEnable();
	task->viewportsEnable = viewportsEnable;
	guiMain->AddUiThreadTask(task);
}

void CUiThreadTaskSetViewportsEnable::RunUIThreadTask()
{
	VID_SetViewportsEnable(viewportsEnable);
}

// TODO: this is called by emulator code when frame is started (i.e. just after VSync)
//       note that this assumes we have *ONE* emulator working (the C64 Vice is supported by now)
//       this *MUST* be refactored as different emulation engines will have different frame rates/syncs
void CViewC64::EmulationStartFrameCallback(CDebugInterface *debugInterface)
{
	// TODO: jukebox is supported only for C64
	if (viewJukeboxPlaylist != NULL)
	{
		viewJukeboxPlaylist->EmulationStartFrame();
	}
	
	// TODO: we have a plugin->DoFrame() after frame canvas refresh, shall we have a VSync too for plugins?
	// Note: VSync is before screen bitmap refresh
}

///
void CViewC64::MapC64MemoryToFile(char *filePath)
{
	mappedC64Memory = SYS_MapMemoryToFile(C64_RAM_SIZE, filePath, (void**)&mappedC64MemoryDescriptor);
}

void CViewC64::UnMapC64MemoryFromFile()
{
	SYS_UnMapMemoryFromFile(mappedC64Memory, C64_RAM_SIZE, (void**)&mappedC64MemoryDescriptor);
	mappedC64Memory = NULL;
	mappedC64MemoryDescriptor = NULL;
}

void CViewC64::SharedMemorySignalCallback(CByteBuffer *sharedMemoryData)
{
	LOGD("CViewC64::SharedMemorySignalCallback");
	C64DebuggerReceivedConfiguration(sharedMemoryData);
}

void CViewC64::InitRasterColors()
{
	viewC64VicDisplay->InitRasterColorsFromScheme();
	viewC64Screen->InitRasterColorsFromScheme();
	viewVicEditor->viewVicDisplay->InitGridLinesColorFromSettings();
}

void CViewC64::ToggleSoundMute()
{
	this->SetSoundMute(!this->isSoundMuted);
}

void CViewC64::SetSoundMute(bool isMuted)
{
	this->isSoundMuted = isMuted;
	UpdateSIDMute();
}

void CViewC64::UpdateSIDMute()
{
	LOGD("CViewC64::UpdateSIDMute: isSoundMuted=%s", STRBOOL(isSoundMuted));
	
	if (debugInterfaceC64 == NULL)
		return;
	
	// logic to control "only mute volume" or "skip SID emulation"
	if (this->isSoundMuted == false)
	{
		// start sound
		debugInterfaceC64->SetAudioVolume((float)(c64SettingsViceAudioVolume) / 100.0f);
		debugInterfaceC64->SetRunSIDEmulation(c64SettingsRunSIDEmulation);
	}
	else
	{
		// stop sound
		debugInterfaceC64->SetAudioVolume(0.0f);
		if (c64SettingsMuteSIDMode == MUTE_SID_MODE_SKIP_EMULATION)
		{
			debugInterfaceC64->SetRunSIDEmulation(false);
		}
	}
}

void CViewC64::CheckMouseCursorVisibility()
{
//	LOGD("CViewC64::CheckMouseCursorVisibility: %f %f  %f %f", guiMain->mousePosX, guiMain->mousePosY, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	if (guiMain->IsViewFullScreen())
	{
		if (guiMain->IsMouseCursorVisible() == false)
		{
			// cursor is already hidden
			return;
		}
		
		// we are full screen, do we need to grab cursor by emulated mouse?
		if (c64SettingsEmulatedMouseC64Enabled && guiMain->viewFullScreen == viewC64Screen
			&& c64SettingsEmulatedMouseCursorAutoHide)
		{
			// hide cursor
			guiMain->SetMouseCursorVisible(false);
			mouseCursorVisibilityCounter = 0;
		}
		else
		{
			// cursor is visible, wait till mouseCursorNumFramesToHideCursor and hide
			if (mouseCursorNumFramesToHideCursor > 0
				&& mouseCursorVisibilityCounter > mouseCursorNumFramesToHideCursor)
			{
				// hide cursor
				guiMain->SetMouseCursorVisible(false);
				mouseCursorVisibilityCounter = 0;
			}
			else
			{
				mouseCursorVisibilityCounter++;
			}
		}
	}
	else
	{
		// not fullscreen
		if (c64SettingsEmulatedMouseC64Enabled && c64SettingsEmulatedMouseCursorAutoHide)
		{
			// hide mouse cursor when on c64 screen
			if (viewC64Screen->IsInsideView(guiMain->mousePosX, guiMain->mousePosY))
			{
				guiMain->SetMouseCursorVisible(false);
			}
			else
			{
				guiMain->SetMouseCursorVisible(true);
			}
		}
		else
		{
			if (guiMain->IsMouseCursorVisible() == false)
			{
				mouseCursorVisibilityCounter = 0;
				guiMain->SetMouseCursorVisible(true);
			}
		}
	}
}

void CViewC64::ShowMouseCursor()
{
	guiMain->SetMouseCursorVisible(true);
}

void CViewC64::GoFullScreen(SetFullScreenMode fullScreenMode, CGuiView *view)
{
	if (fullScreenMode == SetFullScreenMode::ViewEnterFullScreen)
	{
		if (view != NULL)
		{
			guiMain->SetViewFullScreen(SetFullScreenMode::ViewEnterFullScreen, view);
		}
	}
	else if (fullScreenMode == SetFullScreenMode::MainWindowEnterFullScreen)
	{
		guiMain->SetViewFullScreen(SetFullScreenMode::MainWindowEnterFullScreen, view);
	}
}
	
void CViewC64::ToggleFullScreen(CGuiView *view)
{
	if (guiMain->IsViewFullScreen())
	{
		guiMain->SetViewFullScreen(SetFullScreenMode::ViewLeaveFullScreen, NULL);
		return;
	}
	
	if (guiMain->IsApplicationWindowFullScreen())
	{
		guiMain->SetViewFullScreen(SetFullScreenMode::MainWindowLeaveFullScreen, NULL);
//		guiMain->SetApplicationWindowFullScreen(false);
		return;
	}
	
	if (view != NULL)
	{
		guiMain->SetViewFullScreen(SetFullScreenMode::ViewEnterFullScreen, view);
		return;
	}

	guiMain->SetViewFullScreen(SetFullScreenMode::MainWindowEnterFullScreen, NULL);
//	guiMain->SetApplicationWindowFullScreen(true);
}


CDebugInterface *CViewC64::GetDebugInterface(u8 emulatorType)
{
	switch(emulatorType)
	{
		case EMULATOR_TYPE_C64_VICE:
			return (CDebugInterface*)viewC64->debugInterfaceC64;
			
		case EMULATOR_TYPE_ATARI800:
			return (CDebugInterface*)viewC64->debugInterfaceAtari;
			
		case EMULATOR_TYPE_NESTOPIA:
			return (CDebugInterface*)viewC64->debugInterfaceNes;
			
		default:
			return NULL;
	}
}

int CViewC64::CountRunningDebugInterfaces()
{
	int runningInterfaces = 0;
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (debugInterface->isRunning)
		{
			runningInterfaces++;
		}
	}
	return runningInterfaces;
}

bool CViewC64::IsOnlyOneDebugInterfaceRunning()
{
	return (CountRunningDebugInterfaces() == 1);
}

void CViewC64::CreateEmulatorPlugins()
{
	C64D_InitPlugins();
}

// keep current symbols for all emus in settings for the 'keep symbols' functionality:
// observer of symbols data change:
#define DEFAULT_SYMBOLS_FILE_NAME	"defaultSymbols.hjson"
void CViewC64::DefaultSymbolsStore()
{
	LOGD("CViewC64::DefaultSymbolsStore");
	
	// TODO: BUG: workaround for DefaultSymbolsStore will NOT work for multiple emulators only the first
	// Note: this workaround below checks if this is first store of default watches and breakpoints without labels.
	//       examine this case below: we are storing watches/breakpoints without labels
	//		 watches/breakpoints contain a label (if exists) and address,
	//		 when we restore watches/breakpoints we check if label exists, and point to that label/address,
	//		 if there's no label we use backup address instead
	//		 thus this causes a problem:
	//       watches and breakpoints are restored without labels on startup of the application
	//       but as there are no labels on startup, then watches/breakpoints that point to labels will be converted to their backup addresses
	//       and watchpoints/breakpoints labels are lost
	//       then a while later code will load async within startup thread,
	//       and normally before loading code we would store now breakpoints/watches with addresses only and no labels
	//		 now, code loads its own labels and say- some label has been changed (i.e. code recompiled),
	//       but our default symbols created by the user in debugger do not contain labels anymore, so we would see wrong addresses instead
	//		 to solve this as a workaround we do not store watches/breakpoints on first load of code and will re-load default from previous session
	//       allowing properly matching old watches/breakpoints that point to a label to a new label address
	if (firstStoreDefaultSymbols)
	{
		firstStoreDefaultSymbols = false;
		if (c64SettingsKeepSymbolsLabels == false)
		{
			return;
		}
	}
	
	if (c64SettingsKeepSymbolsLabels == false
		&& c64SettingsKeepSymbolsWatches == false
		&& c64SettingsKeepSymbolsBreakpoints == false)
	{
		return;
	}

	Hjson::Value hjsonRoot;
	SerializeAllEmulatorsSymbols(hjsonRoot, c64SettingsKeepSymbolsLabels, c64SettingsKeepSymbolsWatches, c64SettingsKeepSymbolsBreakpoints);

	std::stringstream ss;
	ss << Hjson::Marshal(hjsonRoot);
	
	std::string s = ss.str();
	const char *cstrHjson = s.c_str();

	CSlrString *fileName = new CSlrString(DEFAULT_SYMBOLS_FILE_NAME);
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutBytes((u8*)cstrHjson, strlen(cstrHjson));
	byteBuffer->storeToSettings(fileName);
	delete byteBuffer;
	delete fileName;
}

void CViewC64::SerializeAllEmulatorsSymbols(Hjson::Value hjsonRoot, bool storeLabels, bool storeWatches, bool storeBreakpoints)
{
	LOGD("CViewC64::SerializeAllEmulatorsSymbols");

	hjsonRoot["Version"] = "1";

	Hjson::Value hjsonEmulators;
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
				
		debugInterface->LockMutex();

		LOGD("...emulator %s", debugInterface->GetPlatformNameString());
		
		Hjson::Value hjsonEmulator;
		
		if (storeLabels)
		{
			Hjson::Value hjsonLabels;
			debugInterface->symbols->SerializeLabelsToHjson(hjsonLabels);
			hjsonEmulator["Labels"] = hjsonLabels;
		}
		
		if (storeWatches)
		{
			Hjson::Value hjsonWatches;
			debugInterface->symbols->SerializeWatchesToHjson(hjsonWatches);
			hjsonEmulator["Watches"] = hjsonWatches;
		}

		if (storeBreakpoints)
		{
			Hjson::Value hjsonBreakpoints;
			debugInterface->symbols->SerializeBreakpointsToHjson(hjsonBreakpoints);
			hjsonEmulator["Breakpoints"] = hjsonBreakpoints;
		}

		debugInterface->UnlockMutex();

		const char *name = debugInterface->GetPlatformNameString();
		hjsonEmulators[name] = hjsonEmulator;
	}

	hjsonRoot["Emulators"] = hjsonEmulators;
}

// note, this method should not start with already locked debug interface mutex as this may cause endless lock when gui waits for debug interface
void CViewC64::DefaultSymbolsRestore()
{
	LOGD("CViewC64::DefaultSymbolsRestore");
	if (c64SettingsKeepSymbolsLabels == false
		&& c64SettingsKeepSymbolsWatches == false
		&& c64SettingsKeepSymbolsBreakpoints == false)
	{
		return;
	}

	guiMain->LockMutex();

	CSlrString *fileName = new CSlrString(DEFAULT_SYMBOLS_FILE_NAME);
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->loadFromSettings(fileName);
	
	if (!byteBuffer->IsEmpty())
	{
		char *jsonText = new char[byteBuffer->length+2];
		memcpy(jsonText, byteBuffer->data, byteBuffer->length);
		jsonText[byteBuffer->length] = 0;
		
		Hjson::Value hjsonRoot;
		std::stringstream ss;
		ss.str(jsonText);
		
		try
		{
			ss >> hjsonRoot;
			DeserializeAllEmulatorsSymbols(hjsonRoot, c64SettingsKeepSymbolsLabels, c64SettingsKeepSymbolsWatches, c64SettingsKeepSymbolsBreakpoints);
		}
		catch (const std::exception& e)
		{
			LOGError("CViewC64::DefaultSymbolsRestore error: %s", e.what());
			
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "Loading default symbols failed. Error:\n%s", e.what());
			guiMain->ShowMessageBox("Error", buf);
			SYS_ReleaseCharBuf(buf);
		}
	}
	
	guiMain->UnlockMutex();

	delete byteBuffer;
	delete fileName;

}

void CViewC64::DeserializeAllEmulatorsSymbols(Hjson::Value hjsonRoot, bool restoreLabels, bool restoreWatches, bool restoreBreakpoints)
{
	LOGD("CViewC64::DeserializeAllEmulatorsSymbols");
	Hjson::Value hjsonEmulators = hjsonRoot["Emulators"];
	for (auto it = hjsonEmulators.begin(); it != hjsonEmulators.end(); ++it)
	{
		std::string emulatorName = it->first;
		const char *cEmulatorName = emulatorName.c_str();

		LOGD("....emulatorName=%s", cEmulatorName);
		
		for (std::vector<CDebugInterface *>::iterator itInterface = debugInterfaces.begin();
			 itInterface != debugInterfaces.end(); itInterface++)
		{
			CDebugInterface *debugInterface = *itInterface;
			if (!strcmp(cEmulatorName, debugInterface->GetPlatformNameString()))
			{
				Hjson::Value hjsonEmulator = it->second;
				
				debugInterface->LockMutex();
				
				if (restoreLabels)
				{
					Hjson::Value hjsonLabels = hjsonEmulator["Labels"];
					debugInterface->symbols->DeserializeLabelsFromHjson(hjsonLabels);
				}
				
				if (restoreWatches)
				{
					Hjson::Value hjsonWatches = hjsonEmulator["Watches"];
					debugInterface->symbols->DeserializeWatchesFromHjson(hjsonWatches);
				}

				if (restoreBreakpoints)
				{
					Hjson::Value hjsonBreakpoints = hjsonEmulator["Breakpoints"];
					debugInterface->symbols->DeserializeBreakpointsFromHjson(hjsonBreakpoints);
					debugInterface->UpdateRenderBreakpoints();
				}
				
				debugInterface->UnlockMutex();
			}
		}
	}
}


// REMOVE THESE OLD DIALOGS
void CViewC64::ShowDialogOpenFile(CSystemFileDialogCallback *callback, std::list<CSlrString *> *extensions,
						CSlrString *defaultFolder,
						CSlrString *windowTitle)
{
//	return;
	
//	LOGTODO("ShowDialogOpenFile CHECK MEMORY LEAKS");
	if (c64SettingsUseSystemFileDialogs)
	{
		SYS_DialogOpenFile(callback, extensions, defaultFolder, windowTitle);
	}
	else
	{
		fileDialogPreviousView = guiMain->currentView;
		systemFileDialogCallback = callback;
		
		CSlrString *directoryPath;
		if (defaultFolder != NULL)
		{
			directoryPath = new CSlrString(defaultFolder);
		}
		else
		{
			directoryPath = new CSlrString("/");
		}
		
		viewSelectFile->Init(directoryPath, extensions);
	}
}

// TODO: repeated copypasted code below
#define BUFSIZE 1024*8
static char notificationBuffer[BUFSIZE];
void CViewC64::ShowMessage(const char *fmt, ...)
{
	mutexShowMessage->Lock();
	//memset(buffer, 0x00, BUFSIZE);

	va_list args;

	va_start(args, fmt);
	vsnprintf(notificationBuffer, BUFSIZE, fmt, args);
	va_end(args);
	notificationBuffer[BUFSIZE-1] = 0x00;
	
	viewMessages->AddLog(notificationBuffer);
	viewMessages->AddLog("\n");
	
	guiMain->ShowNotification(ImGuiToastType_Info, 4444, NULL, notificationBuffer);
	
	LOGM("CViewC64::ShowMessage: %s", notificationBuffer);
	
	mutexShowMessage->Unlock();
}

void CViewC64::ShowMessage(CSlrString *showMessage)
{
	char *cStr = showMessage->GetStdASCII();
	this->ShowMessage(cStr);
	delete [] cStr;
}

void CViewC64::ShowMessageWarning(CSlrString *showMessage)
{
	char *cStr = showMessage->GetStdASCII();
	this->ShowMessageWarning(cStr);
	delete [] cStr;
}

void CViewC64::ShowMessageError(CSlrString *showMessage)
{
	char *cStr = showMessage->GetStdASCII();
	this->ShowMessageError(cStr);
	delete [] cStr;
}

void CViewC64::ShowMessageSuccess(CSlrString *showMessage)
{
	char *cStr = showMessage->GetStdASCII();
	this->ShowMessageSuccess(cStr);
	delete [] cStr;
}

void CViewC64::ShowMessageInfo(CSlrString *showMessage)
{
	char *cStr = showMessage->GetStdASCII();
	this->ShowMessageSuccess(cStr);
	delete [] cStr;
}

void CViewC64::ShowMessageWarning(const char *fmt, ...)
{
	mutexShowMessage->Lock();
	//memset(buffer, 0x00, BUFSIZE);

	va_list args;

	va_start(args, fmt);
	vsnprintf(notificationBuffer, BUFSIZE, fmt, args);
	va_end(args);
	notificationBuffer[BUFSIZE-1] = 0x00;
	
	viewMessages->AddLog(notificationBuffer);
	viewMessages->AddLog("\n");
	
	guiMain->ShowNotification(ImGuiToastType_Warning, 8888, NULL, notificationBuffer);
	
	LOGM("CViewC64::ShowMessage: %s", notificationBuffer);
	
	mutexShowMessage->Unlock();
}

void CViewC64::ShowMessageError(const char *fmt, ...)
{
	mutexShowMessage->Lock();
	//memset(buffer, 0x00, BUFSIZE);
	
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(notificationBuffer, BUFSIZE, fmt, args);
	va_end(args);
	notificationBuffer[BUFSIZE-1] = 0x00;
	
	viewMessages->AddLog(notificationBuffer);
	viewMessages->AddLog("\n");
	
	guiMain->ShowNotification(ImGuiToastType_Error, 9999, NULL, notificationBuffer);
	
	LOGM("CViewC64::ShowMessage: %s", notificationBuffer);
	
	mutexShowMessage->Unlock();
}

void CViewC64::ShowMessageSuccess(const char *fmt, ...)
{
	mutexShowMessage->Lock();
	//memset(buffer, 0x00, BUFSIZE);

	va_list args;

	va_start(args, fmt);
	vsnprintf(notificationBuffer, BUFSIZE, fmt, args);
	va_end(args);
	notificationBuffer[BUFSIZE-1] = 0x00;
	
	viewMessages->AddLog(notificationBuffer);
	viewMessages->AddLog("\n");
	
	guiMain->ShowNotification(ImGuiToastType_Success, 4444, NULL, notificationBuffer);
	
	LOGM("CViewC64::ShowMessage: %s", notificationBuffer);
	
	mutexShowMessage->Unlock();
}

void CViewC64::ShowMessageInfo(const char *fmt, ...)
{
	mutexShowMessage->Lock();
	//memset(buffer, 0x00, BUFSIZE);

	va_list args;

	va_start(args, fmt);
	vsnprintf(notificationBuffer, BUFSIZE, fmt, args);
	va_end(args);
	notificationBuffer[BUFSIZE-1] = 0x00;
	
	viewMessages->AddLog(notificationBuffer);
	viewMessages->AddLog("\n");
	
	guiMain->ShowNotification(ImGuiToastType_Info, 4444, NULL, notificationBuffer);
	
	LOGM("CViewC64::ShowMessage: %s", notificationBuffer);
	
	mutexShowMessage->Unlock();
}

void CViewC64::ShowMessage(ImGuiToastType_ toastType, const char *title, const char *fmt, ...)
{
	mutexShowMessage->Lock();
	//memset(buffer, 0x00, BUFSIZE);

	va_list args;

	va_start(args, fmt);
	vsnprintf(notificationBuffer, BUFSIZE, fmt, args);
	va_end(args);
	notificationBuffer[BUFSIZE-1] = 0x00;
	
	viewMessages->AddLog(notificationBuffer);
	viewMessages->AddLog("\n");
	
	guiMain->ShowNotification(toastType, 6666, title, notificationBuffer);
	
	mutexShowMessage->Unlock();
}

//
void CViewC64::FileSelected(CSlrString *filePath)
{
	LOGTODO("ShowDialogOpenFile CHECK MEMORY LEAKS");

	CSlrString *file = new CSlrString(filePath);
	
	systemFileDialogCallback->SystemDialogFileOpenSelected(file);
}

void CViewC64::FileSelectionCancelled()
{
	systemFileDialogCallback->SystemDialogFileOpenCancelled();
}

void CViewC64::ShowDialogSaveFile(CSystemFileDialogCallback *callback, std::list<CSlrString *> *extensions,
						CSlrString *defaultFileName, CSlrString *defaultFolder,
						CSlrString *windowTitle)
{
	LOGTODO("ShowDialogOpenFile CHECK MEMORY LEAKS");

	if (c64SettingsUseSystemFileDialogs)
	{
		SYS_DialogSaveFile(callback, extensions, defaultFileName, defaultFolder, windowTitle);
	}
	else
	{
		fileDialogPreviousView = guiMain->currentView;
		systemFileDialogCallback = callback;
	
		CSlrString *directoryPath;
		if (defaultFolder != NULL)
		{
			directoryPath = new CSlrString(defaultFolder);
		}
		else
		{
			directoryPath = new CSlrString("/");
		}
		
		CSlrString *firstExtension = extensions->front();
		viewSaveFile->Init(defaultFileName, firstExtension, directoryPath);
	}
}

void CViewC64::SaveFileSelected(CSlrString *fullFilePath, CSlrString *fileName)
{
	CSlrString *file = new CSlrString(fullFilePath);
	
	systemFileDialogCallback->SystemDialogFileSaveSelected(file);

}

void CViewC64::SaveFileSelectionCancelled()
{
	systemFileDialogCallback->SystemDialogFileSaveCancelled();
}

char *CViewC64::ATRD_GetPathForRoms_IMPL()
{
	LOGD("CViewC64::ATRD_GetPathForRoms_IMPL");
	char *buf;
	if (c64SettingsPathToAtariROMs == NULL)
	{
		buf = new char[MAX_BUFFER];
		sprintf(buf, ".");
	}
	else
	{
		buf = c64SettingsPathToAtariROMs->GetStdASCII();
	}
	
//	sprintf(buf, "%s/debugroms", gPathToDocuments);
	LOGD("ATRD_GetPathForRoms_IMPL path=%s", buf);
	
	LOGM("buf is=%s", buf);
	return buf;
}

void CViewC64::DebuggerServerWebSocketsStart()
{
	if (debuggerServer != NULL)
	{
		if (debuggerServer->isRunning)
		{
			LOGWarning("CViewC64::DebuggerServerWebSocketsStart: debuggerServer != NULL and isRunning=true");
		}
		else
		{
			debuggerServer->Start();
		}
	}
	else
	{
		debuggerServer = REMOTE_CreateDebuggerServerWebSockets(c64SettingsRunDebuggerServerWebSocketsPort);
		debuggerServer->Start();
	}
}

void CViewC64::DebuggerServerWebSocketsSetPort(int port)
{
	if (this->debuggerServer)
	{
		REMOTE_DebuggerServerWebSocketsSetPort(this->debuggerServer, port);
	}
}

void CViewC64::ApplicationShutdown()
{
	LOGD("CViewC64::ApplicationShutdown");
	
	firstStoreDefaultSymbols = false;
	viewC64->DefaultSymbolsStore();

//	guiMain->RemoveAllViews();
//
//	if (viewC64->debugInterfaceC64)
//	{
//		viewC64->debugInterfaceC64->Shutdown();
//	}
//	if (viewC64->debugInterfaceAtari)
//	{
//		viewC64->debugInterfaceAtari->Shutdown();
//	}
//	if (viewC64->debugInterfaceNes)
//	{
//		viewC64->debugInterfaceNes->Shutdown();
//	}
//	SYS_Sleep(100);
}

std::set<std::string> CViewC64::GetSupportedFileExtensions()
{
	std::set<std::string> extensions = {
		"prg", "d64", "x64", "g64", "p64", "crt", "reu", "sid", "snap", "vsf", "tap", "t64",
		"xex", "obx", "atr", "a8s",
		"sap", "cmc", "cm3", "cmr", "cms", "dmc", "dlt", "fc", "mpt", "mpd", "rmt", "tmc", "tm8", "tm2", "stil",
		"nes",
		"c64jukebox", "rtdl", "rdtl",
		"png", "jpg", "jpeg", "bmp", "gif", "psd", "pic", "hdr", "tga", "kla", "dd", "ddl", "aas", "art", "vce"
	};
	return extensions;
}

void CViewC64::GlobalDropFileCallback(char *filePath, bool consumedByView)
{
	// Note: drop event is also routed to a window on which the file is dropped, so we can define how the drop is exactly consumed. if not consumed by window then this is just a global callback to open a file.

	LOGD("CViewC64::GlobalDropFileCallback: filePath=%s consumedByView=%s", filePath, STRBOOL(consumedByView));

	if (consumedByView == false)
	{
		if (SYS_GetCurrentTimeInMillis() - c64dStartupTime < 500)
		{
			LOGD("CViewC64::GlobalDropFileCallback: sleep 500ms");
			SYS_Sleep(500);
		}

		CSlrString *slrPath = new CSlrString(filePath);
		viewC64->mainMenuHelper->LoadFile(slrPath);
		delete slrPath;
	}
}

void CViewC64::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	viewC64->mainMenuHelper->LoadFile(filePath);
}

void CViewC64::ViewFileBrowserCallbackOpenFile(fs::path path)
{
	LOGD("CViewC64::ViewFileBrowserCallbackOpenFile: %s", path.string().c_str());
	CSlrString *str = new CSlrString(path.string().c_str());
	viewC64->OpenFile(str);
	delete str;
}

void CViewC64::OpenFile(CSlrString *path)
{
	viewC64->mainMenuHelper->LoadFile(path);
}

void CViewC64::OpenFileDialog()
{
	viewC64->mainMenuHelper->OpenDialogOpenFile();
}
