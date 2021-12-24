#include "C64VicDisplayCanvasHiresText.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CViewC64VicControl.h"
#include "C64CharsetHires.h"
#include "C64CharHires.h"
#include "CImageData.h"

C64VicDisplayCanvasHiresText::C64VicDisplayCanvasHiresText(CViewC64VicDisplay *vicDisplay)
: C64VicDisplayCanvas(vicDisplay, C64_CANVAS_TYPE_TEXT, false, false)
{
	
}

void C64VicDisplayCanvasHiresText::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
												 u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	//LOGD("C64VicDisplayCanvasHiresText::RefreshScreen, this=%x", this);
	
	this->viciiState = viciiState;
	
	// refresh texture of C64's character mode screen
	// based on doodle_vicii_text_mode_render
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	if (vicDisplay->viewVicControl && vicDisplay->viewVicControl->forceGrayscaleColors)
	{
		u8 bitmap;
		
		u8 bgColorR = 0, bgColorG = 0, bgColorB = 0;
		u8 fgColorR = 255, fgColorG = 255, fgColorB = 255;
		
		for (int i = 0; i < 25; i++)
		{
			for (int j = 0; j < 40; j++)
			{
				for (int k = 0; k < 8; k++)
				{
					bitmap = chargen_ptr[(screen_ptr[(i * 40) + j] * 8) + k];
					for (int l = 0; l < 8; l++)
					{
						if (bitmap & (1 << (7 - l)))
						{
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
							imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, fgColorR, fgColorG, fgColorB, foregroundColorAlpha);
						}
						else
						{
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
							imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, bgColorR, bgColorG, bgColorB, backgroundColorAlpha);
						}
					}
				}
			}
		}
	}
	else
	{
		u8 bitmap;
		
		u8 bgcolor;
		u8 bgColorR, bgColorG, bgColorB;
		u8 fgcolor;
		u8 fgColorR, fgColorG, fgColorB;
		
		bgcolor = colors[1]; //vicii.regs[0x21] & 0xf;
		
		debugInterface->GetCBMColor(bgcolor, &bgColorR, &bgColorG, &bgColorB);
		
		for (int i = 0; i < 25; i++)
		{
			for (int j = 0; j < 40; j++)
			{
				fgcolor = color_ram_ptr[(i * 40) + j] & 0xf;
				debugInterface->GetCBMColor(fgcolor, &fgColorR, &fgColorG, &fgColorB);
				
				for (int k = 0; k < 8; k++)
				{
					bitmap = chargen_ptr[(screen_ptr[(i * 40) + j] * 8) + k];
					for (int l = 0; l < 8; l++)
					{
						if (bitmap & (1 << (7 - l)))
						{
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = fgcolor;
							imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, fgColorR, fgColorG, fgColorB, foregroundColorAlpha);
						}
						else
						{
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + l] = bgcolor;
							imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, bgColorR, bgColorG, bgColorB, backgroundColorAlpha);
						}
					}
				}
			}
		}
	}
	
	
}

u8 C64VicDisplayCanvasHiresText::PutCharacterAtRaster(int x, int y, u8 color, int charValue)
{
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));

	return this->PutCharacterAt(charColumn, charRow, color, charValue);
}

u8 C64VicDisplayCanvasHiresText::PutCharacterAt(int charColumn, int charRow, u8 color, int charValue)
{
	u16 screenBase = vicDisplay->screenAddress;
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

void C64VicDisplayCanvasHiresText::ClearScreen()
{
	LOGF("C64VicDisplayCanvasHiresText::ClearScreen");
	ClearScreen(0x20, 0x00);
}

void C64VicDisplayCanvasHiresText::ClearScreen(u8 charValue, u8 colorValue)
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

void C64VicDisplayCanvasHiresText::RenderCanvasSpecificGridLines()
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
		else
		{
			float cx = vicDisplay->displayPosWithScrollX + (float)rasterX * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
			
			BlitFilledRectangle(cx - lw2, cys, vicDisplay->posZ, lineWidth, cysz,
							 vicDisplay->gridLinesColorR2, vicDisplay->gridLinesColorG2, vicDisplay->gridLinesColorB2, vicDisplay->gridLinesColorA2);
			
			//			BlitLine(cx, cys, cx, cye, -1,
			//					 1.0f, 1.0f, 1.0f, 0.5f);
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

void C64VicDisplayCanvasHiresText::RenderCanvasSpecificGridValues()
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
u8 C64VicDisplayCanvasHiresText::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
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
u8 C64VicDisplayCanvasHiresText::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
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

///
u8 C64VicDisplayCanvasHiresText::ConvertFrom(CImageData *imageData)
{
	LOGF("C64VicDisplayCanvasHiresText::ConvertFrom");
	
	CDebugInterfaceC64 *debugInterface = vicDisplay->debugInterface;
	
	CImageData *image = ReducePalette(imageData, vicDisplay);
	
	std::vector<C64ColorsHistogramElement *> *colors = GetSortedColorsHistogram(image);

	u8 backgroundColor = (*colors)[0]->color;
	
	LOGF("backgroundColor = %d", backgroundColor);
	
	DeleteColorsHistogram(colors);
	
	debugInterface->SetByteC64(0xD020, backgroundColor);
	debugInterface->SetByteC64(0xD021, backgroundColor);
	
	//
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow), &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	C64CharsetHires *charset = new C64CharsetHires();
	charset->CreateFromCharset(chargen_ptr);
	
	LOGF("...matching chars...");
	
	C64CharHires *bitmapChr = new C64CharHires();
	
	int histogram[16];
	
	for (int yc = 0; yc < 25; yc++)
	{
		for (int xc = 0; xc < 40; xc++)
		{
			LOGF(" xc=%d yc=%d", xc, yc);
			int x = xc * 8;
			int y = yc * 8;
			
			// copy character bitmap from image
			memset(histogram, 0, 16*sizeof(int));

			bitmapChr->Clear();

			for (int yb = 0; yb < 8; yb++)
			{
				for (int xb = 0; xb < 8; xb++)
				{
					u8 v = image->GetPixelResultByte(x + xb, y + yb);
					
					if (v != backgroundColor)
					{
						bitmapChr->SetPixel(xb, yb, 1);
					}
					
					histogram[v]++;
				}
			}
			
			bitmapChr->DebugPrint();
			
			// find max color
			int max = 0;
			u8 color = 0;
			
			for (int i = 0; i < 16; i++)
			{
				LOGF(" histogram[%d] = %d", i, histogram[i]);
				
				if (i == backgroundColor)
					continue;
				
				if (histogram[i] > max)
				{
					max = histogram[i];
					color = i;
				}
			}
			
			// match character
			int fitCharacters[256] = { 0 };
			
			for (int i = 0; i < 256; i++)
			{
				C64CharHires *chr = charset->GetCharacter(i);
				int fit = 0;
				for (int y = 0; y < 8; y++)
				{
					for (int x = 0; x < 8; x++)
					{
						int p1 = chr->GetPixel(x, y);
						int p2 = bitmapChr->GetPixel(x, y);
						
						//LOGF("  p1=%d p2=%d", p1, p2);
						if (p1 == p2)
						{
							fit++;
						}
					}
				}
				
				//LOGF("  --> i=%02x fit=%d", i, fit);
				fitCharacters[i] = fit;
			}
			
			// find best fit (most pixels equal)
			max = 0;
			u8 c = 0;
			for (int i = 0; i < 256; i++)
			{
				if (fitCharacters[i] > max)
				{
					max = fitCharacters[i];
					c = i;
				}
			}
			
			LOGF("  xc=%2d yc=%2d .. c=%02x (fit=%d) '%c'", xc, yc, c, max, c);
			
			//
			PutCharacterAt(xc, yc, color, c);
		}
	}
	
	delete bitmapChr;
	delete charset;
	delete image;
	return PAINT_RESULT_OK;
	
}

