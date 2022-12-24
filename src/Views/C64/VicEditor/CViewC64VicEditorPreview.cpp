#include "CViewC64VicEditorPreview.h"
#include "CViewC64VicEditorDisplayPreview.h"
#include "CViewC64.h"

CViewC64VicEditorPreview::CViewC64VicEditorPreview(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64VicEditor *vicEditor)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->vicEditor = vicEditor;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	viewVicDisplay = new CViewC64VicEditorDisplayPreview("viewVicEditorDisplayPreview", 100, 100, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, viewC64->debugInterfaceC64, vicEditor);
	viewVicDisplay->renderDisplayFrame = true;
	
	viewVicDisplay->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA);
	
	viewVicDisplay->SetDisplayPosition(340, 20, 0.7f, true);

	viewVicDisplay->gridLinesColorA = 0.0f;
	viewVicDisplay->gridLinesColorA2 = 0.0f;

	viewVicDisplay->showRasterCursor = false;
	
	viewVicDisplay->showSpritesFrames = false;
	
	viewVicDisplay->applyScrollRegister = false;
	
}

CViewC64VicEditorPreview::~CViewC64VicEditorPreview()
{
}

void CViewC64VicEditorPreview::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64VicEditorPreview::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64VicEditorPreview::Render()
{
	guiMain->fntConsole->BlitText("CViewVicEditorPreview", posX, posY, 0, 11, 1.0);

	CGuiView::Render();
}

void CViewC64VicEditorPreview::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64VicEditorPreview::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


//@returns is consumed
bool CViewC64VicEditorPreview::DoTap(float x, float y)
{
	LOGG("CViewVicEditorPreview::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewC64VicEditorPreview::DoFinishTap(float x, float y)
{
	LOGG("CViewVicEditorPreview::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64VicEditorPreview::DoDoubleTap(float x, float y)
{
	LOGG("CViewVicEditorPreview::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64VicEditorPreview::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewVicEditorPreview::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64VicEditorPreview::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64VicEditorPreview::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64VicEditorPreview::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64VicEditorPreview::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64VicEditorPreview::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewC64VicEditorPreview::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64VicEditorPreview::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64VicEditorPreview::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64VicEditorPreview::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64VicEditorPreview::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDownRepeat(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64VicEditorPreview::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64VicEditorPreview::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64VicEditorPreview::ActivateView()
{
	LOGG("CViewVicEditorPreview::ActivateView()");
}

void CViewC64VicEditorPreview::DeactivateView()
{
	LOGG("CViewVicEditorPreview::DeactivateView()");
}
