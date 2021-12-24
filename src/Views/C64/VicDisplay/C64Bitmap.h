#ifndef _GENERICC64CHAR_H_
#define _GENERICC64CHAR_H_

#include "SYS_Defs.h"

class CViewC64VicDisplay;

class C64Bitmap
{
public:
	C64Bitmap(int sizeX, int sizeY, bool isMulti);
	bool isMulti;
	
	int sizeX;
	int sizeY;
	
	u8 *pixels;
	
	u8 frameColor;
	u8 *colors;
	u8 *histogram;
	
	virtual void Clear();
	virtual void SetPixel(int x, int y, u8 color);
	virtual u8 GetPixel(int x, int y);
	virtual void DebugPrint();
};

#endif
