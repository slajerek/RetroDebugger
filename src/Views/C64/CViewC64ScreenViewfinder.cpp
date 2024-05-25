#include "CViewC64ScreenViewfinder.h"
#include "CViewC64Screen.h"
#include "CViewC64.h"
#include "C64SettingsStorage.h"
#include "CDebugInterfaceC64.h"

// TODO: refactor this and move logic from C64Screen here
CViewC64ScreenViewfinder::CViewC64ScreenViewfinder(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64Screen *viewC64Screen, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->viewC64Screen = viewC64Screen;
	this->debugInterface = debugInterface;

	// zoomed screen
	this->zoomedScreenLevel = c64SettingsScreenRasterViewfinderScale;

//	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64ScreenViewfinder::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	this->SetZoomedScreenPos(posX, posY, sizeX, sizeY);
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

bool CViewC64ScreenViewfinder::DoScrollWheel(float deltaX, float deltaY)
{
	float newLevel = this->zoomedScreenLevel + deltaY*0.045f;
	
	SetZoomedScreenLevel(newLevel);
	
	return true;
}

bool CViewC64ScreenViewfinder::IsInsideZoomedScreen(float x, float y)
{
	if (x >= this->zoomedScreenPosX && x <= (this->zoomedScreenPosX + this->zoomedScreenSizeX)
		&& y >= this->zoomedScreenPosY && y <= (this->zoomedScreenPosY + this->zoomedScreenSizeY))
	{
		return true;
	}
	
	return false;
}

bool CViewC64ScreenViewfinder::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64ScreenViewfinder::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	float newLevel = this->zoomedScreenLevel + difference * 0.06f;
	SetZoomedScreenLevel(newLevel);
	
	return true;
}


void CViewC64ScreenViewfinder::CalcZoomedScreenTextureFromRaster(int rasterX, int rasterY)
{
	float ttrx = (float)rasterX * zoomedScreenRasterScaleFactorX + zoomedScreenRasterOffsetX;
	float ttry = (float)rasterY * zoomedScreenRasterScaleFactorY + zoomedScreenRasterOffsetY;

	zoomedScreenImageStartX = zoomedScreenCenterX - ttrx;
	zoomedScreenImageStartY = zoomedScreenCenterY - ttry;
}

void CViewC64ScreenViewfinder::RenderZoomedScreen(int rasterX, int rasterY)
{
//	LOGD("CViewC64Screen::RenderZoomedScreen rx=%d ry=%d", rasterX, rasterY);
	
	UpdateZoomedScreenLevel();
	CalcZoomedScreenTextureFromRaster(rasterX, rasterY);
	
	VID_SetClipping(zoomedScreenPosX, zoomedScreenPosY, zoomedScreenSizeX, zoomedScreenSizeY);
	
//	LOGD("zoomedScreenImageStartX=%f zoomedScreenImageStartY=%f zoomedScreenImageSizeX=%f zoomedScreenImageSizeY=%f",
//		 zoomedScreenImageStartX, zoomedScreenImageStartY, zoomedScreenImageSizeX, zoomedScreenImageSizeY);
	Blit(viewC64Screen->image,
		 zoomedScreenImageStartX,
		 zoomedScreenImageStartY, -1,
		 zoomedScreenImageSizeX,
		 zoomedScreenImageSizeY,
		 0.0f, 0.0f, viewC64Screen->renderTextureEndX, viewC64Screen->renderTextureEndY);
	
	float rs = 0.3f;
	float rs2 = rs*2.0f;
	BlitFilledRectangle(zoomedScreenCenterX - rs, zoomedScreenPosY, -1, rs2, zoomedScreenSizeY,
						viewC64Screen->rasterLongScrenLineR, viewC64Screen->rasterLongScrenLineG,
						viewC64Screen->rasterLongScrenLineB, viewC64Screen->rasterLongScrenLineA);
	BlitFilledRectangle(zoomedScreenPosX, zoomedScreenCenterY, -1, zoomedScreenSizeX, rs2,
						viewC64Screen->rasterLongScrenLineR, viewC64Screen->rasterLongScrenLineG,
						viewC64Screen->rasterLongScrenLineB, viewC64Screen->rasterLongScrenLineA);

	VID_ResetClipping();
}
void CViewC64ScreenViewfinder::SetZoomedScreenPos(float zoomedScreenPosX, float zoomedScreenPosY, float zoomedScreenSizeX, float zoomedScreenSizeY)
{
	this->zoomedScreenPosX = zoomedScreenPosX;
	this->zoomedScreenPosY = zoomedScreenPosY;
	this->zoomedScreenSizeX = zoomedScreenSizeX;
	this->zoomedScreenSizeY = zoomedScreenSizeY;
	
	this->zoomedScreenCenterX = zoomedScreenPosX + zoomedScreenSizeX/2.0f;
	this->zoomedScreenCenterY = zoomedScreenPosY + zoomedScreenSizeY/2.0f;

	this->UpdateZoomedScreenLevel();
}

void CViewC64ScreenViewfinder::SetZoomedScreenLevel(float zoomedScreenLevel)
{
	if (zoomedScreenLevel < 0.05f)
	{
		zoomedScreenLevel = 0.05f;
	}
	
	if (zoomedScreenLevel > 25.0f)
	{
		zoomedScreenLevel = 25.0f;
	}
	
	this->zoomedScreenLevel = zoomedScreenLevel;

	UpdateZoomedScreenLevel();
}

void CViewC64ScreenViewfinder::UpdateZoomedScreenLevel()
{
	zoomedScreenImageSizeX = (float)debugInterface->GetScreenSizeX() * zoomedScreenLevel;
	zoomedScreenImageSizeY = (float)debugInterface->GetScreenSizeY() * zoomedScreenLevel;

	zoomedScreenRasterScaleFactorX = zoomedScreenImageSizeX / (float)debugInterface->GetScreenSizeX();
	zoomedScreenRasterScaleFactorY = zoomedScreenImageSizeY / (float)debugInterface->GetScreenSizeY();
	zoomedScreenRasterOffsetX =  -103.787 * zoomedScreenRasterScaleFactorX;
	zoomedScreenRasterOffsetY = -15.500 * zoomedScreenRasterScaleFactorY;
	
}

void CViewC64ScreenViewfinder::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64ScreenViewfinder::DoLogic()
{
}

void CViewC64ScreenViewfinder::RenderImGui()
{
	PreRenderImGui();
	
	// TODO: refactor and move me to C64DebugInterface
	RenderZoomedScreen(viewC64->c64RasterPosToShowX, viewC64->c64RasterPosToShowY);

	PostRenderImGui();
}

void CViewC64ScreenViewfinder::Render()
{
}

bool CViewC64ScreenViewfinder::DoTap(float x, float y)
{
	return false;
}

//
bool CViewC64ScreenViewfinder::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewC64ScreenViewfinder::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

// Layout
void CViewC64ScreenViewfinder::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64ScreenViewfinder::Deserialize(CByteBuffer *byteBuffer)
{
}

