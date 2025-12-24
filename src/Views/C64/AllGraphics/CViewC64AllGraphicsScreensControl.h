#ifndef _CViewC64AllGraphicsScreensControl_h_
#define _CViewC64AllGraphicsScreensControl_h_

#include "CGuiView.h"
#include "CGuiLabel.h"
#include "CGuiLockableList.h"
#include "CGuiButtonSwitch.h"

class CDebugInterfaceC64;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CSlrFont;

class CViewC64AllGraphicsScreensControl : public CGuiView, CGuiButtonSwitchCallback, CGuiListCallback
{
public:
	CViewC64AllGraphicsScreensControl(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64AllGraphicsScreensControl();
	
	CDebugInterfaceC64 *debugInterface;
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void DoLogic();
	
	virtual bool DoTap(float x, float y);
	
	virtual bool DoRightClick(float x, float y);
	
	virtual void ActivateView();
	virtual void DeactivateView();
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float fontSize;
	
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);
	
	CGuiButtonSwitch *btnModeBitmapColorsBlackWhite;
	CGuiButtonSwitch *btnModeHires;
	CGuiButtonSwitch *btnModeMulti;
	void SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn);
	void SetLockableButtonDefaultColors(CGuiButtonSwitch *btn);
	//	void SetButtonState(CGuiButtonSwitch *btn, bool isSet);
	
	CGuiLabel *lblCharsetAddress;
	CGuiLockableList *lstCharsetAddresses;
	
	CGuiButtonSwitch *btnShowRAMorIO;
	void UpdateShowIOButton();
	CGuiButtonSwitch *btnShowGrid;
	void UpdateShowGrid();
	
	virtual bool ListElementPreSelect(CGuiList *listBox, int elementNum);
	
	void UpdateRenderDataWithColors();
	
	volatile u8 forcedRenderScreenMode;
	void ClearGraphicsForcedMode();
	
	void SetupMode();
	
	// resize
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
};

#endif //_C64_VIEW_ALL_GRAPHICS_
