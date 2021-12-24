#include "CViewC64.h"
#include "CViewFileD64.h"
#include "CColorsTheme.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CViewAbout.h"
#include "CDebugInterfaceC64.h"
#include "CViewMemoryMap.h"
#include "C64SettingsStorage.h"
#include "CGuiMain.h"

#include "CDiskImageD64.h"

CViewFileD64::CViewFileD64(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewFileD64";

	font = viewC64->fontCBMShifted;
	fontScale = 3;
	fontHeight = font->GetCharHeight('@', fontScale);

	strHeader = NULL;
	strHeader2 = NULL;
	
	this->diskImage = NULL;
	
	/// menu
	viewMenu = new CGuiViewMenu(35, 56, -1, sizeX-70, sizeY-76, this);

	
//	kbsMainMenuScreen = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Main menu screen", MTKEY_F9, false, false, false);
//	viewC64->keyboardShortcuts->AddShortcut(kbsMainMenuScreen);
	
//	viewMenu->SelectMenuItem(menuItemInsertD64);
	
	
//	std::list<u32> zones;
//	zones.push_back(KBZONE_GLOBAL);
//	CSlrKeyboardShortcut *sr = viewC64->keyboardShortcuts->FindShortcut(zones, '8', false, true, false);
	
	//LOGD("---done");
	


}

CViewFileD64::~CViewFileD64()
{
}

void CViewFileD64::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{
	CViewFileD64EntryItem *fileEntryMenuItem = (CViewFileD64EntryItem *)menuItem;
	DiskImageFileEntry *fileEntry = fileEntryMenuItem->fileEntry;

	this->StartFileEntry(fileEntry, true);
}

void CViewFileD64::StartFileEntry(DiskImageFileEntry *fileEntry, bool showLoadAddressInfo)
{
	LOGD("CViewFileD64::StartFileEntry");
	
	if (fileEntry != NULL && fileEntry->fileType == 0x02) // && fileEntry->fileSize > 0)
	{
		// load entry
		CByteBuffer *byteBuffer = new CByteBuffer();
		
		bool readed = diskImage->ReadEntry(fileEntry, byteBuffer);
		if (readed == false)
		{
			guiMain->ShowMessage("Load error");
			delete byteBuffer;
			return;
		}

		UpdateDriveDiskID();
		
		// load the PRG
		bool ret = viewC64->viewC64MainMenu->LoadPRG(byteBuffer, true, showLoadAddressInfo, false);
		
		if (ret == false)
		{
			guiMain->ShowMessage("Load error");
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

void CViewFileD64::UpdateDriveDiskID()
{
	if (diskImage)
	{
		// set correct disk ID to let 1541 ROM not throw 29, 'disk id mismatch'
		// see $F3F6 in 1541 ROM: http://unusedino.de/ec64/technical/misc/c1541/romlisting.html#FDD3
		//
		LOGD("...diskId= %02x %02x", diskImage->diskId[2], diskImage->diskId[3]);
		
		viewC64->debugInterfaceC64->SetByte1541(0x0012, diskImage->diskId[2]);
		viewC64->debugInterfaceC64->SetByte1541(0x0013, diskImage->diskId[3]);
		
		viewC64->debugInterfaceC64->SetByte1541(0x0016, diskImage->diskId[2]);
		viewC64->debugInterfaceC64->SetByte1541(0x0017, diskImage->diskId[3]);
	}
}

void CViewFileD64::MenuCallbackItemChanged(CGuiViewMenuItem *menuItem)
{
	LOGD("CViewFileD64::MenuCallbackItemChanged");
}


void CViewFileD64::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGM("CViewFileD64::SystemDialogFileOpenSelected, path=%x", path);
	path->DebugPrint("path=");
}


void CViewFileD64::SystemDialogFileOpenCancelled()
{
}


void CViewFileD64::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewFileD64::Render()
{
	//LOGD("CViewFileD64::Render");
	
//	viewC64->fontDisassembly->BlitText("CViewFileD64", 0, 0, 0, 11, 1.0);

	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
		
	float sb = 20;
	float gap = 10;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextB;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = 3;
	
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
	float py = scry + gap;
	
	font->BlitTextColor(strHeader, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
//	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
//	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;

	viewMenu->Render();
	
//	font->BlitTextColor("1541 Device 8...", px, py, -1, fontScale, tr, tg, tb, 1);
//	font->BlitTextColor("Alt+8", ax, py, -1, fontScale, tr, tg, tb, 1);
	CGuiView::Render();
}

void CViewFileD64::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewFileD64::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewFileD64::ButtonPressed(CGuiButton *button)
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
bool CViewFileD64::DoTap(float x, float y)
{
	LOGG("CViewFileD64::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewFileD64::DoFinishTap(float x, float y)
{
	LOGG("CViewFileD64::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewFileD64::DoDoubleTap(float x, float y)
{
	LOGG("CViewFileD64::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewFileD64::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewFileD64::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewFileD64::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewFileD64::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewFileD64::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewFileD64::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewFileD64::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewFileD64::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewFileD64::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewFileD64::StartBrowsingD64(char *fullFilePath)
{
	this->SetDiskImage(fullFilePath);
	
	this->StartSelectedDiskImageBrowsing();
}

void CViewFileD64::SetDiskImage(char *fileName)
{
	if (this->diskImage != NULL)
		delete this->diskImage;
	
	this->fullFilePath = fullFilePath;
	
	CSlrString *filePath = new CSlrString(fullFilePath);
	
	if (strHeader != NULL)
		delete strHeader;
	
	strHeader = filePath->GetFileNameComponentFromPath();
	
	
	
	CSlrFileFromOS *file = new CSlrFileFromOS(fullFilePath);
	
	if (file->Exists() == false)
	{
		LOGError("CViewFileD64::StartBrowsingD64: file '%s' does not exist", fullFilePath);
		return;
	}
	delete file;
	
	//CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	this->diskImage = new CDiskImageD64(fullFilePath);
}

void CViewFileD64::StartBrowsingD64(int deviceId)
{
	this->SetDiskImage(deviceId);

	if (this->diskImage->diskImage != NULL)
	{
		this->StartSelectedDiskImageBrowsing();
	}
	else
	{
		guiMain->ShowMessage("No disk");
	}
}

void CViewFileD64::SetDiskImage(int deviceId)
{
	if (this->diskImage != NULL)
		delete this->diskImage;
	
	// TODO: get from media file?
	this->fullFilePath = "";
	//	CSlrString *filePath = new CSlrString(fullFilePath);
	//	CSlrString *fileName = filePath->GetFileNameComponentFromPath();
	
	if (strHeader != NULL)
		delete strHeader;
	
	strHeader = new CSlrString("Device #8");
	
	this->diskImage = new CDiskImageD64(deviceId);
}

void CViewFileD64::StartSelectedDiskImageBrowsing()
{
	this->SetDiskImage(0);
	this->RefreshInsertedDiskImage();
}

void CViewFileD64::RefreshInsertedDiskImage()
{
	LOGD("CViewFileD64::RefreshInsertedDiskImage");

	this->RefreshDiskImageMenu();
}

void CViewFileD64::RefreshInsertedDiskImageAsync()
{
	LOGD("CViewFileD64::RefreshInsertedDiskImageAsync");
	if (this->isRunning)
		return;
	
	SYS_StartThread(this);
}

void CViewFileD64::ThreadRun(void *passData)
{
	this->RefreshInsertedDiskImage();
}

void CViewFileD64::RefreshDiskImageMenu()
{
	LOGD("CViewFileD64::RefreshDiskImageMenu");
	
	if (this->diskImage == NULL)
	{
		LOGError("CViewFileD64::RefreshDiskImageMenu: no disk image");
		return;
	}
	
	guiMain->LockMutex();
	
	//
	/// colors
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;
	

	
	if (this->viewMenu != NULL)
	{
		delete this->viewMenu;
	}
	
	viewMenu = new CGuiViewMenu(35, 56, -1, sizeX-70, sizeY-76, this);

	CViewFileD64EntryItem *menuItem = NULL;
	
	// header
	CSlrString *strD64Header = new CSlrString("0 ");	
	
	u16 chr = '\"' | 0x80;
	strD64Header->Concatenate(chr);
	
	for (int i = 0; i < 0x10; i++)
	{
		//LOGD("diskImage->diskName[%d]=%02x (%c)", i, diskImage->diskName[i], diskImage->diskName[i]);
		
		chr = diskImage->diskName[i];
		
		if (chr == 0x14)
		{
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
	
	for (int i = 1; i < 0x07; i++)
	{
		chr = diskImage->diskId[i];

		if (chr == 0x14)
		{
			strD64Header->RemoveLastCharacter();
			continue;
		}
		
		chr = ConvertPetsciiToScreenCode(chr);
		
		chr |= 0x80;
		
		strD64Header->Concatenate(chr);
	}
	
	menuItem = new CViewFileD64EntryItem(fontHeight, strD64Header, tr, tg, tb);
	menuItem->canSelect = false;
	viewMenu->AddMenuItem(menuItem);
	
	char buf[128];

	for (std::vector<DiskImageFileEntry *>::iterator it = diskImage->fileEntries.begin(); it != diskImage->fileEntries.end(); it++)
	{
		DiskImageFileEntry *fileEntry = *it;
		
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
		
		
		menuItem = new CViewFileD64EntryItem(fontHeight, strFileEntry, tr, tg, tb);
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

	menuItem = new CViewFileD64EntryItem(fontHeight, strFileEntry, tr, tg, tb);
	menuItem->canSelect = false;
	viewMenu->AddMenuItem(menuItem);

	
//	CByteBuffer *entryData = new CByteBuffer();
//	this->diskImage->ReadEntry(6, entryData);

	
	
	// start browsing with first file entry
	viewMenu->SelectNext();

	// check if PRG if not, scroll to first PRG
	while (true)
	{
		std::list<CGuiViewMenuItem *>::iterator it = viewMenu->selectedItem;
		
		CViewFileD64EntryItem *entry = (CViewFileD64EntryItem*)*it;
		
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
	
	
	guiMain->UnlockMutex();

}

void CViewFileD64::SwitchFileD64Screen()
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

bool CViewFileD64::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_BACKSPACE)
	{
		SwitchFileD64Screen();
		return true;
	}
	
	if (viewMenu->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	if (keyCode == MTKEY_ESC)
	{
		SwitchFileD64Screen();
		return true;
	}
	
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewFileD64::DoScrollWheel(float deltaX, float deltaY)
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
	
	
	return true;
}


bool CViewFileD64::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (viewMenu->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewFileD64::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewFileD64::ActivateView()
{
	LOGG("CViewFileD64::ActivateView()");
	
	viewC64->ShowMouseCursor();
}

void CViewFileD64::DeactivateView()
{
	LOGG("CViewFileD64::DeactivateView()");
}

void CViewFileD64::StartDiskPRGEntry(int entryNum, bool showLoadAddressInfo)
{
	LOGD("CViewFileD64::StartDiskPRGEntry: entryNum=%d", entryNum);
	
	this->SetDiskImage(0);
	
	if (this->diskImage->diskImage == NULL)
	{
		guiMain->ShowMessage("No disk");
		return;
	}
	
	DiskImageFileEntry *fileEntry = this->diskImage->FindDiskPRGEntry(entryNum);
	
	if (fileEntry == NULL)
	{
		guiMain->ShowMessage("No start file");
		return;
	}
	
	this->StartFileEntry(fileEntry, showLoadAddressInfo);

}


CViewFileD64EntryItem::CViewFileD64EntryItem(float height, CSlrString *str, float r, float g, float b)
: CViewC64MenuItem(height, str, NULL, r, g, b)
{
	canSelect = false;
	fileEntry = NULL;
}

CViewFileD64EntryItem::~CViewFileD64EntryItem()
{
	delete str;
}
	
void CViewFileD64EntryItem::RenderItem(float px, float py, float pz)
{
	viewC64->fontCBM1->BlitTextColor(str, px, py, pz,
												  viewC64->viewC64MainMenu->fontScale, r, g, b, 1);

}

