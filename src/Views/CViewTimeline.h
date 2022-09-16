#ifndef _VIEW_TIMELINE_
#define _VIEW_TIMELINE_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "SYS_Threading.h"

class CDebugInterface;

class CTimelineIoThread : public CSlrThread
{
public:
	CDebugInterface *debugInterface;
	u8 asyncOperation;
	CSlrString *timelinePath;
	virtual void ThreadRun(void *passData);
};

class CViewTimeline : public CGuiView, CGuiButtonCallback
{
public:
	CViewTimeline(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewTimeline();

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
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

//	virtual void FinishTouches();

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();

	CGuiButton *btnDone;
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	
	CDebugInterface *debugInterface;
	
	bool isScrubbing;
	bool isLockedVisible;
	
	float fontSize;
	
	u64 scrubIntervalMS;
	u64 nextPossibleScrubTime;

	int CalcFrameNumFromMousePos(int minFrame, int maxFrame);
	
	int GetCurrentFrameNum();
	void GetFramesLimits(int *minFrame, int *maxFrame);
	void ScrubToFrame(int frameNum);
	
	void ScrubToPos(float x);
	
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	static void LoadTimeline(CSlrString *path);
	static void SaveTimeline(CSlrString *path, CDebugInterface *debugInterface);
};

#endif //_VIEW_TIMELINE_
