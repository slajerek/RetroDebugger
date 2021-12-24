#ifndef _C64CharHires_H_
#define _C64CharHires_H_

#include "C64Bitmap.h"

class CViewC64VicDisplay;

class C64CharHires : public C64Bitmap
{
public:
	C64CharHires();
	C64CharHires(CViewC64VicDisplay *vicDisplay, int x, int y);
	virtual void SetPixel(int x, int y, u8 color);
	virtual u8 GetPixel(int x, int y);
	virtual void DebugPrint();
};

#endif
