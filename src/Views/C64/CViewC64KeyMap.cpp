extern "C"{
#include "keyboard.h"
}

#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewC64KeyMap.h"
#include "GUI_Main.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64SettingsStorage.h"
#include "C64KeyMap.h"
#include "CViewC64Screen.h"
#include "CMainMenuBar.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CDebugInterfaceC64.h"
#include "MTH_Random.h"

#include "CViewDataMap.h"

#include "CGuiMain.h"

#include "CDebugInterfaceVice.h"

#define KeyCodeFromRowCol(row, col)  ( ((row)+8)*8 + (col) )


CViewC64KeyMap::CViewC64KeyMap(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "C64 Key mapping";
	
	extKeyMap.push_back(new CSlrString("c64kbm"));

	font = viewC64->fontCBMShifted;
	
	fontScale = 2;
	fontWidth = font->GetCharWidth('@', fontScale);
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	// buttons
	float startX = 30;
	float startY = 47;
	
	float px = startX;
	float py = startY;
	
	float buttonSizeX = 128.3f;
	float buttonSizeY = fontHeight + 8;
	float buttonGap = 8.0f;
	float buttonGapY = 5.0f;
	float textOffsetY = 4.5f;
	
	btnBack = new CGuiButton(NULL, NULL,
								   px, py, posZ, buttonSizeX, buttonSizeY,
								   new CSlrString("SET AS DEFAULT"),
								   FONT_ALIGN_CENTER, buttonSizeX/2, textOffsetY,
								   font, fontScale,
								   1.0, 1.0, 1.0, 1.0,
								   0.3, 0.3, 0.3, 1.0,
								   this);
	this->AddGuiElement(btnBack);
	
	px += buttonSizeX + buttonGap;

	btnImportKeyMap = new CGuiButton(NULL, NULL,
							 px, py, posZ, buttonSizeX, buttonSizeY,
							 new CSlrString("IMPORT"),
							 FONT_ALIGN_CENTER, buttonSizeX/2, textOffsetY,
							 font, fontScale,
							 1.0, 1.0, 1.0, 1.0,
							 0.3, 0.3, 0.3, 1.0,
							 this);
	this->AddGuiElement(btnImportKeyMap);

	px += buttonSizeX + buttonGap;

	btnExportKeyMap = new CGuiButton(NULL, NULL,
							 px, py, posZ, buttonSizeX, buttonSizeY,
							 new CSlrString("EXPORT"),
							 FONT_ALIGN_CENTER, buttonSizeX/2, textOffsetY,
							 font, fontScale,
							 1.0, 1.0, 1.0, 1.0,
							 0.3, 0.3, 0.3, 1.0,
							 this);
	this->AddGuiElement(btnExportKeyMap);

	px += buttonSizeX + buttonGap;

	btnReset = new CGuiButton(NULL, NULL,
							 px, py, posZ, buttonSizeX, buttonSizeY,
							 new CSlrString("RESET"),
							 FONT_ALIGN_CENTER, buttonSizeX/2, textOffsetY,
							 font, fontScale,
							 1.0, 1.0, 1.0, 1.0,
							 0.3, 0.3, 0.3, 1.0,
							 this);
	this->AddGuiElement(btnReset);

	px = 166;
	py = 51 + buttonSizeY + buttonGapY;
	
	btnAssignKey = new CGuiButton(NULL, NULL,
									 px, py, posZ, buttonSizeX, buttonSizeY,
									 new CSlrString("ASSIGN KEY"),
									 FONT_ALIGN_CENTER, buttonSizeX/2, textOffsetY,
									 font, fontScale,
									 1.0, 1.0, 1.0, 1.0,
									 0.3, 0.3, 0.3, 1.0,
									 this);
	this->AddGuiElement(btnAssignKey);
	
	px += buttonSizeX + buttonGap;
	
	btnRemoveKey = new CGuiButton(NULL, NULL,
									 px, py, posZ, buttonSizeX, buttonSizeY,
									 new CSlrString("REMOVE KEY"),
									 FONT_ALIGN_CENTER, buttonSizeX/2, textOffsetY,
									 font, fontScale,
									 1.0, 1.0, 1.0, 1.0,
									 0.3, 0.3, 0.3, 1.0,
									 this);
	this->AddGuiElement(btnRemoveKey);
	
	
	// keyboard
	selectedKeyData = NULL;
	
	fontScale = 1.5;
	fontHeight = font->GetCharHeight(' ', fontScale) + 2;
	fontWidth = font->GetCharWidth(' ', fontScale)+2;
	fontProp = viewC64->fontCBMShifted; //guiMain->fntEngineDefault;
	fontPropScale = 1.25f;
	fontPropHeight = fontProp->GetCharHeight('X', fontPropScale) + 4.5;
	

	float gap = fontWidth;
	float fgap = fontWidth*3.5f;
	
	float w2 = fontWidth*2;
	float w3 = fontWidth*3;
	float w4 = fontWidth*4;
	float w5 = fontWidth*5;
	float w6 = fontWidth*6;
	float w7 = fontWidth*7;
	
	strHeader = new CSlrString("C64 key mapping");
	
	/// colors
	tr = 0.64; //163/255;
	tg = 0.59; //151/255;
	tb = 1.0; //255/255;
	
	startX = 48;
	startY = 207;
	
	px = startX;
	py = startY;
	
	float w;
	
	/*
	 C64 keyboard matrix:
	 
	 Bit   7   6   5   4   3   2   1   0
	 0    CUD  F5  F3  F1  F7 CLR RET DEL
	 1    SHL  E   S   Z   4   A   W   3
	 2     X   T   F   C   6   D   R   5
	 3     V   U   H   B   8   G   Y   7
	 4     N   O   K   M   0   J   I   9
	 5     ,   @   :   .   -   L   P   +
	 6     /   ^   =  SHR HOM  ;   *   £
	 7    R/S  Q   C= SPC  2  CTL  <-  1
	 */

	px = startX + fontWidth;

	float fux = fontWidth * 0.30f;
	float fuy = fontWidth * 0.45f;
	
	line1x = px - fux - fontWidth*0.30f;
	line1y = py - fuy;
	w = w2; AddButtonKey("~", "", px, py, w, 7, 1);  px += w + gap;
	w = w2; AddButtonKey("1!", "BK", px, py, w, 7, 0);  px += w + gap;
	w = w2; AddButtonKey("2\"", "WT", px, py, w, 7, 3);  px += w + gap;
	w = w2; AddButtonKey("3#", "RD", px, py, w, 1, 0);  px += w + gap;
	w = w2; AddButtonKey("4$", "CN", px, py, w, 1, 3);  px += w + gap;
	w = w2; AddButtonKey("5%", "PR", px, py, w, 2, 0);  px += w + gap;
	w = w2; AddButtonKey("6&", "GR", px, py, w, 2, 3);  px += w + gap;
	w = w2; AddButtonKey("7'", "BL", px, py, w, 3, 0);  px += w + gap;
	w = w2; AddButtonKey("8(", "YL", px, py, w, 3, 3);  px += w + gap;
	w = w2; AddButtonKey("9)", "RV", px, py, w, 4, 0);  px += w + gap;
	w = w2; AddButtonKey("0", "NM", px, py, w, 4, 3);  px += w + gap;
	w = w2; AddButtonKey("+", "", px, py, w, 5, 0);  px += w + gap;
	w = w2; AddButtonKey("-", "", px, py, w, 5, 3);  px += w + gap;
	w = w2; AddButtonKey("\x1c", "", px, py, w, 6, 0);  px += w + gap;
	w = w4; AddButtonKey(" CLR", "HOME", px, py, w, 6, 3);  px += w + gap;
	w = w4; AddButtonKey("INST", " DEL", px, py, w, 0, 0);  px += w + gap;

	line1sx = px - line1x - fux - fontWidth * 0.05f;

	px += fgap;
	
	linefx = px - fux - fontWidth*0.30f;
	linefy = line1y;

	w = w4; AddButtonKey(" F1", " F2", px, py, w, 0, 4);  px += w + gap;
	
	linefsx = px - linefx - fux - fontWidth * 0.05f;

	////
	
	py += fontHeight * 3;
	px = startX + fontWidth;
	
	line2x = px - fux - fontWidth*0.30f;
	line2y = py - fuy*1.37f;

	line2sx = line1sx;

	// C64 keys row 2
	w = w4; AddButtonKey(PLATFORM_STR_UPCASE_KEY_CTRL, "", px, py, w, 7, 2);  px += w + gap;

	w = w2; AddButtonKey("Q", "", px, py, w, 7, 6);  px += w + gap;
	w = w2; AddButtonKey("W", "", px, py, w, 1, 1);  px += w + gap;
	w = w2; AddButtonKey("E", "", px, py, w, 1, 6);  px += w + gap;
	w = w2; AddButtonKey("R", "", px, py, w, 2, 1);  px += w + gap;
	w = w2; AddButtonKey("T", "", px, py, w, 2, 6);  px += w + gap;
	w = w2; AddButtonKey("Y", "", px, py, w, 3, 1);  px += w + gap;
	w = w2; AddButtonKey("U", "", px, py, w, 3, 6);  px += w + gap;
	w = w2; AddButtonKey("I", "", px, py, w, 4, 1);  px += w + gap;
	w = w2; AddButtonKey("O", "", px, py, w, 4, 6);  px += w + gap;
	w = w2; AddButtonKey("P", "", px, py, w, 5, 1);  px += w + gap;
	w = w2; AddButtonKey("@", "", px, py, w, 5, 6);  px += w + gap;
	w = w2; AddButtonKey("*", "", px, py, w, 6, 1);  px += w + gap;
	w = w2; AddButtonKey("^", "", px, py, w, 6, 6);  px += w + gap;
	w = w7; AddButtonKey("RESTORE", "", px, py, w, C64_KEYCODE_RESTORE_ROW, C64_KEYCODE_RESTORE_COLUMN);  px += w + gap;	/// RESTORE key!

	px += fgap;

	w = w4; AddButtonKey(" F3", " F4", px, py, w, 0, 5);  px += w + gap;

	///
	
	py += fontHeight * 3;
	px = startX;
	
	linel1sy = py - line1y - fuy*1.37f;
	
	line3x = px - fux - fontWidth*0.30f;
	line3y = py - fuy*1.37f;
	
	// C64 keys row 3
	w = w3; AddButtonKey("RUN", "STP", px, py, w, 7, 7);  px += w + gap;
	w = w3; AddButtonKey("SHF", "LCK", px, py, w, C64_KEYCODE_SHIFT_LOCK_ROW, C64_KEYCODE_SHIFT_LOCK_COLUMN);  px += w + gap;	/// SHIFT lock
	w = w2; AddButtonKey("A", "", px, py, w, 1, 2);  px += w + gap;
	w = w2; AddButtonKey("S", "", px, py, w, 1, 5);  px += w + gap;
	w = w2; AddButtonKey("D", "", px, py, w, 2, 2);  px += w + gap;
	w = w2; AddButtonKey("F", "", px, py, w, 2, 5);  px += w + gap;
	w = w2; AddButtonKey("G", "", px, py, w, 3, 2);  px += w + gap;
	w = w2; AddButtonKey("H", "", px, py, w, 3, 5);  px += w + gap;
	w = w2; AddButtonKey("J", "", px, py, w, 4, 2);  px += w + gap;
	w = w2; AddButtonKey("K", "", px, py, w, 4, 5);  px += w + gap;
	w = w2; AddButtonKey("L", "", px, py, w, 5, 2);  px += w + gap;
	w = w2; AddButtonKey(":[", "", px, py, w, 5, 5);  px += w + gap;
	w = w2; AddButtonKey(";]", "", px, py, w, 6, 2);  px += w + gap;
	w = w2; AddButtonKey("=", "", px, py, w, 6, 5);  px += w + gap;
	w = w7; AddButtonKey("", " RETURN", px, py, w, 0, 1);  px += w + gap;
	
	line3sx = px - line3x - fux + fontWidth * 0.95f;

	px += fgap + fontWidth;
	w = w4; AddButtonKey(" F5", " F6", px, py, w, 0, 6);  px += w + gap;
	

	// C64 keys row 3
	py += fontHeight * 3;
	px = startX;
	
	line4x = px - fux - fontWidth*0.30f;
	line4y = py - fuy*1.37f;

	w = w2; AddButtonKey("", "C=", px, py, w, 7, 5);  px += w + gap;
	w = w5; keyLeftShift = AddButtonKey("", "SHIFT", px, py, w, 1, 7);  px += w + gap;
	w = w2; AddButtonKey("Z", "", px, py, w, 1, 4);  px += w + gap;
	w = w2; AddButtonKey("X", "", px, py, w, 2, 7);  px += w + gap;
	w = w2; AddButtonKey("C", "", px, py, w, 2, 4);  px += w + gap;
	w = w2; AddButtonKey("V", "", px, py, w, 3, 7);  px += w + gap;
	w = w2; AddButtonKey("B", "", px, py, w, 3, 4);  px += w + gap;
	w = w2; AddButtonKey("N", "", px, py, w, 4, 7);  px += w + gap;
	w = w2; AddButtonKey("M", "", px, py, w, 4, 4);  px += w + gap;
	w = w2; AddButtonKey(",<", "", px, py, w, 5, 7);  px += w + gap;
	w = w2; AddButtonKey(".>", "", px, py, w, 5, 4);  px += w + gap;
	w = w2; AddButtonKey("/?", "", px, py, w, 6, 7);  px += w + gap;
	w = w6; keyRightShift = AddButtonKey("", " SHIFT", px, py, w, 6, 4);  px += w + gap;
	w = w2; AddButtonKey("CR", "UD", px, py, w, 0, 7);  px += w + gap;
	w = w2; AddButtonKey("CR", "LR", px, py, w, 0, 2);  px += w + gap;
	
	line4sx = px - line4x - fux - fontWidth * 0.05f;

	px += fgap + fontWidth;
	w = w4; AddButtonKey(" F7", " F8", px, py, w, 0, 3);  px += w + gap;

	py += fontHeight * 3;

	line5x = line4x;
	line5y = py - fuy*1.37f;
	line5sx = line4sx;
	
	linel2sy = py - line3y - fuy*1.37f;

	linefsy = line5y - linefy;

	
	// space bar
	px = startX + fontWidth * 11;
	linespcx = px - fux - fontWidth*0.30f;
	linespc1y = py - fuy*1.37f;

	w = fontWidth * 25.0f;
	AddButtonKey("", "", px, py, w, 7, 4); px += w + gap;

	linespcsx = px - linespcx - fux - fontWidth * 0.05f;

	py += fontHeight * 3;
	linespc2y = py - fuy*1.37f;

	linespcsy = linespc2y - linespc1y;
	
	////
	isAssigningKey = false;
	isShift = false;
	
	C64KeyMap *keyMap = C64KeyMapGetDefault();
	UpdateFromKeyMap(keyMap);

	SelectKey(NULL);
	
	//
	viewC64->colorsTheme->AddThemeChangeListener(this);
}

CViewC64KeyMap::~CViewC64KeyMap()
{
}

// Note, this is deprecated, we need to finally correct in Engine. This below is legacy code transformed to new MTEngineSDL
void CViewC64KeyMap::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);

	// buttons
	float startX = 30 + posX;
	float startY = 47 + posY;
	
	float px = startX;
	float py = startY;
	
	float buttonSizeX = 128.3f;
	float buttonSizeY = fontHeight + 8;
	float buttonGap = 8.0f;
	float buttonGapY = 5.0f;
	
	btnBack->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	
	px += buttonSizeX + buttonGap;

	btnImportKeyMap->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);

	px += buttonSizeX + buttonGap;

	btnExportKeyMap->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);

	px += buttonSizeX + buttonGap;

	btnReset->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);

	px = 166 + posX;
	py = 51 + buttonSizeY + buttonGapY + posY;
	
	btnAssignKey->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	
	px += buttonSizeX + buttonGap;
	
	btnRemoveKey->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
}


bool CViewC64KeyMap::ButtonClicked(CGuiButton *button)
{
	if (button == btnBack)
	{
		SaveKeyboardMapping();
		return true;
	}
	
	if (button == btnAssignKey)
	{
		AssignKey();
		return true;
	}

	if (button == btnRemoveKey)
	{
		RemoveSelectedKey();
		return true;
	}
	
	else if (button == btnExportKeyMap)
	{
		OpenDialogExportKeyMap();
		return true;
	}
	else if (button == btnImportKeyMap)
	{
		OpenDialogImportKeyMap();
		return true;
	}
	else if (button == btnReset)
	{
		ResetKeyboardMappingToFactoryDefault();
		return true;
	}
	
	return false;
}

void CViewC64KeyMap::SaveKeyboardMapping()
{
	viewC64->debugInterfaceC64->LockMutex();
	bool ret = C64KeyMapStoreToSettings();
	viewC64->debugInterfaceC64->InitKeyMap(keyMap);
	viewC64->debugInterfaceC64->UnlockMutex();

	if (ret)
	{
//		viewC64->ShowMessage("Keyboard mapping has been successfully saved and set as the new default. Your custom configurations are now the standard settings.");
	}
	else
	{
		viewC64->ShowMessageError("Failed to save keyboard mapping. Please try again or check for potential issues with your configuration.");
	}
}

void CViewC64KeyMap::ResetKeyboardMappingToFactoryDefault()
{
	viewC64->debugInterfaceC64->LockMutex();
	C64KeyMapCreateDefault();
	C64KeyMapStoreToSettings();
	UpdateFromKeyMap(C64KeyMapGetDefault());
	viewC64->debugInterfaceC64->InitKeyMap(keyMap);
	viewC64->debugInterfaceC64->UnlockMutex();
	
	viewC64->ShowMessage("Keyboard mapping has been reset to factory defaults. Default configuration is now the standard setting.");
}


void CViewC64KeyMap::OpenDialogImportKeyMap()
{
	CSlrString *windowTitle = new CSlrString("Import C64 keyboard mapping");
	viewC64->ShowDialogOpenFile(this, &extKeyMap, gUTFPathToDocuments, windowTitle);
	delete windowTitle;

}

void CViewC64KeyMap::SystemDialogFileOpenSelected(CSlrString *path)
{
	guiMain->LockMutex();
	viewC64->debugInterfaceC64->LockMutex();
	
	if (C64KeyMapLoadFromFile(path))
	{
		C64KeyMap *keyMap = C64KeyMapGetDefault();
		UpdateFromKeyMap(keyMap);

		viewC64->debugInterfaceC64->UnlockMutex();
		guiMain->UnlockMutex();
		
		viewC64->ShowMessageInfo("C64 Key map loaded");
		
		SaveKeyboardMapping();
		return;
	}

	viewC64->debugInterfaceC64->UnlockMutex();
	guiMain->UnlockMutex();

	viewC64->ShowMessageError("C64 Key map loading failed");
}

void CViewC64KeyMap::SystemDialogFileOpenCancelled()
{
	
}

void CViewC64KeyMap::SystemDialogFileSaveSelected(CSlrString *path)
{
	C64KeyMapStoreToFile(path);
	
	viewC64->ShowMessageInfo("C64 Key map saved");
}

void CViewC64KeyMap::SystemDialogFileSaveCancelled()
{
	
}

void CViewC64KeyMap::OpenDialogExportKeyMap()
{
	CSlrString *defaultFileName = new CSlrString("c64keymap");
	
	CSlrString *windowTitle = new CSlrString("Export C64 keyboard mapping");
	viewC64->ShowDialogSaveFile(this, &extKeyMap, defaultFileName, gUTFPathToDocuments, windowTitle);
	delete windowTitle;
	delete defaultFileName;

}



void CViewC64KeyMap::DoLogic()
{
}

CViewC64KeyMapKeyData *CViewC64KeyMap::AddButtonKey(char *keyName1, char *keyName2, float x, float y, float width, int matrixRow, int matrixCol)
{
	CViewC64KeyMapKeyData *keyData = new CViewC64KeyMapKeyData();
	keyData->name1 = keyName1;
	keyData->name2 = keyName2;
	keyData->x = x;
	keyData->y = y;
	keyData->width = width;
	keyData->xl = x + width + fontWidth*0.50f - fontWidth*0.15f;
	keyData->matrixRow = matrixRow;
	keyData->matrixCol = matrixCol;
	
	int code = KeyCodeFromRowCol(matrixRow, matrixCol);
	
//	LOGD("AddButtonKey: code=%d matrixRow=%d matrixCol=%d", code, matrixRow, matrixCol);

	this->buttonKeys[code] = keyData;
	
	return keyData;
}

void CViewC64KeyMap::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64KeyMap::Render()
{
	
	BlitFilledRectangle(posX, posY, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB, 1.0);
	
	float sb = 20;
	float gap = 4;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextR;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = 3;
	
	float scrx = posX + sb;
	float scry = posY + sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = posX + scrsx/2.0f + sb*1.3;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy,
						viewC64->colorsTheme->colorBackgroundR,
						viewC64->colorsTheme->colorBackgroundG,
						viewC64->colorsTheme->colorBackgroundB, 1.0);
	
	float px = scrx + gap;
	float py = scry + gap;
	
	fontScale = 3.0f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;

	font->BlitTextColor(strHeader, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	py += fontHeight;
	//	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	//	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;
	
	
	//// TODO: this is a quick way to have it immediately implemented
	//// this needs of course to be changed into some more meaningful
	
	fontScale = 2.0f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;

	
	py += fontHeight;
	
	
	///
	
	float ls = fontWidth * 0.30f;
	float fu  = fontHeight * 0.30f;
	float fu2  = fu*2;
	float fh2 = fontHeight * 2.60f;
	
	float lineR = viewC64->colorsTheme->colorVerticalLineR;
	float lineG = viewC64->colorsTheme->colorVerticalLineG;
	float lineB = viewC64->colorsTheme->colorVerticalLineB;
	float lineA = 1.0f;
	
	// check shifts
	if (isShift)
	{
		BlitFilledRectangle(posX + keyLeftShift->x - fu, posY + keyLeftShift->y - fu, posZ, keyLeftShift->width + fu2, fh2, 0.7, 0.2, 0.2, 1);
		BlitFilledRectangle(posX + keyRightShift->x - fu, posY + keyRightShift->y - fu, posZ, keyRightShift->width + fu2, fh2, 0.7, 0.2, 0.2, 1);
	}
	
	for (std::map<int, CViewC64KeyMapKeyData *>::iterator it = this->buttonKeys.begin();
		 it != this->buttonKeys.end(); it++)
	{
		CViewC64KeyMapKeyData *keyData = it->second;
		
		float x = keyData->x;
		float y = keyData->y;
		
		if (selectedKeyData == keyData)
		{
			BlitFilledRectangle(posX + x - fu, posY + y - fu, posZ, keyData->width + fu2, fh2, 1, 0, 0, 1);
		}
		
		font->BlitText(keyData->name1, posX + x, posY + y, posZ, fontScale);
		font->BlitText(keyData->name2, posX + x, posY + y + fontHeight, posZ, fontScale);
		
		BlitFilledRectangle(posX + keyData->xl, posY + y - fu, posZ, ls, fh2, lineR, lineG, lineB, lineA);
	}
	
	// horizontal lines
	BlitFilledRectangle(posX + line1x, posY + line1y, posZ, line1sx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linefx, posY + line1y, posZ, linefsx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + line2x, posY + line2y, posZ, line2sx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linefx, posY + line2y, posZ, linefsx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + line3x, posY + line3y, posZ, line3sx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linefx, posY + line3y, posZ, linefsx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + line4x, posY + line4y, posZ, line4sx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linefx, posY + line4y, posZ, linefsx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + line5x, posY + line5y, posZ, line5sx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linefx, posY + line5y, posZ, linefsx, ls, lineR, lineG, lineB, lineA);

	// vertical
	BlitFilledRectangle(posX + line1x, posY + line1y, posZ, ls, linel1sy, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + line3x, posY + line3y, posZ, ls, linel2sy, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linefx, posY + line1y, posZ, ls, linefsy, lineR, lineG, lineB, lineA);

	// space
	BlitFilledRectangle(posX + linespcx, posY + linespc1y, posZ, linespcsx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linespcx, posY + linespc2y, posZ, linespcsx, ls, lineR, lineG, lineB, lineA);
	BlitFilledRectangle(posX + linespcx, posY + linespc1y, posZ, ls, linespcsy, lineR, lineG, lineB, lineA);

	
//	viewC64->viewC64Screen->RefreshScreen();
//	viewC64->viewC64Screen->Render();
	
	///
	//
	
	py = posY + 91;
	float startX = posX + 35;
	px = startX;
	
	char buf[256];
	char hostKeyName[64];
	
	float fontScale2 = fontPropScale * 3.0f;
	
	float rtsy = fontPropHeight * fontScale * 1.60f;
	
	if (this->selectedKeyData)
	{
		for (std::list<C64KeyCode *>::iterator it = this->selectedKeyData->keyCodes.begin();
			 it != this->selectedKeyData->keyCodes.end(); it++)
		{
			C64KeyCode *keyCode = *it;
			
//			sprintf(buf, "Key: %d", keyCode->keyCode);
//			font->BlitText(buf, px + 7, py+2, posZ, fontScale2);
//			px += 120;
//			LOGD("  %x", keyCode->keyCode);

//			CSlrString *strKeyCode = SYS_KeyCodeToString(keyCode->keyCode);
			CSlrString *strKeyCode = SYS_KeyUpperCodeToString(keyCode->keyCode);

			float f = 1.5f;
			float f2 = f/8;
			
			float w = fontProp->GetTextWidth(strKeyCode, fontScale2)*f;
			if (keyCode == this->selectedKeyCode)
			{
				BlitFilledRectangle(px, py, posZ, w + 7, rtsy, 0.7, 0.3, 0, 1);
			}
			else
			{
				BlitFilledRectangle(px, py, posZ, w + 7, rtsy, 0.3, 0.3, 0.3, 0.66);
			}
			

			fontProp->BlitTextColor(strKeyCode, px+3 + w*f2, py+rtsy/4, posZ, fontScale2, 1.0, 1.0, 1.0, 1.0);
			delete strKeyCode;

			px += w + 15;
			
			if (px > 200)
			{
				px = startX;
				py += rtsy + 3;
			}
			
			
//			px = startX;
//			
//			py += fontHeight * 1.5f;
		}
	}
	
	// render UI
	CGuiView::Render();
	
	this->font->BlitText("Host key:", posX + 35, posY + 75, posZ, 2.0f);
	
	if (isAssigningKey)
	{
		BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 0.7);
		
		float px = posX + sizeX/2 - 60;
		float py = posY + 150;
		font->BlitTextColor("Enter key", px, py, posZ, 2.5f, 1.0f, 1.0f, 1.0f, 1.0f); //, FONT_ALIGN_CENTER);
		
//		py += 25;
//		font->BlitTextColor(strKeyFunctionName, px, py, posZ, 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, FONT_ALIGN_CENTER);

	}
}


//NO_SHIFT = 0,             /* Key is not shifted. */
//VIRTUAL_SHIFT = (1 << 0), /* The key needs a shift on the real machine. */
//LEFT_SHIFT = (1 << 1),    /* Key is left shift. */
//RIGHT_SHIFT = (1 << 2),   /* Key is right shift. */
//ALLOW_SHIFT = (1 << 3),   /* Allow key to be shifted. */
//DESHIFT_SHIFT = (1 << 4), /* Although SHIFT might be pressed, do not
//						   press shift on the real machine. */
//ALLOW_OTHER = (1 << 5),   /* Allow another key code to be assigned if
//						   SHIFT is pressed. */
//SHIFT_LOCK = (1 << 6),    /* Key is shift lock. */


void CViewC64KeyMap::Render(float posX, float posY)
{
	this->Render();
}

//@returns is consumed
bool CViewC64KeyMap::DoTap(float x, float y)
{
	LOGG("CViewC64KeyMap::DoTap:  x=%f y=%f", x, y);
	
	if (isAssigningKey)
		return true;
	
	// check button tap first
	if (CGuiView::DoTapNoBackground(x, y) != false)
		return true;

	
	///
	if (this->selectedKeyData)
	{
		float py = posY + 91;
		float startX = posX + 35;
		float px = startX;
	
		float fontScale2 = fontPropScale * 3.0f;
		float rtsy = fontPropHeight * fontScale * 1.60f;
	
		for (std::list<C64KeyCode *>::iterator it = this->selectedKeyData->keyCodes.begin();
			 it != this->selectedKeyData->keyCodes.end(); it++)
		{
			C64KeyCode *keyCode = *it;
			
			//			sprintf(buf, "Key: %d", keyCode->keyCode);
			//			font->BlitText(buf, px + 7, py+2, posZ, fontScale2);
			//			px += 120;
			
			//			LOGD("  %x", keyCode->keyCode);
			
			float f = 1.5f;

			CSlrString *strKeyCode = SYS_KeyCodeToString(keyCode->keyCode);
			float w = fontProp->GetTextWidth(strKeyCode, fontScale2)*f;
			delete strKeyCode;
			
			if (x >= px && x <= px + w + 7 && y >= py && y <= py + rtsy)
			{
				this->selectedKeyCode = keyCode;
				PressSelectedKey(true);
				return true;
			}
			
			
			px += w + 15;
			
			if (px > 250)
			{
				px = startX;
				py += rtsy + 3;
			}
		}
	}

	
	float fu  = fontHeight * 0.30f;
	float fh2 = fontHeight * 2.60f;
	
	bool found = false;
	for (std::map<int, CViewC64KeyMapKeyData *>::iterator it = this->buttonKeys.begin();
		 it != this->buttonKeys.end(); it++)
	{
		CViewC64KeyMapKeyData *keyData = it->second;
		
		float kxs = posX + keyData->x - fu;
		float kxe = posX + keyData->x + keyData->width + fu;
		float kys = posY + keyData->y - fu;
		float kye = posY + keyData->y + fh2;
		
		if (x >= kxs && x <= kxe && y >= kys && y <= kye)
		{
			selectedKeyCode = NULL;
			SelectKey(keyData);
			found = true;
			break;
		}
	}
	
	if (found == false)
	{
		SelectKey(NULL);
	}

	return true;
}

extern "C" {
	extern int keyboard_key_pressed_matrix(int row, int column, int shift);
	extern int keyboard_set_latch_keyarr(int row, int col, int value);
	extern int keyboard_key_released_matrix(int row, int column, int shift);
	extern void c64d_keyboard_key_down_latch();
	extern void c64d_keyboard_key_up_latch();
}

void CViewC64KeyMap::SelectKey(CViewC64KeyMapKeyData *keyData)
{
	LOGD("CViewC64KeyMap::SelectKey");
	
	guiMain->LockMutex();
	
	if (keyData == NULL)
	{
		btnAssignKey->enabled = false;
		btnRemoveKey->enabled = false;
		selectedKeyData = NULL;
		selectedKeyCode = NULL;
		isShift = false;
	}
	else
	{
		btnAssignKey->enabled = true;
		btnRemoveKey->enabled = true;
		
		if (selectedKeyCode == NULL)
		{
			if ((keyData == keyLeftShift || keyData == keyRightShift) && selectedKeyData != NULL)
			{
				if (isShift == false)
				{
					isShift = true;
					
					for (std::list<C64KeyCode *>::iterator it = this->selectedKeyData->keyCodes.begin();
						 it != this->selectedKeyData->keyCodes.end(); it++)
					{
						C64KeyCode *keyCode = *it;
						if ((keyCode->shift & LEFT_SHIFT) == LEFT_SHIFT
							|| (keyCode->shift & RIGHT_SHIFT) == RIGHT_SHIFT)
						{
							selectedKeyCode = keyCode;
						}
					}
				}
				else
				{
					isShift = false;
					
					for (std::list<C64KeyCode *>::iterator it = this->selectedKeyData->keyCodes.begin();
						 it != this->selectedKeyData->keyCodes.end(); it++)
					{
						C64KeyCode *keyCode = *it;
						if (keyCode->shift == NO_SHIFT)
						{
							selectedKeyCode = keyCode;
						}
					}
				}
				PressSelectedKey(false);
			}
			else
			{
				selectedKeyData = keyData;
				isShift = false;
				PressSelectedKey(true);
			}
		}
		else
		{
			selectedKeyData = keyData;
			isShift = false;
			PressSelectedKey(true);
		}

	}
	
	guiMain->UnlockMutex();
}

void CViewC64KeyMap::PressSelectedKey(bool updateIfNotFound)
{
	LOGD("PressSelectedKey");
	
	if (selectedKeyData != NULL)
	{
		int row, col, shift;
		
		LOGD("row=%d col=%d name=%s size=%d", selectedKeyData->matrixRow, selectedKeyData->matrixCol, selectedKeyData->name1, selectedKeyData->keyCodes.size());

		bool found = false;
		for (std::list<C64KeyCode *>::iterator it = this->selectedKeyData->keyCodes.begin();
			 it != this->selectedKeyData->keyCodes.end(); it++)
		{
			C64KeyCode *keyCode = *it;
			if (keyCode == selectedKeyCode)
			{
				found = true;
				row = selectedKeyCode->matrixRow;
				col = selectedKeyCode->matrixCol;
				shift = selectedKeyCode->shift;
			}
		}

		if (found == false && updateIfNotFound)
		{
			if (selectedKeyData->keyCodes.empty() == false)
			{
				selectedKeyCode = selectedKeyData->keyCodes.front();
				row = selectedKeyCode->matrixRow;
				col = selectedKeyCode->matrixCol;
				shift = selectedKeyCode->shift;
				
				btnRemoveKey->enabled = true;
				found = true;
			}
		}
		
		if (found == false)
		{
			selectedKeyCode = NULL;
			row = selectedKeyData->matrixRow;
			col = selectedKeyData->matrixCol;
			if (isShift)
			{
				shift = LEFT_SHIFT;
			}
			else
			{
				shift = NO_SHIFT;
			}
			
			btnRemoveKey->enabled = false;
		}
		else
		{
			if (selectedKeyCode != NULL && ((selectedKeyCode->shift & LEFT_SHIFT) == LEFT_SHIFT
											|| (selectedKeyCode->shift & RIGHT_SHIFT) == RIGHT_SHIFT))
			{
				isShift = true;
			}
			else
			{
				isShift = false;
			}
		}
		
		// press key
		keyboard_key_clear();
		
		LOGD("pressed: row=%d col=%d shift=%d", row, col, shift);
		keyboard_key_pressed_matrix(row, col, shift);
		keyboard_set_latch_keyarr(row, col, 1);
		c64d_keyboard_key_down_latch();
		SYS_Sleep(50);
		LOGD("released: row=%d col=%d shift=%d", row, col, shift);
		keyboard_key_released_matrix(row, col, shift);
		keyboard_set_latch_keyarr(row, col, 0);
		c64d_keyboard_key_up_latch();
	}

}

void CViewC64KeyMap::ClearKeys()
{
	for (std::map<int, CViewC64KeyMapKeyData *>::iterator it = this->buttonKeys.begin();
		 it != this->buttonKeys.end(); it++)
	{
		CViewC64KeyMapKeyData *keyData = it->second;
		keyData->keyCodes.clear();
	}
	
	SelectKey(NULL);
}

void CViewC64KeyMap::UpdateFromKeyMap(C64KeyMap *keyMap)
{
	guiMain->LockMutex();
	
	this->keyMap = keyMap;
	
	ClearKeys();
	
	for (std::map<u32, C64KeyCode *>::iterator it = keyMap->keyCodes.begin();
		 it != keyMap->keyCodes.end(); it++)
	{
		C64KeyCode *keyCode = it->second;
		
		int code = KeyCodeFromRowCol(keyCode->matrixRow, keyCode->matrixCol);
		//LOGD("code=%d", code);

		std::map<int, CViewC64KeyMapKeyData *>::iterator itKey = buttonKeys.find(code);
		if (itKey == buttonKeys.end())
		{
			LOGError("CViewC64KeyMap::UpdateFromKeyMap: key not found for row=%d col=%d", keyCode->matrixRow, keyCode->matrixCol);
			continue;
		}
		
		CViewC64KeyMapKeyData *keyData = itKey->second;
		
		if (keyCode->shift == NO_SHIFT)
		{
			keyData->keyCodes.push_front(keyCode);
		}
		else
		{
			keyData->keyCodes.push_back(keyCode);
		}
	}
	
	guiMain->UnlockMutex();

}

void CViewC64KeyMap::AssignKey()
{
	if (selectedKeyData != NULL)
	{
		isAssigningKey = true;
	}
	else
	{
		LOGError("CViewC64KeyMap::AssignKey: selected is NULL");
		btnAssignKey->enabled = false;
		btnRemoveKey->enabled = false;
	}
}

void CViewC64KeyMap::AssignKey(u32 keyCode)
{
	LOGI("CViewC64KeyMap::AssignKey: keyCode=%d", keyCode);
	guiMain->LockMutex();
	isAssigningKey = false;
	
	std::map<u32, C64KeyCode *>::iterator it = keyMap->keyCodes.find(keyCode);
	
	if (it != keyMap->keyCodes.end())
	{
		char *buf = SYS_GetCharBuf();
		
		CSlrString *strKey = SYS_KeyCodeToString(keyCode);
		
		char *asciiKey = strKey->GetStdASCII();
		
		sprintf(buf, "Key   %s   is already assigned", asciiKey);
		viewC64->ShowMessage(buf);
		SYS_ReleaseCharBuf(buf);
		
		delete [] asciiKey;
		
		SelectKeyCode(keyCode);
		
		LOGI("CViewC64KeyMap::AssignKey: keyCode=%d is already assigned", keyCode);
		guiMain->UnlockMutex();
		
		SaveKeyboardMapping();
		return;
	}
	
	
	// assign key
	C64KeyCode *key = new C64KeyCode();
	
	key->keyCode = keyCode;
	key->matrixCol = selectedKeyData->matrixCol;
	key->matrixRow = selectedKeyData->matrixRow;
	key->shift = NO_SHIFT;
	
	if (isShift)
	{
		key->shift |= LEFT_SHIFT;
	}
	
	LOGD("CViewC64KeyMap::AssignKey: assigned keyCode=%d col=%d row=%d shift=%d", key->keyCode, key->matrixCol, key->matrixRow, key->shift);
	
	keyMap->AddKeyCode(key);
	selectedKeyData->keyCodes.push_back(key);
	
	selectedKeyCode = key;
	
	btnRemoveKey->enabled = true;
	
	guiMain->UnlockMutex();
	
	SaveKeyboardMapping();
}


void CViewC64KeyMap::RemoveSelectedKey()
{
	if (selectedKeyData != NULL && selectedKeyCode != NULL)
	{
		guiMain->LockMutex();
		
		std::map<u32, C64KeyCode *>::iterator it = keyMap->keyCodes.find(selectedKeyCode->keyCode);
		
		if (it == keyMap->keyCodes.end())
		{
			LOGError("CViewC64KeyMap::RemoveSelectedKey: key not found in keyMap (keyCode=%x)", selectedKeyCode->keyCode);
		}
		else
		{
			keyMap->keyCodes.erase(it);
		}
		
		selectedKeyData->keyCodes.remove(selectedKeyCode);
		delete selectedKeyCode;
		
		if (selectedKeyData->keyCodes.empty())
		{
			selectedKeyCode = NULL;
		}
		else
		{
			selectedKeyCode = selectedKeyData->keyCodes.front();
			
			if ((selectedKeyCode->shift & LEFT_SHIFT) == LEFT_SHIFT
				|| (selectedKeyCode->shift & RIGHT_SHIFT) == RIGHT_SHIFT)
			{
				isShift = true;
			}
			else
			{
				isShift = false;
			}
		}
		
		guiMain->UnlockMutex();
	}
	
	if (selectedKeyCode == NULL)
	{
		btnRemoveKey->enabled = false;
	}
	
	SaveKeyboardMapping();
}


bool CViewC64KeyMap::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	///
//	keyCode = 33;
//	isShift = false;
//	isAlt = false;
//	isControl = false;
	
	LOGI("CViewC64KeyMap::KeyDown: keyCode=%d %d %d %d", keyCode, isShift, isAlt, isControl);

	// block repeats first
	std::map<u32, bool>::iterator it = pressedKeyCodes.find(keyCode);
	
	if (it == pressedKeyCodes.end())
	{
		pressedKeyCodes[keyCode] = true;
	}
	else
	{
		// key is already pressed
		LOGD("key %d is already pressed, skipping...", keyCode);
		return true;
	}

	//
	
	if (isAssigningKey)
	{
		LOGD("...isAssigningKey...");
#if !defined(WIN32)
		if (keyCode == MTKEY_LALT || keyCode == MTKEY_RALT || keyCode == MTKEY_LCONTROL || keyCode == MTKEY_RCONTROL
			|| keyCode == MTKEY_LSHIFT || keyCode == MTKEY_RSHIFT)
#else

		// windows fucked up stuff
		if (keyCode == MTKEY_RALT || keyCode == MTKEY_LCONTROL || keyCode == MTKEY_RCONTROL
			|| keyCode == MTKEY_LSHIFT || keyCode == MTKEY_RSHIFT)
#endif

		{
			return true;
		}

		keyCode = SYS_KeyCodeConvertSpecial(keyCode, isShift, isAlt, isControl, isSuper);
		
		AssignKey(keyCode);
		return true;
	}
	
//	CSlrKeyboardShortcut *shortcut = guiMain->keyboardShortcuts->FindGlobalShortcut(keyCode, isShift, isAlt, isControl, isSuper);
//	if (shortcut == viewC64->mainMenuBar->kbsMainMenuScreen)
//	{
//		SaveAndGoBack();
//		return true;
//	}
	
	SelectKeyCode(keyCode);

	return true;
}

void CViewC64KeyMap::SelectKeyCode(u32 keyCode)
{
	guiMain->LockMutex();
	
	if (selectedKeyData != NULL && (keyCode == MTKEY_LSHIFT || keyCode == MTKEY_RSHIFT))
	{
		selectedKeyCode = NULL;
		this->SelectKey(keyLeftShift);
	}
	else
	{
		std::map<u32, C64KeyCode *>::iterator it = keyMap->keyCodes.find(keyCode);
		
		if (it != keyMap->keyCodes.end())
		{
			C64KeyCode *c64KeyCode = it->second;
			
			int code = KeyCodeFromRowCol(c64KeyCode->matrixRow, c64KeyCode->matrixCol);
			LOGD("code=%d row=%d col=%d", code, c64KeyCode->matrixRow, c64KeyCode->matrixCol);

			std::map<int, CViewC64KeyMapKeyData *>::iterator itKey = buttonKeys.find(code);
			if (itKey == buttonKeys.end())
			{
				LOGError("CViewC64KeyMap::KeyDown: not consistent: key not found for row=%d col=%d", c64KeyCode->matrixRow, c64KeyCode->matrixCol);
				guiMain->UnlockMutex();
				return;
			}
			
			this->selectedKeyCode = c64KeyCode;
			this->SelectKey(itKey->second);
		}
	}
	
	guiMain->UnlockMutex();
}

bool CViewC64KeyMap::DoFinishTap(float x, float y)
{
	LOGG("CViewC64KeyMap::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64KeyMap::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64KeyMap::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64KeyMap::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64KeyMap::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64KeyMap::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64KeyMap::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64KeyMap::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64KeyMap::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64KeyMap::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64KeyMap::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64KeyMap::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewC64KeyMap::SwitchScreen()
{
	if (guiMain->currentView == this)
	{
		guiMain->SetView(viewC64->mainMenuHelper);
	}
	else
	{
		guiMain->SetView(this);
	}
}

bool CViewC64KeyMap::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64KeyMap::KeyUp: keyCode=%d %d %d %d", keyCode, isShift, isAlt, isControl);

	std::map<u32, bool>::iterator it = pressedKeyCodes.find(keyCode);
	
	if (it == pressedKeyCodes.end())
	{
		// key is already not pressed
		LOGD("key %d is already not pressed, skipping...", keyCode);
		return true;
	}
	else
	{
		pressedKeyCodes.erase(it);
	}

	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64KeyMap::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64KeyMap::ActivateView()
{
	LOGG("CViewC64KeyMap::ActivateView()");
	
//	viewC64->viewC64Screen->SetVisible(true);
	
//	float scale = 0.676f;
//	float sx = (float)viewC64->debugInterfaceC64->GetScreenSizeX() * scale;
//	float sy = (float)viewC64->debugInterfaceC64->GetScreenSizeY() * scale;
//	float px = this->sizeX - sx;
//	float py = 0.0;//this->sizeY - sy;
//	viewC64->viewC64Screen->SetPosition(px, py, sx, sy);

}

void CViewC64KeyMap::DeactivateView()
{
	LOGG("CViewC64KeyMap::DeactivateView()");
	
	// refresh pos
//	viewC64->SwitchToScreenLayout(viewC64->currentScreenLayoutId);
}

