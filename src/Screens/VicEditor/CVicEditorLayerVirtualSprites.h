#ifndef _VICEDITORLAYERVIRTUALSPRITES_H_
#define _VICEDITORLAYERVIRTUALSPRITES_H_

#include "SYS_Defs.h"
#include "CVicEditorLayer.h"
#include <list>

class C64Sprite;
class CViewC64VicDisplay;

class CVicEditorLayerVirtualSprites : public CVicEditorLayer
{
	public:
	CVicEditorLayerVirtualSprites(CViewVicEditor *vicEditor);
	~CVicEditorLayerVirtualSprites();
	
	virtual void RenderMain(vicii_cycle_state_t *viciiState);
	virtual void RenderPreview(vicii_cycle_state_t *viciiState);
	virtual void RenderGridMain(vicii_cycle_state_t *viciiState);
	virtual void RenderGridPreview(vicii_cycle_state_t *viciiState);

	void RenderGrid(vicii_cycle_state_t *viciiState, CViewC64VicDisplay *vicDisplay);
	
	virtual void Serialise(CByteBuffer *byteBuffer);
	virtual void Deserialise(CByteBuffer *byteBuffer, int version);
	
	//
	std::list<C64Sprite *> sprites;
	
	void ClearSprites();
	void FullScanSpritesInThisFrame();
	void SimpleScanSpritesInThisFrame();
	void ScanSpritesStoreAddressesOnly(int rx, int ry,
									   int *addrPosXHighBits,
									   int addrPosX[8],
									   int addrPosY[8],
									   int *addrSetStretchHorizontal,
									   int *addrSetStretchVertical,
									   int *addrSetMultiColor,
									   int *addrColorChangeCommon1,
									   int *addrColorChangeCommon2,
									   int addrColorChange[8]);
	
	bool showSpriteFrames;
	bool showSpriteGrid;
	
	virtual void ClearDitherMask();
	int multiDitherMaskPosX;
	int multiDitherMaskPosY;
	int hiresDitherMaskPosX;
	int hiresDitherMaskPosY;
	
	C64Sprite *FindSpriteByRasterPos(int rx, int ry);
	
	void UpdateSpriteView(int rx, int ry);
	
	//
	virtual u8 ReplaceColor(int rx, int ry, int spriteId, int colorNum, u8 colorValue);
	virtual u8 ReplaceColor(int rx, int ry, int spriteId, int colorNum, u8 colorValue,
							 int addrColorChange[8],
							 int addrColorChangeCommon1,
							 int addrColorChangeCommon2);
	
	virtual void ReplacePosX(int rx, int ry, int spriteId, int posX);
	virtual void ReplacePosY(int rx, int ry, int spriteId, int posY);
	
	virtual void ReplaceMultiColor(int rx, int ry, int spriteId, bool isMultiColor);
	virtual void ReplaceStretchX(int rx, int ry, int spriteId, bool isStretchX);
	virtual void ReplaceStretchY(int rx, int ry, int spriteId, bool isStretchY);

	virtual u8 Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue);

	virtual bool GetColorAtPixel(int x, int y, u8 *color);

	virtual void ClearScreen();
	virtual void ClearScreen(u8 charValue, u8 colorValue);
	
};

#endif
