#ifndef _CViewC64AllGraphicsCharsets_h_
#define _CViewC64AllGraphicsCharsets_h_

#include "CGuiViewMovingPane.h"

class CDebugInterfaceC64;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CSlrFont;

class CViewC64AllGraphicsCharsets : public CGuiViewMovingPane
{
public:
	CViewC64AllGraphicsCharsets(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64AllGraphicsCharsets();

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

	float charsetScale;
	float charsetSizeX;
	float charsetSizeY;
	float charsetScaleB;
	float charsetSizeXB;
	float charsetSizeYB;
	float charsetsOffsetY;
	
	volatile u8 forcedRenderScreenMode;

	bool GetIsSelectedItem();
	void SetIsSelectedItem(bool isSelected);
	bool isSelectedItemCharset;
	volatile int selectedCharsetId;

	int GetItemIdAt(float x, float y);
	int GetSelectedItemId();
	void SetSelectedItemId(int itemId);
	
	// charsets
	std::vector<CImageData *> charsetsImageData;
	std::vector<CSlrImage *> charsetsImages;
	void UpdateCharsets(bool useColors, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800);
	
	// handle ctrl+k shortcut
	void UpdateRenderDataWithColors();

	void ClearCursorPos();
	void ClearGraphicsForcedMode();
};

#endif //_C64_VIEW_ALL_GRAPHICS_
