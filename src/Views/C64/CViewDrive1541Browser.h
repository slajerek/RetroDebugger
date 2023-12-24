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
class CViewDrive1541Browser;
class CDebugInterfaceVice;

enum ViewDrive1541FileD64DialogOperation : u8 {
	ViewDrive1541FileD64Dialog_None = 0,
	ViewDrive1541FileD64Dialog_InsertFile,
	ViewDrive1541FileD64Dialog_CreateNew
};

class CViewDrive1541FileD64RefreshDiskImageThread : public CSlrThread
{
public:
	CViewDrive1541FileD64RefreshDiskImageThread(CViewDrive1541Browser *viewDrive1541FileD64);
	virtual void ThreadRun(void *passData);
	
	CViewDrive1541Browser *viewDrive1541FileD64;
};

class CViewDrive1541Browser : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CSlrThread
{
public:
	CViewDrive1541Browser(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewDrive1541Browser();

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

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

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual void ActivateView();
	virtual void DeactivateView();

	CViewDrive1541FileD64RefreshDiskImageThread *threadRefreshDiskImage;
	
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

	std::list<CSlrString *> insertFileExtensions;
	std::list<CSlrString *> diskFileExtensions;

	ViewDrive1541FileD64DialogOperation dialogOperation;
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	
	void SwitchFileD64Screen();
	
	void StartBrowsingD64(char *fileName);
	void StartBrowsingD64(int deviceId);
	void StartSelectedDiskImageBrowsing();
	
	bool keepScrollPositionOnRefreshDiskImage;
	void RefreshDiskImageMenu();
	void RefreshInsertedDiskImage();
	
	void RefreshInsertedDiskImageAsync();
	virtual void ThreadRun(void *passData);

	void SetDiskImage(char *fullFilePath);
	void SetDiskImage(int deviceId);
	
	void StartFileEntry(DiskImageFileEntry *fileEntry, bool showLoadAddressInfo);
	
	void UpdateDriveDiskID();
	
	char *fullFilePath;
	
	CDiskImageD64 *diskImage;
	
	// like LOAD "*" + RUN
	void StartDiskPRGEntry(int entryNum, bool showLoadAddressInfo);
	
	char diskName[0x11];
	char diskId[0x08];
	
	//
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
};

class CViewDrive1541FileD64EntryItem : public CViewC64MenuItem
{
public:
	CViewDrive1541FileD64EntryItem(float height, CSlrString *str, float r, float g, float b);
	~CViewDrive1541FileD64EntryItem();
	
	virtual void RenderItem(float px, float py, float pz);
	
	bool canSelect;
	
	DiskImageFileEntry *fileEntry;
};

#endif //_VIEW_FILED64_
