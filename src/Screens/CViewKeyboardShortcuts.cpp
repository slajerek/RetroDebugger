//
// NOTE:
// This view will be removed. It is being refactored and moved to main menu bar instead
//

#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewKeyboardShortcuts.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64SettingsStorage.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CViewC64KeyMap.h"
#include "CDebugInterfaceC64.h"
#include "MTH_Random.h"

#include "CViewDataMap.h"
#include "CMainMenuBar.h"

#include "CGuiMain.h"
#include "SND_SoundEngine.h"

#define C64DEBUGGER_KEYBOARD_SHORTCUTS_VERSION 1

CViewKeyboardShortcuts::CViewKeyboardShortcuts(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewKeyboardShortcuts";

	extKeyboardShortucts.push_back(new CSlrString("kbs"));

	font = viewC64->fontDefaultCBMShifted;
	fontScale = 2.7;
	fontHeight = font->GetCharHeight('@', fontScale) + 3;

	strHeader = new CSlrString("Keyboard Shortcuts");
	strEnterKeyFor = new CSlrString("Enter key for");
	strKeyFunctionName = NULL;
	
	
	/// colors
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;

	float sb = 20;

	enteringKey = NULL;

	/// menu
	viewMenu = new CGuiViewMenu(35, 51, -1, sizeX-70, sizeY-51-sb, this);
	
	UpdateMenuKeyboardShortcuts();
}



CViewKeyboardShortcuts::~CViewKeyboardShortcuts()
{
}

void CViewKeyboardShortcuts::UpdateMenuKeyboardShortcuts()
{
	LOGTODO("SETTINGS FOR KEY SHORTCUTS");
	
	/*
	
	// TODO: get start displaying item remember index number and recreate after update
	
	guiMain->LockMutex();

	viewMenu->ClearSelection();
	viewMenu->ClearItems();
	
	//
	menuItemBack  = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString("<< SAVE"),
										 NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemBack);
	
	menuItemImportKeyboardShortcuts = new CViewC64MenuItem(fontHeight, new CSlrString(" Import keyboard shortcuts..."),
														   NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemImportKeyboardShortcuts);

	menuItemExportKeyboardShortcuts = new CViewC64MenuItem(fontHeight*2.0f, new CSlrString(" Export keyboard shortcuts..."),
														   NULL, tr, tg, tb);
	viewMenu->AddMenuItem(menuItemExportKeyboardShortcuts);
	

	
	this->shortcuts = viewC64->keyboardShortcuts;

	// iterate over zones & hash codes
	for (std::map<u32, CSlrKeyboardShortcutsZone *>::iterator it = shortcuts->mapOfZones->begin();
		 it != shortcuts->mapOfZones->end(); it++)
	{
		CSlrKeyboardShortcutsZone *zone = it->second;
		
		for (std::list<CSlrKeyboardShortcut *>::iterator itShortcut = zone->shortcuts.begin();
			 itShortcut != zone->shortcuts.end(); itShortcut++)
		{
			CSlrKeyboardShortcut *shortcut = *itShortcut;
			
			CViewC64MenuItem *menuItemShortcut = new CViewC64MenuItem(fontHeight, new CSlrString(shortcut->name),
																	  shortcut, tr, tg, tb);
			
//			menuItemShortcut->str2 = shortcut->str;
			
			viewMenu->AddMenuItem(menuItemShortcut);
			
			shortcut->userData = menuItemShortcut;

		}
	}

	guiMain->UnlockMutex();
	*/
}

void CViewKeyboardShortcuts::SaveAndBack()
{
	// TODO: REMOVE ME, ADD SETTINGS FOR SHORTCUTS
	LOGD("CViewKeyboardShortcuts::SaveAndBack");
	
//	if (viewC64->viewC64MainMenu->kbsMainMenuScreen->keyCode < 1)
//	{
//		viewC64->ShowMessage("Please assign \"Main Menu\" keyboard shortcut");
//
//		// TODO: scroll to Main Menu shortcut menu item
//
//		return;
//	}
	
	StoreKeyboardShortcuts();
	
	UpdateQuitShortcut();
	
//	guiMain->SetView(viewC64->viewC64SettingsMenu);
}

void CViewKeyboardShortcuts::StoreKeyboardShortcuts()
{
	LOGTODO("CViewKeyboardShortcuts::StoreKeyboardShortcuts !!!!!!!!!!!!!!!!!!!!");
	
	/*
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutU16(C64DEBUGGER_KEYBOARD_SHORTCUTS_VERSION);
	shortcuts->StoreToByteBuffer(byteBuffer);
	
	CSlrString *fileName = new CSlrString(C64D_KEYBOARD_SHORTCUTS_FILE_PATH);
	byteBuffer->storeToSettings(fileName);
	delete fileName;
	
	delete byteBuffer;
	 */

	LOGD("CViewKeyboardShortcuts::StoreKeyboardShortcuts: done");
}

void CViewKeyboardShortcuts::RestoreKeyboardShortcuts()
{
	LOGTODO("CViewKeyboardShortcuts::RestoreKeyboardShortcuts !!!!!!!!!!");

	/*
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	CSlrString *fileName = new CSlrString(C64D_KEYBOARD_SHORTCUTS_FILE_PATH);
	bool ret = byteBuffer->loadFromSettings(fileName);
	delete fileName;
	
	if (ret == false || byteBuffer->length == 0)
	{
		LOGD("... no keyboard shortcuts found");
		delete byteBuffer;
		return;
	}
	
	u16 version = byteBuffer->GetU16();
	if (version != C64DEBUGGER_KEYBOARD_SHORTCUTS_VERSION)
	{
		LOGError("CViewKeyboardShortcuts: incompatible version %04x", version);
		delete byteBuffer;
		return;
	}
	
	shortcuts->LoadFromByteBuffer(byteBuffer);

	delete byteBuffer;
	
	UpdateQuitShortcut();
	 */
	
	LOGD("CViewKeyboardShortcuts::RestoreKeyboardShortcuts: done");
}

void CViewKeyboardShortcuts::UpdateQuitShortcut()
{
	SYS_SetQuitKey(viewC64->mainMenuBar->kbsQuitApplication->keyCode,
				   viewC64->mainMenuBar->kbsQuitApplication->isShift,
				   viewC64->mainMenuBar->kbsQuitApplication->isAlt,
				   viewC64->mainMenuBar->kbsQuitApplication->isControl);
}

void CViewKeyboardShortcuts::OpenDialogExportKeyboardShortcuts()
{
	CSlrString *defaultFileName = new CSlrString("shortcuts");
	
	CSlrString *windowTitle = new CSlrString("Export keyboard shortcuts");
	viewC64->ShowDialogSaveFile(this, &extKeyboardShortucts, defaultFileName, gUTFPathToDocuments, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewKeyboardShortcuts::SystemDialogFileSaveSelected(CSlrString *path)
{
	LOGTODO("CViewKeyboardShortcuts::SystemDialogFileSaveSelected");
	/*
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutU16(C64DEBUGGER_KEYBOARD_SHORTCUTS_VERSION);
	this->shortcuts->StoreToByteBuffer(byteBuffer);
	
	byteBuffer->storeToFile(path);
	
	delete byteBuffer;
	
	viewC64->ShowMessage("Keyboard shortcuts saved");
	 */
}

void CViewKeyboardShortcuts::SystemDialogFileSaveCancelled()
{
	
}

void CViewKeyboardShortcuts::OpenDialogImportKeyboardShortcuts()
{
	CSlrString *windowTitle = new CSlrString("Import keyboard shortcuts");
	viewC64->ShowDialogOpenFile(this, &extKeyboardShortucts, gUTFPathToDocuments, windowTitle);
	delete windowTitle;
}

void CViewKeyboardShortcuts::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGTODO("CViewKeyboardShortcuts::SystemDialogFileOpenSelected");
	
	/*
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->readFromFile(path);
	
	if (byteBuffer->length == 0)
	{
		viewC64->ShowMessage("Keyboard shortcuts not found");
		delete byteBuffer;
		return;
	}
	
	u16 version = byteBuffer->GetU16();
	if (version != C64DEBUGGER_KEYBOARD_SHORTCUTS_VERSION)
	{
		viewC64->ShowMessage("Not compatible file version");
		delete byteBuffer;
		return;
	}
	
	shortcuts->LoadFromByteBuffer(byteBuffer);
	
	delete byteBuffer;

	UpdateMenuKeyboardShortcuts();
	
	viewC64->ShowMessage("Load keyboard shortcuts");
	 */
}

void CViewKeyboardShortcuts::SystemDialogFileOpenCancelled()
{
	
}


void CViewKeyboardShortcuts::MenuCallbackItemChanged(CGuiViewMenuItem *menuItem)
{

}

void CViewKeyboardShortcuts::MenuCallbackItemEntered(CGuiViewMenuItem *menuItem)
{
	if (menuItem == menuItemBack)
	{
		SaveAndBack();
		return;
	}
	else if (menuItem == menuItemExportKeyboardShortcuts)
	{
		OpenDialogExportKeyboardShortcuts();
		return;
	}
	else if (menuItem == menuItemImportKeyboardShortcuts)
	{
		OpenDialogImportKeyboardShortcuts();
		return;
	}
	
	CViewC64MenuItem *menuItemC64 = (CViewC64MenuItem *)menuItem;
	
	if (menuItemC64->shortcut)
	{
		isShift = false;
		isAlt = false;
		isControl = false;
		keyUpEaten = false;
		enteringKey = menuItemC64->shortcut;
		if (strKeyFunctionName != NULL)
			delete strKeyFunctionName;
		
		strKeyFunctionName = new CSlrString(enteringKey->name);
	}
}

void CViewKeyboardShortcuts::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewKeyboardShortcuts::Render()
{
//	guiMain->fntConsole->BlitText("CViewKeyboardShortcuts", 0, 0, 0, 11, 1.0);

	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
		
	float sb = 20;
	float gap = 4;
	
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
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;

	viewMenu->Render();

	if (enteringKey)
	{
		BlitFilledRectangle(0, 0, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0.7);
		
		float px = SCREEN_WIDTH/2;
		float py = 150;
		font->BlitTextColor(strEnterKeyFor, px, py, posZ, 2.5f, 1.0f, 1.0f, 1.0f, 1.0f, FONT_ALIGN_CENTER);

		py += 25;
		font->BlitTextColor(strKeyFunctionName, px, py, posZ, 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, FONT_ALIGN_CENTER);

	
	}
	
	CGuiView::Render();
}

void CViewKeyboardShortcuts::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewKeyboardShortcuts::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewKeyboardShortcuts::ButtonPressed(CGuiButton *button)
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
bool CViewKeyboardShortcuts::DoTap(float x, float y)
{
	LOGG("CViewKeyboardShortcuts::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewKeyboardShortcuts::DoFinishTap(float x, float y)
{
	LOGG("CViewKeyboardShortcuts::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewKeyboardShortcuts::DoDoubleTap(float x, float y)
{
	LOGG("CViewKeyboardShortcuts::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewKeyboardShortcuts::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewKeyboardShortcuts::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewKeyboardShortcuts::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewKeyboardShortcuts::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewKeyboardShortcuts::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewKeyboardShortcuts::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewKeyboardShortcuts::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewKeyboardShortcuts::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewKeyboardShortcuts::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewKeyboardShortcuts::SwitchScreen()
{
//	if (guiMain->currentView == this)
//	{
//		SaveAndBack();
//	}
//	else
//	{
//		guiMain->SetView(this);
//	}
}

bool CViewKeyboardShortcuts::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("KeyDown: %d", keyCode);
	if (enteringKey != NULL)
	{
		this->isShift = isShift;
		this->isAlt = isAlt;
		this->isControl = isControl;
		
		if (keyCode == MTKEY_LALT || keyCode == MTKEY_RALT || keyCode == MTKEY_LCONTROL || keyCode == MTKEY_RCONTROL
			|| keyCode == MTKEY_LSHIFT || keyCode == MTKEY_RSHIFT)
		{
			return true;
		}
		
		EnteredKeyCode(keyCode);
		return true;
	}
	
	if (viewMenu->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	if (keyCode == MTKEY_ESC || keyCode == MTKEY_BACKSPACE)
	{
		SwitchScreen();
		return true;
	}


	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewKeyboardShortcuts::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("KeyUp: %d", keyCode);
	if (enteringKey != NULL)
	{
		if (keyUpEaten == false)
		{
			keyUpEaten = true;
			return true;
		}

		EnteredKeyCode(keyCode);
		return true;
	}

	if (viewMenu->KeyUp(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewKeyboardShortcuts::EnteredKeyCode(u32 keyCode)
{
	LOGTODO("CViewKeyboardShortcuts::EnteredKeyCode");
	
	/*
	bool finalise = true;
	
	if (keyCode == MTKEY_LALT || keyCode == MTKEY_RALT
		|| keyCode == MTKEY_LCONTROL || keyCode == MTKEY_RCONTROL
		|| keyCode == MTKEY_LSHIFT || keyCode == MTKEY_RSHIFT)
	{
		isShift = false;
		isAlt = false;
		isControl = false;
	}
	
	//LOGD("EnteredKeyCode: %04x %d %d %d", keyCode, isShift, isAlt, isControl);

	u32 bareKey = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
	
	bool isSuper = false;
	CSlrKeyboardShortcut *conflictingShortcut =
		shortcuts->FindShortcut(enteringKey->zone, bareKey, isShift, isAlt, isControl, isSuper);
	
	if (conflictingShortcut == NULL)
	{
		shortcuts->FindShortcut(MT_KEYBOARD_SHORTCUT_GLOBAL, bareKey, isShift, isAlt, isControl);
	}
	
	char *buf = SYS_GetCharBuf();

	if (conflictingShortcut == this->enteringKey)
	{
		CSlrString *keyCodeName = SYS_KeyCodeToString(conflictingShortcut->keyCode,
													  conflictingShortcut->isShift, conflictingShortcut->isAlt, conflictingShortcut->isControl);
		char *bufKeyCode = keyCodeName->GetStdASCII();
		delete keyCodeName;
		sprintf(buf, "Unassigned key %s", bufKeyCode);
		delete [] bufKeyCode;
		
		guiMain->LockMutex();
		
		shortcuts->RemoveShortcut(enteringKey);
		enteringKey->SetKeyCode(-1, false, false, false);

		guiMain->UnlockMutex();

	}
	else if (conflictingShortcut != NULL)
	{
		CSlrString *keyCodeName = SYS_KeyCodeToString(conflictingShortcut->keyCode,
													  conflictingShortcut->isShift, conflictingShortcut->isAlt, conflictingShortcut->isControl);
		char *bufKeyCode = keyCodeName->GetStdASCII();
		delete keyCodeName;
		sprintf(buf, "Key  %s  conflicts with  %s", bufKeyCode, conflictingShortcut->name);
		delete [] bufKeyCode;
	}
	else
	{
		CSlrString *keyCodeName = SYS_KeyCodeToString(bareKey, this->isShift, this->isAlt, this->isControl);
		char *bufKeyCode = keyCodeName->GetStdASCII();
		sprintf(buf, "%s   is   %s", bufKeyCode, enteringKey->name);
		delete [] bufKeyCode;


		guiMain->LockMutex();
		
		shortcuts->RemoveShortcut(enteringKey);
		enteringKey->SetKeyCode(bareKey, isShift, isAlt, isControl);
		shortcuts->AddShortcut(enteringKey);
		
		guiMain->UnlockMutex();
		
	}
	
	viewC64->ShowMessage(buf);
	
	SYS_ReleaseCharBuf(buf);
	
	if (finalise)
	{
		this->enteringKey = NULL;
	}
	 */
}

bool CViewKeyboardShortcuts::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewKeyboardShortcuts::ActivateView()
{
	LOGG("CViewKeyboardShortcuts::ActivateView()");
}

void CViewKeyboardShortcuts::DeactivateView()
{
	LOGG("CViewKeyboardShortcuts::DeactivateView()");
}

