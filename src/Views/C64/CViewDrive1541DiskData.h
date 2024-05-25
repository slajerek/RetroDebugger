#ifndef _CViewDrive1541DiskData_h_
#define _CViewDrive1541DiskData_h_

#include "CGuiView.h"
#include "CDiskImageD64.h"
#include "imgui_memory_editor.h"

extern "C" {
struct disk_image_s;
typedef struct disk_image_s disk_image_t;
}

class CViewDrive1541DiskData : public CGuiView
{
public:
	CViewDrive1541DiskData(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewDrive1541DiskData();

	virtual void Render();
	virtual void Render(float posX, float posY);
	//virtual void Render(float posX, float posY, float sizeX, float sizeY);
	virtual void RenderImGui();

	virtual void DoLogic();

	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool DoRightClick(float x, float y);
	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	// not touched move = mouse move with not clicked button
	virtual bool DoNotTouchedMove(float x, float y);

	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual bool DoGamePadButtonDown(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadButtonUp(CGamePad *gamePad, u8 button);
	virtual bool DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value);

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	//
	disk_image_t *diskImage;

	MemoryEditor memoryEditorSector;
	MemoryEditor memoryEditorGCR;
};

#endif //_GUI_VIEW_DUMMY_
