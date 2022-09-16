//
// NOTE:
// This view will be removed. It is being refactored and moved to main menu bar instead.
//

extern "C" {
#include "c64model.h"
};

#include "SND_SoundEngine.h"
#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewSnapshots.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64SettingsStorage.h"
#include "CViewC64Screen.h"

#include "C64KeyboardShortcuts.h"
#include "CViewMemoryMap.h"

#include "CGuiMain.h"
#include "CViewMainMenu.h"

#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"

#define C64SNAPSHOT_MAGIC1		'S'

#define VIEWC64SNAPSHOTS_LOAD_SNAPSHOT	1
#define VIEWC64SNAPSHOTS_SAVE_SNAPSHOT	2

CViewSnapshots::CViewSnapshots(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewSnapshots";

	prevView = viewC64;

	font = viewC64->fontCBMShifted;
	fontScale = 3;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	snapshotExtensions.push_back(new CSlrString("snap"));
	snapshotExtensions.push_back(new CSlrString("vsf"));
	snapshotExtensions.push_back(new CSlrString("a8s"));
	snapshotExtensions.push_back(new CSlrString("sav"));

	snapshotExtensionsC64.push_back(new CSlrString("snap"));
	snapshotExtensionsC64.push_back(new CSlrString("vsf"));
	snapshotExtensionsAtari800.push_back(new CSlrString("a8s"));
	snapshotExtensionsNES.push_back(new CSlrString("sav"));

//	pathToSnapshot = NULL;
	
	strHeader = new CSlrString("Snapshots");
	
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;
	
	snapshotBuffer = NULL;
	
	const int numFullSnapshots = 6;
	fullSnapshots = new CByteBuffer *[numFullSnapshots];
	for (int i = 0; i < numFullSnapshots; i++)
	{
		fullSnapshots[i] = NULL;
	}
	
	/// menu
	viewMenu = new CGuiViewMenu(35, 57, -1, sizeX-70, sizeY-87, this);
	
	menuItemBack  = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< BACK"),
										 NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemBack);
	
//	LOGTODO("ATARI SNAPSHOTS SHORTCUTS");

	kbsSaveSnapshot = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Save snapshot", 's', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsSaveSnapshot);
		
	menuItemSaveSnapshot = new CViewC64MenuItem(fontHeight, new CSlrString("Save Snapshot"), kbsSaveSnapshot, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemSaveSnapshot);
	
	kbsSaveSnapshot = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Save snapshot", 's', false, false, true, false, this);
	kbsLoadSnapshot = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Load snapshot", 'd', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsLoadSnapshot);

	menuItemLoadSnapshot = new CViewC64MenuItem(fontHeight*2, new CSlrString("Load Snapshot"), kbsLoadSnapshot, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemLoadSnapshot);


	// ctrl+shift+1,2,3... store snapshot
	kbsStoreSnapshot1 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #1", '1', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot1);
	kbsStoreSnapshot2 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #2", '2', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot2);
	kbsStoreSnapshot3 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #3", '3', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot3);
	kbsStoreSnapshot4 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #4", '4', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot4);
	kbsStoreSnapshot5 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #5", '5', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot5);
	kbsStoreSnapshot6 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #6", '6', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot6);
	kbsStoreSnapshot7 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Store snapshot #7", '7', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsStoreSnapshot7);

	// ctrl+1,2,3,... restore snapshot
	kbsRestoreSnapshot1 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #1", '1', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot1);
	kbsRestoreSnapshot2 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #2", '2', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot2);
	kbsRestoreSnapshot3 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #3", '3', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot3);
	kbsRestoreSnapshot4 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #4", '4', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot4);
	kbsRestoreSnapshot5 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #5", '5', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot5);
	kbsRestoreSnapshot6 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #6", '6', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot6);
	kbsRestoreSnapshot7 = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Restore snapshot #7", '7', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsRestoreSnapshot7);

	//
	strStoreSnapshotText = new CSlrString("Quick store snapshot");
	
#if defined(MACOS)
	strStoreSnapshotKeys = new CSlrString("Shift+Cmd+1,2,3...");
#else
	strStoreSnapshotKeys = new CSlrString("Shift+Ctrl+1,2,3...");
#endif
	strRestoreSnapshotText = new CSlrString("Quick restore snapshot");

#if defined(MACOS)
	strRestoreSnapshotKeys = new CSlrString("Cmd+1,2,3...");
#else
	strRestoreSnapshotKeys = new CSlrString("Ctrl+1,2,3...");
#endif

	
	this->updateThread = new CSnapshotUpdateThread();
}

CViewSnapshots::~CViewSnapshots()
{
}

bool CViewSnapshots::ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *shortcut)
{
//	if (shortcut == KBFUN_SNAPSHOT_MENU)
//	{
//		SwitchSnapshotsScreen();
//		return true;
//	}
//	else
	if (shortcut == kbsSaveSnapshot)
	{
		OpenDialogSaveSnapshot();
	}
	else if (shortcut == kbsLoadSnapshot)
	{
		OpenDialogLoadSnapshot();
	}
	else if (shortcut == kbsStoreSnapshot1)
	{
		QuickStoreFullSnapshot(0);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot1)
	{
		QuickRestoreFullSnapshot(0);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot2)
	{
		QuickStoreFullSnapshot(1);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot2)
	{
		QuickRestoreFullSnapshot(1);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot3)
	{
		QuickStoreFullSnapshot(2);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot3)
	{
		QuickRestoreFullSnapshot(2);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot4)
	{
		QuickStoreFullSnapshot(3);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot4)
	{
		QuickRestoreFullSnapshot(3);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot5)
	{
		QuickStoreFullSnapshot(4);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot5)
	{
		QuickRestoreFullSnapshot(4);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot6)
	{
		QuickStoreFullSnapshot(5);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot6)
	{
		QuickRestoreFullSnapshot(5);
		return true;
	}
	else if (shortcut == kbsStoreSnapshot7)
	{
		QuickStoreFullSnapshot(6);
		return true;
	}
	else if (shortcut == kbsRestoreSnapshot7)
	{
		QuickRestoreFullSnapshot(6);
		return true;
	}
	
	return false;

}

void CViewSnapshots::QuickStoreFullSnapshot(int snapshotId)
{
//	if (viewC64->debugInterface->GetEmulatorType() != C64_EMULATOR_VICE)
//	{
//		if (fullSnapshots[snapshotId] == NULL)
//			fullSnapshots[snapshotId] = new CByteBuffer();
//		
//		viewC64->debugInterface->LockMutex();
//		fullSnapshots[snapshotId]->Rewind();
//		viewC64->debugInterface->SaveFullSnapshot(fullSnapshots[snapshotId]);
//		viewC64->debugInterface->UnlockMutex();
//	}
//	else
	
	char *fname = SYS_GetCharBuf();
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		
		if (!debugInterface->isRunning)
			continue;
		
		// TODO: generalize the snapshot file extension
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			sprintf(fname, "snapshot-%d.snap", snapshotId);
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
		{
			sprintf(fname, "snapshot-%d.a8s", snapshotId);
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_NESTOPIA)
		{
			sprintf(fname, "snapshot-%d.sav", snapshotId);
		}
		else
		{
			LOGError("Unknown emulator type %d", debugInterface->GetEmulatorType());
			SYS_ReleaseCharBuf(fname);
			return;
		}

		CSlrString *path = new CSlrString();
		path->Concatenate(gUTFPathToSettings);
		path->Concatenate(fname);
		
		path->DebugPrint("QuickStoreFullSnapshot: path=");
		
		this->SaveSnapshot(path, debugInterface);
		
		delete path;
	}
	SYS_ReleaseCharBuf(fname);

	char *buf = SYS_GetCharBuf();
	sprintf(buf, "Snapshot #%d stored", snapshotId+1);
	viewC64->ShowMessage(buf);
	SYS_ReleaseCharBuf(buf);
}

void CViewSnapshots::QuickRestoreFullSnapshot(int snapshotId)
{
//	if (viewC64->debugInterface->GetEmulatorType() != C64_EMULATOR_VICE)
//	{
//		char *buf = SYS_GetCharBuf();
//		
//		if (fullSnapshots[snapshotId] == NULL)
//		{
//			sprintf(buf, "No snapshot stored at #%d", snapshotId+1);
//			viewC64->ShowMessage(buf);
//			SYS_ReleaseCharBuf(buf);
//			return;
//		}
//		viewC64->debugInterface->LockMutex();
//		fullSnapshots[snapshotId]->Rewind();
//		fullSnapshots[snapshotId]->GetU8();
//		viewC64->debugInterface->LoadFullSnapshot(fullSnapshots[snapshotId]);
//		viewC64->debugInterface->UnlockMutex();
//		
//		sprintf(buf, "Snapshot #%d restored", snapshotId+1);
//		viewC64->ShowMessage(buf);
//		SYS_ReleaseCharBuf(buf);
//	}
//	else
	
	char *fname = SYS_GetCharBuf();
	
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (!debugInterface->isRunning)
			continue;
		
		// TODO: generalize the snapshot file extension
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			sprintf(fname, "snapshot-%d.snap", snapshotId);
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
		{
			sprintf(fname, "snapshot-%d.a8s", snapshotId);
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_NESTOPIA)
		{
			sprintf(fname, "snapshot-%d.sav", snapshotId);
		}
		else
		{
			LOGError("Unknown emulator type %d", debugInterface->GetEmulatorType());
			SYS_ReleaseCharBuf(fname);
			return;
		}
		
		CSlrString *path = new CSlrString();
		path->Concatenate(gUTFPathToSettings);
		path->Concatenate(fname);
		
		path->DebugPrint("QuickStoreFullSnapshot: path=");
		
		this->LoadSnapshot(path, false, debugInterface);
		delete path;
	}
	SYS_ReleaseCharBuf(fname);
}

void CViewSnapshots::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{
//	if (menuItem == menuItemSaveSnapshot)
//	{
//		OpenDialogSaveSnapshot();
//	}
//	else if (menuItem == menuItemLoadSnapshot)
//	{
//		OpenDialogLoadSnapshot();
//	}
//	else if (menuItem == menuItemBack)
//	{
//		guiMain->SetView(prevView);
//	}

}

void CViewSnapshots::OpenDialogSaveSnapshot()
{
	openDialogFunction = VIEWC64SNAPSHOTS_SAVE_SNAPSHOT;

//	if (viewC64->debugInterface->GetEmulatorType() != C64_EMULATOR_VICE)
//	{
//		// store snapshot to buffer immediately
//		if (snapshotBuffer != NULL)
//			delete snapshotBuffer;
//		
//		snapshotBuffer = new CByteBuffer();
//		
//		snapshotBuffer->PutByte(C64SNAPSHOT_MAGIC1);
//		viewC64->debugInterface->SaveFullSnapshot(snapshotBuffer);
//		
//		//	viewC64->theC64->SaveChipsSnapshot(snapshotBuffer);
//		
//		viewC64->debugInterface->UnlockMutex();
//	}
	
	// Idea for now is that we will open dialog only for first running emulator. This will be fixed in the future.
	
	// TODO: fix me, this will backup only one emulator's run mode
	// TODO: generalize me
	std::list<CSlrString *> *selectedSnapshotExtensions = NULL;
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (!debugInterface->isRunning)
			continue;
		
		debugRunModeWhileTakingSnapshot = debugInterface->GetDebugMode();
		debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
		
		LOGD("debugInterface->GetEmulatorType()=%d", debugInterface->GetEmulatorType());
		
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			selectedSnapshotExtensions = &snapshotExtensionsC64;
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
		{
			selectedSnapshotExtensions = &snapshotExtensionsAtari800;
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_NESTOPIA)
		{
			selectedSnapshotExtensions = &snapshotExtensionsNES;
		}
		break;
	}
	
	CSlrString *defaultFileName = new CSlrString("snapshot");
	
	CSlrString *windowTitle = new CSlrString("Save snapshot");
	viewC64->ShowDialogSaveFile(this, selectedSnapshotExtensions, defaultFileName, c64SettingsDefaultSnapshotsFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewSnapshots::OpenDialogLoadSnapshot()
{
	openDialogFunction = VIEWC64SNAPSHOTS_LOAD_SNAPSHOT;
	
	std::list<CSlrString *> *selectedSnapshotExtensions = NULL;
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (!debugInterface->isRunning)
			continue;
		
		debugRunModeWhileTakingSnapshot = debugInterface->GetDebugMode();
		debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
		
		LOGD("debugInterface->GetEmulatorType()=%d", debugInterface->GetEmulatorType());
		
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			selectedSnapshotExtensions = &snapshotExtensionsC64;
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
		{
			selectedSnapshotExtensions = &snapshotExtensionsAtari800;
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_NESTOPIA)
		{
			selectedSnapshotExtensions = &snapshotExtensionsNES;
		}
		break;
	}
	
	CSlrString *windowTitle = new CSlrString("Load snapshot");
	viewC64->ShowDialogOpenFile(this, selectedSnapshotExtensions, c64SettingsDefaultSnapshotsFolder, windowTitle);
	delete windowTitle;	
}

void CViewSnapshots::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGM("CViewSnapshots::SystemDialogFileOpenSelected");

	if (openDialogFunction == VIEWC64SNAPSHOTS_LOAD_SNAPSHOT)
	{
		CSlrString *ext = path->GetFileExtensionComponentFromPath();
		if (ext->CompareWith("snap"))
		{
			LoadSnapshot(path, true, viewC64->debugInterfaceC64);
		}
		else if (ext->CompareWith("a8s"))
		{
			LoadSnapshot(path, true, viewC64->debugInterfaceAtari);
		}
		else if (ext->CompareWith("sav"))
		{
			LoadSnapshot(path, true, viewC64->debugInterfaceNes);
		}
		
		if (c64SettingsDefaultSnapshotsFolder != NULL)
			delete c64SettingsDefaultSnapshotsFolder;
		
		c64SettingsDefaultSnapshotsFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		C64DebuggerStoreSettings();
		
		// TODO: fixme, this will restore only last iterated debug interface
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (!debugInterface->isRunning)
				continue;
			
			debugInterface->SetDebugMode(debugRunModeWhileTakingSnapshot);
		}

	}
}

void CViewSnapshots::SystemDialogFileSaveSelected(CSlrString *path)
{
	if (openDialogFunction == VIEWC64SNAPSHOTS_SAVE_SNAPSHOT)
	{
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			LOGTODO("CViewSnapshots::SystemDialogFileSaveSelected: this is not implemented properly, this will store only one running emulator, not all");
			CDebugInterface *debugInterface = *it;
			if (!debugInterface->isRunning)
			{
				continue;
			}
			
			SaveSnapshot(path, debugInterface);
			break;
		}

		if (c64SettingsDefaultSnapshotsFolder != NULL)
			delete c64SettingsDefaultSnapshotsFolder;
		
		c64SettingsDefaultSnapshotsFolder = path->GetFilePathWithoutFileNameComponentFromPath();
		C64DebuggerStoreSettings();
		
		// TODO: fixme, this will restore only last iterated debug interface
		for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
		{
			CDebugInterface *debugInterface = *it;
			if (!debugInterface->isRunning)
				continue;
			
			debugInterface->SetDebugMode(debugRunModeWhileTakingSnapshot);
		}
	}
}

void CViewSnapshots::SystemDialogFileSaveCancelled()
{
	if (snapshotBuffer != NULL)
	{
		delete snapshotBuffer;
		snapshotBuffer = NULL;
	}
	
	// TODO: fixme, this will restore only last iterated debug interface
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		if (!debugInterface->isRunning)
			continue;

		debugInterface->SetDebugMode(debugRunModeWhileTakingSnapshot);
	}
}

void CViewSnapshots::LoadSnapshot(CSlrString *path, bool showMessage, CDebugInterface *debugInterface)
{
	path->DebugPrint("CViewSnapshots::LoadSnapshot, path=");

	// TODO: support UTF paths
	char *asciiPath = path->GetStdASCII();
	
//	if (viewC64->debugInterface->GetEmulatorType() != EMULATOR_TYPE_C64_VICE)
//	{
//		CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath, SLR_FILE_MODE_READ);
//		CByteBuffer *buffer = new CByteBuffer(file, false);
//		delete file;
//		
//		LoadSnapshot(buffer, showMessage);
//		
//		delete buffer;
//	}
//	else
	
	bool ret = debugInterface->LoadFullSnapshot(asciiPath);
	
	// TODO: generalize this
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		viewC64->viewC64MemoryMap->ClearExecuteMarkers();
		viewC64->viewDrive1541MemoryMap->ClearExecuteMarkers();
	}
	else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
	{
		viewC64->viewAtariMemoryMap->ClearExecuteMarkers();
	}
	else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_NESTOPIA)
	{
		viewC64->viewNesMemoryMap->ClearExecuteMarkers();
	}

	if (showMessage)
	{
		if (ret == true)
		{
			//viewC64->ShowMessage("Snapshot restored");
		}
		else
		{
			viewC64->ShowMessage("Snapshot file is not supported");
			return;
		}
	}

	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		// workaround for C64 Vice snapshot loading when machine is different than current
		debugInterface->LockIoMutex();
		if (updateThread->isRunning == false)
		{
			updateThread->snapshotLoadedTime = SYS_GetCurrentTimeInMillis();
			SYS_StartThread(updateThread);
		}
		else
		{
			updateThread->snapshotLoadedTime = SYS_GetCurrentTimeInMillis();
		}
		debugInterface->UnlockIoMutex();
	}
	
	delete asciiPath;
}

void CViewSnapshots::LoadSnapshot(CByteBuffer *buffer, bool showMessage, CDebugInterface *debugInterface)
{
	LOGTODO("LoadSnapshot: loading snapshot from buffer is not supported");
//	u8 magic = buffer->GetByte();
//	if (magic != C64SNAPSHOT_MAGIC1)
//	{
//		if (showMessage)
//			viewC64->ShowMessage("Snapshot file is corrupted");
//		return;
//	}
//	
//	u8 snapshotType = buffer->GetByte();
//	if (snapshotType != C64_SNAPSHOT)
//	{
//		if (showMessage)
//			viewC64->ShowMessage("File version not supported");
//		return;
//	}
//	
//	viewC64->debugInterface->LockMutex();
//	bool ret = viewC64->debugInterface->LoadFullSnapshot(buffer);
//	viewC64->debugInterface->UnlockMutex();
//	
//	if (showMessage)
//	{
//		if (ret == true)
//		{
//			//viewC64->ShowMessage("Snapshot restored");
//		}
//		else
//		{
//			viewC64->ShowMessage("Snapshot file is not supported");
//		}
//	}
}

void CViewSnapshots::SaveSnapshot(CSlrString *path, CDebugInterface *debugInterface)
{
	path->DebugPrint("CViewSnapshots::SaveSnapshot, path=");

//	if (viewC64->debugInterface->GetEmulatorType() != C64_EMULATOR_VICE)
//	{
//		if (snapshotBuffer == NULL)
//		{
//			LOGError("CViewSnapshots::SaveSnapshot: snapshotBuffer is NULL");
//			return;
//		}
//		
//		char *asciiPath = path->GetStdASCII();
//		
//		CSlrFileFromOS *file = new CSlrFileFromOS(asciiPath, SLR_FILE_MODE_WRITE);
//		snapshotBuffer->storeToFileNoHeader(file);
//		
//		delete snapshotBuffer;
//		snapshotBuffer = NULL;
//		
//		delete file;
//		delete asciiPath;
//	}
//	else
	
	char *asciiPath = path->GetStdASCII();
	
	debugInterface->SaveFullSnapshot(asciiPath);
	
	delete asciiPath;
	
	viewC64->ShowMessage("Snapshot saved");
}



void CViewSnapshots::SystemDialogFileOpenCancelled()
{
}


void CViewSnapshots::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewSnapshots::Render()
{
//	guiMain->fntConsole->BlitText("CViewSnapshots", 0, 0, 0, 11, 1.0);

	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
	
	float sb = 20;
	float gap = 15;
	
	float lSizeY = 3;

	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSize = 3;
	
	float scrx = sb;
	float scry = sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = scrsx/2.0f + sb;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy,
						viewC64->colorsTheme->colorBackgroundR,
						viewC64->colorsTheme->colorBackgroundG,
						viewC64->colorsTheme->colorBackgroundB, 1.0);
	
	
	float px = scrx + gap;
	float py = scry + 5;// + gap;
	
	font->BlitTextColor(strHeader, cx, py, -1, 3.0f, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
	py += 6.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSize, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;
	
	viewMenu->Render();
	
	// temporary just print text here
	py += fontHeight;
	py += fontHeight;
	py += fontHeight;
	py += fontHeight;
	font->BlitTextColor(strStoreSnapshotText, px, py, -1, 3.0f, tr, tg, tb, 1);
	font->BlitTextColor(strStoreSnapshotKeys, px + 510, py, -1, 3.0f, 0.5, 0.5, 0.5, 1, FONT_ALIGN_RIGHT);
	py += fontHeight;
	font->BlitTextColor(strRestoreSnapshotText, px, py, -1, 3.0f, tr, tg, tb, 1);
	font->BlitTextColor(strRestoreSnapshotKeys, px + 510, py, -1, 3.0f, 0.5, 0.5, 0.5, 1, FONT_ALIGN_RIGHT);
	
	
	CGuiView::Render();
}

void CViewSnapshots::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewSnapshots::DoTap(float x, float y)
{
	LOGG("CViewSnapshots::DoTap:  x=%f y=%f", x, y);
	
	if (viewMenu->DoTap(x, y))
		return true;

	return CGuiView::DoTap(x, y);
}

bool CViewSnapshots::DoFinishTap(float x, float y)
{
	LOGG("CViewSnapshots::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewSnapshots::DoDoubleTap(float x, float y)
{
	LOGG("CViewSnapshots::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewSnapshots::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewSnapshots::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewSnapshots::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewSnapshots::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewSnapshots::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewSnapshots::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewSnapshots::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewSnapshots::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewSnapshots::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewSnapshots::SwitchSnapshotsScreen()
{
//	if (guiMain->currentView == this)
//	{
//		viewC64->ShowMainScreen();
//	}
//	else
//	{
//		guiMain->SetView(this);
//	}
}

bool CViewSnapshots::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
	
	if (keyCode == MTKEY_BACKSPACE)
	{
		guiMain->SetView(prevView);
		return true;
	}
	
	if (viewMenu->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	if (keyCode == MTKEY_ESC)
	{
		SwitchSnapshotsScreen();
		return true;
	}


	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewSnapshots::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (viewMenu->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewSnapshots::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewSnapshots::ActivateView()
{
	LOGG("CViewSnapshots::ActivateView()");
	
	prevView = guiMain->currentView;
	
	viewC64->ShowMouseCursor();
}

void CViewSnapshots::DeactivateView()
{
	LOGG("CViewSnapshots::DeactivateView()");
}

CSnapshotUpdateThread::CSnapshotUpdateThread()
{
	this->ThreadSetName("CSnapshotUpdateThread");
	
	this->snapshotLoadedTime = 0;
	this->snapshotUpdatedTime = -1;
}

// workaround for C64 Vice snapshot loading when machine is different than current
void CSnapshotUpdateThread::ThreadRun(void *data)
{
	LOGD("CSnapshotUpdateThread::ThreadRun");
	
	// come back t0 me to me
	SYS_Sleep(1000);
	
	viewC64->debugInterfaceC64->LockIoMutex();
	
	while (this->snapshotUpdatedTime != this->snapshotLoadedTime)
	{
		this->snapshotUpdatedTime = this->snapshotLoadedTime;
		
		viewC64->debugInterfaceC64->UnlockIoMutex();
		
		while (viewC64->debugInterfaceC64->GetC64MachineType() == MACHINE_TYPE_LOADING_SNAPSHOT)
		{
			SYS_Sleep(100);
		}
		
		SYS_Sleep(100);
		
		if (viewC64->debugInterfaceC64->GetC64MachineType() == MACHINE_TYPE_UNKNOWN)
		{
			// failed to load snapshot, make a full reset, this will likely crash
			int model = viewC64->debugInterfaceC64->GetC64ModelType();
			
			if (model == 0)
			{
				viewC64->debugInterfaceC64->SetC64ModelType(1);
			}
			else
			{
				viewC64->debugInterfaceC64->SetC64ModelType(0);
			}
			
			if (model == C64MODEL_UNKNOWN)
			{
				model = 0;
			}
			
			SYS_Sleep(200);
			
			viewC64->debugInterfaceC64->SetC64ModelType(model);
			
			SYS_Sleep(200);

			viewC64->debugInterfaceC64->HardReset();
		}
		
		// perform update
		viewC64->viewC64Screen->UpdateRasterCrossFactors();
		
		viewC64->debugInterfaceC64->LockIoMutex();
	}
	
	viewC64->debugInterfaceC64->UnlockIoMutex();
	
	LOGD("CSnapshotUpdateThread::ThreadRun finished");
}

