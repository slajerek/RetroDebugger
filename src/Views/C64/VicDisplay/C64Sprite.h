#ifndef _C64Sprite_H_
#define _C64Sprite_H_

#include "C64Bitmap.h"
#include "CViewVicEditor.h"

class CSlrImage;
class CImageData;
class CByteBuffer;

class C64Sprite : public C64Bitmap
{
public:
	C64Sprite(CViewVicEditor *vicEditor, int width, int height, bool isMulti);
	virtual ~C64Sprite();
	
	CViewVicEditor *vicEditor;
	
	int spriteId;
	
	u8 spriteColor;
	
	CSlrImage *image;
	CImageData *imageData;
	int posX, posY;
	
	bool isStretchedHorizontally;
	bool isStretchedVertically;
	
	int pointerValue;
	int pointerAddr;
	
	virtual bool IsEqual(C64Sprite *otherSprite);
	
	//
	int rasterLine;
	int rasterCycle;

	//
	int addrPosXHighBits;
	int addrPosX;
	int addrPosY;
	int addrSetStretchHorizontal;
	int addrSetStretchVertical;
	int addrSetMultiColor;
	int addrColorChangeCommon1;
	int addrColorChangeCommon2;
	int addrColorChange;
	
	//
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);

	virtual u8 GetColorAtPixel(int x, int y);

	virtual void Clear();
	
	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer);
	
	u8 spriteData[63];
};

#endif
