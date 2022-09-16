#include "C64VicDisplayCanvasMultiBitmap.h"
#include "CViewC64VicDisplay.h"
#include "C64CharMulti.h"
#include "CDebugInterfaceC64.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CImageData.h"
#include "C64Tools.h"
#include <float.h>

C64VicDisplayCanvasMultiBitmap::C64VicDisplayCanvasMultiBitmap(CViewC64VicDisplay *vicDisplay)
: C64VicDisplayCanvas(vicDisplay, C64_CANVAS_TYPE_BITMAP, true, false)
{
	
}

///////

void C64VicDisplayCanvasMultiBitmap::RefreshScreen(vicii_cycle_state_t *viciiState, CImageData *imageDataScreen,
												   u8 backgroundColorAlpha, u8 foregroundColorAlpha)
{
	this->viciiState = viciiState;
	
	// refresh texture of C64's character mode screen
	// based on doodle_vicii_multicolor_bitmap_mode_render
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	u8 bitmap;
	
	u8 color0R, color0G, color0B;
	this->debugInterface->GetCBMColor(colors[1], &color0R, &color0G, &color0B);
	
	u8 color1;
	u8 color1R, color1G, color1B;
	u8 color2;
	u8 color2R, color2G, color2B;
	u8 color3;
	u8 color3R, color3G, color3B;
	
	for (int i = 0; i < 25; i++)
	{
		for (int j = 0; j < 40; j++)
		{
			color1 = (screen_ptr[(i * 40) + j] & 0xf0) >> 4;
			color2 = screen_ptr[(i * 40) + j] & 0xf;
			color3 = color_ram_ptr[(i * 40) + j] & 0xf;
			
			debugInterface->GetCBMColor(color1, &color1R, &color1G, &color1B);
			debugInterface->GetCBMColor(color2, &color2R, &color2G, &color2B);
			debugInterface->GetCBMColor(color3, &color3R, &color3G, &color3B);
			
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
				
				for (int l = 0; l < 4; l++)
				{
					switch ((bitmap & (3 << ((3 - l) * 2))) >> ((3 - l) * 2))
					{
						case 0:
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color0;
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color0;
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color0R, color0G, color0B, backgroundColorAlpha);
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color0R, color0G, color0B, backgroundColorAlpha);
							
							break;
						case 1:
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color1;
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color1;
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color1R, color1G, color1B, foregroundColorAlpha);
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color1R, color1G, color1B, foregroundColorAlpha);
							break;
						case 2:
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color2;
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color2;
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color2R, color2G, color2B, foregroundColorAlpha);
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color2R, color2G, color2B, foregroundColorAlpha);
							break;
						case 3:
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2)] = color3;
							//data->colormap[(i * 320 * 8) + (j * 8) + (k * 320) + (l * 2) + 1] = color3;
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2, i*8 + k, color3R, color3G, color3B, foregroundColorAlpha);
							imageDataScreen->SetPixelResultRGBA(j*8 + l*2 + 1, i*8 + k, color3R, color3G, color3B, foregroundColorAlpha);
							break;
					}
				}
			}
		}
	}
}

//// render grid

void C64VicDisplayCanvasMultiBitmap::RenderCanvasSpecificGridLines()
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

void C64VicDisplayCanvasMultiBitmap::RenderCanvasSpecificGridValues()
{
	/// values
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
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
			
			//			if (cy < 0.0f || cy > SCREEN_HEIGHT)
			//				continue;
			
			//			BlitFilledRectangle(cxs, cy - lw2, posZ, cxsz, lineWidth,
			//							 gridLinesColorR2, gridLinesColorG2, gridLinesColorB2, gridLinesColorA2);
			
			//			BlitLine(cxs, cy, cxe, cy, -1,
			//					 1.0f, 1.0f, 1.0f, 0.5f);
			
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
					
					
					colors[2] = (screen_ptr[(charRow * 40) + charColumn] & 0xf0) >> 4;
					colors[3] = screen_ptr[(charRow * 40) + charColumn] & 0xf;
					colors[4] = color_ram_ptr[(charRow * 40) + charColumn] & 0xf;
					
					char buf[256];
					sprintf(buf, "%02x %02x  %02x %02x %02x",
							colors[0], colors[1], colors[2], colors[3], colors[4]);	//screenBase + offset
					
					if (cx >= -fs2 && cx < SCREEN_WIDTH && cy >= -fs2 && cy < SCREEN_HEIGHT)
					{
						viewC64->fontDisassembly->BlitText(buf, cx + vox + fs*7, cy, vicDisplay->posZ, fs);
					}
				}
				
				
				//LOGF("rasterX=%d rasterY=%d", rasterX, rasterY);
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
				
				
				if (cx >= -fs2 && cx < SCREEN_WIDTH && cy >= -fs2 && cy < SCREEN_HEIGHT)
				{
					viewC64->fontDisassembly->BlitText(buf1, cx, cy, vicDisplay->posZ, fs);
				}
				
				for (int pixelNum = 0; pixelNum < 4; pixelNum++)
				{
					// TODO: this is extremely unoptimal
					
					///LOGF("pixelNum=%d val=%02x", pixelNum, val);
					u8 a = 0x03 << ((3-pixelNum)*2);
					//LOGF(" a=%02x", a);
					u8 va = val & a;
					//LOGF("va=%02x", va);
					u8 vb = va >> ((3-pixelNum)*2);
					//LOGF("vb=%02x", vb);
					
					buf2[0] = ((vb & 0x02) == 0x02) ? '1' : '0';
					buf2[1] = ((vb & 0x01) == 0x01) ? '1' : '0';
					
					//LOGF("buf2=%s", buf2);
					
					if (cx >= -fs2 && cx < SCREEN_WIDTH && cy >= -fs2 && cy < SCREEN_HEIGHT)
					{
						viewC64->fontDisassembly->BlitText(buf2, cx + vox, cy + voy, vicDisplay->posZ, fs);
					}
					
					cx += vicDisplay->rasterScaleFactorX * 2.0f;
				}
			}
			
		}
	}
}

//
void C64VicDisplayCanvasMultiBitmap::ClearScreen()
{
	LOGF("C64VicDisplayCanvasMultiBitmap::ClearScreen");
	ClearScreen(0x00, 0x00);
}

void C64VicDisplayCanvasMultiBitmap::ClearScreen(u8 charValue, u8 colorValue)
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

C64CharMulti *C64VicDisplayCanvasMultiBitmap::GetBitCharMultiBitmap(int x, int y)
{
	LOGF("C64CharMulti ============================ GetBitCharMultiBitmap %d %d", x, y);
	
	return new C64CharMulti(vicDisplay, x, y);
}

u8 C64VicDisplayCanvasMultiBitmap::GetBitPixelMultiBitmap(int x, int y)
{
	LOGF("C64CharMulti ============================ GetBitPixelMultiBitmap %d %d", x, y);
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	
	// multi
	x = x / 2;
	
	u16 bitmapBase = vicDisplay->bitmapAddress;
	u16 screenBase = vicDisplay->screenAddress;
	
	int charColumn = floor((float)((float)x / 4.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	int pixelNum = x % 4;
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
	
	//LOGF("addr=%04x pixelNum=%d val=%02x", bitmapBase + offset, pixelNum, val);
	u8 a = 0x03 << ((3-pixelNum)*2);
	//LOGF(" a=%02x", a);
	u8 va = val & a;
	//LOGF("va=%02x", va);
	u8 vb = va >> ((3-pixelNum)*2);
	LOGF("                                                   =%02x", vb);
	
	return vb;
	
}

void C64VicDisplayCanvasMultiBitmap::ReplaceColorMultiBitmapRaster(int x, int y, u8 colorNum, u8 paintColor)
{
	LOGF("............. ReplaceColorMultiBitmapRaster %d %d colorNum=%d paintColor=%02x", x, y, colorNum, paintColor);
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	ReplaceColorMultiBitmapCharacter(charColumn, charRow, colorNum, paintColor);
}

// colorNum: 0=$D021, 1=char ram x0  2=char ram 0x  3=D800
void C64VicDisplayCanvasMultiBitmap::ReplaceColorMultiBitmapCharacter(int charColumn, int charRow, u8 colorNum, u8 paintColor)
{
	LOGF("............. ReplaceColorMultiBitmapRaster %d %d colorNum=%d paintColor=%02x", charColumn, charRow, colorNum, paintColor);
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	u16 bitmapBase = vicDisplay->bitmapAddress;
	u16 screenBase = vicDisplay->screenAddress;
	
	int offset = charColumn + charRow * 40;
	
	if (colorNum == 0)
	{
		LOGError("ReplaceColor: colorNum=0");
	}
	
	
	switch(colorNum)
	{
		default:
		case 0x00:
		{
			debugInterface->SetByteC64(0xD021, paintColor);
			break;
		}
		case 0x01:
		{
			u8 val = screen_ptr[(charRow * 40) + charColumn];	//this->debugInterface->GetByteC64(screenBase + offset)
			
			LOGF("addr=%02x v=%02x", screenBase + offset, val);
			
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
		case 0x02:
		{
			u8 val = screen_ptr[(charRow * 40) + charColumn];	//this->debugInterface->GetByteC64(screenBase + offset)
			
			LOGF("addr=%02x v=%02x", screenBase + offset, val);
			
			
			u8 color1 = (screen_ptr[(charRow * 40) + charColumn] & 0xf0) >> 4;
			//			u8 color1 = this->debugInterface->GetByteC64(screenBase + offset) >> 4;
			
			LOGF("color2=%02x", color1);
			
			u8 newColor2 = (paintColor & 0x0F) | ((color1 & 0x0F) << 4);
			
			LOGF("newColor2=%02x", newColor2);
			
			debugInterface->SetByteC64(screenBase + offset, newColor2);
			LOGF("2     poke %04x %02x", screenBase + offset, newColor2);
			
			screen_ptr[(charRow * 40) + charColumn] = newColor2;
			
			break;
		}
		case 0x03:
		{
			u8 color3 = color_ram_ptr[(charRow * 40) + charColumn] & 0x0f;
			u8 newColor3 = (paintColor & 0x0F);
			debugInterface->SetByteC64(0xD800 + offset, newColor3);
			LOGF("3     poke %04x %02x   (old: %02x     )", 0xD800 + offset, (paintColor & 0x0F), color3);
			
			color_ram_ptr[(charRow * 40) + charColumn] = newColor3;
			break;
		}
	}
}

void C64VicDisplayCanvasMultiBitmap::PutBitPixelMultiBitmap(int x, int y, u8 bitColor)
{
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicDisplay->GetViciiPointers(this->viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);
	
	
	LOGF("============================ PutBitPixelMultiBitmap %d %d | %d", x, y, bitColor);
	
	// multi
	x = x / 2;
	
	
	u16 bitmapBase = vicDisplay->bitmapAddress;
	u16 screenBase = vicDisplay->screenAddress;
	
	int charColumn = floor((float)((float)x / 4.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	int pixelNum = x % 4;
	int pixelCharY = y % 8;
	
	int offset = charColumn*8 + charRow * 40*8 + pixelCharY;
	
	LOGF("charColumn=%d pixelNum=%d  |  charRow=%d pixelCharY=%d  | offset=%d",
		 charColumn, pixelNum,
		 charRow, pixelCharY,
		 offset);
	
	u8 color = bitColor;	//0=00, 1=01, 2=10, 3=11
	
	u8 a = ~(0x03 << (3-pixelNum)*2);
	u8 o = color << (3-pixelNum)*2;
	
	
	
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


u8 C64VicDisplayCanvasMultiBitmap::GetColorAtPixel(int x, int y)
{
	// get pixel
	u8 pixelBitColor = this->GetBitPixelMultiBitmap(x, y);
	C64CharMulti *bitChar = this->GetBitCharMultiBitmap(x, y);
	
	u8 color = bitChar->colors[pixelBitColor];
	
	delete bitChar;

	return color;
}

u8 C64VicDisplayCanvasMultiBitmap::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 paintColor)
{
	return PutPixelMultiBitmap(forceColorReplace, x, y, paintColor);
}

u8 C64VicDisplayCanvasMultiBitmap::PutPixelMultiBitmap(bool forceColorReplace, int x, int y, u8 paintColor)
{
	LOGF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PutPixelMultiBitmap %d %d %02x", x, y, paintColor);
	
	u8 result = PAINT_RESULT_OK;
	
	u8 prevColor = GetBitPixelMultiBitmap(x, y);
	LOGF("prevColor=%02x", prevColor);
	
	C64CharMulti *bitChar = GetBitCharMultiBitmap(x, y);
	
	int newColor = -1;
	
	// find the same color
	for (int i = 0; i < 4; i++)
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
		// find surrounding color from histogram (s)
		for (int i = 1; i < 4; i++)
		{
			if (bitChar->histogram[i] < minColorCount)
			{
				minColorCount = bitChar->histogram[i];
				newColor = i;
			}
		}
		
		LOGF("   minColorCount=%d newColor=%02x", minColorCount, newColor);
		
		if (minColorCount != 0)
		{
			if (forceColorReplace == false)
			{
				return PAINT_RESULT_BLOCKED;
			}
			
			// force replace color
			if (prevColor != 0)
			{
				result = PAINT_RESULT_REPLACED_COLOR;
				
				newColor = prevColor;
				ReplaceColorMultiBitmapRaster(x, y, newColor, paintColor);
			}
		}
		
	}
	
	if (prevColor != newColor)
	{
		LOGF("newColor=%02x", newColor);
		PutBitPixelMultiBitmap(x, y, newColor);
		
		if (newColor != 0)
		{
			ReplaceColorMultiBitmapRaster(x, y, newColor, paintColor);
		}
	}
	
	delete bitChar;
	
	return result;
}

u8 C64VicDisplayCanvasMultiBitmap::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
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
			
			int dX = abs( x/2 - ditherMaskPosX/2 ) % 2;
			int dY = abs( y   - ditherMaskPosY   ) % 2;
			
			int d = (dX + dY) % 2;
			
			LOGF("==================== dX=%d dY=%d d=%d", dX, dY, d);
			
			if (d != 0)
			{
				paintColor = color2;
			}
		}
	}
	
	return this->PutColorAtPixel(forceColorReplace, x, y, paintColor);

}

// @returns painting status (ok, replaced color, blocked)
u8 C64VicDisplayCanvasMultiBitmap::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
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
u8 C64VicDisplayCanvasMultiBitmap::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return this->PaintDither(forceColorReplace, x, y, colorLMB, colorRMB, colorSource);
}

//
//
u8 C64VicDisplayCanvasMultiBitmap::ConvertFrom(CImageData *imageData)
{
	return ConvertFrom(imageData, false, 0);
}

u8 C64VicDisplayCanvasMultiBitmap::ConvertFrom(CImageData *imageData, bool isForceBackgroundColor, u8 forcedBackgroundColor)
{
	// TODO: 'bit align mode' to always align colors the same way in each char
//	return ConvertFromWithForcedColors(imageData, 0x00, 0x0F, 0x0B, 0x08);
	
	CDebugInterfaceC64 *debugInterface = vicDisplay->debugInterface;
	
	CImageData *image = ReducePalette(imageData, vicDisplay);
	
	std::vector<C64ColorsHistogramElement *> *colors = GetSortedColorsHistogram(image);
	
	u8 backgroundColor;
	if (isForceBackgroundColor == false)
	{
		backgroundColor = (*colors)[0]->color;
	}
	else
	{
		backgroundColor = forcedBackgroundColor;
	}
	
	LOGF("backgroundColor = %d numColors=%d", backgroundColor, colors->size());
	
	debugInterface->SetByteC64(0xD020, backgroundColor);
	debugInterface->SetByteC64(0xD021, backgroundColor);
	
	int histogram[16];
	
	// remember which index had color, to keep the same over all image if possible
	// TODO: find best combination knowing all histograms of all 8x8 chars, create indexes-colors map first and then parse image
	int commonIndexByColor[16];
	for (int i = 0; i < 16; i++)
	{
		commonIndexByColor[i] = -1;
	}
	
	int commonColorByIndex[4];
	for (int i = 0; i < 4; i++)
	{
		commonColorByIndex[i] = -1;
	}
	
	std::list<C64VicDisplayCanvasMultiBitmapMissingIndexes *> missingIndexes;
	
	for (int yc = 0; yc < 25; yc++)
	{
		for (int xc = 0; xc < 40; xc++)
		{
			LOGF(" xc=%d yc=%d", xc, yc);
			int x = xc * 8;
			int y = yc * 8;
			
			// find histogram from one bitmap character from reduced image
			memset(histogram, 0, 16*sizeof(int));
			
			for (int yb = 0; yb < 8; yb++)
			{
				for (int xb = 0; xb < 8; xb++)
				{
					u8 v = image->GetPixelResultByte(x + xb, y + yb);
					histogram[v]++;
				}
			}
			
			int colorForeground[3] = { 0, 0, 0 };
			
			// find second max color (foreground 1)
			int max = 0;
			colorForeground[0] = -1;
			
			for (int i = 0; i < 16; i++)
			{
//				LOGF(" histogram[%d] = %d", i, histogram[i]);
				
				if (i == backgroundColor)
					continue;
				
				if (histogram[i] > max)
				{
					max = histogram[i];
					colorForeground[0] = i;
				}
			}

			// find third max color (foreground 2)
			max = 0;
			colorForeground[1] = -1;
			
			for (int i = 0; i < 16; i++)
			{
				LOGF(" histogram[%d] = %d", i, histogram[i]);
				
				if (i == backgroundColor
					|| i == colorForeground[0])
					continue;
				
				if (histogram[i] > max)
				{
					max = histogram[i];
					colorForeground[1] = i;
				}
			}

			// find fourth max color (foreground 3)
			max = 0;
			colorForeground[2] = -1;
			
			for (int i = 0; i < 16; i++)
			{
//				LOGF(" histogram[%d] = %d", i, histogram[i]);
				
				if (i == backgroundColor
					|| i == colorForeground[0]
					|| i == colorForeground[1])
					continue;
				
				if (histogram[i] > max)
				{
					max = histogram[i];
					colorForeground[2] = i;
				}
			}
			
			int colorsToAssign[3];
			for (int i = 0; i < 3; i++)
			{
				colorsToAssign[i] = colorForeground[i];
			}
			
			LOGF("...checking commons");
			int colorByIndex[3] = { -1, -1, -1 };
			for (int i = 0; i < 3; i++)
			{
				if (colorForeground[i] == -1)
				{
					continue;
				}
				
				int commonColorIndex = commonIndexByColor[colorForeground[i]];
				LOGF("i=%d colorForeground[%d]=%02x commonColorIndex=%d", i, i, colorForeground[i], commonColorIndex);
				
				if (commonColorIndex != -1)
				{
					if (colorByIndex[commonColorIndex] == -1)
					{
						colorByIndex[commonColorIndex] = colorForeground[i];
						colorsToAssign[i] = -1;
						
						LOGF("---> set colorByIndex[%d]=%02x", commonColorIndex, colorForeground[i]);
					}
				}
			}
			
			LOGF("...checking if all are set");
			for (int i = 0; i < 3; i++)
			{
				if (colorByIndex[i] == -1)
				{
					for (int j = 0; j < 3; j++)
					{
						if (colorsToAssign[j] != -1)
						{
							LOGF("---> set colorByIndex[%d]=%02x", i, colorsToAssign[j]);
							colorByIndex[i] = colorsToAssign[j];
							colorsToAssign[j] = -1;
							break;
						}
					}
				}
			}
			
			for (int i = 0; i < 3; i++)
			{
				int color = colorByIndex[i];
				if (color != -1)
				{
					if (commonIndexByColor[color] == -1)
					{
						commonIndexByColor[color] = i;
					}
				}
				
				if (commonColorByIndex[i] == -1)
				{
					commonColorByIndex[i] = colorByIndex[i];
				}
				
				if (colorByIndex[i] == -1)
				{
					if (commonColorByIndex[i] != -1)
					{
						colorByIndex[i] = commonColorByIndex[i];
					}
					else
					{
						colorByIndex[i] = backgroundColor;
						C64VicDisplayCanvasMultiBitmapMissingIndexes *missingIndex = new C64VicDisplayCanvasMultiBitmapMissingIndexes();
						missingIndex->xc = xc;
						missingIndex->yc = yc;
						missingIndex->index = i;
						missingIndexes.push_back(missingIndex);
						LOGD(".. added missing index %d %d index=%d", xc, yc, i);
					}
				}
			}
			
			LOGF("xc=%d yc=%d | colors: %02x %02x %02x %02x", xc, yc, backgroundColor, colorByIndex[0], colorByIndex[1], colorByIndex[2]);
			
//			for (int i = 0; i < 16; i++)
//			{
//				LOGF("commonIndexByColor[%01x] = %d", i, commonIndexByColor[i]);
//			}
			
			// TODO: go through colors and replace background with common index
			
			ReplaceColorMultiBitmapCharacter(xc, yc, 1, colorByIndex[0]);
			ReplaceColorMultiBitmapCharacter(xc, yc, 2, colorByIndex[1]);
			ReplaceColorMultiBitmapCharacter(xc, yc, 3, colorByIndex[2]);
			
			for (int yb = 0; yb < 8; yb++)
			{
				for (int xb = 0; xb < 8; xb++)
				{
					u8 v = image->GetPixelResultByte(x + xb, y + yb);

					float distances[4];
					distances[0] = GetC64ColorDistance(v, backgroundColor, debugInterface);
					distances[1] = GetC64ColorDistance(v, colorByIndex[0], debugInterface);
					distances[2] = GetC64ColorDistance(v, colorByIndex[1], debugInterface);
					distances[3] = GetC64ColorDistance(v, colorByIndex[2], debugInterface);

					float minDistance = FLT_MAX;
					int minColorNum = 0;
					for (int i = 0; i < 4; i++)
					{
						if (distances[i] < minDistance)
						{
							minDistance = distances[i];
							minColorNum = i;
						}
					}
					
					PutBitPixelMultiBitmap(x + xb, y + yb, minColorNum);

				}
			}
		}
	}
	
	// update missing indexes
	LOGF("..update missing indexes");
	while(!missingIndexes.empty())
	{
		C64VicDisplayCanvasMultiBitmapMissingIndexes *missingIndex = missingIndexes.front();
		missingIndexes.pop_front();
		
		if (commonColorByIndex[missingIndex->index] != -1)
		{
			LOGF("replace: %d %d index=%d with %02x",
				 missingIndex->xc, missingIndex->yc, missingIndex->index, commonColorByIndex[missingIndex->index]);
			ReplaceColorMultiBitmapCharacter(missingIndex->xc, missingIndex->yc, missingIndex->index+1, commonColorByIndex[missingIndex->index]);
		}
		
		delete missingIndex;
	}
	
	DeleteColorsHistogram(colors);
	delete colors;

	delete image;
	return PAINT_RESULT_OK;
}

u8 C64VicDisplayCanvasMultiBitmap::ConvertFromWithForcedColors(CImageData *imageData,
															   u8 backgroundColor, u8 colorForeground1, u8 colorForeground2, u8 colorForeground3)
{
	CDebugInterfaceC64 *debugInterface = vicDisplay->debugInterface;
	
	CImageData *image = ReducePalette(imageData, vicDisplay);
	
	int histogram[16];
	
	for (int yc = 0; yc < 25; yc++)
	{
		for (int xc = 0; xc < 40; xc++)
		{
			LOGF(" xc=%d yc=%d", xc, yc);
			int x = xc * 8;
			int y = yc * 8;
			
			ReplaceColorMultiBitmapCharacter(xc, yc, 1, colorForeground1);
			ReplaceColorMultiBitmapCharacter(xc, yc, 2, colorForeground2);
			ReplaceColorMultiBitmapCharacter(xc, yc, 3, colorForeground3);
			
			for (int yb = 0; yb < 8; yb++)
			{
				for (int xb = 0; xb < 8; xb++)
				{
					u8 v = image->GetPixelResultByte(x + xb, y + yb);
					
					float distances[4];
					distances[0] = GetC64ColorDistance(v, backgroundColor, debugInterface);
					distances[1] = GetC64ColorDistance(v, colorForeground1, debugInterface);
					distances[2] = GetC64ColorDistance(v, colorForeground2, debugInterface);
					distances[3] = GetC64ColorDistance(v, colorForeground3, debugInterface);
					
					float minDistance = FLT_MAX;
					int minColorNum = 0;
					for (int i = 0; i < 4; i++)
					{
						if (distances[i] < minDistance)
						{
							minDistance = distances[i];
							minColorNum = i;
						}
					}
					
					PutBitPixelMultiBitmap(x + xb, y + yb, minColorNum);
					
				}
			}
		}
	}
	
	delete image;
	return PAINT_RESULT_OK;
}
