#include "CGuiMain.h"
#include "CViewC64ColorRamScreen.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64StateVIC.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"

CViewC64ColorRamScreen::CViewC64ColorRamScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;
	
	imGuiWindowAspectRatio = 320.0f / 200.0f;
	imGuiWindowKeepAspectRatio = true;
	
	// alloc image that will store charset pixels
	colorImageData = new CImageData(64, 32, IMG_TYPE_RGBA);
	colorImageData->AllocImage(false, true);
	
	colorImage = new CSlrImage(true, false);
	colorImage->LoadImageForRebinding(colorImageData, RESOURCE_PRIORITY_STATIC);
	colorImage->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	colorImage->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(colorImage, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);
}

CViewC64ColorRamScreen::~CViewC64ColorRamScreen()
{
}

void CViewC64ColorRamScreen::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64ColorRamScreen::RenderImGui()
{
	PreRenderImGui();

	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	u8 *screen_ptr, *color_ram_ptr, *chargen_ptr, *bitmap_low_ptr, *bitmap_high_ptr;
	u8 colors[0x0F];
	
	viewC64->viewC64VicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

	u8 fgcolor;
	u8 fgColorR, fgColorG, fgColorB;
	
	for (int i = 0; i < 25; i++)
	{
		for (int j = 0; j < 40; j++)
		{
			fgcolor = color_ram_ptr[(i * 40) + j] & 0xf;
			viewC64->debugInterfaceC64->GetCBMColor(fgcolor, &fgColorR, &fgColorG, &fgColorB);
			
			//LOGD("i=%d j=%d %d | %d %d %d", i, j, fgcolor, fgColorR, fgColorG, fgColorB);
			colorImageData->SetPixelResultRGBA(j, i, fgColorR, fgColorG, fgColorB, 255);
		}
	}
	
	colorImage->ReBindImage();
	
	// nearest neighbour
	const float itex = 40.0f/64.0f;
	const float itey = 25.0f/32.0f;

	Blit(colorImage, posX, posY, -1, sizeX, sizeY, 0, 0, itex, itey);
//	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 1.0, 0.0, 0.0f, 0.7f);

	PostRenderImGui();
}

//@returns is consumed
bool CViewC64ColorRamScreen::DoTap(float x, float y)
{
	LOGG("CViewC64ColorRamScreen::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewC64ColorRamScreen::DoFinishTap(float x, float y)
{
	LOGG("CViewC64ColorRamScreen::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64ColorRamScreen::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64ColorRamScreen::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64ColorRamScreen::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64ColorRamScreen::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64ColorRamScreen::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64ColorRamScreen::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64ColorRamScreen::DoRightClick(float x, float y)
{
	return CGuiView::DoRightClick(x, y);
}

bool CViewC64ColorRamScreen::DoFinishRightClick(float x, float y)
{
	return CGuiView::CGuiElement::DoFinishRightClick(x, y);
}

bool CViewC64ColorRamScreen::DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoRightClickMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64ColorRamScreen::FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::CGuiElement::FinishRightClickMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64ColorRamScreen::DoNotTouchedMove(float x, float y)
{
	return CGuiView::DoNotTouchedMove(x, y);
}

bool CViewC64ColorRamScreen::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64ColorRamScreen::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64ColorRamScreen::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewC64ColorRamScreen::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64ColorRamScreen::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64ColorRamScreen::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64ColorRamScreen::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64ColorRamScreen::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64ColorRamScreen::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64ColorRamScreen::DoGamePadButtonDown(CGamePad *gamePad, u8 button)
{
	return CGuiView::DoGamePadButtonDown(gamePad, button);
}

bool CViewC64ColorRamScreen::DoGamePadButtonUp(CGamePad *gamePad, u8 button)
{
	return CGuiView::DoGamePadButtonUp(gamePad, button);
}

bool CViewC64ColorRamScreen::DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value)
{
	return CGuiView::DoGamePadAxisMotion(gamePad, axis, value);
}

bool CViewC64ColorRamScreen::HasContextMenuItems()
{
	return false;
}

void CViewC64ColorRamScreen::RenderContextMenuItems()
{
}

void CViewC64ColorRamScreen::ActivateView()
{
	LOGG("CViewC64ColorRamScreen::ActivateView()");
}

void CViewC64ColorRamScreen::DeactivateView()
{
	LOGG("CViewC64ColorRamScreen::DeactivateView()");
}
