#ifndef _VIEW_KEYBOARDSHORTCUTS_
#define _VIEW_KEYBOARDSHORTCUTS_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include <list>

class C64KeyboardShortcuts;
class CSlrKeyboardShortcut;
class CViewC64MenuItem;

class CViewKeyboardShortcuts : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback
{
public:
	CViewKeyboardShortcuts(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewKeyboardShortcuts();

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
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();

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
	CSlrString *strEnterKeyFor;
	CSlrString *strKeyFunctionName;

	
	CGuiViewMenu *viewMenu;
	virtual void MenuCallbackItemEntered(CGuiViewMenuItem *menuItem);
	virtual void MenuCallbackItemChanged(CGuiViewMenuItem *menuItem);
	
	void UpdateMenuKeyboardShortcuts();

	CViewC64MenuItem *menuItemBack;

	CViewC64MenuItem *menuItemExportKeyboardShortcuts;
	CViewC64MenuItem *menuItemImportKeyboardShortcuts;

	void SwitchScreen();
	
	C64KeyboardShortcuts *shortcuts;

	CSlrKeyboardShortcut *enteringKey;
	
	bool keyUpEaten;
	
	bool isShift, isAlt, isControl;
	
	void EnteredKeyCode(u32 keyCode);
	void SaveAndBack();
	
	void StoreKeyboardShortcuts();
	void RestoreKeyboardShortcuts();
	
	void UpdateQuitShortcut();
	
	//
	std::list<CSlrString *> extKeyboardShortucts;
	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	
	void OpenDialogExportKeyboardShortcuts();
	void OpenDialogImportKeyboardShortcuts();
	
};


#endif //_VIEW_KEYBOARDSHORTCUTS_
