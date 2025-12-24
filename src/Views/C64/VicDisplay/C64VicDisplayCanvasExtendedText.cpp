#include "C64VicDisplayCanvasExtendedText.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "C64CharsetHires.h"
#include "C64CharHires.h"
#include "CImageData.h"
#include "C64Tools.h"
#include <float.h>

C64VicDisplayCanvasExtendedText::C64VicDisplayCanvasExtendedText(CViewC64VicDisplay *vicDisplay)
: C64VicDisplayCanvas(vicDisplay, C64_CANVAS_TYPE_TEXT, false, false)
{
	
}

void C64VicDisplayCanvasExtendedText::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
													u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	this->viciiState = viciiState;
	
	// refresh texture of C64's character mode screen
	// based on doodle_vicii_extended_background_mode_render
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	u8 bitmap;
	
	u8 bgcolor;
	u8 bgColorR, bgColorG, bgColorB;
	u8 fgcolor;
	u8 fgColorR, fgColorG, fgColorB;
	
	for (int i = 0; i < 25; i++)
	{
		for (int j = 0; j < 40; j++)
		{
			fgcolor = color_ram_ptr[(i * 40) + j] & 0xf;
			bgcolor = colors[1 + ((screen_ptr[(i * 40) + j] & 0xc0) >> 6)] & 0xf;
			
			debugInterface->GetCBMColor(bgcolor, &bgColorR, &bgColorG, &bgColorB);
			debugInterface->GetCBMColor(fgcolor, &fgColorR, &fgColorG, &fgColorB);
			
			for (int k = 0; k < 8; k++)
			{
				bitmap = chargen_ptr[((screen_ptr[(i * 40) + j] & 0x3f) * 8) + k];
				for (int l = 0; l < 8; l++)
				{
					if (bitmap & (1 << (7 - l)))
					{
						imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, fgColorR, fgColorG, fgColorB, foregroundColorAlpha);
					}
					else
					{
						imageDataScreen->SetPixelResultRGBA(j*8 + l, i*8 + k, bgColorR, bgColorG, bgColorB, backgroundColorAlpha);
					}
				}
			}
		}
	}
}

//
void C64VicDisplayCanvasExtendedText::ClearScreen()
{
	LOGD("C64VicDisplayCanvasExtendedText::ClearScreen");
	ClearScreen(0x20, 0x00);
}

void C64VicDisplayCanvasExtendedText::ClearScreen(u8 charValue, u8 colorValue)
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

u8 C64VicDisplayCanvasExtendedText::PutCharacterAtRaster(int x, int y, u8 color, int charValue)
{
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	return this->PutCharacterAt(charColumn, charRow, color, charValue);
}

u8 C64VicDisplayCanvasExtendedText::PutCharacterAt(int charColumn, int charRow, u8 color, int charValue)
{
	u16 screenBase = vicDisplay->screenAddress;
	if (charColumn < 0 || charColumn > 39)
		return PAINT_RESULT_OUTSIDE;
	
	if (charRow < 0 || charRow > 24)
		return PAINT_RESULT_OUTSIDE;
	
	int offset = charColumn + charRow * 40;
	
//	LOGF("1     poke %04x %02x", screenBase + offset, charValue);
	debugInterface->SetByteC64(screenBase + offset, charValue);
	
//	LOGF("2     poke %04x %02x", 0xD800 + offset, color);
	debugInterface->SetByteC64(0xD800 + offset, color);
	
	return PAINT_RESULT_OK;
}


void C64VicDisplayCanvasExtendedText::RenderCanvasSpecificGridLines()
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

void C64VicDisplayCanvasExtendedText::RenderCanvasSpecificGridValues()
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
					
					if (cx >= -fs2 && cx < vicDisplay->sizeX && cy >= -fs2 && cy < vicDisplay->sizeY)
					{
						viewC64->fontDisassembly->BlitText(buf, cx + vox + fs*7, cy, vicDisplay->posZ, fs);
					}
					
					//
					
					//LOGD("rasterX=%d rasterY=%d", rasterX, rasterY);
					
					sprintfHexCode16WithoutZeroEnding(buf1, vicDisplay->screenAddress + offset);
					sprintfHexCode8(buf1 + 5, charValue);
					
					
					if (cx >= -fs2 && cx < vicDisplay->sizeX && cy >= -fs2 && cy < vicDisplay->sizeY)
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
u8 C64VicDisplayCanvasExtendedText::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
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
u8 C64VicDisplayCanvasExtendedText::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	u8 color1, color2;
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		color1 = colorLMB;
		color2 = colorRMB;
	}
	else //if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
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

//
///
u8 C64VicDisplayCanvasExtendedText::ConvertFrom(CImageData *imageData)
{
	LOGTODO("C64VicDisplayCanvasExtendedText::ConvertFrom: not implemented properly yet");
	
	return PAINT_RESULT_ERROR;
	
	LOGD("C64VicDisplayCanvasExtendedText::ConvertFrom");
	
	CDebugInterfaceC64 *debugInterface = vicDisplay->debugInterface;
	
	CImageData *image = ReducePalette(imageData, vicDisplay);
	
	std::vector<C64ColorsHistogramElement *> *sortedColors = GetSortedColorsHistogram(image);

	u8 colors[4];
	
	colors[0] = (*sortedColors)[0]->color;
	colors[1] = (*sortedColors)[1]->color;
	colors[2] = (*sortedColors)[2]->color;
	colors[3] = (*sortedColors)[3]->color;
	
	debugInterface->SetByteC64(0xD020, colors[0]);
	
	debugInterface->SetByteC64(0xD021, colors[0]);
	debugInterface->SetByteC64(0xD022, colors[1]);
	debugInterface->SetByteC64(0xD023, colors[2]);
	debugInterface->SetByteC64(0xD024, colors[3]);

	DeleteColorsHistogram(sortedColors);
	
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
					histogram[v]++;
				}
			}
			
			// find backgroundColor
			u8 backgroundColor = 0;
			int max = -1;
			for (int i = 0; i < 16; i++)
			{
				if (histogram[i] > max)
				{
					backgroundColor = i;
					max = i;
				}
			}
			
			// find minimum distance
			float min = FLT_MAX;
			u8 backgroundColorNum = 0;
			for (int i = 0; i < 4; i++)
			{
				float d = GetC64ColorDistance(colors[i], backgroundColor, debugInterface);
				if (d < min)
				{
					min = d;
					backgroundColorNum = i;
				}
			}
			
			backgroundColor = colors[backgroundColorNum];
			
			// find fore color
			u8 foregroundColor = 0;
			max = -1;
			for (int i = 0; i < 16; i++)
			{
				if (i == backgroundColor)
					continue;
				
				if (histogram[i] > max)
				{
					foregroundColor = i;
					max = i;
				}
			}
			
			//
			
			
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
			
			
			// match  only first 64 characters
			int fitCharacters[64] = { 0 };
			
			for (int i = 0; i < 64; i++)
			{
				C64CharHires *chr = charset->GetCharacter(i);
				int fit = 0;
				for (int y = 0; y < 8; y++)
				{
					for (int x = 0; x < 8; x++)
					{
						int p1 = chr->GetPixel(x, y);
						int p2 = bitmapChr->GetPixel(x, y);
						
						//LOGD("  p1=%d p2=%d", p1, p2);
						if (p1 == p2)
						{
							fit++;
						}
					}
				}
				
				//LOGD("  --> i=%02x fit=%d", i, fit);
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
			if (backgroundColorNum == 0)
			{
				// nothing
			}
			else if (backgroundColorNum == 1)
			{
				c |= 0x40;
			}
			else if (backgroundColorNum == 2)
			{
				c |= 0x80;
			}
			else if (backgroundColorNum == 3)
			{
				c += 0xC0;
			}
			else
			{
				SYS_FatalExit("YOUR REASONING IS BAD");
			}
			
			PutCharacterAt(xc, yc, foregroundColor, c); //color, c);
		}
	}
	
	
	
	
	delete charset;
	delete image;
	return PAINT_RESULT_OK;
	
}
