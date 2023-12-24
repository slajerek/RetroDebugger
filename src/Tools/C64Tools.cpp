#include "psid64.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CSlrFontProportional.h"
#include "VID_ImageBinding.h"
#include "C64Tools.h"
#include "CSlrString.h"
#include "CDebugInterfaceC64.h"
#include "C64SIDFrequencies.h"
#include "C64SettingsStorage.h"
#include "CColorsTheme.h"
#include "CViewC64.h"
#include "exomizer.h"
#include "SidTuneMod.h"
#include "CGuiMain.h"
#include <float.h>

u8 ConvertPetsciiToScreenCode(u8 chr)
{
	if (chr >= 0x00 && chr <= 0x1F)
		return chr + 0x80;
	else if (chr >= 0x20 && chr <= 0x3F)
		return chr;
	else if (chr >= 0x40 && chr <= 0x5F)
		return chr - 0x40;
	else if (chr >= 0x60 && chr <= 0x7F)
		return chr - 0x20;
	else if (chr >= 0x80 && chr <= 0x9F)
		return chr + 0x40;
	else if (chr >= 0xA0 && chr <= 0xBF)
		return chr - 0x40;
	else if (chr >= 0xC0 && chr <= 0xDF)
		return chr - 0x80;
	else if (chr >= 0xC0 && chr <= 0xDF)
		return chr - 0x80;
	else if (chr >= 0xE0 && chr <= 0xFE)
		return chr - 0x80;

	// $FF
	return 0x5E;
}

void AddCBMScreenCharacters(CSlrFontProportional *font);
void AddASCIICharacters(CSlrFontProportional *font);

void ConvertCharacterDataToImage(u8 *characterData, CImageData *imageData)
{
	u8 *chd = characterData;
	
	int gap = 4;

	int chx = 0 + gap;
	int chy = 0 + gap;

	imageData->EraseContent(0,0,0,255);
	
	// copy pixels around character for better linear scaling
	for (int y = -1; y < 9; y++)
	{
		if ((*chd & 0x01) == 0x01)
		{
			imageData->SetPixelResultRGBA(chx + 8, chy + y, 255, 255, 255, 255);
			imageData->SetPixelResultRGBA(chx + 7, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x02) == 0x02)
		{
			imageData->SetPixelResultRGBA(chx + 6, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x04) == 0x04)
		{
			imageData->SetPixelResultRGBA(chx + 5, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x08) == 0x08)
		{
			imageData->SetPixelResultRGBA(chx + 4, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x10) == 0x10)
		{
			imageData->SetPixelResultRGBA(chx + 3, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x20) == 0x20)
		{
			imageData->SetPixelResultRGBA(chx + 2, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x40) == 0x40)
		{
			imageData->SetPixelResultRGBA(chx + 1, chy + y, 255, 255, 255, 255);
		}
		if ((*chd & 0x80) == 0x80)
		{
			imageData->SetPixelResultRGBA(chx + 0, chy + y, 255, 255, 255, 255);
			imageData->SetPixelResultRGBA(chx - 1, chy + y, 255, 255, 255, 255);
		}
		
		if (y == -1 || y == 7)
			continue;
		
		chd++;
	}
}

void ConvertColorCharacterDataToImage(u8 *characterData, CImageData *imageData, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800, CDebugInterfaceC64 *debugInterface)
{
	u8 cD021r, cD021g, cD021b;
	u8 cD022r, cD022g, cD022b;
	u8 cD023r, cD023g, cD023b;
	u8 cD800r, cD800g, cD800b;
	
	debugInterface->GetCBMColor(colorD021, &cD021r, &cD021g, &cD021b);
	debugInterface->GetCBMColor(colorD022, &cD022r, &cD022g, &cD022b);
	debugInterface->GetCBMColor(colorD023, &cD023r, &cD023g, &cD023b);
	debugInterface->GetCBMColor(colorD800, &cD800r, &cD800g, &cD800b);

	u8 *chd = characterData;
	
	int gap = 4;
	
	int chx = 0 + gap;
	int chy = 0 + gap;
	
	imageData->EraseContent(0,0,0,255);
	
	// copy pixels around character for better linear scaling
	for (int y = -1; y < 9; y++)
	{
		u8 v;
		
		// 00000011
		v = (*chd & 0x03);
		if (v == 0x01)
		{
			imageData->SetPixelResultRGBA(chx + 8, chy + y, cD022r, cD022g, cD022b, 255);
			imageData->SetPixelResultRGBA(chx + 7, chy + y, cD022r, cD022g, cD022b, 255);
			imageData->SetPixelResultRGBA(chx + 6, chy + y, cD022r, cD022g, cD022b, 255);
		}
		else if (v == 0x02)
		{
			imageData->SetPixelResultRGBA(chx + 8, chy + y, cD023r, cD023g, cD023b, 255);
			imageData->SetPixelResultRGBA(chx + 7, chy + y, cD023r, cD023g, cD023b, 255);
			imageData->SetPixelResultRGBA(chx + 6, chy + y, cD023r, cD023g, cD023b, 255);
		}
		else if (v == 0x03)
		{
			imageData->SetPixelResultRGBA(chx + 8, chy + y, cD800r, cD800g, cD800b, 255);
			imageData->SetPixelResultRGBA(chx + 7, chy + y, cD800r, cD800g, cD800b, 255);
			imageData->SetPixelResultRGBA(chx + 6, chy + y, cD800r, cD800g, cD800b, 255);
		}

		// 00001100
		v = (*chd & 0x0C);
		if (v == 0x04)
		{
			imageData->SetPixelResultRGBA(chx + 5, chy + y, cD022r, cD022g, cD022b, 255);
			imageData->SetPixelResultRGBA(chx + 4, chy + y, cD022r, cD022g, cD022b, 255);
		}
		else if (v == 0x08)
		{
			imageData->SetPixelResultRGBA(chx + 5, chy + y, cD023r, cD023g, cD023b, 255);
			imageData->SetPixelResultRGBA(chx + 4, chy + y, cD023r, cD023g, cD023b, 255);
		}
		else if (v == 0x0C)
		{
			imageData->SetPixelResultRGBA(chx + 5, chy + y, cD800r, cD800g, cD800b, 255);
			imageData->SetPixelResultRGBA(chx + 4, chy + y, cD800r, cD800g, cD800b, 255);
		}

		// 00110000
		v = (*chd & 0x30);
		if (v == 0x10)
		{
			imageData->SetPixelResultRGBA(chx + 3, chy + y, cD022r, cD022g, cD022b, 255);
			imageData->SetPixelResultRGBA(chx + 2, chy + y, cD022r, cD022g, cD022b, 255);
		}
		else if (v == 0x20)
		{
			imageData->SetPixelResultRGBA(chx + 3, chy + y, cD023r, cD023g, cD023b, 255);
			imageData->SetPixelResultRGBA(chx + 2, chy + y, cD023r, cD023g, cD023b, 255);
		}
		else if (v == 0x30)
		{
			imageData->SetPixelResultRGBA(chx + 3, chy + y, cD800r, cD800g, cD800b, 255);
			imageData->SetPixelResultRGBA(chx + 2, chy + y, cD800r, cD800g, cD800b, 255);
		}
		
		// 11000000
		v = (*chd & 0xC0);
		if (v == 0x40)
		{
			imageData->SetPixelResultRGBA(chx + 1, chy + y, cD022r, cD022g, cD022b, 255);
			imageData->SetPixelResultRGBA(chx    , chy + y, cD022r, cD022g, cD022b, 255);
			imageData->SetPixelResultRGBA(chx - 1, chy + y, cD022r, cD022g, cD022b, 255);
		}
		else if (v == 0x80)
		{
			imageData->SetPixelResultRGBA(chx + 1, chy + y, cD023r, cD023g, cD023b, 255);
			imageData->SetPixelResultRGBA(chx    , chy + y, cD023r, cD023g, cD023b, 255);
			imageData->SetPixelResultRGBA(chx - 1, chy + y, cD023r, cD023g, cD023b, 255);
		}
		else if (v == 0xC0)
		{
			imageData->SetPixelResultRGBA(chx + 1, chy + y, cD800r, cD800g, cD800b, 255);
			imageData->SetPixelResultRGBA(chx    , chy + y, cD800r, cD800g, cD800b, 255);
			imageData->SetPixelResultRGBA(chx - 1, chy + y, cD800r, cD800g, cD800b, 255);
		}
		
		if (y == -1 || y == 7)
			continue;
		
		chd++;
	}
}

void ConvertSpriteDataToImage(u8 *spriteData, CImageData *imageData, int gap)
{
	ConvertSpriteDataToImage(spriteData, imageData, 0, 0, 0, 255, 255, 255, gap);
}

void ConvertSpriteDataToImage(u8 *spriteData, CImageData *imageData, u8 colorD021, u8 colorD027, CDebugInterfaceC64 *debugInterface, int gap)
{
	u8 cD021r, cD021g, cD021b;
	u8 cD027r, cD027g, cD027b;
	
	debugInterface->GetCBMColor(colorD021, &cD021r, &cD021g, &cD021b);
	debugInterface->GetCBMColor(colorD027, &cD027r, &cD027g, &cD027b);

	ConvertSpriteDataToImage(spriteData, imageData, cD021r, cD021g, cD021b, cD027r, cD027g, cD027b, gap);
}

void ConvertSpriteDataToImage(u8 *spriteData, CImageData *imageData, u8 bkgColorR, u8 bkgColorG, u8 bkgColorB, u8 spriteColorR, u8 spriteColorG, u8 spriteColorB, int gap)
{
	u8 *chd = spriteData;
	
	int chy = 0 + gap;
	
	// erase with transparent pixels
	imageData->EraseContent(bkgColorR, bkgColorG, bkgColorB, 0);
	
	// copy pixels around character for better linear scaling
	for (int y = 0; y < 21; y++)
	{
		int chx = 0 + gap;
		for (int x = 0; x < 3; x++)
		{
			if ((*chd & 0x01) == 0x01)
			{
				imageData->SetPixelResultRGBA(chx + 7, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x02) == 0x02)
			{
				imageData->SetPixelResultRGBA(chx + 6, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x04) == 0x04)
			{
				imageData->SetPixelResultRGBA(chx + 5, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x08) == 0x08)
			{
				imageData->SetPixelResultRGBA(chx + 4, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x10) == 0x10)
			{
				imageData->SetPixelResultRGBA(chx + 3, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x20) == 0x20)
			{
				imageData->SetPixelResultRGBA(chx + 2, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x40) == 0x40)
			{
				imageData->SetPixelResultRGBA(chx + 1, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}
			if ((*chd & 0x80) == 0x80)
			{
				imageData->SetPixelResultRGBA(chx + 0, chy + y, spriteColorR, spriteColorG, spriteColorB, 255);
			}

			chx += 8;
			chd++;
		}
	}
}


void ConvertColorSpriteDataToImage(u8 *spriteData, CImageData *imageData, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027, CDebugInterfaceC64 *debugInterface, int gap, u8 alpha)
{
	u8 cD021r, cD021g, cD021b;
	u8 cD025r, cD025g, cD025b;
	u8 cD026r, cD026g, cD026b;
	u8 cD027r, cD027g, cD027b;
	
	debugInterface->GetCBMColor(colorD021, &cD021r, &cD021g, &cD021b);
	debugInterface->GetCBMColor(colorD025, &cD025r, &cD025g, &cD025b);
	debugInterface->GetCBMColor(colorD026, &cD026r, &cD026g, &cD026b);
	debugInterface->GetCBMColor(colorD027, &cD027r, &cD027g, &cD027b);
	
	
	u8 *chd = spriteData;
	
	int chy = 0 + gap;
	
	// erase with transparent pixels
	imageData->EraseContent(cD021r, cD021g, cD021b, alpha);
	
	// copy pixels around character for better linear scaling
	for (int y = 0; y < 21; y++)
	{
		int chx = 0 + gap;
		for (int x = 0; x < 3; x++)
		{
			u8 v;
			
			// 00000011
			v = (*chd & 0x03);
			if (v == 0x01)
			{
				//D025
				imageData->SetPixelResultRGBA(chx + 7, chy + y, cD025r, cD025g, cD025b, 255);
				imageData->SetPixelResultRGBA(chx + 6, chy + y, cD025r, cD025g, cD025b, 255);
			}
			else if (v == 0x03)
			{
				//D026
				imageData->SetPixelResultRGBA(chx + 7, chy + y, cD026r, cD026g, cD026b, 255);
				imageData->SetPixelResultRGBA(chx + 6, chy + y, cD026r, cD026g, cD026b, 255);
			}
			else if (v == 0x02)
			{
				//D027
				imageData->SetPixelResultRGBA(chx + 7, chy + y, cD027r, cD027g, cD027b, 255);
				imageData->SetPixelResultRGBA(chx + 6, chy + y, cD027r, cD027g, cD027b, 255);
			}

			// 00001100
			v = (*chd & 0x0C);
			if (v == 0x04)
			{
				//D025
				imageData->SetPixelResultRGBA(chx + 5, chy + y, cD025r, cD025g, cD025b, 255);
				imageData->SetPixelResultRGBA(chx + 4, chy + y, cD025r, cD025g, cD025b, 255);
			}
			else if (v == 0x0C)
			{
				//D026
				imageData->SetPixelResultRGBA(chx + 5, chy + y, cD026r, cD026g, cD026b, 255);
				imageData->SetPixelResultRGBA(chx + 4, chy + y, cD026r, cD026g, cD026b, 255);
			}
			else if (v == 0x08)
			{
				//D027
				imageData->SetPixelResultRGBA(chx + 5, chy + y, cD027r, cD027g, cD027b, 255);
				imageData->SetPixelResultRGBA(chx + 4, chy + y, cD027r, cD027g, cD027b, 255);
			}

			// 00110000
			v = (*chd & 0x30);
			if (v == 0x10)
			{
				//D025
				imageData->SetPixelResultRGBA(chx + 3, chy + y, cD025r, cD025g, cD025b, 255);
				imageData->SetPixelResultRGBA(chx + 2, chy + y, cD025r, cD025g, cD025b, 255);
			}
			else if (v == 0x30)
			{
				//D026
				imageData->SetPixelResultRGBA(chx + 3, chy + y, cD026r, cD026g, cD026b, 255);
				imageData->SetPixelResultRGBA(chx + 2, chy + y, cD026r, cD026g, cD026b, 255);
			}
			else if (v == 0x20)
			{
				//D027
				imageData->SetPixelResultRGBA(chx + 3, chy + y, cD027r, cD027g, cD027b, 255);
				imageData->SetPixelResultRGBA(chx + 2, chy + y, cD027r, cD027g, cD027b, 255);
			}
			
			// 11000000
			v = (*chd & 0xC0);
			if (v == 0x40)
			{
				//D025
				imageData->SetPixelResultRGBA(chx + 1, chy + y, cD025r, cD025g, cD025b, 255);
				imageData->SetPixelResultRGBA(chx    , chy + y, cD025r, cD025g, cD025b, 255);
			}
			else if (v == 0xC0)
			{
				//D026
				imageData->SetPixelResultRGBA(chx + 1, chy + y, cD026r, cD026g, cD026b, 255);
				imageData->SetPixelResultRGBA(chx    , chy + y, cD026r, cD026g, cD026b, 255);
			}
			else if (v == 0x80)
			{
				//D027
				imageData->SetPixelResultRGBA(chx + 1, chy + y, cD027r, cD027g, cD027b, 255);
				imageData->SetPixelResultRGBA(chx    , chy + y, cD027r, cD027g, cD027b, 255);
			}
			
			chx += 8;
			chd++;
		}
	}
}



CSlrFontProportional *ProcessFonts(u8 *charsetData, bool useScreenCodes)
{
	LOGD("--- process fonts ---");
	
	CSlrFontProportional *font = new CSlrFontProportional();
	
	int gap = 4;
	
	CImageData *imageData = new CImageData(256, 256, IMG_TYPE_RGBA);
	imageData->AllocImage(false, true);
	
	int chx = 0 + gap;
	int chy = 0 + gap;
	
	int c = 0;
	for (int charId = 0; charId < 256; charId++)
	{
		u8 *chd = charsetData + 8*charId;
		
		// copy pixels around character for better linear scaling 
		for (int y = -1; y < 9; y++)
		{
			if ((*chd & 0x01) == 0x01)
			{
				imageData->SetPixelResultRGBA(chx + 8, chy + y, 255, 255, 255, 255);
				imageData->SetPixelResultRGBA(chx + 7, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x02) == 0x02)
			{
				imageData->SetPixelResultRGBA(chx + 6, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x04) == 0x04)
			{
				imageData->SetPixelResultRGBA(chx + 5, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x08) == 0x08)
			{
				imageData->SetPixelResultRGBA(chx + 4, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x10) == 0x10)
			{
				imageData->SetPixelResultRGBA(chx + 3, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x20) == 0x20)
			{
				imageData->SetPixelResultRGBA(chx + 2, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x40) == 0x40)
			{
				imageData->SetPixelResultRGBA(chx + 1, chy + y, 255, 255, 255, 255);
			}
			if ((*chd & 0x80) == 0x80)
			{
				imageData->SetPixelResultRGBA(chx + 0, chy + y, 255, 255, 255, 255);
				imageData->SetPixelResultRGBA(chx - 1, chy + y, 255, 255, 255, 255);
			}
			
			if (y == -1 || y == 7)
				continue;
			
			chd++;
		}
		
		chx += 8 + gap;
		c++;
		if (c == 16)
		{
			c = 0;
			chx = 0 + gap;
			chy += 8 + gap;
		}
	}
	
	// don't ask me why - there's a bug in macos engine part waiting to be fixed
	// this has been fixed in imgui branch
//#if !defined(WIN32) && !defined(LINUX)
//	imageData->FlipVertically();
//#endif
	
	font->texturePageImageData = imageData;
	
	font->texturePage = new CSlrImage(true, false);
	font->texturePage->LoadImage(imageData, RESOURCE_PRIORITY_STATIC, false);
	font->texturePage->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	font->texturePage->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(font->texturePage, NULL);

	//font->texturePage = RES_GetImage("/c64/c64chars-set1", false, true);
	
	font->lineHeight = 8;
	font->fontHeight = 8;
	font->base = 0;
	font->width = font->texturePage->width;
	font->height = font->texturePage->height;
	font->pages = 1;
	font->outline = 0;
	font->scaleAdjust = 0.5f;
	
	// no kerning
	
	// get chars
	if (useScreenCodes)
	{
		AddCBMScreenCharacters(font);
	}
	else
	{
		AddASCIICharacters(font);
	}
	
	font->texDividerX = 1.0f;
	font->texDividerY = 1.0f;
	font->texAdvX = (float) font->texDividerX/font->width;
	font->texAdvY = (float) font->texDividerY/font->height;
	
//	Cu8Buffer *u8Buffer = new Cu8Buffer();
//	font->StoreFontDataTou8Buffer(u8Buffer);
//	u8Buffer->storeToDocuments("c64chars-set1.fnt");
	
	return font;
}

void AddCBMScreenCharacters(CSlrFontProportional *font)
{
	int gap = 4;
	int gap2 = gap/2;
	

	int val = 0;
	for (int fy = 0; fy < 16; fy++)
	{
		for (int fx = 0; fx < 16; fx++)
		{
			CharDescriptor *charDescriptor = new CharDescriptor();
			charDescriptor->x = gap + fx * (8+gap);
			charDescriptor->y = gap + fy * (8+gap);
			charDescriptor->width = 8;
			charDescriptor->height = 8;
			charDescriptor->xOffset = 0;
			charDescriptor->yOffset = 0;
			charDescriptor->xAdvance = 8;
			charDescriptor->page = 1;
			
			//LOGD("adding charVal=%d", val);
			font->chars[val] = charDescriptor;
			
			val++;
		}
	}
}

void AddASCIICharacter(CSlrFontProportional *font, int fx, int fy, u16 chr)
{
	const int gap = 4;

	// normal
	CharDescriptor *charDescriptor = new CharDescriptor();
	charDescriptor->x = gap + fx * (8+gap);
	charDescriptor->y = gap + fy * (8+gap);
	charDescriptor->width = 8;
	charDescriptor->height = 8;
	charDescriptor->xOffset = 0;
	charDescriptor->yOffset = 0;
	charDescriptor->xAdvance = 8;
	charDescriptor->page = 1;

	font->chars[chr] = charDescriptor;

	// inverted char+0x0080 CBMSHIFTEDFONT_INVERT
	charDescriptor = new CharDescriptor();
	charDescriptor->x = gap + fx * (8+gap);
	charDescriptor->y = gap + (fy+8) * (8+gap);
	charDescriptor->width = 8;
	charDescriptor->height = 8;
	charDescriptor->xOffset = 0;
	charDescriptor->yOffset = 0;
	charDescriptor->xAdvance = 8;
	charDescriptor->page = 1;

	font->chars[chr + CBMSHIFTEDFONT_INVERT] = charDescriptor;
}

void AddASCIICharacters(CSlrFontProportional *font)
{
	
	int val = 0;
	for (int fy = 0; fy < 8; fy++)
	{
		for (int fx = 0; fx < 16; fx++)
		{
			//LOGD("adding charVal=%2.2x", val);
			if (val == 0x00)
			{
				AddASCIICharacter(font, fx, fy, '@');
			}
			else if (val >= 0x01 && val <= 0x1A)
			{
				AddASCIICharacter(font, fx, fy, val + 0x60);
			}
			else if (val >= 0x30 && val <= 0x39)
			{
				AddASCIICharacter(font, fx, fy, val);
			}
			else if (val >= 0x41 && val <= 0x5A)
			{
				AddASCIICharacter(font, fx, fy, val);
			}
			else if (val == 0x1B)
			{
				AddASCIICharacter(font, fx, fy, '[');
			}
			else if (val == 0x1C)
			{
				AddASCIICharacter(font, fx, fy, 0x1C);
			}
			else if (val == 0x1D)
			{
				AddASCIICharacter(font, fx, fy, ']');
			}
			else if (val == 0x1E)
			{
				AddASCIICharacter(font, fx, fy, '^');
			}
			else if (val == 0x1F)
			{
				AddASCIICharacter(font, fx, fy, '~');
			}
			else if (val == 0x20)
			{
				AddASCIICharacter(font, fx, fy, ' ');
			}
			else if (val == 0x21)
			{
				AddASCIICharacter(font, fx, fy, '!');
			}
			else if (val == 0x22)
			{
				AddASCIICharacter(font, fx, fy, '\"');
			}
			else if (val == 0x23)
			{
				AddASCIICharacter(font, fx, fy, '#');
			}
			else if (val == 0x24)
			{
				AddASCIICharacter(font, fx, fy, '$');
			}
			else if (val == 0x25)
			{
				AddASCIICharacter(font, fx, fy, '%');
			}
			else if (val == 0x26)
			{
				AddASCIICharacter(font, fx, fy, '&');
			}
			else if (val == 0x27)
			{
				AddASCIICharacter(font, fx, fy, '\'');
			}
			else if (val == 0x28)
			{
				AddASCIICharacter(font, fx, fy, '(');
			}
			else if (val == 0x29)
			{
				AddASCIICharacter(font, fx, fy, ')');
			}
			else if (val == 0x2A)
			{
				AddASCIICharacter(font, fx, fy, '*');
			}
			else if (val == 0x2B)
			{
				AddASCIICharacter(font, fx, fy, '+');
			}
			else if (val == 0x2C)
			{
				AddASCIICharacter(font, fx, fy, ',');
			}
			else if (val == 0x2D)
			{
				AddASCIICharacter(font, fx, fy, '-');
			}
			else if (val == 0x2E)
			{
				AddASCIICharacter(font, fx, fy, '.');
			}
			else if (val == 0x2F)
			{
				AddASCIICharacter(font, fx, fy, '/');
			}
			else if (val == 0x3A)
			{
				AddASCIICharacter(font, fx, fy, ':');
			}
			else if (val == 0x3B)
			{
				AddASCIICharacter(font, fx, fy, ';');
			}
			else if (val == 0x3C)
			{
				AddASCIICharacter(font, fx, fy, '<');
			}
			else if (val == 0x3B)
			{
				AddASCIICharacter(font, fx, fy, ';');
			}
			else if (val == 0x3C)
			{
				AddASCIICharacter(font, fx, fy, '<');
			}
			else if (val == 0x3D)
			{
				AddASCIICharacter(font, fx, fy, '=');
			}
			else if (val == 0x3E)
			{
				AddASCIICharacter(font, fx, fy, '>');
			}
			else if (val == 0x3F)
			{
				AddASCIICharacter(font, fx, fy, '?');
			}
			else if (val == 0x64)
			{
				AddASCIICharacter(font, fx, fy, '_');
			}
			
			val++;
		}
	}
}

void InvertCBMText(CSlrString *text)
{
	for (int i = 0; i < text->GetLength(); i++)
	{
		u16 c = text->GetChar(i);
		c += CBMSHIFTEDFONT_INVERT;
		text->SetChar(i, c);
	}
}

void ClearInvertCBMText(CSlrString *text)
{
	for (int i = 0; i < text->GetLength(); i++)
	{
		u16 c = text->GetChar(i);
		c = c & 0x7F;
		text->SetChar(i, c);
	}
	
}

void InvertCBMText(char *text)
{
	int len = strlen(text);
	for (int i = 0; i < len; i++)
	{
		u8 c = text[i];
		c += CBMSHIFTEDFONT_INVERT;
		text[i] = c;
	}
}

void ClearInvertCBMText(char *text)
{
	int len = strlen(text);
	for (int i = 0; i < len; i++)
	{
		u8 c = text[i];
		c = c & 0x7F;
		text[i] = c;
	}
}

///

void CopyHiresCharsetToImage(u8 *charsetData, CImageData *imageData, int numColumns,
							  u8 colorBackground, u8 colorForeground, CDebugInterfaceC64 *debugInterface)
{
//	int numRows = 256 / numColumns;
	int cols = numColumns * 8;

	u8 br, bg, bb;
	u8 fr, fg, fb;
	
	debugInterface->GetCBMColor(colorBackground, &br, &bg, &bb);
	debugInterface->GetCBMColor(colorForeground, &fr, &fg, &fb);

	imageData->EraseContent(br, bg, bb, 255);

	int chx = 0;
	int chy = 0;

	for (int charId = 0; charId < 256; charId++)
	{
		u8 *chd = charsetData + 8*charId;
		
		for (int y = 0; y < 8; y++)
		{
			if ((*chd & 0x01) == 0x01)
			{
				imageData->SetPixelResultRGBA(chx + 7, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x02) == 0x02)
			{
				imageData->SetPixelResultRGBA(chx + 6, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x04) == 0x04)
			{
				imageData->SetPixelResultRGBA(chx + 5, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x08) == 0x08)
			{
				imageData->SetPixelResultRGBA(chx + 4, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x10) == 0x10)
			{
				imageData->SetPixelResultRGBA(chx + 3, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x20) == 0x20)
			{
				imageData->SetPixelResultRGBA(chx + 2, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x40) == 0x40)
			{
				imageData->SetPixelResultRGBA(chx + 1, chy + y, fr, fg, fb, 255);
			}
			if ((*chd & 0x80) == 0x80)
			{
				imageData->SetPixelResultRGBA(chx + 0, chy + y, fr, fg, fb, 255);
			}
			
			chd++;
		}
		
		chx += 8;
		
		if (chx == cols)
		{
			chx = 0;
			chy += 8;
		}
	}
}

void CopyMultiCharsetToImage(u8 *charsetData, CImageData *imageData, int numColumns,
							 u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800, CDebugInterfaceC64 *debugInterface)
{
	//	int numRows = 256 / numColumns;
	int cols = numColumns * 8;
	
	u8 cD021r, cD021g, cD021b;
	u8 cD022r, cD022g, cD022b;
	u8 cD023r, cD023g, cD023b;
	u8 cD800r, cD800g, cD800b;
	
	debugInterface->GetCBMColor(colorD021, &cD021r, &cD021g, &cD021b);
	debugInterface->GetCBMColor(colorD022, &cD022r, &cD022g, &cD022b);
	debugInterface->GetCBMColor(colorD023, &cD023r, &cD023g, &cD023b);
	debugInterface->GetCBMColor(colorD800, &cD800r, &cD800g, &cD800b);
	
	imageData->EraseContent(cD021r, cD021g, cD021b, 255);
	
	int chx = 0;
	int chy = 0;
	
	for (int charId = 0; charId < 256; charId++)
	{
		u8 *chd = charsetData + 8*charId;
		u8 v;
		
		for (int y = 0; y < 8; y++)
		{
			v = (*chd & 0x03);
			if (v == 0x01)
			{
				imageData->SetPixelResultRGBA(chx + 7, chy + y, cD022r, cD022g, cD022b, 255);
				imageData->SetPixelResultRGBA(chx + 6, chy + y, cD022r, cD022g, cD022b, 255);
			}
			else if (v == 0x02)
			{
				imageData->SetPixelResultRGBA(chx + 7, chy + y, cD023r, cD023g, cD023b, 255);
				imageData->SetPixelResultRGBA(chx + 6, chy + y, cD023r, cD023g, cD023b, 255);
			}
			else if (v == 0x03)
			{
				imageData->SetPixelResultRGBA(chx + 7, chy + y, cD800r, cD800g, cD800b, 255);
				imageData->SetPixelResultRGBA(chx + 6, chy + y, cD800r, cD800g, cD800b, 255);
			}
			
			// 00001100
			v = (*chd & 0x0C);
			if (v == 0x04)
			{
				imageData->SetPixelResultRGBA(chx + 5, chy + y, cD022r, cD022g, cD022b, 255);
				imageData->SetPixelResultRGBA(chx + 4, chy + y, cD022r, cD022g, cD022b, 255);
			}
			else if (v == 0x08)
			{
				imageData->SetPixelResultRGBA(chx + 5, chy + y, cD023r, cD023g, cD023b, 255);
				imageData->SetPixelResultRGBA(chx + 4, chy + y, cD023r, cD023g, cD023b, 255);
			}
			else if (v == 0x0C)
			{
				imageData->SetPixelResultRGBA(chx + 5, chy + y, cD800r, cD800g, cD800b, 255);
				imageData->SetPixelResultRGBA(chx + 4, chy + y, cD800r, cD800g, cD800b, 255);
			}
			
			// 00110000
			v = (*chd & 0x30);
			if (v == 0x10)
			{
				imageData->SetPixelResultRGBA(chx + 3, chy + y, cD022r, cD022g, cD022b, 255);
				imageData->SetPixelResultRGBA(chx + 2, chy + y, cD022r, cD022g, cD022b, 255);
			}
			else if (v == 0x20)
			{
				imageData->SetPixelResultRGBA(chx + 3, chy + y, cD023r, cD023g, cD023b, 255);
				imageData->SetPixelResultRGBA(chx + 2, chy + y, cD023r, cD023g, cD023b, 255);
			}
			else if (v == 0x30)
			{
				imageData->SetPixelResultRGBA(chx + 3, chy + y, cD800r, cD800g, cD800b, 255);
				imageData->SetPixelResultRGBA(chx + 2, chy + y, cD800r, cD800g, cD800b, 255);
			}
			
			// 11000000
			v = (*chd & 0xC0);
			if (v == 0x40)
			{
				imageData->SetPixelResultRGBA(chx + 1, chy + y, cD022r, cD022g, cD022b, 255);
				imageData->SetPixelResultRGBA(chx    , chy + y, cD022r, cD022g, cD022b, 255);
			}
			else if (v == 0x80)
			{
				imageData->SetPixelResultRGBA(chx + 1, chy + y, cD023r, cD023g, cD023b, 255);
				imageData->SetPixelResultRGBA(chx    , chy + y, cD023r, cD023g, cD023b, 255);
			}
			else if (v == 0xC0)
			{
				imageData->SetPixelResultRGBA(chx + 1, chy + y, cD800r, cD800g, cD800b, 255);
				imageData->SetPixelResultRGBA(chx    , chy + y, cD800r, cD800g, cD800b, 255);
			}

			chd++;
		}
		
		chx += 8;
		
		if (chx == cols)
		{
			chx = 0;
			chy += 8;
		}
	}
}

u8 FindC64Color(u8 r, u8 g, u8 b, CDebugInterfaceC64 *debugInterface)
{
	//LOGD("FindC64Color: %d %d %d", r, g, b);
	float minDistance = FLT_MAX;
	int color = -1;
	for (int i = 0; i < 16; i++)
	{
		u8 pr, pg, pb;
		debugInterface->GetCBMColor(i, &pr, &pg, &pb);
		
		// calc distance
		float tr = ((float)pr - (float)r);
		float tg = ((float)pg - (float)g);
		float tb = ((float)pb - (float)b);

		float d = sqrt( tr*tr + tg*tg + tb*tb );
		
		if (d < minDistance)
		{
			minDistance = d;
			color = i;
		}
	}
	
	//LOGD("          color=%d", color);
	
	return color;
}

float GetC64ColorDistance(u8 color1, u8 color2, CDebugInterfaceC64 *debugInterface)
{
	u8 c1r, c1g, c1b;
	debugInterface->GetCBMColor(color1, &c1r, &c1g, &c1b);

	u8 c2r, c2g, c2b;
	debugInterface->GetCBMColor(color2, &c2r, &c2g, &c2b);
	
	// calc distance
	float tr = ((float)c1r - (float)c2r);
	float tg = ((float)c1g - (float)c2g);
	float tb = ((float)c1b - (float)c2b);
	
	float d = sqrt( tr*tr + tg*tg + tb*tb );
	
	return d;
}

//
void RenderColorRectangle(float px, float py, float ledSizeX, float ledSizeY, float gap, bool isLocked, u8 color,
						  CDebugInterfaceC64 *debugInterface)
{
	float colorR, colorG, colorB;
	debugInterface->GetFloatCBMColor(color, &colorR, &colorG, &colorB);
	
	BlitFilledRectangle(px, py, -1, ledSizeX, ledSizeY,
						colorR, colorG, colorB, 1.0f);
	
	
	if (!isLocked)
	{
		colorR = colorG = colorB = 0.3f;
	}
	else
	{
		colorR = 1.0f;
		colorG = colorB = 0.3f;
	}
	
	BlitRectangle(px, py, -1, ledSizeX, ledSizeY,
				  colorR, colorG, colorB, 1.0f, gap);
	
}

void RenderColorRectangleWithHexCode(float px, float py, float ledSizeX, float ledSizeY, float gap, bool isLocked, u8 color, float fontSize, CDebugInterfaceC64 *debugInterface)
{
	float colorR, colorG, colorB;
	debugInterface->GetFloatCBMColor(color, &colorR, &colorG, &colorB);
	
	BlitFilledRectangle(px, py, -1, ledSizeX, ledSizeY,
						colorR, colorG, colorB, 1.0f);
	
	if (!isLocked)
	{
		colorR = colorG = colorB = 0.3f;
	}
	else
	{
		colorR = 1.0f;
		colorG = colorB = 0.3f;
	}
	
	BlitRectangle(px, py - gap, -1, ledSizeX, ledSizeY + gap,
				  colorR, colorG, colorB, 1.0f, gap);
	
	// blit hex color code
	char buf[2];
	Byte2Hex1digitR(color, buf);
	buf[1] = 0x00;
	viewC64->fontDisassembly->BlitText(buf, px + ledSizeX/2.0f - fontSize/2.0f, py+0.5f, -1, fontSize);
}

uint16 GetSidAddressByChipNum(int chipNum)
{
	if (chipNum == 1)
	{
		return c64SettingsSIDStereoAddress;
	}

	if (chipNum == 2)
	{
		return c64SettingsSIDTripleAddress;
	}

	return 0xD400;
}

// *.SID file conversion
CByteBuffer *ConvertSIDtoPRG(CByteBuffer *sidFileData)
{
	Psid64 psid64;
	
	psid64.load(sidFileData->data, sidFileData->length);
	if (!psid64.convert())
	{
		return NULL;
	}
	
	psid64.setCompress(false);
	
	unsigned int length;
	u8 *data = psid64.getProgramDataBuffer(&length);
	
	CByteBuffer *byteBuffer = new CByteBuffer(length);
	byteBuffer->putBytes(data, length);
	byteBuffer->Rewind();
	
	return byteBuffer;
}

bool C64LoadSIDToRam(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr)
{
	SidTuneMod sidTune(0);
	if (!sidTune.load(filePath))
	{
		LOGError("C64LoadSIDToRam: load failed %s", sidTune.getInfo().statusString);
		return false;
	}
	
	u8 *sidBuf = new u8[0x10000];
	sidTune.placeSidTuneInC64mem(sidBuf);
	
	*fromAddr = sidTune.getInfo().loadAddr;
	*toAddr = sidTune.getInfo().loadAddr + sidTune.getInfo().c64dataLen;
	
	for (int i = *fromAddr; i < *toAddr; i++)
	{
		viewC64->debugInterfaceC64->SetByteToRamC64(i, sidBuf[i]);
	}
	
	*initAddr = sidTune.getInfo().initAddr;
	*playAddr = sidTune.getInfo().playAddr;
	
	delete [] sidBuf;
	
	return true;
}

bool C64LoadSIDToBuffer(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr, u8 **buffer)
{
	SidTuneMod sidTune(0);
	if (!sidTune.load(filePath))
	{
		LOGError("C64LoadSIDToRam: load failed %s", sidTune.getInfo().statusString);
		return false;
	}
	
	u8 *sidBuf = new u8[0x10000];
	sidTune.placeSidTuneInC64mem(sidBuf);
	
	*fromAddr = sidTune.getInfo().loadAddr;
	*toAddr = sidTune.getInfo().loadAddr + sidTune.getInfo().c64dataLen;
	
	int len = *toAddr - *fromAddr;
	*buffer = new u8[len];

	int j = 0;
	for (int i = *fromAddr; i < *toAddr; i++)
	{
		(*buffer)[j++] = sidBuf[i];
	}
	
	*initAddr = sidTune.getInfo().initAddr;
	*playAddr = sidTune.getInfo().playAddr;
	
	delete [] sidBuf;
	
	return true;
}

// Exomizer
bool C64SaveMemoryExomizerPRG(int fromAddr, int toAddr, int jmpAddr, const char *filePath)
{
	FILE *fp = fopen(filePath, "wb");
	if (!fp)
	{
		return false;
	}

	int dataSize = toAddr - fromAddr;
	u8 *data = new u8[dataSize];
	int i = 0;
	for (int d = fromAddr; d < toAddr; d++)
	{
		data[i++] = viewC64->debugInterfaceC64->GetByteFromRamC64(d);
	}
	
	u8 *compressedData = new u8[0x10000];
	int prgSize = exomizer(data, dataSize,
						   fromAddr, jmpAddr, compressedData);
	
	int lineNumber = 666;
	// set BASIC line number
	compressedData[4] = (u8) (lineNumber & 0xff);
	compressedData[5] = (u8) (lineNumber >> 8);
	
	fwrite(compressedData, prgSize, 1, fp);
	fclose(fp);
	
	delete [] compressedData;
	delete [] data;
	
	LOGD("C64SaveMemoryExomizerPRG: stored to %s", filePath);
	
	return true;
}

u8 *C64ExomizeMemoryRaw(int fromAddr, int toAddr, int *compressedSize)
{
	int dataSize = toAddr - fromAddr;
	u8 *data = new u8[dataSize];
	int i = 0;
	for (int d = fromAddr; d < toAddr; d++)
	{
		data[i++] = viewC64->debugInterfaceC64->GetByteFromRamC64(d);
	}
	
	u8 *compressedData = new u8[0x10000];
	*compressedSize = exomizer_raw_backwards(data, dataSize, compressedData);
	
	delete [] data;
	
	LOGD("C64ExomizeMemoryRaw: dataSize=%d compressedSize=%d", dataSize, compressedSize);
	
	return compressedData;
}

bool C64SaveMemory(int fromAddr, int toAddr, bool isPRG, CDataAdapter *dataAdapter, const char *filePath)
{
	FILE *fp;
	fp = fopen(filePath, "wb");
	
	if (!fp)
	{
		return false;
	}

	int len = toAddr - fromAddr;
	uint8 *memoryBuffer = new uint8[0x10000];
	dataAdapter->AdapterReadBlockDirect(memoryBuffer, fromAddr, toAddr);
	
	uint8 *writeBuffer = new uint8[len];
	memcpy(writeBuffer, memoryBuffer + fromAddr, len);
	
	if (isPRG)
	{
		// write header
		uint8 buf[2];
		buf[0] = fromAddr & 0x00FF;
		buf[1] = (fromAddr >> 8) & 0x00FF;
		
		fwrite(buf, 1, 2, fp);
	}
	
	
	int lenWritten = fwrite(writeBuffer, 1, len, fp);
	fclose(fp);
	
	delete [] writeBuffer;
	delete [] memoryBuffer;
	
	if (lenWritten != len)
	{
		return false;
	}

	return true;
}

int C64LoadMemory(int fromAddr, CDataAdapter *dataAdapter, const char *filePath)
{
	FILE *fp;
	fp = fopen(filePath, "rb");
	
	if (!fp)
	{
		return -1;
	}
	
	u16 addr = fromAddr;
	int len = 0;
	while(true)
	{
		int c = getc(fp);
		
		if (c == EOF)
			break;
		
		dataAdapter->AdapterWriteByte(addr, c);
		addr++;
		len++;
	}

	fclose(fp);
	return len;
}

int ConvertSdlAxisToJoystickAxis(int sdlAxis)
{
	switch(sdlAxis)
	{
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			return JOYPAD_N;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			return JOYPAD_S;
			// TODO: bug? why this is reversed here?
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			return JOYPAD_E;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			return JOYPAD_W;
		case SDL_CONTROLLER_BUTTON_A:
			return JOYPAD_FIRE;
		case SDL_CONTROLLER_BUTTON_B:
			return JOYPAD_FIRE_B;
		case SDL_CONTROLLER_BUTTON_START:
			return JOYPAD_START;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			return JOYPAD_SELECT;
	}
	return JOYPAD_IDLE;
}

void GetC64VicAddrFromState(vicii_cycle_state_t *viciiState, int *screenAddr, int *charsetAddr, int *bitmapBank)
{
	u16 screen_addr;            // Screen start address.
	int charset_addr, bitmap_bank;
	
	if (viciiState == NULL)
	{
		LOGError("viciiState can't be NULL!");
		return;
	}
	
	screen_addr = viciiState->vbank_phi2 + ((viciiState->regs[0x18] & 0xf0) << 6);
	screen_addr = (screen_addr & viciiState->vaddr_mask_phi2) | viciiState->vaddr_offset_phi2;
	
	charset_addr = (viciiState->regs[0x18] & 0xe) << 10;
	charset_addr = (charset_addr + viciiState->vbank_phi1);
	charset_addr &= viciiState->vaddr_mask_phi1;
	charset_addr |= viciiState->vaddr_offset_phi1;
	
	bitmap_bank = charset_addr & 0xe000;
	
	*screenAddr = screen_addr;
	*charsetAddr = charset_addr;
	*bitmapBank = bitmap_bank;
	

}
