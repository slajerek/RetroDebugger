#ifndef _CViewBreakpoints_h_
#define _CViewBreakpoints_h_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class CGuiEvent;
class CDebuggerApi;
class CDebugBreakpointsAddr;

class CViewBreakpoints : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback
{
public:
	CViewBreakpoints(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugSymbols *symbols, int breakpointType);
	virtual ~CViewBreakpoints();
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
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
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();
	
	virtual void Run();
	
	CDebugSymbols *symbols;
	int breakpointType;
};

#endif //_VIEW_C64GOATTRACKER_
