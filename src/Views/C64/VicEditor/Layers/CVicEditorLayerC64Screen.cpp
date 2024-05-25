#include "CVicEditorLayerC64Screen.h"
#include "CViewC64VicEditor.h"
#include "CViewC64.h"
#include "CViewC64Screen.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerImage.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceVice.h"

#define VICII_NORMAL_BORDERS 0
#define VICII_FULL_BORDERS   1
#define VICII_DEBUG_BORDERS  2
#define VICII_NO_BORDERS     3

CVicEditorLayerC64Screen::CVicEditorLayerC64Screen(CViewC64VicEditor *vicEditor)
: CVicEditorLayer(vicEditor, "C64 Screen")
{
	this->isVisible = true;
}

CVicEditorLayerC64Screen::~CVicEditorLayerC64Screen()
{
	
}

void CVicEditorLayerC64Screen::RenderMain(vicii_cycle_state_t *viciiState)
{
	CViewC64Screen *viewC64Screen = viewC64->viewC64Screen;

	if (vicEditor->viewVicDisplay->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
		|| vicEditor->viewVicDisplay->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_NONE)
	{
//		LOGD("CVicEditorLayerC64Screen: display is %f %f %f %f", vicEditor->viewVicDisplay->visibleScreenPosX, vicEditor->viewVicDisplay->visibleScreenPosY, vicEditor->viewVicDisplay->visibleScreenSizeX, vicEditor->viewVicDisplay->visibleScreenSizeY);
		
		// TODO: fix me and generalize (how to generalize properly vice debug borders?)
		CDebugInterfaceVice *debugInterfaceVice = (CDebugInterfaceVice*)vicEditor->debugInterface;
		int borderMode = debugInterfaceVice->GetViciiBorderMode();
		
		int x,y,w,h;
		debugInterfaceVice->GetViciiGeometry(&w, &h, &x, &y);
//		LOGD("x=%d y=%d w=%d h=%d", x, y, w, h);
		
		// Below we are mapping VicDisplay space that was originally based on VICII_NORMAL_BORDERS
		// to different new screen display spaces, depending on the selected debug borders mode.
		// Thus we need to scale and offset screen canvas to fit into VicDisplay's space.
		// Note the screen canvas has already Y offset (skip top lines), as Vice for some reason
		// in some borderModes renders blank black lines on top (e.g. in VICII_NORMAL_BORDERS there
		// are 16 lines that are black on top of the screen canvas, etc.), and we are skipping these already
		// when we render screen on the bitmap canvas.
		
		float px, py, sx, sy;
		if (borderMode == VICII_NORMAL_BORDERS)
		{
			// x=32 y=51 w=384 h=272, skip top lines=16
			px = vicEditor->viewVicDisplay->visibleScreenPosX;
			py = vicEditor->viewVicDisplay->visibleScreenPosY;
			sx = vicEditor->viewVicDisplay->visibleScreenSizeX;
			sy = vicEditor->viewVicDisplay->visibleScreenSizeY;
		}
		else if (borderMode == VICII_FULL_BORDERS)
		{
			// x=48 y=51 w=408 h=293, skip top lines=8
			float ox = -16;
			float oy = 8-16;
			float osx = 408.0f/384.f;
			float osy = 293.0f/272.f;
			px = vicEditor->viewVicDisplay->visibleScreenPosX + ox*vicEditor->viewVicDisplay->rasterScaleFactorX;
			py = vicEditor->viewVicDisplay->visibleScreenPosY + oy*vicEditor->viewVicDisplay->rasterScaleFactorY;
			sx = vicEditor->viewVicDisplay->visibleScreenSizeX * osx;
			sy = vicEditor->viewVicDisplay->visibleScreenSizeY * osy;
		}
		else if (borderMode == VICII_DEBUG_BORDERS)
		{
			// x=136 y=51 w=504 h=312, skip top lines=0
			float ox = -104;
			float oy = -16;
			float osx = 504.0f/384.f;
			float osy = 312.0f/272.f;
			px = vicEditor->viewVicDisplay->visibleScreenPosX + ox*vicEditor->viewVicDisplay->rasterScaleFactorX;
			py = vicEditor->viewVicDisplay->visibleScreenPosY + oy*vicEditor->viewVicDisplay->rasterScaleFactorY;
			sx = vicEditor->viewVicDisplay->visibleScreenSizeX * osx;
			sy = vicEditor->viewVicDisplay->visibleScreenSizeY * osy;
		}
		else
		{
			// VICII_NO_BORDERS
			// x=0 y=51 w=320 h=200, skip top lines=51
			float ox = 32;
			float oy = 51-16;
			float osx = 320.0f/384.f;
			float osy = 200.0f/272.f;
			px = vicEditor->viewVicDisplay->visibleScreenPosX + ox*vicEditor->viewVicDisplay->rasterScaleFactorX;
			py = vicEditor->viewVicDisplay->visibleScreenPosY + oy*vicEditor->viewVicDisplay->rasterScaleFactorY;
			sx = vicEditor->viewVicDisplay->visibleScreenSizeX * osx;
			sy = vicEditor->viewVicDisplay->visibleScreenSizeY * osy;
		}
				
		Blit(viewC64Screen->image,
			 px, py, -1,
			 sx, sy,
			 0.0f, 0.0f, viewC64Screen->renderTextureEndX, viewC64Screen->renderTextureEndY);
//		BlitRectangle(px, py, -1, sx, sy, 1, 0, 0, 1);
	}
	
//	BlitRectangle(vicEditor->viewVicDisplay->displayPosX,
//				  vicEditor->viewVicDisplay->displayPosY, -1,
//				  vicEditor->viewVicDisplay->displaySizeX,
//				  vicEditor->viewVicDisplay->displaySizeY, 1.0f, 0.0f, 0.0f, 1.0f, 1.5f);

}

CImageData *CVicEditorLayerC64Screen::GetScreenImage(int *width, int *height)
{
	CViewC64Screen *viewC64Screen = viewC64->viewC64Screen;

	viewC64Screen->debugInterface->LockRenderScreenMutex();
	
	// refresh texture of C64's screen
	CImageData *imageData = viewC64Screen->debugInterface->GetScreenImageData();

	viewC64Screen->debugInterface->UnlockRenderScreenMutex();

	*width = viewC64Screen->debugInterface->GetScreenSizeX();
	*height = viewC64Screen->debugInterface->GetScreenSizeY();
	CImageData *imgDataSave = new CImageData(*width, *height);
	for (int x = 0; x < *width; x++)
	{
		for (int y = 0; y < *height; y++)
		{
			int tx = x * viewC64Screen->debugInterface->screenSupersampleFactor;
			int ty = y * viewC64Screen->debugInterface->screenSupersampleFactor;

			u8 r,g,b,a;
			imageData->GetPixelResultRGBA(tx, ty, &r, &g, &b, &a);
			imgDataSave->SetPixelResultRGBA(x, y, r, g, b, a);
		}
	}
	
	viewC64Screen->debugInterface->UnlockRenderScreenMutex();

	return imgDataSave;
}

CImageData *CVicEditorLayerC64Screen::GetInteriorScreenImage()
{
	LOGTODO("CVicEditorLayerC64Screen::GetInteriorScreenImage: check vicii borderMode");
	CViewC64Screen *viewC64Screen = viewC64->viewC64Screen;
	
	viewC64Screen->debugInterface->LockRenderScreenMutex();
	
	// refresh texture of C64's screen
	CImageData *imageData = viewC64Screen->debugInterface->GetScreenImageData();
	
	CImageData *imgDataSave = new CImageData(320, 200);
	for (int x = 0; x < 320; x++)
	{
		for (int y = 0; y < 200; y++)
		{
			int tx = (x + C64_SCREEN_OFFSET_X) * viewC64Screen->debugInterface->screenSupersampleFactor;
			int ty = (y + C64_SCREEN_OFFSET_Y) * viewC64Screen->debugInterface->screenSupersampleFactor;
			u8 r,g,b,a;
			imageData->GetPixelResultRGBA(tx, ty, &r, &g, &b, &a);
			imgDataSave->SetPixelResultRGBA(x, y, r, g, b, a);
		}
	}
	
	viewC64Screen->debugInterface->UnlockRenderScreenMutex();

	return imgDataSave;
}


void CVicEditorLayerC64Screen::RenderPreview(vicii_cycle_state_t *viciiState)
{
	LOGError("CVicEditorLayerC64Screen::RenderPreview: not implemented");
//	CViewC64Screen *viewC64Screen = viewC64->viewC64Screen;
//
//	// nearest neighbour
//	LOGTODO("is it required?");
//	viewC64Screen->imageScreen->SetLinearScaling(false);
//	
//	if (vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
//		|| vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_NONE)
//	{
//		Blit(viewC64Screen->imageScreen,
//			 vicEditor->viewVicDisplaySmall->visibleScreenPosX,
//			 vicEditor->viewVicDisplaySmall->visibleScreenPosY, -1,
//			 vicEditor->viewVicDisplaySmall->visibleScreenSizeX,
//			 vicEditor->viewVicDisplaySmall->visibleScreenSizeY,
//			 0.0f, 1.0f, viewC64Screen->screenTexEndX, viewC64Screen->screenTexEndY);
//		
//	}
}

u8 CVicEditorLayerC64Screen::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return vicEditor->layerC64Canvas->Paint(forceColorReplace, isDither, x, y, colorLMB, colorRMB, colorSource, charValue);
}

bool CVicEditorLayerC64Screen::GetColorAtPixel(int x, int y, u8 *color)
{
	return vicEditor->layerC64Canvas->GetColorAtPixel(x, y, color);
}


void CVicEditorLayerC64Screen::Serialize(CByteBuffer *byteBuffer)
{
	CVicEditorLayer::Serialize(byteBuffer);
}

void CVicEditorLayerC64Screen::Deserialize(CByteBuffer *byteBuffer, int version)
{
	CVicEditorLayer::Deserialize(byteBuffer, version);
}
