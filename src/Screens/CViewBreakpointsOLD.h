#ifndef _VIEW_C64BREAKPOINTS_
#define _VIEW_C64BREAKPOINTS_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiButtonSwitch.h"
#include "CGuiLabel.h"
#include "CGuiEditHex.h"
#include "CColorsTheme.h"

class CBreakpointAddr;
class CBreakpointMemory;
class CDebugBreakpointsAddr;
class CDebugBreakpointsMemory;

class CViewBreakpointsOLD : public CGuiView, CGuiButtonSwitchCallback, CGuiEditHexCallback
{
public:
	CViewBreakpointsOLD(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewBreakpointsOLD();

	virtual void Render();
	virtual void Render(float posX, float posY);
	//virtual void Render(float posX, float posY, float sizeX, float sizeY);
	virtual void DoLogic();

	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();

	CGuiButton *btnDone;
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	
	CDebugInterface *debugInterface;
	
	CSlrFont *font;
	float fontScale;
	float fontWidth;
	float fontHeight;
	float fontNumbersScale;
	float fontNumbersWidth;
	float fontNumbersHeight;
	
	float tr;
	float tg;
	float tb;
	
	char buf[128];
	
	CSlrString *strHeader;

	CGuiLabel *lblPlatform;
	CGuiButtonSwitch *btnBreakpointsPC;
	CGuiButtonSwitch *btnBreakpointsMemory;
	CGuiButtonSwitch *btnBreakpointsRaster;

	CGuiButtonSwitch *btnBreakpointC64IrqVIC;
	CGuiButtonSwitch *btnBreakpointC64IrqCIA;
	CGuiButtonSwitch *btnBreakpointC64IrqNMI;

	CGuiLabel *lbl1541Drive;
	CGuiButtonSwitch *btnBreakpointDrive1541IrqVIA1;
	CGuiButtonSwitch *btnBreakpointDrive1541IrqVIA2;
	CGuiButtonSwitch *btnBreakpointDrive1541IrqIEC;
	CGuiButtonSwitch *btnBreakpointsDrive1541PC;
	CGuiButtonSwitch *btnBreakpointsDrive1541Memory;

	
	float pcBreakpointsX;
	float pcBreakpointsY;
	float memoryBreakpointsX;
	float memoryBreakpointsY;
	float rasterBreakpointsX;
	float rasterBreakpointsY;

	float Drive1541PCBreakpointsX;
	float Drive1541PCBreakpointsY;
	float Drive1541MemoryBreakpointsX;
	float Drive1541MemoryBreakpointsY;
	float Drive1541RasterBreakpointsX;
	float Drive1541RasterBreakpointsY;

	int cursorGroup;
	int cursorElement;
	int cursorPosition;
	
	CSlrString *strTemp;
	
	void SwitchBreakpointsScreen();
	void UpdateCursor();
	void ClearCursor();

	bool isEditingValue;
	
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);

	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);

	CBreakpointAddr *editingBreakpoint;
	CGuiEditHex *editHex;
	
	//
	void RenderAddrBreakpoints(CDebugBreakpointsAddr *addrBreakpoints, float pStartX, float pStartY, int cursorGroupId,
							   char *addrFormatStr, char *addrEmptyStr);
	void RenderMemoryBreakpoints(CDebugBreakpointsMemory *memoryBreakpoints, float pStartX, float pStartY, int cursorGroupId);
	
	bool CheckTapAddrBreakpoints(float x, float y,
								 CDebugBreakpointsAddr *addrBreakpoints,
								 float pStartX, float pStartY, int cursorGroupId);
	
	bool CheckTapMemoryBreakpoints(float x, float y,
								   CDebugBreakpointsMemory *memoryBreakpoints,
								   float pStartX, float pStartY, int cursorGroupId);

	
	void GuiEditHexEnteredValueAddr(CGuiEditHex *editHex, CDebugBreakpointsAddr *addrBreakpoints);
	void GuiEditHexEnteredValueMemory(CGuiEditHex *editHex, u32 lastKeyCode, CDebugBreakpointsMemory *memoryBreakpoints);
	
	void StartEditingSelectedAddrBreakpoint(CDebugBreakpointsAddr *addrBreakpoints, char *emptyAddrStr);
	void StartEditingSelectedMemoryBreakpoint(CDebugBreakpointsMemory *memoryBreakpoints);
	
	void DeleteSelectedAddrBreakpoint(CDebugBreakpointsAddr *addrBreakpoints);
	void DeleteSelectedMemoryBreakpoint(CDebugBreakpointsMemory *memoryBreakpoints);
	
	CGuiView *prevView;
	
	virtual void UpdateTheme();
};

#endif //_VIEW_C64BREAKPOINTS_
