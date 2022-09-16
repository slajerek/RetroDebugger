#ifndef _VIEW_C64SETTINGSMENU_
#define _VIEW_C64SETTINGSMENU_

// TODO: this is phased out and moved to CMainMenuBar. this will be removed very soon

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "CSlrKeyboardShortcuts.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;

class CViewSettingsMenu : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CSlrKeyboardShortcutCallback
{
public:
	CViewSettingsMenu(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewSettingsMenu();


	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float tr;
	float tg;
	float tb;
	
	CGuiViewMenu *viewMenu;
	virtual void MenuCallbackItemEntered(CGuiViewMenuItem *menuItem);
	virtual void MenuCallbackItemChanged(CGuiViewMenuItem *menuItem);
	
	CViewC64MenuItem *menuItemSubMenuEmulation;
	CViewC64MenuItem *menuItemSubMenuAudio;
	CViewC64MenuItem *menuItemSubMenuMemory;
	CViewC64MenuItem *menuItemSubMenuTape;
	CViewC64MenuItem *menuItemSubMenuReu;
	CViewC64MenuItem *menuItemSubMenuUI;
	
	//
	CViewC64MenuItem *menuItemStartJukeboxPlaylist;
	
	CViewC64MenuItem *menuItemSetC64KeyboardMapping;
	CViewC64MenuItem *menuItemSetKeyboardShortcuts;

	CSlrKeyboardShortcut *kbsCartridgeFreezeButton;
	CViewC64MenuItem *menuItemCartridgeFreeze;

	//
	CViewC64MenuItem *menuItemMapC64MemoryToFile;
	void UpdateMapC64MemoryToFileLabels();


	//
	
	CViewC64MenuItemOption *menuItemMemoryCellsColorStyle;
	CViewC64MenuItemOption *menuItemMemoryMarkersColorStyle;
	CViewC64MenuItemOption *menuItemMultiTouchMemoryMap;
	CViewC64MenuItemOption *menuItemMemoryMapInvert;
	CViewC64MenuItemOption *menuItemMemoryMapRefreshRate;
	CViewC64MenuItemOption *menuItemMemoryMapFadeSpeed;

	CViewC64MenuItemOption *menuItemAtariPokeyStereo;

	virtual bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut);

	// tape
	CSlrKeyboardShortcut *kbsTapeAttach;
	CViewC64MenuItem *menuItemTapeAttach;
	CSlrKeyboardShortcut *kbsTapeDetach;
	CViewC64MenuItem *menuItemTapeDetach;
	CViewC64MenuItem *menuItemTapeCreate;

	CSlrKeyboardShortcut *kbsTapeStop;
	CViewC64MenuItem *menuItemTapeStop;
	CSlrKeyboardShortcut *kbsTapePlay;
	CViewC64MenuItem *menuItemTapePlay;
	CSlrKeyboardShortcut *kbsTapeForward;
	CViewC64MenuItem *menuItemTapeForward;
	CSlrKeyboardShortcut *kbsTapeRewind;
	CViewC64MenuItem *menuItemTapeRewind;
	CSlrKeyboardShortcut *kbsTapeRecord;
	CViewC64MenuItem *menuItemTapeRecord;
	CViewC64MenuItem *menuItemTapeReset;

	CViewC64MenuItemFloat *menuItemDatasetteSpeedTuning;
	CViewC64MenuItemFloat *menuItemDatasetteZeroGapDelay;
	CViewC64MenuItemFloat *menuItemDatasetteTapeWobble;
	CViewC64MenuItemOption *menuItemDatasetteResetWithCPU;
	
	//
	CViewC64MenuItemFloat *menuItemScreenGridLinesAlpha;
	CViewC64MenuItemOption *menuItemScreenGridLinesColorScheme;
	CViewC64MenuItemFloat *menuItemScreenRasterCrossLinesAlpha;
	CViewC64MenuItemOption *menuItemScreenRasterCrossLinesColorScheme;
	CViewC64MenuItemFloat *menuItemScreenRasterCrossAlpha;
	CViewC64MenuItemOption *menuItemScreenRasterCrossInteriorColorScheme;
	CViewC64MenuItemOption *menuItemScreenRasterCrossExteriorColorScheme;
	CViewC64MenuItemOption *menuItemScreenRasterCrossTipColorScheme;

	CViewC64MenuItemOption *menuItemShowPositionsInHex;

	CViewC64MenuItemFloat *menuItemPaintGridCharactersColorR;
	CViewC64MenuItemFloat *menuItemPaintGridCharactersColorG;
	CViewC64MenuItemFloat *menuItemPaintGridCharactersColorB;
	CViewC64MenuItemFloat *menuItemPaintGridCharactersColorA;

	CViewC64MenuItemFloat *menuItemPaintGridPixelsColorR;
	CViewC64MenuItemFloat *menuItemPaintGridPixelsColorG;
	CViewC64MenuItemFloat *menuItemPaintGridPixelsColorB;
	CViewC64MenuItemFloat *menuItemPaintGridPixelsColorA;

	CViewC64MenuItemFloat *menuItemPaintGridShowZoomLevel;

	CViewC64MenuItemOption *menuItemVicEditorForceReplaceColor;

	CViewC64MenuItemOption *menuItemRunSIDWhenInWarp;

	CViewC64MenuItemOption *menuItemRestartAudioOnEmulationReset;

	CViewC64MenuItemOption *menuItemRunSIDEmulation;
	CViewC64MenuItemFloat *menuItemAudioVolume;
	CViewC64MenuItemOption *menuItemMuteSIDMode;
	
	CSlrKeyboardShortcut *kbsSwitchSoundOnOff;  // mojzesh
	
	
	CViewC64MenuItemFloat *menuItemFocusBorderLineWidth;

	CViewC64MenuItemOption *menuItemDisassembleExecuteAware;
	
	CViewC64MenuItemOption *menuItemDisassemblyBackgroundColor;
	CViewC64MenuItemOption *menuItemDisassemblyExecuteColor;
	CViewC64MenuItemOption *menuItemDisassemblyNonExecuteColor;

	CViewC64MenuItemOption *menuItemVicPalette;
	CViewC64MenuItemOption *menuItemRenderScreenInterpolation;
	CViewC64MenuItemOption *menuItemRenderScreenSupersample;

	CViewC64MenuItemOption *menuItemWindowAlwaysOnTop;
	CViewC64MenuItemOption *menuItemUseSystemDialogs;
	
	CViewC64MenuItemOption *menuItemUseOnlyFirstCPU;
	
	CViewC64MenuItem *menuItemClearSettings;
	
	CViewC64MenuItem *menuItemBack;

	void SwitchMainMenuScreen();
	
	std::list<CSlrString *> memoryExtensions;
	std::list<CSlrString *> profilerExtensions;
	
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();

	void OpenDialogMapC64MemoryToFile();
	

	void MapC64MemoryToFile(CSlrString *path);
	
	u8 openDialogFunction;
	
	//
	CViewC64MenuItemOption *menuItemIsProcessPriorityBoostDisabled;
	CViewC64MenuItemOption *menuItemProcessPriority;
		
};


#endif //_VIEW_C64SETTINGSMENU_
