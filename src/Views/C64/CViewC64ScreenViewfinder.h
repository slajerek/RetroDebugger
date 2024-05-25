#ifndef _CVIEWC64VIEWFINDER_H_
#define _CVIEWC64VIEWFINDER_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewC64Screen;
class CDebugInterfaceC64;

class CViewC64ScreenViewfinder : public CGuiView
{
public:
	CViewC64ScreenViewfinder(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64Screen *viewC64Screen, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);

	CDebugInterfaceC64 *debugInterface;
	CViewC64Screen *viewC64Screen;
	float fontSize;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

	//
	void SetZoomedScreenPos(float zoomedScreenPosX, float zoomedScreenPosY, float zoomedScreenSizeX, float zoomedScreenSizeY);
	void SetZoomedScreenLevel(float zoomedScreenLevel);
	void UpdateZoomedScreenLevel();
	void CalcZoomedScreenTextureFromRaster(int rasterX, int rasterY);
	void RenderZoomedScreen(int rasterX, int rasterY);

	virtual bool IsInsideZoomedScreen(float x, float y);

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

};


#endif

