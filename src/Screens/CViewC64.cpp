//
// C64 Debugger (C) Marcin Skoczylas, slajerek@gmail.com
//
// created on 2016-02-22

// define also in CGuiMain
//#define DO_NOT_USE_AUDIO_QUEUE

// TODO: move me ..
#define MAX_BUFFER 2048

extern "C"{
#include "c64mem.h"
}

#include "FontProFontIIx.h"

#include "CViewC64.h"
#include "SYS_Defs.h"
#include "VID_Main.h"
#include "VID_Blits.h"
#include "CGuiMain.h"
#include "CLayoutManager.h"
#include "RES_ResourceManager.h"
#include "CSlrFontProportional.h"
#include "VID_ImageBinding.h"
#include "GAM_GamePads.h"
#include "CByteBuffer.h"
#include "CSlrKeyboardShortcuts.h"
#include "SYS_KeyCodes.h"
#include "C64SettingsStorage.h"
#include "SYS_PIPE.h"
#include "CAudioChannelVice.h"
#include "CAudioChannelAtari.h"
#include "CAudioChannelNes.h"
#include "RetroDebuggerEmbeddedData.h"

#include "CDebugDataAdapter.h"
#include "imgui_freetype.h"

#include "CViewDataDump.h"
#include "CViewDataWatch.h"
#include "CViewMemoryMap.h"
#include "CViewDisassembly.h"
#include "CViewSourceCode.h"

#include "CViewC64Screen.h"
#include "CViewC64ScreenWrapper.h"

#include "CViewC64StateCIA.h"
#include "CViewC64StateREU.h"
#include "CViewEmulationCounters.h"
#include "CViewC64StateSID.h"
#include "CViewC64StateVIC.h"
#include "CViewDrive1541StateVIA.h"
#include "CViewEmulationState.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64VicControl.h"
#include "CViewC64SidPianoKeyboard.h"
#include "CViewC64MemoryDebuggerLayoutToolbar.h"
#include "CViewC64StateCPU.h"
#include "CViewInputEvents.h"
#include "CViewBreakpoints.h"
#include "CViewTimeline.h"
#include "CViewDriveStateCPU.h"

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

#include "CViewMainMenu.h"
#include "CViewSettingsMenu.h"
#include "CViewFileD64.h"
#include "CViewC64KeyMap.h"
#include "CViewC64AllGraphics.h"
#include "CViewC64SidTrackerHistory.h"
#include "CViewKeyboardShortcuts.h"
#include "CViewMonitorConsole.h"
#include "CViewSnapshots.h"
#include "CViewColodore.h"
#include "CViewAbout.h"
#include "CViewVicEditor.h"
#include "CViewJukeboxPlaylist.h"
#include "CMainMenuBar.h"
#include "CDebugMemoryMap.h"
#include "CDebugMemoryMapCell.h"

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
#include "SND_SoundEngine.h"
#include "CSlrFileFromOS.h"
#include "CColorsTheme.h"
#include "SYS_Threading.h"
#include "CDebugAsmSource.h"
#include "CDebuggerEmulatorPlugin.h"
#include "CSnapshotsManager.h"
#include "CDebugSymbolsSegmentC64.h"
#include "C64D_InitPlugins.h"

#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"

CViewC64 *viewC64 = NULL;

unsigned long c64dStartupTime = 0;

#define TEXT_ADDR	0x0400
#define COLOR_ADDR	0xD800

void TEST_Editor();
void TEST_Editor_Render();

// TODO: refactor this. after transition to ImGui the CViewC64 is no longer a view, it is just a application holder of other views.
CViewC64::CViewC64(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView("CViewC64", posX, posY, posZ, sizeX, sizeY)
{
	LOGM("CViewC64::CViewC64 starting init");
	
	isInitialized = false;
	
	this->name = "CViewC64";
	viewC64 = this;
	
	SYS_SetThreadName("CViewC64");
	
	c64dStartupTime = SYS_GetCurrentTimeInMillis();
	
	this->config = new CConfigStorageHjson(C64D_SETTINGS_HJSON_FILE_PATH);

	defaultFontPath = NULL;
	defaultFontSize = 13.0f;
	
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

	// init the Commodore 64 object
	this->InitViceC64();
		
	GAM_InitGamePads();
	
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

	
	// create fonts from Commodore 64/Atari800 kerne/al data
	this->CreateFonts();

	guiRenderFrameCounter = 0;
	isShowingRasterCross = false;

	
	///
	///
	
	this->InitViews();
	
	
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
		traversalOfViews.push_back(viewC64Disassemble);
		traversalOfViews.push_back(viewC64Disassemble2);
		traversalOfViews.push_back(viewC64MemoryDataDump);
		traversalOfViews.push_back(viewDrive1541MemoryDataDump);
		traversalOfViews.push_back(viewDrive1541Disassemble);
		traversalOfViews.push_back(viewDrive1541Disassemble2);
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
		traversalOfViews.push_back(viewAtariDisassemble);
		traversalOfViews.push_back(viewAtariMemoryDataDump);
		traversalOfViews.push_back(viewAtariMemoryMap);
		traversalOfViews.push_back(viewAtariMonitorConsole);
	}

	if (debugInterfaceNes != NULL)
	{
		traversalOfViews.push_back(viewNesScreen);
		traversalOfViews.push_back(viewNesDisassemble);
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
	viewC64MainMenu = new CViewMainMenu(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewC64MainMenu);
	
	viewC64SettingsMenu = new CViewSettingsMenu(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewC64SettingsMenu);
	
	if (this->debugInterfaceC64 != NULL)
	{
		viewFileD64 = new CViewFileD64(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewFileD64);
		
//		viewC64BreakpointsPC = new CViewBreakpoints(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->breakpointsPC);
//		guiMain->AddGuiElement(viewC64BreakpointsPC);
		
		viewC64Snapshots = new CViewSnapshots(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewC64Snapshots);
		
		viewC64KeyMap = new CViewC64KeyMap(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewC64KeyMap);

//		viewColodore = new CViewColodore(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//		guiMain->AddGuiElement(viewColodore);
	}
	else
	{
		viewFileD64 = NULL;
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

	viewKeyboardShortcuts = new CViewKeyboardShortcuts(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewKeyboardShortcuts);

	viewAbout = new CViewAbout(0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewAbout);
	
	// open/save file dialogs replacement
	viewSelectFile = new CGuiViewSelectFile(0, 0, posZ, SCREEN_WIDTH-80.0, SCREEN_HEIGHT, false, this);
	viewSelectFile->SetFont(fontCBMShifted, 2.0f);
//	guiMain->AddGuiElement(viewSelectFile);

	viewSaveFile = new CGuiViewSaveFile(0, 0, posZ, SCREEN_WIDTH-80.0, SCREEN_HEIGHT, this);
	viewSaveFile->SetFont(fontCBMShifted, 2.0f);
//	guiMain->AddGuiElement(viewSaveFile);
	
#if defined(RUN_COMMODORE64)
	//
	viewVicEditor = new CViewVicEditor("C64 VIC Editor", 0, 0, -3.0, SCREEN_WIDTH, SCREEN_HEIGHT);
//	guiMain->AddGuiElement(viewVicEditor);
#endif
	
	SYS_AddApplicationPauseResumeListener(this);
	
	
	//
	// Init menu bar
	this->mainMenuBar = new CMainMenuBar();
	
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
	
	isInitialized = true;
	
	// attach disks, cartridges etc
	C64DebuggerPerformStartupTasks();
	
	// restore selected layout
	CLayoutData *layoutData = guiMain->layoutManager->currentLayout;
	guiMain->layoutManager->SetLayoutAsync(layoutData, false);
	
	// and settings
	guiMain->fntConsole->image->SetLinearScaling(c64SettingsInterpolationOnDefaultFont);
	guiMain->fntConsoleInverted->image->SetLinearScaling(c64SettingsInterpolationOnDefaultFont);

	// register for dropfile callback
	guiMain->AddGlobalDropFileCallback(this);
	
	// restore open recent menu items
	recentlyOpenedFiles = new CRecentlyOpenedFiles(new CSlrString(C64D_RECENTS_FILE_NAME), this);

	TEST_Editor();
}

void CViewC64::ShowMainScreen()
{
	return;
	
//	if (c64SettingsIsInVicEditor)
//	{
//		guiMain->SetView(viewVicEditor);
//	}
//	else
//	{
//		guiMain->SetView(this);
//	}

	//	guiMain->SetView(viewKeyboardShortcuts);
//		guiMain->SetView(viewC64KeyMap);
	//	guiMain->SetView(viewAbout);
	//	guiMain->SetView(viewC64SettingsMenu);
	//	guiMain->SetView(viewC64MainMenu);
	//	guiMain->SetView(viewC64Breakpoints);
	//	guiMain->SetView(viewVicEditor);
//	guiMain->SetView(this->viewColodore);

	CheckMouseCursorVisibility();
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

	LOGM("CViewC64::InitViceC64: done");
}

void CViewC64::InitNestopia()
{
	LOGM("CViewC64::InitNestopia");

	this->debugInterfaceNes = new CDebugInterfaceNes(this);

	this->debugInterfaces.push_back(this->debugInterfaceNes);

	LOGM("CViewC64::InitNestopia: done");
}


void CViewC64::InitViews()
{
	// set mouse cursor outside at startup
	mouseCursorX = -SCREEN_WIDTH;
	mouseCursorY = -SCREEN_HEIGHT;
		
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
			guiMain->AddLayoutView(view);
		}
	}
}

void CViewC64::InitViceViews()
{
	// create views
	float minPosX = 10.0f;
	float minPosY = 30.0f;
	
	// note: views are created first to have dependencies fulfilled. They are added later to have them sorted.
	
	// this is regular c64 screen
	viewC64Screen = new CViewC64Screen("C64 Screen##direct", 510, 40, posZ, 540, 402, debugInterfaceC64);

	//	this->AddGuiElement(viewC64Screen);   this will be added on the top

	// wrapper wraps c64 screen with selectable display type: c64 screen, vic display, zoomed in c64 screen
//	viewC64ScreenWrapper = new CViewC64ScreenWrapper("C64 Screen", 510, 40, posZ, 540, 402, debugInterfaceC64);
	
	viewC64StateCPU = new CViewC64StateCPU("C64 CPU", 510, 5, posZ, 350, 35, debugInterfaceC64);

	
	// views
	viewC64MemoryMap = new CViewMemoryMap("C64 Memory map", 190, 20, posZ, 320, 280,
										  debugInterfaceC64, debugInterfaceC64->dataAdapterC64,
										  256, 256, 0x10000, true, false);	// 256x256 = 64kB

	viewDrive1541MemoryMap = new CViewMemoryMap("1541 Memory map", 120, 80, posZ, 200, 200,
												debugInterfaceC64, debugInterfaceC64->dataAdapterDrive1541,
												64, 1024, 0x10000, true, true);

	
	viewC64Disassembly = new CViewDisassembly("C64 Disassembly", 0, 20, posZ, 190, 420,
											  debugInterfaceC64->symbols, NULL, viewC64MemoryMap);
	
	viewC64Disassembly2 = new CViewDisassembly("C64 Disassembly 2", 100, 100, posZ, 200, 300,
												 debugInterfaceC64->symbols, NULL, viewC64MemoryMap);

	viewDrive1541Disassembly = new CViewDisassembly("1541 Disassembly", 110, 110, posZ, 200, 300,
													debugInterfaceC64->symbolsDrive1541, NULL, viewDrive1541MemoryMap);
	
	viewDrive1541Disassembly2 = new CViewDisassembly("1541 Disassembly 2", 120, 120, posZ, 200, 300,
													debugInterfaceC64->symbolsDrive1541, NULL, viewDrive1541MemoryMap);

	viewC64MemoryDataDump = new CViewDataDump("C64 Memory", 190, 300, posZ, 320, 140,
											  debugInterfaceC64->symbols, viewC64MemoryMap, viewC64Disassembly);
	viewC64Disassembly->SetViewDataDump(viewC64MemoryDataDump);


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
	
	viewDrive1541MemoryDataDump = new CViewDataDump("1541 Memory", 20, 30, posZ, 300, 300,
													debugInterfaceC64->symbols,
													viewDrive1541MemoryMap, viewDrive1541Disassembly);
	viewDrive1541Disassembly->SetViewDataDump(viewDrive1541MemoryDataDump);

	viewDrive1541MemoryDataWatch = new CViewDataWatch("1541 Data watch", 40, 40, posZ, 300, 300,
													  debugInterfaceC64->symbolsDrive1541,
													  viewDrive1541MemoryMap);
	//
	viewDrive1541MemoryDataDump2 = new CViewDataDump("1541 Memory 2", 10, 10, posZ, 300, 300,
													 debugInterfaceC64->symbols, viewDrive1541MemoryMap, viewDrive1541Disassembly);
	viewDrive1541Disassembly2->SetViewDataDump(viewDrive1541MemoryDataDump2);

	//
	viewDrive1541MemoryDataDump3 = new CViewDataDump("1541 Memory 3", 10, 10, posZ, 300, 300,
													 debugInterfaceC64->symbols, viewDrive1541MemoryMap, viewDrive1541Disassembly);

	// set first drive data dump as main to be controlled by drive memory map
	viewDrive1541MemoryMap->SetDataDumpView(viewDrive1541MemoryDataDump);
	
	//
	viewC64BreakpointsPC = new CViewBreakpoints("C64 PC Breakpoints", 10, 40, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->symbols, BREAKPOINT_TYPE_CPU_PC);

	viewC64BreakpointsMemory = new CViewBreakpoints("C64 Memory Breakpoints", 30, 50, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->symbols, BREAKPOINT_TYPE_MEMORY);

	viewC64BreakpointsRaster = new CViewBreakpoints("C64 Raster Breakpoints", 40, 40, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->symbols, BREAKPOINT_TYPE_RASTER_LINE);
	
	viewC64BreakpointsDrive1541PC = new CViewBreakpoints("1541 PC Breakpoints", 50, 50, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->symbolsDrive1541, BREAKPOINT_TYPE_CPU_PC);
	
	viewC64BreakpointsDrive1541Memory = new CViewBreakpoints("1541 Memory Breakpoints", 60, 60, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, this->debugInterfaceC64->symbolsDrive1541, BREAKPOINT_TYPE_MEMORY);
	
	//
	viewC64CartridgeMemoryMap = new CViewMemoryMap("C64 Cartridge memory map", 40, 70, posZ, 400, 300,
										  debugInterfaceC64, debugInterfaceC64->dataAdapterCartridgeC64,
										  512, 1024, 512*1024, true, false);	// 512*1024 = 512kB
	viewC64CartridgeMemoryMap->updateMapNumberOfFps = 1.0f;
	viewC64CartridgeMemoryMap->updateMapIsAnimateEvents = false;
	viewC64CartridgeMemoryMap->showCurrentExecutePC = false;
	
	viewC64CartridgeMemoryDataDump = new CViewDataDump("C64 Cartridge memory", 50, 50, posZ, 400, 300,
											  debugInterfaceC64->symbolsCartridgeC64, viewC64CartridgeMemoryMap, viewC64Disassembly);
	viewC64CartridgeMemoryDataDump->SetNumDigitsInAddress(5);

	//
	viewC64SourceCode = new CViewSourceCode("C64 Assembler source", 40, 70, posZ, 500, 350,
											debugInterfaceC64, debugInterfaceC64->dataAdapterC64, viewC64MemoryMap, viewC64Disassembly);
	//
	viewC64StateCIA = new CViewC64StateCIA("C64 CIA", 10, 40, posZ, 400, 200, debugInterfaceC64);

	viewC64StateSID = new CViewC64StateSID("C64 SID", 10, 40, posZ, 250, 270, debugInterfaceC64);

	viewC64StateVIC = new CViewC64StateVIC("C64 VIC", 10, 40, posZ, 300, 200, debugInterfaceC64);
	
	viewC64StateREU = new CViewC64StateREU("C64 REU", 10, 40, posZ, 300, 200, debugInterfaceC64);
	

	viewC64EmulationCounters = new CViewEmulationCounters("C64 Counters", 860, -1.25, posZ, 130, 43, debugInterfaceC64);

	viewDrive1541StateVIA = new CViewDrive1541StateVIA("1541 VIA", 10, 40, posZ, 300, 200, debugInterfaceC64);

	viewEmulationState = new CViewEmulationState("C64 Emulation", 10, 40, posZ, 350, 10, debugInterfaceC64);

	viewC64VicDisplay = new CViewC64VicDisplay("C64 VIC Display", 50, 50, posZ, 400, 500, debugInterfaceC64);

	viewC64VicControl = new CViewC64VicControl("C64 VIC Control", 10, 50, posZ, 100, 360, viewC64VicDisplay);

	viewC64MonitorConsole = new CViewMonitorConsole("C64 Monitor console", 40, 70, posZ, 500, 300, debugInterfaceC64);

	//
	viewDriveStateCPU = new CViewDriveStateCPU("1541 CPU", 20, 50, posZ, 300, 35, debugInterfaceC64);

	//
	viewC64AllGraphics = new CViewC64AllGraphics("C64 All graphics", 0, 0, posZ, 500, 500, debugInterfaceC64);

	//
	viewC64SidTrackerHistory = new CViewC64SidTrackerHistory("C64 SID Tracker history", 150, 40, posZ, 600, 400, (CDebugInterfaceVice*)debugInterfaceC64);

	viewC64SidPianoKeyboard = new CViewC64SidPianoKeyboard("C64 SID Piano keyboard", 50, 100, posZ, 400, 65, viewC64SidTrackerHistory);

	float timelineHeight = 40;
	viewC64Timeline = new CViewTimeline("C64 Timeline", 0, 440, posZ, 700, timelineHeight, debugInterfaceC64);

	// add sorted views
	debugInterfaceC64->AddView(viewC64Screen);
	debugInterfaceC64->AddView(viewC64StateCPU);
	debugInterfaceC64->AddView(viewC64Disassembly);
	debugInterfaceC64->AddView(viewC64Disassembly2);
	viewC64Disassembly2->visible = false;
	debugInterfaceC64->AddView(viewC64SourceCode);
	viewC64SourceCode->visible = false;
	debugInterfaceC64->AddView(viewC64MemoryDataDump);
	debugInterfaceC64->AddView(viewC64MemoryDataDump2);
	viewC64MemoryDataDump2->visible = false;
	debugInterfaceC64->AddView(viewC64MemoryDataDump3);
	viewC64MemoryDataDump3->visible = false;
	debugInterfaceC64->AddView(viewC64MemoryDataWatch);
	viewC64MemoryDataWatch->visible = false;
	debugInterfaceC64->AddView(viewC64MemoryMap);

	debugInterfaceC64->AddView(viewC64BreakpointsPC);
	viewC64BreakpointsPC->visible = false;
	debugInterfaceC64->AddView(viewC64BreakpointsMemory);
	viewC64BreakpointsMemory->visible = false;
	debugInterfaceC64->AddView(viewC64BreakpointsRaster);
	viewC64BreakpointsRaster->visible = false;
	
	debugInterfaceC64->AddView(viewC64CartridgeMemoryDataDump);
	viewC64CartridgeMemoryDataDump->visible = false;
	debugInterfaceC64->AddView(viewC64CartridgeMemoryMap);
	viewC64CartridgeMemoryMap->visible = false;

	debugInterfaceC64->AddView(viewC64StateCIA);
	viewC64StateCIA->visible = false;
	debugInterfaceC64->AddView(viewC64StateSID);
	viewC64StateSID->visible = false;
	debugInterfaceC64->AddView(viewC64StateVIC);
	viewC64StateVIC->visible = false;
	debugInterfaceC64->AddView(viewC64StateREU);
	viewC64StateREU->visible = false;

	debugInterfaceC64->AddView(viewC64VicDisplay);
	viewC64VicDisplay->visible = false;
	debugInterfaceC64->AddView(viewC64VicControl);
	viewC64VicControl->visible = false;
	
//	debugInterfaceC64->AddView(viewC64AllGraphics);
//	viewC64AllGraphics->visible = false;
	debugInterfaceC64->AddView(viewC64SidTrackerHistory);
	viewC64SidTrackerHistory->visible = false;
	debugInterfaceC64->AddView(viewC64SidPianoKeyboard);
	viewC64SidPianoKeyboard->visible = false;

	debugInterfaceC64->AddView(viewDriveStateCPU);
	viewDriveStateCPU->visible = false;
	debugInterfaceC64->AddView(viewDrive1541Disassembly);
	viewDrive1541Disassembly->visible = false;
	debugInterfaceC64->AddView(viewDrive1541Disassembly2);
	viewDrive1541Disassembly2->visible = false;
	debugInterfaceC64->AddView(viewDrive1541MemoryDataDump);
	viewDrive1541MemoryDataDump->visible = false;
	debugInterfaceC64->AddView(viewDrive1541MemoryDataDump2);
	viewDrive1541MemoryDataDump2->visible = false;
	debugInterfaceC64->AddView(viewDrive1541MemoryDataDump3);
	viewDrive1541MemoryDataDump3->visible = false;
	debugInterfaceC64->AddView(viewDrive1541MemoryDataWatch);
	viewDrive1541MemoryDataWatch->visible = false;
	debugInterfaceC64->AddView(viewDrive1541MemoryMap);
	viewDrive1541MemoryMap->visible = false;
	
	debugInterfaceC64->AddView(viewC64BreakpointsDrive1541PC);
	viewC64BreakpointsDrive1541PC->visible = false;
	debugInterfaceC64->AddView(viewC64BreakpointsDrive1541Memory);
	viewC64BreakpointsDrive1541Memory->visible = false;
	
	debugInterfaceC64->AddView(viewDrive1541StateVIA);
	viewDrive1541StateVIA->visible = false;

	debugInterfaceC64->AddView(viewC64MonitorConsole);
	viewC64MonitorConsole->visible = false;

	debugInterfaceC64->AddView(viewEmulationState);
	viewEmulationState->visible = false;
	debugInterfaceC64->AddView(viewC64EmulationCounters);
	viewC64EmulationCounters->visible = true;
	
	debugInterfaceC64->AddView(viewC64Timeline);

	
	
	// add c64 screen on top of all other views
//	this->AddGuiElement(viewC64Screen);
//	this->AddGuiElement(viewC64ScreenWrapper);
//	guiMain->AddView(viewC64ScreenWrapper);
}

// ATARI800 views
void CViewC64::InitAtari800Views()
{
	///
	viewAtariScreen = new CViewAtariScreen("Atari Screen", 510, 40, posZ, 612, 402, debugInterfaceAtari);

	viewAtariStateCPU = new CViewAtariStateCPU("Atari CPU", 510, 5, posZ, 350, 35, debugInterfaceAtari);

	//
	
	viewAtariMemoryMap = new CViewMemoryMap("Atari Memory map", 190, 20, posZ, 320, 280,
											debugInterfaceAtari, debugInterfaceAtari->dataAdapter,
											256, 256, 0x10000, true, false);	// 256x256 = 64kB

	viewAtariDisassembly = new CViewDisassembly("Atari Disassembly", 0, 20, posZ, 190, 420,
												debugInterfaceAtari->symbols, NULL, viewAtariMemoryMap);

	//
	viewAtariBreakpointsPC = new CViewBreakpoints("Atari PC Breakpoints", 70, 70, posZ, 200, 300, this->debugInterfaceAtari->symbols, BREAKPOINT_TYPE_CPU_PC);

	viewAtariBreakpointsMemory = new CViewBreakpoints("Atari Memory Breakpoints", 90, 70, posZ, 200, 300, this->debugInterfaceAtari->symbols, BREAKPOINT_TYPE_MEMORY);

	//
	viewAtariSourceCode = new CViewSourceCode("Atari Assembler source", 40, 70, posZ, 500, 350,
											  debugInterfaceAtari, debugInterfaceAtari->dataAdapter, viewAtariMemoryMap, viewAtariDisassembly);

	viewAtariMemoryDataDump = new CViewDataDump("Atari Memory", 190, 300, posZ, 320, 140,
												debugInterfaceAtari->symbols, viewAtariMemoryMap, viewAtariDisassembly);
	viewAtariMemoryDataDump->selectedCharset = 2;

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

	// add sorted views
	debugInterfaceAtari->AddView(viewAtariScreen);
	debugInterfaceAtari->AddView(viewAtariStateCPU);
	debugInterfaceAtari->AddView(viewAtariDisassembly);
	debugInterfaceAtari->AddView(viewAtariSourceCode);
	viewAtariSourceCode->visible = false;
	debugInterfaceAtari->AddView(viewAtariMemoryDataDump);
	debugInterfaceAtari->AddView(viewAtariMemoryDataWatch);
	viewAtariMemoryDataWatch->visible = false;
	debugInterfaceAtari->AddView(viewAtariMemoryMap);
	debugInterfaceAtari->AddView(viewAtariBreakpointsPC);
	viewAtariBreakpointsPC->visible = false;
	debugInterfaceAtari->AddView(viewAtariBreakpointsMemory);
	viewAtariBreakpointsMemory->visible = false;
	debugInterfaceAtari->AddView(viewAtariStateANTIC);
	viewAtariStateANTIC->visible = false;
	debugInterfaceAtari->AddView(viewAtariStateGTIA);
	viewAtariStateGTIA->visible = false;
	debugInterfaceAtari->AddView(viewAtariStatePIA);
	viewAtariStatePIA->visible = false;
	debugInterfaceAtari->AddView(viewAtariStatePOKEY);
	viewAtariStatePOKEY->visible = false;
	debugInterfaceAtari->AddView(viewAtariMonitorConsole);
	viewAtariMonitorConsole->visible = false;
	debugInterfaceAtari->AddView(viewAtariEmulationCounters);
	viewAtariEmulationCounters->visible = true;
	debugInterfaceAtari->AddView(viewAtariTimeline);
}

void CViewC64::InitNestopiaViews()
{
	///
	viewNesScreen = new CViewNesScreen("NES Screen", 510, 40, posZ, 408, 402, debugInterfaceNes);

	viewNesStateCPU = new CViewNesStateCPU("NES CPU", 510, 5, posZ, 290, 35, debugInterfaceNes);

	viewNesMemoryMap = new CViewMemoryMap("NES Memory map", 190, 20, posZ, 320, 280,
										  debugInterfaceNes, debugInterfaceNes->dataAdapter,
										  256, 256, 0x10000, true, false);	// 256x256 = 64kB

	viewNesDisassembly = new CViewDisassembly("NES Disassembly", 0, 20, posZ, 190, 424,
												debugInterfaceNes->symbols, NULL, viewNesMemoryMap);
		
	//
	viewNesBreakpointsPC = new CViewBreakpoints("NES PC Breakpoints", 70, 70, posZ, 200, 300, this->debugInterfaceNes->symbols, BREAKPOINT_TYPE_CPU_PC);

	viewNesBreakpointsMemory = new CViewBreakpoints("NES Memory Breakpoints", 90, 70, posZ, 200, 300, this->debugInterfaceNes->symbols, BREAKPOINT_TYPE_MEMORY);

	//
	viewNesSourceCode = new CViewSourceCode("NES Assembler source", 40, 70, posZ, 500, 350,
											  debugInterfaceNes, debugInterfaceNes->dataAdapter, viewNesMemoryMap, viewNesDisassembly);


	viewNesMemoryDataDump = new CViewDataDump("NES Memory", 190, 300, posZ, 320, 144,
											  debugInterfaceNes->symbols, viewNesMemoryMap, viewNesDisassembly);
	viewNesDisassembly->SetViewDataDump(viewNesMemoryDataDump);

	viewNesMemoryDataWatch = new CViewDataWatch("NES Watches", 140, 140, posZ, 300, 300,
												  debugInterfaceNes->symbols, viewNesMemoryMap);

	viewNesPpuNametables = new CViewNesPpuNametables("NES Nametables", 10, 40, posZ, 400, 200, debugInterfaceNes);

	viewNesPpuNametableMemoryMap = new CViewMemoryMap("NES Nametables Memory map", 140, 140, posZ, 300, 300, debugInterfaceNes,
													  debugInterfaceNes->dataAdapterPpuNmt, 64, 64, 0x1000, false, false);

	viewNesPpuNametableMemoryDataDump = new CViewDataDump("NES Nametables Memory", 140, 250, posZ, 300, 150,
													debugInterfaceNes->symbolsPpuNmt, viewNesPpuNametableMemoryMap, viewNesDisassembly);
	debugInterfaceNes->dataAdapterPpuNmt->SetViewMemoryMap(viewNesPpuNametableMemoryMap);
//	viewNesDisassemble->SetViewDataDump(viewNesMemoryDataDumpPpuNmt);

	
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

	// add sorted views
	debugInterfaceNes->AddView(viewNesScreen);
	debugInterfaceNes->AddView(viewNesStateCPU);
	debugInterfaceNes->AddView(viewNesDisassembly);
	debugInterfaceNes->AddView(viewNesSourceCode);
	viewNesSourceCode->visible = false;
	debugInterfaceNes->AddView(viewNesMemoryDataDump);
	debugInterfaceNes->AddView(viewNesMemoryDataWatch);
	viewNesMemoryDataWatch->visible = false;
	debugInterfaceNes->AddView(viewNesMemoryMap);
	debugInterfaceNes->AddView(viewNesBreakpointsPC);
	viewNesBreakpointsPC->visible = false;
	debugInterfaceNes->AddView(viewNesBreakpointsMemory);
	viewNesBreakpointsMemory->visible = false;
	debugInterfaceNes->AddView(viewNesStateAPU);
	viewNesStateAPU->visible = false;
	debugInterfaceNes->AddView(viewNesPianoKeyboard);
	viewNesPianoKeyboard->visible = false;
	debugInterfaceNes->AddView(viewNesStatePPU);
	viewNesStatePPU->visible = false;
	debugInterfaceNes->AddView(viewNesPpuPalette);
	debugInterfaceNes->AddView(viewNesPpuNametables);
	viewNesPpuNametables->visible = false;
	debugInterfaceNes->AddView(viewNesPpuNametableMemoryDataDump);
	viewNesPpuNametableMemoryDataDump->visible = false;
	debugInterfaceNes->AddView(viewNesPpuNametableMemoryMap);
	viewNesPpuNametableMemoryMap->visible = false;
	debugInterfaceNes->AddView(viewNesPpuPatterns);
	viewNesPpuPatterns->visible = false;
	debugInterfaceNes->AddView(viewNesPpuAttributes);
	viewNesPpuAttributes->visible = false;
	debugInterfaceNes->AddView(viewNesPpuOam);
	viewNesPpuOam->visible = false;
	debugInterfaceNes->AddView(viewNesInputEvents);
	viewNesInputEvents->visible = false;
	debugInterfaceNes->AddView(viewNesMonitorConsole);
	viewNesMonitorConsole->visible = false;
	debugInterfaceNes->AddView(viewNesEmulationCounters);
	debugInterfaceNes->AddView(viewNesTimeline);
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
			debugInterfaceC64->snapshotsManager->ClearSnapshotsHistory();
			debugInterfaceC64->HardReset();
			debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
			((CDebugInterfaceVice *)debugInterfaceC64)->audioChannel->Start();
			((CDebugInterfaceVice *)debugInterfaceC64)->RefreshSync();
		}

		// add views
		for (std::list<CGuiView *>::iterator it = debugInterfaceVice->views.begin(); it != debugInterfaceVice->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddView(view);
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
			debugInterfaceAtari->snapshotsManager->ClearSnapshotsHistory();
			debugInterfaceAtari->HardReset();
			debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_RUNNING);
			debugInterfaceAtari->audioChannel->Start();
		}
		
		// add views
		for (std::list<CGuiView *>::iterator it = debugInterfaceAtari->views.begin(); it != debugInterfaceAtari->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddView(view);
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
			debugInterfaceNes->snapshotsManager->ClearSnapshotsHistory();
			debugInterfaceNes->HardReset();
			debugInterfaceNes->SetDebugMode(DEBUGGER_MODE_RUNNING);
			debugInterfaceNes->audioChannel->Start();
		}
		
		// add views
		for (std::list<CGuiView *>::iterator it = debugInterfaceNes->views.begin(); it != debugInterfaceNes->views.end(); it++)
		{
			CGuiView *view = *it;
			guiMain->AddView(view);
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
	debugInterfaceC64->snapshotsManager->ClearSnapshotsHistory();
	debugInterfaceC64->isRunning = false;
	((CDebugInterfaceVice *)debugInterfaceC64)->audioChannel->Stop();

	// remove views
	for (std::list<CGuiView *>::iterator it = debugInterfaceC64->views.begin(); it != debugInterfaceC64->views.end(); it++)
	{
		CGuiView *view = *it;
		guiMain->RemoveView(view);
	}
	debugInterfaceC64->UnlockMutex();
	guiMain->UnlockMutex();
}

void CViewC64::StopAtari800EmulationThread()
{
	guiMain->LockMutex();
	debugInterfaceAtari->LockMutex();
	debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_PAUSED);
	debugInterfaceAtari->snapshotsManager->ClearSnapshotsHistory();
	debugInterfaceAtari->isRunning = false;
	debugInterfaceAtari->audioChannel->Stop();

	// remove views
	for (std::list<CGuiView *>::iterator it = debugInterfaceAtari->views.begin(); it != debugInterfaceAtari->views.end(); it++)
	{
		CGuiView *view = *it;
		guiMain->RemoveView(view);
	}
	debugInterfaceAtari->UnlockMutex();
	guiMain->UnlockMutex();
}

void CViewC64::StopNestopiaEmulationThread()
{
	guiMain->LockMutex();
	debugInterfaceNes->LockMutex();
	debugInterfaceNes->SetDebugMode(DEBUGGER_MODE_PAUSED);
	debugInterfaceNes->snapshotsManager->ClearSnapshotsHistory();
	debugInterfaceNes->isRunning = false;
	debugInterfaceNes->audioChannel->Stop();

	// remove views
	for (std::list<CGuiView *>::iterator it = debugInterfaceNes->views.begin(); it != debugInterfaceNes->views.end(); it++)
	{
		CGuiView *view = *it;
		guiMain->RemoveView(view);
	}
	debugInterfaceNes->UnlockMutex();
	guiMain->UnlockMutex();
}

void CEmulationThreadC64::ThreadRun(void *data)
{
	ThreadSetName("c64");

	LOGD("CEmulationThreadC64::ThreadRun");
		
	viewC64->debugInterfaceC64->RunEmulationThread();
	
	LOGD("CEmulationThreadC64::ThreadRun: finished");
}

void CEmulationThreadAtari::ThreadRun(void *data)
{
	ThreadSetName("atari");
	
	viewC64->debugInterfaceAtari->SetMachineType(c64SettingsAtariMachineType);
	viewC64->debugInterfaceAtari->SetRamSizeOption(c64SettingsAtariRamSizeOption);
	viewC64->debugInterfaceAtari->SetVideoSystem(c64SettingsAtariVideoSystem);

	LOGD("CEmulationThreadAtari::ThreadRun");
	
	viewC64->debugInterfaceAtari->RunEmulationThread();
	
	LOGD("CEmulationThreadAtari::ThreadRun: finished");
}

void CEmulationThreadNes::ThreadRun(void *data)
{
	ThreadSetName("nes");
	
	LOGD("CEmulationThreadNes::ThreadRun");
	
	viewC64->debugInterfaceNes->RunEmulationThread();
	
	LOGD("CEmulationThreadNes::ThreadRun: finished");
}


void CViewC64::DoLogic()
{
//	viewC64MemoryMap->DoLogic();
//	viewDrive1541MemoryMap->DoLogic();
	
//	if (nextScreenUpdateFrame < frameCounter)
//	{
//		//RefreshScreen();
//
//		nextScreenUpdateFrame = frameCounter;
//		
//	}

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
	
//	if (frameCounter % 2 == 0)
	{
		//if (viewC64ScreenWrapper->visible)   always do this anyway
		
		if (debugInterfaceC64 && debugInterfaceC64->isRunning && viewC64Screen)
		{
			viewC64StateVIC->UpdateSpritesImages();
			viewC64Screen->RefreshScreen();
		}

		if (debugInterfaceAtari && debugInterfaceAtari->isRunning && viewAtariScreen)
		{
			viewAtariScreen->RefreshScreen();
		}

		if (debugInterfaceNes && debugInterfaceNes->isRunning && viewNesScreen)
		{
			viewNesScreen->RefreshScreen();
		}
	}
	
	
	//////////

#ifdef RUN_COMMODORE64
	// copy current state of VIC
	c64d_vicii_copy_state(&(this->currentViciiState));

	viewC64VicDisplay->UpdateViciiState();

	this->UpdateViciiColors();
	
	//////////
	
	if (viewC64VicDisplay->canScrollDisassemble)
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
#endif
	
	///
#ifdef RUN_ATARI
	viewAtariDisassembly->SetCurrentPC(debugInterfaceAtari->GetCpuPC());
#endif

#ifdef RUN_NES
	viewNesDisassembly->SetCurrentPC(debugInterfaceNes->GetCpuPC());
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
	
	if (viewC64Screen->showZoomedScreen)
	{
		viewC64Screen->RenderZoomedScreen(rasterToShowX, rasterToShowY);
	}
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
	ImGui::SetCurrentFont(imFontDefault);
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
	
	TEST_Editor_Render();
}

///////////////

void CViewC64::UpdateViciiColors()
{
	int rasterX = viciiStateToShow.raster_cycle*8;
	int rasterY = viciiStateToShow.raster_line;
	
	// update current colors for rendering states
	
	this->rasterToShowX = rasterX;
	this->rasterToShowY = rasterY;
	this->rasterCharToShowX = (viewC64->viciiStateToShow.raster_cycle - 0x11);
	this->rasterCharToShowY = (viewC64->viciiStateToShow.raster_line - 0x32) / 8;
	
	//LOGD("       |   rasterCharToShowX=%3d rasterCharToShowY=%3d", rasterCharToShowX, rasterCharToShowY);
	
	if (rasterCharToShowX < 0)
	{
		rasterCharToShowX = 0;
	}
	else if (rasterCharToShowX > 39)
	{
		rasterCharToShowX = 39;
	}
	
	if (rasterCharToShowY < 0)
	{
		rasterCharToShowY = 0;
	}
	else if (rasterCharToShowY > 24)
	{
		rasterCharToShowY = 24;
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
		this->colorToShowD800 = color_ram_ptr[ rasterCharToShowY * 40 + rasterCharToShowX ] & 0x0F;
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
	if (viewC64->viewC64VicDisplay->visible
		|| viewC64->viewVicEditor->visible)
	{
		viewC64->viewC64VicDisplay->backupRenderDataWithColors = !viewC64->viewC64VicDisplay->backupRenderDataWithColors;
	}
	else
	{
		viewC64MemoryDataDump->renderDataWithColors = !viewC64MemoryDataDump->renderDataWithColors;
		viewC64->viewC64VicDisplay->backupRenderDataWithColors = viewC64MemoryDataDump->renderDataWithColors;
		viewC64AllGraphics->UpdateRenderDataWithColors();
	}
}

void CViewC64::SetIsMulticolorDataDump(bool isMultiColor)
{
	if (viewC64->viewC64VicDisplay->visible
		|| viewC64->viewVicEditor->visible)
	{
		viewC64->viewC64VicDisplay->backupRenderDataWithColors = isMultiColor;
	}
	else
	{
		viewC64MemoryDataDump->renderDataWithColors = isMultiColor;
		viewC64->viewC64VicDisplay->backupRenderDataWithColors = isMultiColor;
		viewC64AllGraphics->UpdateRenderDataWithColors();
	}
}


void CViewC64::SwitchIsShowRasterBeam()
{
	this->isShowingRasterCross = !this->isShowingRasterCross;
}

void CViewC64::StepOverInstruction()
{
	for (std::vector<CDebugInterface *>::iterator it = debugInterfaces.begin(); it != debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		debugInterface->StepOverInstruction();
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

void CViewC64::HardReset()
{
	if (c64SettingsRestartAudioOnEmulationReset)
	{
		// TODO: gSoundEngine->RestartAudioUnit();
	}
	
	// TODO: CViewC64::HardReset  make generic & move execute markers to debug interface
	if (debugInterfaceC64)
	{
		debugInterfaceC64->HardReset();
		viewC64MemoryMap->ClearExecuteMarkers();
		viewC64->viewDrive1541MemoryMap->ClearExecuteMarkers();
	}
	
	if (debugInterfaceAtari)
	{
		debugInterfaceAtari->HardReset();
		viewAtariMemoryMap->ClearExecuteMarkers();
	}
	
	if (debugInterfaceNes)
	{
		debugInterfaceNes->HardReset();
		LOGTODO("                           viewNesMemoryMap                     !!!!!!!!!! ");
		//				viewNesMemoryMap->ClearExecuteMarkers();
	}
	
	if (c64SettingsIsInVicEditor)
	{
		viewC64->viewC64VicControl->UnlockAll();
	}
}

void CViewC64::SoftReset()
{
	if (c64SettingsRestartAudioOnEmulationReset)
	{
		// TODO: gSoundEngine->RestartAudioUnit();
	}
	
	// TODO: make a list of avaliable interfaces and iterate
	if (debugInterfaceC64)
	{
		debugInterfaceC64->Reset();
	}
	
	if (debugInterfaceAtari)
	{
		debugInterfaceAtari->Reset();
	}
	
	if (debugInterfaceNes)
	{
		debugInterfaceNes->Reset();
	}
	
	if (c64SettingsIsInVicEditor)
	{
		viewC64->viewC64VicControl->UnlockAll();
	}
}

CViewDisassembly *CViewC64::GetActiveDisassembleView()
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

void CViewC64::SwitchUseKeyboardAsJoystick()
{
	viewC64SettingsMenu->menuItemUseKeyboardAsJoystick->SwitchToNext();
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
		
		viewC64->viewC64AllGraphics->UpdateShowIOButton();
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
	}
	
}

void CViewC64::MoveFocusToPrevView()
{
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
	}
}

//////
extern "C" {
	void machine_drive_flush(void);
}

bool CViewC64::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64::KeyDown, keyCode=%4.4x (%d) %c", keyCode, keyCode, keyCode);

#if defined(LOG_KEYBOARD_PRESS_KEY_NAME)
	CSlrString *keyCodeStr = SYS_KeyCodeToString(keyCode);
	char *str = keyCodeStr->GetStdASCII();
	LOGI("                   KeyDown=%s", str);
	delete [] str;
	delete keyCodeStr;
#endif
	
	if (mainMenuBar->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
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
//		guiMain->ShowMessage("mapped");
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
		if (focusElement != viewC64->viewC64Disassemble
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
		if (debugInterfaceC64 && viewC64ScreenWrapper->hasFocus)
		{
			return viewC64ScreenWrapper->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
		
		if (debugInterfaceAtari && viewAtariScreen->hasFocus)
		{
			return viewAtariScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
		
		if (debugInterfaceNes && viewNesScreen->hasFocus)
		{
			return viewNesScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
	}
	*/
	
	// TODO: is this needed?
	
	/*
	// TODO: this is a temporary UX workaround for step over jsr
	CSlrKeyboardShortcut *shortcut = guiMain->keyboardShortcuts->FindShortcut(KBZONE_DISASSEMBLE, keyCode, isShift, isAlt, isControl, isSuper);

	if (shortcut == mainMenuBar->kbsStepOverJsr)
	{
		if (this->debugInterfaceC64)
		{
			if (focusElement != viewDrive1541Disassemble && viewC64Disassemble->visible)
			{
				viewC64Disassemble->StepOverJsr();
				return true;
			}
			if (focusElement != viewC64Disassemble && viewDrive1541Disassemble->visible)
			{
				viewDrive1541Disassemble->StepOverJsr();
				return true;
			}
		}
		
		if (this->debugInterfaceAtari)
		{
			viewAtariDisassemble->StepOverJsr();
			return true;
		}
		
		if (this->debugInterfaceNes)
		{
			viewNesDisassemble->StepOverJsr();
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
	
//	if (debugInterfaceC64 && viewC64ScreenWrapper->hasFocus)
//	{
//		return viewC64ScreenWrapper->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
//
//	if (debugInterfaceAtari && viewAtariScreen->hasFocus)
//	{
//		return viewAtariScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
//	
//	if (debugInterfaceNes && viewNesScreen->hasFocus)
//	{
//		return viewNesScreen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
//	}
	
	return false; //CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64::KeyUp, keyCode=%d isShift=%d isAlt=%d isControl=%d", keyCode, isShift, isAlt, isControl);
	
#if defined(LOG_KEYBOARD_PRESS_KEY_NAME)
	CSlrString *keyCodeStr = SYS_KeyCodeToString(keyCode);
	char *str = keyCodeStr->GetStdASCII();
	LOGI("                   KeyUp=%s", str);
	delete [] str;
	delete keyCodeStr;
#endif
	
	if (keyCode >= MTKEY_F1 && keyCode <= MTKEY_F8 && !guiMain->isControlPressed)
	{
		if (debugInterfaceC64 && debugInterfaceC64->isRunning && viewC64Screen->hasFocus)
		{
			return viewC64Screen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
		}
		
		if (debugInterfaceAtari && debugInterfaceAtari->isRunning && viewAtariScreen->hasFocus)
		{
			return viewAtariScreen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
		}

		if (debugInterfaceNes && debugInterfaceNes->isRunning && viewNesScreen->hasFocus)
		{
			return viewNesScreen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
		}
	}

	//
	// TODO: another nasty UX workaround for memory map, generalize me
	//
	if (debugInterfaceC64 && debugInterfaceC64->isRunning && viewC64MemoryMap->visible)
	{
		if (keyCode == MTKEY_SPACEBAR && !isShift && !isAlt && !isControl)
		{
			if (viewC64MemoryMap->IsInside(guiMain->mousePosX, guiMain->mousePosY))
			{
				this->SetFocus(viewC64MemoryMap);
				if (viewC64MemoryMap->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
					return true;
			}
		}
	}

	if (debugInterfaceAtari && debugInterfaceAtari->isRunning && viewAtariMemoryMap->visible)
	{
		if (keyCode == MTKEY_SPACEBAR && !isShift && !isAlt && !isControl)
		{
			if (viewAtariMemoryMap->IsInside(guiMain->mousePosX, guiMain->mousePosY))
			{
				this->SetFocus(viewAtariMemoryMap);
				if (viewAtariMemoryMap->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
					return true;
			}
		}
	}

	if (debugInterfaceNes && debugInterfaceNes->isRunning && viewNesMemoryMap->visible)
	{
		if (keyCode == MTKEY_SPACEBAR && !isShift && !isAlt && !isControl)
		{
			if (viewNesMemoryMap->IsInside(guiMain->mousePosX, guiMain->mousePosY))
			{
				this->SetFocus(viewNesMemoryMap);
				if (viewNesMemoryMap->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
					return true;
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
	
	// TODO: is this correct?
	
	if (debugInterfaceC64 && debugInterfaceC64->isRunning && viewC64Screen->visible)
	{
		viewC64Screen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	}

	if (debugInterfaceAtari && debugInterfaceAtari->isRunning && viewAtariScreen->visible)
	{
		viewAtariScreen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	}

	if (debugInterfaceNes && debugInterfaceNes->isRunning && viewNesScreen->visible)
	{
		viewNesScreen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	}

	return false; //CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
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

			if (guiElement->IsFocusable() && ((focusElement != guiElement) || (guiElement->hasFocus == false)))
			{
				SetFocus((CGuiView *)guiElement);
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
//	LOGG("CViewC64::DoNotTouchedMove, mouseCursor=%f %f", mouseCursorX, mouseCursorY);

	if (guiMain->IsMouseCursorVisible() == false)
	{
		guiMain->SetMouseCursorVisible(true);
		mouseCursorVisibilityCounter = 0;
	}

	mouseCursorX = x;
	mouseCursorY = y;
	
	return false;
	
	return CGuiView::DoNotTouchedMove(x, y);
}

bool CViewC64::DoScrollWheel(float deltaX, float deltaY)
{
	LOGG("CViewC64::DoScrollWheel, mouseCursor=%f %f", mouseCursorX, mouseCursorY);

	// TODO: ugly hack, fix me
#if defined(RUN_COMMODORE64)
	if (viewC64Screen->showZoomedScreen)
	{
		if (viewC64Screen->IsInsideZoomedScreen(mouseCursorX, mouseCursorY))
		{
			viewC64Screen->DoScrollWheel(deltaX, deltaY);
			return true;
		}
	}
#endif	
	
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

void CViewC64::ApplicationEnteredBackground()
{
	LOGG("CViewC64::ApplicationEnteredBackground");
	
	// workaround for alt+tab
	
	// TODO: generalize me
	if (this->debugInterfaceC64)
	{
		viewC64Screen->KeyUpModifierKeys(true, true, true);
	}
	
	if (this->debugInterfaceAtari)
	{
		viewAtariScreen->KeyUpModifierKeys(true, true, true);
	}

	if (this->debugInterfaceNes)
	{
		viewNesScreen->KeyUpModifierKeys(true, true, true);
	}
}

void CViewC64::ApplicationEnteredForeground()
{
	LOGG("CViewC64::ApplicationEnteredForeground");

	// workaround for alt+tab
	if (this->debugInterfaceC64)
	{
		viewC64Screen->KeyUpModifierKeys(true, true, true);
	}
	
	if (this->debugInterfaceAtari)
	{
		viewAtariScreen->KeyUpModifierKeys(true, true, true);
	}

	if (this->debugInterfaceNes)
	{
		viewNesScreen->KeyUpModifierKeys(true, true, true);
	}
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

	uint8 *charRom = debugInterfaceVice->GetCharRom();

	uint8 *charData;

	charData = charRom;
	fontCBM1 = ProcessFonts(charData, true);

	charData = charRom + 0x0800;
	fontCBM2 = ProcessFonts(charData, true);

	fontCBMShifted = ProcessFonts(charData, false);
	
	fontAtari = NULL;
	
#if defined(RUN_ATARI)
	u8 charRomAtari[0x0800];
	MEMORY_GetCharsetScreenCodes(charRomAtari);
	fontAtari = ProcessFonts(charRomAtari, true);
#endif
	
//	u64 t2 = SYS_GetCurrentTimeInMillis();
//	LOGD("time=%u", t2-t1);
}

void CViewC64::CreateDefaultUIFont()
{
	ImGuiIO& io = ImGui::GetIO();
		
#if defined(MACOS)
	config->GetString("uiDefaultFont", &defaultFontPath, "CousineRegular");
	config->GetFloat("uiDefaultFontSize", &defaultFontSize, 16.0f);
	config->GetInt("uiDefaultFontOversampling", &defaultFontOversampling, 8);
#else
	config->GetString("uiDefaultFont", &defaultFontPath, "Sweet16");
	config->GetFloat("uiDefaultFontSize", &defaultFontSize, 16.0f);
	config->GetInt("uiDefaultFontOversampling", &defaultFontOversampling, 4);
#endif
	
	defaultFontSize = URANGE(4, viewC64->defaultFontSize, 64);
	defaultFontOversampling = URANGE(1, viewC64->defaultFontOversampling, 8);

	if (!strcmp(defaultFontPath, "ProFontIIx"))
	{
		imFontDefault = AddProFontIIx(defaultFontSize, defaultFontOversampling);
	}
	else if (!strcmp(defaultFontPath, "Sweet16"))
	{
		imFontDefault = AddSweet16MonoFont(defaultFontSize, defaultFontOversampling);
	}
	else if (!strcmp(defaultFontPath, "CousineRegular"))
	{
		imFontDefault = AddCousineRegularFont(defaultFontSize, defaultFontOversampling);
	}
	else if (!strcmp(defaultFontPath, "DroidSans"))
	{
		imFontDefault = AddDroidSansFont(defaultFontSize, defaultFontOversampling);
	}
	else if (!strcmp(defaultFontPath, "KarlaRegular"))
	{
		imFontDefault = AddKarlaRegularFont(defaultFontSize, defaultFontOversampling);
	}
	else if (!strcmp(defaultFontPath, "RobotoMedium"))
	{
		imFontDefault = AddRobotoMediumFont(defaultFontSize, defaultFontOversampling);
	}

	else
	{
		ImFontConfig fontConfig = ImFontConfig();
		fontConfig.SizePixels = defaultFontSize;
		fontConfig.OversampleH = defaultFontOversampling;
		fontConfig.OversampleV = defaultFontOversampling;
		imFontDefault = io.Fonts->AddFontDefault();
	}
//	else if (!strcmp(defaultFontPath, "Custom"))
//	{
//
//	}
	
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
	
	guiMain->CreateUiFontsTexture();
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
	viewVicEditor->viewVicDisplayMain->InitGridLinesColorFromSettings();
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
		debugInterfaceC64->SetAudioVolume((float)(c64SettingsAudioVolume) / 100.0f);
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

		// cursor is visible
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
	else
	{
		// not fullscreen, show cursor
		if (guiMain->IsMouseCursorVisible() == false)
		{
			mouseCursorVisibilityCounter = 0;
			guiMain->SetMouseCursorVisible(true);
		}
	}
	
}

void CViewC64::ShowMouseCursor()
{
	guiMain->SetMouseCursorVisible(true);
}

void CViewC64::GoFullScreen(CGuiView *view)
{
	if (view != NULL)
	{
		guiMain->SetViewFullScreen(view);
	}
	else
	{
		guiMain->SetApplicationWindowFullScreen(true);
	}
}
	
void CViewC64::ToggleFullScreen(CGuiView *view)
{
	if (guiMain->IsViewFullScreen())
	{
		guiMain->SetViewFullScreen(NULL);
		return;
	}
	
	if (guiMain->IsApplicationWindowFullScreen())
	{
		guiMain->SetApplicationWindowFullScreen(false);
		return;
	}
	
	if (view != NULL)
	{
		guiMain->SetViewFullScreen(view);
		return;
	}
	
	guiMain->SetApplicationWindowFullScreen(true);
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

void CViewC64::ApplicationShutdown()
{
	LOGD("CViewC64::ApplicationShutdown");
	
	_exit(0);
	
	guiMain->RemoveAllViews();
	
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->Shutdown();
	}
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->Shutdown();
	}
	if (viewC64->debugInterfaceNes)
	{
		viewC64->debugInterfaceNes->Shutdown();
	}
	SYS_Sleep(100);
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
		viewC64->viewC64MainMenu->LoadFile(slrPath);
		delete slrPath;
	}
}

void CViewC64::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	viewC64->viewC64MainMenu->LoadFile(filePath);
}

