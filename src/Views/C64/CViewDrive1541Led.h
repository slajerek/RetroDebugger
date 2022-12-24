#ifndef _CViewDrive1541Led_h_
#define _CViewDrive1541Led_h_

#include "CGuiView.h"

class CDebugInterfaceC64;

class CViewDrive1541Led : public CGuiView
{
public:
	CViewDrive1541Led(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewDrive1541Led();
	
	CDebugInterfaceC64 *debugInterface;
	
	virtual void RenderImGui();

	virtual void DoLogic();

	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool DoRightClick(float x, float y);
	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	// not touched move = mouse move with not clicked button
	virtual bool DoNotTouchedMove(float x, float y);

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
	
	virtual bool DoGamePadButtonDown(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadButtonUp(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value);

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
};

#endif //_GUI_VIEW_DUMMY_
