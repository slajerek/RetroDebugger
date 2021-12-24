#ifndef _VIEW_C64MAINMENU_
#define _VIEW_C64MAINMENU_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "SYS_Threading.h"
#include "CColorsTheme.h"
#include "CSlrKeyboardShortcuts.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class CViewC64MenuItemOption;
class CDebugInterface;

class CViewMainMenu : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CSlrThread, CSlrKeyboardShortcutCallback
{
public:
	CViewMainMenu(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewMainMenu();

	virtual void Render();
	virtual void Render(float posX, float posY);
	//virtual void Render(float posX, float posY, float sizeX, float sizeY);
	virtual void DoLogic();

	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();

	virtual bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut);

	CGuiButton *btnDone;
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);

	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float tr;
	float tg;
	float tb;
	
	CSlrString *strHeader;
	CSlrString *strHeader2;
	CSlrString *strHeader3;
	

	CGuiViewMenu *viewMenu;
	virtual void MenuCallbackItemEntered(CGuiViewMenuItem *menuItem);
	virtual void MenuCallbackItemChanged(CGuiViewMenuItem *menuItem);



	
	std::list<CSlrString *> openFileExtensions;
	std::list<CSlrString *> diskExtensions;
	std::list<CSlrString *> tapeExtensions;
	std::list<CSlrString *> crtExtensions;
	std::list<CSlrString *> reuExtensions;
	std::list<CSlrString *> jukeboxExtensions;
	std::list<CSlrString *> romsFileExtensions;

	void OpenDialogOpenFile();

	void LoadFile(CSlrString *path);
	void OpenDialogInsertD64();
	void InsertD64(CSlrString *path, bool updatePathToD64, bool autoRun, int autoRunEntryNum, bool showLoadAddressInfo);
	std::list<char *> cDiskExtensions;
	void InsertNextD64();
	void OpenDialogInsertCartridge();
	bool InsertCartridge(CSlrString *path, bool updatePathToCRT);
	void OpenDialogInsertAtariCartridge();
	
	bool LoadPRG(CSlrString *path, bool autoStart, bool updatePRGFolderPath, bool showAddressInfo, bool forceFastReset);
	bool LoadPRG(CByteBuffer *byteBuffer, bool autoStart, bool showAddressInfo, bool forceFastReset);
	void LoadPRG(CByteBuffer *byteBuffer, u16 *startAddr, u16 *endAddr);
	bool LoadPRGNotThreaded(CByteBuffer *byteBuffer, bool autoStart, bool showAddressInfo);

	bool LoadSID(CSlrString *filePath);
	
	void OpenDialogInsertTape();
	bool LoadTape(CSlrString *path, bool autoStart, bool updateTAPFolderPath, bool showAddressInfo);
	void DetachTape();

	//
	void OpenDialogAttachReu();
	void OpenDialogSaveReu();
	bool AttachReu(CSlrString *path, bool updatePathToReu, bool showDetails);
	bool SaveReu(CSlrString *path, bool updatePathToReu, bool showDetails);

	//
	CViewC64MenuItem *menuItemSetFolderWithAtariROMs;
	void OpenDialogSetFolderWithAtariROMs();
	
	bool LoadXEX(CSlrString *path, bool autoStart, bool updatePRGFolderPath, bool showAddressInfo);
	bool LoadCAS(CSlrString *path, bool autoStart, bool updatePRGFolderPath, bool showAddressInfo);
	bool InsertAtariCartridge(CSlrString *path, bool autoStart, bool updatePRGFolderPath, bool showAddressInfo);
	
	void OpenDialogInsertATR();
	void InsertATR(CSlrString *path, bool updatePathToATR, bool autoRun, int autoRunEntryNum, bool showLoadAddressInfo);
		
	//
	CViewC64MenuItem *menuItemSetFolderWithNesROMs;
	void OpenDialogSetFolderWithNesROMs();

	bool LoadNES(CSlrString *path, bool updateNESFolderPath);

	
	void LoadLabelsAndWatches(CSlrString *path, CDebugInterface *debugInterface);
	void SetBasicEndAddr(int endAddr);

	void OpenDialogStartJukeboxPlaylist();

	// LoadPRG threaded
	bool loadPrgAutoStart;
	bool loadPrgShowAddressInfo;
	bool loadPrgForceFastReset;
	virtual void ThreadRun(void *data);
	
	void ReloadAndRestartPRG();
	void ResetAndJSR(int startAddr);
	
	u8 openDialogFunction;
	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();

	void SwitchMainMenuScreen();
	
	void ApplyColorsToMenu(CGuiViewMenu *menu);
	virtual void UpdateTheme();
};

class CViewC64MenuItem : public CGuiViewMenuItem
{
public:
	CViewC64MenuItem(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b);
	CViewC64MenuItem(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b,
					 CGuiViewMenu *mainMenu);
	
	virtual void SetSelected(bool selected);
	virtual void RenderItem(float px, float py, float pz);

	virtual void SetString(CSlrString *str);
	
	CSlrString *str;
	CSlrString *str2;
	CSlrKeyboardShortcut *shortcut;
	float r;
	float g;
	float b;
	
	virtual void Execute();
	
	virtual void DebugPrint();
};

class CViewC64MenuItemOption : public CViewC64MenuItem
{
public:
	CViewC64MenuItemOption(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b,
						   std::vector<CSlrString *> *options, CSlrFont *font, float fontScale);
	
	void SetOptions(std::vector<CSlrString *> *options);
	void SetOptionsWithoutDelete(std::vector<CSlrString *> *options);
	
	std::vector<CSlrString *> *options;
	
	CSlrString *textStr;
	
	virtual void SetString(CSlrString *str);
	virtual void UpdateDisplayString();
	
	virtual bool KeyDown(u32 keyCode);
	
	virtual void SwitchToNext();
	virtual void SwitchToPrev();

	virtual void Execute();

	int selectedOption;
	virtual void SetSelectedOption(int newSelectedOption, bool runCallback);
};

class CViewC64MenuItemFloat : public CViewC64MenuItem
{
public:
	CViewC64MenuItemFloat(float height, CSlrString *str, CSlrKeyboardShortcut *shortcut, float r, float g, float b,
						   float minimum, float maximum, float step, CSlrFont *font, float fontScale);
	
	float minimum, maximum, step;
	
	CSlrString *textStr;
	
	virtual void SetString(CSlrString *str);
	virtual void UpdateDisplayString();
	
	virtual bool KeyDown(u32 keyCode);
	
	virtual void SwitchToNext();
	virtual void SwitchToPrev();

	virtual void Execute();
	
	int numLeadingDigits;
	int numDecimalsDigits;

	float value;
	virtual void SetValue(float value, bool runCallback);
};


#endif //_VIEW_C64MAINMENU_
