#ifndef _CViewC64VicControl_H_
#define _CViewC64VicControl_H_

#include "CGuiView.h"
#include "CGuiLockableList.h"
#include "CGuiButtonSwitch.h"
#include "CGuiLabel.h"
#include "CGuiViewFrame.h"

extern "C"
{
#include "ViceWrapper.h"
};

class CSlrMutex;
class CDebugInterfaceC64;
class CSlrFont;
class CViewC64VicDisplay;

class CViewC64VicControl : public CGuiView, CGuiButtonSwitchCallback, CGuiListCallback
{
public:
	
	CViewC64VicControl(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64VicDisplay *vicDisplay);
	virtual ~CViewC64VicControl();
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void DoLogic();
	
	virtual bool IsInside(float x, float y);
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);
	
	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);
	
	virtual bool DoNotTouchedMove(float x, float y);
	
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	virtual bool DoRightClick(float x, float y);
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual void ActivateView();
	virtual void DeactivateView();
	
	void AddGuiButtons();
	void HideGuiButtons();
	
	float fontSize;
	
	void RefreshScreenStateOnly(vicii_cycle_state_t *viciiState);
	
	CDebugInterfaceC64 *debugInterface;
		
	//
	volatile bool forceGrayscaleColors;
	volatile bool forceDataFromRam;
	CGuiButtonSwitch *btnModeText;
	CGuiButtonSwitch *btnModeBitmap;
	CGuiButtonSwitch *btnModeHires;
	CGuiButtonSwitch *btnModeMulti;
	CGuiButtonSwitch *btnModeStandard;
	CGuiButtonSwitch *btnModeExtended;
	void SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn);
	void SetLockableButtonDefaultColors(CGuiButtonSwitch *btn);
	void SetButtonState(CGuiButtonSwitch *btn, bool isSet);
	
	CGuiLabel *lblScreenAddress;
	CGuiLockableList *lstScreenAddresses;
	CGuiLabel *lblCharsetAddress;
	CGuiLockableList *lstCharsetAddresses;
	CGuiLabel *lblBitmapAddress;
	CGuiLockableList *lstBitmapAddresses;
	virtual bool ListElementPreSelect(CGuiList *listBox, int elementNum);

	CGuiButtonSwitch *btnApplyScrollRegister;
	void UpdateApplyScrollRegister();
	CGuiButtonSwitch *btnShowBadLines;
	void ShowBadLines(bool showBadLines);

	CGuiButtonSwitch *btnShowWithBorder;
	CGuiButtonSwitch *btnShowGrid;
	
	CGuiButtonSwitch *btnShowSpritesGraphics;
	CGuiButtonSwitch *btnShowSpritesFrames;
	
	CGuiButtonSwitch *btnToggleBreakpoint;
	float txtCursorPosX, txtCursorPosY;
	float txtCursorCharPosX, txtCursorCharPosY;
	
	CGuiLabel *lblAutolockText;
	CGuiButton *btnAutolockScrollMode;
	
	CSlrString *txtAutolockRasterPC;
	CSlrString *txtAutolockBitmapAddress;
	CSlrString *txtAutolockTextAddress;
	CSlrString *txtAutolockColourAddress;
	CSlrString *txtAutolockCharsetAddress;
	
	CGuiButtonSwitch *btnLockCursor;
	//	CGuiButton *btnCursorCycleLeft;
	//	CGuiButton *btnCursorCycleRight;
	//	CGuiButton *btnCursorRasterLineUp;
	//	CGuiButton *btnCursorRasterLineDown;
	
	void UnlockAll();
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	
	void SetBorderType(u8 borderType);
	void SwitchBorderType();

	void SetGridLines(bool isOn);
	void SetApplyScroll(bool isOn);
	
	//
	virtual bool ButtonClicked(CGuiButton *button);
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);
	virtual void RenderFocusBorder();
	
	//
	CGuiViewFrame *viewFrame;
	
	// VIC DISPLAY TO CONTROL
	CViewC64VicDisplay *vicDisplay;
	
	//
	void SetAutoScrollModeUI(int newMode);
	void SetViciiPointersFromUI(uint16 *screenAddress, int *charsetAddress, int *bitmapBank);
	void RefreshStateButtonsUI(u8 *mc, u8 *eb, u8 *bm, u8 *blank);

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

	// data
	virtual void SerializeState(CByteBuffer *byteBuffer);
	virtual void DeserializeState(CByteBuffer *byteBuffer);
};

#endif //_CViewC64VicControl_H_
