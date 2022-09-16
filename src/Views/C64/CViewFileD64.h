#ifndef _VIEW_FILED64_
#define _VIEW_FILED64_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "CDiskImageD64.h"
#include "SYS_Threading.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class CViewC64MenuItemOption;

class CViewC64FileD64 : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CSlrThread
{
public:
	CViewC64FileD64(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewC64FileD64();

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
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual void ActivateView();
	virtual void DeactivateView();

	CGuiButton *btnDone;
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);

	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float tr;
	float tg;
	float tb;
	
	CSlrString *strHeader;
	CSlrString *strHeader2;
	CSlrString *strHeader3;
	

	CGuiViewMenu *viewMenu;
	virtual void MenuCallbackItemEntered(CGuiViewMenuItem *menuItem);
	virtual void MenuCallbackItemChanged(CGuiViewMenuItem *menuItem);

	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
//	virtual void SystemDialogFileSaveSelected(CSlrString *path);
//	virtual void SystemDialogFileSaveCancelled();
	
	void SwitchFileD64Screen();
	
	void StartBrowsingD64(char *fileName);
	void StartBrowsingD64(int deviceId);
	void StartSelectedDiskImageBrowsing();
	
	void RefreshDiskImageMenu();
	void RefreshInsertedDiskImage();
	
	void RefreshInsertedDiskImageAsync();
	virtual void ThreadRun(void *passData);

	void SetDiskImage(char *fileName);
	void SetDiskImage(int deviceId);
	
	void StartFileEntry(DiskImageFileEntry *fileEntry, bool showLoadAddressInfo);
	
	void UpdateDriveDiskID();
	
	char *fullFilePath;
	
	CDiskImageD64 *diskImage;
	
	// like LOAD "*" + RUN
	void StartDiskPRGEntry(int entryNum, bool showLoadAddressInfo);
};

class CViewFileD64EntryItem : public CViewC64MenuItem
{
public:
	CViewFileD64EntryItem(float height, CSlrString *str, float r, float g, float b);
	~CViewFileD64EntryItem();
	
	virtual void RenderItem(float px, float py, float pz);
	
	bool canSelect;
	
	DiskImageFileEntry *fileEntry;
};

#endif //_VIEW_FILED64_
