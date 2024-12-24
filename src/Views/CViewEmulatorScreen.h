#ifndef _CViewEmulatorScreen_h_
#define _CViewEmulatorScreen_h_

#include "CGuiViewMovingPaneImage.h"

class CSlrMutex;
class CDebugInterface;
class CGamePad;

class CViewEmulatorScreen : public CGuiViewMovingPaneImage
{
public:
	CViewEmulatorScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewEmulatorScreen();
	
	CDebugInterface *debugInterface;
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);
	virtual bool DoRightClick(float x, float y);
	virtual bool DoFinishRightClick(float x, float y);
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual void RefreshEmulatorScreenImageData();
	virtual bool UpdateImageData();
	
	virtual void RefreshImage();
	virtual void RefreshImageParameters();
	
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
	
	virtual bool DoGamePadButtonDown(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadButtonUp(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value);
	virtual int  GetJoystickAxis(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual void JoystickDown(int port, u32 axis);
	virtual void JoystickUp(int port, u32 axis);
	
	// skip key and do not send to debugInterface?
	virtual bool IsSkipKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	// convert keycode before sending to debugInterface
	virtual u32 ConvertKeyCode(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual void PostDebugInterfaceKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual void PostDebugInterfaceKeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual void SetSupersampleFactor(int supersampleFactor);
};

#endif
