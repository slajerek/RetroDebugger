#include "CViewC64.h"
#include "CViewDrive1541Browser.h"
#include "CColorsTheme.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "CSlrDate.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CViewAbout.h"
#include "CDebugInterfaceC64.h"
#include "CViewDataMap.h"
#include "C64SettingsStorage.h"
#include "CGuiMain.h"

#include "CDiskImageD64.h"
#include "CLayoutParameter.h"

extern "C" {
struct disk_image_s;
typedef struct disk_image_s disk_image_t;

disk_image_t *c64d_get_drive_disk_image(int driveId);
};

CViewDrive1541Browser::CViewDrive1541Browser(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	int driveId = 0;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	font = viewC64->fontDefaultCBMShifted;
	fontScale = 3;
	fontHeight = font->GetCharHeight('@', fontScale);
	hasManualFontScale = false;
	AddLayoutParameter(new CLayoutParameterFloat("Font Scale", &fontScale));

	strHeader = NULL;
	strHeader2 = NULL;
	
	this->diskImage = NULL;
	diskName[0] = 0;
	diskId[0] = 0;
	
	insertFileExtensions.push_back(new CSlrString("prg"));
//	insertFileExtensions.push_back(new CSlrString("seq"));

	diskFileExtensions.push_back(new CSlrString("d64"));
	
	dialogOperation = ViewDrive1541FileD64Dialog_None;
	
	/// menu
	float menuMarginX = fontScale * 12.0f;
	float menuMarginTop = fontScale * 19.0f;
	float menuWidthReduction = fontScale * 24.0f;
	float menuHeightReduction = fontScale * 26.0f;
	viewMenu = new CGuiViewMenu(posX+menuMarginX, posY+menuMarginTop, -1, sizeX-menuWidthReduction, sizeY-menuHeightReduction, this);

	// TODO: get from media file?
	this->fullFilePath = "";
	//	CSlrString *filePath = new CSlrString(fullFilePath);
	//	CSlrString *fileName = filePath->GetFileNameComponentFromPath();
	
	if (strHeader != NULL)
		delete strHeader;
	
	strHeader = new CSlrString("Device #8");
	
	this->diskImage = new CDiskImageD64((CDebugInterfaceVice*)viewC64->debugInterfaceC64, driveId);

	
	threadRefreshDiskImage = new CViewDrive1541FileD64RefreshDiskImageThread(this);
	SYS_StartThread(threadRefreshDiskImage);
	
//	kbsMainMenuScreen = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Main menu screen", MTKEY_F9, false, false, false);
//	viewC64->keyboardShortcuts->AddShortcut(kbsMainMenuScreen);
	
//	viewMenu->SelectMenuItem(menuItemInsertD64);
	
	
//	std::list<u32> zones;
//	zones.push_back(KBZONE_GLOBAL);
//	CSlrKeyboardShortcut *sr = viewC64->keyboardShortcuts->FindShortcut(zones, '8', false, true, false);
	
	//LOGD("---done");
}

CViewDrive1541Browser::~CViewDrive1541Browser()
{
}

void CViewDrive1541Browser::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	bool widthChanged = (fabs(sizeX - this->sizeX) > 0.5f);

	if (hasManualFontScale && !widthChanged)
	{
		// Width unchanged (startup/layout restore): keep manually set font scale
	}
	else
	{
		// Width-only autoscale: wider view = larger font, height just shows more entries
		// A C64 directory line is ~30 chars wide; scale font so text fills the view
		float charWidth = font->GetCharWidth('@', 1.0f);
		float contentWidth = 27.0f * charWidth + 24.0f; // ~28 char line + scaled margins
		fontScale = sizeX / contentWidth;

		if (widthChanged)
			hasManualFontScale = false;
	}

	fontHeight = font->GetCharHeight('@', fontScale);

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);

	float menuMarginX = fontScale * 12.0f;
	float menuMarginTop = fontScale * 19.0f;
	float menuWidthReduction = fontScale * 24.0f;
	float menuHeightReduction = fontScale * 26.0f;
	viewMenu->SetPosition(posX+menuMarginX, posY+menuMarginTop, posZ, sizeX-menuWidthReduction, sizeY-menuHeightReduction);

	// Update existing menu item heights to match new fontScale
	for (std::list<CGuiViewMenuItem *>::iterator it = viewMenu->menuItems.begin();
		 it != viewMenu->menuItems.end(); it++)
	{
		(*it)->height = fontHeight;
	}
}

void CViewDrive1541Browser::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	if (layoutParameter != NULL)
	{
		hasManualFontScale = true;
	}
	else
	{
		float charWidth = font->GetCharWidth('@', 1.0f);
		float contentWidth = 27.0f * charWidth + 24.0f;
		float autoFontScale = sizeX / contentWidth;

		if (fabs(fontScale - autoFontScale) > 0.01f)
		{
			hasManualFontScale = true;
		}
	}

	fontHeight = font->GetCharHeight('@', fontScale);

	// Update menu item heights
	for (std::list<CGuiViewMenuItem *>::iterator it = viewMenu->menuItems.begin();
		 it != viewMenu->menuItems.end(); it++)
	{
		(*it)->height = fontHeight;
	}

	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewDrive1541Browser::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{
	CViewDrive1541FileD64EntryItem *fileEntryMenuItem = (CViewDrive1541FileD64EntryItem *)menuItem;
	DiskImageFileEntry *fileEntry = fileEntryMenuItem->fileEntry;

	this->StartFileEntry(fileEntry, true);
}

void CViewDrive1541Browser::StartFileEntry(DiskImageFileEntry *fileEntry, bool showLoadAddressInfo)
{
	LOGD("CViewDrive1541FileD64::StartFileEntry");
	
	if (fileEntry != NULL && fileEntry->fileType == 0x02) // && fileEntry->fileSize > 0)
	{
		// load entry
		CByteBuffer *byteBuffer = new CByteBuffer();
		
		bool readed = diskImage->ReadEntry(fileEntry, byteBuffer);
		if (readed == false)
		{
			viewC64->ShowMessageError("Load error");
			delete byteBuffer;
			return;
		}

		UpdateDriveDiskID();
		
		// load the PRG
		bool ret = viewC64->mainMenuHelper->LoadPRG(byteBuffer, true, showLoadAddressInfo, false);
		
		if (ret == false)
		{
			viewC64->ShowMessageError("Load error");
		}
		// note: this is handled by LoadPRG asynchronously
//		else
//		{
//			viewC64->ShowMainScreen();
//		}
		
		delete byteBuffer;
		return;
	}

}

extern "C" {
void c64d_set_drive_disk_memory(int driveId, BYTE *diskId, unsigned int track, unsigned int sector);
void c64d_set_drive_half_track(int driveId, int halfTrack);
};

// TODO: move me
void CViewDrive1541Browser::UpdateDriveDiskID()
{
	LOGD("CViewDrive1541FileD64::UpdateDriveDiskID, diskImage=%x", diskImage);
	if (diskImage)
	{
		// set correct disk ID to let 1541 ROM not throw 29, 'disk id mismatch'
		// see $F3F6 in 1541 ROM: http://unusedino.de/ec64/technical/misc/c1541/romlisting.html#FDD3
		//
		LOGD("...diskId= %02x %02x", diskImage->diskId[2], diskImage->diskId[3]);
		BYTE diskId[2];
		diskId[0] = diskImage->diskId[2];
		diskId[1] = diskImage->diskId[3];
		c64d_set_drive_disk_memory(0, diskId, 6, 17);
		
//		NOTE: this breaks Samar NGC1277 100% loader and it seems is not required actually as other loaders work ok
		//c64d_set_drive_half_track(0, 12);

		// NOTE: this is handled by c64d_set_drive_disk_memory
//		viewC64->debugInterfaceC64->SetByte1541(0x0012, diskImage->diskId[2]);
//		viewC64->debugInterfaceC64->SetByte1541(0x0013, diskImage->diskId[3]);
//		
//		viewC64->debugInterfaceC64->SetByte1541(0x0016, diskImage->diskId[2]);
//		viewC64->debugInterfaceC64->SetByte1541(0x0017, diskImage->diskId[3]);
	}
	LOGD("CViewDrive1541FileD64::UpdateDriveDiskID completed");
}

void CViewDrive1541Browser::MenuCallbackItemChanged(CGuiViewMenuItem *menuItem)
{
	LOGD("CViewDrive1541FileD64::MenuCallbackItemChanged");
}



void CViewDrive1541Browser::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewDrive1541Browser::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewDrive1541Browser::Render()
{
	//LOGD("CViewDrive1541FileD64::Render");
	
//	viewC64->fontDisassembly->BlitText("CViewDrive1541FileD64", 0, 0, 0, 11, 1.0);

	BlitFilledRectangle(posX, posY, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
		
	float sb = fontScale * 6.7f;
	float gap = fontScale * 3.3f;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextB;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = fontScale * 1.0f;
	
	float scrx = posX+sb;
	float scry = posY+sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = posX+scrsx/2.0f + sb;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy,
						viewC64->colorsTheme->colorBackgroundR,
						viewC64->colorsTheme->colorBackgroundG,
						viewC64->colorsTheme->colorBackgroundB, 1.0);
	
	float px = scrx + gap;
	float py = scry + gap;
	
	font->BlitTextColor(strHeader, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
//	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
//	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;

	if (diskImage->IsDiskAttached())
	{
		viewMenu->Render();
	}
	// just leave empty
//	else
//	{
//		// disk is not attached
//		font->BlitTextColor("DISK NOT ATTACHED", px, py, -1, fontScale, tr, tg, tb, 1);
//	}
	
	CGuiView::Render();
}

void CViewDrive1541Browser::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewDrive1541Browser::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewDrive1541Browser::ButtonPressed(CGuiButton *button)
{
	/*
	if (button == btnDone)
	{
		guiMain->SetView((CGuiView*)guiMain->viewMainEditor);
		GUI_SetPressConsumed(true);
		return true;
	}
	*/
	return false;
}

//@returns is consumed
bool CViewDrive1541Browser::DoTap(float x, float y)
{
	LOGG("CViewDrive1541FileD64::DoTap:  x=%f y=%f", x, y);
	
	guiMain->LockMutex();
	if (viewMenu->DoTap(x, y))
	{
		guiMain->UnlockMutex();
		return true;
	}
	guiMain->UnlockMutex();

	return CGuiView::DoTap(x, y);
}

bool CViewDrive1541Browser::DoFinishTap(float x, float y)
{
	LOGG("CViewDrive1541FileD64::DoFinishTap: %f %f", x, y);

	guiMain->LockMutex();
	if (viewMenu->DoFinishTap(x, y))
	{
		guiMain->UnlockMutex();
		return true;
	}
	guiMain->UnlockMutex();

	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewDrive1541Browser::DoDoubleTap(float x, float y)
{
	LOGG("CViewDrive1541FileD64::DoDoubleTap:  x=%f y=%f", x, y);
	
	guiMain->LockMutex();
	if (viewMenu->DoDoubleTap(x, y))
	{
		guiMain->UnlockMutex();
		return true;
	}
	guiMain->UnlockMutex();

	return CGuiView::DoDoubleTap(x, y);
}

bool CViewDrive1541Browser::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewDrive1541FileD64::DoFinishTap: %f %f", x, y);
	
	guiMain->LockMutex();
	if (viewMenu->DoFinishDoubleTap(x, y))
	{
		guiMain->UnlockMutex();
		return true;
	}
	guiMain->UnlockMutex();

	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewDrive1541Browser::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewDrive1541Browser::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewDrive1541Browser::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewDrive1541Browser::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewDrive1541Browser::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewDrive1541Browser::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewDrive1541Browser::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewDrive1541Browser::StartBrowsingD64(char *fullFilePath)
{
	SYS_FatalExit("StartBrowsingD64 NOT SUPPORTED");
	this->SetDiskImage(fullFilePath);
	
	this->StartSelectedDiskImageBrowsing();
}

void CViewDrive1541Browser::SetDiskImage(char *fullFilePath)
{
	SYS_FatalExit("SetDiskImage NOT SUPPORTED");
	if (this->diskImage != NULL)
	{
		delete this->diskImage;
		this->diskImage = NULL;
	}
	
	this->fullFilePath = fullFilePath;
	
	CSlrString *filePath = new CSlrString(fullFilePath);
	
	if (strHeader != NULL)
		delete strHeader;
	
	strHeader = filePath->GetFileNameComponentFromPath();
	
	CSlrFileFromOS *file = new CSlrFileFromOS(fullFilePath);
	
	if (file->Exists() == false)
	{
		LOGError("CViewDrive1541FileD64::StartBrowsingD64: file '%s' does not exist", fullFilePath);
		return;
	}
	delete file;
	
	//CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	this->diskImage = new CDiskImageD64(fullFilePath);
}

void CViewDrive1541Browser::StartBrowsingD64(int deviceId)
{
	this->SetDiskImage(deviceId);

	if (this->diskImage->IsDiskAttached())
	{
		this->StartSelectedDiskImageBrowsing();
	}
	else
	{
		viewC64->ShowMessageError("No disk");
	}
}

void CViewDrive1541Browser::SetDiskImage(int deviceId)
{
	
//	if (this->diskImage != NULL)
//	{
//		delete this->diskImage;
//		this->diskImage = NULL;
//	}

	this->diskImage->RefreshImage();
}

void CViewDrive1541Browser::StartSelectedDiskImageBrowsing()
{
	this->SetDiskImage(0);
	this->RefreshInsertedDiskImage();
}

void CViewDrive1541Browser::RefreshInsertedDiskImage()
{
	LOGD("CViewDrive1541FileD64::RefreshInsertedDiskImage");

	this->RefreshDiskImageMenu();
}

void CViewDrive1541Browser::RefreshInsertedDiskImageAsync()
{
	LOGD("CViewDrive1541FileD64::RefreshInsertedDiskImageAsync");
	if (this->isRunning)
		return;
	
	SYS_StartThread(this);
}

void CViewDrive1541Browser::ThreadRun(void *passData)
{
	LOGD("CViewDrive1541FileD64::ThreadRun");
	this->RefreshInsertedDiskImage();
}

void CViewDrive1541Browser::RefreshDiskImageMenu()
{
	LOGD("CViewDrive1541FileD64::RefreshDiskImageMenu");
	
	if (this->diskImage == NULL)
	{
		LOGError("CViewDrive1541FileD64::RefreshDiskImageMenu: no disk image");
		return;
	}
	
	guiMain->LockMutex();
	debugInterfaceVice->LockMutex();
	
	//
	/// colors
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;
	
	unsigned long targetScrollPosition = 0;
	if (this->viewMenu != NULL)
	{
		if (keepScrollPositionOnRefreshDiskImage)
		{
			if (this->viewMenu->menuItems.size() > 0)
			{
				targetScrollPosition = std::distance(this->viewMenu->menuItems.begin(), this->viewMenu->selectedItem);
//				LOGD("size=%d", this->viewMenu->menuItems.size());
			}
			
			LOGD("targetScrollPosition=%d", targetScrollPosition);
		}
		delete this->viewMenu;
	}
	
	float menuMarginX = fontScale * 12.0f;
	float menuMarginTop = fontScale * 19.0f;
	float menuWidthReduction = fontScale * 24.0f;
	float menuHeightReduction = fontScale * 26.0f;
	viewMenu = new CGuiViewMenu(posX+menuMarginX, posY+menuMarginTop, -1, sizeX-menuWidthReduction, sizeY-menuHeightReduction, this);
	
	if (diskImage->IsDiskAttached() == false)
	{
		debugInterfaceVice->UnlockMutex();
		guiMain->UnlockMutex();
		return;
	}

	CViewDrive1541FileD64EntryItem *menuItem = NULL;
	
	// header
	CSlrString *strD64Header = new CSlrString("0 ");	
	
	u16 chr = '\"' | 0x80;
	strD64Header->Concatenate(chr);
	
	memset(diskName, 0, 0x11);
	memset(diskId, 0, 0x08);
	for (int i = 0; i < 0x10; i++)
	{
//		LOGD("diskImage->diskName[%d]=%02x (%c)", i, diskImage->diskName[i], diskImage->diskName[i]);
		
		chr = diskImage->diskName[i];
		
		if (chr < 0x7F)
		{
			diskName[i] = chr;
		}
		else
		{
			diskName[i] = '?';
		}

		if (chr == 0xA0)
			diskName[i] = 0;
		
		if (chr == 0x14)
		{
			diskName[i] = 0;
			strD64Header->RemoveLastCharacter();
			continue;
		}
		
		chr = ConvertPetsciiToScreenCode(chr);

		//LOGD("   chr=%02x (%c)", chr, chr);

		chr |= 0x80;
		
		strD64Header->Concatenate(chr);
	}
	
	chr = '\"' | 0x80;
	strD64Header->Concatenate(chr);
	
	int j = 0;
	for (int i = 1; i < 0x07; i++)
	{
//		LOGD("diskImage->diskId[%d]=%02x (%c)", i, diskImage->diskId[i], diskImage->diskId[i]);

		chr = diskImage->diskId[i];

		if (i != 1)
		{
			if (chr < 0x7F)
			{
				diskId[j] = chr;
			}
			else
			{
				diskId[j] = '?';
			}
			
			if (chr == 0xA0)
				diskId[j] = 0;
			
			j++;
		}
		
		if (chr == 0x14)
		{
			diskId[j] = 0;
			strD64Header->RemoveLastCharacter();
			continue;
		}
		
		chr = ConvertPetsciiToScreenCode(chr);
		
		chr |= 0x80;
		
		strD64Header->Concatenate(chr);
	}
	
//	LOGD("diskName='%s' diskId='%s'", diskName, diskId);
	
	menuItem = new CViewDrive1541FileD64EntryItem(fontHeight, strD64Header, tr, tg, tb, this);
	menuItem->canSelect = false;
	viewMenu->AddMenuItem(menuItem);
	
	char buf[128];

	for (std::vector<DiskImageFileEntry *>::iterator it = diskImage->fileEntries.begin(); it != diskImage->fileEntries.end(); it++)
	{
		DiskImageFileEntry *fileEntry = *it;
		
		// TODO: check who is adding these NULLs
		if (!fileEntry)
			continue;
	
		CSlrString *strFileEntry = new CSlrString();
		
		sprintf(buf, "%-4d \"", fileEntry->fileSize);
		strFileEntry->Concatenate(buf);

		for (int i = 0; i < 0x10; i++)
		{
			chr = fileEntry->fileName[i];
			
			if (chr == 0x14)
			{
				strFileEntry->RemoveLastCharacter();
				continue;
			}
			
			chr = ConvertPetsciiToScreenCode(chr);
			strFileEntry->Concatenate(chr);
		}
		
		if (fileEntry->fileType == 0x00)
		{
			// DEL
			strFileEntry->Concatenate("\" ");
			strFileEntry->Concatenate((u16)0x04);
			strFileEntry->Concatenate((u16)0x05);
			strFileEntry->Concatenate((u16)0x0C);
		}
		else if (fileEntry->fileType == 0x01)
		{
			// SEQ
			strFileEntry->Concatenate("\" ");
			strFileEntry->Concatenate((u16)0x13);
			strFileEntry->Concatenate((u16)0x05);
			strFileEntry->Concatenate((u16)0x11);
		}
		else if (fileEntry->fileType == 0x02)
		{
			// PRG
			strFileEntry->Concatenate("\" ");
			strFileEntry->Concatenate((u16)0x10);
			strFileEntry->Concatenate((u16)0x12);
			strFileEntry->Concatenate((u16)0x07);
		}
		else if (fileEntry->fileType == 0x03)
		{
			// USR
			strFileEntry->Concatenate("\" ");
			strFileEntry->Concatenate((u16)0x15);
			strFileEntry->Concatenate((u16)0x13);
			strFileEntry->Concatenate((u16)0x12);
		}
		else if (fileEntry->fileType == 0x04)
		{
			// REL
			strFileEntry->Concatenate("\" ");
			strFileEntry->Concatenate((u16)0x12);
			strFileEntry->Concatenate((u16)0x05);
			strFileEntry->Concatenate((u16)0x0C);
		}
		else
		{
			// ?
			strFileEntry->Concatenate("\" ");
			strFileEntry->Concatenate((u16)0x3F);
		}

//		strFileEntry->DebugPrint("DiskImageFileEntry: ");
		
		menuItem = new CViewDrive1541FileD64EntryItem(fontHeight, strFileEntry, tr, tg, tb, this);
		if (fileEntry->fileType == 0x02) // && fileEntry->fileSize > 0)
		{
			menuItem->canSelect = true;
		}
		else
		{
			menuItem->canSelect = false;
		}
		
		menuItem->fileEntry = fileEntry;
		
		viewMenu->AddMenuItem(menuItem);
	}
	
	sprintf(buf, "%d ", diskImage->numFreeSectors);

	CSlrString *strFileEntry = new CSlrString();

	strFileEntry->Concatenate(buf);

	// BLOCKS FREE.
	strFileEntry->Concatenate((u16)0x02);
	strFileEntry->Concatenate((u16)0x0C);
	strFileEntry->Concatenate((u16)0x0F);
	strFileEntry->Concatenate((u16)0x03);
	strFileEntry->Concatenate((u16)0x0B);
	strFileEntry->Concatenate((u16)0x13);
	strFileEntry->Concatenate((u16)0x20);
	strFileEntry->Concatenate((u16)0x06);
	strFileEntry->Concatenate((u16)0x12);
	strFileEntry->Concatenate((u16)0x05);
	strFileEntry->Concatenate((u16)0x05);
	strFileEntry->Concatenate((u16)0x2E);

	menuItem = new CViewDrive1541FileD64EntryItem(fontHeight, strFileEntry, tr, tg, tb, this);
	menuItem->canSelect = false;
	viewMenu->AddMenuItem(menuItem);

	
//	CByteBuffer *entryData = new CByteBuffer();
//	this->diskImage->ReadEntry(6, entryData);

	LOGD("start browsing d64");

	viewMenu->InitSelection();

	if (keepScrollPositionOnRefreshDiskImage)
	{
		for (int i = 0; i < targetScrollPosition; i++)
		{
			viewMenu->SelectNext();
		}
		
		keepScrollPositionOnRefreshDiskImage = false;
	}
	else
	{
		// Is that fixed? CViewDrive1541FileD64: start browsing with first file entry viewMenu->SelectNext() this does not work when window has just appeared, it seems pos/size is wrong

		// start browsing with first file entry
		viewMenu->SelectNext();

		// check if PRG if not, scroll to first PRG
		while (true)
		{
			std::list<CGuiViewMenuItem *>::iterator it = viewMenu->selectedItem;
			
			CViewDrive1541FileD64EntryItem *entry = (CViewDrive1541FileD64EntryItem*)*it;
			
			// end of disk (empty disk?)
			if (entry->fileEntry == NULL)
			{
				break;
			}
			
			// PRG?
			if (entry->fileEntry->fileType == 0x02)
			{
				// found PRG
				break;
			}
			
			it++;
			if (it == viewMenu->menuItems.end())
			{
				// not found PRG, scroll to top
				viewMenu->selectedItem = viewMenu->menuItems.begin();
				viewMenu->SelectNext();
				break;
			}
			
			viewMenu->SelectNext();
		}
	}
	
	debugInterfaceVice->UnlockMutex();
	guiMain->UnlockMutex();

	LOGD("CViewDrive1541FileD64::RefreshDiskImageMenu completed");
}

void CViewDrive1541Browser::SwitchFileD64Screen()
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

bool CViewDrive1541Browser::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return this->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541Browser::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_BACKSPACE)
	{
		SwitchFileD64Screen();
		return true;
	}
	
	guiMain->LockMutex();
	if (viewMenu && viewMenu->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		guiMain->UnlockMutex();
		return true;
	}
	guiMain->UnlockMutex();

	if (keyCode == MTKEY_ESC)
	{
		SwitchFileD64Screen();
		return true;
	}
	
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541Browser::DoScrollWheel(float deltaX, float deltaY)
{
	LOGD("CViewDrive1541FileD64::DoScrollWheel");
	guiMain->LockMutex();
	if (viewMenu)
	{
		float f = 0.5f;
		float nt = 0.0f;
		
		nt = fabs(deltaY) * f;

		if (deltaY < 0)
		{
			for (float v = 0.0f; v < nt; v += 1.0f)
			{
				viewMenu->SelectNext();
			}
		}
		else
		{
			for (float v = 0.0f; v < nt; v += 1.0f)
			{
				viewMenu->SelectPrev();
			}
		}
	}
	guiMain->UnlockMutex();
	
	return true;
}


bool CViewDrive1541Browser::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	guiMain->LockMutex();
	if (viewMenu && viewMenu->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
	{
		guiMain->UnlockMutex();
		return true;
	}
	guiMain->UnlockMutex();

	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDrive1541Browser::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewDrive1541Browser::ActivateView()
{
	LOGG("CViewDrive1541FileD64::ActivateView()");
	
	viewC64->ShowMouseCursor();
}

void CViewDrive1541Browser::DeactivateView()
{
	LOGG("CViewDrive1541FileD64::DeactivateView()");
}

void CViewDrive1541Browser::StartDiskPRGEntry(int entryNum, bool showLoadAddressInfo)
{
	LOGD("CViewDrive1541FileD64::StartDiskPRGEntry: entryNum=%d", entryNum);
	
	this->SetDiskImage(0);
	
	if (!this->diskImage->IsDiskAttached())
	{
		viewC64->ShowMessageError("No disk");
		return;
	}
	
	DiskImageFileEntry *fileEntry = this->diskImage->FindDiskPRGEntry(entryNum);
	
	if (fileEntry == NULL)
	{
//		viewC64->ShowMessageError("Diskette is empty");
		return;
	}
	
	this->StartFileEntry(fileEntry, showLoadAddressInfo);

}

bool CViewDrive1541Browser::HasContextMenuItems()
{
	return true;
}

void CViewDrive1541Browser::RenderContextMenuItems()
{
	if (ImGui::MenuItem("Open"))
	{
		viewC64->OpenFileDialog();
	}
	
	if (ImGui::MenuItem("Create new diskette"))
	{
		CSlrString *windowTitle = new CSlrString("Create new D64");
		
		char *buf = SYS_GetCharBuf();
		CSlrDate *date = new CSlrDate();
		date->DateTimeToFileNameString(buf);
		delete date;
		
		char *buf2 = SYS_GetCharBuf();
		sprintf(buf2, "disk-%s.d64", buf);
		
		CSlrString *defaultFileName = new CSlrString(buf2);

		SYS_ReleaseCharBuf(buf);
		SYS_ReleaseCharBuf(buf2);

		dialogOperation = ViewDrive1541FileD64Dialog_CreateNew;
		viewC64->ShowDialogSaveFile(this, &diskFileExtensions, defaultFileName, c64SettingsDefaultD64Folder, windowTitle);
		delete windowTitle;
	}

	ImGui::Separator();
	if (c64SettingsPathToD64 != NULL)
	{
		CSlrString *fileName;
		if (guiMain->isAltPressed)
		{
			fileName = new CSlrString(c64SettingsPathToD64);
		}
		else
		{
			fileName = c64SettingsPathToD64->GetFileNameComponentFromPath();
		}
		char *cFileName = fileName->GetUTF8();
		ImGui::TextDisabled("%s", cFileName);
		STRFREE(cFileName);
		delete fileName;
	}

//	if (diskImage->diskImage != NULL)
	disk_image_t *viceDiskImage = c64d_get_drive_disk_image(0);
	bool diskAvailable = viceDiskImage != NULL;
	
	if (c64SettingsPathToD64 != NULL)
	{
		if (ImGui::BeginMenu("Format", diskAvailable))
		{
	//		bool ImGui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
			
			ImGui::InputText("Disk name##FormatDisk", diskName, 0x11);
			ImGui::InputText("Disk id##FormatDisk", diskId, 0x08);

			if (ImGui::MenuItem("Perform format"))
			{
				CDebugInterfaceVice *debugInterface = (CDebugInterfaceVice*)viewC64->debugInterfaceC64;
				debugInterface->LockMutex();
				
				char *lowDiskName = SYS_GetCharBuf();
				char *lowDiskId = SYS_GetCharBuf();

				for(int i = 0; i < strlen(diskName); i++)
				{
					lowDiskName[i] = tolower(diskName[i]);
				}
				lowDiskName[strlen(diskName)] = 0;

				for(int i = 0; i < strlen(diskId); i++)
				{
					lowDiskId[i] = tolower(diskId[i]);
				}
				lowDiskId[strlen(diskId)] = 0;

				diskImage->FormatDisk(lowDiskName, lowDiskId);
				
				SYS_ReleaseCharBuf(lowDiskName);
				SYS_ReleaseCharBuf(lowDiskId);

				debugInterface->UnlockMutex();
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Insert file", NULL, false, diskAvailable))
		{
			CSlrString *windowTitle = new CSlrString("Select File to Insert into Disk Image");
			dialogOperation = ViewDrive1541FileD64Dialog_InsertFile;
			viewC64->ShowDialogOpenFile(this, &insertFileExtensions, c64SettingsDefaultPRGFolder, windowTitle);
			delete windowTitle;
		}
	}
	
//	ImGui::Separator();
//	if (ImGui::MenuItem("Refresh"))
//	{
//		this->RefreshInsertedDiskImageAsync();
//	}

}

//
void CViewDrive1541Browser::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGM("CViewDrive1541FileD64::SystemDialogFileOpenSelected, path=%x", path);
	path->DebugPrint("path=");
	
	if (dialogOperation == ViewDrive1541FileD64Dialog_InsertFile)
	{
		char *cPath = path->GetStdASCII();
		std::filesystem::path filePath = std::filesystem::u8path(cPath);
		int ret = diskImage->InsertFile(filePath, true);
		if (ret == RET_STATUS_OK)
		{
			viewC64->ShowMessage("Inserted file %s", filePath.filename().string().c_str());
		}
		else if (ret == RET_STATUS_REPLACED)
		{
			viewC64->ShowMessage("Replaced file %s", filePath.filename().string().c_str());
		}
		else //(ret == RET_STATUS_FAILED)
		{
			viewC64->ShowMessageError("Failed to insert file %s", filePath.filename().string().c_str());
		}
		
		STRFREE(cPath);
	}
}


void CViewDrive1541Browser::SystemDialogFileOpenCancelled()
{
}

void CViewDrive1541Browser::SystemDialogFileSaveSelected(CSlrString *path)
{
	if (dialogOperation == ViewDrive1541FileD64Dialog_CreateNew)
	{
//		debugInterfaceVice->LockMutex();
		
		char *cPath = path->GetUTF8();
		diskImage->CreateDiskImage(cPath);
		viewC64->debugInterfaceC64->InsertD64(path);
	
		// this below is removed on purpose, as we are preparing for ARC in next C++ versions
//		if (c64SettingsPathToD64)
//			STRFREE(c64SettingsPathToD64);

		c64SettingsPathToD64 = new CSlrString(path);
		
		// TODO: move this to vice action callback, this will always fail:
		SYS_Sleep(1000);
		diskImage->SetDiskImage(0);
		diskImage->FormatDisk("retrodebugger", "rd");
		STRFREE(cPath);
		
		C64DebuggerStoreSettings();
		
//		debugInterfaceVice->UnlockMutex();
	}
}

void CViewDrive1541Browser::SystemDialogFileSaveCancelled()
{
	
}



//
CViewDrive1541FileD64EntryItem::CViewDrive1541FileD64EntryItem(float height, CSlrString *str, float r, float g, float b, CViewDrive1541Browser *browser)
: CViewC64MenuItem(height, str, NULL, r, g, b)
{
	this->browser = browser;
	canSelect = false;
	fileEntry = NULL;
}

CViewDrive1541FileD64EntryItem::~CViewDrive1541FileD64EntryItem()
{
	delete str;
}
	
void CViewDrive1541FileD64EntryItem::RenderItem(float px, float py, float pz)
{
	viewC64->fontCBM1->BlitTextColor(str, px, py, pz,
												  browser->fontScale, r, g, b, 1);

}

////
CViewDrive1541FileD64RefreshDiskImageThread::CViewDrive1541FileD64RefreshDiskImageThread(CViewDrive1541Browser *viewDrive1541FileD64)
{
	this->viewDrive1541FileD64 = viewDrive1541FileD64;
}

void CViewDrive1541FileD64RefreshDiskImageThread::ThreadRun(void *passData)
{
	while(true)
	{
		SYS_Sleep(500);
		
		if (viewDrive1541FileD64->IsVisible())
		{
			CDebugInterfaceVice *debugInterface = (CDebugInterfaceVice*)viewC64->debugInterfaceC64;
			if (debugInterface->IsDriveDirtyForRefresh(0))
			{
				debugInterface->LockMutex();
				viewDrive1541FileD64->SetDiskImage(0);
				debugInterface->ClearDriveDirtyForRefreshFlag(0);
				debugInterface->UnlockMutex();
				
				viewDrive1541FileD64->keepScrollPositionOnRefreshDiskImage = true;
				viewDrive1541FileD64->RefreshInsertedDiskImageAsync();
			}
		}
	}
}

