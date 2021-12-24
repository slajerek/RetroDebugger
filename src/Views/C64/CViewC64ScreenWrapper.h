#ifndef _CVIEWC64SCREENWRAPPER_H_
#define _CVIEWC64SCREENWRAPPER_H_

#include "CGuiView.h"
#include <map>

class CSlrMutex;
class CDebugInterfaceC64;
class C64ColodoreScreen;

typedef enum c64ScreenWrapperModes : u8
{
	C64SCREENWRAPPER_MODE_C64_SCREEN=0,
	C64SCREENWRAPPER_MODE_C64_ZOOMED,
	C64SCREENWRAPPER_MODE_C64_DISPLAY
};

class CViewC64ScreenWrapper : public CGuiView
{
public:
	CViewC64ScreenWrapper(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64ScreenWrapper();

	virtual void Render();
	virtual void Render(float posX, float posY);
	//virtual void Render(float posX, float posY, float sizeX, float sizeY);
	virtual void DoLogic();

	virtual void RenderImGui();
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);

	virtual bool DoRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool DoScrollWheel(float deltaX, float deltaY);
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual bool DoGamePadButtonDown(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadButtonUp(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats

	virtual void ActivateView();
	virtual void DeactivateView();
	
	CDebugInterfaceC64 *debugInterface;
	
	void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	void SetSelectedScreenMode(u8 newScreenMode);
	u8 selectedScreenMode;
	
	//
	void UpdateC64ScreenPosition();
	void RenderRaster(int rasterX, int rasterY);

};

#endif //_CVIEWC64SCREEN_H_
