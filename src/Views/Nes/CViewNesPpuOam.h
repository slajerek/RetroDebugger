#ifndef _CViewNesPpuOam_H_
#define _CViewNesPpuOam_H_

#include "CGuiView.h"
#include <map>

class CSlrMutex;
class CDebugInterfaceNes;

class CViewNesPpuOam : public CGuiView
{
public:
	CViewNesPpuOam(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface);
	virtual ~CViewNesPpuOam();

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

	CSlrImage *imageScreen;

	CImageData *imageDataScreenDefault;
	CSlrImage *imageScreenDefault;

	void RefreshScreen();

	float screenTexEndX, screenTexEndY;
	
	CDebugInterfaceNes *debugInterface;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	bool showGridLines;

};

#endif //_CViewNesPpuOam_H_
