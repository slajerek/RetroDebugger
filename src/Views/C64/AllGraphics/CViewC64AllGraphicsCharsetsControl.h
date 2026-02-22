#ifndef _CViewC64AllGraphicsCharsetsControl_h_
#define _CViewC64AllGraphicsCharsetsControl_h_

#include "CGuiView.h"
#include "CGuiLabel.h"
#include "CGuiButtonSwitch.h"

class CDebugInterfaceC64;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CSlrFont;

class CViewC64AllGraphicsCharsetsControl : public CGuiView, CGuiButtonSwitchCallback
{
public:
	CViewC64AllGraphicsCharsetsControl(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64AllGraphicsCharsetsControl();

	CDebugInterfaceC64 *debugInterface;

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void PostRenderImGui();
	virtual void DoLogic();

	virtual bool DoTap(float x, float y);

	virtual bool DoRightClick(float x, float y);

	virtual void ActivateView();
	virtual void DeactivateView();

	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float fontSize;
	bool hasManualFontScale;

	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);

	CGuiButtonSwitch *btnModeBitmapColorsBlackWhite;
	CGuiButtonSwitch *btnModeHires;
	CGuiButtonSwitch *btnModeMulti;
	void SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn);
	void SetLockableButtonDefaultColors(CGuiButtonSwitch *btn);
//	void SetButtonState(CGuiButtonSwitch *btn, bool isSet);
	
	CGuiButtonSwitch *btnShowRAMorIO;
	void UpdateShowIOButton();
	CGuiButtonSwitch *btnShowGrid;
	void UpdateShowGrid();
	
	void UpdateRenderDataWithColors();

	volatile u8 forcedRenderScreenMode;
	void ClearGraphicsForcedMode();

	void SetupMode();
	void RepositionButtons();
};

#endif //_C64_VIEW_ALL_GRAPHICS_
