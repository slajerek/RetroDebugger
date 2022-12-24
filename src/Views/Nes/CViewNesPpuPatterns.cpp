#include "CViewNesPpuPatterns.h"
#include "CColorsTheme.h"
#include "GUI_Main.h"
#include "CGuiMain.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterfaceNes.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"

CViewNesPpuPatterns::CViewNesPpuPatterns(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	int w = 512 * debugInterface->screenSupersampleFactor;
	int h = 512 * debugInterface->screenSupersampleFactor;
	imageDataScreenDefault = new CImageData(w, h, IMG_TYPE_RGBA);
	imageDataScreenDefault->AllocImage(false, true);
	
	//	for (int x = 0; x < w; x++)
	//	{
	//		for (int y = 0; y < h; y++)
	//		{
	//			imageDataScreen->SetPixelResultRGBA(x, y, x % 255, y % 255, 0, 255);
	//		}
	//	}
	
	
	imageScreenDefault = new CSlrImage(true, true);
	imageScreenDefault->LoadImage(imageDataScreenDefault, RESOURCE_PRIORITY_STATIC, false);
	imageScreenDefault->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	imageScreenDefault->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(imageScreenDefault, NULL);
	
	imageScreen = imageScreenDefault;
	
	// screen texture boundaries
	screenTexEndX = (float)debugInterface->GetScreenSizeX() / 512.0f;
	screenTexEndY = 1.0f - (float)debugInterface->GetScreenSizeY() / 512.0f;

	this->showGridLines = true;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}


CViewNesPpuPatterns::~CViewNesPpuPatterns()
{
}

void CViewNesPpuPatterns::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewNesPpuPatterns::RefreshScreen()
{
//	LOGD("CViewNesPpuPatterns::RefreshScreen");
	
	debugInterface->LockRenderScreenMutex();
	
	// refresh texture of Nes screen
	CImageData *screen = debugInterface->GetScreenImageData();
	
#if !defined(FINAL_RELEASE)
	if (screen == NULL)
	{
		//LOGError("CViewNesPpuPatterns::RefreshScreen: screen is NULL!");
		debugInterface->UnlockRenderScreenMutex();
		return;
	}
#endif
	
	imageScreen->SetLoadImageData(screen);
	imageScreen->ReBindImage();
	imageScreen->loadImageData = NULL;
	
	debugInterface->UnlockRenderScreenMutex();
}


void CViewNesPpuPatterns::Render()
{
//	LOGD("CViewNesPpuPatterns::Render");
	// render texture of Nes screen
	
//	BlitFilledRectangle(posX, posY, -1, 50, 50, 1.0f, 0.0f, 0, 1);

	RefreshScreen();
	
	imageScreen->SetLinearScaling(!c64SettingsRenderScreenNearest);
	
	Blit(imageScreen,
		 posX,
		 posY, -1,
		 sizeX,
		 sizeY,
		 0.0f, 1.0f, screenTexEndX, screenTexEndY);
	
//	if (showGridLines)
//	{
//		// raster screen in hex:
//		// startx = 68 (88) endx = 1e8 (1c8)
//		// starty = 10 (32) endy = 120 ( fa)
//
//		float cys = posY + (float)0x0010 * rasterScaleFactorY  + rasterCrossOffsetY;
//		float cye = posY + (float)0x0120 * rasterScaleFactorY  + rasterCrossOffsetY;
//
//		float cxs = posX + (float)0x0068 * rasterScaleFactorX  + rasterCrossOffsetX;
//		float cxe = posX + (float)0x01E8 * rasterScaleFactorX  + rasterCrossOffsetX;
//
//
//		// vertical lines
//		for (float rasterX = 103.5f; rasterX < 0x01e8; rasterX += 0x08)
//		{
//			float cx = posX + (float)rasterX * rasterScaleFactorX  + rasterCrossOffsetX;
//
//			BlitLine(cx, cys, cx, cye, -1,
//					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
//		}
//
//		// horizontal lines
//		for (float rasterY = 18.5f; rasterY < 0x0120; rasterY += 0x08)
//		{
//			float cy = posY + (float)rasterY * rasterScaleFactorY  + rasterCrossOffsetY;
//
//			BlitLine(cxs, cy, cxe, cy, -1,
//					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
//		}
//	}
	
}

void CViewNesPpuPatterns::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewNesPpuPatterns::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewNesPpuPatterns::DoTap(float x, float y)
{
	LOGG("CViewNesPpuPatterns::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewNesPpuPatterns::DoFinishTap(float x, float y)
{
	LOGG("CViewNesPpuPatterns::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewNesPpuPatterns::DoDoubleTap(float x, float y)
{
	LOGG("CViewNesPpuPatterns::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewNesPpuPatterns::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewNesPpuPatterns::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewNesPpuPatterns::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewNesPpuPatterns::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewNesPpuPatterns::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewNesPpuPatterns::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewNesPpuPatterns::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewNesPpuPatterns::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewNesPpuPatterns::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewNesPpuPatterns::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewNesPpuPatterns::KeyDown: keyCode=%d", keyCode);
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewNesPpuPatterns::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewNesPpuPatterns::KeyUp: keyCode=%d", keyCode);
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewNesPpuPatterns::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewNesPpuPatterns::ActivateView()
{
	LOGG("CViewNesPpuPatterns::ActivateView()");
}

void CViewNesPpuPatterns::DeactivateView()
{
	LOGG("CViewNesPpuPatterns::DeactivateView()");
}
