#include "CViewDrive1541Led.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CGuiMain.h"

CViewDrive1541Led::CViewDrive1541Led(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;
}

CViewDrive1541Led::~CViewDrive1541Led()
{
}

void CViewDrive1541Led::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewDrive1541Led::RenderImGui()
{
	PreRenderImGui();
	
	int driveId = 0;
	float colorGreen = debugInterface->ledGreenPwm[driveId];
	float colorRed = viewC64->debugInterfaceC64->ledRedPwm[driveId];

	
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY,
						colorRed, colorGreen, 0.0f, 1.0f);

	PostRenderImGui();
}

//@returns is consumed
bool CViewDrive1541Led::DoTap(float x, float y)
{
	LOGG("CViewDrive1541Led::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewDrive1541Led::DoFinishTap(float x, float y)
{
	LOGG("CViewDrive1541Led::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewDrive1541Led::DoDoubleTap(float x, float y)
{
	LOGG("CViewDrive1541Led::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewDrive1541Led::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewDrive1541Led::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewDrive1541Led::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewDrive1541Led::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewDrive1541Led::DoRightClick(float x, float y)
{
	return CGuiView::DoRightClick(x, y);
}

bool CViewDrive1541Led::DoFinishRightClick(float x, float y)
{
	return CGuiView::CGuiElement::DoFinishRightClick(x, y);
}

bool CViewDrive1541Led::DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoRightClickMove(x, y, distX, distY, diffX, diffY);
}

bool CViewDrive1541Led::FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::CGuiElement::FinishRightClickMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewDrive1541Led::DoNotTouchedMove(float x, float y)
{
	return CGuiView::DoNotTouchedMove(x, y);
}

bool CViewDrive1541Led::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewDrive1541Led::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewDrive1541Led::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewDrive1541Led::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewDrive1541Led::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewDrive1541Led::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewDrive1541Led::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541Led::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541Led::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541Led::DoGamePadButtonDown(CGamePad *gamePad, u8 button)
{
	return CGuiView::DoGamePadButtonDown(gamePad, button);
}

bool CViewDrive1541Led::DoGamePadButtonUp(CGamePad *gamePad, u8 button)
{
	return CGuiView::DoGamePadButtonUp(gamePad, button);
}

bool CViewDrive1541Led::DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value)
{
	return CGuiView::DoGamePadAxisMotion(gamePad, axis, value);
}

bool CViewDrive1541Led::HasContextMenuItems()
{
	return false;
}

void CViewDrive1541Led::RenderContextMenuItems()
{
}

void CViewDrive1541Led::ActivateView()
{
	LOGG("CViewDrive1541Led::ActivateView()");
}

void CViewDrive1541Led::DeactivateView()
{
	LOGG("CViewDrive1541Led::DeactivateView()");
}
