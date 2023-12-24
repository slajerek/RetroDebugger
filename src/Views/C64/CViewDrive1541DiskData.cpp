#include "CViewDrive1541DiskData.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"

extern "C" {
#define DWORD u32
disk_image_t *c64d_get_drive_disk_image(int driveId);
#include "gcr.h"
#include "diskimage.h"
}

CViewDrive1541DiskData::CViewDrive1541DiskData(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
//	imGuiNoWindowPadding = true;
//	imGuiNoScrollbar = true;
}

CViewDrive1541DiskData::~CViewDrive1541DiskData()
{
}

void CViewDrive1541DiskData::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewDrive1541DiskData::Render()
{
	guiMain->fntConsole->BlitText("CViewDrive1541DiskData", posX, posY, 0, 11, 1.0);

	CGuiView::Render();
}

void CViewDrive1541DiskData::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewDrive1541DiskData::RenderImGui()
{
	PreRenderImGui();

	int driveId = 0;
	CDebugInterface *debugInterface = viewC64->debugInterfaceC64;
	
	debugInterface->LockMutex();
	this->diskImage = c64d_get_drive_disk_image(driveId);

	/* Number of tracks we emulate. 84 for 1541, 168 for 1571 */
	#define MAX_GCR_TRACKS 168

	//
	int sum = 0;
	for (int t = 0; t < MAX_GCR_TRACKS; t++) //84; t++)
	{
		ImGui::Text("track %d size=%d", t, this->diskImage->gcr->tracks[t].size);
	}
	sum++;
	
	
	
	
	
	debugInterface->UnlockMutex();
	
	PostRenderImGui();
}

//@returns is consumed
bool CViewDrive1541DiskData::DoTap(float x, float y)
{
	LOGG("CViewDrive1541DiskData::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewDrive1541DiskData::DoFinishTap(float x, float y)
{
	LOGG("CViewDrive1541DiskData::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewDrive1541DiskData::DoDoubleTap(float x, float y)
{
	LOGG("CViewDrive1541DiskData::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewDrive1541DiskData::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewDrive1541DiskData::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewDrive1541DiskData::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewDrive1541DiskData::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewDrive1541DiskData::DoRightClick(float x, float y)
{
	return CGuiView::DoRightClick(x, y);
}

bool CViewDrive1541DiskData::DoFinishRightClick(float x, float y)
{
	return CGuiView::CGuiElement::DoFinishRightClick(x, y);
}

bool CViewDrive1541DiskData::DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoRightClickMove(x, y, distX, distY, diffX, diffY);
}

bool CViewDrive1541DiskData::FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::CGuiElement::FinishRightClickMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewDrive1541DiskData::DoNotTouchedMove(float x, float y)
{
	return CGuiView::DoNotTouchedMove(x, y);
}

bool CViewDrive1541DiskData::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewDrive1541DiskData::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewDrive1541DiskData::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewDrive1541DiskData::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewDrive1541DiskData::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewDrive1541DiskData::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewDrive1541DiskData::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541DiskData::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541DiskData::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541DiskData::DoGamePadButtonDown(CGamePad *gamePad, u8 button)
{
	return CGuiView::DoGamePadButtonDown(gamePad, button);
}

bool CViewDrive1541DiskData::DoGamePadButtonUp(CGamePad *gamePad, u8 button)
{
	return CGuiView::DoGamePadButtonUp(gamePad, button);
}

bool CViewDrive1541DiskData::DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value)
{
	return CGuiView::DoGamePadAxisMotion(gamePad, axis, value);
}

bool CViewDrive1541DiskData::HasContextMenuItems()
{
	return false;
}

void CViewDrive1541DiskData::RenderContextMenuItems()
{
}

void CViewDrive1541DiskData::ActivateView()
{
	LOGG("CViewDrive1541DiskData::ActivateView()");
}

void CViewDrive1541DiskData::DeactivateView()
{
	LOGG("CViewDrive1541DiskData::DeactivateView()");
}
