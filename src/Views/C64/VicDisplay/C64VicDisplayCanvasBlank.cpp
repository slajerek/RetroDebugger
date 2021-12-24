#include "C64VicDisplayCanvasBlank.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CImageData.h"

C64VicDisplayCanvasBlank::C64VicDisplayCanvasBlank(CViewC64VicDisplay *vicDisplay)
: C64VicDisplayCanvas(vicDisplay, C64_CANVAS_TYPE_BLANK, false, false)
{
	
}

void C64VicDisplayCanvasBlank::ClearScreen()
{
	LOGWarning("C64VicDisplayCanvasBlank::ClearScreen");
}

void C64VicDisplayCanvasBlank::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
											 u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	this->viciiState = viciiState;

	// refresh texture of C64's character mode screen
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];

	vicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

	u8 bgcolor;
	u8 bgColorR, bgColorG, bgColorB;

	bgcolor = colors[1];

	debugInterface->GetCBMColor(bgcolor, &bgColorR, &bgColorG, &bgColorB);

	for (int y = 0; y < 200; y++)
	{
		for (int x = 0; x < 320; x++)
		{
			imageDataScreen->SetPixelResultRGBA(x, y, bgColorR, bgColorG, bgColorB, backgroundColorAlpha);
		}
	}
}

void C64VicDisplayCanvasBlank::RenderCanvasSpecificGridLines()
{
	// nothing
}

void C64VicDisplayCanvasBlank::RenderCanvasSpecificGridValues()
{
	// nothing
}

u8 C64VicDisplayCanvasBlank::GetColorAtPixel(int x, int y)
{
	// nothing
	return 0;
}

void C64VicDisplayCanvasBlank::PutColorAtPixel(int x, int y, u8 paintColor)
{
	// nothing
}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasBlank::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return PAINT_RESULT_ERROR;
}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasBlank::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return PAINT_RESULT_ERROR;
}

