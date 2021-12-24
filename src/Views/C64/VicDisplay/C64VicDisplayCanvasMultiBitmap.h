#ifndef _C64VICDISPLAYCANVASMULTIBITMAP_H_
#define _C64VICDISPLAYCANVASMULTIBITMAP_H_

#include "C64VicDisplayCanvas.h"

class C64CharMulti;

class C64VicDisplayCanvasMultiBitmapMissingIndexes
{
public:
	int xc, yc, index;
};

class C64VicDisplayCanvasMultiBitmap : public C64VicDisplayCanvas
{
public:
	C64VicDisplayCanvasMultiBitmap(CViewC64VicDisplay *vicDisplay);

	
	C64CharMulti *GetBitCharMultiBitmap(int x, int y);
	u8 GetBitPixelMultiBitmap(int x, int y);
	void ReplaceColorMultiBitmapRaster(int x, int y, u8 colorNum, u8 paintColor);
	void ReplaceColorMultiBitmapCharacter(int charColumn, int charRow, u8 colorNum, u8 paintColor);
	void PutBitPixelMultiBitmap(int x, int y, u8 bitColor);

	virtual u8 GetColorAtPixel(int x, int y);
	
	u8 PutPixelMultiBitmap(bool forceColorReplace, int x, int y, u8 paintColor);
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 paintColor);
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);

	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	
	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);

	virtual void RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
							   u8 backgroundColorAlpha, u8 foregroundColorAlpha);

	virtual void RenderCanvasSpecificGridLines();
	virtual void RenderCanvasSpecificGridValues();

	virtual void ClearScreen();
	virtual void ClearScreen(u8 charValue, u8 colorValue);

	virtual u8 ConvertFrom(CImageData *imageData);
	virtual u8 ConvertFromWithForcedColors(CImageData *imageData, u8 backgroundColor, u8 colorForeground1, u8 colorForeground2, u8 colorForeground3);
};

#endif
