//
// NOTE:
// This view will be removed. It is being refactored and moved to main menu bar instead
//

#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewSettingsMenu.h"
#include "VID_Main.h"
#include "SYS_DefaultConfig.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64SettingsStorage.h"
#include "CMainMenuBar.h"

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

#define VIEWC64SETTINGS_MAP_C64_MEMORY_TO_FILE			5
#define VIEWC64SETTINGS_ATTACH_TAPE						6
#define VIEWC64SETTINGS_SET_C64_PROFILER_OUTPUT			7

CViewSettingsMenu::CViewSettingsMenu(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewSettingsMenu";

	font = viewC64->fontCBMShifted;
	fontScale = 2.7;
	fontHeight = font->GetCharHeight('@', fontScale) + 3;

//	strHeader = new CSlrString("Settings");

	memoryExtensions.push_back(new CSlrString("bin"));
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

	//
	if (viewC64->debugInterfaceC64)
	{

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

	// N/A:
//	menuItemRestartAudioOnEmulationReset = new CViewC64MenuItemOption(fontHeight, new CSlrString("Restart audio on reset: "),
//																	  NULL, tr, tg, tb, optionsYesNo, font, fontScale);
//	menuItemRestartAudioOnEmulationReset->SetSelectedOption(c64SettingsRestartAudioOnEmulationReset ? 1 : 0, false);
//	menuItemSubMenuAudio->AddMenuItem(menuItemRestartAudioOnEmulationReset);
	
	

	kbsSwitchSoundOnOff = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Switch sound mute On/Off", 't', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSwitchSoundOnOff);
	
	//////////////////
	
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
	
	
	
	
	///////////
	
	

	//
	menuItemWindowAlwaysOnTop = new CViewC64MenuItemOption(fontHeight*2, new CSlrString("Window always on top: "),
																 NULL, tr, tg, tb, optionsYesNo, font, fontScale);
	menuItemWindowAlwaysOnTop->SetSelectedOption(c64SettingsWindowAlwaysOnTop, false);
	menuItemSubMenuUI->AddMenuItem(menuItemWindowAlwaysOnTop);
	

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
	
		
	//
	menuItemFocusBorderLineWidth = new CViewC64MenuItemFloat(fontHeight*2, new CSlrString("Focus border line width: "),
															 NULL, tr, tg, tb,
															 0.0f, 5.0f, 0.05f, font, fontScale);
	menuItemFocusBorderLineWidth->SetValue(0.7f, false);
	menuItemSubMenuUI->AddMenuItem(menuItemFocusBorderLineWidth);


	//
	

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


	}
	
	//

	//
	
	
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
	

//		else if (shortcut == viewC64SettingsMenu->kbsTapeReset)
//		{
//			viewC64->debugInterfaceC64->DatasetteReset();
//			return true;
//		}
	
//	else if (shortcut == kbsIsWarpSpeed)
//	{
//		viewC64->SwitchIsWarpSpeed();
//		return true;
//	}
	if (shortcut == kbsSwitchSoundOnOff)
	{
		viewC64->ToggleSoundMute();
		return true;
	}
	return false;
}


CViewSettingsMenu::~CViewSettingsMenu()
{
}

void CViewSettingsMenu::MenuCallbackItemChanged(CGuiViewMenuItem *menuItem)
{
	return;
	
	if (menuItem == menuItemRenderScreenInterpolation)
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
	
	else if (menuItem == menuItemRunSIDWhenInWarp)
	{
		bool v = menuItemRunSIDWhenInWarp->selectedOption == 0 ? false : true;
		C64DebuggerSetSetting("RunSIDWhenWarp", &(v));
	}
	
	else if (menuItem == menuItemAudioVolume)
	{
		float v = menuItemAudioVolume->value;
		u16 vu16 = ((u16)v);
		C64DebuggerSetSetting("ViceAudioVolume", &(vu16));
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
		viewC64->ShowMessage("Please restart Retro Debugger to apply configuration.");
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
	
	C64DebuggerStoreSettings();
}

void CViewSettingsMenu::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{

	
	if (menuItem == menuItemSetC64KeyboardMapping)
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
		return;
	}
	else if (menuItem == menuItemBack)
	{
		this->DeactivateView();
//		guiMain->SetView(viewC64->viewC64MainMenu);
	}
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

void CViewSettingsMenu::SystemDialogFileSaveSelected(CSlrString *path)
{
	if (openDialogFunction == VIEWC64SETTINGS_MAP_C64_MEMORY_TO_FILE)
	{
		MapC64MemoryToFile(path);
		C64DebuggerStoreSettings();
	}
}

void CViewSettingsMenu::SystemDialogFileSaveCancelled()
{
	
}


void CViewSettingsMenu::MapC64MemoryToFile(CSlrString *path)
{
	/// DELETE ME
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
}

