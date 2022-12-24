#ifndef _CViewC64AllGraphicsBitmaps_h_
#define _CViewC64AllGraphicsBitmaps_h_

#include "CGuiViewMovingPane.h"

class CDebugInterfaceC64;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CViewC64AllGraphicsBitmapsControl;
class CSlrFont;

class CViewC64AllGraphicsBitmaps : public CGuiViewMovingPane
{
public:
	CViewC64AllGraphicsBitmaps(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface, CViewC64AllGraphicsBitmapsControl *viewControl);
	virtual ~CViewC64AllGraphicsBitmaps();

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

	int numBitmapDisplays;
	
	CViewC64VicDisplay **vicDisplays;
	CViewC64VicControl **vicControl;
	int numVicDisplays;
	int numVisibleDisplays;
	int numDisplaysColumns;
	int numDisplaysRows;
	void SetupVicDisplays();

	void UpdateShowIOButton();
	void UpdateShowGrid();
	
	bool GetIsSelectedItem();
	void SetIsSelectedItem(bool isSelected);
	bool isSelectedItemBitmap;
	volatile int selectedBitmapId;

	int GetItemIdAt(float x, float y);
	int GetSelectedItemId();
	void SetSelectedItemId(int itemId);
	
	// handle ctrl+k shortcut
	void UpdateRenderDataWithColors();

	void ClearCursorPos();
	void ClearGraphicsForcedMode();
	
	CViewC64AllGraphicsBitmapsControl *viewControl;
};

#endif //_C64_VIEW_ALL_GRAPHICS_
