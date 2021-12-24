#include "C64Bitmap.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"

C64Bitmap::C64Bitmap(int sizeX, int sizeY, bool isMulti)
{
	this->isMulti = isMulti;
	
	this->pixels = NULL;
	this->colors = NULL;
	this->histogram = NULL;
	
	this->frameColor = 0;

	this->sizeX = sizeX;
	this->sizeY = sizeY;
}

void C64Bitmap::Clear()
{
	memset(this->pixels, 0x00, this->sizeX*this->sizeY);
}

void C64Bitmap::SetPixel(int x, int y, u8 color)
{
	SYS_FatalExit("C64Bitmap::SetPixel");
}

u8 C64Bitmap::GetPixel(int x, int y)
{
	SYS_FatalExit("C64Bitmap::GetPixel");
	return 0;
}

void C64Bitmap::DebugPrint()
{
	SYS_FatalExit("C64Bitmap::DebugPrint");
}
