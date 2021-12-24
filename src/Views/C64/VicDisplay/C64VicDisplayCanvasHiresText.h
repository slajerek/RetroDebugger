#ifndef _C64VicDisplayCanvasHiresText_H_
#define _C64VicDisplayCanvasHiresText_H_

#include "C64VicDisplayCanvas.h"

class C64VicDisplayCanvasHiresText : public C64VicDisplayCanvas
{
public:
	C64VicDisplayCanvasHiresText(CViewC64VicDisplay *vicDisplay);
	
	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	
	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	
	virtual u8 PutCharacterAtRaster(int x, int y, u8 color, int charValue);
	virtual u8 PutCharacterAt(int x, int y, u8 color, int charValue);

	virtual void RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen, u8 backgroundColorAlpha, u8 foregroundColorAlpha);

	virtual void RenderCanvasSpecificGridLines();
	virtual void RenderCanvasSpecificGridValues();
	
	virtual void ClearScreen();
	virtual void ClearScreen(u8 charValue, u8 colorValue);

	virtual u8 ConvertFrom(CImageData *imageData);
};

#endif
