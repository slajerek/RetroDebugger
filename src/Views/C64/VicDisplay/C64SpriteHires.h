#ifndef _C64SpriteHires_H_
#define _C64SpriteHires_H_

#include "C64Sprite.h"

class CViewVicEditor;

class C64SpriteHires : public C64Sprite
{
public:
	C64SpriteHires();
	C64SpriteHires(CViewVicEditor *vicEditor, int x, int y, bool isStretchedHorizontally, bool isStretchedVertically, int pointerValue, int pointerAddr);
	C64SpriteHires(CViewVicEditor *vicEditor, CByteBuffer *byteBuffer);

	virtual ~C64SpriteHires();

	virtual void SetPixel(int x, int y, u8 color);
	virtual u8 GetPixel(int x, int y);
	virtual void DebugPrint();
	
	virtual u8 PutPixelHiresSprite(bool forceColorReplace, int x, int y, u8 paintColor, u8 replacementColorNum);
	
	virtual u8 PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	virtual u8 PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource);
	
	virtual u8 GetColorAtPixel(int x, int y);
	
	//
	virtual void FetchSpriteData(int addr);
	virtual void StoreSpriteData(int addr);
	
	//
	virtual void Clear();

	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer);

};

#endif
