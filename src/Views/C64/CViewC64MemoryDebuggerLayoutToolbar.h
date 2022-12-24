#ifndef _VIEW_C64MEMORYDEBUGGERLAYOUTTOOLBAR_
#define _VIEW_C64MEMORYDEBUGGERLAYOUTTOOLBAR_

#include "CGuiView.h"
#include "CGuiButtonSwitch.h"

class CDebugInterface;

class CViewC64MemoryDebuggerLayoutToolbar : public CGuiView, CGuiButtonSwitchCallback
{
public:
	CViewC64MemoryDebuggerLayoutToolbar(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewC64MemoryDebuggerLayoutToolbar();

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
	
	virtual void SetPosition(float posX, float posY);
	virtual bool IsFocusableElement();

	virtual void ActivateView();
	virtual void DeactivateView();

	void UpdateStateFromButtons();
	
	CGuiButton *btnDone;
	virtual bool ButtonClicked(CGuiButton *button);
	virtual bool ButtonPressed(CGuiButton *button);
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);

	CDebugInterface *debugInterface;
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;

	CGuiButtonSwitch *btnMemoryDump1IsFromDisk;
	CGuiButtonSwitch *btnMemoryDump2IsFromDisk;
	CGuiButtonSwitch *btnMemoryDump3IsFromDisk;
	
	float fontSize;
};

#endif //_VIEW_TIMELINE_
