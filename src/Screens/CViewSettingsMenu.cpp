//
// NOTE:
// This view will be removed. It is being refactored and moved to main menu bar instead
//

#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewSettingsMenu.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64SettingsStorage.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CViewC64KeyMap.h"
#include "CViewKeyboardShortcuts.h"
#include "CViewMonitorConsole.h"
#include "CDebugMemoryMap.h"
#include "CDebugMemoryMapCell.h"
#include "MTH_Random.h"
#include "C64Palette.h"

#include "CViewC64StateSID.h"
#include "CViewMemoryMap.h"

#include "CGuiMain.h"
#include "CLayoutManager.h"
#include "SND_SoundEngine.h"

#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"

#include "CSnapshotsManager.h"


#if defined(WIN32)
extern "C" {
	int uilib_cpu_is_smp(void);
	int set_single_cpu(int val, void *param);	// 1=set to first CPU, 0=set to all CPUs
}
#endif

#define VIEWC64SETTINGS_DUMP_C64_MEMORY					1
#define VIEWC64SETTINGS_DUMP_C64_MEMORY_MARKERS			2
#define VIEWC64SETTINGS_DUMP_DRIVE1541_MEMORY			3
#define VIEWC64SETTINGS_DUMP_DRIVE1541_MEMORY_MARKERS	4
#define VIEWC64SETTINGS_MAP_C64_MEMORY_TO_FILE			5
#define VIEWC64SETTINGS_ATTACH_TAPE						6
#define VIEWC64SETTINGS_SET_C64_PROFILER_OUTPUT			7

int settingsReuSizes[8] = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384 };

CViewSettingsMenu::CViewSettingsMenu(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewSettingsMenu";

	font = viewC64->fontCBMShifted;
	fontScale = 2.7;
	fontHeight = font->GetCharHeight('@', fontScale) + 3;

	strHeader = new CSlrString("Settings");

	memoryExtensions.push_back(new CSlrString("bin"));
	csvExtensions.push_back(new CSlrString("csv"));
	profilerExtensions.push_back(new CSlrString("pd"));
	
	/// colors
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;

	float sb = 20;

	/// menu
	viewMenu = new CGuiViewMenu(35, 51, -1, sizeX-70, sizeY-51-sb, this);

	//
	menuItemBack  = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< BACK"),
										 NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemBack);

	//
	CViewC64MenuItem *menuItemBackSubMenu;
	
	///
	
	menuItemSubMenuEmulation = new CViewC64MenuItem(fontHeight, new CSlrString("Emulation >>"),
													NULL, tr, tg, tb, viewMenu);
	viewMenu->AddMenuItem(menuItemSubMenuEmulation);
	
	
	menuItemBackSubMenu = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< BACK to Settings"),
																 NULL, tr, tg, tb);
	menuItemBackSubMenu->subMenu = viewMenu;
	menuItemSubMenuEmulation->subMenu->AddMenuItem(menuItemBackSubMenu);
	
	//
	
	//
	if (viewC64->debugInterfaceC64)
	{
		menuItemSubMenuTape = new CViewC64MenuItem(fontHeight, new CSlrString("Tape >>"),
													NULL, tr, tg, tb, viewMenu);
		viewMenu->AddMenuItem(menuItemSubMenuTape);
		
//		menuItemSubMenuTape->DebugPrint();

		//
		menuItemSubMenuReu = new CViewC64MenuItem(fontHeight, new CSlrString("REU >>"),
												   NULL, tr, tg, tb, viewMenu);
		viewMenu->AddMenuItem(menuItemSubMenuReu);
		
		//

		menuItemBackSubMenu = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< BACK to Settings"),
												   NULL, tr, tg, tb);
		menuItemBackSubMenu->subMenu = viewMenu;
		menuItemSubMenuTape->subMenu->AddMenuItem(menuItemBackSubMenu);
		menuItemSubMenuReu->subMenu->AddMenuItem(menuItemBackSubMenu);
	}

	//
	menuItemSubMenuAudio = new CViewC64MenuItem(fontHeight, new CSlrString("Audio >>"),
												NULL, tr, tg, tb, viewMenu);
	viewMenu->AddMenuItem(menuItemSubMenuAudio);
	
	
	menuItemBackSubMenu = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< BACK to Settings"),
											   NULL, tr, tg, tb);
	menuItemBackSubMenu->subMenu = viewMenu;
	menuItemSubMenuAudio->subMenu->AddMenuItem(menuItemBackSubMenu);
	
	//
	
	//
	menuItemSubMenuMemory = new CViewC64MenuItem(fontHeight, new CSlrString("Memory >>"),
												NULL, tr, tg, tb, viewMenu);
	viewMenu->AddMenuItem(menuItemSubMenuMemory);
	
	
	menuItemBackSubMenu = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< BACK to Settings"),
											   NULL, tr, tg, tb);
	menuItemBackSubMenu->subMenu = viewMenu;
	menuItemSubMenuMemory->subMenu->AddMenuItem(menuItemBackSubMenu);
	
	//

	//
	menuItemSubMenuUI = new CViewC64MenuItem(fontHeight*2, new CSlrString("UI >>"),
												 NULL, tr, tg, tb, viewMenu);
	viewMenu->AddMenuItem(menuItemSubMenuUI);
	
	
	CSlrString *str = new CSlrString("<< BACK to Settings");
	menuItemBackSubMenu = new CViewC64MenuItem(fontHeight*2.0f, str,
											   NULL, tr, tg, tb);
	menuItemBackSubMenu->subMenu = viewMenu;
	menuItemSubMenuUI->subMenu->AddMenuItem(menuItemBackSubMenu);
	
	//

	
	///
	
	//
	std::vector<CSlrString *> *options = NULL;
	std::vector<CSlrString *> *optionsYesNo = new std::vector<CSlrString *>();
	optionsYesNo->push_back(new CSlrString("No"));
	optionsYesNo->push_back(new CSlrString("Yes"));

	std::vector<CSlrString *> *optionsColors = new std::vector<CSlrString *>();
	optionsColors->push_back(new CSlrString("red"));
	optionsColors->push_back(new CSlrString("green"));
	optionsColors->push_back(new CSlrString("blue"));
	optionsColors->push_back(new CSlrString("black"));
	optionsColors->push_back(new CSlrString("dark gray"));
	optionsColors->push_back(new CSlrString("light gray"));
	optionsColors->push_back(new CSlrString("white"));
	optionsColors->push_back(new CSlrString("yellow"));
	optionsColors->push_back(new CSlrString("cyan"));
	optionsColors->push_back(new CSlrString("magenta"));
	optionsColors->push_back(new CSlrString("orange"));
	optionsColors->push_back(new CSlrString("mid gray"));

	if (viewC64->debugInterfaceAtari)
	{
		menuItemSetFolderWithAtariROMs = new CViewC64MenuItem(fontHeight*2, new CSlrString("Set ROMs folder"), NULL, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemSetFolderWithAtariROMs);
	}

	//
	kbsDetachEverything = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Everything", 'd', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachEverything);
	
	kbsDetachDiskImage = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Disk Image", '8', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachDiskImage);
	
	kbsDetachCartridge = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Detach Cartridge", '0', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsDetachCartridge);
	

	if (viewC64->debugInterfaceC64)
	{
		//
		c64ModelTypeIds = new std::vector<int>();
		options = new std::vector<CSlrString *>();
		viewC64->debugInterfaceC64->GetC64ModelTypes(options, c64ModelTypeIds);
		
		menuItemC64Model = new CViewC64MenuItemOption(fontHeight, new CSlrString("Machine model: "),
													  NULL, tr, tg, tb, options, font, fontScale);
		menuItemC64Model->SetSelectedOption(c64SettingsC64Model, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemC64Model);
		
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("10"));
		options->push_back(new CSlrString("20"));
		options->push_back(new CSlrString("50"));
		options->push_back(new CSlrString("100"));
		options->push_back(new CSlrString("200"));
		options->push_back(new CSlrString("300"));
		options->push_back(new CSlrString("400"));
		
		kbsSwitchNextMaximumSpeed = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Next maximum speed", ']', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsSwitchNextMaximumSpeed);
		kbsSwitchPrevMaximumSpeed = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Previous maximum speed", '[', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsSwitchPrevMaximumSpeed);

		menuItemMaximumSpeed = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Maximum speed: "),
														  NULL, tr, tg, tb, options, font, fontScale);
		menuItemMaximumSpeed->SetSelectedOption(3, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemMaximumSpeed);
		
		//
		
		
		//
		options = new std::vector<CSlrString *>();
		viewC64->debugInterfaceC64->GetSidTypes(options);
		menuItemSIDModel = new CViewC64MenuItemOption(fontHeight, new CSlrString("SID model: "),
													  NULL, tr, tg, tb, options, font, fontScale);
		menuItemSIDModel->SetSelectedOption(c64SettingsSIDEngineModel, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemSIDModel);
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		//
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("PAL"));
		options->push_back(new CSlrString("NTSC"));
		
		menuItemAtariVideoSystem = new CViewC64MenuItemOption(fontHeight, new CSlrString("Video system: "),
														 NULL, tr, tg, tb, options, font, fontScale);
		menuItemAtariVideoSystem->SetSelectedOption(c64SettingsAtariVideoSystem, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAtariVideoSystem);

		optionsAtariMachineTypes = new std::vector<CSlrString *>();
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 400 (16 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 800 (48 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 1200XL (64 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 600XL (16 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 800XL (64 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 130XE (128 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari XEGS (64 KB)"));
		optionsAtariMachineTypes->push_back(new CSlrString("Atari 5200 (16 KB)"));
		menuItemAtariMachineType = new CViewC64MenuItemOption(fontHeight, new CSlrString("Machine type: "),
															  NULL, tr, tg, tb, optionsAtariMachineTypes, font, fontScale);
		menuItemAtariMachineType->SetSelectedOption(c64SettingsAtariMachineType, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAtariMachineType);

		optionsAtariRamSize800 = new std::vector<CSlrString *>();
		optionsAtariRamSize800->push_back(new CSlrString("8 KB"));
		optionsAtariRamSize800->push_back(new CSlrString("16 KB"));
		optionsAtariRamSize800->push_back(new CSlrString("24 KB"));
		optionsAtariRamSize800->push_back(new CSlrString("32 KB"));
		optionsAtariRamSize800->push_back(new CSlrString("40 KB"));
		optionsAtariRamSize800->push_back(new CSlrString("48 KB"));
		optionsAtariRamSize800->push_back(new CSlrString("52 KB"));

		optionsAtariRamSizeXL = new std::vector<CSlrString *>();
		optionsAtariRamSizeXL->push_back(new CSlrString("16 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("32 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("48 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("64 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("128 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("192 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("320 KB (Rambo)"));
		optionsAtariRamSizeXL->push_back(new CSlrString("320 KB (Compy-Shop)"));
		optionsAtariRamSizeXL->push_back(new CSlrString("576 KB"));
		optionsAtariRamSizeXL->push_back(new CSlrString("1088 KB"));

		optionsAtariRamSize5200 = new std::vector<CSlrString *>();
		optionsAtariRamSize5200->push_back(new CSlrString("16 kB"));

		menuItemAtariRamSize = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Ram size: "),
															  NULL, tr, tg, tb, options, font, fontScale);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAtariRamSize);
		UpdateAtariRamSizeOptions();
		menuItemAtariRamSize->SetSelectedOption(c64SettingsAtariRamSizeOption, false);

	}
	
	//
	menuItemAudioOutDevice = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Audio Out device: "),
														NULL, tr, tg, tb, NULL, font, fontScale);
	menuItemSubMenuAudio->AddMenuItem(menuItemAudioOutDevice);
	
	if (viewC64->debugInterfaceC64)
	{
		menuItemAudioVolume = new CViewC64MenuItemFloat(fontHeight, new CSlrString("VICE Audio volume: "),
														NULL, tr, tg, tb,
														0.0f, 100.0f, 1.0f, font, fontScale);
		menuItemAudioVolume->numDecimalsDigits = 0;
		menuItemAudioVolume->SetValue(100.0f, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemAudioVolume);
		
		//
		menuItemMuteSIDOnPause = new CViewC64MenuItemOption(fontHeight, new CSlrString("Mute SID on pause: "),
															NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemMuteSIDOnPause->SetSelectedOption(c64SettingsMuteSIDOnPause ? 1 : 0, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemMuteSIDOnPause);
		
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
		
		//
		// samplingMethod: Fast=0, Interpolating=1, Resampling=2, Fast Resampling=3
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("Fast"));
		options->push_back(new CSlrString("Interpolating"));
		options->push_back(new CSlrString("Resampling"));
		options->push_back(new CSlrString("Fast Resampling"));
		
		menuItemRESIDSamplingMethod = new CViewC64MenuItemOption(fontHeight, new CSlrString("RESID Sampling method: "),
																 NULL, tr, tg, tb, options, font, fontScale);
		menuItemRESIDSamplingMethod->SetSelectedOption(c64SettingsRESIDSamplingMethod, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemRESIDSamplingMethod);
		
		menuItemRESIDEmulateFilters = new CViewC64MenuItemOption(fontHeight, new CSlrString("RESID Emulate filters: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemRESIDEmulateFilters->SetSelectedOption((c64SettingsRESIDEmulateFilters == 1 ? true : false), false);
		menuItemSubMenuAudio->AddMenuItem(menuItemRESIDEmulateFilters);
		
		//
		menuItemRESIDPassBand = new CViewC64MenuItemFloat(fontHeight, new CSlrString("RESID Pass Band: "),
														  NULL, tr, tg, tb,
														  0.00f, 90.0f, 1.00f, font, fontScale);
		menuItemRESIDPassBand->SetValue(c64SettingsRESIDPassBand, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemRESIDPassBand);
		
		menuItemRESIDFilterBias = new CViewC64MenuItemFloat(fontHeight*2, new CSlrString("RESID Filter Bias: "),
															NULL, tr, tg, tb,
															-500.0f, 500.0f, 1.00f, font, fontScale);
		menuItemRESIDFilterBias->SetValue(c64SettingsRESIDFilterBias, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemRESIDFilterBias);
		
		//
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("None"));
		options->push_back(new CSlrString("Two SIDs"));
		options->push_back(new CSlrString("Three SIDs"));
		menuItemSIDStereo = new CViewC64MenuItemOption(fontHeight, new CSlrString("SID stereo: "),
													   NULL, tr, tg, tb, options, font, fontScale);
		menuItemSIDStereo->SetSelectedOption(c64SettingsSIDStereo, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemSIDStereo);
		
		options = GetSidAddressOptions();
		
		menuItemSIDStereoAddress = new CViewC64MenuItemOption(fontHeight, new CSlrString("SID #1 address: "),
															  NULL, tr, tg, tb, options, font, fontScale);
		int optNum = GetOptionNumFromSidAddress(c64SettingsSIDStereoAddress);
		menuItemSIDStereoAddress->SetSelectedOption(optNum, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemSIDStereoAddress);
		
		menuItemSIDTripleAddress = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("SID #2 address: "),
															  NULL, tr, tg, tb, options, font, fontScale);
		optNum = GetOptionNumFromSidAddress(c64SettingsSIDTripleAddress);
		menuItemSIDTripleAddress->SetSelectedOption(optNum, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemSIDTripleAddress);

		//
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("Copy to RAM"));
		options->push_back(new CSlrString("PSID64"));
		menuItemSIDImportMode = new CViewC64MenuItemOption(fontHeight*1, new CSlrString("SID import: "),
														 NULL, tr, tg, tb, options, font, fontScale);
		menuItemSIDImportMode->SetSelectedOption(c64SettingsC64SidImportMode, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemSIDImportMode);

		//
		menuItemSIDHistoryMaxSize = new CViewC64MenuItemFloat(fontHeight*2, new CSlrString("SID History size: "),
															  NULL, tr, tg, tb,
															  50.0f, 120000.0f, 50.00f, font, fontScale);
		menuItemSIDHistoryMaxSize->SetValue(c64SettingsSidDataHistoryMaxSize, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemSIDHistoryMaxSize);

	}
	
	if (viewC64->debugInterfaceAtari)
	{
		/* TODO: re-init Atari SoundInit and update num channels when this is changed
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("None"));
		options->push_back(new CSlrString("Stereo"));
		menuItemAtariPokeyStereo = new CViewC64MenuItemOption(fontHeight, new CSlrString("POKEY stereo: "),
													   NULL, tr, tg, tb, options, font, fontScale);
		menuItemAtariPokeyStereo->SetSelectedOption(c64SettingsAtariPokeyStereo, false);
		menuItemSubMenuAudio->AddMenuItem(menuItemAtariPokeyStereo);
		 */
	}

	//
	menuItemRestartAudioOnEmulationReset = new CViewC64MenuItemOption(fontHeight, new CSlrString("Restart audio on reset: "),
																	  NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemRestartAudioOnEmulationReset->SetSelectedOption(c64SettingsRestartAudioOnEmulationReset ? 1 : 0, false);
	menuItemSubMenuAudio->AddMenuItem(menuItemRestartAudioOnEmulationReset);
	
	kbsSwitchSoundOnOff = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Switch sound mute On/Off", 't', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSwitchSoundOnOff);
	
	//
	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("RGB"));
	options->push_back(new CSlrString("Gray"));
	options->push_back(new CSlrString("None"));
	
	menuItemMemoryCellsColorStyle = new CViewC64MenuItemOption(fontHeight, new CSlrString("Memory map values color: "),
														NULL, tr, tg, tb, options, font, fontScale);
	menuItemMemoryCellsColorStyle->SetSelectedOption(c64SettingsMemoryValuesStyle, false);
	menuItemSubMenuMemory->AddMenuItem(menuItemMemoryCellsColorStyle);
	
	//
	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("Default"));
	options->push_back(new CSlrString("ICU"));
	
	menuItemMemoryMarkersColorStyle = new CViewC64MenuItemOption(fontHeight, new CSlrString("Memory map markers color: "),
															   NULL, tr, tg, tb, options, font, fontScale);
	menuItemMemoryMarkersColorStyle->SetSelectedOption(c64SettingsMemoryMarkersStyle, false);
	menuItemSubMenuMemory->AddMenuItem(menuItemMemoryMarkersColorStyle);
	
	//
//	options = new std::vector<CSlrString *>();
//	options->push_back(new CSlrString("No"));
//	options->push_back(new CSlrString("Yes"));
	
	menuItemMemoryMapInvert = new CViewC64MenuItemOption(fontHeight, new CSlrString("Invert memory map zoom: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemMemoryMapInvert->SetSelectedOption(c64SettingsMemoryMapInvertControl, false);
	menuItemSubMenuMemory->AddMenuItem(menuItemMemoryMapInvert);

	//
	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("1"));
	options->push_back(new CSlrString("10"));
	options->push_back(new CSlrString("20"));
	options->push_back(new CSlrString("50"));
	options->push_back(new CSlrString("100"));
	options->push_back(new CSlrString("200"));
	options->push_back(new CSlrString("300"));
	options->push_back(new CSlrString("400"));
	options->push_back(new CSlrString("500"));
	options->push_back(new CSlrString("1000"));
	
	menuItemMemoryMapFadeSpeed = new CViewC64MenuItemOption(fontHeight, new CSlrString("Markers fade out speed: "),
													  NULL, tr, tg, tb, options, font, fontScale);
	menuItemMemoryMapFadeSpeed->SetSelectedOption(5, false);
	menuItemSubMenuMemory->AddMenuItem(menuItemMemoryMapFadeSpeed);

	
	//
	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("1"));
	options->push_back(new CSlrString("2"));
	options->push_back(new CSlrString("4"));
	options->push_back(new CSlrString("10"));
	options->push_back(new CSlrString("20"));

#if defined(MACOS)
	float fh = fontHeight;
#else
	float fh = fontHeight*2;
#endif
	
	menuItemMemoryMapRefreshRate = new CViewC64MenuItemOption(fh, new CSlrString("Memory map refresh rate: "),
														 NULL, tr, tg, tb, options, font, fontScale);
	menuItemMemoryMapRefreshRate->SetSelectedOption(1, false);
	menuItemSubMenuMemory->AddMenuItem(menuItemMemoryMapRefreshRate);
	
	//
#if defined(MACOS)
//	options = new std::vector<CSlrString *>();
//	options->push_back(new CSlrString("No"));
//	options->push_back(new CSlrString("Yes"));
	
	menuItemMultiTouchMemoryMap = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Multi-touch map control: "),
														NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemMultiTouchMemoryMap->SetSelectedOption(c64SettingsUseMultiTouchInMemoryMap, false);
	menuItemSubMenuMemory->AddMenuItem(menuItemMultiTouchMemoryMap);
#endif
	

	//
	menuItemWindowAlwaysOnTop = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Window always on top: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemWindowAlwaysOnTop->SetSelectedOption(c64SettingsWindowAlwaysOnTop, false);
	menuItemSubMenuUI->AddMenuItem(menuItemWindowAlwaysOnTop);
	

	menuItemScreenRasterViewfinderScale = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Screen viewfinder scale: "),
																	NULL, tr, tg, tb,
																	0.05f, 25.0f, 0.05f, font, fontScale);
	menuItemScreenRasterViewfinderScale->SetValue(1.5f, false);
	menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterViewfinderScale);

	
	menuItemScreenGridLinesAlpha = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Screen grid lines alpha: "),
															 NULL, tr, tg, tb,
															 0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemScreenGridLinesAlpha->SetValue(0.35f, false);
	menuItemSubMenuUI->AddMenuItem(menuItemScreenGridLinesAlpha);

///////
	
	menuItemScreenGridLinesColorScheme = new CViewC64MenuItemOption(fontHeight*2.0f, new CSlrString("Grid lines: "),
																			  NULL, tr, tg, tb, optionsColors, font, fontScale);
	menuItemScreenGridLinesColorScheme->SetSelectedOption(0, false);
	menuItemSubMenuUI->AddMenuItem(menuItemScreenGridLinesColorScheme);

	
	if (viewC64->debugInterfaceC64)
	{
		menuItemScreenRasterCrossLinesAlpha = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Raster cross lines alpha: "),
																		NULL, tr, tg, tb,
																		0.0f, 1.0f, 0.05f, font, fontScale);
		menuItemScreenRasterCrossLinesAlpha->SetValue(0.35f, false);
		menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterCrossLinesAlpha);
		
		menuItemScreenRasterCrossLinesColorScheme = new CViewC64MenuItemOption(fontHeight, new CSlrString("Raster cross lines: "),
																			   NULL, tr, tg, tb, optionsColors, font, fontScale);
		menuItemScreenRasterCrossLinesColorScheme->SetSelectedOption(6, false);
		menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterCrossLinesColorScheme);
		
		
		menuItemScreenRasterCrossAlpha = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Raster cross alpha: "),
																   NULL, tr, tg, tb,
																   0.0f, 1.0f, 0.05f, font, fontScale);
		menuItemScreenRasterCrossAlpha->SetValue(0.85f, false);
		menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterCrossAlpha);
		
		menuItemScreenRasterCrossInteriorColorScheme = new CViewC64MenuItemOption(fontHeight, new CSlrString("Raster cross interior: "),
																				  NULL, tr, tg, tb, optionsColors, font, fontScale);
		menuItemScreenRasterCrossInteriorColorScheme->SetSelectedOption(4, false);
		menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterCrossInteriorColorScheme);
		
		menuItemScreenRasterCrossExteriorColorScheme = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Raster cross exterior: "),
																				  NULL, tr, tg, tb, optionsColors, font, fontScale);
		menuItemScreenRasterCrossExteriorColorScheme->SetSelectedOption(0, false);
		menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterCrossExteriorColorScheme);
		
		menuItemScreenRasterCrossTipColorScheme = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Raster cross tip: "),
																			 NULL, tr, tg, tb, optionsColors, font, fontScale);
		menuItemScreenRasterCrossTipColorScheme->SetSelectedOption(3, false);
		menuItemSubMenuUI->AddMenuItem(menuItemScreenRasterCrossTipColorScheme);
		
		//
		menuItemVicEditorForceReplaceColor = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Vic Editor always replace color: "),
																		NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemVicEditorForceReplaceColor->SetSelectedOption(c64SettingsVicEditorForceReplaceColor, false);
		menuItemSubMenuUI->AddMenuItem(menuItemVicEditorForceReplaceColor);
		//menuItemSubMenuVicEditor

	}
	//
	menuItemShowPositionsInHex = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Show positions in hex: "),
																	  NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemShowPositionsInHex->SetSelectedOption(c64SettingsShowPositionsInHex ? 1 : 0, false);
	menuItemSubMenuUI->AddMenuItem(menuItemShowPositionsInHex);

	
	
	//
	menuItemDisassemblyExecuteColor = new CViewC64MenuItemOption(fontHeight, new CSlrString("Disassembly execute color: "),
																	NULL, tr, tg, tb, optionsColors, font, fontScale);
	menuItemDisassemblyExecuteColor->SetSelectedOption(C64D_COLOR_WHITE, false);
	menuItemSubMenuUI->AddMenuItem(menuItemDisassemblyExecuteColor);
	
	menuItemDisassemblyNonExecuteColor = new CViewC64MenuItemOption(fontHeight, new CSlrString("Disassembly non execute color: "),
																	NULL, tr, tg, tb, optionsColors, font, fontScale);
	menuItemDisassemblyNonExecuteColor->SetSelectedOption(C64D_COLOR_LIGHT_GRAY, false);
	menuItemSubMenuUI->AddMenuItem(menuItemDisassemblyNonExecuteColor);

	menuItemDisassemblyBackgroundColor = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Disassembly background color: "),
																	NULL, tr, tg, tb, optionsColors, font, fontScale);
	menuItemDisassemblyBackgroundColor->SetSelectedOption(C64D_COLOR_BLACK, false);
	menuItemSubMenuUI->AddMenuItem(menuItemDisassemblyBackgroundColor);
	
	
	if (viewC64->debugInterfaceC64)
	{
		//
		options = new std::vector<CSlrString *>();
		C64GetAvailablePalettes(options);
		menuItemVicPalette = new CViewC64MenuItemOption(fontHeight, new CSlrString("VIC palette: "),
														NULL, tr, tg, tb, options, font, fontScale);
		menuItemSubMenuUI->AddMenuItem(menuItemVicPalette);
		
	}
	
	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("Billinear"));
	options->push_back(new CSlrString("Nearest"));
	
	menuItemRenderScreenInterpolation = new CViewC64MenuItemOption(fontHeight, new CSlrString("Screen interpolation: "),
															 NULL, tr, tg, tb, options, font, fontScale);
	menuItemRenderScreenInterpolation->SetSelectedOption(c64SettingsRenderScreenNearest, false);
	menuItemSubMenuUI->AddMenuItem(menuItemRenderScreenInterpolation);

	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("1"));
	options->push_back(new CSlrString("2"));
	options->push_back(new CSlrString("4"));
	options->push_back(new CSlrString("8"));
	options->push_back(new CSlrString("16"));
	
	menuItemRenderScreenSupersample = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Super sampling factor: "),
																   NULL, tr, tg, tb, options, font, fontScale);
	
	// TODO: make generic selector for fixed values
	switch(c64SettingsScreenSupersampleFactor)
	{
		case 1:
			menuItemRenderScreenSupersample->SetSelectedOption(0, false);
			break;
		case 2:
			menuItemRenderScreenSupersample->SetSelectedOption(1, false);
			break;
		case 4:
			menuItemRenderScreenSupersample->SetSelectedOption(2, false);
			break;
		case 8:
			menuItemRenderScreenSupersample->SetSelectedOption(3, false);
			break;
		case 16:
			menuItemRenderScreenSupersample->SetSelectedOption(4, false);
			break;
	}
	
	menuItemSubMenuUI->AddMenuItem(menuItemRenderScreenSupersample);

	
#if !defined(WIN32)
	menuItemUseSystemDialogs = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Use system dialogs: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemUseSystemDialogs->SetSelectedOption(c64SettingsUseSystemFileDialogs, false);
	menuItemSubMenuUI->AddMenuItem(menuItemUseSystemDialogs);
#endif
	
#if defined(WIN32)
	menuItemUseOnlyFirstCPU = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Use only first CPU: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemUseOnlyFirstCPU->SetSelectedOption(c64SettingsUseOnlyFirstCPU, false);
	if (uilib_cpu_is_smp() == 1)
	{
		menuItemSubMenuUI->AddMenuItem(menuItemUseOnlyFirstCPU);
	}
#endif
	
	
	std::vector<CSlrString *> *colorThemeOptions = viewC64->colorsTheme->GetAvailableColorThemes();
	menuItemMenusColorTheme = new CViewC64MenuItemOption(fontHeight, new CSlrString("Menus color theme: "),
														 NULL, tr, tg, tb, colorThemeOptions, font, fontScale);
	menuItemMenusColorTheme->SetSelectedOption(c64SettingsMenusColorTheme, false);
	menuItemSubMenuUI->AddMenuItem(menuItemMenusColorTheme);
	
	//
	menuItemFocusBorderLineWidth = new CViewC64MenuItemFloat(fontHeight*2, new CSlrString("Focus border line width: "),
															 NULL, tr, tg, tb,
															 0.0f, 5.0f, 0.05f, font, fontScale);
	menuItemFocusBorderLineWidth->SetValue(0.7f, false);
	menuItemSubMenuUI->AddMenuItem(menuItemFocusBorderLineWidth);


	//
	menuItemPaintGridShowZoomLevel = new CViewC64MenuItemFloat(fontHeight*2, new CSlrString("Show paint grid from zoom level: "),
															   NULL, tr, tg, tb,
															   1.0f, 50.0f, 0.05f, font, fontScale);
	menuItemPaintGridShowZoomLevel->SetValue(c64SettingsPaintGridShowZoomLevel, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridShowZoomLevel);
	

	menuItemPaintGridCharactersColorR = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid characters Color R: "),
																  NULL, tr, tg, tb,
																  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridCharactersColorR->SetValue(c64SettingsPaintGridCharactersColorR, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridCharactersColorR);
	
	menuItemPaintGridCharactersColorG = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid characters Color G: "),
																  NULL, tr, tg, tb,
																  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridCharactersColorG->SetValue(c64SettingsPaintGridCharactersColorG, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridCharactersColorG);
	
	menuItemPaintGridCharactersColorB = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid characters Color B: "),
																  NULL, tr, tg, tb,
																  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridCharactersColorB->SetValue(c64SettingsPaintGridCharactersColorB, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridCharactersColorB);
	
	menuItemPaintGridCharactersColorA = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid characters Color A: "),
																  NULL, tr, tg, tb,
																  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridCharactersColorA->SetValue(c64SettingsPaintGridCharactersColorA, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridCharactersColorA);
	
	menuItemPaintGridPixelsColorR = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid pixels Color R: "),
															  NULL, tr, tg, tb,
															  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridPixelsColorR->SetValue(c64SettingsPaintGridPixelsColorR, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridPixelsColorR);
	
	menuItemPaintGridPixelsColorG = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid pixels Color G: "),
															  NULL, tr, tg, tb,
															  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridPixelsColorG->SetValue(c64SettingsPaintGridPixelsColorG, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridPixelsColorG);
	
	menuItemPaintGridPixelsColorB = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Paint grid pixels Color B: "),
															  NULL, tr, tg, tb,
															  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridPixelsColorB->SetValue(c64SettingsPaintGridPixelsColorB, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridPixelsColorB);
	
	menuItemPaintGridPixelsColorA = new CViewC64MenuItemFloat(fontHeight*2, new CSlrString("Paint grid pixels Color A: "),
															  NULL, tr, tg, tb,
															  0.0f, 1.0f, 0.05f, font, fontScale);
	menuItemPaintGridPixelsColorA->SetValue(c64SettingsPaintGridPixelsColorA, false);
	menuItemSubMenuUI->AddMenuItem(menuItemPaintGridPixelsColorA);

	//
#if defined(WIN32)
	menuItemIsProcessPriorityBoostDisabled = new CViewC64MenuItemOption(fontHeight, new CSlrString("Disable priority boost: "),
														  NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemIsProcessPriorityBoostDisabled->SetSelectedOption(c64SettingsIsProcessPriorityBoostDisabled, false);
	menuItemSubMenuUI->AddMenuItem(menuItemIsProcessPriorityBoostDisabled);

	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("Idle"));
	options->push_back(new CSlrString("Below normal"));
	options->push_back(new CSlrString("Normal"));
	options->push_back(new CSlrString("Above normal"));
	options->push_back(new CSlrString("High priority"));

	menuItemProcessPriority = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Process priority: "),
														  NULL, tr, tg, tb, options, font, fontScale);
	menuItemProcessPriority->SetSelectedOption(c64SettingsProcessPriority, false);
	menuItemSubMenuUI->AddMenuItem(menuItemProcessPriority);
#endif

	//
	if (viewC64->debugInterfaceC64)
	{
		// tape menu
		kbsTapeAttach = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Attach", 't', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeAttach);
		
		menuItemTapeAttach = new CViewC64MenuItem(fontHeight, new CSlrString("Attach Tape"),
													 kbsTapeAttach, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeAttach);
		
		// TODO: add showing path when tape is attached
		
		kbsTapeDetach = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Detach", 't', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeDetach);

		menuItemTapeDetach = new CViewC64MenuItem(fontHeight*2, new CSlrString("Detach Tape"),
													 kbsTapeDetach, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeDetach);
		
		menuItemTapeCreate = new CViewC64MenuItem(fontHeight*2, new CSlrString("Create Tape"),
													 NULL, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeCreate);
		
		//

		kbsTapeStop = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Stop", 's', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeStop);

		menuItemTapeStop = new CViewC64MenuItem(fontHeight, new CSlrString("Stop"),
													 kbsTapeStop, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeStop);

		kbsTapePlay = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Play", 'p', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapePlay);

		menuItemTapePlay = new CViewC64MenuItem(fontHeight, new CSlrString("Play"),
											kbsTapePlay, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapePlay);
		
		kbsTapeForward = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Forward", 'f', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeForward);

		menuItemTapeForward = new CViewC64MenuItem(fontHeight, new CSlrString("Forward"),
											kbsTapeForward, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeForward);
		
		kbsTapeRewind = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Rewind", 'r', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeRewind);

		menuItemTapeRewind = new CViewC64MenuItem(fontHeight, new CSlrString("Rewind"),
											kbsTapeRewind, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeRewind);
		
		kbsTapeRecord = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Tape Record", 'y', true, true, true, false, this);
		guiMain->AddKeyboardShortcut(kbsTapeRecord);

		menuItemTapeRecord = new CViewC64MenuItem(fontHeight*2, new CSlrString("Record"),
											kbsTapeRecord, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeRecord);
		
		menuItemTapeReset = new CViewC64MenuItem(fontHeight*2, new CSlrString("Reset Datasette"),
											NULL, tr, tg, tb);
		menuItemSubMenuTape->AddMenuItem(menuItemTapeReset);
		
		//
		menuItemDatasetteSpeedTuning = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Datasette speed tuning: "),
														NULL, tr, tg, tb,
														0.0f, 100.0f, 1.0f, font, fontScale);
		menuItemDatasetteSpeedTuning->numDecimalsDigits = 0;
		menuItemDatasetteSpeedTuning->SetValue(0.0f, false);
		menuItemSubMenuTape->AddMenuItem(menuItemDatasetteSpeedTuning);
		
		menuItemDatasetteZeroGapDelay = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Datasette zero-gap delay: "),
																 NULL, tr, tg, tb,
																 0.0f, 50000.0f, 100.0f, font, fontScale);
		menuItemDatasetteZeroGapDelay->numDecimalsDigits = 0;
		menuItemDatasetteZeroGapDelay->SetValue(20000.0f, false);
		menuItemSubMenuTape->AddMenuItem(menuItemDatasetteZeroGapDelay);

		menuItemDatasetteTapeWobble = new CViewC64MenuItemFloat(fontHeight, new CSlrString("Datasette tape wobble: "),
																 NULL, tr, tg, tb,
																 0.0f, 100.0f, 1.0f, font, fontScale);
		menuItemDatasetteTapeWobble->numDecimalsDigits = 0;
		menuItemDatasetteTapeWobble->SetValue(10.0f, false);
		menuItemSubMenuTape->AddMenuItem(menuItemDatasetteTapeWobble);

		
		menuItemDatasetteResetWithCPU = new CViewC64MenuItemOption(fontHeight, new CSlrString("Datasette reset with CPU: "),
																   NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemSubMenuTape->AddMenuItem(menuItemDatasetteResetWithCPU);

		/// REU
		menuItemReuEnabled = new CViewC64MenuItemOption(fontHeight, new CSlrString("REU Enabled: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemSubMenuReu->AddMenuItem(menuItemReuEnabled);

		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("128"));
		options->push_back(new CSlrString("256"));
		options->push_back(new CSlrString("512"));
		options->push_back(new CSlrString("1024"));
		options->push_back(new CSlrString("2048"));
		options->push_back(new CSlrString("4096"));
		options->push_back(new CSlrString("8192"));
		options->push_back(new CSlrString("16384"));

		menuItemReuSize = new CViewC64MenuItemOption(fontHeight, new CSlrString("REU Size: "),
													 NULL, tr, tg, tb, options, font, fontScale);
		
		menuItemSubMenuReu->AddMenuItem(menuItemReuSize);

		// REU menu
//		kbsReuAttach = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Attach REU", 'r', true, false, true);
//		viewC64->keyboardShortcuts->AddShortcut(kbsReuAttach);
		menuItemReuAttach = new CViewC64MenuItem(fontHeight*3, new CSlrString("Attach REU"),
													 NULL, tr, tg, tb);
		menuItemSubMenuReu->AddMenuItem(menuItemReuAttach);
		
		// TODO: add showing path when reu is attached
//		kbsReuDetach = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Save REU", 'i', true, true, true);
//		viewC64->keyboardShortcuts->AddShortcut(kbsReuDetach);
		
		menuItemReuSave = new CViewC64MenuItem(fontHeight, new CSlrString("Save REU"),
													 NULL, tr, tg, tb);
		menuItemSubMenuReu->AddMenuItem(menuItemReuSave);
		
	}
	
	//
	
	if (viewC64->debugInterfaceC64)
	{
		//
		// memory mapping can be initialised only on startup
		menuItemMapC64MemoryToFile = new CViewC64MenuItem(fontHeight*3, NULL,
														  NULL, tr, tg, tb);
		menuItemSubMenuMemory->AddMenuItem(menuItemMapC64MemoryToFile);
		
		UpdateMapC64MemoryToFileLabels();
		
		///
		kbsDumpC64Memory = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Dump C64 memory", 'u', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsDumpC64Memory);
		
		menuItemDumpC64Memory = new CViewC64MenuItem(fontHeight, new CSlrString("Dump C64 memory"),
													 kbsDumpC64Memory, tr, tg, tb);
		menuItemSubMenuMemory->AddMenuItem(menuItemDumpC64Memory);
		
		menuItemDumpC64MemoryMarkers = new CViewC64MenuItem(fontHeight, new CSlrString("Dump C64 memory markers"),
															NULL, tr, tg, tb);
		menuItemSubMenuMemory->AddMenuItem(menuItemDumpC64MemoryMarkers);
		
		kbsDumpDrive1541Memory = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Dump Drive 1541 memory", 'u', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsDumpDrive1541Memory);
		
		menuItemDumpDrive1541Memory = new CViewC64MenuItem(fontHeight, new CSlrString("Dump Disk 1541 memory"),
														   kbsDumpDrive1541Memory, tr, tg, tb);
		menuItemSubMenuMemory->AddMenuItem(menuItemDumpDrive1541Memory);
		
		menuItemDumpDrive1541MemoryMarkers = new CViewC64MenuItem(fontHeight*2, new CSlrString("Dump Disk 1541 memory markers"),
																  NULL, tr, tg, tb);
		menuItemSubMenuMemory->AddMenuItem(menuItemDumpDrive1541MemoryMarkers);
	}

	//
	
	
	//
//	options = new std::vector<CSlrString *>();
//	options->push_back(new CSlrString("No"));
//	options->push_back(new CSlrString("Yes"));

	kbsUseKeboardAsJoystick = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Use keyboard as joystick", 'y', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsUseKeboardAsJoystick);
	menuItemUseKeyboardAsJoystick = new CViewC64MenuItemOption(fontHeight, new CSlrString("Use keyboard as joystick: "),
															   kbsUseKeboardAsJoystick, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemSubMenuEmulation->AddMenuItem(menuItemUseKeyboardAsJoystick);
	///
	
	options = new std::vector<CSlrString *>();
	options->push_back(new CSlrString("both"));
	options->push_back(new CSlrString("1"));
	options->push_back(new CSlrString("2"));
	menuItemJoystickPort = new CViewC64MenuItemOption(fontHeight, new CSlrString("Joystick port: "),
													  NULL, tr, tg, tb, options, font, fontScale);
	menuItemSubMenuEmulation->AddMenuItem(menuItemJoystickPort);
	
//	//
//	kbsIsWarpSpeed = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Warp speed", 'p', false, false, true, false, this);
//	guiMain->AddKeyboardShortcut(kbsIsWarpSpeed);
//	options = new std::vector<CSlrString *>();
//	options->push_back(new CSlrString("Off"));
//	options->push_back(new CSlrString("On"));
//	menuItemIsWarpSpeed = new CViewC64MenuItemOption(fontHeight, new CSlrString("Warp Speed: "), kbsIsWarpSpeed, tr, tg, tb, options, font, fontScale);
//	menuItemSubMenuEmulation->AddMenuItem(menuItemIsWarpSpeed);
	

	

	if (viewC64->debugInterfaceC64)
	{
		kbsCartridgeFreezeButton = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Cartridge freeze", 'f', false, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsCartridgeFreezeButton);
		menuItemCartridgeFreeze = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("Cartridge freeze"),
													   kbsCartridgeFreezeButton, tr, tg, tb);
		menuItemSubMenuEmulation->AddMenuItem(menuItemCartridgeFreeze);
		
		// champ profiler output
		isProfilingC64 = false;
		menuItemC64ProfilerFilePath = new CViewC64MenuItem(fontHeight*2, NULL,
															  NULL, tr, tg, tb);
		menuItemSubMenuEmulation->AddMenuItem(menuItemC64ProfilerFilePath);
		
		UpdateC64ProfilerFilePath();
		
		menuItemC64ProfilerDoVic = new CViewC64MenuItemOption(fontHeight, new CSlrString("Perform VIC profiling: "),
																			 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemC64ProfilerDoVic->SetSelectedOption(c64SettingsC64ProfilerDoVicProfile, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemC64ProfilerDoVic);

		kbsC64ProfilerStartStop = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Start/Stop profiling C64", 'i', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsC64ProfilerStartStop);
		menuItemC64ProfilerStartStop = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("Start profiling C64"),
													   kbsC64ProfilerStartStop, tr, tg, tb);
		menuItemSubMenuEmulation->AddMenuItem(menuItemC64ProfilerStartStop);

		
		
		//
		//
		menuItemAutoJmp = new CViewC64MenuItemOption(fontHeight, new CSlrString("Auto JMP: "),
													 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemAutoJmp->SetSelectedOption(c64SettingsAutoJmp, false);
		//menuItemSubMenuEmulation->AddMenuItem(menuItemAutoJmp);
		
		//
		kbsAutoJmpAlwaysToLoadedPRGAddress = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Always JMP to loaded addr", 'j', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsAutoJmpAlwaysToLoadedPRGAddress);

		menuItemAutoJmpAlwaysToLoadedPRGAddress = new CViewC64MenuItemOption(fontHeight, new CSlrString("Always JMP to loaded addr: "),
																			 kbsAutoJmpAlwaysToLoadedPRGAddress, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemAutoJmpAlwaysToLoadedPRGAddress->SetSelectedOption(c64SettingsAutoJmpAlwaysToLoadedPRGAddress, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAutoJmpAlwaysToLoadedPRGAddress);
		
		
		//
		kbsAutoJmpFromInsertedDiskFirstPrg = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Auto load first PRG from D64", 'a', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsAutoJmpFromInsertedDiskFirstPrg);
		menuItemAutoJmpFromInsertedDiskFirstPrg = new CViewC64MenuItemOption(fontHeight, new CSlrString("Load first PRG from D64: "),
																			 kbsAutoJmpFromInsertedDiskFirstPrg, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemAutoJmpFromInsertedDiskFirstPrg->SetSelectedOption(c64SettingsAutoJmpFromInsertedDiskFirstPrg, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAutoJmpFromInsertedDiskFirstPrg);
		
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("No"));
		options->push_back(new CSlrString("Soft"));
		options->push_back(new CSlrString("Hard"));

		kbsAutoJmpDoReset = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Reset C64 before PRG load", 'h', true, false, true, false, this);
		guiMain->AddKeyboardShortcut(kbsAutoJmpDoReset);

		menuItemAutoJmpDoReset = new CViewC64MenuItemOption(fontHeight, new CSlrString("Reset C64 before PRG load: "),
													  kbsAutoJmpDoReset, tr, tg, tb, options, font, fontScale);
		menuItemAutoJmpDoReset->SetSelectedOption(c64SettingsAutoJmpDoReset, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAutoJmpDoReset);
		
		
		menuItemAutoJmpWaitAfterReset = new CViewC64MenuItemFloat(fontHeight * 2.0f, new CSlrString("Wait after Reset: "),
																  NULL, tr, tg, tb,
																  0.0f, 5000.0f, 10.00f, font, fontScale);
		menuItemAutoJmpWaitAfterReset->SetValue(c64SettingsAutoJmpWaitAfterReset, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemAutoJmpWaitAfterReset);
		
		//
#if defined(RUN_COMMODORE64)
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("C64 Debugger"));
		options->push_back(new CSlrString("VICE"));

		menuItemUseNativeEmulatorMonitor = new CViewC64MenuItemOption(fontHeight, new CSlrString("Monitor: "),
													 NULL, tr, tg, tb, options, font, fontScale);
		menuItemUseNativeEmulatorMonitor->SetSelectedOption(c64SettingsUseNativeEmulatorMonitor, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemUseNativeEmulatorMonitor);
#endif
		
		///
		//	options = new std::vector<CSlrString *>();
		//	options->push_back(new CSlrString("No"));
		//	options->push_back(new CSlrString("Yes"));
		
		menuItemFastBootKernalPatch = new CViewC64MenuItemOption(fontHeight, new CSlrString("Fast boot kernal patch: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemFastBootKernalPatch->SetSelectedOption(c64SettingsFastBootKernalPatch, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemFastBootKernalPatch);

		menuItemEmulateVSPBug = new CViewC64MenuItemOption(fontHeight, new CSlrString("Emulate VSP bug: "),
														   NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemEmulateVSPBug->SetSelectedOption(c64SettingsEmulateVSPBug, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemEmulateVSPBug);

		menuItemVicSetSkipDrawingSprites = new CViewC64MenuItemOption(fontHeight, new CSlrString("Skip drawing sprites: "),
														   NULL, tr, tg, tb, optionsYesNo, font, fontScale);
		menuItemVicSetSkipDrawingSprites->SetSelectedOption(c64SettingsVicSkipDrawingSprites, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemVicSetSkipDrawingSprites);

		//
		options = new std::vector<CSlrString *>();
		options->push_back(new CSlrString("None"));
		options->push_back(new CSlrString("Each raster line"));
		options->push_back(new CSlrString("Each VIC cycle"));
		menuItemVicStateRecordingMode = new CViewC64MenuItemOption(fontHeight, new CSlrString("VIC Display recording: "),
																   NULL, tr, tg, tb, options, font, fontScale);
		menuItemVicStateRecordingMode->SetSelectedOption(c64SettingsVicStateRecordingMode, false);
		menuItemSubMenuEmulation->AddMenuItem(menuItemVicStateRecordingMode);
		
		
	}
	
	//
	
	
	menuItemDisassembleExecuteAware = new CViewC64MenuItemOption(fontHeight, new CSlrString("Execute-aware disassemble: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemDisassembleExecuteAware->SetSelectedOption(c64SettingsRenderDisassembleExecuteAware, false);
	menuItemSubMenuEmulation->AddMenuItem(menuItemDisassembleExecuteAware);
	

	//
	menuItemStartJukeboxPlaylist = new CViewC64MenuItem(fontHeight*2, new CSlrString("Start JukeBox playlist"),
														NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemStartJukeboxPlaylist);
	
	if (viewC64->debugInterfaceC64)
	{
		menuItemSetC64KeyboardMapping = new CViewC64MenuItem(fontHeight, new CSlrString("Set C64 keyboard mapping"),
															 NULL, tr, tg, tb);
		viewMenu->AddMenuItem(menuItemSetC64KeyboardMapping);		
	}
	
	menuItemSetKeyboardShortcuts = new CViewC64MenuItem(fontHeight*2, new CSlrString("Set keyboard shortcuts"),
														NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemSetKeyboardShortcuts);
	
	


	float d = 1.25f;//0.75f;
	menuItemClearSettings = new CViewC64MenuItem(fontHeight*d, new CSlrString("Clear settings to factory defaults"),
															 NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemClearSettings);

}

bool CViewSettingsMenu::ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *shortcut)
{
	LOGD("CViewSettingsMenu::ProcessKeyboardShortcut");
	
	if (shortcut == kbsDumpC64Memory)
	{
		viewC64->viewC64SettingsMenu->OpenDialogDumpC64Memory();
		return true;
	}
	else if (shortcut == kbsDumpDrive1541Memory)
	{
		viewC64->viewC64SettingsMenu->OpenDialogDumpDrive1541Memory();
		return true;
	}
	else if (shortcut == kbsTapeAttach)
	{
		viewC64->viewC64MainMenu->OpenDialogInsertTape();
		return true;
	}
	else if (shortcut == kbsTapeDetach)
	{
		viewC64->debugInterfaceC64->DetachTape();
		guiMain->ShowMessage("Tape detached");
		return true;
	}
	else if (shortcut == kbsTapeStop)
	{
		viewC64->debugInterfaceC64->DatasetteStop();
		guiMain->ShowMessage("Datasette STOP");
		return true;
	}
	else if (shortcut == kbsTapePlay)
	{
		viewC64->debugInterfaceC64->DatasettePlay();
		guiMain->ShowMessage("Datasette PLAY");
		return true;
	}
	else if (shortcut == kbsTapeForward)
	{
		viewC64->debugInterfaceC64->DatasetteForward();
		guiMain->ShowMessage("Datasette FORWARD");
		return true;
	}
	else if (shortcut == kbsTapeRewind)
	{
		viewC64->debugInterfaceC64->DatasetteRewind();
		guiMain->ShowMessage("Datasette REWIND");
		return true;
	}
//		else if (shortcut == viewC64SettingsMenu->kbsTapeReset)
//		{
//			viewC64->debugInterfaceC64->DatasetteReset();
//			return true;
//		}
	
	else if (shortcut == kbsSwitchNextMaximumSpeed)
	{
		SwitchNextMaximumSpeed();
		return true;
	}
	else if (shortcut == kbsSwitchPrevMaximumSpeed)
	{
		SwitchPrevMaximumSpeed();
		return true;
	}
//	else if (shortcut == kbsIsWarpSpeed)
//	{
//		viewC64->SwitchIsWarpSpeed();
//		return true;
//	}
	else if (shortcut == kbsUseKeboardAsJoystick)
	{
//		viewC64->SwitchUseKeyboardAsJoystick();
		return true;
	}
	else if (shortcut == kbsCartridgeFreezeButton)
	{
		viewC64->debugInterfaceC64->CartridgeFreezeButtonPressed();
		return true;
	}
	else if (shortcut == kbsC64ProfilerStartStop)
	{
		C64ProfilerStartStop();
		return true;
	}
	else if (shortcut == kbsDetachEverything)
	{
		DetachEverything(true, true);
		return true;
	}
	else if (shortcut == kbsDetachCartridge)
	{
		DetachCartridge(true);
		return true;
	}
	else if (shortcut == kbsDetachDiskImage)
	{
		DetachDiskImage();
		return true;
	}
	else if (shortcut == kbsAutoJmpFromInsertedDiskFirstPrg)
	{
		ToggleAutoLoadFromInsertedDisk();
		return true;
	}
	else if (shortcut == kbsAutoJmpAlwaysToLoadedPRGAddress)
	{
		ToggleAutoJmpAlwaysToLoadedPRGAddress();
		return true;
	}
	else if (shortcut == kbsAutoJmpDoReset)
	{
		ToggleAutoJmpDoReset();
		return true;
	}
	else if (shortcut == kbsSwitchSoundOnOff)
	{
		viewC64->ToggleSoundMute();
		return true;
	}
	return false;
}


void CViewSettingsMenu::ToggleAutoLoadFromInsertedDisk()
{
	if (c64SettingsAutoJmpFromInsertedDiskFirstPrg)
	{
		menuItemAutoJmpFromInsertedDiskFirstPrg->SetSelectedOption(0, true);
		guiMain->ShowMessage("Auto load from disk is OFF");
	}
	else
	{
		menuItemAutoJmpFromInsertedDiskFirstPrg->SetSelectedOption(1, true);
		guiMain->ShowMessage("Auto load from disk is ON");
	}
}

void CViewSettingsMenu::ToggleAutoJmpAlwaysToLoadedPRGAddress()
{
	if (c64SettingsAutoJmpAlwaysToLoadedPRGAddress)
	{
		menuItemAutoJmpAlwaysToLoadedPRGAddress->SetSelectedOption(0, true);
		guiMain->ShowMessage("Auto JMP to loaded address is OFF");
	}
	else
	{
		menuItemAutoJmpAlwaysToLoadedPRGAddress->SetSelectedOption(1, true);
		guiMain->ShowMessage("Auto JMP to loaded address is ON");
	}
}

void CViewSettingsMenu::ToggleAutoJmpDoReset()
{
	if (c64SettingsAutoJmpDoReset == MACHINE_RESET_HARD)
	{
		menuItemAutoJmpDoReset->SetSelectedOption(0, true);
		guiMain->ShowMessage("Do not Reset before PRG load");
	}
	else if (c64SettingsAutoJmpDoReset == MACHINE_RESET_NONE)
	{
		menuItemAutoJmpDoReset->SetSelectedOption(1, true);
		guiMain->ShowMessage("Soft Reset before PRG load");
	}
	else if (c64SettingsAutoJmpDoReset == MACHINE_RESET_SOFT)
	{
		menuItemAutoJmpDoReset->SetSelectedOption(2, true);
		guiMain->ShowMessage("Hard Reset before PRG load");
	}
}

void CViewSettingsMenu::UpdateMapC64MemoryToFileLabels()
{
	guiMain->LockMutex();

	if (c64SettingsPathToC64MemoryMapFile == NULL)
	{
		menuItemMapC64MemoryToFile->SetString(new CSlrString("Map C64 memory to a file"));
		if (menuItemMapC64MemoryToFile->str2 != NULL)
			delete menuItemMapC64MemoryToFile->str2;
		menuItemMapC64MemoryToFile->str2 = NULL;
	}
	else
	{
		menuItemMapC64MemoryToFile->SetString(new CSlrString("Unmap C64 memory from file"));
		
		char *asciiPath = c64SettingsPathToC64MemoryMapFile->GetStdASCII();
		
		// display file name in menu
		char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
		
		if (menuItemMapC64MemoryToFile->str2 != NULL)
			delete menuItemMapC64MemoryToFile->str2;
		
		menuItemMapC64MemoryToFile->str2 = new CSlrString(fname);
		delete [] fname;
	}
	guiMain->UnlockMutex();
}

void CViewSettingsMenu::UpdateC64ProfilerFilePath()
{
	guiMain->LockMutex();
	
	if (c64SettingsC64ProfilerFileOutputPath == NULL)
	{
		menuItemC64ProfilerFilePath->SetString(new CSlrString("Set C64 profiler file"));
		if (menuItemC64ProfilerFilePath->str2 != NULL)
		delete menuItemC64ProfilerFilePath->str2;
		menuItemC64ProfilerFilePath->str2 = NULL;
	}
	else
	{
		menuItemC64ProfilerFilePath->SetString(new CSlrString("Set C64 profiler file:"));
		
		char *asciiPath = c64SettingsC64ProfilerFileOutputPath->GetStdASCII();
		
		// display file name in menu
		char *fname = SYS_GetFileNameWithExtensionFromFullPath(asciiPath);
		
		if (menuItemC64ProfilerFilePath->str2 != NULL)
			delete menuItemC64ProfilerFilePath->str2;
		
		menuItemC64ProfilerFilePath->str2 = new CSlrString(fname);
		delete [] fname;
	}
	
	guiMain->UnlockMutex();
}

void CViewSettingsMenu::UpdateAudioOutDevices()
{
	guiMain->LockMutex();
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->snapshotsManager->LockMutex();
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->snapshotsManager->LockMutex();
	}

	if (viewC64->debugInterfaceNes)
	{
		viewC64->debugInterfaceNes->snapshotsManager->LockMutex();
	}
	
	/*
	std::list<CSlrString *> *audioDevicesList = NULL;
	audioDevicesList = gSoundEngine->EnumerateAvailableOutputDevices();
	
	std::vector<CSlrString *> *audioDevices = new std::vector<CSlrString *>();
	for (std::list<CSlrString *>::iterator it = audioDevicesList->begin(); it != audioDevicesList->end(); it++)
	{
		CSlrString *str = *it;
		audioDevices->push_back(str);
	}
	delete audioDevicesList;
	
	menuItemAudioOutDevice->SetOptions(audioDevices);*/
	
	// TODO: gSoundEngine->audioOutDeviceName;
	/*
	LOGD("CViewSettingsMenu::UpdateAudioOutDevices: selected AudioOut device=%s", gSoundEngine->audioOutDeviceName);

	CSlrString *deviceOutNameStr = gSoundEngine->audioOutDeviceName;
	
	int i = 0;
	for (std::vector<CSlrString *>::iterator it = audioDevices->begin(); it != audioDevices->end(); it++)
	{
		CSlrString *str = *it;
		if (deviceOutNameStr->CompareWith(str))
		{
			menuItemAudioOutDevice->SetSelectedOption(i, false);
			break;
		}
		
		i++;
	}
	 */
	
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->snapshotsManager->UnlockMutex();
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->snapshotsManager->UnlockMutex();
	}
	
	if (viewC64->debugInterfaceNes)
	{
		viewC64->debugInterfaceNes->snapshotsManager->UnlockMutex();
	}
	guiMain->UnlockMutex();
}

CViewSettingsMenu::~CViewSettingsMenu()
{
}

void CViewSettingsMenu::MenuCallbackItemChanged(CGuiViewMenuItem *menuItem)
{
	if (menuItem == menuItemIsWarpSpeed)
	{
		if (menuItemIsWarpSpeed->selectedOption == 0)
		{
			// TODO: generalize and iterate over base class
			if (viewC64->debugInterfaceC64 != NULL)
			{
				viewC64->debugInterfaceC64->SetSettingIsWarpSpeed(false);
			}
			
			if (viewC64->debugInterfaceAtari != NULL)
			{
				viewC64->debugInterfaceAtari->SetSettingIsWarpSpeed(false);
			}

			if (viewC64->debugInterfaceNes != NULL)
			{
				viewC64->debugInterfaceNes->SetSettingIsWarpSpeed(false);
			}
		}
		else
		{
			// TODO: generalize and iterate over base class
			if (viewC64->debugInterfaceC64 != NULL)
			{
				viewC64->debugInterfaceC64->SetSettingIsWarpSpeed(true);
			}
			
			if (viewC64->debugInterfaceAtari != NULL)
			{
				viewC64->debugInterfaceAtari->SetSettingIsWarpSpeed(true);
			}

			if (viewC64->debugInterfaceNes != NULL)
			{
				viewC64->debugInterfaceNes->SetSettingIsWarpSpeed(true);
			}
		}
	}
	else if (menuItem == menuItemUseKeyboardAsJoystick)
	{
//		if (menuItemUseKeyboardAsJoystick->selectedOption == 0)
//		{
//			c64SettingsUseKeyboardAsJoystick = false;
//			guiMain->ShowMessage("Joystick is OFF");
//		}
//		else
//		{
//			c64SettingsUseKeyboardAsJoystick = true;
//			guiMain->ShowMessage("Joystick is ON");
//		}
//		C64DebuggerStoreSettings();
	}
	else if (menuItem == menuItemJoystickPort)
	{
		C64DebuggerSetSetting("JoystickPort", &(menuItemJoystickPort->selectedOption));
	}
	else if (menuItem == menuItemVicPalette)
	{
		C64DebuggerSetSetting("VicPalette", &(menuItemVicPalette->selectedOption));
	}
	else if (menuItem == menuItemRenderScreenInterpolation)
	{
		bool v = menuItemRenderScreenInterpolation->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("RenderScreenNearest", &(v));
	}
	else if (menuItem == menuItemRenderScreenSupersample)
	{
		int v = 0;
		switch(menuItemRenderScreenSupersample->selectedOption)
		{
			default:
			case 0:
				v = 1;
				break;
			case 1:
				v = 2;
				break;
			case 2:
				v = 4;
				break;
			case 3:
				v = 8;
				break;
			case 4:
				v = 16;
				break;
		}
		C64DebuggerSetSetting("ScreenSupersampleFactor", &(v));
	}
	else if (menuItem == menuItemSIDModel)
	{
		C64DebuggerSetSetting("SIDEngineModel", &(menuItemSIDModel->selectedOption));
	}
	else if (menuItem == menuItemRESIDSamplingMethod)
	{
		C64DebuggerSetSetting("RESIDSamplingMethod", &(menuItemRESIDSamplingMethod->selectedOption));
	}
	else if (menuItem == menuItemRESIDEmulateFilters)
	{
		C64DebuggerSetSetting("RESIDEmulateFilters", &(menuItemRESIDEmulateFilters->selectedOption));
	}
	else if (menuItem == menuItemRESIDPassBand)
	{
		i32 v = (i32)(menuItemRESIDPassBand->value);
		C64DebuggerSetSetting("RESIDPassBand", &v);
	}
	else if (menuItem == menuItemRESIDFilterBias)
	{
		i32 v = (i32)(menuItemRESIDFilterBias->value);
		C64DebuggerSetSetting("RESIDFilterBias", &v);
	}
	else if (menuItem == menuItemSIDHistoryMaxSize)
	{
		i32 v = (i32)(menuItemSIDHistoryMaxSize->value);
		C64DebuggerSetSetting("SIDDataHistoryMaxSize", &v);
	}
	else if (menuItem == menuItemSIDStereo)
	{
		C64DebuggerSetSetting("SIDStereo", &(menuItemSIDStereo->selectedOption));
	}
	else if (menuItem == menuItemSIDStereoAddress)
	{
		uint16 addr = GetSidAddressFromOptionNum(menuItemSIDStereoAddress->selectedOption);
		C64DebuggerSetSetting("SIDStereoAddress", &addr);
	}
	else if (menuItem == menuItemSIDTripleAddress)
	{
		uint16 addr = GetSidAddressFromOptionNum(menuItemSIDTripleAddress->selectedOption);
		C64DebuggerSetSetting("SIDTripleAddress", &addr);
	}
	else if (menuItem == menuItemMuteSIDOnPause)
	{
		bool v = menuItemMuteSIDOnPause->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("MuteSIDOnPause", &(v));
	}
	else if (menuItem == menuItemRunSIDWhenInWarp)
	{
		bool v = menuItemRunSIDWhenInWarp->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("RunSIDWhenWarp", &(v));
	}
	else if (menuItem == menuItemAudioVolume)
	{
		float v = menuItemAudioVolume->value;
		u16 vu16 = ((u16)v);
		C64DebuggerSetSetting("AudioVolume", &(vu16));
	}
	else if (menuItem == menuItemRunSIDEmulation)
	{
		bool v = menuItemRunSIDEmulation->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("RunSIDEmulation", &(v));
	}
	else if (menuItem == menuItemMuteSIDMode)
	{
		int v = menuItemMuteSIDMode->selectedOption;
		C64DebuggerSetSetting("MuteSIDMode", &(v));
	}
	else if (menuItem == menuItemSIDImportMode)
	{
		int v = menuItemSIDImportMode->selectedOption;
		C64DebuggerSetSetting("SIDImportMode", &(v));
	}
	else if (menuItem == menuItemRestartAudioOnEmulationReset)
	{
		bool v = menuItemRestartAudioOnEmulationReset->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("RestartAudioOnEmulationReset", &(v));
	}
	else if (menuItem == menuItemShowPositionsInHex)
	{
		bool v = menuItemShowPositionsInHex->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("ShowPositionsInHex", &(v));
	}

	else if (menuItem == menuItemAtariPokeyStereo)
	{
		bool v = menuItemAtariPokeyStereo->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("AtariPokeyStereo", &v);
	}

	else if (menuItem == menuItemDatasetteSpeedTuning)
	{
		i32 v = (i32)(menuItemDatasetteSpeedTuning->value);
		C64DebuggerSetSetting("DatasetteSpeedTuning", &v);
	}
	else if (menuItem == menuItemDatasetteZeroGapDelay)
	{
		i32 v = (i32)(menuItemDatasetteZeroGapDelay->value);
		C64DebuggerSetSetting("DatasetteZeroGapDelay", &v);
	}
	else if (menuItem == menuItemDatasetteTapeWobble)
	{
		i32 v = (i32)(menuItemDatasetteTapeWobble->value);
		C64DebuggerSetSetting("DatasetteTapeWobble", &v);
	}
	else if (menuItem == menuItemDatasetteResetWithCPU)
	{
		bool v = menuItemDatasetteResetWithCPU->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("DatasetteResetWithCPU", &(v));
		
		viewC64->debugInterfaceC64->SetPatchKernalFastBoot(v);
	}
	else if (menuItem == menuItemFastBootKernalPatch)
	{
		bool v = menuItemFastBootKernalPatch->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("FastBootPatch", &(v));
		
		viewC64->debugInterfaceC64->SetPatchKernalFastBoot(v);
	}
	else if (menuItem == menuItemEmulateVSPBug)
	{
		bool v = menuItemEmulateVSPBug->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("EmulateVSPBug", &(v));
	}
	else if (menuItem == menuItemVicSetSkipDrawingSprites)
	{
		bool v = menuItemVicSetSkipDrawingSprites->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("VicSkipDrawingSprites", &(v));
	}
	else if (menuItem == menuItemReuEnabled)
	{
		bool v = menuItemReuEnabled->selectedOption == 0 ? false : true;
		
		LOGD("menuItemReuEnabled: %s", STRBOOL(v));
		C64DebuggerSetSetting("ReuEnabled", &(v));
	}
	else if (menuItem == menuItemReuSize)
	{
		int reuSize = settingsReuSizes[menuItemReuSize->selectedOption];
		C64DebuggerSetSetting("ReuSize", &(reuSize));
	}
	
	else if (menuItem == menuItemAutoJmp)
	{
		bool v = menuItemAutoJmp->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("AutoJmp", &(v));
	}
	else if (menuItem == menuItemAutoJmpAlwaysToLoadedPRGAddress)
	{
		bool v = menuItemAutoJmpAlwaysToLoadedPRGAddress->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("AutoJmpAlwaysToLoadedPRGAddress", &(v));
	}
	else if (menuItem == menuItemAutoJmpFromInsertedDiskFirstPrg)
	{
		bool v = menuItemAutoJmpFromInsertedDiskFirstPrg->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("AutoJmpFromInsertedDiskFirstPrg", &(v));
	}
	else if (menuItem == menuItemAutoJmpDoReset)
	{
		C64DebuggerSetSetting("AutoJmpDoReset", &(menuItemAutoJmpDoReset->selectedOption));
	}
	else if (menuItem == menuItemAutoJmpWaitAfterReset)
	{
		int v = menuItemAutoJmpWaitAfterReset->value;
		C64DebuggerSetSetting("AutoJmpWaitAfterReset", &v);
	}
	else if (menuItem == menuItemC64ProfilerDoVic)
	{
		bool v = menuItemC64ProfilerDoVic->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("C64ProfilerDoVic", &(v));
	}
	else if (menuItem == menuItemUseNativeEmulatorMonitor)
	{
		bool v = menuItemUseNativeEmulatorMonitor->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("UseNativeEmulatorMonitor", &(v));
		//viewC64->RefreshLayout();
		viewC64->viewC64MonitorConsole->PrintInitPrompt();
	}
	
	else if (menuItem == menuItemDisassembleExecuteAware)
	{
		bool v = menuItemDisassembleExecuteAware->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("DisassembleExecuteAware", &(v));
	}
	else if (menuItem == menuItemDisassemblyBackgroundColor)
	{
		int v = menuItemDisassemblyBackgroundColor->selectedOption;
		C64DebuggerSetSetting("DisassemblyBackgroundColor", &v);
	}
	else if (menuItem == menuItemDisassemblyExecuteColor)
	{
		int v = menuItemDisassemblyExecuteColor->selectedOption;
		C64DebuggerSetSetting("DisassemblyExecuteColor", &v);
	}
	else if (menuItem == menuItemDisassemblyNonExecuteColor)
	{
		int v = menuItemDisassemblyNonExecuteColor->selectedOption;
		C64DebuggerSetSetting("DisassemblyNonExecuteColor", &v);
	}
	else if (menuItem == menuItemMenusColorTheme)
	{
		int v = menuItemMenusColorTheme->selectedOption;
		C64DebuggerSetSetting("MenusColorTheme", &v);
	}
	else if (menuItem == menuItemWindowAlwaysOnTop)
	{
		bool v = menuItemWindowAlwaysOnTop->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("WindowAlwaysOnTop", &(v));
	}
	else if (menuItem == menuItemUseSystemDialogs)
	{
		bool v = menuItemUseSystemDialogs->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("UseSystemDialogs", &(v));
	}
	else if (menuItem == menuItemUseOnlyFirstCPU)
	{
		bool v = menuItemUseOnlyFirstCPU->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("UseOnlyFirstCPU", &(v));
		guiMain->ShowMessage("Please restart C64 Debugger to apply configuration.");
	}
	else if (menuItem == menuItemC64SnapshotsManagerIsActive)
	{
		bool v = menuItemC64SnapshotsManagerIsActive->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("SnapshotsManagerIsActive", &(v));
	}
	else if (menuItem == menuItemC64SnapshotsManagerStoreInterval)
	{
		i32 v = (i32)menuItemC64SnapshotsManagerStoreInterval->value;
		C64DebuggerSetSetting("SnapshotsManagerStoreInterval", &v);
	}
	else if (menuItem == menuItemC64SnapshotsManagerLimit)
	{
		i32 v = (i32)menuItemC64SnapshotsManagerLimit->value;
		C64DebuggerSetSetting("SnapshotsManagerLimit", &v);
	}
	else if (menuItem == menuItemC64TimelineIsActive)
	{
		bool v = menuItemC64TimelineIsActive->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("TimelineIsActive", &(v));
	}
	
	else if (menuItem == menuItemVicStateRecordingMode)
	{
		int sel = menuItemVicStateRecordingMode->selectedOption;
		C64DebuggerSetSetting("VicStateRecording", &sel);
	}
	else if (menuItem == menuItemAudioOutDevice)
	{
		CSlrString *deviceName = (*menuItemAudioOutDevice->options)[menuItemAudioOutDevice->selectedOption];
		C64DebuggerSetSetting("AudioOutDevice", deviceName);
	}
	else if (menuItem == menuItemC64Model)
	{
		int modelId = (*c64ModelTypeIds)[menuItemC64Model->selectedOption];
		C64DebuggerSetSetting("C64Model", &(modelId));
	}
	else if (menuItem == menuItemMemoryCellsColorStyle)
	{
		C64DebuggerSetSetting("MemoryValuesStyle", &(menuItemMemoryCellsColorStyle->selectedOption));
	}
	else if (menuItem == menuItemMemoryMarkersColorStyle)
	{
		C64DebuggerSetSetting("MemoryMarkersStyle", &(menuItemMemoryMarkersColorStyle->selectedOption));
	}
#if defined(MACOS)
	else if (menuItem == menuItemMultiTouchMemoryMap)
	{
		bool v = menuItemMultiTouchMemoryMap->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("MemMapMultiTouch", &(v));
	}
#endif
	else if (menuItem == menuItemMemoryMapInvert)
	{
		bool v = menuItemMemoryMapInvert->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("MemMapInvert", &(v));
	}
	else if (menuItem == menuItemMemoryMapRefreshRate)
	{
		int sel = menuItemMemoryMapRefreshRate->selectedOption;
		
		if (sel == 0)
		{
			int v = 1;
			C64DebuggerSetSetting("MemMapRefresh", &v);
		}
		else if (sel == 1)
		{
			int v = 2;
			C64DebuggerSetSetting("MemMapRefresh", &v);
		}
		else if (sel == 2)
		{
			int v = 4;
			C64DebuggerSetSetting("MemMapRefresh", &v);
		}
		else if (sel == 3)
		{
			int v = 10;
			C64DebuggerSetSetting("MemMapRefresh", &v);
		}
		else if (sel == 4)
		{
			int v = 20;
			C64DebuggerSetSetting("MemMapRefresh", &v);
		}
	}
	else if (menuItem == menuItemFocusBorderLineWidth)
	{
		float v = menuItemFocusBorderLineWidth->value;
		C64DebuggerSetSetting("FocusBorderWidth", &v);
	}
	else if (menuItem == menuItemScreenGridLinesAlpha)
	{
		float v = menuItemScreenGridLinesAlpha->value;
		C64DebuggerSetSetting("GridLinesAlpha", &v);
	}
	else if (menuItem == menuItemScreenGridLinesColorScheme)
	{
		int v = menuItemScreenGridLinesColorScheme->selectedOption;
		C64DebuggerSetSetting("GridLinesColor", &v);
	}
	else if (menuItem == menuItemScreenRasterViewfinderScale)
	{
		float v = menuItemScreenRasterViewfinderScale->value;
		C64DebuggerSetSetting("ViewfinderScale", &v);
	}
	else if (menuItem == menuItemScreenRasterCrossLinesAlpha)
	{
		float v = menuItemScreenRasterCrossLinesAlpha->value;
		C64DebuggerSetSetting("CrossLinesAlpha", &v);
	}
	else if (menuItem == menuItemScreenRasterCrossLinesColorScheme)
	{
		int v = menuItemScreenRasterCrossLinesColorScheme->selectedOption;
		C64DebuggerSetSetting("CrossLinesColor", &v);
	}
	else if (menuItem == menuItemScreenRasterCrossAlpha)
	{
		float v = menuItemScreenRasterCrossAlpha->value;
		C64DebuggerSetSetting("CrossAlpha", &v);
	}
	else if (menuItem == menuItemScreenRasterCrossInteriorColorScheme)
	{
		int v = menuItemScreenRasterCrossInteriorColorScheme->selectedOption;
		C64DebuggerSetSetting("CrossInteriorColor", &v);
	}
	else if (menuItem == menuItemScreenRasterCrossExteriorColorScheme)
	{
		int v = menuItemScreenRasterCrossExteriorColorScheme->selectedOption;
		C64DebuggerSetSetting("CrossExteriorColor", &v);
	}
	else if (menuItem == menuItemScreenRasterCrossTipColorScheme)
	{
		int v = menuItemScreenRasterCrossTipColorScheme->selectedOption;
		C64DebuggerSetSetting("CrossTipColor", &v);
	}
	
	//
	else if (menuItem == menuItemPaintGridShowZoomLevel)
	{
		float v = menuItemPaintGridShowZoomLevel->value;
		C64DebuggerSetSetting("PaintGridShowZoomLevel", &v);
	}
	
	else if (menuItem == menuItemPaintGridCharactersColorR)
	{
		float v = menuItemPaintGridCharactersColorR->value;
		C64DebuggerSetSetting("PaintGridCharactersColorR", &v);
	}
	else if (menuItem == menuItemPaintGridCharactersColorG)
	{
		float v = menuItemPaintGridCharactersColorG->value;
		C64DebuggerSetSetting("PaintGridCharactersColorG", &v);
	}
	else if (menuItem == menuItemPaintGridCharactersColorB)
	{
		float v = menuItemPaintGridCharactersColorB->value;
		C64DebuggerSetSetting("PaintGridCharactersColorB", &v);
	}
	else if (menuItem == menuItemPaintGridCharactersColorA)
	{
		float v = menuItemPaintGridCharactersColorA->value;
		C64DebuggerSetSetting("PaintGridCharactersColorA", &v);
	}
	
	else if (menuItem == menuItemPaintGridPixelsColorR)
	{
		float v = menuItemPaintGridPixelsColorR->value;
		C64DebuggerSetSetting("PaintGridPixelsColorR", &v);
	}
	else if (menuItem == menuItemPaintGridPixelsColorG)
	{
		float v = menuItemPaintGridPixelsColorG->value;
		C64DebuggerSetSetting("PaintGridPixelsColorG", &v);
	}
	else if (menuItem == menuItemPaintGridPixelsColorB)
	{
		float v = menuItemPaintGridPixelsColorB->value;
		C64DebuggerSetSetting("PaintGridPixelsColorB", &v);
	}
	else if (menuItem == menuItemPaintGridPixelsColorA)
	{
		float v = menuItemPaintGridPixelsColorA->value;
		C64DebuggerSetSetting("PaintGridPixelsColorA", &v);
	}
	else if (menuItem == menuItemVicEditorForceReplaceColor)
	{
		bool v = menuItemVicEditorForceReplaceColor->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("VicEditorForceReplaceColor", &(v));
	}
	else if (menuItem == menuItemIsProcessPriorityBoostDisabled)
	{
		bool v = menuItemIsProcessPriorityBoostDisabled->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("DisableProcessPriorityBoost", &(v));
	}
	else if (menuItem == menuItemProcessPriority)
	{
		u8 v = menuItemProcessPriority->selectedOption;
		C64DebuggerSetSetting("ProcessPriority", &(v));
	}
	
	//
	else if (menuItem == menuItemMemoryMapFadeSpeed)
	{
		int sel = menuItemMemoryMapFadeSpeed->selectedOption;
		
		int newFadeSpeed = 100;
		if (sel == 0)
		{
			newFadeSpeed = 1;
		}
		else if (sel == 1)
		{
			newFadeSpeed = 10;
		}
		else if (sel == 2)
		{
			newFadeSpeed = 20;
		}
		else if (sel == 3)
		{
			newFadeSpeed = 50;
		}
		else if (sel == 4)
		{
			newFadeSpeed = 100;
		}
		else if (sel == 5)
		{
			newFadeSpeed = 200;
		}
		else if (sel == 6)
		{
			newFadeSpeed = 300;
		}
		else if (sel == 7)
		{
			newFadeSpeed = 400;
		}
		else if (sel == 8)
		{
			newFadeSpeed = 500;
		}
		else if (sel == 9)
		{
			newFadeSpeed = 1000;
		}
		
		C64DebuggerSetSetting("MemMapFadeSpeed", &newFadeSpeed);
	}
	else if (menuItem == menuItemMaximumSpeed)
	{
		int sel = menuItemMaximumSpeed->selectedOption;
		
		int newMaximumSpeed = 100;
		if (sel == 0)
		{
			newMaximumSpeed = 10;
		}
		else if (sel == 1)
		{
			newMaximumSpeed = 20;
		}
		else if (sel == 2)
		{
			newMaximumSpeed = 50;
		}
		else if (sel == 3)
		{
			newMaximumSpeed = 100;
		}
		else if (sel == 4)
		{
			newMaximumSpeed = 200;
		}
		else if (sel == 5)
		{
			newMaximumSpeed = 300;
		}
		else if (sel == 6)
		{
			newMaximumSpeed = 400;
		}
		
		SetEmulationMaximumSpeed(newMaximumSpeed);
	}
	else if (viewC64->debugInterfaceAtari)
	{
		if (menuItem == menuItemAtariVideoSystem)
		{
			C64DebuggerSetSetting("AtariVideoSystem", &(menuItemAtariVideoSystem->selectedOption));
		}
		else if (menuItem == menuItemAtariMachineType)
		{
			C64DebuggerSetSetting("AtariMachineType", &(menuItemAtariMachineType->selectedOption));
			c64SettingsAtariRamSizeOption = menuItemAtariRamSize->selectedOption;
		}
		else if (menuItem == menuItemAtariRamSize)
		{
			C64DebuggerSetSetting("AtariRamSizeOption", &(menuItemAtariRamSize->selectedOption));
		}
	}
	
	C64DebuggerStoreSettings();
}

void CViewSettingsMenu::UpdateAtariRamSizeOptions()
{
	int optionNum = 0;
	menuItemAtariRamSize->SetSelectedOption(optionNum, false);

	switch(menuItemAtariMachineType->selectedOption)
	{
		case 0:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSize800);
			optionNum = 1;
			break;
		case 1:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSize800);
			optionNum = 5;
			break;
		default:
		case 2:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSizeXL);
			optionNum = 3;
			break;
		case 3:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSizeXL);
			optionNum = 0;
			break;
		case 4:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSizeXL);
			optionNum = 3;
			break;
		case 5:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSizeXL);
			optionNum = 4;
			break;
		case 6:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSizeXL);
			optionNum = 3;
			break;
		case 7:
			menuItemAtariRamSize->SetOptionsWithoutDelete(optionsAtariRamSize5200);
			optionNum = 0;
			break;
	}
	
	menuItemAtariRamSize->SetSelectedOption(optionNum, false);
}

void CViewSettingsMenu::SwitchNextMaximumSpeed()
{
	int newMaximumSpeed = 100;
	switch(c64SettingsEmulationMaximumSpeed)
	{
		case 10:
			newMaximumSpeed = 20;
			break;
		case 20:
			newMaximumSpeed = 50;
			break;
		case 50:
			newMaximumSpeed = 100;
			break;
		case 100:
			newMaximumSpeed = 200;
			break;
		case 200:
			newMaximumSpeed = 300;
			break;
		case 300:
			newMaximumSpeed = 400;
			break;
		case 400:
			newMaximumSpeed = 10;
			break;
		default:
			newMaximumSpeed = 100;
			break;
	}
	
	SetEmulationMaximumSpeed(newMaximumSpeed);
}

void CViewSettingsMenu::SwitchPrevMaximumSpeed()
{
	int newMaximumSpeed = 100;
	switch(c64SettingsEmulationMaximumSpeed)
	{
		case 10:
			newMaximumSpeed = 400;
			break;
		case 20:
			newMaximumSpeed = 10;
			break;
		case 50:
			newMaximumSpeed = 20;
			break;
		case 100:
			newMaximumSpeed = 50;
			break;
		case 200:
			newMaximumSpeed = 100;
			break;
		case 300:
			newMaximumSpeed = 200;
			break;
		case 400:
			newMaximumSpeed = 300;
			break;
		default:
			newMaximumSpeed = 100;
			break;
	}
	
	SetEmulationMaximumSpeed(newMaximumSpeed);
	
}

void CViewSettingsMenu::SetEmulationMaximumSpeed(int maximumSpeed)
{
	C64DebuggerSetSetting("EmulationMaximumSpeed", &maximumSpeed);
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "Emulation speed set to %d", maximumSpeed);
	guiMain->ShowMessage(buf);
	SYS_ReleaseCharBuf(buf);
}

void CViewSettingsMenu::DetachEverything(bool showMessage, bool storeSettings)
{
	// detach drive, cartridge & tape
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->DetachEverything();
		
		guiMain->LockMutex();
		
//		if (viewC64->viewC64MainMenu->menuItemInsertD64->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemInsertD64->str2;
//		viewC64->viewC64MainMenu->menuItemInsertD64->str2 = NULL;
		
		delete c64SettingsPathToD64;
		c64SettingsPathToD64 = NULL;
		
//		if (viewC64->viewC64MainMenu->menuItemInsertCartridge->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemInsertCartridge->str2;
//		viewC64->viewC64MainMenu->menuItemInsertCartridge->str2 = NULL;
		
		delete c64SettingsPathToCartridge;
		c64SettingsPathToCartridge = NULL;
		
//		if (viewC64->viewC64MainMenu->menuItemOpenFile->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemOpenFile->str2;
//		viewC64->viewC64MainMenu->menuItemOpenFile->str2 = NULL;
		
		delete c64SettingsPathToPRG;
		c64SettingsPathToPRG = NULL;

		delete c64SettingsPathToTAP;
		c64SettingsPathToTAP = NULL;

		viewC64->viewC64MemoryMap->ClearExecuteMarkers();
		viewC64->viewDrive1541MemoryMap->ClearExecuteMarkers();		
		
		guiMain->UnlockMutex();
	}
	else if (viewC64->debugInterfaceAtari)
	{
		viewC64->debugInterfaceAtari->DetachEverything();
		
		guiMain->LockMutex();
		
//		if (viewC64->viewC64MainMenu->menuItemInsertATR->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemInsertATR->str2;
//		viewC64->viewC64MainMenu->menuItemInsertATR->str2 = NULL;
		
		delete c64SettingsPathToATR;
		c64SettingsPathToATR = NULL;
		
//		if (viewC64->viewC64MainMenu->menuItemInsertAtariCartridge->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemInsertAtariCartridge->str2;
//		viewC64->viewC64MainMenu->menuItemInsertAtariCartridge->str2 = NULL;
		
		delete c64SettingsPathToAtariCartridge;
		c64SettingsPathToAtariCartridge = NULL;
		
//		if (viewC64->viewC64MainMenu->menuItemOpenFile->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemOpenFile->str2;
//		viewC64->viewC64MainMenu->menuItemOpenFile->str2 = NULL;
		
		delete c64SettingsPathToXEX;
		c64SettingsPathToXEX = NULL;
		
		delete c64SettingsPathToCAS;
		c64SettingsPathToCAS = NULL;

		viewC64->viewAtariMemoryMap->ClearExecuteMarkers();
		
		guiMain->UnlockMutex();

	}

	
	if (storeSettings)
	{
		C64DebuggerStoreSettings();
	}
	
	if (showMessage)
	{
		guiMain->ShowMessage("Detached everything");
	}
}

void CViewSettingsMenu::DetachDiskImage()
{
	if (viewC64->debugInterfaceC64)
	{
		// detach drive
		viewC64->debugInterfaceC64->DetachDriveDisk();
		
		guiMain->LockMutex();
		
//		if (viewC64->viewC64MainMenu->menuItemInsertD64->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemInsertD64->str2;
//		viewC64->viewC64MainMenu->menuItemInsertD64->str2 = NULL;
		
		delete c64SettingsPathToD64;
		c64SettingsPathToD64 = NULL;
		
		guiMain->UnlockMutex();
	}
	
	if (viewC64->debugInterfaceAtari)
	{
		// detach drive
		viewC64->debugInterfaceAtari->DetachDriveDisk();
		
		guiMain->LockMutex();
		
//		if (viewC64->viewC64MainMenu->menuItemInsertATR->str2 != NULL)
//			delete viewC64->viewC64MainMenu->menuItemInsertATR->str2;
//		viewC64->viewC64MainMenu->menuItemInsertATR->str2 = NULL;
		
		delete c64SettingsPathToATR;
		c64SettingsPathToATR = NULL;
		
		guiMain->UnlockMutex();
	}
	
	C64DebuggerStoreSettings();
	
	guiMain->ShowMessage("Drive image detached");
}

void CViewSettingsMenu::DetachCartridge(bool showMessage)
{
	// detach cartridge
	viewC64->debugInterfaceC64->DetachCartridge();
	
	guiMain->LockMutex();
	
//	if (viewC64->viewC64MainMenu->menuItemInsertCartridge->str2 != NULL)
//		delete viewC64->viewC64MainMenu->menuItemInsertCartridge->str2;
//	viewC64->viewC64MainMenu->menuItemInsertCartridge->str2 = NULL;
	
	delete c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = NULL;
	
	guiMain->UnlockMutex();
	
	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		guiMain->ShowMessage("Cartridge detached");
	}
}

void CViewSettingsMenu::DetachTape(bool showMessage)
{
	// detach tape
	viewC64->debugInterfaceC64->DetachTape();
	
	guiMain->LockMutex();
	
//	if (viewC64->viewC64MainMenu->menuItemInsertCartridge->str2 != NULL)
//		delete viewC64->viewC64MainMenu->menuItemInsertCartridge->str2;
//	viewC64->viewC64MainMenu->menuItemInsertCartridge->str2 = NULL;
	
	delete c64SettingsPathToTAP;
	c64SettingsPathToTAP = NULL;
	
	guiMain->UnlockMutex();
	
	C64DebuggerStoreSettings();
	
	if (showMessage)
	{
		guiMain->ShowMessage("Tape detached");
	}

}

void CViewSettingsMenu::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{
	if (menuItem == menuItemSetFolderWithAtariROMs)
	{
		viewC64->viewC64MainMenu->OpenDialogSetFolderWithAtariROMs();
	}
	if (menuItem == menuItemDetachEverything)
	{
		DetachEverything(true, true);
	}
	if (menuItem == menuItemDetachDiskImage)
	{
		DetachDiskImage();
	}
	if (menuItem == menuItemDetachCartridge)
	{
		DetachCartridge(true);
	}
	else if (menuItem == menuItemTapeAttach)
	{
		viewC64->viewC64MainMenu->OpenDialogInsertTape();
	}
	else if (menuItem == menuItemTapeDetach)
	{
		DetachTape(true);
	}
	else if (menuItem == menuItemTapeStop)
	{
		viewC64->debugInterfaceC64->DatasetteStop();
		guiMain->ShowMessage("Datasette STOP");
	}
	else if (menuItem == menuItemTapePlay)
	{
		viewC64->debugInterfaceC64->DatasettePlay();
		guiMain->ShowMessage("Datasette PLAY");
	}
	else if (menuItem == menuItemTapeForward)
	{
		viewC64->debugInterfaceC64->DatasetteForward();
		guiMain->ShowMessage("Datasette FORWARD");
	}
	else if (menuItem == menuItemTapeRewind)
	{
		viewC64->debugInterfaceC64->DatasetteRewind();
		guiMain->ShowMessage("Datasette REWIND");
	}
	else if (menuItem == menuItemTapeReset)
	{
		viewC64->debugInterfaceC64->DatasetteReset();
		guiMain->ShowMessage("Datasette RESET");
	}
	else if (menuItem == menuItemReuAttach)
	{
		viewC64->viewC64MainMenu->OpenDialogAttachReu();
	}
	else if (menuItem == menuItemReuSave)
	{
		viewC64->viewC64MainMenu->OpenDialogSaveReu();
	}
	else if (menuItem == menuItemDumpC64Memory)
	{
		OpenDialogDumpC64Memory();
	}
	else if (menuItem == menuItemDumpC64MemoryMarkers)
	{
		OpenDialogDumpC64MemoryMarkers();
	}
	else if (menuItem == menuItemDumpDrive1541Memory)
	{
		OpenDialogDumpDrive1541Memory();
	}
	else if (menuItem == menuItemDumpDrive1541MemoryMarkers)
	{
		OpenDialogDumpDrive1541MemoryMarkers();
	}
	else if (menuItem == menuItemMapC64MemoryToFile)
	{
		if (c64SettingsPathToC64MemoryMapFile == NULL)
		{
			OpenDialogMapC64MemoryToFile();
		}
		else
		{
			guiMain->LockMutex();
			delete c64SettingsPathToC64MemoryMapFile;
			c64SettingsPathToC64MemoryMapFile = NULL;
			guiMain->UnlockMutex();
			
			C64DebuggerStoreSettings();
			
			UpdateMapC64MemoryToFileLabels();
			guiMain->ShowMessage("Please restart debugger to unmap file");
		}
	}
	else if (menuItem == menuItemC64ProfilerFilePath)
	{
		OpenDialogSetC64ProfilerFileOutputPath();
	}
	else if (menuItem == menuItemC64ProfilerStartStop)
	{
		C64ProfilerStartStop();
	}
	else if (menuItem == menuItemSetC64KeyboardMapping)
	{
//		guiMain->SetView(viewC64->viewC64KeyMap);
	}
	else if (menuItem == menuItemSetKeyboardShortcuts)
	{
//		guiMain->SetView(viewC64->viewKeyboardShortcuts);
	}
	else if (menuItem == menuItemStartJukeboxPlaylist)
	{
		viewC64->viewC64MainMenu->OpenDialogStartJukeboxPlaylist();
	}
	else if (menuItem == menuItemClearSettings)
	{
		// TODO: move to C64DebuggerClearSettings
		
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
		
		fileName->Set(C64D_SETTINGS_HJSON_FILE_PATH);
		byteBuffer->storeToSettings(fileName);
		
		delete fileName;
		delete byteBuffer;
		
		LOGTODO("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");
//#if defined(MACOS)
//		NSString *appDomain = NSBundle.mainBundle.bundleIdentifier;
//		[[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];
//		[[NSUserDefaults standardUserDefaults] synchronize];
//#endif
		// that could be a bad bug:  :D
		
		guiMain->ShowMessage("Settings cleared, please restart C64 debugger");
		return;
	}
	else if (menuItem == menuItemBack)
	{
		this->DeactivateView();
//		guiMain->SetView(viewC64->viewC64MainMenu);
	}
}

void CViewSettingsMenu::C64ProfilerStartStop()
{
	if (isProfilingC64)
	{
		viewC64->debugInterfaceC64->ProfilerDeactivate();
		menuItemC64ProfilerStartStop->SetString(new CSlrString("Start profiling C64"));
		guiMain->ShowMessage("C64 Profiler stopped");
		isProfilingC64 = false;
		return;
	}
	
	if (c64SettingsC64ProfilerFileOutputPath == NULL)
	{
		guiMain->ShowMessage("Please set the C64 profiler output file first");
		return;
	}
	
	char *path = c64SettingsC64ProfilerFileOutputPath->GetStdASCII();
	FILE *fp = fopen(path, "wb");
	if (fp == NULL)
	{
		guiMain->ShowMessage("C64 Profiler can't write to selected file");
		return;
	}
	fclose(fp);
	
	// TODO: add option to run profiler for selected number of cycles
	viewC64->debugInterfaceC64->ProfilerActivate(path, -1, false);
	menuItemC64ProfilerStartStop->SetString(new CSlrString("Stop profiling C64"));
	isProfilingC64 = true;
	guiMain->ShowMessage("C64 Profiler started");
}

void CViewSettingsMenu::OpenDialogDumpC64Memory()
{
	//c64SettingsDefaultMemoryDumpFolder->DebugPrint("c64SettingsDefaultMemoryDumpFolder=");
	
	openDialogFunction = VIEWC64SETTINGS_DUMP_C64_MEMORY;
	
	CSlrString *defaultFileName = new CSlrString("c64memory");
	
	CSlrString *windowTitle = new CSlrString("Dump C64 memory");
	viewC64->ShowDialogSaveFile(this, &memoryExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewSettingsMenu::OpenDialogDumpC64MemoryMarkers()
{
	openDialogFunction = VIEWC64SETTINGS_DUMP_C64_MEMORY_MARKERS;
	
	CSlrString *defaultFileName = new CSlrString("c64markers");
	
	CSlrString *windowTitle = new CSlrString("Dump C64 memory markers");
	viewC64->ShowDialogSaveFile(this, &csvExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewSettingsMenu::OpenDialogDumpDrive1541Memory()
{
	openDialogFunction = VIEWC64SETTINGS_DUMP_DRIVE1541_MEMORY;
	
	CSlrString *defaultFileName = new CSlrString("1541memory");
	
	CSlrString *windowTitle = new CSlrString("Dump Disk 1541 memory");
	viewC64->ShowDialogSaveFile(this, &memoryExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewSettingsMenu::OpenDialogDumpDrive1541MemoryMarkers()
{
	openDialogFunction = VIEWC64SETTINGS_DUMP_DRIVE1541_MEMORY_MARKERS;
	
	CSlrString *defaultFileName = new CSlrString("1541markers");
	
	CSlrString *windowTitle = new CSlrString("Dump Disk 1541 memory markers");
	viewC64->ShowDialogSaveFile(this, &csvExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}


void CViewSettingsMenu::OpenDialogMapC64MemoryToFile()
{
	openDialogFunction = VIEWC64SETTINGS_MAP_C64_MEMORY_TO_FILE;
	
	CSlrString *defaultFileName = new CSlrString("c64memory");
	
	CSlrString *windowTitle = new CSlrString("Map C64 memory to file");
	viewC64->ShowDialogSaveFile(this, &memoryExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewSettingsMenu::OpenDialogSetC64ProfilerFileOutputPath()
{
	openDialogFunction = VIEWC64SETTINGS_SET_C64_PROFILER_OUTPUT;
	
	CSlrString *defaultFileName = new CSlrString("c64profile");
	
	CSlrString *windowTitle = new CSlrString("Set C64 profiler output file");
	viewC64->ShowDialogSaveFile(this, &profilerExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewSettingsMenu::SystemDialogFileSaveSelected(CSlrString *path)
{
	if (openDialogFunction == VIEWC64SETTINGS_DUMP_C64_MEMORY)
	{
		DumpC64Memory(path);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_DUMP_C64_MEMORY_MARKERS)
	{
		DumpC64MemoryMarkers(path);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_DUMP_DRIVE1541_MEMORY)
	{
		DumpDisk1541Memory(path);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_DUMP_DRIVE1541_MEMORY_MARKERS)
	{
		DumpDisk1541MemoryMarkers(path);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_MAP_C64_MEMORY_TO_FILE)
	{
		MapC64MemoryToFile(path);
		C64DebuggerStoreSettings();
	}
	else if (openDialogFunction == VIEWC64SETTINGS_SET_C64_PROFILER_OUTPUT)
	{
		SetC64ProfilerOutputFile(path);
		C64DebuggerStoreSettings();
	}
}

void CViewSettingsMenu::SystemDialogFileSaveCancelled()
{
	
}

void CViewSettingsMenu::DumpC64Memory(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpC64Memory, path=");

	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();

	char *asciiPath = path->GetStdASCII();
	
	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		viewC64->debugInterfaceC64->GetWholeMemoryMapFromRam(memoryBuffer);
	}
	else
	{
		viewC64->debugInterfaceC64->GetWholeMemoryMap(memoryBuffer);
	}

	// BUG: this below will read value from RAM, but $0000 and $0001 are a special case which is already handled by GetWholeMemoryMapFromRam
//	memoryBuffer[0x0000] = viewC64->debugInterfaceC64->GetByteFromRamC64(0x0000);
//	memoryBuffer[0x0001] = viewC64->debugInterfaceC64->GetByteFromRamC64(0x0001);

	FILE *fp = fopen(asciiPath, "wb");
	if (fp == NULL)
	{
		guiMain->ShowMessage("Saving memory dump failed");
		return;
	}
	
	fwrite(memoryBuffer, 0x10000, 1, fp);
	fclose(fp);
	
	delete [] memoryBuffer;
	delete [] asciiPath;
	
	guiMain->ShowMessage("C64 memory dumped");
}

void CViewSettingsMenu::DumpDisk1541Memory(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpDisk1541Memory, path=");
	
	char *asciiPath = path->GetStdASCII();

	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		viewC64->debugInterfaceC64->GetWholeMemoryMapFromRam1541(memoryBuffer);
	}
	else
	{
		viewC64->debugInterfaceC64->GetWholeMemoryMap1541(memoryBuffer);
	}
	
	memoryBuffer[0x0000] = viewC64->debugInterfaceC64->GetByteFromRam1541(0x0000);
	memoryBuffer[0x0001] = viewC64->debugInterfaceC64->GetByteFromRam1541(0x0001);
	
	FILE *fp = fopen(asciiPath, "wb");
	if (fp == NULL)
	{
		guiMain->ShowMessage("Saving memory dump failed");
		return;
	}
	
//	fwrite(memoryBuffer, 0x10000, 1, fp);
	fwrite(memoryBuffer, 0x0800, 1, fp);
	
	fclose(fp);

	delete [] memoryBuffer;
	delete [] asciiPath;
	
	guiMain->ShowMessage("Drive 1541 memory dumped");
}


void CViewSettingsMenu::DumpC64MemoryMarkers(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpC64MemoryMarkers, path=");
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	
	char *asciiPath = path->GetStdASCII();
	
	FILE *fp = fopen(asciiPath, "wb");
	delete [] asciiPath;

	if (fp == NULL)
	{
		guiMain->ShowMessage("Saving memory markers failed");
		return;
	}
	
	viewC64->debugInterfaceC64->LockMutex();
	
	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		viewC64->debugInterfaceC64->GetWholeMemoryMapFromRam(memoryBuffer);
	}
	else
	{
		viewC64->debugInterfaceC64->GetWholeMemoryMap(memoryBuffer);
	}

	memoryBuffer[0x0000] = viewC64->debugInterfaceC64->GetByteFromRamC64(0x0000);
	memoryBuffer[0x0001] = viewC64->debugInterfaceC64->GetByteFromRamC64(0x0001);

	fprintf(fp, "Address,Value,Read,Write,Execute,Argument\n");
	
	for (int i = 0; i < 0x10000; i++)
	{
		CViewMemoryMapCell *cell = viewC64->viewC64MemoryMap->memoryCells[i];
		
		fprintf(fp, "%04x,%02x,%s,%s,%s,%s\n", i, memoryBuffer[i],
				cell->isRead ? "read" : "",
				cell->isWrite ? "write" : "",
				cell->isExecuteCode ? "execute" : "",
				cell->isExecuteArgument ? "argument" : "");
	}
	
	fclose(fp);

	delete [] memoryBuffer;

	viewC64->debugInterfaceC64->UnlockMutex();

	guiMain->ShowMessage("C64 memory markers saved");
}

void CViewSettingsMenu::DumpDisk1541MemoryMarkers(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpDisk1541MemoryMarkers, path=");
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	
	char *asciiPath = path->GetStdASCII();
	
	FILE *fp = fopen(asciiPath, "wb");
	delete [] asciiPath;
	
	if (fp == NULL)
	{
		guiMain->ShowMessage("Saving memory markers failed");
		return;
	}
	
	viewC64->debugInterfaceC64->LockMutex();
	
	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		for (int addr = 0; addr < 0x10000; addr++)
		{
			memoryBuffer[addr] = viewC64->debugInterfaceC64->GetByteFromRam1541(addr);
		}
	}
	else
	{
		for (int addr = 0; addr < 0x10000; addr++)
		{
			memoryBuffer[addr] = viewC64->debugInterfaceC64->GetByte1541(addr);
		}
	}
	
	memoryBuffer[0x0000] = viewC64->debugInterfaceC64->GetByteFromRam1541(0x0000);
	memoryBuffer[0x0001] = viewC64->debugInterfaceC64->GetByteFromRam1541(0x0001);
	
	fprintf(fp, "Address,Value,Read,Write,Execute,Argument\n");
	
	for (int i = 0; i < 0x10000; i++)
	{
		CViewMemoryMapCell *cell = viewC64->viewDrive1541MemoryMap->memoryCells[i];
		
		fprintf(fp, "%04x,%02x,%s,%s,%s,%s\n", i, memoryBuffer[i],
				cell->isRead ? "read" : "",
				cell->isWrite ? "write" : "",
				cell->isExecuteCode ? "execute" : "",
				cell->isExecuteArgument ? "argument" : "");
	}
	
	fclose(fp);
	
	delete [] memoryBuffer;
	
	viewC64->debugInterfaceC64->UnlockMutex();
	
	guiMain->ShowMessage("Drive 1541 memory markers saved");
}




void CViewSettingsMenu::MapC64MemoryToFile(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::MapC64MemoryToFile, path=");
	
	if (c64SettingsPathToC64MemoryMapFile != path)
	{
		if (c64SettingsPathToC64MemoryMapFile != NULL)
			delete c64SettingsPathToC64MemoryMapFile;
		c64SettingsPathToC64MemoryMapFile = new CSlrString(path);
	}
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	
	UpdateMapC64MemoryToFileLabels();
	
	guiMain->ShowMessage("Please restart debugger to map memory");
}

void CViewSettingsMenu::SetC64ProfilerOutputFile(CSlrString *path)
{
	path->DebugPrint("CViewSettingsMenu::SetC64ProfilerOutputFile, path=");
	
	if (c64SettingsC64ProfilerFileOutputPath != path)
	{
		if (c64SettingsC64ProfilerFileOutputPath != NULL)
		delete c64SettingsC64ProfilerFileOutputPath;
		c64SettingsC64ProfilerFileOutputPath = new CSlrString(path);
	}
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
	delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	
	UpdateC64ProfilerFilePath();
	
	C64DebuggerStoreSettings();
}


void CViewSettingsMenu::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewSettingsMenu::Render()
{
//	guiMain->fntConsole->BlitText("CViewSettingsMenu", 0, 0, 0, 11, 1.0);

	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
		
	float sb = 20;
	float gap = 4;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextB;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = 3;
	
	float ar = lr;
	float ag = lg;
	float ab = lb;
	
	float scrx = sb;
	float scry = sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = scrsx/2.0f + sb;
	float ax = scrx + scrsx - sb;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy,
						viewC64->colorsTheme->colorBackgroundR,
						viewC64->colorsTheme->colorBackgroundG,
						viewC64->colorsTheme->colorBackgroundB, 1.0);
	
	float px = scrx + gap;
	float py = scry + gap;
	
	font->BlitTextColor(strHeader, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
//	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
//	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;

	viewMenu->Render();
	
//	font->BlitTextColor("1541 Device 8...", px, py, -1, fontScale, tr, tg, tb, 1);
//	font->BlitTextColor("Alt+8", ax, py, -1, fontScale, tr, tg, tb, 1);
	
	CGuiView::Render();
}

void CViewSettingsMenu::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewSettingsMenu::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewSettingsMenu::ButtonPressed(CGuiButton *button)
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
bool CViewSettingsMenu::DoTap(float x, float y)
{
	LOGG("CViewSettingsMenu::DoTap:  x=%f y=%f", x, y);
	
	if (viewMenu->DoTap(x, y))
		return true;

	return CGuiView::DoTap(x, y);
}

bool CViewSettingsMenu::DoFinishTap(float x, float y)
{
	LOGG("CViewSettingsMenu::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewSettingsMenu::DoDoubleTap(float x, float y)
{
	LOGG("CViewSettingsMenu::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewSettingsMenu::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewSettingsMenu::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewSettingsMenu::DoScrollWheel(float deltaX, float deltaY)
{
	return viewMenu->DoScrollWheel(deltaX, deltaY);
}


bool CViewSettingsMenu::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewSettingsMenu::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewSettingsMenu::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewSettingsMenu::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewSettingsMenu::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewSettingsMenu::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewSettingsMenu::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewSettingsMenu::SwitchMainMenuScreen()
{
//	if (guiMain->currentView == this)
//	{
//		this->DeactivateView();
//		viewC64->ShowMainScreen();
//	}
//	else
//	{
//		guiMain->SetView(this);
//	}
}

bool CViewSettingsMenu::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
	
	if (keyCode == MTKEY_BACKSPACE)
	{
//		guiMain->SetView(viewC64->viewC64MainMenu);
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

bool CViewSettingsMenu::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (viewMenu->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewSettingsMenu::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewSettingsMenu::ActivateView()
{
	LOGG("CViewSettingsMenu::ActivateView()");
	
	if (viewC64->debugInterfaceC64)
	{
		viewC64->debugInterfaceC64->snapshotsManager->LockMutex();
		
		//
		int modelType = viewC64->debugInterfaceC64->GetC64ModelType();
		this->SetOptionC64ModelType(modelType);
		
		LOGD("c64SettingsReuEnabled=%s", STRBOOL(c64SettingsReuEnabled));
		if (c64SettingsReuEnabled)
		{
			menuItemReuEnabled->SetSelectedOption(1, false);
		}
		else
		{
			menuItemReuEnabled->SetSelectedOption(0, false);
		}
		
		for (int i = 0; i < 8; i++)
		{
			if (settingsReuSizes[i] == c64SettingsReuSize)
			{
				viewC64->viewC64SettingsMenu->menuItemReuSize->SetSelectedOption(i, false);
				break;
			}
		}
		
		viewC64->debugInterfaceC64->snapshotsManager->UnlockMutex();
	}

	UpdateAudioOutDevices();
}

void CViewSettingsMenu::DeactivateView()
{
	LOGG("CViewSettingsMenu::DeactivateView()");
	
//	// restart saving snapshots
//	viewC64->debugInterfaceC64->snapshotsManager->ClearSnapshotsHistory();
}

//
void CViewSettingsMenu::SetOptionC64ModelType(int modelTypeId)
{
	int menuOptionNum = 0;
	for(std::vector<int>::iterator it = c64ModelTypeIds->begin(); it != c64ModelTypeIds->end(); it++)
	{
		int menuModelId = *it;
		if (menuModelId == modelTypeId)
		{
			this->menuItemC64Model->SetSelectedOption(menuOptionNum, false);
			return;
		}
		
		menuOptionNum++;
	}
	
	LOGError("CViewSettingsMenu::SetOptionC64ModelType: modelTypeId=%d not found", modelTypeId);
}

std::vector<CSlrString *> *CViewSettingsMenu::GetSidAddressOptions()
{
	std::vector<CSlrString *> *opts = new std::vector<CSlrString *>();
	
	char *buf = SYS_GetCharBuf();

	for (uint16 j = 0x0020; j < 0x0100; j += 0x0020)
	{
		uint16 addr = 0xD400 + j;
		sprintf(buf, "$%04X", addr);
		CSlrString *str = new CSlrString(buf);
		opts->push_back(str);
	}

	for (uint16 i = 0xD500; i < 0xD800; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			sprintf(buf, "$%04X", addr);
			CSlrString *str = new CSlrString(buf);
			opts->push_back(str);
		}
	}

	for (uint16 i = 0xDE00; i < 0xE000; i += 0x0100)
	{
		for (uint16 j = 0x0000; j < 0x0100; j += 0x0020)
		{
			uint16 addr = i + j;
			sprintf(buf, "$%04X", addr);
			CSlrString *str = new CSlrString(buf);
			opts->push_back(str);
		}
	}
	
	SYS_ReleaseCharBuf(buf);
	
	return opts;
}

uint16 CViewSettingsMenu::GetSidAddressFromOptionNum(int optionNum)
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
	
	LOGError("CViewSettingsMenu::GetSidAddressFromOptionNum: sid address not correct, option num=%d", optionNum);
	return 0xD420;
}

int CViewSettingsMenu::GetOptionNumFromSidAddress(uint16 sidAddress)
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
	
	LOGError("CViewSettingsMenu::GetSidAddressFromOptionNum: sid address not correct, sidAddress=%04x", sidAddress);
	return 0;
}

void CViewSettingsMenu::UpdateSidSettings()
{
	int optNum = GetOptionNumFromSidAddress(c64SettingsSIDStereoAddress);
	menuItemSIDStereoAddress->SetSelectedOption(optNum, false);

	optNum = GetOptionNumFromSidAddress(c64SettingsSIDTripleAddress);
	menuItemSIDTripleAddress->SetSelectedOption(optNum, false);
	
	viewC64->viewC64StateSID->UpdateSidButtonsState();
	
}



