#include "CViewC64.h"
#include "CViewBreakpoints.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "CColorsTheme.h"
#include "C64SettingsStorage.h"
#include "SYS_Main.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CDebugInterface.h"
#include "MTH_Random.h"
#include "VID_ImageBinding.h"
#include "CViewMemoryMap.h"
#include "CGuiMain.h"
#include "CGuiEvent.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"

#include <string>
#include <sstream>
#include "hjson.h"

CViewBreakpoints::CViewBreakpoints(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugSymbols *symbols, int breakpointType)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->symbols = symbols;
	this->breakpointType = breakpointType;
	
	imGuiNoWindowPadding = false;
}

CViewBreakpoints::~CViewBreakpoints()
{
}

void CViewBreakpoints::Run()
{
}

void CViewBreakpoints::DoLogic()
{
	
}

void CViewBreakpoints::Render()
{
}

void CViewBreakpoints::RenderImGui()
{
//	LOGD("CViewBreakpoints::RenderImGui");
		
//	float w = (float)imageDataScreen->width;
//	float h = (float)imageDataScreen->height;
//
//	this->imGuiWindowAspectRatio = w/h;
//	this->imGuiWindowKeepAspectRatio = true;
//
	PreRenderImGui();
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsByType[breakpointType];
	breakpoints->RenderImGui();
	
	PostRenderImGui();
	
//	LOGD("CViewBreakpoints::RenderImGui done");
}

void CViewBreakpoints::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewBreakpoints::DoTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoTap:  x=%f y=%f", x, y);
	
	return true; //CGuiView::DoTap(x, y);
}

bool CViewBreakpoints::DoFinishTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoFinishTap: %f %f", x, y);

	return true; //CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewBreakpoints::DoDoubleTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewBreakpoints::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewBreakpoints::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewBreakpoints::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewBreakpoints::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewBreakpoints::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewBreakpoints::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewBreakpoints::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewBreakpoints::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewBreakpoints::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewBreakpoints::KeyDown: keyCode=%d");

	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewBreakpoints::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewBreakpoints::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewBreakpoints::KeyUp: keyCode=%d", keyCode);
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewBreakpoints::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewBreakpoints::ActivateView()
{
	LOGG("CViewBreakpoints::ActivateView()");
}

void CViewBreakpoints::DeactivateView()
{
	LOGG("CViewBreakpoints::DeactivateView()");
}

