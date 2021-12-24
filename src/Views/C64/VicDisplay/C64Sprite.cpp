#include "C64Sprite.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"
#include "C64VicDisplayCanvas.h"

C64Sprite::C64Sprite(CViewVicEditor *vicEditor, int width, int height, bool isMulti)
: C64Bitmap(width, height, isMulti)
{
	this->vicEditor = vicEditor;
	
	image = NULL;
	imageData = NULL;
	
	spriteColor = 0;
	posX = -1;
	posY = -1;
	spriteId = -1;
	
	pointerAddr = -1;
	
	isStretchedHorizontally = false;
	isStretchedVertically = false;
}

C64Sprite::~C64Sprite()
{
	
}


bool C64Sprite::IsEqual(C64Sprite *otherSprite)
{
	if (otherSprite->isMulti != this->isMulti)
		return false;

	if (otherSprite->posX != this->posX)
		return false;

	if (otherSprite->posY != this->posY)
		return false;

	if (otherSprite->spriteId != this->spriteId)
		return false;
	
	return true;
}

u8 C64Sprite::PutColorAtPixel(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	LOGError("C64Sprite::PutColorAtPixel");
	return PAINT_RESULT_ERROR;
}

u8 C64Sprite::PaintDither(bool forceColorReplace, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
	LOGError("C64Sprite::PaintDither");
	return PAINT_RESULT_ERROR;
}

u8 C64Sprite::GetColorAtPixel(int x, int y)
{
	LOGError("C64Sprite::GetColorAtPixel");
	return false;
}

void C64Sprite::Clear()
{
	LOGError("C64Sprite::Clear");
}

void C64Sprite::Serialise(CByteBuffer *byteBuffer)
{
	LOGError("C64Sprite::Serialise");
}

void C64Sprite::Deserialise(CByteBuffer *byteBuffer)
{
	LOGError("C64Sprite::Deserialise");
}
