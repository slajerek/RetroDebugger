#ifndef _C64VICDISPLAYCANVASBLANK_H_
#define _C64VICDISPLAYCANVASBLANK_H_

#include "C64VicDisplayCanvas.h"

class C64VicDisplayCanvasBlank : public C64VicDisplayCanvas
{
public:
	C64VicDisplayCanvasBlank(CViewC64VicDisplay *vicDisplay);
	
	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	
	// @returns painting status (ok, replaced color, blocked)
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);
	

	virtual u8 GetColorAtPixel(int x, int y);
	virtual void PutColorAtPixel(int x, int y, u8 paintColor);
	
	virtual void ClearScreen();
	virtual void RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen, u8 backgroundColorAlpha, u8 foregroundColorAlpha);

	virtual void RenderCanvasSpecificGridLines();
	virtual void RenderCanvasSpecificGridValues();
};

#endif
