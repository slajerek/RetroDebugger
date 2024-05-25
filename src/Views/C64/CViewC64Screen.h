#ifndef _CViewC64Screen_h_
#define _CViewC64Screen_h_

#include "CViewEmulatorScreen.h"

class CViewC64Screen : public CViewEmulatorScreen
{
public:
	CViewC64Screen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewC64Screen();
	
	virtual bool IsSkipKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual u32 ConvertKeyCode(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual void PostDebugInterfaceKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual void PostDebugInterfaceKeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	bool shiftDown;
		
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

	bool showGridLines;
	float gridLinesColorR;
	float gridLinesColorG;
	float gridLinesColorB;
	float gridLinesColorA;

	bool alwaysShowRaster;
	bool renderRasterOnForeground;

	void InitRasterColorsFromScheme();
	void UpdateRasterCrossFactors();
	void RenderRaster(int rasterX, int rasterY);
	void BlitRasterFilledRectangle(float destX, float destY, float z, float sizeX, float sizeY,
								   float colorR, float colorG, float colorB, float alpha);
};

#endif
