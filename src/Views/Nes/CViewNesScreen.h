#ifndef _CVIEWNESSCREEN_H_
#define _CVIEWNESSCREEN_H_

#include "CGuiView.h"
#include <map>

class CSlrMutex;
class CDebugInterfaceNes;

class CViewNesScreen : public CGuiView
{
public:
	CViewNesScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface);
	virtual ~CViewNesScreen();

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

	virtual int GetJoystickAxis(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoGamePadButtonDown(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadButtonUp(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value);

	virtual void JoystickDown(int port, u32 axis);
	virtual void JoystickUp(int port, u32 axis);

	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();

	CSlrImage *imageScreen;

	CImageData *imageDataScreenDefault;
	CSlrImage *imageScreenDefault;

	void RefreshScreen();

	float screenTexEndX, screenTexEndY;
	
	void KeyUpModifierKeys(bool isShift, bool isAlt, bool isControl);
	
	CDebugInterfaceNes *debugInterface;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	void UpdateRasterCrossFactors();
	
	float rasterScaleFactorX;
	float rasterScaleFactorY;
	
	float rasterCrossOffsetX;
	float rasterCrossOffsetY;
	
	float rasterCrossWidth;
	float rasterCrossWidth2;
	
	float rasterCrossSizeX;
	float rasterCrossSizeY;
	float rasterCrossSizeX2;
	float rasterCrossSizeY2;
	float rasterCrossSizeX34;
	float rasterCrossSizeY34;
	float rasterCrossSizeX3;
	float rasterCrossSizeY3;
	float rasterCrossSizeX4;
	float rasterCrossSizeY4;
	float rasterCrossSizeX6;
	float rasterCrossSizeY6;
	
	/// long screen line
	float rasterLongScrenLineR;
	float rasterLongScrenLineG;
	float rasterLongScrenLineB;
	float rasterLongScrenLineA;
	
	// red cross
	float rasterCrossExteriorR;
	float rasterCrossExteriorG;
	float rasterCrossExteriorB;
	float rasterCrossExteriorA;
	
	// cross ending tip
	float rasterCrossEndingTipR;
	float rasterCrossEndingTipG;
	float rasterCrossEndingTipB;
	float rasterCrossEndingTipA;
	
	// white interior cross
	float rasterCrossInteriorR;
	float rasterCrossInteriorG;
	float rasterCrossInteriorB;
	float rasterCrossInteriorA;

	void InitRasterColorsFromScheme();
	
	void RenderRaster(int rasterX, int rasterY);
	
	bool showGridLines;
	
	float gridLinesColorR;
	float gridLinesColorG;
	float gridLinesColorB;
	float gridLinesColorA;
	
	void SetZoomedScreenPos(float zoomedScreenPosX, float zoomedScreenPosY, float zoomedScreenSizeX, float zoomedScreenSizeY);
	void SetZoomedScreenLevel(float zoomedScreenLevel);
	void CalcZoomedScreenTextureFromRaster(int rasterX, int rasterY);
	void RenderZoomedScreen(int rasterX, int rasterY);
	
	bool showZoomedScreen;
	float zoomedScreenPosX;
	float zoomedScreenPosY;
	float zoomedScreenSizeX;
	float zoomedScreenSizeY;
	float zoomedScreenCenterX;
	float zoomedScreenCenterY;
	float zoomedScreenLevel;
	
	float zoomedScreenImageStartX;
	float zoomedScreenImageStartY;
	float zoomedScreenImageSizeX;
	float zoomedScreenImageSizeY;

	float zoomedScreenRasterScaleFactorX;
	float zoomedScreenRasterScaleFactorY;
	float zoomedScreenRasterOffsetX;
	float zoomedScreenRasterOffsetY;

	//std::map<u32, bool> pressedKeyCodes;
	virtual void SetSupersampleFactor(int supersampleFactor);

};

#endif //_CViewNesScreen_H_
