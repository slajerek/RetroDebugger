#include "C64CharHires.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"

// create empty hires char
C64CharHires::C64CharHires()
: C64Bitmap(8, 8, false)
{
	pixels = new u8[8*8];
	
	colors = new u8[2];
	histogram = new u8[2];
	
	memset(pixels, 0x00, 8*8);
	memset(colors, 0x00, 2);
	memset(histogram, 0x00, 2);
	
}

// copy from hires bitmap
C64CharHires::C64CharHires(CViewC64VicDisplay *vicDisplay, int x, int y)
: C64Bitmap(8, 8, false)
{
	pixels = new u8[8*8];
	
	colors = new u8[2];
	histogram = new u8[2];

	memset(colors, 0x00, 2);
	memset(histogram, 0x00, 2);
	
	
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
	this->colors[0] = screen_ptr[(charRow * 40) + charColumn] & 0xf;
	this->colors[1] = (screen_ptr[(charRow * 40) + charColumn] & 0xf0) >> 4;
	
	
	// copy pixels: HIRES
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
		
		for (int pixelNum = 0; pixelNum < 8; pixelNum++)
		{
			
			//LOGD("addr=%04x pixelNum=%d val=%02x", bitmapBase + offset, pixelNum, val);
			u8 a = 0x01 << ((7-pixelNum));
			//LOGD(" a=%02x", a);
			u8 va = val & a;
			//LOGD("va=%02x", va);
			u8 vb = va >> ((7-pixelNum));
			//LOGD("                                                   =%02x", vb);
			
			
			pixels[pixelNum + 8 * pixelCharY] = vb;
			
			histogram[vb]++;
			
		}
		
	}
	
	
	//this->DebugPrint();
	
}

void C64CharHires::SetPixel(int x, int y, u8 color)
{
	pixels[x + sizeX * y] = color;
}

u8 C64CharHires::GetPixel(int x, int y)
{
	return pixels[x + sizeX * y];
}

void C64CharHires::DebugPrint()
{
	// debug print char
	for (int pixelCharY = 0; pixelCharY < 8; pixelCharY++)
	{
		char buf[256];
		sprintf(buf, " %d: ", pixelCharY);
		
		for (int pixelNum = 0; pixelNum < 8; pixelNum++)
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
	LOGD("========== C64CharHires");
}
