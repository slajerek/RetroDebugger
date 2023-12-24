#ifndef _VIEW_C64GOATTRACKER_
#define _VIEW_C64GOATTRACKER_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class CGuiEvent;

class CViewC64GoatTracker : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback
{
public:
	CViewC64GoatTracker(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewC64GoatTracker();
	
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
	
	virtual bool DoNotTouchedMove(float x, float y);

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
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	//

	//
	CImageData *imageDataScreen;
	CSlrImage *imageScreen;
	
	//
	CSlrMutex *mutex;
	std::list<CGuiEvent *> events;
	void AddEvent(CGuiEvent *event);
	void ForwardEvents();
};


#endif //_VIEW_C64GOATTRACKER_
