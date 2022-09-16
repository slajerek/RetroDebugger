#include "CViewC64ScreenViewfinder.h"
#include "CViewC64Screen.h"
#include "CViewC64.h"

// TODO: refactor this and move logic from C64Screen here
CViewC64ScreenViewfinder::CViewC64ScreenViewfinder(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64Screen *viewC64Screen)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->viewC64Screen = viewC64Screen;
	
//	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64ScreenViewfinder::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	viewC64Screen->SetZoomedScreenPos(posX, posY, sizeX, sizeY);
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
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
	viewC64Screen->RenderZoomedScreen(viewC64->c64RasterPosToShowX, viewC64->c64RasterPosToShowY);

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

bool CViewC64ScreenViewfinder::DoScrollWheel(float deltaX, float deltaY)
{
	return viewC64Screen->DoScrollWheel(deltaX, deltaY);
}

// Layout
void CViewC64ScreenViewfinder::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64ScreenViewfinder::Deserialize(CByteBuffer *byteBuffer)
{
}

