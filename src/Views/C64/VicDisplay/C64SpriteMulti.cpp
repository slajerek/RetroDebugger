#include "CViewVicEditor.h"
#include "C64SpriteMulti.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64Sprite.h"

// 0 00 = D021
// 1 01 = D025
// 2 10 = D027+sprId
// 3 11 = D026


C64SpriteMulti::C64SpriteMulti()
: C64Sprite(NULL, 12, 21, true)
{
	pixels = new u8[this->sizeX*this->sizeY];
	
	colors = new u8[4];
	histogram = new u8[4];
	
	memset(pixels, 0x00, this->sizeX*this->sizeY);
	memset(colors, 0x00, 4);
	memset(histogram, 0x00, 4);
	
	vicEditor = NULL;
}

C64SpriteMulti::C64SpriteMulti(CViewVicEditor *vicEditor)
: C64Sprite(vicEditor, 12, 21, true)
{
	pixels = new u8[this->sizeX*this->sizeY];
	
	colors = new u8[4];
	histogram = new u8[4];
	
	memset(pixels, 0x00, this->sizeX*this->sizeY);
	memset(colors, 0x00, 4);
	memset(histogram, 0x00, 4);
}

// copy from multi bitmap
C64SpriteMulti::C64SpriteMulti(CViewVicEditor *vicEditor, int x, int y, bool isStretchedHorizontally, bool isStretchedVertically, int pointerValue, int pointerAddr)
: C64Sprite(vicEditor, 12, 21, true)
{
	this->posX = x;
	this->posY = y;
	this->isStretchedHorizontally = isStretchedHorizontally;
	this->isStretchedVertically = isStretchedVertically;
	
	this->pointerValue = pointerValue;
	this->pointerAddr = pointerAddr;
	
	pixels = new u8[sizeX*sizeY];
	
	colors = new u8[4];
	histogram = new u8[4];
	
	memset(colors, 0x00, 4);
	memset(histogram, 0x00, 4);
	
//	this->DebugPrint();
	
}

C64SpriteMulti::C64SpriteMulti(CViewVicEditor *vicEditor, CByteBuffer *byteBuffer)
: C64Sprite(vicEditor, 12, 21, true)
{
	pixels = new u8[sizeX*sizeY];
	colors = new u8[4];
	histogram = new u8[4];
	
	this->Deserialise(byteBuffer);
}

C64SpriteMulti::~C64SpriteMulti()
{
	delete [] pixels;
	delete [] colors;
	delete [] histogram;
}

void C64SpriteMulti::SetPixel(int x, int y, u8 color)
{
	pixels[x + sizeX * y] = color;
}

u8 C64SpriteMulti::GetPixel(int x, int y)
{
	return pixels[x + sizeX * y];
}

void C64SpriteMulti::DebugPrint()
{
	LOGD(" >>  C64SpriteMulti: x=%4d y=%4d  (%3d %3d) streth %s %s", this->posX, this->posY,
		 this->posX + 0x18, this->posY + 0x32,
		 STRBOOL(isStretchedHorizontally), STRBOOL(isStretchedVertically));

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
	
	LOGD(" colors: d020=%02X | d021=%02X (%3d) sprite=%02X (%3d) common1=%02X (%3d) common2=%02X (%3d)",
		 this->frameColor,
		 this->colors[0], this->histogram[0],
		 this->colors[1], this->histogram[1],
		 this->colors[2], this->histogram[2],
		 this->colors[3], this->histogram[3]);
	
}

////
u8 C64SpriteMulti::PutPixelMultiSprite(bool forceColorReplace, int x, int y, u8 paintColor, u8 replacementColorNum)
{
	LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PutPixelMultiSprite %d %d %02x", x, y, paintColor);
	
//	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
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

	if (paintColor == viewSprite->paintColorD021)
	{
		colorNum = 0;
	}

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
		else if (paintColor == viewSprite->paintColorD025)
		{
			colorNum = 1;
		}
		else if (paintColor == viewSprite->paintColorD026)
		{
			colorNum = 3;
		}
		else
		{
			// not found, replace color
			if (forceColorReplace)
			{
				// always replace color with default for now
				colorNum = replacementColorNum;
				vicEditor->layerVirtualSprites->ReplaceColor(x, y, this->spriteId, colorNum, paintColor, addrColorChange, addrColorChangeCommon1, addrColorChangeCommon2);
			}
			else
			{
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
			}
			else
			{
				vicEditor->viewPalette->SetColorLMB(viewSprite->paintColorD021);
			}
		}
		else if (colorNum == 1 && paintColor != viewSprite->paintColorD025)
		{
			if (forceColorReplace)
			{
				vicEditor->layerVirtualSprites->ReplaceColor(x, y, this->spriteId, colorNum, paintColor, addrColorChange, addrColorChangeCommon1, addrColorChangeCommon2);
			}
			else
			{
				vicEditor->viewPalette->SetColorLMB(viewSprite->paintColorD025);
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
		else if (colorNum == 3 && paintColor != viewSprite->paintColorD026)
		{
			if (forceColorReplace)
			{
				vicEditor->layerVirtualSprites->ReplaceColor(x, y, this->spriteId, colorNum, paintColor, addrColorChange, addrColorChangeCommon1, addrColorChangeCommon2);
			}
			else
			{
				vicEditor->viewPalette->SetColorLMB(viewSprite->paintColorD026);
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
	
	LOGD(" PAINT PutPixelMultiSprite: colorNum=%d spx=%d spy=%d", colorNum, spx, spy);

	int addr;
	
	if (spy > 2)
	{
		int rasterLine = y + 0x33;
		int rasterCycle = (x + 0x88)/8;
		vicii_cycle_state_t *viciiState = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);
		int v_bank = viciiState->vbank_phi1;
		addr = v_bank + viciiState->sprite[spriteId].pointer * 64;
	}
	else
	{
		addr = this->pointerAddr;
	}
	
	LOGD(" ... addr=%04x\n", addr);
	
	this->FetchSpriteData(addr);
	
	//this->DebugPrint();
	
	// multi
	spx /= 2;
	LOGD(" ... put pixel spx=%d spy=%d colorNum=%d", spx, spy, colorNum);
	
	this->SetPixel(spx, spy, colorNum);
	
	//this->DebugPrint();

	this->StoreSpriteData(addr);
	
	u8 result = PAINT_RESULT_OK;

	return result;
}

u8 C64SpriteMulti::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	CVicEditorLayerVirtualSprites *layerSprites = this->vicEditor->layerVirtualSprites;
	
	u8 replacementColor = 2;
	u8 color1, color2;
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		color1 = colorLMB;
		color2 = colorRMB;
		replacementColor = 2;
	}
	else if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
	{
		color1 = colorRMB;
		color2 = colorLMB;
		replacementColor = 1;
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
			
			int dX = abs( xw/2 - layerSprites->multiDitherMaskPosX/2 ) % 2;
			int dY = abs( yw   - layerSprites->multiDitherMaskPosY   ) % 2;
			
			int d = (dX + dY) % 2;
			
			LOGD("==================== dX=%d dY=%d d=%d", dX, dY, d);
			
			if (d != 0)
			{
				paintColor = color2;
				replacementColor = 1;
			}
		}
	}
	
	return this->PutPixelMultiSprite(forceColorReplace, x, y, paintColor, replacementColor);
	
}

// @returns painting status (ok, replaced color, blocked)
u8 C64SpriteMulti::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	if (colorSource == VICEDITOR_COLOR_SOURCE_LMB)
	{
		// always replace with sprite color
		return this->PutPixelMultiSprite(forceColorReplace, x, y, colorLMB, 2);
	}
	else
	{
		// always replace with sprite color
		return this->PutPixelMultiSprite(forceColorReplace, x, y, colorRMB, 2);
	}
}

u8 C64SpriteMulti::GetColorAtPixel(int x, int y)
{
	LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> GetColorAtPixel %d %d", x, y);
	
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
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
	
//	int v_bank = viciiState->vbank_phi1;
//	int addr = v_bank + viciiState->sprite[spriteId].pointer * 64;

	int addr = this->pointerAddr;
	
	this->FetchSpriteData(addr);
	
	//this->DebugPrint();
	
	// multi
	spx /= 2;

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
		return viciiState->regs[0x25];
	}
	else if (colorNum == 2)
	{
		return viciiState->regs[0x27 + spriteId];
	}
	else if (colorNum == 3)
	{
		return viciiState->regs[0x26];
	}
	
	return 0;
}

void C64SpriteMulti::FetchSpriteData(int addr)
{
	LOGD("C64SpriteMulti::FetchSpriteData: addr=%04x", addr);
	
	int a = addr;
	
	for (int i = 0; i < 63; i++)
	{
		u8 v;
		v = vicEditor->viewVicDisplayMain->debugInterface->GetByteFromRamC64(a);
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
			u8 v;
			
			//LOGD("  sprite[%d]=%02x", i, (*chd));
			
			// 00000011
			v = (*chd & 0x03);
			if (v == 0x01)
			{
				//D025 = 01 | 1
				SetPixel(chx + 3, y, 1);
			}
			else if (v == 0x03)
			{
				//D026 = 11 | 3
				SetPixel(chx + 3, y, 3);
			}
			else if (v == 0x02)
			{
				//D027 = 10 | 2
				SetPixel(chx + 3, y, 2);
			}
			else
			{
				SetPixel(chx + 3, y, 0);
			}
			
			// 00001100
			v = (*chd & 0x0C);
			if (v == 0x04)
			{
				//D025
				SetPixel(chx + 2, y, 1);
			}
			else if (v == 0x0C)
			{
				//D026
				SetPixel(chx + 2, y, 3);
			}
			else if (v == 0x08)
			{
				//D027
				SetPixel(chx + 2, y, 2);
			}
			else
			{
				SetPixel(chx + 2, y, 0);
			}

			// 00110000
			v = (*chd & 0x30);
			if (v == 0x10)
			{
				//D025
				SetPixel(chx + 1, y, 1);
			}
			else if (v == 0x30)
			{
				//D026
				SetPixel(chx + 1, y, 3);
			}
			else if (v == 0x20)
			{
				//D027
				SetPixel(chx + 1, y, 2);
			}
			else
			{
				SetPixel(chx + 1, y, 0);
			}

			// 11000000
			v = (*chd & 0xC0);
			if (v == 0x40)
			{
				//D025
				SetPixel(chx + 0, y, 1);
			}
			else if (v == 0xC0)
			{
				//D026
				SetPixel(chx + 0, y, 3);
			}
			else if (v == 0x80)
			{
				//D027
				SetPixel(chx + 0, y, 2);
			}
			else
			{
				SetPixel(chx + 0, y, 0);
			}

			chx += 4;
			chd++;
			i++;
		}
	}

}

void C64SpriteMulti::StoreSpriteData(int addr)
{
	u8 *chd = spriteData;
	for (int y = 0; y < 21; y++)
	{
		int chx = 0;
		for (int x = 0; x < 3; x++)
		{
			u8 p1 = GetPixel(chx + 3, y);
			u8 p2 = GetPixel(chx + 2, y);
			u8 p3 = GetPixel(chx + 1, y);
			u8 p4 = GetPixel(chx + 0, y);
			
			u8 v = p4 << 6 | p3 << 4 | p2 << 2 | p1;
			
			*chd = v;
			
			chx += 4;
			chd++;
		}
	}

	int a = addr;
	
	chd = spriteData;
	for (int i = 0; i < 63; i++)
	{
		vicEditor->viewVicDisplayMain->debugInterface->SetByteToRamC64(a, *chd);
		
		//LOGD("  set %04x spriteData[%d]=%02x", a, i, *chd);
		a++;
		chd++;
	}
}

void C64SpriteMulti::Clear()
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


void C64SpriteMulti::Serialise(CByteBuffer *byteBuffer)
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
		v = vicEditor->viewVicDisplayMain->debugInterface->GetByteFromRamC64(a);
		spriteData[i] = v;
		a++;
	}

	byteBuffer->PutBytes(spriteData, 63);
}

void C64SpriteMulti::Deserialise(CByteBuffer *byteBuffer)
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
			vicEditor->viewVicDisplayMain->debugInterface->SetByteToRamC64(a, *chd);
			a++;
			chd++;
		}
	}
}


