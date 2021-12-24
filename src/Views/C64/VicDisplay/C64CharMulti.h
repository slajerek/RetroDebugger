#ifndef _C64CharMulti_H_
#define _C64CharMulti_H_

#include "C64Bitmap.h"

class CViewC64VicDisplay;

class C64CharMulti : public C64Bitmap
{
public:
	
	C64CharMulti(CViewC64VicDisplay *vicDisplay, int x, int y);
	virtual u8 GetPixel(int x, int y);
	virtual void DebugPrint();
	
};

#endif
