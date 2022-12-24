#include "C64VicDisplayCanvasHiresBitmap.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "C64CharHires.h"
#include "CImageData.h"
#include "C64Tools.h"
#include "CViewC64VicControl.h"
#include "CViewC64Palette.h"
#include "CViewC64VicEditor.h"

C64VicDisplayCanvasHiresBitmap::C64VicDisplayCanvasHiresBitmap(CViewC64VicDisplay *vicDisplay)
: C64VicDisplayCanvas(vicDisplay, C64_CANVAS_TYPE_BITMAP, false, false)
{
	
}

void C64VicDisplayCanvasHiresBitmap::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
												   u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	this->viciiState = viciiState;
	
	// refresh texture of C64's character mode screen
	// based on doodle_vicii_hires_bitmap_mode_render
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
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
					if (((i * 40 * 8) + (j * 8) + k) < 4096)
					{
						
						bitmap = bitmap_low_ptr[(i * 40 * 8) + (j * 8) + k];
					}
					else
					{
						bitmap = bitmap_high_ptr[((i * 40 * 8) + (j * 8) + k) - 4096];
					}
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
		
		for (int i = 0; i < 25; i++)
		{
			for (int j = 0; j < 40; j++)
			{
				bgcolor = screen_ptr[(i * 40) + j] & 0x0F;
				fgcolor = (screen_ptr[(i * 40) + j] & 0xF0) >> 4;
				
				debugInterface->GetCBMColor(bgcolor, &bgColorR, &bgColorG, &bgColorB);
				debugInterface->GetCBMColor(fgcolor, &fgColorR, &fgColorG, &fgColorB);
				
				for (int k = 0; k < 8; k++)
				{
					if (((i * 40 * 8) + (j * 8) + k) < 4096)
					{
						
						bitmap = bitmap_low_ptr[(i * 40 * 8) + (j * 8) + k];
					}
					else
					{
						bitmap = bitmap_high_ptr[((i * 40 * 8) + (j * 8) + k) - 4096];
					}
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


//// render grid

void C64VicDisplayCanvasHiresBitmap::RenderCanvasSpecificGridLines()
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

void C64VicDisplayCanvasHiresBitmap::RenderCanvasSpecificGridValues()
{
	/// values
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	u8 bgcolor;
	u8 fgcolor;
	
	
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
			
			//			if (cy < 0.0f || cy > SCREEN_HEIGHT)
			//				continue;
			
			//			BlitFilledRectangle(cxs, cy - lw2, posZ, cxsz, lineWidth,
			//							 gridLinesColorR2, gridLinesColorG2, gridLinesColorB2, gridLinesColorA2);
			
			//			BlitLine(cxs, cy, cxe, cy, -1,
			//					 1.0f, 1.0f, 1.0f, 0.5f);
			
			float cx = vicDisplay->displayPosWithScrollX + vicDisplay->rasterCrossOffsetX;
			
//			for (int rasterX = 0.0f; rasterX < 320.0f; rasterX += 8.0f)
			for (int rasterX = 0; rasterX < 320; rasterX += 8)
			{
				if (rasterY % 8 == 0)
				{
					//colors[2] = this->debugInterface->GetByteC64(screenBase + offset) >> 4;
					
					u16 screenBase = vicDisplay->screenAddress;
					
					int charColumn = floor((float)((float)rasterX / 8.0f));
					int charRow = floor((float)((float)rasterY / 8.0f));
					
					int offset = charColumn + charRow * 40;
					
					fgcolor = (screen_ptr[(charRow * 40) + charColumn] & 0xf0) >> 4;
					bgcolor = screen_ptr[(charRow * 40) + charColumn] & 0xf;
					
					char buf[256];
					sprintf(buf, "%02x %02x  %02x %02x",
							d020colors[0], d020colors[1], bgcolor, fgcolor);
					
					if (cx >= -fs2 && cx < vicDisplay->sizeX && cy >= -fs2 && cy < vicDisplay->sizeY)
					{
						viewC64->fontDisassembly->BlitText(buf, cx + vox + fs*7, cy, vicDisplay->posZ, fs);
					}
				}
				
				
				//LOGD("rasterX=%d rasterY=%d", rasterX, rasterY);
				int addr = vicDisplay->GetAddressForRaster(rasterX, rasterY);
				
				int vicAddr = addr - vicDisplay->bitmapAddress;
				
				//u8 val = this->debugInterface->GetByteC64(addr);
				
				u8 val;
				
				if (vicAddr < 4096)
				{
					
					val = bitmap_low_ptr[vicAddr];
				}
				else
				{
					val = bitmap_high_ptr[vicAddr - 4096];
				}
				
				
				sprintfHexCode16WithoutZeroEnding(buf1, addr);
				sprintfHexCode8(buf1 + 5, val);
				
				
				if (cx >= -fs2 && cx < vicDisplay->sizeX && cy >= -fs2 && cy < vicDisplay->sizeY)
				{
					viewC64->fontDisassembly->BlitText(buf1, cx, cy, vicDisplay->posZ, fs);
				}
				
				for (int pixelNum = 0; pixelNum < 8; pixelNum++)
				{
					bool vb = (val & (1 << (7 - pixelNum)));
					
					buf2[0] = vb ? '1' : '0';
					
					//LOGD("buf2=%s", buf2);
					
					if (cx >= -fs2 && cx < vicDisplay->sizeX && cy >= -fs2 && cy < vicDisplay->sizeY)
					{
						viewC64->fontDisassembly->BlitText(buf2, cx + vox, cy + voy, vicDisplay->posZ, fs);
					}
					
					cx += vicDisplay->rasterScaleFactorX;
				}
			}
			
		}
	}
}

//
void C64VicDisplayCanvasHiresBitmap::ClearScreen()
{
	LOGD("C64VicDisplayCanvasHiresBitmap::ClearScreen");
	// TODO: check this
	ClearScreen(viewC64->viewVicEditor->viewPalette->colorD021, 0x00);
}

void C64VicDisplayCanvasHiresBitmap::ClearScreen(u8 charValue, u8 colorValue)
{
	// 0x0000 - 0x1F40
	
	u8 bitmapValue = 0x00;
	
	u16 bitmapPtr = vicDisplay->bitmapAddress;
	u16 screenPtr = vicDisplay->screenAddress;
	u16 colorPtr = 0xD800;
	
	for (int i = 0x0000; i < 0x1F40; i++)
	{
		this->debugInterface->SetByteC64(bitmapPtr, bitmapValue);
		bitmapPtr++;
	}
	
	for (int i = 0; i < 0x03E8; i++)
	{
		debugInterface->SetByteC64(screenPtr, charValue);
		debugInterface->SetByteC64(colorPtr, colorValue);
		
		screenPtr++;
		colorPtr++;
	}
}

////// pixel manipulation

C64CharHires *C64VicDisplayCanvasHiresBitmap::GetBitCharHiresBitmap(int x, int y)
{
	LOGF("C64CharHires ============================ GetBitCharHiresBitmap %d %d", x, y);
	
	return new C64CharHires(vicDisplay, x, y);
}

u8 C64VicDisplayCanvasHiresBitmap::GetBitPixelHiresBitmap(int x, int y)
{
	LOGF("C64CharHires ============================ GetBitPixelHiresBitmap %d %d", x, y);
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	
	// multi
	u16 bitmapBase = vicDisplay->bitmapAddress;
	u16 screenBase = vicDisplay->screenAddress;
	
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	int pixelNum = x % 8;
	int pixelCharY = y % 8;
	
	int offset = charColumn*8 + charRow * 40*8 + pixelCharY;
	
	int vicAddr = offset; //addr - this->bitmapAddress;
	
	//u8 val = this->debugInterface->GetByteC64(addr);
	
	u8 val;
	
	if (vicAddr < 4096)
	{
		
		val = bitmap_low_ptr[vicAddr];
	}
	else
	{
		val = bitmap_high_ptr[vicAddr - 4096];
	}
	
	//u8 val = this->debugInterface->GetByteC64(bitmapBase + offset);
	
	//LOGD("addr=%04x pixelNum=%d val=%02x", bitmapBase + offset, pixelNum, val);
	bool vb = (val & (1 << (7 - pixelNum)));
	
	return vb ? 1 : 0;
	
}

void C64VicDisplayCanvasHiresBitmap::ReplaceColorHiresBitmapRaster(int x, int y, u8 colorNum, u8 paintColor)
{
	LOGF("............. ReplaceColorHiresBitmapRaster %d %d colorNum=%d paintColor=%02x", x, y, colorNum, paintColor);
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	ReplaceColorHiresBitmapCharacter(charColumn, charRow, colorNum, paintColor);
}

void C64VicDisplayCanvasHiresBitmap::ReplaceColorHiresBitmapCharacter(int charColumn, int charRow, u8 colorNum, u8 paintColor)
{
	LOGF("............. ReplaceColorHiresBitmapCharacter %d %d colorNum=%d paintColor=%02x", charColumn, charRow, colorNum, paintColor);
	if (charColumn < 0 || charColumn > 39)
		return; // PAINT_RESULT_OUTSIDE;
	
	if (charRow < 0 || charRow > 24)
		return; // PAINT_RESULT_OUTSIDE;

	int offset = charColumn + charRow * 40;

	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	u16 bitmapBase = vicDisplay->bitmapAddress;
	u16 screenBase = vicDisplay->screenAddress;
	

	switch(colorNum)
	{
		default:
		case 0x01:
		{
			u8 val = screen_ptr[(charRow * 40) + charColumn];	//this->debugInterface->GetByteC64(screenBase + offset)
			
			LOGF("addr=%04x v=%02x", screenBase + offset, val);
			
			u8 color2 = val & 0x0F; //screen_ptr[(charRow * 40) + charColumn] & 0xf;
			//			u8 color2 = this->debugInterface->GetByteC64(screenBase + offset) & 0x0f;
			
			LOGF("color2=%02x", color2);
			
			u8 newColor1 = ((paintColor & 0x0F) << 4) | (color2 & 0x0F);
			
			LOGF("newColor1=%02x", newColor1);
			
			LOGF("1     poke %04x %02x", screenBase + offset, newColor1);
			debugInterface->SetByteC64(screenBase + offset, newColor1);
			
			screen_ptr[(charRow * 40) + charColumn] = newColor1;
			break;
		}
		case 0x00:
		{
			u8 val = screen_ptr[(charRow * 40) + charColumn];	//this->debugInterface->GetByteC64(screenBase + offset)
			
			LOGF("addr=%04x v=%02x", screenBase + offset, val);
			
			
			u8 color1 = (screen_ptr[(charRow * 40) + charColumn] & 0xF0) >> 4;
			//			u8 color1 = this->debugInterface->GetByteC64(screenBase + offset) >> 4;
			
			LOGF("color2=%02x", color1);
			
			u8 newColor2 = (paintColor & 0x0F) | ((color1 & 0x0F) << 4);
			
			LOGF("newColor2=%02x", newColor2);
			
			debugInterface->SetByteC64(screenBase + offset, newColor2);
			LOGF("2     poke %04x %02x", screenBase + offset, newColor2);
			
			screen_ptr[(charRow * 40) + charColumn] = newColor2;
			
			break;
		}
	}
}

void C64VicDisplayCanvasHiresBitmap::PutBitPixelHiresBitmapRaster(int x, int y, u8 bitColor)
{
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	
	LOGF("============================ PutBitPixelHiresBitmapRaster %d %d | %d", x, y, bitColor);
	
	u16 bitmapBase = vicDisplay->bitmapAddress;
	u16 screenBase = vicDisplay->screenAddress;
	
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	int pixelNum = x % 8;
	int pixelCharY = y % 8;
	
	int offset = charColumn*8 + charRow * 40*8 + pixelCharY;
	
	LOGF("charColumn=%d pixelNum=%d  |  charRow=%d pixelCharY=%d  | offset=%d",
		 charColumn, pixelNum,
		 charRow, pixelCharY,
		 offset);
	
	u8 color = bitColor;	//0=00, 1=01, 2=10, 3=11
	
	u8 a = ~(0x01 << (7-pixelNum));
	u8 o = color << (7-pixelNum);
	
	
	
	//int offset = charColumn*8 + charRow * 40*8 + pixelCharY;
	
	int vicAddr = offset; //addr - this->bitmapAddress;
	
	//u8 val = this->debugInterface->GetByteC64(addr);
	
	u8 val;
	
	if (vicAddr < 4096)
	{
		
		val = bitmap_low_ptr[vicAddr];
	}
	else
	{
		val = bitmap_high_ptr[vicAddr - 4096];
	}
	
	// u8 val = this->debugInterface->GetByteC64(bitmapBase + offset);
	
	
	
	val &= a;
	val |= o;
	
	this->debugInterface->SetByteC64(bitmapBase + offset, val);
	
	//	bitmap_low_ptr[charColumn] = 0xFF;
}



u8 C64VicDisplayCanvasHiresBitmap::GetColorAtPixel(int x, int y)
{
	// get pixel
	u8 pixelBitColor = this->GetBitPixelHiresBitmap(x, y);
	C64CharHires *bitChar = this->GetBitCharHiresBitmap(x, y);
	
	u8 color = bitChar->colors[pixelBitColor];
	
	delete bitChar;
	
	return color;
}

//

u8 C64VicDisplayCanvasHiresBitmap::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 paintColor)
{
	return PutPixelHiresBitmap(forceColorReplace, x, y, paintColor);
}

u8 C64VicDisplayCanvasHiresBitmap::PutPixelHiresBitmap(bool forceColorReplace, int x, int y, u8 paintColor)
{
	LOGF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PutPixelHiresBitmap %d %d %02x", x, y, paintColor);
	
	u8 result = PAINT_RESULT_OK;
	
	u8 prevColor = GetBitPixelHiresBitmap(x, y);
	LOGF("prevColor=%02x", prevColor);
	
	C64CharHires *bitChar = GetBitCharHiresBitmap(x, y);
	
	u8 prevPaintColor = bitChar->colors[prevColor];
	
	LOGF("prevPaintColor=%02x", prevPaintColor);
	
	bitChar->DebugPrint();
	
	
	int newColor = -1;
	int minColor = -1;
	
	// find the same color
	for (int i = 0; i < 2; i++)
	{
		if (bitChar->colors[i] == paintColor)
		{
			LOGF("    found the same color paintColor=%02x at colors[%d]", paintColor, i);
			newColor = i;
			break;
		}
	}
	
	int minColorCount = 999;
	if (newColor == -1)
	{
		newColor = prevColor;

		// find surrounding color from histogram (s)
		for (int i = 0; i < 2; i++)
		{
			if (bitChar->histogram[i] < minColorCount)
			{
				minColorCount = bitChar->histogram[i];
				// do not replace color in hires newColor = i;
				minColor = i;
			}
		}
		
		LOGF("   minColorCount=%d newColor=%02x minColor=%02x", minColorCount, newColor, minColor);
		
		if (minColorCount != 0)
		{
			if (forceColorReplace == false)
			{
				return PAINT_RESULT_BLOCKED;
			}
			
			// force replace color
			result = PAINT_RESULT_REPLACED_COLOR;
			
			newColor = prevColor;
			ReplaceColorHiresBitmapRaster(x, y, newColor, paintColor);
		}
		else
		{
			newColor = minColor;
			//ReplaceColorHiresBitmap(x, y, newColor, paintColor);
		}
	}
	
	LOGF("prevColor=%02x prevPaintColor=%02x | newColor=%02x paintColor=%02x",
		 prevColor, prevPaintColor, newColor, paintColor);
	
	if (prevColor != newColor)
	{
		LOGF("  != newColor=%02x", newColor);
		PutBitPixelHiresBitmapRaster(x, y, newColor);
		
		ReplaceColorHiresBitmapRaster(x, y, newColor, paintColor);
	}
	
	delete bitChar;
	
	return result;
}

u8 C64VicDisplayCanvasHiresBitmap::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	if (x < 0 || x > 319
		|| y < 0 || y > 199)
		return PAINT_RESULT_OUTSIDE;
	
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
				LOGF("******** START DITHER ********");
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
			
			LOGF("==================== dX=%d dY=%d d=%d", dX, dY, d);
			
			if (d != 0)
			{
				paintColor = color2;
				
				// check if we need to replace color
				C64CharHires *bitChar = GetBitCharHiresBitmap(x, y);

				if (bitChar->colors[0] == color1)
				{
					if (bitChar->colors[1] != color2)
					{
						if (bitChar->histogram[1] != 0
							&& forceColorReplace == false)
						{
							return PAINT_RESULT_BLOCKED;
						}

						ReplaceColorHiresBitmapRaster(x, y, 1, color2);
						
						this->PutColorAtPixel(forceColorReplace, x, y, paintColor);
						
						return PAINT_RESULT_REPLACED_COLOR;
					}
				}
				else if (bitChar->colors[1] == color1)
				{
					if (bitChar->colors[0] != color2)
					{
						if (bitChar->histogram[0] != 0
							&& forceColorReplace == false)
						{
							return PAINT_RESULT_BLOCKED;
						}
						
						ReplaceColorHiresBitmapRaster(x, y, 0, color2);
						
						this->PutColorAtPixel(forceColorReplace, x, y, paintColor);
						
						return PAINT_RESULT_REPLACED_COLOR;
					}
				}
			}
		}
	}
	
	return this->PutColorAtPixel(forceColorReplace, x, y, paintColor);

}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasHiresBitmap::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	if (x < 0 || x > 319
		|| y < 0 || y > 199)
		return PAINT_RESULT_OUTSIDE;

	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		return this->PutColorAtPixel(forceColorReplace, x, y, colorLMB);
	}
	else
	{
		return this->PutColorAtPixel(forceColorReplace, x, y, colorRMB);
	}
}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasHiresBitmap::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return this->PaintDither(forceColorReplace, x, y, colorLMB, colorRMB, colorSource);
}

//
u8 C64VicDisplayCanvasHiresBitmap::ConvertFrom(CImageData *imageData)
{
	CDebugInterfaceC64 *debugInterface = vicDisplay->debugInterface;
	
	CImageData *image = ReducePalette(imageData, vicDisplay);
	
	std::vector<C64ColorsHistogramElement *> *colors = GetSortedColorsHistogram(image);
	
	u8 backgroundColor = (*colors)[0]->color;
	
	LOGF("backgroundColor = %d", backgroundColor);
	
	DeleteColorsHistogram(colors);
	
	debugInterface->SetByteC64(0xD020, backgroundColor);
	debugInterface->SetByteC64(0xD021, backgroundColor);

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
			
			for (int yb = 0; yb < 8; yb++)
			{
				for (int xb = 0; xb < 8; xb++)
				{
					u8 v = image->GetPixelResultByte(x + xb, y + yb);
					histogram[v]++;
				}
			}
			
			// find max color (background)
			int max = 0;
			u8 colorBackground = 0;
			
			for (int i = 0; i < 16; i++)
			{
				LOGF(" histogram[%d] = %d", i, histogram[i]);
				
				if (histogram[i] > max)
				{
					max = histogram[i];
					colorBackground = i;
				}
			}
			
			// find second max color (foreground)
			max = 0;
			u8 colorForeground = 0;
			
			for (int i = 0; i < 16; i++)
			{
				LOGF(" histogram[%d] = %d", i, histogram[i]);
				
				if (i == colorBackground)
					continue;
				
				if (histogram[i] > max)
				{
					max = histogram[i];
					colorForeground = i;
				}
			}
			
			ReplaceColorHiresBitmapCharacter(xc, yc, 0, colorBackground);
			ReplaceColorHiresBitmapCharacter(xc, yc, 1, colorForeground);

			for (int yb = 0; yb < 8; yb++)
			{
				for (int xb = 0; xb < 8; xb++)
				{
					u8 v = image->GetPixelResultByte(x + xb, y + yb);
					
					float db = GetC64ColorDistance(v, colorBackground, debugInterface);
					float df = GetC64ColorDistance(v, colorForeground, debugInterface);

					if (db < df)
					{
						PutBitPixelHiresBitmapRaster(x + xb, y + yb, 0);
					}
					else
					{
						PutBitPixelHiresBitmapRaster(x + xb, y + yb, 1);
					}
				}
			}
		}
	}
	
	delete image;
	return PAINT_RESULT_OK;
}

