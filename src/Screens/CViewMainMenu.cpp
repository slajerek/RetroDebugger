//
// NOTE:
// This view will be removed. It is being refactored and moved to main menu bar instead
//

#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewMainMenu.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64D_Version.h"
#include "CDebugSymbols.h"
#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CViewAbout.h"
#include "CViewMemoryMap.h"
#include "CViewFileD64.h"
#include "CViewKeyboardShortcuts.h"
#include "CSnapshotsManager.h"
#include "C64D_Version.h"

#include "CMainMenuBar.h"

#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"

#include "C64SettingsStorage.h"

#include "CGuiMain.h"
#include "CViewVicEditor.h"


#define VIEWC64SETTINGS_OPEN_NONE	0
#define VIEWC64SETTINGS_OPEN_FILE	1
#define VIEWC64SETTINGS_OPEN_D64	2
#define VIEWC64SETTINGS_OPEN_CRT	3
#define VIEWC64SETTINGS_OPEN_PRG	4
#define VIEWC64SETTINGS_OPEN_JUKEBOX	5
#define VIEWC64SETTINGS_OPEN_TAPE	6
#define VIEWC64SETTINGS_OPEN_REU	7
#define VIEWC64SETTINGS_SAVE_REU	8
#define VIEWC64SETTINGS_SET_ATARI_ROMS_FOLDER	9
#define VIEWC64SETTINGS_OPEN_ATR	10
#define VIEWC64SETTINGS_OPEN_ROM	11

CViewMainMenu::CViewMainMenu(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewMainMenu";

	this->ThreadSetName("CViewMainMenu");
	
	font = viewC64->fontCBMShifted;
	fontScale = 3;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	// TODO: make generic
	if (viewC64->debugInterfaceC64)
	{
		diskExtensions.push_back(new CSlrString("d64"));
		diskExtensions.push_back(new CSlrString("x64"));
		diskExtensions.push_back(new CSlrString("g64"));
		diskExtensions.push_back(new CSlrString("p64"));

		cDiskExtensions.push_back("d64"); cDiskExtensions.push_back("D64");
		cDiskExtensions.push_back("x64"); cDiskExtensions.push_back("X64");
		cDiskExtensions.push_back("g64"); cDiskExtensions.push_back("G64");
		cDiskExtensions.push_back("p64"); cDiskExtensions.push_back("P64");
		
		tapeExtensions.push_back(new CSlrString("tap"));
		tapeExtensions.push_back(new CSlrString("t64"));

		crtExtensions.push_back(new CSlrString("crt"));
		crtExtensions.push_back(new CSlrString("reu"));
		
		reuExtensions.push_back(new CSlrString("reu"));
		reuExtensions.push_back(new CSlrString("bin"));
		
		// Open dialog
		openFileExtensions.push_back(new CSlrString("prg"));
		openFileExtensions.push_back(new CSlrString("d64"));
		openFileExtensions.push_back(new CSlrString("x64"));
		openFileExtensions.push_back(new CSlrString("g64"));
		openFileExtensions.push_back(new CSlrString("p64"));
		openFileExtensions.push_back(new CSlrString("tap"));
		openFileExtensions.push_back(new CSlrString("t64"));
		openFileExtensions.push_back(new CSlrString("crt"));
		openFileExtensions.push_back(new CSlrString("sid"));
		openFileExtensions.push_back(new CSlrString("reu"));
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		diskExtensions.push_back(new CSlrString("atr"));

		crtExtensions.push_back(new CSlrString("car"));

		openFileExtensions.push_back(new CSlrString("xex"));
		openFileExtensions.push_back(new CSlrString("atr"));
		openFileExtensions.push_back(new CSlrString("a8s"));
		openFileExtensions.push_back(new CSlrString("cas"));
		openFileExtensions.push_back(new CSlrString("car"));
		openFileExtensions.push_back(new CSlrString("snap"));
	}

	if (viewC64->debugInterfaceNes)
	{
		openFileExtensions.push_back(new CSlrString("nes"));
	}

	openFileExtensions.push_back(new CSlrString("c64jukebox"));
	openFileExtensions.push_back(new CSlrString("json"));
	openFileExtensions.push_back(new CSlrString("snap"));
	openFileExtensions.push_back(new CSlrString("vsf"));

	// jukebox
	jukeboxExtensions.push_back(new CSlrString("c64jukebox"));
	jukeboxExtensions.push_back(new CSlrString("json"));
	
	// roms
	romsFileExtensions.push_back(new CSlrString("rom"));
	romsFileExtensions.push_back(new CSlrString("bin"));

	char *buf = SYS_GetCharBuf();
	
	// TODO: this must be changed to generic.
	if (viewC64->debugInterfaceC64)
	{
		sprintf(buf, "C64 Debugger v%s by Slajerek/Samar", RETRODEBUGGER_VERSION_STRING);
	}
	else if (viewC64->debugInterfaceAtari)
	{
		sprintf(buf, "65XE Debugger v%s by Slajerek/Samar", RETRODEBUGGER_VERSION_STRING);
	}
	else if (viewC64->debugInterfaceNes)
	{
		sprintf(buf, "NES Debugger v%s by Slajerek/Samar", RETRODEBUGGER_VERSION_STRING);
	}
	strHeader = new CSlrString(buf);

	SYS_ReleaseCharBuf(buf);

	/*
	if (viewC64->debugInterfaceC64)
	{
		strHeader2 = viewC64->debugInterfaceC64->GetEmulatorVersionString();
	}
	else if (viewC64->debugInterfaceAtari)
	{
		strHeader2 = viewC64->debugInterfaceAtari->GetEmulatorVersionString();
	}
	else if (viewC64->debugInterfaceNes)
	{
		strHeader2 = viewC64->debugInterfaceNes->GetEmulatorVersionString();
	}*/
	
	/// colors
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;
	
	/*
	/// menu
	viewMenu = new CGuiViewMenu(35, 76, -1, sizeX-70, sizeY-76, this);

	kbsMainMenuScreen = NULL; //new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Main menu screen", MTKEY_F9, false, false, false);
//	viewC64->keyboardShortcuts->AddShortcut(kbsMainMenuScreen);
	
	//
	


	//
	
#if defined(RUN_COMMODORE64)
	
	if (viewC64->debugInterfaceC64)
	{
//		kbsVicEditorScreen = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "VIC Editor screen", MTKEY_F6, true, false, true);
//		guiMain->AddKeyboardShortcut();
//	//
//
		kbsInsertD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Device #8", '8', false, false, true);
		guiMain->AddKeyboardShortcut();
//		menuItemInsertD64 = new CViewC64MenuItem(fontHeight*2.5, new CSlrString("1541 Device 8..."), kbsInsertD64, tr, tg, tb);
//		viewMenu->AddMenuItem(menuItemInsertD64);
		
		//
		kbsInsertNextD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert next disk to Device #8", '8', false, true, true);
		guiMain->AddKeyboardShortcut();

//		// TODO: add shortcut to second line of menuItemInsertD64 (browse)
//		kbsBrowseD64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Browse Device #8", MTKEY_F7, false, false, false);
//		viewC64->keyboardShortcuts->AddShortcut(kbsBrowseD64);
		
		kbsStartFromDisk = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Start from Device #8", MTKEY_F3, false, false, false);
		guiMain->AddKeyboardShortcut();
	}
	

#endif

#if defined(RUN_ATARI)
	if (viewC64->debugInterfaceAtari)
	{
		kbsInsertATR = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Disk", '8', false, false, true);
		guiMain->AddKeyboardShortcut();
	}
#endif
	
	//
	
//	kbsOpenFile = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Load", 'o', false, false, true);
//	viewC64->keyboardShortcuts->AddShortcut(kbsOpenFile);
//	menuItemOpenFile = new CViewC64MenuItem(fontHeight*2.5, new CSlrString("Load..."), kbsOpenFile, tr, tg, tb);
//	viewMenu->AddMenuItem(menuItemOpenFile);
		
	if (viewC64->debugInterfaceC64)
	{
		kbsReloadAndRestart = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Reload & Start PRG", 'l', false, false, true);
		guiMain->AddKeyboardShortcut();
//		menuItemReloadAndRestart = new CViewC64MenuItem(fontHeight, new CSlrString("Reload PRG & Start"), kbsReloadAndRestart, tr, tg, tb);
//		viewMenu->AddMenuItem(menuItemReloadAndRestart);
		
		kbsRestartPRG = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Reload & Restart PRG", MTKEY_F5, false, false, false);
		guiMain->AddKeyboardShortcut();
	}
	
	kbsSoftReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Soft Reset", 'r', false, false, true);
	guiMain->AddKeyboardShortcut();
//	menuItemSoftReset = new CViewC64MenuItem(fontHeight, new CSlrString("Soft Reset"), kbsSoftReset, tr, tg, tb);
//	viewMenu->AddMenuItem(menuItemSoftReset);

	kbsHardReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Hard Reset", 'r', true, false, true);
	guiMain->AddKeyboardShortcut();
	menuItemHardReset = new CViewC64MenuItem(fontHeight*2, new CSlrString("Hard Reset"), kbsHardReset, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemHardReset);

	if (viewC64->debugInterfaceC64)
	{
		kbsDiskDriveReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Disk Drive Reset", 'r', false, true, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsDiskDriveReset);
	//	menuItemDiskDriveReset = new CViewC64MenuItem(fontHeight, new CSlrString("Disk Drive Reset"), kbsDiskDriveReset, tr, tg, tb);
	//	viewMenu->AddMenuItem(menuItemDiskDriveReset);

		kbsInsertCartridge = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Cartridge", '0', false, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsInsertCartridge);
		menuItemInsertCartridge = new CViewC64MenuItem(fontHeight*3, new CSlrString("Insert cartridge..."), kbsInsertCartridge, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemInsertCartridge);
		
		kbsSnapshotsC64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Snapshots screen", 's', true, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsSnapshotsC64);
		menuItemSnapshotsC64 = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Snapshots..."), kbsSnapshotsC64, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemSnapshotsC64);
		
		kbsBreakpointsC64 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Breakpoints screen", 'b', false, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsBreakpointsC64);
		menuItemBreakpointsC64 = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Breakpoints..."), kbsBreakpointsC64, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemBreakpointsC64);
	
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		kbsInsertAtariCartridge = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Insert Cartridge", '0', false, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsInsertAtariCartridge);
		menuItemInsertAtariCartridge = new CViewC64MenuItem(fontHeight*3, new CSlrString("Insert cartridge..."), kbsInsertAtariCartridge, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemInsertAtariCartridge);
		
		kbsSnapshotsAtari = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Snapshots screen", 's', true, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsSnapshotsAtari);
		menuItemSnapshotsAtari = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Snapshots..."), kbsSnapshotsAtari, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemSnapshotsAtari);
		
		kbsBreakpointsAtari = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Breakpoints screen", 'b', false, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsBreakpointsAtari);
		menuItemBreakpointsAtari = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Breakpoints..."), kbsBreakpointsAtari, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemBreakpointsAtari);
	}
	
	if (viewC64->debugInterfaceNes)
	{
		menuItemSetFolderWithNesROMs = new CViewC64MenuItem(fontHeight*2, new CSlrString("Set ROMs folder"), NULL, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemSetFolderWithNesROMs);
		
		kbsSnapshotsNes = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Snapshots screen", 's', true, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsSnapshotsNes);
		menuItemSnapshotsNes = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Snapshots..."), kbsSnapshotsNes, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemSnapshotsNes);
		
		kbsBreakpointsNes = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Breakpoints screen", 'b', false, false, true);
		viewC64->keyboardShortcuts->AddShortcut(kbsBreakpointsNes);
		menuItemBreakpointsNes = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Breakpoints..."), kbsBreakpointsNes, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemBreakpointsNes);
	}
	

//	kbsEmulationSettings = new CSlrKeyboardShortcut(KBZONE_GLOBAL, KBFUN_BREAKPOINTS, 'b', false, false, true);
//	viewC64->keyboardShortcuts->AddShortcut(kbsEmulationSettings);
	menuItemSettings = new CViewC64MenuItem(fontHeight*1.7, new CSlrString("Settings..."), NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemSettings);

	menuItemAbout = new CViewC64MenuItem(fontHeight, new CSlrString("About..."), NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemAbout);

//	// TODO: N/A kbsMoveFocusToNextView
//	kbsMoveFocusToNextView = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Move focus to next view", MTKEY_TAB, false, false, false);
//	viewC64->keyboardShortcuts->AddShortcut(kbsMoveFocusToNextView);
//
//	kbsMoveFocusToPreviousView = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Move focus to previous view", MTKEY_TAB, true, false, false);
//	viewC64->keyboardShortcuts->AddShortcut(kbsMoveFocusToPreviousView);
	
	viewMenu->InitSelection();
	
	
//	std::list<u32> zones;
//	zones.push_back(KBZONE_GLOBAL);
//	CSlrKeyboardShortcut *sr = viewC64->keyboardShortcuts->FindShortcut(zones, '8', false, true, false);
	
	//LOGD("---done");
	
	//
	
	viewC64->colorsTheme->AddThemeChangeListener(this);
	*/
}

CViewMainMenu::~CViewMainMenu()
{
}

bool CViewMainMenu::ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut)
{
	LOGTODO("CViewMainMenu::ProcessKeyboardShortcut");
	return false;
}

void CViewMainMenu::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{
	/*
	if (menuItem == menuItemInsertD64)
	{
		OpenDialogInsertD64();
	}
	else if (menuItem == menuItemInsertCartridge)
	{
		OpenDialogInsertCartridge();
	}
	if (menuItem == menuItemInsertATR)
	{
		OpenDialogInsertATR();
	}
	else if (menuItem == menuItemInsertAtariCartridge)
	{
		OpenDialogInsertAtariCartridge();
	}
	else if (menuItem == menuItemOpenFile)
	{
		OpenDialogOpenFile();
	}
	else if (menuItem == menuItemSetFolderWithAtariROMs)
	{
		OpenDialogSetFolderWithAtariROMs();
	}
	else if (menuItem == menuItemHardReset)
	{
		if (viewC64->debugInterfaceC64)
		{
			viewC64->debugInterfaceC64->HardReset();
		}
		if (viewC64->debugInterfaceAtari)
		{
			viewC64->debugInterfaceAtari->HardReset();
		}
	}
	else if (menuItem == menuItemSoftReset)
	{
		if (viewC64->debugInterfaceC64)
		{
			viewC64->debugInterfaceC64->Reset();
		}
		if (viewC64->debugInterfaceAtari)
		{
			viewC64->debugInterfaceAtari->Reset();
		}
	}
	else if (menuItem == menuItemBreakpointsC64)
	{
		viewC64->viewC64Breakpoints->SwitchBreakpointsScreen();
	}
	else if (menuItem == menuItemBreakpointsAtari)
	{
		viewC64->viewAtariBreakpoints->SwitchBreakpointsScreen();
	}
	else if (menuItem == menuItemBreakpointsNes)
	{
		viewC64->viewNesBreakpoints->SwitchBreakpointsScreen();
	}
	else if (menuItem == menuItemSnapshotsC64)
	{
		if (viewC64->debugInterfaceC64)
		{
			viewC64->viewC64Snapshots->SwitchSnapshotsScreen();
		}
	}
	else if (menuItem == menuItemSnapshotsAtari)
	{
		if (viewC64->debugInterfaceAtari)
		{
			viewC64->viewAtariSnapshots->SwitchSnapshotsScreen();
		}
	}
	else if (menuItem == menuItemSnapshotsNes)
	{
		if (viewC64->debugInterfaceNes)
		{
			viewC64->viewNesSnapshots->SwitchSnapshotsScreen();
		}
	}
	else if (menuItem == menuItemReloadAndRestart)
	{
		ReloadAndRestartPRG();
	}
	else if (menuItem == menuItemSettings)
	{
		viewC64->viewC64SettingsMenu->SwitchMainMenuScreen();
	}
	else if (menuItem == menuItemAbout)
	{
		viewC64->viewAbout->SwitchAboutScreen();
	}
	*/
}

void CViewMainMenu::MenuCallbackItemChanged(CGuiViewMenuItem *menuItem)
{
	LOGD("CViewMainMenu::MenuCallbackItemChanged");
}

void CViewMainMenu::OpenDialogInsertD64()
{
	LOGM("OpenDialogInsertD64");
	openDialogFunction = VIEWC64SETTINGS_OPEN_D64;

	CSlrString *windowTitle = new CSlrString("Open D64 disk image");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &diskExtensions, c64SettingsDefaultD64Folder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogInsertATR()
{
	LOGM("OpenDialogInsertATR");
	openDialogFunction = VIEWC64SETTINGS_OPEN_ATR;
	
	CSlrString *windowTitle = new CSlrString("Open ATR disk image");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &diskExtensions, c64SettingsDefaultATRFolder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogInsertCartridge()
{
	LOGM("OpenDialogInsertCartridge");
	openDialogFunction = VIEWC64SETTINGS_OPEN_CRT;
	
	CSlrString *windowTitle = new CSlrString("Open CRT cartridge image");
	viewC64->ShowDialogOpenFile(this, &crtExtensions, c64SettingsDefaultCartridgeFolder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogInsertAtariCartridge()
{
	LOGM("OpenDialogInsertCartridge");
	openDialogFunction = VIEWC64SETTINGS_OPEN_ROM;
	
	CSlrString *windowTitle = new CSlrString("Open CAR cartridge image");
	viewC64->ShowDialogOpenFile(this, &crtExtensions, c64SettingsDefaultCartridgeFolder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogInsertTape()
{
	LOGM("OpenDialogInsertTape");
	openDialogFunction = VIEWC64SETTINGS_OPEN_TAPE;
	
	CSlrString *windowTitle = new CSlrString("Open tape image");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &tapeExtensions, c64SettingsDefaultTAPFolder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogAttachReu()
{
	LOGM("OpenDialogAttachReu");
	openDialogFunction = VIEWC64SETTINGS_OPEN_REU;
	
	CSlrString *windowTitle = new CSlrString("Open REU image");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &reuExtensions, c64SettingsDefaultReuFolder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogSaveReu()
{
	LOGM("OpenDialogSaveReu");
	openDialogFunction = VIEWC64SETTINGS_OPEN_REU;
	
	CSlrString *windowTitle = new CSlrString("Save REU image");
	windowTitle->DebugPrint("windowTitle=");
	CSlrString *defaultFileName = new CSlrString("image");
	viewC64->ShowDialogSaveFile(this, &reuExtensions, defaultFileName, c64SettingsDefaultReuFolder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogOpenFile()
{
	LOGM("OpenDialogOpenFile");
	openDialogFunction = VIEWC64SETTINGS_OPEN_FILE;
	
	CSlrString *windowTitle = new CSlrString("Open file");
	viewC64->ShowDialogOpenFile(this, &openFileExtensions, c64SettingsDefaultPRGFolder, windowTitle);
	delete windowTitle;	
}

void CViewMainMenu::OpenDialogSetFolderWithAtariROMs()
{
	LOGM("OpenDialogSetFolderWithAtariROMs");
	openDialogFunction = VIEWC64SETTINGS_SET_ATARI_ROMS_FOLDER;
	
	CSlrString *windowTitle = new CSlrString("Set Atari ROMs folder");
	viewC64->ShowDialogOpenFile(this, &romsFileExtensions, c64SettingsPathToAtariROMs, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::OpenDialogStartJukeboxPlaylist()
{
	LOGM("OpenDialogStartJukeboxPlaylist");
	openDialogFunction = VIEWC64SETTINGS_OPEN_JUKEBOX;
	
	CSlrString *windowTitle = new CSlrString("Start JukeBox playlist");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &jukeboxExtensions, c64SettingsDefaultD64Folder, windowTitle);
	delete windowTitle;
}

void CViewMainMenu::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGM("CViewMainMenu::SystemDialogFileOpenSelected, path=%x", path);
	path->DebugPrint("path=");

	if (openDialogFunction == VIEWC64SETTINGS_OPEN_D64)
	{
		InsertD64(path, true, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_OPEN_TAPE)
	{
		LoadTape(path, false, true, true);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_OPEN_CRT)
	{
		InsertCartridge(path, true);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_OPEN_PRG)
	{
		LoadPRG(path, true, true, true, false);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_OPEN_REU)
	{
		AttachReu(path, true, true);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_OPEN_JUKEBOX)
	{
		viewC64->InitJukebox(path);
		//C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_SET_ATARI_ROMS_FOLDER)
	{
		if (c64SettingsPathToAtariROMs != NULL)
		{
			delete c64SettingsPathToAtariROMs;
		}
		
		CSlrString *romPath = path->GetFilePathWithoutFileNameComponentFromPath();
		romPath->DebugPrint("set Atari romPath=");
		romPath->DebugPrintVector("romPath=");
		c64SettingsPathToAtariROMs = romPath;
		C64DebuggerStoreSettings();
		
		guiMain->ShowMessage("Please restart 65XE Debugger to confirm ROMs path");
		
		
		/*
		 TODO: someday soon maybe
		guiMain->LockMutex();
		
		viewC64->debugInterfaceAtari->RestartEmulation();
		
		guiMain->UnlockMutex();*/
	}
	if (openDialogFunction == VIEWC64SETTINGS_OPEN_ATR)
	{
		InsertATR(path, true, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
		C64DebuggerStoreSettings();
	}
	else
	{
		///
		
		LoadFile(path);
		C64DebuggerStoreSettings();
	}
	
	openDialogFunction = VIEWC64SETTINGS_OPEN_NONE;
}

void CViewMainMenu::SystemDialogFileSaveSelected(CSlrString *path)
{
	LOGM("CViewMainMenu::SystemDialogFileSaveSelected, path=%x", path);
	path->DebugPrint("path=");
	
	if (openDialogFunction == VIEWC64SETTINGS_SAVE_REU)
	{
		SaveReu(path, true, true);
		C64DebuggerStoreSettings();
	}

	openDialogFunction = VIEWC64SETTINGS_OPEN_NONE;
}

void CViewMainMenu::SystemDialogFileSaveCancelled()
{
}


void CViewMainMenu::LoadFile(CSlrString *path)
{
	viewC64->recentlyOpenedFiles->Add(path);
	
	CSlrString *ext = path->GetFileExtensionComponentFromPath();
	ext->ConvertToLowerCase();
	
	if (ext->CompareWith("prg"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		LoadPRG(path, true, true, true, false);
	}
	else if (ext->CompareWith("d64") ||
			 ext->CompareWith("x64") ||
			 ext->CompareWith("g64") ||
			 ext->CompareWith("p64"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		InsertD64(path, true, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
	}
	else if (ext->CompareWith("crt"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		InsertCartridge(path, true);
	}
	else if (ext->CompareWith("reu"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		bool val = true;
		C64DebuggerSetSetting("ReuEnabled", &val);
		AttachReu(path, true, true);
	}
	else if (ext->CompareWith("sid"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		LoadSID(path);
	}
	else if (ext->CompareWith("snap") ||
			 ext->CompareWith("vsf"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		viewC64->viewC64Snapshots->LoadSnapshot(path, true, viewC64->debugInterfaceC64);
	}
	else if (ext->CompareWith("tap") ||
			 ext->CompareWith("t64"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceC64);
		LoadTape(path, true, true, true);
	}
	else if (ext->CompareWith("xex"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceAtari);
		LoadXEX(path, true, true, true);
	}
	else if (ext->CompareWith("atr"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceAtari);
		InsertATR(path, true, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
	}
	else if (ext->CompareWith("a8s"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceAtari);
		viewC64->viewAtariSnapshots->LoadSnapshot(path, true, viewC64->debugInterfaceAtari);
	}
	else if (ext->CompareWith("nes"))
	{
		viewC64->StartEmulationThread(viewC64->debugInterfaceNes);
		LoadNES(path, true);
	}
	else if (ext->CompareWith("c64jukebox") ||
			 ext->CompareWith("json"))

	{
		viewC64->InitJukebox(path);
	}
	
	viewC64->ShowMainScreen();

	delete ext;
}

void CViewMainMenu::InsertD64(CSlrString *path, bool updatePathToD64, bool autoRun, int autoRunEntryNum, bool showLoadAddressInfo)
{
	LOGD("CViewMainMenu::InsertD64: path=%x autoRun=%s autoRunEntryNum=%d", path, STRBOOL(autoRun), autoRunEntryNum);
	path->DebugPrint("CViewMainMenu::InsertD64: path=");
	
	if (SYS_FileExists(path) == false)
	{
		if (c64SettingsPathToD64 != NULL)
			delete c64SettingsPathToD64;
		
		c64SettingsPathToD64 = NULL;
		LOGError("InsertD64: file not found, skipping");
		
		if (showLoadAddressInfo)
		{
			guiMain->ShowMessageBox("Error", "Can not open file");
		}
		return;
	}
	
	if (c64SettingsPathToD64 != path)
	{
		if (c64SettingsPathToD64 != NULL)
			delete c64SettingsPathToD64;
		c64SettingsPathToD64 = new CSlrString(path);
	}
	
	if (updatePathToD64)
	{
		LOGD("...updatePathToD64");
		if (c64SettingsDefaultD64Folder != NULL)
			delete c64SettingsDefaultD64Folder;
		c64SettingsDefaultD64Folder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultD64Folder->DebugPrint("c64SettingsDefaultD64Folder=");
	}
	
	// insert D64
	viewC64->debugInterfaceC64->InsertD64(path);

	
	// TODO: support UTF paths
	char *asciiPath = path->GetStdASCII();
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);

	guiMain->LockMutex();

//	if (menuItemInsertD64->str2 != NULL)
//		delete menuItemInsertD64->str2;
//	menuItemInsertD64->str2 = new CSlrString(fname);
//	delete [] fname;
	
	guiMain->UnlockMutex();

	LOGM("Inserted new d64: %s", asciiPath);
	delete [] asciiPath;
	
	if (guiMain->currentView == viewC64->viewFileD64)
		viewC64->viewFileD64->StartSelectedDiskImageBrowsing();
	
	if (autoRun)
	{
		viewC64->viewFileD64->StartDiskPRGEntry(autoRunEntryNum, showLoadAddressInfo);
	}
}

// insert next D64 from the same folder
void CViewMainMenu::InsertNextD64()
{
	LOGM("CViewMainMenu::InsertNextD64");
	if (SYS_FileExists(c64SettingsPathToD64) == false)
	{
		if (c64SettingsPathToD64 != NULL)
			delete c64SettingsPathToD64;
		
		c64SettingsPathToD64 = NULL;
		LOGError("InsertNextD64: file not found, skipping");
		return;
	}

	char *asciiPath = c64SettingsPathToD64->GetStdASCII();
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
	char *fpath = SYS_GetPathFromFullPath(asciiPath);
	LOGD("..fname=%s fpath=%s", fname, fpath);
	
	std::vector<CFileItem *> *filesInFolder = SYS_GetFilesInFolder(fpath, &cDiskExtensions);

	// find current file, and then insert next
	for (std::vector<CFileItem *>::iterator it = filesInFolder->begin(); it != filesInFolder->end(); it++)
	{
		CFileItem *file = *it;
		
		LOGD("...%s", file->name);
		if (!strcmp(file->name, fname))
		{
			LOGD("FOUND!");
			
			it++;
			if (it == filesInFolder->end())
			{
				it = filesInFolder->begin();
			}
			
			CFileItem *nextFile = *it;
			CSlrString *nextFilePath = new CSlrString();
			nextFilePath->Concatenate(fpath);
			nextFilePath->Concatenate(nextFile->name);

			nextFilePath->DebugPrint("nextFilePath=");
			
			InsertD64(nextFilePath, false, false, -1, false);
			
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "Inserted %s", nextFile->name);
			guiMain->ShowMessage(buf);
			
			delete nextFilePath;
			break;
		}
	}
	
	while(!filesInFolder->empty())
	{
		CFileItem *f = filesInFolder->back();
		filesInFolder->pop_back();
		delete f;
	}
	delete filesInFolder;
	
	LOGD("CViewMainMenu::InsertNextD64: done");
}

bool CViewMainMenu::AttachReu(CSlrString *path, bool updatePathToReu, bool showDetails)
{
	LOGD("CViewMainMenu::AttachReu: path=%x");
	
	if (SYS_FileExists(path) == false)
	{
		if (c64SettingsPathToReu != NULL)
			delete c64SettingsPathToReu;
		
		c64SettingsPathToReu = NULL;
		LOGError("AttachReu: file not found, skipping");
		return false;
	}
	
	if (c64SettingsPathToReu != path)
	{
		if (c64SettingsPathToReu != NULL)
			delete c64SettingsPathToReu;
		c64SettingsPathToReu = new CSlrString(path);
	}
	
	if (updatePathToReu)
	{
		LOGD("...updatePathToReu");
		if (c64SettingsDefaultReuFolder != NULL)
			delete c64SettingsDefaultReuFolder;
		c64SettingsDefaultReuFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultReuFolder->DebugPrint("c64SettingsDefaultReuFolder=");
	}
	
	// insert REU
	char *asciiPath = c64SettingsPathToReu->GetStdASCII();
	
	bool ret = viewC64->debugInterfaceC64->LoadReu(asciiPath);
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
	
	guiMain->LockMutex();
	
	if (viewC64->viewC64SettingsMenu->menuItemReuAttach->str2 != NULL)
		delete viewC64->viewC64SettingsMenu->menuItemReuAttach->str2;
	
	viewC64->viewC64SettingsMenu->menuItemReuAttach->str2 = new CSlrString(fname);
	
	guiMain->UnlockMutex();
	
	LOGM("%s REU: %s", (ret ? "Attached" : "Failed to attach"), asciiPath);
	delete [] asciiPath;
	
	if (showDetails)
	{
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "%s REU: %s", (ret ? "Attached" : "Failed to attach"), fname);
		guiMain->ShowMessage(buf);
		SYS_ReleaseCharBuf(buf);
	}

	delete [] fname;
	
	return ret;
}

bool CViewMainMenu::SaveReu(CSlrString *path, bool updatePathToReu, bool showDetails)
{
	LOGD("CViewMainMenu::SaveReu: path=%x");
	
	if (updatePathToReu)
	{
		LOGD("...updatePathToReu");
		if (c64SettingsPathToReu != path)
		{
			if (c64SettingsPathToReu != NULL)
				delete c64SettingsPathToReu;
			c64SettingsPathToReu = new CSlrString(path);
		}
		
		if (c64SettingsDefaultReuFolder != NULL)
			delete c64SettingsDefaultReuFolder;
		c64SettingsDefaultReuFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultReuFolder->DebugPrint("c64SettingsDefaultReuFolder=");
	}
	
	// save REU
	char *asciiPath = c64SettingsPathToReu->GetStdASCII();
	
	bool ret = viewC64->debugInterfaceC64->SaveReu(asciiPath);
	
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
	
	guiMain->LockMutex();
	if (viewC64->viewC64SettingsMenu->menuItemReuAttach->str2 != NULL)
		delete viewC64->viewC64SettingsMenu->menuItemReuAttach->str2;

	viewC64->viewC64SettingsMenu->menuItemReuAttach->str2 = new CSlrString(fname);
	
	guiMain->UnlockMutex();
	
	LOGM("%s REU: %s", (ret ? "Saved" : "Failed to save"), asciiPath);
	delete [] asciiPath;
	
	if (showDetails)
	{
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "%s REU: %s", (ret ? "Saved" : "Failed to save"), fname);
		guiMain->ShowMessage(buf);
		SYS_ReleaseCharBuf(buf);
	}
	
	delete [] fname;
	
	return true;
}

bool CViewMainMenu::InsertCartridge(CSlrString *path, bool updatePathToCRT)
{
	path->DebugPrint("CViewMainMenu::InsertCartridge, path=");
	
	if (SYS_FileExists(path) == false)
	{
		if (c64SettingsPathToCartridge != NULL)
			delete c64SettingsPathToCartridge;
		
		c64SettingsPathToCartridge = NULL;
		LOGError("CViewMainMenu::InsertCartridge: file not found, skipping");
		return false;
	}

	if (c64SettingsPathToCartridge != path)
	{
		if (c64SettingsPathToCartridge != NULL)
			delete c64SettingsPathToCartridge;
		c64SettingsPathToCartridge = new CSlrString(path);
	}
	
	if (updatePathToCRT)
	{
		LOGD("...updatePathToCRT");
		if (c64SettingsDefaultCartridgeFolder != NULL)
			delete c64SettingsDefaultCartridgeFolder;
		c64SettingsDefaultCartridgeFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultCartridgeFolder->DebugPrint("strDefaultCartridgeFolder=");
	}
	
	// insert CRT
	viewC64->debugInterfaceC64->AttachCartridge(path);
	
	LoadLabelsAndWatches(path, viewC64->debugInterfaceC64);

	// TODO: support UTF paths
	char *asciiPath = c64SettingsPathToCartridge->GetStdASCII();
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);

	guiMain->LockMutex();
	
//	if (menuItemInsertCartridge->str2 != NULL)
//		delete menuItemInsertCartridge->str2;
//	menuItemInsertCartridge->str2 = new CSlrString(fname);
//	delete [] fname;

	guiMain->UnlockMutex();
	
	LOGM("Attached new cartridge: %s", asciiPath);
	delete [] asciiPath;
	
	return true;
}

bool CViewMainMenu::LoadPRG(CSlrString *path, bool autoStart, bool updatePRGFolderPath, bool showAddressInfo, bool forceFastReset)
{
	path->DebugPrint("CViewMainMenu::LoadPRG: path=");
	
	LOGD("   >>> LoadPRG, autostart=%d forceFastReset=%d", autoStart, forceFastReset);
	
	// TODO: p00 http://vice-emu.sourceforge.net/vice_15.html#SEC299
	
	if (c64SettingsPathToPRG != path)
	{
		if (c64SettingsPathToPRG != NULL)
			delete c64SettingsPathToPRG;
		c64SettingsPathToPRG = new CSlrString(path);
	}
	
	if (updatePRGFolderPath)
	{
		LOGD("...updatePRGFolderPath");
		if (c64SettingsDefaultPRGFolder != NULL)
			delete c64SettingsDefaultPRGFolder;
		c64SettingsDefaultPRGFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultPRGFolder->DebugPrint("c64SettingsDefaultPRGFolder=");
	}
	
	LOGD("... LoadPRG (2)");
	
	c64SettingsPathToPRG->DebugPrint("c64SettingsPathToPRG=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = c64SettingsPathToPRG->GetStdASCII();

	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading PRG file");
		return false;
	}
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
	
	guiMain->LockMutex();
	
//	if (menuItemOpenFile->str2 != NULL)
//		delete menuItemOpenFile->str2;
//	menuItemOpenFile->str2 = new CSlrString(fname);
//	delete [] fname;

	viewC64->debugInterfaceC64->LockMutex();
	LoadLabelsAndWatches(path, viewC64->debugInterfaceC64);
	viewC64->debugInterfaceC64->UnlockMutex();

	guiMain->UnlockMutex();

	//
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	LoadPRG(byteBuffer, autoStart, showAddressInfo, forceFastReset);
	
	delete [] asciiPath;
	delete file;

	delete byteBuffer;
	
	return true;
}

bool CViewMainMenu::LoadSID(CSlrString *filePath)
{
	if (viewC64->debugInterfaceC64 == NULL)
		return false;
	
	filePath->DebugPrint("CViewMainMenu::LoadSID: path=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = filePath->GetStdASCII();
	
	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading PRG file");
		return false;
	}
	
	if (c64SettingsC64SidImportMode == SID_IMPORT_MODE_PSID64)
	{
		CByteBuffer *byteBufferSID = new CByteBuffer(file, false);
		CByteBuffer *byteBufferPRG = ConvertSIDtoPRG(byteBufferSID);
		
		LoadPRG(byteBufferPRG, true, false, true);
		
		delete byteBufferPRG;
		delete byteBufferSID;
	}
	else if (c64SettingsC64SidImportMode == SID_IMPORT_MODE_DIRECT_COPY)
	{
		u16 fromAddr;
		u16 toAddr;
		u16 initAddr;
		u16 playAddr;
		C64LoadSIDToRam(asciiPath, &fromAddr, &toAddr, &initAddr, &playAddr);
		
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "SID from $%04X to $%04X. Init $%04X Play $%04X.", fromAddr, toAddr, initAddr, playAddr);
		guiMain->ShowMessage(buf);
		SYS_ReleaseCharBuf(buf);
	}
	
	delete asciiPath;
	delete file;
	
	
	return true;

}


extern "C" {
	int tape_image_attach(unsigned int unit, const char *name);
	int tape_image_detach(unsigned int unit);
}


bool CViewMainMenu::LoadTape(CSlrString *path, bool autoStart, bool updateTAPFolderPath, bool showAddressInfo)
{
	path->DebugPrint("CViewMainMenu::LoadTape: path=");
	
	if (SYS_FileExists(path) == false)
	{
		if (c64SettingsPathToTAP != NULL)
			delete c64SettingsPathToTAP;
		
		c64SettingsPathToTAP = NULL;
		LOGError("CViewMainMenu::LoadTape: file not found, skipping");
		return false;
	}

	LOGD("   >>> LoadTape, autostart=%d", autoStart);
	
	if (c64SettingsPathToTAP != path)
	{
		if (c64SettingsPathToTAP != NULL)
			delete c64SettingsPathToTAP;
		c64SettingsPathToTAP = new CSlrString(path);
	}
	
	if (updateTAPFolderPath)
	{
		LOGD("...updateTAPFolderPath");
		if (c64SettingsDefaultTAPFolder != NULL)
			delete c64SettingsDefaultTAPFolder;
		c64SettingsDefaultTAPFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultTAPFolder->DebugPrint("c64SettingsDefaultTAPFolder=");
	}
	
	LOGD("... LoadTAP (2)");
	
	c64SettingsPathToTAP->DebugPrint("c64SettingsPathToTAP=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = c64SettingsPathToTAP->GetStdASCII();
	
	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading TAP file");
		return false;
	}
	
//	// display file name in menu
//	char *fname = SYS_GetFileNameFromFullPath(asciiPath);
	
	/*
	 guiMain->LockMutex();
	 if (menuItemLoadPRG->str2 != NULL)
		delete menuItemLoadPRG->str2;
	 
	 menuItemLoadPRG->str2 = new CSlrString(fname);
	 delete [] fname;
	 
	 LoadLabelsAndWatches(path);
	 guiMain->UnlockMutex();
	 
	 */
	
	//
	viewC64->debugInterfaceC64->AttachTape(c64SettingsPathToTAP);
	
	delete [] asciiPath;
	delete file;
	
	char *buf = SYS_GetCharBuf();
	CSlrString *fname = c64SettingsPathToTAP->GetFileNameComponentFromPath();
	char *cName = fname->GetStdASCII();
	sprintf(buf, "Tape %s attached", cName);
	guiMain->ShowMessage(buf);
	delete [] cName;
	SYS_ReleaseCharBuf(buf);
	
	return true;
}

bool CViewMainMenu::LoadXEX(CSlrString *path, bool autoStart, bool updateXEXFolderPath, bool showAddressInfo)
{
	path->DebugPrint("CViewMainMenu::LoadXEX: path=");
	
	LOGD("   >>> LoadXEX, autostart=%d", autoStart);
	
	if (c64SettingsPathToXEX != path)
	{
		if (c64SettingsPathToXEX != NULL)
			delete c64SettingsPathToXEX;
		c64SettingsPathToXEX = new CSlrString(path);
	}
	
	if (updateXEXFolderPath)
	{
		LOGD("...updateXEXFolderPath");
		if (c64SettingsDefaultXEXFolder != NULL)
			delete c64SettingsDefaultXEXFolder;
		c64SettingsDefaultXEXFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultXEXFolder->DebugPrint("c64SettingsDefaultXEXFolder=");
	}
	
	LOGD("... LoadXEX (2)");
	
	c64SettingsPathToXEX->DebugPrint("c64SettingsPathToXEX=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = c64SettingsPathToXEX->GetStdASCII();
	
	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading XEX file");
		if (c64SettingsPathToXEX != NULL)
			delete c64SettingsPathToXEX;
		c64SettingsPathToXEX = NULL;
		C64DebuggerStoreSettings();
		return false;
	}
	
	// display file name in menu
//	char *fname = SYS_GetFileNameFromFullPath(asciiPath);
	
	LOGTODO("XEX: update menu item");

	guiMain->LockMutex();

	/*
	 if (menuItemLoadPRG->str2 != NULL)
		delete menuItemLoadPRG->str2;
	
	menuItemLoadPRG->str2 = new CSlrString(fname);
	delete [] fname;
	 */
	
	LoadLabelsAndWatches(path, viewC64->debugInterfaceAtari);
	guiMain->UnlockMutex();
	
	
	//
	debugInterfaceAtari->LockMutex();
	debugInterfaceAtari->LoadExecutable(asciiPath);
	
	if (c64SettingsResetCountersOnAutoRun)
	{
		debugInterfaceAtari->ResetMainCpuDebugCycleCounter();
		debugInterfaceAtari->ResetEmulationFrameCounter();
	}

	debugInterfaceAtari->snapshotsManager->ClearSnapshotsHistory();

	debugInterfaceAtari->UnlockMutex();
	
	delete asciiPath;
	delete file;
	
	return true;
}

bool CViewMainMenu::LoadCAS(CSlrString *path, bool autoStart, bool updateCASFolderPath, bool showAddressInfo)
{
	path->DebugPrint("CViewMainMenu::LoadCAS: path=");
	
	LOGD("   >>> LoadCAS, autostart=%d", autoStart);
	
	if (c64SettingsPathToCAS != path)
	{
		if (c64SettingsPathToCAS != NULL)
			delete c64SettingsPathToCAS;
		c64SettingsPathToCAS = new CSlrString(path);
	}
	
	if (updateCASFolderPath)
	{
		LOGD("...updateCASFolderPath");
		if (c64SettingsDefaultCASFolder != NULL)
			delete c64SettingsDefaultCASFolder;
		c64SettingsDefaultCASFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultCASFolder->DebugPrint("c64SettingsDefaultCASFolder=");
	}
	
	LOGD("... LoadCAS (2)");
	
	c64SettingsPathToCAS->DebugPrint("c64SettingsPathToCAS=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = c64SettingsPathToCAS->GetStdASCII();
	
	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading CAS file");
		return false;
	}
	
	// display file name in menu
//	char *fname = SYS_GetFileNameFromFullPath(asciiPath);
	LOGTODO("CAS: load labels/symbols/watches & update menu item");
	
	/*
	 guiMain->LockMutex();
	 if (menuItemLoadPRG->str2 != NULL)
		delete menuItemLoadPRG->str2;
	 
	 menuItemLoadPRG->str2 = new CSlrString(fname);
	 delete [] fname;
	 
	 LoadLabelsAndWatches(path);
	 guiMain->UnlockMutex();
	 
	 */
	
	//
	debugInterfaceAtari->LockMutex();
	viewC64->debugInterfaceAtari->AttachTape(asciiPath, false);
	debugInterfaceAtari->UnlockMutex();
	
	delete asciiPath;
	delete file;
	
	return true;
}

bool CViewMainMenu::InsertAtariCartridge(CSlrString *path, bool autoStart, bool updateAtariCartridgeFolderPath, bool showAddressInfo)
{
	path->DebugPrint("CViewMainMenu::LoadAtariCartridge: path=");
	
	LOGD("   >>> LoadAtariCartridge, autostart=%d", autoStart);
	
	if (c64SettingsPathToAtariCartridge != path)
	{
		if (c64SettingsPathToAtariCartridge != NULL)
			delete c64SettingsPathToAtariCartridge;
		c64SettingsPathToAtariCartridge = new CSlrString(path);
	}
	
	if (updateAtariCartridgeFolderPath)
	{
		LOGD("...updateAtariCartridgeFolderPath");
		if (c64SettingsDefaultAtariCartridgeFolder != NULL)
			delete c64SettingsDefaultAtariCartridgeFolder;
		c64SettingsDefaultAtariCartridgeFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultAtariCartridgeFolder->DebugPrint("c64SettingsDefaultAtariCartridgeFolder=");
	}
	
	LOGD("... LoadAtariCartridge (2)");
	
	c64SettingsPathToAtariCartridge->DebugPrint("c64SettingsPathToAtariCartridge=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = c64SettingsPathToAtariCartridge->GetStdASCII();
	
	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading AtariCartridge file");
		return false;
	}
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
	
	guiMain->LockMutex();
	
//	if (menuItemInsertAtariCartridge->str2 != NULL)
//		delete menuItemInsertAtariCartridge->str2;
//	menuItemInsertAtariCartridge->str2 = new CSlrString(fname);
//	delete [] fname;
	
	LoadLabelsAndWatches(path, viewC64->debugInterfaceAtari);
	guiMain->UnlockMutex();

	//
	debugInterfaceAtari->LockMutex();
	viewC64->debugInterfaceAtari->InsertCartridge(asciiPath, false);
	debugInterfaceAtari->UnlockMutex();
	
	delete asciiPath;
	delete file;
	
	return true;
}

//
void CViewMainMenu::InsertATR(CSlrString *path, bool updatePathToATR, bool autoRun, int autoRunEntryNum, bool showLoadAddressInfo)
{
	LOGD("CViewMainMenu::InsertATR: path=%x autoRun=%s autoRunEntryNum=%d", path, STRBOOL(autoRun), autoRunEntryNum);
	
	if (SYS_FileExists(path) == false)
	{
		if (c64SettingsPathToATR != NULL)
			delete c64SettingsPathToATR;
		
		c64SettingsPathToATR = NULL;
		LOGError("InsertATR: file not found, skipping");
		return;
	}
	
	if (c64SettingsPathToATR != path)
	{
		if (c64SettingsPathToATR != NULL)
			delete c64SettingsPathToATR;
		c64SettingsPathToATR = new CSlrString(path);
	}
	
	if (updatePathToATR)
	{
		LOGD("...updatePathToATR");
		if (c64SettingsDefaultATRFolder != NULL)
			delete c64SettingsDefaultATRFolder;
		c64SettingsDefaultATRFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultATRFolder->DebugPrint("c64SettingsDefaultATRFolder=");
	}
	
	char *asciiPath = c64SettingsPathToATR->GetStdASCII();

	debugInterfaceAtari->LockMutex();
	
	// insert ATR
	viewC64->debugInterfaceAtari->MountDisk(asciiPath, 0, false);

	debugInterfaceAtari->UnlockMutex();

	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
	
	guiMain->LockMutex();
	
//	if (menuItemInsertATR->str2 != NULL)
//		delete menuItemInsertATR->str2;
//	menuItemInsertATR->str2 = new CSlrString(fname);
//	delete [] fname;
	
	guiMain->UnlockMutex();
	
	LOGM("Inserted new ATR: %s", asciiPath);
	delete asciiPath;
	
//	if (guiMain->currentView == viewC64->viewFileD64)
//		viewC64->viewFileD64->StartSelectedDiskImageBrowsing();
//	
//	if (autoRun)
//	{
//		viewC64->viewFileD64->StartDiskPRGEntry(autoRunEntryNum, showLoadAddressInfo);
//	}
}

bool CViewMainMenu::LoadNES(CSlrString *path, bool updateNESFolderPath)
{
	if (path == NULL)
		return false;
	
	path->DebugPrint("CViewMainMenu::LoadNES: path=");
	
	LOGD("   >>> LoadNES");
	
	if (c64SettingsPathToNES != path)
	{
		if (c64SettingsPathToNES != NULL)
			delete c64SettingsPathToNES;
		c64SettingsPathToNES = new CSlrString(path);
	}
	
	if (updateNESFolderPath)
	{
		LOGD("...updateNESFolderPath");
		if (c64SettingsDefaultNESFolder != NULL)
			delete c64SettingsDefaultNESFolder;
		c64SettingsDefaultNESFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		
		c64SettingsDefaultNESFolder->DebugPrint("c64SettingsDefaultNESFolder=");
	}
	
	LOGD("... LoadNES (2)");
	
	c64SettingsPathToNES->DebugPrint("c64SettingsPathToNES=");
	
	// TODO: make CSlrFileFromOS support UTF paths
	char *asciiPath = c64SettingsPathToNES->GetStdASCII();
	
	LOGD("asciiPath='%s'", asciiPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("Error loading NES file");
		return false;
	}
	
	// display file name in menu
	char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
		
	guiMain->LockMutex();
	
//	if (menuItemOpenFile->str2 != NULL)
//		delete menuItemOpenFile->str2;
//	menuItemOpenFile->str2 = new CSlrString(fname);
//	delete [] fname;

	LOGTODO("NES: load labels/symbols/watches");
//	LoadLabelsAndWatches(path);
	
	guiMain->UnlockMutex();
	
	//
	debugInterfaceNes->LockMutex();
	viewC64->debugInterfaceNes->InsertCartridge(asciiPath);
	
	if (c64SettingsResetCountersOnAutoRun)
	{
		viewC64->debugInterfaceNes->ResetEmulationFrameCounter();
		viewC64->debugInterfaceNes->ResetMainCpuDebugCycleCounter();
	}

	debugInterfaceNes->UnlockMutex();
	
	delete asciiPath;
	delete file;
	
	return true;
}



void CViewMainMenu::LoadLabelsAndWatches(CSlrString *path, CDebugInterface *debugInterface)
{
	LOGM("CViewMainMenu::LoadLabelsAndWatches, emulator type=%d", debugInterface->GetEmulatorType());
	path->DebugPrint("path=");
	
	CSlrString *fPath = path->GetFilePathWithoutExtension();
	char *noExtPath = fPath->GetStdASCII();
	char *buf = SYS_GetCharBuf();
	
	// check for labels
	if (c64SettingsLoadViceLabels)
	{
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "%s.labels", noExtPath);
		
		LOGD("...searching for labels: %s", buf);
		
		CSlrFileFromOS *file = new CSlrFileFromOS(buf);
		if (file->Exists() == false)
		{
			delete file;
			
			sprintf(buf, "%s.vs", noExtPath);
			
			LOGD("...searching for labels: %s", buf);
			
			file = new CSlrFileFromOS(buf);
		}
		
		if (file->Exists())
		{
			debugInterface->symbols->DeleteAllSymbols();
			debugInterface->symbols->ParseSymbols(file);
		}
		
		delete file;
	}
	
	// check for watches
	if (c64SettingsLoadWatches)
	{
		sprintf(buf, "%s.watch", noExtPath);
		
		LOGD("...searching for watches: %s", buf);
		
		CSlrFileFromOS *file = new CSlrFileFromOS(buf);
		if (file->Exists())
		{
			debugInterface->symbols->DeleteAllWatches();
			debugInterface->symbols->ParseWatches(file);
		}
		
		delete file;
	}
	
	// check for debug info
	if (c64SettingsLoadDebugInfo)
	{
		sprintf(buf, "%s.dbg", noExtPath);
		
		LOGD("...searching for debug info: %s", buf);
		
		CSlrFileFromOS *file = new CSlrFileFromOS(buf);
		if (file->Exists())
		{
			debugInterface->symbols->DeleteAllSymbols();
			debugInterface->symbols->ParseSourceDebugInfo(file);
		}
		else
		{
			LOGError("dbg file does not exist, parsed path=%s", buf);
			path->DebugPrint("error: dbg at origin path does not exist, path=");
		}
		
		delete file;
	}
	
	
	SYS_ReleaseCharBuf(buf);
	delete [] noExtPath;
	delete fPath;
}

bool CViewMainMenu::LoadPRG(CByteBuffer *byteBuffer, bool autoStart, bool showAddressInfo, bool forceFastReset)
{
	if (viewC64->debugInterfaceC64 == NULL)
		return false;
	
	LOGM("CViewMainMenu::LoadPRG: autoStart=%d showAddressInfo=%d c64SettingsAutoJmpDoReset=%d forceFastReset=%d", autoStart, showAddressInfo, c64SettingsAutoJmpDoReset, forceFastReset);
	CByteBuffer *loadPrgByteBuffer = new CByteBuffer(byteBuffer);
	this->loadPrgAutoStart = autoStart;
	this->loadPrgShowAddressInfo = showAddressInfo;
	this->loadPrgForceFastReset = forceFastReset;
	
	viewC64->viewC64MemoryMap->ClearExecuteMarkers();
	viewC64->viewDrive1541MemoryMap->ClearExecuteMarkers();

	if (!this->isRunning)
	{
		SYS_StartThread(this, (void*)loadPrgByteBuffer);
	}
	return true;
}

// TODO: move LoadPRG logic to C64 Tools
void CViewMainMenu::ThreadRun(void *data)
{
	LOGD("CViewMainMenu::ThreadRun");
	
	this->ThreadSetName("CViewMainMenu::LoadPRG");
	
	CByteBuffer *loadPrgByteBuffer = (CByteBuffer *)data;
	
	if (loadPrgForceFastReset)
	{
		// force fast reset
		viewC64->debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
		viewC64->debugInterfaceC64->SetPatchKernalFastBoot(true);
	
		viewC64->debugInterfaceC64->HardReset();
		SYS_Sleep(350);
		
		viewC64->viewFileD64->UpdateDriveDiskID();
		viewC64->debugInterfaceC64->SetPatchKernalFastBoot(c64SettingsFastBootKernalPatch);
		
	}
	else
	{
		// do regular check, depends on user's setting
		if (loadPrgAutoStart && c64SettingsAutoJmpDoReset != MACHINE_RESET_NONE)
		{
			viewC64->debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
			viewC64->debugInterfaceC64->SetPatchKernalFastBoot(true);
			
			if (c64SettingsAutoJmpDoReset == MACHINE_RESET_SOFT)
			{
				viewC64->debugInterfaceC64->Reset();
			}
			else if (c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD)
			{
				viewC64->debugInterfaceC64->HardReset();
			}
			
			SYS_Sleep(c64SettingsAutoJmpWaitAfterReset);
			
			viewC64->viewFileD64->UpdateDriveDiskID();
			viewC64->debugInterfaceC64->SetPatchKernalFastBoot(c64SettingsFastBootKernalPatch);
		}
	}
	
	LoadPRGNotThreaded(loadPrgByteBuffer, loadPrgAutoStart, loadPrgShowAddressInfo);
	delete loadPrgByteBuffer;
	
	LOGD("CViewMainMenu::finished");
}

void CViewMainMenu::SetBasicEndAddr(int endAddr)
{
	// some decrunchers need correct basic pointers
	
	// set beginning of BASIC area
	viewC64->debugInterfaceC64->SetByteC64(0x002B, 0x01);
	viewC64->debugInterfaceC64->SetByteC64(0x002C, 0x08);
	
	// set beginning of variable/arrays area
	viewC64->debugInterfaceC64->SetByteC64(0x002D, endAddr & 0x00FF);
	viewC64->debugInterfaceC64->SetByteC64(0x002F, endAddr & 0x00FF);
	viewC64->debugInterfaceC64->SetByteC64(0x0031, endAddr & 0x00FF);
	viewC64->debugInterfaceC64->SetByteC64(0x00AE, endAddr & 0x00FF);
	
	viewC64->debugInterfaceC64->SetByteC64(0x002E, (endAddr >> 8) & 0x00FF);
	viewC64->debugInterfaceC64->SetByteC64(0x0030, (endAddr >> 8) & 0x00FF);
	viewC64->debugInterfaceC64->SetByteC64(0x0032, (endAddr >> 8) & 0x00FF);
	viewC64->debugInterfaceC64->SetByteC64(0x00AF, (endAddr >> 8) & 0x00FF);
	
	// set end of BASIC area
	viewC64->debugInterfaceC64->SetByteC64(0x0033, 0x00);
	viewC64->debugInterfaceC64->SetByteC64(0x0037, 0x00);
	
	viewC64->debugInterfaceC64->SetByteC64(0x0034, 0xA0);
	viewC64->debugInterfaceC64->SetByteC64(0x0038, 0xA0);
	
	// stop cursor flash
	viewC64->debugInterfaceC64->SetByteC64(0x00CC, 0x01);

}

bool CViewMainMenu::LoadPRGNotThreaded(CByteBuffer *byteBuffer, bool autoStart, bool showAddressInfo)
{
	viewC64->debugInterfaceC64->LockMutex();

	u16 startAddr;
	u16 endAddr;
	
	LoadPRG(byteBuffer, &startAddr, &endAddr);
	
	bool foundBasicSys = false;
	
//	bool isRunBasicCompatibleMode = true;

	if (autoStart == true)
	{
		LOGD("LoadPRG: autostart");
		
		//http://www.lemon64.com/forum/viewtopic.php?t=870&sid=a13a63a952d295ff70c67d93409bc392
		
		if (startAddr == 0x0801)
		{
//			if (isRunBasicCompatibleMode)
			{
				// new "RUN"
				SetBasicEndAddr(endAddr);
				
				viewC64->viewC64MemoryMap->ClearReadWriteMarkers();
				viewC64->viewDrive1541MemoryMap->ClearReadWriteMarkers();
				
				viewC64->debugInterfaceC64->MakeBasicRunC64();

				viewC64->ShowMainScreen();
				
				if (viewC64->debugInterfaceC64->IsCpuJam())
				{
					viewC64->debugInterfaceC64->ForceRunAndUnJamCpu();
				}
			}
			/*
			else
			{
				// search for SYS
			
				// 1001 SYS 2066
				
				// hidden SYS:          $0805    1  0  0  1 SYS   B
				// 0800: 00 10 08 E9 03    00   31 30 30 31  9E  32 30 36 36 00
				
				// not hidden SYS:
				// 0800: 00 0C 08 E9 03 9E      20 32 30 36 36 00
				
				int sysNumberAddr = -1;
				
				u8 b = viewC64->debugInterfaceC64->GetByteC64(0x0805);
				
				if (b == 0x9E)
				{
					// regular SYS
					sysNumberAddr = 0x0806;
				}
				else if (b == 0x00)
				{
					// hidden SYS, scan for SYS ($9E), hope $0900 is enough :)
					for (int i = 0x0806; i < 0x0900; i++)
					{
						if (viewC64->debugInterfaceC64->GetByteC64(i) == 0x9E)
						{
							sysNumberAddr = i + 1;
							break;
						}
					}
				}
				
				if (sysNumberAddr != -1)
				{
					char *buf = SYS_GetCharBuf();
					int i = 0;
					
					u16 addr = sysNumberAddr;
					
					bool isOK = true;
					
					while (true)
					{
						byte c = viewC64->debugInterfaceC64->GetByteC64(addr);
						
						LOGD("addr=%4.4x c=%2.2x '%c'", addr, c, c);
						buf[i] = c;
						
						if (c == 0x20)
						{
							addr++;
							continue;
						}
						
						if (c < 0x30 || c > 0x39)
							break;
						
						addr++;
						i++;
						
						if (i == 254)
						{
							isOK = false;
							break;
						}
					}
					
					if (isOK)
					{
						int startAddr = -1;

						if (isOK)
						{
							foundBasicSys = true;
							startAddr = atoi(buf);
						}

						SetBasicEndAddr(endAddr);

						viewC64->viewC64MemoryMap->ClearReadWriteMarkers();
						viewC64->viewDrive1541MemoryMap->ClearReadWriteMarkers();
						
						viewC64->debugInterfaceC64->MakeJsrC64(startAddr);

						viewC64->ShowMainScreen();

						if (viewC64->debugInterfaceC64->IsCpuJam())
						{
							viewC64->debugInterfaceC64->ForceRunAndUnJamCpu();
						}
					}
					
					SYS_ReleaseCharBuf(buf);
				}
			}
			 */
		}
	}
	
	if (autoStart && c64SettingsAutoJmpAlwaysToLoadedPRGAddress && !foundBasicSys) // && !isRunBasicCompatibleMode)
	{
		LOGD("LoadPRG: c64SettingsAutoJmpAlwaysToLoadedPRGAddress");
		
		if (c64SettingsResetCountersOnAutoRun)
		{
			viewC64->debugInterfaceC64->ResetEmulationFrameCounter();
			viewC64->debugInterfaceC64->ResetMainCpuDebugCycleCounter();
		}
		
		viewC64->debugInterfaceC64->MakeJsrC64(startAddr);
		viewC64->ShowMainScreen();
		
		if (viewC64->debugInterfaceC64->IsCpuJam())
		{
			viewC64->debugInterfaceC64->ForceRunAndUnJamCpu();
		}

	}

	if ((c64SettingsAutoJmpAlwaysToLoadedPRGAddress || autoStart) && showAddressInfo)
	{
		char *buf = SYS_GetCharBuf();
		
		sprintf(buf, "Loaded from $%04X to $%04X", startAddr, endAddr);
		guiMain->ShowMessage(buf);
		
		SYS_ReleaseCharBuf(buf);
	}
	
	if (c64SettingsForceUnpause)
	{
		LOGD("LoadPRG: unpause");
		viewC64->debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
	}
	
	viewC64->debugInterfaceC64->UnlockMutex();
	
	return true;
}


void CViewMainMenu::LoadPRG(CByteBuffer *byteBuffer, u16 *startAddr, u16 *endAddr)
{
	u16 b1 = byteBuffer->GetByte();
	u16 b2 = byteBuffer->GetByte();
	
	u16 loadPoint = (b2 << 8) | b1;
	
	LOGD("..loadPoint=%4.4x", loadPoint);
	
	u16 addr = loadPoint;
	while (!byteBuffer->IsEof())
	{
		u8 b = byteBuffer->GetByte();
		
		
		//		viewC64->debugInterface->SetByteC64(addr, b);
		
		viewC64->debugInterfaceC64->SetByteToRamC64(addr, b);
		addr++;
	}
	
	LOGD("LoadPRG: ..loaded till=%4.4x", addr);
	
	*startAddr = loadPoint;
	*endAddr = addr;
}

void CViewMainMenu::ResetAndJSR(int startAddr)
{
	viewC64->debugInterfaceC64->Reset();
	viewC64->debugInterfaceC64->MakeJsrC64(startAddr);

}

void CViewMainMenu::ReloadAndRestartPRG()
{
	LOGM("CViewMainMenu::ReloadAndRestartPRG");

	if (c64SettingsPathToPRG != NULL)
	{
		char *asciiPath = c64SettingsPathToPRG->GetStdASCII();
		LOGD("asciiPath='%s'", asciiPath);
		
		CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath);
		if (!file->Exists())
		{
			delete file;
			guiMain->ShowMessage("Error loading PRG file");
			return;
		}
		
		guiMain->LockMutex();
		LoadLabelsAndWatches(c64SettingsPathToPRG, viewC64->debugInterfaceC64);
		guiMain->UnlockMutex();

		CByteBuffer *byteBuffer = new CByteBuffer(file, false);
		LoadPRG(byteBuffer, true, false, false);
		
		delete asciiPath;
		delete file;
		
		delete byteBuffer;
		
		return;
	}
	else
	{
		guiMain->ShowMessage("Select PRG first");
	}
}


void CViewMainMenu::SystemDialogFileOpenCancelled()
{
}

void CViewMainMenu::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewMainMenu::Render()
{
	//LOGD("CViewMainMenu::Render");
	
//	guiMain->fntConsole->BlitText("CViewMainMenu", 0, 0, 0, 11, 1.0);

	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
		
	float sb = 20;
	float gap = 10;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextB;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = 3;
	
	float scrx = sb;
	float scry = sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = scrsx/2.0f + sb;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy,
						viewC64->colorsTheme->colorBackgroundR,
						viewC64->colorsTheme->colorBackgroundG,
						viewC64->colorsTheme->colorBackgroundB, 1.0);
	
	float px = scrx + gap;
	float py = scry + gap;
	
	font->BlitTextColor(strHeader, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;

	viewMenu->Render();
	
//	font->BlitTextColor("1541 Device 8...", px, py, -1, fontScale, tr, tg, tb, 1);
//	font->BlitTextColor("Alt+8", ax, py, -1, fontScale, tr, tg, tb, 1);
	
	CGuiView::Render();
}

void CViewMainMenu::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewMainMenu::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewMainMenu::ButtonPressed(CGuiButton *button)
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

//@returns is consumed
bool CViewMainMenu::DoTap(float x, float y)
{
	LOGG("CViewMainMenu::DoTap:  x=%f y=%f", x, y);
	
	viewMenu->DoTap(x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewMainMenu::DoFinishTap(float x, float y)
{
	LOGG("CViewMainMenu::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewMainMenu::DoDoubleTap(float x, float y)
{
	LOGG("CViewMainMenu::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewMainMenu::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewMainMenu::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewMainMenu::DoScrollWheel(float deltaX, float deltaY)
{
	return viewMenu->DoScrollWheel(deltaX, deltaY);
}

bool CViewMainMenu::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewMainMenu::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewMainMenu::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewMainMenu::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewMainMenu::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewMainMenu::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewMainMenu::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewMainMenu::SwitchMainMenuScreen()
{
//	if (guiMain->currentView == this)
//	{
//		viewC64->ShowMainScreen();
//	}
//	else
//	{
//		guiMain->SetView(this);
//	}
}

bool CViewMainMenu::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
	
	if (keyCode == MTKEY_BACKSPACE)
	{
		SwitchMainMenuScreen();
		return true;
	}
	
	if (viewMenu->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	if (keyCode == MTKEY_ESC)
	{
		SwitchMainMenuScreen();
		return true;
	}


	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewMainMenu::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (viewMenu->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewMainMenu::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewMainMenu::ActivateView()
{
	LOGG("CViewMainMenu::ActivateView()");
	
	viewC64->ShowMouseCursor();
}

void CViewMainMenu::DeactivateView()
{
	LOGG("CViewMainMenu::DeactivateView()");
}

void CViewMainMenu::ApplyColorsToMenu(CGuiViewMenu *menu)
{
	float r = viewC64->colorsTheme->colorTextR;
	float g = viewC64->colorsTheme->colorTextG;
	float b = viewC64->colorsTheme->colorTextB;
	
	for (std::list<CGuiViewMenuItem *>::iterator it = menu->menuItems.begin(); it != menu->menuItems.end(); it++)
	{
		CGuiViewMenuItem *menuItem = *it;
		CViewC64MenuItem *menuItemC64 = (CViewC64MenuItem*)menuItem;
		menuItemC64->r = r;
		menuItemC64->g = g;
		menuItemC64->b = b;		
	}
}

void CViewMainMenu::UpdateTheme()
{
	ApplyColorsToMenu(this->viewMenu);
	
	ApplyColorsToMenu(viewC64->viewC64SettingsMenu->viewMenu);
	ApplyColorsToMenu(viewC64->viewC64SettingsMenu->menuItemSubMenuEmulation->subMenu);
	ApplyColorsToMenu(viewC64->viewC64SettingsMenu->menuItemSubMenuAudio->subMenu);
	ApplyColorsToMenu(viewC64->viewC64SettingsMenu->menuItemSubMenuMemory->subMenu);
	ApplyColorsToMenu(viewC64->viewC64SettingsMenu->menuItemSubMenuUI->subMenu);
	ApplyColorsToMenu(viewC64->viewKeyboardShortcuts->viewMenu);

	if (viewC64->debugInterfaceC64)
	{
		ApplyColorsToMenu(viewC64->viewFileD64->viewMenu);
	}

	CGuiView::UpdateTheme();
}

CViewC64MenuItem::CViewC64MenuItem(float height, CSlrString *strIn, CSlrKeyboardShortcut *shortcut, float r, float g, float b)
: CGuiViewMenuItem(height)
{
	this->str = NULL;
	this->str2 = NULL;
	this->shortcut = shortcut;
	this->r = r;
	this->g = g;
	this->b = b;
	
	if (strIn != NULL)
	{
		this->SetString(strIn);
	}
}

// sub item
CViewC64MenuItem::CViewC64MenuItem(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b,
								   CGuiViewMenu *mainMenu)
: CGuiViewMenuItem(height, mainMenu)
{
	this->str = NULL;
	this->str2 = NULL;
	this->shortcut = shortcut;
	this->r = r;
	this->g = g;
	this->b = b;
	
	if (str != NULL)
		this->SetString(str);
}

void CViewC64MenuItem::SetString(CSlrString *str)
{
	if (this->str != NULL)
	{
		delete this->str;
	}
	this->str = str;
	if (this->isSelected)
	{
		this->isSelected = false;
		this->SetSelected(true);
		this->isSelected = true;
	}
}

void CViewC64MenuItem::Execute()
{
}

void CViewC64MenuItem::DebugPrint()
{
	LOGD("CGuiViewMenuItem: %x menu=%x subMenu=%x str=%x", this, this->menu, this->subMenu, this->str);

//	if (this->str != NULL)
//	{
//		this->str->DebugPrint("menu item str=");
//		LOGD("color=%f %f %f", r, g, b);
//	}
}

void CViewC64MenuItem::SetSelected(bool selected)
{
	if (this->isSelected != selected == true)
	{
		for (int i = 0; i < str->GetLength(); i++)
		{
			u16 chr = str->GetChar(i);
			chr ^= CBMSHIFTEDFONT_INVERT;
			str->SetChar(i, chr);
		}
		return;
	}
}

void CViewC64MenuItem::RenderItem(float px, float py, float pz)
{
	viewC64->viewC64MainMenu->font->BlitTextColor(str, px, py, pz,
												  viewC64->viewC64MainMenu->fontScale, r, g, b, 1);

	if (shortcut != NULL && shortcut->str != NULL)
	{
		viewC64->viewC64MainMenu->font->BlitTextColor(shortcut->str, px + 510, py, pz,
													  viewC64->viewC64MainMenu->fontScale,
													  viewC64->colorsTheme->colorTextKeyShortcutR,
													  viewC64->colorsTheme->colorTextKeyShortcutG,
													  viewC64->colorsTheme->colorTextKeyShortcutB, 1,
													  FONT_ALIGN_RIGHT);
	}
	
	if (str2 != NULL)
	{
		py += viewC64->viewC64MainMenu->fontHeight;
		viewC64->viewC64MainMenu->font->BlitTextColor(str2, px, py, pz,
													  viewC64->viewC64MainMenu->fontScale, r, g, b, 1);
	}
}

//

CViewC64MenuItemOption::CViewC64MenuItemOption(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b,
					   std::vector<CSlrString *> *options, CSlrFont *font, float fontScale)
: CViewC64MenuItem(height, NULL, shortcut, r, g, b)
{
	this->options = options;
	this->selectedOption = 0;
	
	textStr = NULL;

	// update display string
	this->SetString(str);
}

void CViewC64MenuItemOption::SetOptions(std::vector<CSlrString *> *options)
{
	if (this->options != NULL)
	{
		while (!this->options->empty())
		{
			CSlrString *opt = this->options->back();
			this->options->pop_back();
			delete opt;
		}
		delete this->options;
	}
	
	this->options = options;
	
	UpdateDisplayString();
}

void CViewC64MenuItemOption::SetOptionsWithoutDelete(std::vector<CSlrString *> *options)
{
	this->options = options;
	
	UpdateDisplayString();
}

void CViewC64MenuItemOption::SetString(CSlrString *str)
{
	if (this->textStr != NULL)
		delete this->textStr;
	
	this->textStr = str;
	
	UpdateDisplayString();
	
}

void CViewC64MenuItemOption::UpdateDisplayString()
{
	if (this->options != NULL)
	{
		CSlrString *newStr = new CSlrString(this->textStr);
		newStr->Concatenate((*this->options)[selectedOption]);
		
		CViewC64MenuItem::SetString(newStr);
	}
}

void CViewC64MenuItemOption::SwitchToPrev()
{
	if (selectedOption == 0)
	{
		selectedOption = options->size()-1;
	}
	else
	{
		selectedOption--;
	}
	
	this->UpdateDisplayString();
	
	this->menu->callback->MenuCallbackItemChanged(this);
}

void CViewC64MenuItemOption::SwitchToNext()
{
	if (selectedOption == options->size()-1)
	{
		selectedOption = 0;
	}
	else
	{
		selectedOption++;
	}
	
	this->UpdateDisplayString();
	
	this->menu->callback->MenuCallbackItemChanged(this);
}

void CViewC64MenuItemOption::SetSelectedOption(int newSelectedOption, bool runCallback)
{
	selectedOption = newSelectedOption;
	this->UpdateDisplayString();
	
	if (runCallback)
		this->menu->callback->MenuCallbackItemChanged(this);
}

bool CViewC64MenuItemOption::KeyDown(u32 keyCode)
{
	if (keyCode == MTKEY_ARROW_LEFT)
	{
		SwitchToPrev();
		return true;
	}
	else if (keyCode == MTKEY_ARROW_RIGHT || keyCode == MTKEY_ENTER)
	{
		SwitchToNext();
		return true;
	}
	
	return false;
}

void CViewC64MenuItemOption::Execute()
{
	SwitchToNext();
}

//


CViewC64MenuItemFloat::CViewC64MenuItemFloat(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b,
											 float minimum, float maximum, float step, CSlrFont *font, float fontScale)
: CViewC64MenuItem(height, NULL, shortcut, r, g, b)
{
	this->minimum = minimum;
	this->maximum = maximum;
	this->step = step;
	
	textStr = NULL;
	
	numLeadingDigits = 5;
	numDecimalsDigits = 2;
	value = 0;
	
	// update display string
	this->SetString(str);
}

void CViewC64MenuItemFloat::SetValue(float value, bool runCallback)
{
	this->value = value;
	
	UpdateDisplayString();
	
	if (runCallback)
		this->menu->callback->MenuCallbackItemChanged(this);
}

void CViewC64MenuItemFloat::SetString(CSlrString *str)
{
	if (this->textStr != NULL)
		delete this->textStr;
	
	this->textStr = str;
	
	UpdateDisplayString();
	
}

void CViewC64MenuItemFloat::UpdateDisplayString()
{
	char *buf = SYS_GetCharBuf();
	char *bufFormat = SYS_GetCharBuf();

	sprintf(bufFormat, "%%-%d.%df", numLeadingDigits, numDecimalsDigits);
	sprintf(buf, bufFormat, value);
	CSlrString *valStr = new CSlrString(buf);
	
	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(bufFormat);
	
	CSlrString *newStr = new CSlrString(this->textStr);
	newStr->Concatenate(valStr);
	CViewC64MenuItem::SetString(newStr);
	
	delete valStr;
}

void CViewC64MenuItemFloat::SwitchToPrev()
{
	value -= step;
	
	if (value < minimum)
		value = minimum;
	
	this->UpdateDisplayString();
	
	this->menu->callback->MenuCallbackItemChanged(this);
}

void CViewC64MenuItemFloat::SwitchToNext()
{
	value += step;
	
	if (value > maximum)
		value = maximum;
	
	this->UpdateDisplayString();
	
	this->menu->callback->MenuCallbackItemChanged(this);
}

bool CViewC64MenuItemFloat::KeyDown(u32 keyCode)
{
	if (keyCode == MTKEY_ARROW_LEFT)
	{
		SwitchToPrev();
		return true;
	}
	else if (keyCode == MTKEY_ARROW_RIGHT || keyCode == MTKEY_ENTER)
	{
		SwitchToNext();
		return true;
	}
	
	return false;
}

void CViewC64MenuItemFloat::Execute()
{
	SwitchToNext();
}

