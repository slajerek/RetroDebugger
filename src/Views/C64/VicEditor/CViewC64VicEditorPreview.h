#ifndef _CViewVicEditorPreview_h_
#define _CViewVicEditorPreview_h_

#include "CGuiView.h"
#include <list>
#include <vector>
#include <map>

class CViewC64VicDisplay;
class CViewC64VicEditor;

class CViewC64VicEditorPreview : public CGuiView
{
public:
	
	CViewC64VicDisplay *viewVicDisplay;
	CViewC64VicEditor *vicEditor;
	
	CViewC64VicEditorPreview(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64VicEditor *vicEditor);
	virtual ~CViewC64VicEditorPreview();

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

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
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual void ActivateView();
	virtual void DeactivateView();

};

#endif

