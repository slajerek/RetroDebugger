#include "CVicEditorLayerImage.h"
#include "VID_Main.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CViewVicEditor.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerC64Canvas.h"
#include "C64Tools.h"

CVicEditorLayerImage::CVicEditorLayerImage(CViewVicEditor *vicEditor, char *layerName)
: CVicEditorLayer(vicEditor, layerName)
{
	CDebugInterfaceC64 *debugInterface = viewC64->debugInterfaceC64;
	
	this->isVisible = false;
	
	int w = 512;
	int h = 512;
	imageData = new CImageData(w, h, IMG_TYPE_RGBA);
	imageData->AllocImage(false, true);
	
	image = new CSlrImage(true, false);
	image->LoadImageForRebinding(imageData, RESOURCE_PRIORITY_STATIC);
	VID_PostImageBinding(image, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);
	
	// c64 screen texture boundaries
	screenTexEndX = (float)debugInterface->GetScreenSizeX() / 512.0f;
	screenTexEndY = 1.0f - (float)debugInterface->GetScreenSizeY() / 512.0f;
	
	this->ClearScreen();

	this->isPaintingLocked = true;

}

CVicEditorLayerImage::~CVicEditorLayerImage()
{
	
}

void CVicEditorLayerImage::RefreshImage()
{
	image->ReBindImage();
}

void CVicEditorLayerImage::RenderMain(vicii_cycle_state_t *viciiState)
{
//	LOGD("CVicEditorLayerImage::RenderMain");
	
	RefreshImage();
	
	// nearest neighbour
	LOGTODO("is it required?");
	image->SetLinearScaling(false);
	
	if (vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
		|| vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_NONE)
	{
		Blit(image,
			 vicEditor->viewVicDisplayMain->visibleScreenPosX,
			 vicEditor->viewVicDisplayMain->visibleScreenPosY, -1,
			 vicEditor->viewVicDisplayMain->visibleScreenSizeX,
			 vicEditor->viewVicDisplayMain->visibleScreenSizeY,
			 0.0f, 1.0f, screenTexEndX, screenTexEndY);
		
	}
	
}

void CVicEditorLayerImage::RenderPreview(vicii_cycle_state_t *viciiState)
{
	RefreshImage();
	
	// nearest neighbour
	LOGTODO("is it required?");
	image->SetLinearScaling(false);

	if (vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA
		|| vicEditor->viewVicDisplayMain->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_NONE)
	{
		Blit(image,
			 vicEditor->viewVicDisplaySmall->visibleScreenPosX,
			 vicEditor->viewVicDisplaySmall->visibleScreenPosY, -1,
			 vicEditor->viewVicDisplaySmall->visibleScreenSizeX,
			 vicEditor->viewVicDisplaySmall->visibleScreenSizeY,
			 0.0f, 1.0f, screenTexEndX, screenTexEndY);
		
	}
}

void CVicEditorLayerImage::ClearScreen()
{
	ClearScreen(0x00, 0x00);
}

void CVicEditorLayerImage::ClearScreen(u8 charValue, u8 colorValue)
{
//	u8 r, g, b;
//	debugInterface->GetCBMColor(paintColor, &r, &g, &b);
//
	
	for (int x = 0; x < 384; x++)
	{
		for (int y = 0; y < 272; y++)
		{
			imageData->SetPixelResultRGBA(x, y, 0, 0, 0, 0);
		}
	}
}

int CVicEditorLayerImage::NumVisiblePixels()
{
	int numPixels = 0;
	for (int x = 0; x < 384; x++)
	{
		for (int y = 0; y < 272; y++)
		{
			u8 r,g,b,a;
			imageData->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
			if (a != 0)
			{
				numPixels++;
			}
		}
	}
	
	return numPixels;
}

u8 CVicEditorLayerImage::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	if (isDither)
	{
		return this->PaintDither(x, y, colorLMB, colorRMB, colorSource);
	}
	else
	{
		this->ClearDitherMask();
		
		return this->PutColorAtPixel(x, y, colorLMB, colorRMB, colorSource);
	}
}

bool CVicEditorLayerImage::GetColorAtPixel(int x, int y, u8 *color)
{
	u8 r, g, b, a;

	int x2 = x+C64_SCREEN_OFFSET_X;
	int y2 = y+C64_SCREEN_OFFSET_Y;

	if (x2 < 0 || x2 > 383
		|| y2 < 0 || y2 > 271)
		return false;

	imageData->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
	
	*color = FindC64Color(r, g, b, vicEditor->viewVicDisplayMain->debugInterface);

	return true;
}

u8 CVicEditorLayerImage::PutPixelImage(int x, int y, u8 paintColor)
{
	LOGF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PutPixelImage %d %d %02x", x, y, paintColor);
	
	int x2 = x+C64_SCREEN_OFFSET_X;
	int y2 = y+C64_SCREEN_OFFSET_Y;
	
	if (x2 < 0 || x2 > 383
		|| y2 < 0 || y2 > 271)
		return PAINT_RESULT_OUTSIDE;
	
	u8 result = PAINT_RESULT_OK;

	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;

	u8 r, g, b;
	debugInterface->GetCBMColor(paintColor, &r, &g, &b);
	
	imageData->SetPixelResultRGBA(x2, y2, r, g, b, 255);
	
	return result;
}

u8 CVicEditorLayerImage::PaintDither(int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	u8 color1, color2;
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		color1 = colorLMB;
		color2 = colorRMB;
	}
	else if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
	{
		color1 = colorRMB;
		color2 = colorLMB;
	}
	
	u8 paintColor = color1;
	
	// check if starting dither
	{
		LOGD("													---- isAltPressed: dither -----");
		{
			if (ditherMaskPosX == -1 || ditherMaskPosY == -1)
			{
				LOGD("******** START DITHER ********");
				// start dither painting
				if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
				{
					ditherMaskPosX = x;
				}
				else if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
				{
					ditherMaskPosX = x;
				}
				
				ditherMaskPosY = y;
			}
			
			int dX = abs(x - ditherMaskPosX) % 2;
			int dY = abs(y - ditherMaskPosY) % 2;
			
			int d = (dX + dY) % 2;
			
			LOGD("==================== dX=%d dY=%d d=%d", dX, dY, d);
			
			if (d != 0)
			{
				paintColor = color2;
			}
		}
	}
	
	return this->PutPixelImage(x, y, paintColor);
	
}

// @returns painting status (ok, replaced color, blocked)
u8 CVicEditorLayerImage::PutColorAtPixel(int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		return this->PutPixelImage(x, y, colorLMB);
	}
	else
	{
		return this->PutPixelImage(x, y, colorRMB);
	}
}

u8 CVicEditorLayerImage::PutPixelImage(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	LOGF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PutPixelImage %d %d %02x%02x%02x%02x", x, y, r, g, b, a);
	
	int x2 = x+C64_SCREEN_OFFSET_X;
	int y2 = y+C64_SCREEN_OFFSET_Y;
	
	if (x2 < 0 || x2 > 383
		|| y2 < 0 || y2 > 271)
		return PAINT_RESULT_OUTSIDE;
	
	u8 result = PAINT_RESULT_OK;
	
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	imageData->SetPixelResultRGBA(x2, y2, r, g, b, a);
	
	return result;
}


void CVicEditorLayerImage::ClearDitherMask()
{
	ditherMaskPosX = -1;
	ditherMaskPosY = -1;
}

bool CVicEditorLayerImage::LoadFrom(CImageData *imageDataLoad)
{
	if (imageDataLoad->width == 384 && imageDataLoad->height == 272)
	{
		for (int x = 0; x < 384; x++)
		{
			for (int y = 0; y < 272; y++)
			{
				u8 r,g,b,a;
				imageDataLoad->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
				imageData->SetPixelResultRGBA(x, y, r, g, b, a);
			}
		}
		return true;
	}
	else if (imageDataLoad->width == 320 && imageDataLoad->height == 200)
	{
		u8 *data = imageData->GetResultDataAsRGBA();
		memset(data, 0x00, 384*272*4);
		
		for (int x = 0; x < 320; x++)
		{
			for (int y = 0; y < 200; y++)
			{
				u8 r,g,b,a;
				imageDataLoad->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
				imageData->SetPixelResultRGBA(x + C64_SCREEN_OFFSET_X, y + C64_SCREEN_OFFSET_Y, r, g, b, a);
			}
		}

		return true;
	}
	return false;
}

CImageData *CVicEditorLayerImage::GetFullDisplayImage()
{
	CImageData *imgDataSave = new CImageData(384, 272);
	for (int x = 0; x < 384; x++)
	{
		for (int y = 0; y < 272; y++)
		{
			u8 r,g,b,a;
			imageData->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
			imgDataSave->SetPixelResultRGBA(x, y, r, g, b, a);
		}
	}
	
	return imgDataSave;
}

CImageData *CVicEditorLayerImage::GetScreenImage()
{
	CImageData *imgDataSave = new CImageData(320, 200);
	for (int x = 0; x < 320; x++)
	{
		for (int y = 0; y < 200; y++)
		{
			u8 r,g,b,a;
			imageData->GetPixelResultRGBA(x + C64_SCREEN_OFFSET_X, y + C64_SCREEN_OFFSET_Y, &r, &g, &b, &a);
			imgDataSave->SetPixelResultRGBA(x, y, r, g, b, a);
		}
	}
	
	return imgDataSave;
}


void CVicEditorLayerImage::LayerSelected(bool isSelected)
{
	if (isSelected)
	{
		isPaintingLocked = false;
	}
	else
	{
		isPaintingLocked = true;
	}
}

void CVicEditorLayerImage::Serialise(CByteBuffer *byteBuffer)
{
	CVicEditorLayer::Serialise(byteBuffer);

	if (this->NumVisiblePixels() > 0)
	{
		byteBuffer->PutBool(true);
		u8 *data = imageData->GetResultDataAsRGBA();
		byteBuffer->PutBytes(data, 512*512*4);
	}
	else
	{
		byteBuffer->PutBool(false);
	}
}

void CVicEditorLayerImage::Deserialise(CByteBuffer *byteBuffer, int version)
{
	if (version < 3)
	{
		// there was no reference layer in versions prior 3
		return;
	}
	
	CVicEditorLayer::Deserialise(byteBuffer, version);

	bool withPixels = byteBuffer->GetBool();
	if (withPixels)
	{
		u8 *data = imageData->GetResultDataAsRGBA();
		byteBuffer->GetBytes(data, 512*512*4);
	}
}
