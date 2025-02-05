#ifndef _VIEW_C64KEYMAP_
#define _VIEW_C64KEYMAP_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include <map>
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class C64KeyMap;
class C64KeyCode;

class CViewC64KeyMapKeyData
{
public:
	const char *name1;
	const char *name2;
	float x, y;
	float width;
	float xl;
	int matrixRow;
	int matrixCol;
	
	std::list<C64KeyCode *> keyCodes;
};

class CViewC64KeyMap : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback
{
public:
	CViewC64KeyMap(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewC64KeyMap();
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
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

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float fontWidth;
	float tr;
	float tg;
	float tb;
	
	CSlrFont *fontProp;
	float fontPropScale;
	float fontPropHeight;
	
	CSlrString *strHeader;
	void SwitchScreen();
	

	float line1x, line1y, line1sx;
	float line2x, line2y, line2sx;
	float line3x, line3y, line3sx;
	float line4x, line4y, line4sx;
	float line5x, line5y, line5sx;
	
	float linel1x, linel1sy;
	float linel2x, linel2sy;

	float linespcx, linespc1y, linespc2y, linespcsx, linespcsy;
	
	float linefx, linefy, linefsx, linefsy;
	
	std::map<int, CViewC64KeyMapKeyData *> buttonKeys;
	CViewC64KeyMapKeyData *AddButtonKey(const char *keyName1, const char *keyName2, float x, float y, float width, int matrixRow, int matrixCol);
	
	//
	
	CViewC64KeyMapKeyData *selectedKeyData;
	C64KeyCode *selectedKeyCode;
	void SelectKey(CViewC64KeyMapKeyData *keyData);
	void PressSelectedKey(bool updateIfNotFound);
	
	void ClearKeys();
	void UpdateFromKeyMap(C64KeyMap *keyMap);
	C64KeyMap *keyMap;
	
	CGuiButton *btnBack;
	CGuiButton *btnExportKeyMap;
	CGuiButton *btnImportKeyMap;
	
	CGuiButton *btnAssignKey;
	CGuiButton *btnRemoveKey;

	CGuiButton *btnReset;

	bool ButtonClicked(CGuiButton *button);
	
	void SaveKeyboardMapping();
	
	bool isShift;
	
	void SelectKeyCode(u32 keyCode);
	
	void AssignKey();
	volatile bool isAssigningKey;
	void AssignKey(u32 keyCode); //, bool isShift, bool isAlt, bool isControl);
	
	void RemoveSelectedKey();

	void ResetKeyboardMappingToFactoryDefault();
	
	CViewC64KeyMapKeyData *keyLeftShift;
	CViewC64KeyMapKeyData *keyRightShift;
	
	
	//
	std::list<CSlrString *> extKeyMap;
	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	
	void OpenDialogExportKeyMap();
	void OpenDialogImportKeyMap();
	
	//
	std::map<u32, bool> pressedKeyCodes;

};


#endif //_VIEW_C64ABOUT_
