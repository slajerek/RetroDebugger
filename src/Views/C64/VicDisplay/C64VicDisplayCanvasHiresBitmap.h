#ifndef _C64VICDISPLAYCANVASHIRESBITMAP_H_
#define _C64VICDISPLAYCANVASHIRESBITMAP_H_

#include "C64VicDisplayCanvas.h"

class C64CharHires;

class C64VicDisplayCanvasHiresBitmap : public C64VicDisplayCanvas
{
public:
	C64VicDisplayCanvasHiresBitmap(CViewC64VicDisplay *vicDisplay);
	
	C64CharHires *GetBitCharHiresBitmap(int x, int y);
	u8 GetBitPixelHiresBitmap(int x, int y);
	void ReplaceColorHiresBitmapRaster(int x, int y, u8 colorNum, u8 paintColor);
	void ReplaceColorHiresBitmapCharacter(int charColumn, int charRow, u8 colorNum, u8 paintColor);
	void PutBitPixelHiresBitmapRaster(int x, int y, u8 bitColor);
	
	virtual u8 GetColorAtPixel(int x, int y);
	
	u8 PutPixelHiresBitmap(bool forceColorReplace, int x, int y, u8 paintColor);
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 paintColor);
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);

	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	
	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	
	virtual void RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen, u8 backgroundColorAlpha, u8 foregroundColorAlpha);
	
	virtual void RenderCanvasSpecificGridLines();
	virtual void RenderCanvasSpecificGridValues();
	
	virtual void ClearScreen();
	virtual void ClearScreen(u8 charValue, u8 colorValue);

	virtual u8 ConvertFrom(CImageData *imageData);

};

#endif
