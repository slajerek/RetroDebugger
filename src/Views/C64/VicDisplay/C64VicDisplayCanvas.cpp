#include "C64VicDisplayCanvas.h"
#include "CViewC64VicDisplay.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "C64Tools.h"
#include <algorithm>

C64VicDisplayCanvas::C64VicDisplayCanvas(CViewC64VicDisplay *vicDisplay, u8 canvasType, bool isMultiColor, bool isExtendedColor)
{
	this->vicDisplay = vicDisplay;
	this->debugInterface = viewC64->debugInterfaceC64;
	
	this->canvasType = canvasType;
	this->isMultiColor = isMultiColor;
	this->isExtendedColor = isExtendedColor;

	// -1 means no dither mask
	ditherMaskPosX = -1;
	ditherMaskPosY = -1;
}

void C64VicDisplayCanvas::SetViciiState(vicii_cycle_state_t *viciiState)
{
	this->viciiState = viciiState;
}

void C64VicDisplayCanvas::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
										u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	SYS_FatalExit("C64VicDisplayCanvas::RefreshScreen");
}

void C64VicDisplayCanvas::ClearScreen()
{
	LOGError("C64VicDisplayCanvas::ClearScreen: not implemented");
}

void C64VicDisplayCanvas::ClearScreen(u8 charValue, u8 colorValue)
{
	LOGError("C64VicDisplayCanvas::ClearScreen: not implemented");
}

void C64VicDisplayCanvas::RenderGridLines()
{
	RenderCanvasSpecificGridLines();
	
	//LOGD("scale=%f", vicDisplay->scale);
	if (vicDisplay->scale > c64SettingsPaintGridShowValuesZoomLevel)
	{
		RenderCanvasSpecificGridValues();
	}
}

void C64VicDisplayCanvas::RenderCanvasSpecificGridLines()
{
	LOGError("C64VicDisplayCanvas::RenderCanvasSpecificGridLines: not implemented");
}

void C64VicDisplayCanvas::RenderCanvasSpecificGridValues()
{
	LOGError("C64VicDisplayCanvas::RenderCanvasSpecificGridValues: not implemented");
}

u8 C64VicDisplayCanvas::GetColorAtPixel(int x, int y)
{
	LOGError("C64VicDisplayCanvas::GetColorAtPixel: not implemented");
	return 0;
}

u8 C64VicDisplayCanvas::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	LOGError("C64VicDisplayCanvas::PutColorAtPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 C64VicDisplayCanvas::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	LOGError("C64VicDisplayCanvas::PaintDither: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 C64VicDisplayCanvas::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	if (isDither)
	{
		return this->PaintDither(forceColorReplace, x, y, colorLMB, colorRMB, colorSource, charValue);
	}
	else
	{
		this->ClearDitherMask();
		
		return this->PutColorAtPixel(forceColorReplace, x, y, colorLMB, colorRMB, colorSource, charValue);
	}

}

void C64VicDisplayCanvas::ClearDitherMask()
{
	ditherMaskPosX = -1;
	ditherMaskPosY = -1;
}

u8 C64VicDisplayCanvas::ConvertFrom(CImageData *imageData)
{
	LOGError("C64VicDisplayCanvas::ConvertFrom: not implemented");
	return PAINT_RESULT_ERROR;
}

struct colorPairLess_t {	bool operator()(const C64ColorsHistogramElement *a, const C64ColorsHistogramElement *b) const
	{
		return a->num > b->num;
	}
} colorPairLess;

// finds background color (the color that has highest number of appearances)
std::vector<C64ColorsHistogramElement *> *C64VicDisplayCanvas::GetSortedColorsHistogram(CImageData *imageData)
{
	int histogram[16] = { 0 };
	
	for (int x = 0; x < imageData->width; x++)
	{
		for (int y = 0; y < imageData->height; y++)
		{
			int color = imageData->GetPixelResultByte(x, y);
			histogram[color]++;
		}
	}
	
	std::vector<C64ColorsHistogramElement *> *colors = new std::vector<C64ColorsHistogramElement *>();
	for (int i = 0; i < 16; i++)
	{
		C64ColorsHistogramElement *pair = new C64ColorsHistogramElement(i, histogram[i]);
		colors->push_back(pair);
	}
	
	std::sort(colors->begin(), colors->end(), colorPairLess);
	
//	LOGD("... sorted:");
//	for (std::vector<C64ColorPair *>::iterator it = colors->begin(); it != colors->end(); it++)
//	{
//		C64ColorPair *pair = *it;
//		LOGD("  %d %d", pair->num, pair->color);
//	}
	
	return colors;
}

void C64VicDisplayCanvas::DeleteColorsHistogram(std::vector<C64ColorsHistogramElement *> *colors)
{
	while(!colors->empty())
	{
		C64ColorsHistogramElement *pair = colors->back();
		colors->pop_back();
		
		delete pair;
	}
}

// reduces color space to C64 colors only (nearest)
CImageData *C64VicDisplayCanvas::ReducePalette(CImageData *imageData, CViewC64VicDisplay *vicDisplay)
{
	CImageData *imageReducedPalette = new CImageData(imageData->width, imageData->height, IMG_TYPE_GRAYSCALE);
	imageReducedPalette->AllocImage(false, true);
	
	for (int x = 0; x < imageData->width; x++)
	{
		for (int y = 0; y < imageData->height; y++)
		{
			u8 r, g, b, a;
			
			imageData->GetPixelResultRGBA(x, y, &r, &g, &b, &a);
			
			u8 color = FindC64Color(r, g, b, vicDisplay->debugInterface);
			
			imageReducedPalette->SetPixelResultByte(x, y, color);
		}
	}
	
	return imageReducedPalette;
}
