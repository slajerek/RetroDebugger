#ifndef _C64_VIEW_ALL_GRAPHICS_
#define _C64_VIEW_ALL_GRAPHICS_

#include "CGuiView.h"
#include "CGuiLabel.h"
#include "CGuiLockableList.h"
#include "CGuiButtonSwitch.h"

class CDebugInterfaceC64;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CSlrFont;

#define VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS	1
#define VIEW_C64_ALL_GRAPHICS_MODE_SCREENS	2
#define VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS	3
#define VIEW_C64_ALL_GRAPHICS_MODE_SPRITES	4
#define VIEW_C64_ALL_GRAPHICS_MODE_COLOR	5

#define VIEW_C64_ALL_GRAPHICS_FORCED_NONE	0
#define VIEW_C64_ALL_GRAPHICS_FORCED_GRAY	1
#define VIEW_C64_ALL_GRAPHICS_FORCED_HIRES	2
#define VIEW_C64_ALL_GRAPHICS_FORCED_MULTI	3

class CViewC64AllGraphics : public CGuiView, CGuiButtonSwitchCallback, CGuiListCallback
{
public:
	CViewC64AllGraphics(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64AllGraphics();

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

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);
	virtual bool DoNotTouchedMove(float x, float y);
	
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();

	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float fontSize;

	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);

	int displayMode;
	void SetMode(int newMode);
	
	int numBitmapDisplays;
	int numScreenDisplays;
	
	CViewC64VicDisplay **vicDisplays;
	CViewC64VicControl **vicControl;
	int numVicDisplays;
	
	int numVisibleDisplays;
	int numDisplaysColumns;
	int numDisplaysRows;

	float charsetScale;
	float charsetSizeX;
	float charsetSizeY;
	float charsetScaleB;
	float charsetSizeXB;
	float charsetSizeYB;
	float spriteScale;
	float spriteSizeX;
	float spriteSizeY;
	float spriteScaleB;
	float spriteSizeXB;
	float spriteSizeYB;

	float charsetsOffsetY;
	
	CGuiButtonSwitch *btnShowBitmaps;
	CGuiButtonSwitch *btnShowScreens;
	CGuiButtonSwitch *btnShowCharsets;
	CGuiButtonSwitch *btnShowSprites;
	CGuiButtonSwitch *btnShowColor;

	CGuiButtonSwitch *btnModeBitmapColorsBlackWhite;
	CGuiButtonSwitch *btnModeHires;
	CGuiButtonSwitch *btnModeMulti;
	void SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn);
	void SetLockableButtonDefaultColors(CGuiButtonSwitch *btn);
//	void SetButtonState(CGuiButtonSwitch *btn, bool isSet);
	
	CGuiLabel *lblScreenAddress;
	CGuiLockableList *lstScreenAddresses;

	CGuiLabel *lblCharsetAddress;
	CGuiLockableList *lstCharsetAddresses;

	CGuiButtonSwitch *btnShowRAMorIO;
	void UpdateShowIOButton();
	CGuiButtonSwitch *btnShowGrid;
	void UpdateShowGrid();
	
	virtual bool ListElementPreSelect(CGuiList *listBox, int elementNum);

	volatile u8 forcedRenderScreenMode;

	bool GetIsSelectedItem();
	void SetIsSelectedItem(bool isSelected);
	bool isSelectedItemBitmap;
	bool isSelectedItemScreen;
	bool isSelectedItemCharset;
	bool isSelectedItemSprite;
	bool isSelectedItemColor;
	volatile int selectedBitmapId;
	volatile int selectedScreenId;
	volatile int selectedCharsetId;
	volatile int selectedSpriteId;

	int GetItemIdAt(float x, float y);
	int GetSelectedItemId();
	void SetSelectedItemId(int itemId);
	
	// sprites
	std::vector<CImageData *> spritesImageData;
	std::vector<CSlrImage *> spritesImages;
	void UpdateSprites(bool useColors, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027);
	
	// charsets
	std::vector<CImageData *> charsetsImageData;
	std::vector<CSlrImage *> charsetsImages;
	void UpdateCharsets(bool useColors, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800);
	
	// color
	CImageData *colorImageData;
	CSlrImage *colorImage;

	// handle ctrl+k shortcut
	void UpdateRenderDataWithColors();

	void ClearRasterCursorPos();
	void ClearGraphicsForcedMode();
};

#endif //_C64_VIEW_ALL_GRAPHICS_
