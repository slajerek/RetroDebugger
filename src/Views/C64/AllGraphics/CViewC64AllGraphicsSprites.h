#ifndef _CViewC64AllGraphicsSprites_h_
#define _CViewC64AllGraphicsSprites_h_

#include "CGuiViewMovingPane.h"

class CDebugInterfaceC64;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CSlrFont;

class CViewC64AllGraphicsSprites : public CGuiViewMovingPane
{
public:
	CViewC64AllGraphicsSprites(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64AllGraphicsSprites();
	
	CDebugInterfaceC64 *debugInterface;
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void DoLogic();
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);
	
	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	virtual bool DoRightClick(float x, float y);
	
	virtual bool DoNotTouchedMove(float x, float y);
	
	virtual void ActivateView();
	virtual void DeactivateView();
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float fontSize;
	
	float spriteScale;
	float spriteSizeX;
	float spriteSizeY;
	float spriteScaleB;
	float spriteSizeXB;
	float spriteSizeYB;
	
	void UpdateShowGrid();
	
	volatile u8 forcedRenderScreenMode;
	
	bool GetIsSelectedItem();
	void SetIsSelectedItem(bool isSelected);
	bool isSelectedItemSprite;
	volatile int selectedSpriteId;
	
	int GetItemIdAt(float x, float y);
	int GetSelectedItemId();
	void SetSelectedItemId(int itemId);
	
	// sprites
	std::vector<CImageData *> spritesImageData;
	std::vector<CSlrImage *> spritesImages;
	void UpdateSprites(bool useColors, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027);
	
	// handle ctrl+k shortcut
	void UpdateRenderDataWithColors();
	
	void ClearCursorPos();
	void ClearGraphicsForcedMode();
	
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
};

#endif
