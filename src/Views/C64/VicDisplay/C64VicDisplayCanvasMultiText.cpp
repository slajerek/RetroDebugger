#include "C64VicDisplayCanvasMultiText.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CImageData.h"

C64VicDisplayCanvasMultiText::C64VicDisplayCanvasMultiText(CViewC64VicDisplay *vicDisplay)
: C64VicDisplayCanvas(vicDisplay, C64_CANVAS_TYPE_TEXT, false, false)
{
	
}

void C64VicDisplayCanvasMultiText::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
												 u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
//	LOGD("C64VicDisplayCanvasMultiText::RefreshScreen, this=%x", this);
	this->viciiState = viciiState;
	
	// refresh texture of C64's character mode screen
	// based on doodle_vicii_multicolor_text_mode_render
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	u8 bitmap;
	
	u8 color0R, color0G, color0B;
	u8 color1R, color1G, color1B;
	u8 color2R, color2G, color2B;
	
	debugInterface->GetCBMColor(colors[1], &color0R, &color0G, &color0B);
	debugInterface->GetCBMColor(colors[2], &color1R, &color1G, &color1B);
	debugInterface->GetCBMColor(colors[3], &color2R, &color2G, &color2B);
	
	u8 color3;
	u8 color3R, color3G, color3B;
	
	for (int i = 0; i < 25; i++)
	{
		for (int j = 0; j < 40; j++)
		{
			color3 = color_ram_ptr[(i * 40) + j] & 0x0F;
			for (int k = 0; k < 8; k++)
			{
				bitmap = chargen_ptr[(screen_ptr[(i * 40) + j] * 8) + k];
				if (color3 & 8)
				{
					debugInterface->GetCBMColor((color3 & 7), &color3R, &color3G, &color3B);
					
					for (int l = 0; l < 4; l++)
					{
						switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2))
						{
							case 0:
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color0R, color0G, color0B, backgroundColorAlpha);
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color0R, color0G, color0B, backgroundColorAlpha);
								break;
							case 1:
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color1R, color1G, color1B, foregroundColorAlpha);
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color1R, color1G, color1B, foregroundColorAlpha);
								break;
							case 2:
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color2R, color2G, color2B, foregroundColorAlpha);
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color2R, color2G, color2B, foregroundColorAlpha);
								break;
							case 3:
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color3R, color3G, color3B, foregroundColorAlpha);
								imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color3R, color3G, color3B, foregroundColorAlpha);
								break;
						}
					}
				}
				else
				{
					debugInterface->GetCBMColor(color3, &color3R, &color3G, &color3B);
					
					for (int l = 0; l < 8; l++)
					{
						if (bitmap & (1 << (7 - l)))
						{
							//							data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = color3;
							imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, color3R, color3G, color3B, 255);
						}
						else
						{
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = color0;
							imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, color0R, color0G, color0B, 255);
						}
					}
				}
			}
		}
	}
}

//
void C64VicDisplayCanvasMultiText::ClearScreen()
{
	LOGF("C64VicDisplayCanvasMultiText::ClearScreen");
	ClearScreen(0x20, 0x00);
}

void C64VicDisplayCanvasMultiText::ClearScreen(u8 charValue, u8 colorValue)
{
	u16 screenPtr = vicDisplay->screenAddress;
	u16 colorPtr = 0xD800;
	
	for (int i = 0; i < 0x03E8; i++)
	{
		debugInterface->SetByteC64(screenPtr, charValue);
		debugInterface->SetByteC64(colorPtr, colorValue);
		
		screenPtr++;
		colorPtr++;
	}
}


//
u8 C64VicDisplayCanvasMultiText::PutCharacterAtRaster(int x, int y, u8 color, int charValue)
{
	u16 screenBase = vicDisplay->screenAddress;
	
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	if (charColumn < 0 || charColumn > 39)
		return PAINT_RESULT_OUTSIDE;
	
	if (charRow < 0 || charRow > 24)
		return PAINT_RESULT_OUTSIDE;
	
	int offset = charColumn + charRow * 40;
	
	LOGF("1     poke %04x %02x", screenBase + offset, charValue);
	debugInterface->SetByteC64(screenBase + offset, charValue);
	
	LOGF("2     poke %04x %02x", 0xD800 + offset, color);
	debugInterface->SetByteC64(0xD800 + offset, color);
	
	return PAINT_RESULT_OK;
}

//
void C64VicDisplayCanvasMultiText::RenderCanvasSpecificGridLines()
{
	// raster screen in hex:
	// startx = 68 (88) endx = 1e8 (1c8)
	// starty = 10 (32) endy = 120 ( fa)
	
	float lineWidth = 1.0f;
	float lw2 = lineWidth/2.0f;
	
	
	float cys = vicDisplay->displayPosWithScrollY + 0.0f * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
	float cye = vicDisplay->displayPosWithScrollY + 200.0f * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
	float cysz = 200.0f * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
	
	float cxs = vicDisplay->displayPosWithScrollX + 0.0f * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
	float cxe = vicDisplay->displayPosWithScrollX + 320.0f * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
	float cxsz = 320.0f * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
	
	// TODO: optimize this
	
	// vertical lines
	for (int rasterX = 0.0f; rasterX <= 320.0f; rasterX++)
	{
		if (rasterX % 8 == 0)
		{
			float cx = vicDisplay->displayPosWithScrollX + (float)rasterX * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
			
			BlitFilledRectangle(cx - lw2, cys, vicDisplay->posZ, lineWidth, cysz,
							 vicDisplay->gridLinesColorR, vicDisplay->gridLinesColorG, vicDisplay->gridLinesColorB, vicDisplay->gridLinesColorA);
			
			//			BlitLine(cx, cys, cx, cye, -1,
			//					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
		}
		else if (rasterX % 2 == 0)
		{
			float cx = vicDisplay->displayPosWithScrollX + (float)rasterX * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
			
			BlitFilledRectangle(cx - lw2, cys, vicDisplay->posZ, lineWidth, cysz,
							 vicDisplay->gridLinesColorR2, vicDisplay->gridLinesColorG2, vicDisplay->gridLinesColorB2, vicDisplay->gridLinesColorA2);
			
			//			BlitLine(cx, cys, cx, cye, -1,
			//					 1.0f, 1.0f, 1.0f, 0.5f);
		}
	}
	
	// add vertical lines where char is in hires
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

	for (int y = 0; y < 25; y++)
	{
		for (int x = 0; x < 40; x++)
		{
			u8 color3 = color_ram_ptr[(y * 40) + x] & 0x0F;
			if (!(color3 & 8))
			{
				//LOGF("RenderCanvasSpecificGridLines: hires x=%d y=%d", x, y);

				float rasterX = (float)x * 8.0f;
				float rasterY = (float)y * 8.0f;

				//LOGF("   rx=%f ry=%f", rasterX, rasterY);
				
				float cx = vicDisplay->displayPosWithScrollX + (float)rasterX * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
				float cy = vicDisplay->displayPosWithScrollY + (float)rasterY * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
				float cysch = 8.0f * vicDisplay->rasterScaleFactorY;

				cx += vicDisplay->rasterScaleFactorX;
				
				// render just adjacent lines
				for (int l = 0; l < 4; l++)
				{
					BlitFilledRectangle(cx - lw2, cy, vicDisplay->posZ, lineWidth, cysch,
										vicDisplay->gridLinesColorR2, vicDisplay->gridLinesColorG2, vicDisplay->gridLinesColorB2, vicDisplay->gridLinesColorA2);
					
					cx += vicDisplay->rasterScaleFactorX * 2.0f;
				}

			}
		}
	}

	
	// horizontal lines
	for (int rasterY = 0.0f; rasterY <= 200.0f; rasterY++)
	{
		if (rasterY % 8 == 0)
		{
			float cy = vicDisplay->displayPosWithScrollY + (float)rasterY * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
			
			BlitFilledRectangle(cxs, cy - lw2, vicDisplay->posZ, cxsz, lineWidth,
							 vicDisplay->gridLinesColorR, vicDisplay->gridLinesColorG, vicDisplay->gridLinesColorB, vicDisplay->gridLinesColorA);
			
			//			BlitLine(cxs, cy, cxe, cy, -1,
			//					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
		}
		else
		{
			float cy = vicDisplay->displayPosWithScrollY + (float)rasterY * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
			
			BlitFilledRectangle(cxs, cy - lw2, vicDisplay->posZ, cxsz, lineWidth,
							 vicDisplay->gridLinesColorR2, vicDisplay->gridLinesColorG2, vicDisplay->gridLinesColorB2, vicDisplay->gridLinesColorA2);
			
			//			BlitLine(cxs, cy, cxe, cy, -1,
			//					 1.0f, 1.0f, 1.0f, 0.5f);
			
		}
	}
}

void C64VicDisplayCanvasMultiText::RenderCanvasSpecificGridValues()
{
	/// values
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow), &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	u8 colors[5];
	colors[0] = d020colors[0];
	colors[1] = d020colors[1];
	
	
	//float frx = (float)rasterX * rasterScaleFactorX;
	char buf1[8] = {0};
	char buf2[8] = {0};
	
	buf1[4] = ' ';
	
	
	// horizontal lines
	float fs = 8.0f;
	float fs2 = fs*4.0f;
	
	float vox = vicDisplay->rasterScaleFactorX - fs/1.0f;
	float voy = vicDisplay->rasterScaleFactorY/2.0f - fs/4.0f;
	
	
	for (int rasterY = 0.0f; rasterY < 200.0f; rasterY++)
	{
		{
			float cy = vicDisplay->displayPosWithScrollY + (float)rasterY * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;
			
			float cx = vicDisplay->displayPosWithScrollX + vicDisplay->rasterCrossOffsetX;
			
			for (int rasterX = 0.0f; rasterX < 320.0f; rasterX += 8.0f)
			{
				if (rasterY % 8 == 0)
				{
					//colors[2] = this->debugInterface->GetByteC64(screenBase + offset) >> 4;
					
					u16 screenBase = vicDisplay->screenAddress;
					
					int charColumn = floor((float)((float)rasterX / 8.0f));
					int charRow = floor((float)((float)rasterY / 8.0f));
					
					int offset = charColumn + charRow * 40;
					
					u8 charValue = (screen_ptr[(charRow * 40) + charColumn]);
					u8 colorValue = color_ram_ptr[(charRow * 40) + charColumn] & 0xf;
					
					char buf[256];
					sprintf(buf, "%02x %02x  %02x",
							colors[0], colors[1], colorValue);
					
					if (cx >= -fs2 && cx < SCREEN_WIDTH && cy >= -fs2 && cy < SCREEN_HEIGHT)
					{
						viewC64->fontDisassembly->BlitText(buf, cx + vox + fs*7, cy, vicDisplay->posZ, fs);
					}
					
					//
					
					//LOGF("rasterX=%d rasterY=%d", rasterX, rasterY);
					
					sprintfHexCode16WithoutZeroEnding(buf1, vicDisplay->screenAddress + offset);
					sprintfHexCode8(buf1 + 5, charValue);
					
					
					if (cx >= -fs2 && cx < SCREEN_WIDTH && cy >= -fs2 && cy < SCREEN_HEIGHT)
					{
						viewC64->fontDisassembly->BlitText(buf1, cx, cy, vicDisplay->posZ, fs);
					}
					
					cx += vicDisplay->rasterScaleFactorX * 8.0f;
					
				}
			}
			
		}
	}

}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasMultiText::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		return this->PutCharacterAtRaster(x, y, colorLMB, charValue);
	}
	else
	{
		return this->PutCharacterAtRaster(x, y, colorRMB, charValue);
	}
}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasMultiText::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
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
		LOGF("													---- isAltPressed: dither -----");
		{
			if (ditherMaskPosX == -1 || ditherMaskPosY == -1)
			{
				LOGF("******** START DITHER ********");
				// start dither painting
				if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
				{
					ditherMaskPosX = x;
				}
				else if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
				{
					ditherMaskPosX = x + 2;
				}
				
				ditherMaskPosY = y;
			}
			
			int dX = abs( x/8 - ditherMaskPosX/8 ) % 2;
			int dY = abs( y/8 - ditherMaskPosY/8 ) % 2;
			
			int d = (dX + dY) % 2;
			
			LOGF("==================== dX=%d dY=%d d=%d", dX, dY, d);
			
			if (d != 0)
			{
				paintColor = color2;
			}
		}
	}
	
	this->PutCharacterAtRaster(x, y, paintColor, charValue);
	
	return PAINT_RESULT_OK;
}
