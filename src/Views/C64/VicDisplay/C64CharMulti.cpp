#include "C64CharMulti.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"

// copy from multi bitmap
C64CharMulti::C64CharMulti(CViewC64VicDisplay *vicDisplay, int x, int y)
: C64Bitmap(4, 8, true)
{
	pixels = new u8[4*8];
	
	colors = new u8[4];
	histogram = new u8[4];
	
	memset(colors, 0x00, 4);
	memset(histogram, 0x00, 4);
	
	int charColumn = floor((float)((float)x / 8.0f));
	int charRow = floor((float)((float)y / 8.0f));
	
	int offset = charColumn + charRow * 40;
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	vicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow), &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	///
	
	this->frameColor = d020colors[0];
	this->colors[0] = d020colors[1];
	this->colors[1] = (screen_ptr[(charRow * 40) + charColumn] & 0xf0) >> 4;
	this->colors[2] = screen_ptr[(charRow * 40) + charColumn] & 0xf;
	this->colors[3] = color_ram_ptr[(charRow * 40) + charColumn] & 0xf;
	
	
	//	this->colors[1] = viewC64->debugInterface->GetByteC64(vicDisplay->screenAddress + offset) >> 4;
	//	this->colors[2] = viewC64->debugInterface->GetByteC64(vicDisplay->screenAddress + offset) & 0xf;
	
	
	
	// copy pixels: MULTI
	for (int pixelCharY = 0; pixelCharY < 8; pixelCharY++)
	{
		int vicAddr = charColumn*8 + charRow * 40*8 + pixelCharY;
		
		
		u8 val;
		
		if (vicAddr < 4096)
		{
			
			val = bitmap_low_ptr[vicAddr];
		}
		else
		{
			val = bitmap_high_ptr[vicAddr - 4096];
		}
		
		//u8 val = viewC64->debugInterface->GetByteC64(bitmapBase + offset);
		
		for (int pixelNum = 0; pixelNum < 4; pixelNum++)
		{
			
			//LOGD("addr=%04x pixelNum=%d val=%02x", bitmapBase + offset, pixelNum, val);
			u8 a = 0x03 << ((3-pixelNum)*2);
			//LOGD(" a=%02x", a);
			u8 va = val & a;
			//LOGD("va=%02x", va);
			u8 vb = va >> ((3-pixelNum)*2);
			//LOGD("                                                   =%02x", vb);
			
			
			pixels[pixelNum + 4 * pixelCharY] = vb;
			
			histogram[vb]++;
			
		}
		
	}
	
	
//	this->DebugPrint();
	
}

u8 C64CharMulti::GetPixel(int x, int y)
{
	return pixels[x + 4 * y];
}

void C64CharMulti::DebugPrint()
{
	// debug print char
	for (int pixelCharY = 0; pixelCharY < 8; pixelCharY++)
	{
		char buf[256];
		sprintf(buf, " %d: ", pixelCharY);
		
		for (int pixelNum = 0; pixelNum < 4; pixelNum++)
		{
			char buf2[256];
			sprintf(buf2, " %d", GetPixel(pixelNum, pixelCharY));
			strcat(buf, buf2);
		}
		
		LOGD(buf);
	}
	
	LOGD(" colors: d020=%02X | d021=%02X (%3d) screen1=%02X (%3d) screen2=%02X (%3d) colorRam=%02X (%3d)",
		 this->frameColor,
		 this->colors[0], this->histogram[0],
		 this->colors[1], this->histogram[1],
		 this->colors[2], this->histogram[2],
		 this->colors[3], this->histogram[3]);
	
}
