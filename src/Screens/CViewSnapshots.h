#ifndef _VIEW_C64SNAPSHOTS_
#define _VIEW_C64SNAPSHOTS_

#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "CSlrKeyboardShortcuts.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include <list>

class CViewC64MenuItem;
class CByteBuffer;

class CSnapshotUpdateThread : public CSlrThread
{
public:
	CSnapshotUpdateThread();
	volatile long snapshotLoadedTime;
	volatile long snapshotUpdatedTime;
	virtual void ThreadRun(void *data);
};

class CViewSnapshots : public CGuiView, CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CSlrKeyboardShortcutCallback
{
public:
	CViewSnapshots(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewSnapshots();

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
	
	virtual void ActivateView();
	virtual void DeactivateView();

	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float tr;
	float tg;
	float tb;
	
	CSlrString *strHeader;
	
	CSlrString *strStoreSnapshotText;
	CSlrString *strStoreSnapshotKeys;
	CSlrString *strRestoreSnapshotText;
	CSlrString *strRestoreSnapshotKeys;
	
	CGuiViewMenu *viewMenu;
	virtual void MenuCallbackItemEntered(CGuiViewMenuItem *menuItem);


	CViewC64MenuItem *menuItemBack;

	CSlrKeyboardShortcut *kbsSaveSnapshot;
	CViewC64MenuItem *menuItemSaveSnapshot;
	CSlrKeyboardShortcut *kbsLoadSnapshot;
	CViewC64MenuItem *menuItemLoadSnapshot;

	
	CSlrKeyboardShortcut *kbsStoreSnapshot1;
	CSlrKeyboardShortcut *kbsStoreSnapshot2;
	CSlrKeyboardShortcut *kbsStoreSnapshot3;
	CSlrKeyboardShortcut *kbsStoreSnapshot4;
	CSlrKeyboardShortcut *kbsStoreSnapshot5;
	CSlrKeyboardShortcut *kbsStoreSnapshot6;
	CSlrKeyboardShortcut *kbsStoreSnapshot7;

	CSlrKeyboardShortcut *kbsRestoreSnapshot1;
	CSlrKeyboardShortcut *kbsRestoreSnapshot2;
	CSlrKeyboardShortcut *kbsRestoreSnapshot3;
	CSlrKeyboardShortcut *kbsRestoreSnapshot4;
	CSlrKeyboardShortcut *kbsRestoreSnapshot5;
	CSlrKeyboardShortcut *kbsRestoreSnapshot6;
	CSlrKeyboardShortcut *kbsRestoreSnapshot7;

	bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut);
	

	CByteBuffer **fullSnapshots;
	
	void QuickStoreFullSnapshot(int snapshotId);
	void QuickRestoreFullSnapshot(int snapshotId);
	
	std::list<CSlrString *> snapshotExtensions;
	std::list<CSlrString *> snapshotExtensionsC64;
	std::list<CSlrString *> snapshotExtensionsAtari800;
	std::list<CSlrString *> snapshotExtensionsNES;

	void OpenDialogLoadSnapshot();
	void LoadSnapshot(CSlrString *path, bool showMessage, CDebugInterface *debugInterface);
	void LoadSnapshot(CByteBuffer *byteBuffer, bool showMessage, CDebugInterface *debugInterface);
	void OpenDialogSaveSnapshot();
	void SaveSnapshot(CSlrString *path, CDebugInterface *debugInterface);

	CByteBuffer *snapshotBuffer;
	
//	CSlrString *pathToSnapshot;

	u8 openDialogFunction;
	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	
	
	void SwitchSnapshotsScreen();
	
	CGuiView *prevView;
	
	CSnapshotUpdateThread *updateThread;

	int debugRunModeWhileTakingSnapshot;
};

#endif //_VIEW_C64SNAPSHOTS_
