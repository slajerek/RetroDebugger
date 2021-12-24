#include "CViewC64ScreenWrapper.h"
#include "CColorsTheme.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterfaceC64.h"
#include "C64SettingsStorage.h"
#include "C64ColodoreScreen.h"
#include "C64KeyboardShortcuts.h"
#include "CViewC64Screen.h"
#include "CViewC64VicDisplay.h"

CViewC64ScreenWrapper::CViewC64ScreenWrapper(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	selectedScreenMode = C64SCREENWRAPPER_MODE_C64_SCREEN;
}

CViewC64ScreenWrapper::~CViewC64ScreenWrapper()
{
}

void CViewC64ScreenWrapper::DoLogic()
{
//	CGuiView::DoLogic();
}

void CViewC64ScreenWrapper::Render()
{
//	LOGD("CViewC64ScreenWrapper::Render");

	if (selectedScreenMode == C64SCREENWRAPPER_MODE_C64_SCREEN)
	{
		viewC64->viewC64Screen->Render();
	}
	else if (selectedScreenMode == C64SCREENWRAPPER_MODE_C64_DISPLAY)
	{
		VID_SetClipping(posX, posY, sizeX, sizeY);
		viewC64->viewC64VicDisplay->Render();
		VID_ResetClipping();
	}
}

void CViewC64ScreenWrapper::RenderImGui()
{
	viewC64->viewC64Screen->visible = this->visible;
	viewC64->viewC64Screen->imGuiWindowAspectRatio = this->imGuiWindowAspectRatio;
	viewC64->viewC64Screen->imGuiWindowKeepAspectRatio = this->imGuiWindowKeepAspectRatio;

	viewC64->viewC64Screen->RenderImGui();
	
	this->visible = viewC64->viewC64Screen->visible;
	this->imGuiWindowAspectRatio = viewC64->viewC64Screen->imGuiWindowAspectRatio;
	this->imGuiWindowKeepAspectRatio = viewC64->viewC64Screen->imGuiWindowKeepAspectRatio;
}

void CViewC64ScreenWrapper::RenderRaster(int rasterX, int rasterY)
{
	if (viewC64->isShowingRasterCross &&
		(selectedScreenMode == C64SCREENWRAPPER_MODE_C64_SCREEN
		 || selectedScreenMode == C64SCREENWRAPPER_MODE_C64_DISPLAY))
	{
		viewC64->viewC64Screen->RenderRaster(rasterX, rasterY);
	}
	
	if (selectedScreenMode == C64SCREENWRAPPER_MODE_C64_ZOOMED)
	{
		viewC64->viewC64Screen->RenderZoomedScreen(rasterX, rasterY);
	}

}


void CViewC64ScreenWrapper::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	UpdateC64ScreenPosition();
}

void CViewC64ScreenWrapper::UpdateC64ScreenPosition()
{
	viewC64->viewC64Screen->SetPosition(posX, posY, sizeX, sizeY);
	viewC64->viewC64Screen->UpdateRasterCrossFactors();
	
//	if (viewC64->currentScreenLayoutId != SCREEN_LAYOUT_C64_VIC_DISPLAY)
//	{
//		float scaleFactor = 1.35995; //1.575f;
//		
//		float scale = scaleFactor * sizeX / (float)debugInterface->GetScreenSizeX();
//		float offsetX = -24.23*scale;
//		float offsetY = 0;
//		viewC64->viewC64VicDisplay->SetDisplayPosition(posX + offsetX, posY + offsetY, scale, false);
//		
//		viewC64->viewC64VicDisplay->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA);
//	}
//	else
	{
		viewC64->viewC64VicDisplay->SetShowDisplayBorderType(c64SettingsVicDisplayBorderType);
	}
}

void CViewC64ScreenWrapper::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewC64ScreenWrapper::DoTap(float x, float y)
{
	LOGG("CViewC64ScreenWrapper::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewC64ScreenWrapper::DoFinishTap(float x, float y)
{
	LOGG("CViewC64ScreenWrapper::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64ScreenWrapper::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64ScreenWrapper::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64ScreenWrapper::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64ScreenWrapper::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewC64ScreenWrapper::DoRightClick(float x, float y)
{
	if (IsInside(x, y))
	{
		if (selectedScreenMode == C64SCREENWRAPPER_MODE_C64_SCREEN)
		{
			this->SetSelectedScreenMode(C64SCREENWRAPPER_MODE_C64_DISPLAY);
		}
		else if (selectedScreenMode == C64SCREENWRAPPER_MODE_C64_ZOOMED)
		{
			this->SetSelectedScreenMode(C64SCREENWRAPPER_MODE_C64_SCREEN);
		}
		else //if (selectedScreenMode == C64SCREENWRAPPER_MODE_C64_DISPLAY)
		{
			this->SetSelectedScreenMode(C64SCREENWRAPPER_MODE_C64_SCREEN);
		}
		
		UpdateC64ScreenPosition();
		
		return true;
	}

	return false;
}

void CViewC64ScreenWrapper::SetSelectedScreenMode(u8 newScreenMode)
{
	this->selectedScreenMode = newScreenMode;
}

bool CViewC64ScreenWrapper::DoScrollWheel(float deltaX, float deltaY)
{
	if (this->selectedScreenMode == C64SCREENWRAPPER_MODE_C64_ZOOMED)
	{
		return viewC64->viewC64Screen->DoScrollWheel(deltaX, deltaY);
	}
	
	return false;
}


bool CViewC64ScreenWrapper::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64ScreenWrapper::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64ScreenWrapper::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64ScreenWrapper::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64ScreenWrapper::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64ScreenWrapper::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64ScreenWrapper::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64ScreenWrapper::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewC64ScreenWrapper::KeyDown: keyCode=%d isShift=%s isAlt=%s isControl=%s", keyCode,
		 STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl));
	
	return viewC64->viewC64Screen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
	
	//return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}



bool CViewC64ScreenWrapper::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewC64ScreenWrapper::KeyUp: keyCode=%d isShift=%s isAlt=%s isControl=%s", keyCode,
		 STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl));
	
	viewC64->viewC64Screen->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);

	return true;

//	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64ScreenWrapper::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64ScreenWrapper::DoGamePadButtonDown(CGamePad *gamePad, u8 button)
{
	return viewC64->viewC64Screen->DoGamePadButtonDown(gamePad, button);
}

bool CViewC64ScreenWrapper::DoGamePadButtonUp(CGamePad *gamePad, u8 button)
{
	return viewC64->viewC64Screen->DoGamePadButtonUp(gamePad, button);
}

bool CViewC64ScreenWrapper::DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value)
{
	return viewC64->viewC64Screen->DoGamePadAxisMotion(gamePad, axis, value);
}

void CViewC64ScreenWrapper::ActivateView()
{
	LOGG("CViewC64ScreenWrapper::ActivateView()");
	
	this->UpdateC64ScreenPosition();
}

void CViewC64ScreenWrapper::DeactivateView()
{
	LOGG("CViewC64ScreenWrapper::DeactivateView()");
}
