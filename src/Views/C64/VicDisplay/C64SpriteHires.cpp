#include "C64SpriteHires.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64Sprite.h"
#include "CViewC64Palette.h"
#include "CViewC64VicEditor.h"

// create empty hires sprite
C64SpriteHires::C64SpriteHires()
: C64Sprite(NULL, 24, 21, false)
{
	pixels = new u8[this->sizeX*this->sizeY];
	
	colors = new u8[2];
	histogram = new u8[2];
	
	memset(pixels, 0x00, this->sizeX*this->sizeY);
	memset(colors, 0x00, 2);
	memset(histogram, 0x00, 2);
}

// copy from hires bitmap
C64SpriteHires::C64SpriteHires(CViewC64VicEditor *vicEditor, int x, int y, bool isStretchedHorizontally, bool isStretchedVertically, int pointerValue, int pointerAddr)
: C64Sprite(vicEditor, 24, 21, false)
{
	this->posX = x;
	this->posY = y;
	this->isStretchedHorizontally = isStretchedHorizontally;
	this->isStretchedVertically = isStretchedVertically;

	this->pointerValue = pointerValue;
	this->pointerAddr = pointerAddr;

	pixels = new u8[this->sizeX*this->sizeY];
	
	colors = new u8[2];
	histogram = new u8[2];

	memset(colors, 0x00, 2);
	memset(histogram, 0x00, 2);
		
//	this->DebugPrint();
	
}

C64SpriteHires::C64SpriteHires(CViewC64VicEditor *vicEditor, CByteBuffer *byteBuffer)
: C64Sprite(vicEditor, 24, 21, false)
{
	pixels = new u8[this->sizeX*this->sizeY];
	colors = new u8[2];
	histogram = new u8[2];
	
	this->Deserialize(byteBuffer);
}



C64SpriteHires::~C64SpriteHires()
{
	delete [] pixels;
	delete [] colors;
	delete [] histogram;
}


void C64SpriteHires::SetPixel(int x, int y, u8 color)
{
	pixels[x + this->sizeX * y] = color;
}

u8 C64SpriteHires::GetPixel(int x, int y)
{
	return pixels[x + this->sizeX * y];
}

void C64SpriteHires::DebugPrint()
{
	LOGD(" >>  C64SpriteHires: x=%4d y=%4d  (%d %d) stretch: %s %s", this->posX, this->posY, this->posX + 0x18, this->posY + 0x32, STRBOOL(isStretchedHorizontally), STRBOOL(isStretchedVertically));

	
	// debug print char
	for (int pixelCharY = 0; pixelCharY < this->sizeY; pixelCharY++)
	{
		char buf[256];
		sprintf(buf, " %2d: ", pixelCharY);
		
		for (int pixelNum = 0; pixelNum < this->sizeX; pixelNum++)
		{
			char buf2[256];
			sprintf(buf2, " %d", GetPixel(pixelNum, pixelCharY));
			strcat(buf, buf2);
		}
		
		LOGD(buf);
	}
	
	LOGD(" colors: d020=%02X | color0=%02X (%3d) color1=%02X (%3d)",
		 this->frameColor,
		 this->colors[0], this->histogram[0],
		 this->colors[1], this->histogram[1]);
	LOGD("========== C64SpriteHires");
}

//
////
u8 C64SpriteHires::PutPixelHiresSprite(bool forceColorReplace, int x, int y, u8 paintColor, u8 replacementColorNum)
{
	LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> C64SpriteHires::PutPixelHiresSprite %d %d %02x  (force=%d)", x, y, paintColor, forceColorReplace);
	
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplay->debugInterface;
	CViewC64Sprite *viewSprite = vicEditor->viewSprite;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	vicEditor->layerVirtualSprites->ScanSpritesStoreAddressesOnly(x, y,
																  &addrPosXHighBits,
																  addrPosX,
																  addrPosY,
																  &addrSetStretchHorizontal,
																  &addrSetStretchVertical,
																  &addrSetMultiColor,
																  &addrColorChangeCommon1,
																  &addrColorChangeCommon2,
																  addrColorChange);
	
	int colorNum = viewSprite->selectedColor;
	if (colorNum == -1)
	{
		// 0 00 = D021
		// 1 01 = D025
		// 2 10 = D027+sprId
		// 3 11 = D026
		
		if (paintColor == viewSprite->paintColorD021)
		{
			colorNum = 0;
		}
		else if (paintColor == viewSprite->paintColorSprite)
		{
			colorNum = 2;
		}
		else
		{
			// not found, replace color
			if (forceColorReplace)
			{
				// always replace color with default for now
				colorNum = replacementColorNum;
				vicEditor->layerVirtualSprites->ReplaceColor(x, y, this->spriteId, colorNum, paintColor,
															 addrColorChange, addrColorChangeCommon1, addrColorChangeCommon2);
			}
			else
			{
				LOGD("   C64SpriteHires::PutPixelHiresSprite: blocked");
				return PAINT_RESULT_BLOCKED;
			}
		}
	}
	else
	{
		if (colorNum == 0 && paintColor != viewSprite->paintColorD021)
		{
			if (forceColorReplace)
			{
				return PAINT_RESULT_ERROR;
			}
			else
			{
				vicEditor->viewPalette->SetColorLMB(viewSprite->paintColorD021);
			}
		}
		else if (colorNum == 2 && paintColor != viewSprite->paintColorSprite)
		{
			if (forceColorReplace)
			{
				vicEditor->layerVirtualSprites->ReplaceColor(x, y, this->spriteId, colorNum, paintColor, addrColorChange, addrColorChangeCommon1, addrColorChangeCommon2);
			}
			else
			{
				vicEditor->viewPalette->SetColorLMB(viewSprite->paintColorSprite);
			}
		}
	}
	
	int spx = x - this->posX;
	int spy = y - this->posY;
	
	if (this->isStretchedHorizontally)
	{
		spx /= 2;
	}
	
	if (this->isStretchedVertically)
	{
		spy /= 2;
	}
	
	if (colorNum == 2)
		colorNum = 1;
	
	LOGD(" PAINT PutPixelHiresSprite: colorNum=%d spx=%d spy=%d", colorNum, spx, spy);
	
//	int rasterLine = y + 0x32;
//	int rasterCycle = (x + 0x88)/8;
//	vicii_cycle_state_t *viciiState = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);
	//	int v_bank = viciiState->vbank_phi1;
	//	int addr = v_bank + viciiState->sprite[spriteId].pointer * 64;
	
	int addr = this->pointerAddr;
	
	this->FetchSpriteData(addr);
	
	//this->DebugPrint();
	
	LOGD(" ... put pixel spx=%d spy=%d colorNum=%d", spx, spy, colorNum);
	
	this->SetPixel(spx, spy, colorNum);
	
	//this->DebugPrint();
	
	this->StoreSpriteData(addr);
	
	u8 result = PAINT_RESULT_OK;
	
	return result;
}

u8 C64SpriteHires::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	CVicEditorLayerVirtualSprites *layerSprites = this->vicEditor->layerVirtualSprites;
	
	u8 replacementColor = 1;
	u8 color1, color2;
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		color1 = colorLMB;
		color2 = colorRMB;
		replacementColor = 1;
	}
	else if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
	{
		color1 = colorRMB;
		color2 = colorLMB;
		replacementColor = 0;
	}
	
	u8 paintColor = color1;
	
	// check if starting dither
	{
		LOGD("													---- isAltPressed: dither -----");
		{
			// avoid values < 0 for dithering mask calculation, this does not reflect pixel's x,y just the dither mask value
			int xw = x + 100;
			int yw = y + 100;
			
			if (layerSprites->multiDitherMaskPosX == -1 || layerSprites->multiDitherMaskPosY == -1)
			{
				LOGD("******** START DITHER ********");
				// start dither painting
				if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
				{
					layerSprites->multiDitherMaskPosX = xw;
					layerSprites->hiresDitherMaskPosX = xw;
				}
				else if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
				{
					layerSprites->multiDitherMaskPosX = xw + 2;
					layerSprites->hiresDitherMaskPosX = xw;
				}
				
				layerSprites->multiDitherMaskPosY = yw;
			}
			
			int dX = abs( x/8 - layerSprites->hiresDitherMaskPosX/8 ) % 2;
			int dY = abs( y/8 - layerSprites->hiresDitherMaskPosY/8 ) % 2;
			
			int d = (dX + dY) % 2;
			
			LOGD("==================== dX=%d dY=%d d=%d", dX, dY, d);
			
			if (d != 0)
			{
				paintColor = color2;
				replacementColor = 0;
			}
		}
	}
	
	return this->PutPixelHiresSprite(forceColorReplace, x, y, paintColor, replacementColor);
	
}

// @returns painting status (ok, replaced color, blocked)
u8 C64SpriteHires::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	LOGD("C64SpriteHires::PutColorAtPixel: x=%d y=%d colorLMB=%x", colorLMB);
	
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		// always replace with sprite color
		return this->PutPixelHiresSprite(forceColorReplace, x, y, colorLMB, 2);
	}
	else
	{
		// always replace with background color
		return this->PutPixelHiresSprite(forceColorReplace, x, y, colorRMB, 2);
	}
}

u8 C64SpriteHires::GetColorAtPixel(int x, int y)
{
	LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> C64SpriteHires::GetColorAtPixel %d %d", x, y);
	
	CDebugInterfaceC64 *debugInterface = vicEditor->debugInterface;
	CViewC64Sprite *viewSprite = vicEditor->viewSprite;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	vicEditor->layerVirtualSprites->ScanSpritesStoreAddressesOnly(x, y,
																  &addrPosXHighBits,
																  addrPosX,
																  addrPosY,
																  &addrSetStretchHorizontal,
																  &addrSetStretchVertical,
																  &addrSetMultiColor,
																  &addrColorChangeCommon1,
																  &addrColorChangeCommon2,
																  addrColorChange);
	
	int spx = x - this->posX;
	int spy = y - this->posY;
	
	if (this->isStretchedHorizontally)
	{
		spx /= 2;
	}
	
	if (this->isStretchedVertically)
	{
		spy /= 2;
	}
	
	int rasterLine = y + 0x32;
	int rasterCycle = (x + 0x88)/8;
	vicii_cycle_state_t *viciiState = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);

//	//	int v_bank = viciiState->vbank_phi1;
//	//	int addr = v_bank + viciiState->sprite[spriteId].pointer * 64;
	
	int addr = this->pointerAddr;
	
	this->FetchSpriteData(addr);
	
	//this->DebugPrint();
	
	u8 colorNum = this->GetPixel(spx, spy);
	
	// 0 00 = D021
	// 1 01 = D025
	// 2 10 = D027+sprId
	// 3 11 = D026
	
	if (colorNum == 0)
	{
		return viciiState->regs[0x21];
	}
	else if (colorNum == 1)
	{
		return viciiState->regs[0x27 + spriteId];
	}
	
	return 0;
}

void C64SpriteHires::FetchSpriteData(int addr)
{
	LOGD("C64SpriteHires::FetchSpriteData: addr=%04x", addr);
	
	int a = addr;
	
	for (int i = 0; i < 63; i++)
	{
		u8 v;
		v = vicEditor->debugInterface->GetByteFromRamC64(a);
		spriteData[i] = v;
		
		//LOGD("  get %04x spriteData[%d]=%02x", a, i, v);
		a++;
	}
	
	a = addr;
	
	u8 *chd = spriteData;
	int i = 0;
	for (int y = 0; y < 21; y++)
	{
		int chx = 0;
		for (int x = 0; x < 3; x++)
		{
			if ((*chd & 0x01) == 0x01)
			{
				SetPixel(chx + 7, y, 1);
			}
			else
			{
				SetPixel(chx + 7, y, 0);
			}
			
			if ((*chd & 0x02) == 0x02)
			{
				SetPixel(chx + 6, y, 1);
			}
			else
			{
				SetPixel(chx + 6, y, 0);
			}
			
			if ((*chd & 0x04) == 0x04)
			{
				SetPixel(chx + 5, y, 1);
			}
			else
			{
				SetPixel(chx + 5, y, 0);
			}
			
			if ((*chd & 0x08) == 0x08)
			{
				SetPixel(chx + 4, y, 1);
			}
			else
			{
				SetPixel(chx + 4, y, 0);
			}
			
			if ((*chd & 0x10) == 0x10)
			{
				SetPixel(chx + 3, y, 1);
			}
			else
			{
				SetPixel(chx + 3, y, 0);
			}
			
			if ((*chd & 0x20) == 0x20)
			{
				SetPixel(chx + 2, y, 1);
			}
			else
			{
				SetPixel(chx + 2, y, 0);
			}
			
			if ((*chd & 0x40) == 0x40)
			{
				SetPixel(chx + 1, y, 1);
			}
			else
			{
				SetPixel(chx + 1, y, 0);
			}
			
			if ((*chd & 0x80) == 0x80)
			{
				SetPixel(chx + 0, y, 1);
			}
			else
			{
				SetPixel(chx + 0, y, 0);
			}
			
			
			chx += 8;
			chd++;
			i++;
		}
	}
	
}

void C64SpriteHires::StoreSpriteData(int addr)
{
	LOGD("C64SpriteHires::StoreSpriteData, addr=%04x", addr);
	
	u8 *chd = spriteData;
	for (int y = 0; y < 21; y++)
	{
		int chx = 0;
		for (int x = 0; x < 3; x++)
		{
			u8 p0 = GetPixel(chx + 7, y);
			u8 p1 = GetPixel(chx + 6, y);
			u8 p2 = GetPixel(chx + 5, y);
			u8 p3 = GetPixel(chx + 4, y);
			u8 p4 = GetPixel(chx + 3, y);
			u8 p5 = GetPixel(chx + 2, y);
			u8 p6 = GetPixel(chx + 1, y);
			u8 p7 = GetPixel(chx + 0, y);
			
			u8 v = p7 << 7 | p6 << 6 | p5 << 5 | p4 << 4 | p3 << 3 | p2 << 2 | p1 << 1 | p0;
			
			*chd = v;
			
			chx += 8;
			chd++;
		}
	}
	
	int a = addr;
	
	chd = spriteData;
	for (int i = 0; i < 63; i++)
	{
		vicEditor->debugInterface->SetByteToRamC64(a, *chd);
		
		LOGD("  set %04x spriteData[%d]=%02x", a, i, *chd);
		a++;
		chd++;
	}
}

void C64SpriteHires::Clear()
{
	int addr = this->pointerAddr;
	for (int x = 0; x < this->sizeX; x++)
	{
		for (int y = 0; y < this->sizeY; y++)
		{
			this->SetPixel(x, y, 0);
		}
	}
	
	this->StoreSpriteData(addr);
	
}

void C64SpriteHires::Serialize(CByteBuffer *byteBuffer)
{
	byteBuffer->PutI32(spriteId);
	byteBuffer->PutU8(spriteColor);
	byteBuffer->PutI32(posX);
	byteBuffer->PutI32(posY);
	byteBuffer->PutBool(isStretchedHorizontally);
	byteBuffer->PutBool(isStretchedVertically);
	byteBuffer->PutI32(pointerAddr);
	byteBuffer->PutI32(rasterLine);
	byteBuffer->PutI32(rasterCycle);
	
	int a = pointerAddr;
	
	for (int i = 0; i < 63; i++)
	{
		u8 v;
		v = vicEditor->debugInterface->GetByteFromRamC64(a);
		spriteData[i] = v;
		a++;
	}
	
	byteBuffer->PutBytes(spriteData, 63);
}

void C64SpriteHires::Deserialize(CByteBuffer *byteBuffer)
{
	spriteId = byteBuffer->GetI32();
	spriteColor = byteBuffer->GetU8();
	posX = byteBuffer->GetI32();
	posY = byteBuffer->GetI32();
	isStretchedHorizontally = byteBuffer->GetBool();
	isStretchedVertically = byteBuffer->GetBool();
	pointerAddr = byteBuffer->GetI32();
	rasterLine = byteBuffer->GetI32();
	rasterCycle = byteBuffer->GetI32();
	
	byteBuffer->GetBytes(spriteData, 63);
	
	if (pointerAddr >= 0 && pointerAddr <= 0xFFC0)
	{
		int a = pointerAddr;
		
		u8 *chd = spriteData;
		for (int i = 0; i < 63; i++)
		{
			vicEditor->debugInterface->SetByteToRamC64(a, *chd);
			a++;
			chd++;
		}
	}
}

