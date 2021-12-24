#include "CVicEditorLayerC64Screen.h"
#include "CViewC64.h"
#include "CViewC64Screen.h"
#include "CViewVicEditor.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerImage.h"
#include "CDebugInterfaceC64.h"
#include "VID_Main.h"

CVicEditorLayerC64Screen::CVicEditorLayerC64Screen(CViewVicEditor *vicEditor)
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

	viewC64Screen->RefreshScreen();
	
	// nearest neighbour
	{
		glBindTexture(GL_TEXTURE_2D, viewC64Screen->imageScreen->textureId);
		
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	}
	
	if (vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
		|| vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_NONE)
	{
		Blit(viewC64Screen->imageScreen,
			 vicEditor->viewVicDisplayMain->visibleScreenPosX,
			 vicEditor->viewVicDisplayMain->visibleScreenPosY, -1,
			 vicEditor->viewVicDisplayMain->visibleScreenSizeX,
			 vicEditor->viewVicDisplayMain->visibleScreenSizeY,
			 0.0f, 1.0f, viewC64Screen->screenTexEndX, viewC64Screen->screenTexEndY);
		
	}
	
//	BlitRectangle(vicEditor->viewVicDisplayMain->displayPosX,
//				  vicEditor->viewVicDisplayMain->displayPosY, -1,
//				  vicEditor->viewVicDisplayMain->displaySizeX,
//				  vicEditor->viewVicDisplayMain->displaySizeY, 1.0f, 0.0f, 0.0f, 1.0f, 1.5f);

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
	CViewC64Screen *viewC64Screen = viewC64->viewC64Screen;

	// nearest neighbour
	{
		glBindTexture(GL_TEXTURE_2D, viewC64Screen->imageScreen->textureId);
		
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	}
	
	if (vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
		|| vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_NONE)
	{
		Blit(viewC64Screen->imageScreen,
			 vicEditor->viewVicDisplaySmall->visibleScreenPosX,
			 vicEditor->viewVicDisplaySmall->visibleScreenPosY, -1,
			 vicEditor->viewVicDisplaySmall->visibleScreenSizeX,
			 vicEditor->viewVicDisplaySmall->visibleScreenSizeY,
			 0.0f, 1.0f, viewC64Screen->screenTexEndX, viewC64Screen->screenTexEndY);
		
	}
}

u8 CVicEditorLayerC64Screen::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return vicEditor->layerC64Canvas->Paint(forceColorReplace, isDither, x, y, colorLMB, colorRMB, colorSource, charValue);
}

bool CVicEditorLayerC64Screen::GetColorAtPixel(int x, int y, u8 *color)
{
	return vicEditor->layerC64Canvas->GetColorAtPixel(x, y, color);
}


void CVicEditorLayerC64Screen::Serialise(CByteBuffer *byteBuffer)
{
	CVicEditorLayer::Serialise(byteBuffer);
}

void CVicEditorLayerC64Screen::Deserialise(CByteBuffer *byteBuffer, int version)
{
	CVicEditorLayer::Deserialise(byteBuffer, version);
}
