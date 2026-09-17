#include <math.h>
#include "GUI_Main.h"
#include "CViewDataDump.h"
#include "RES_ResourceManager.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "C64Tools.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CDebugDataAdapter.h"
#include "CDataAddressEditBox.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "SYS_SharedMemory.h"
#include "CViewC64VicDisplay.h"
#include "CSnapshotsManager.h"
#include "CLayoutParameter.h"
#include "CMainMenuBar.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"

#include "VID_Blits.h"
#include "SYS_Platform.h"

#include "CViewC64Screen.h"
#include "CViewAtariScreen.h"
#include "CViewNesScreen.h"
#include "MT_UiScale.h"

// ASCII -> C64 Screen Code mapping tables
// 0xFF = unmapped key (ignored)

static const u8 kAsciiToC64ScreenCodeUppercase[128] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x00-0x0F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x10-0x1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 0x20-0x2F: space ! " # $ % & ' ( ) * + , - . /
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 0x30-0x3F: 0-9 : ; < = > ?
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 0x40-0x4F: @ A-O
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0xFF, 0x1D, 0xFF, 0xFF, // 0x50-0x5F: P-Z [ \ ] ^ _
    0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 0x60-0x6F: ` a-o (same as A-O)
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x70-0x7F: p-z (same as P-Z)
};

static const u8 kAsciiToC64ScreenCodeLowercase[128] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x00-0x0F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x10-0x1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 0x20-0x2F: space ! " # $ % & ' ( ) * + , - . /
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 0x30-0x3F: 0-9 : ; < = > ?
    0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // 0x40-0x4F: @ A-O (uppercase glyphs in lowercase mode)
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x1B, 0xFF, 0x1D, 0xFF, 0xFF, // 0x50-0x5F: P-Z [ \ ] ^ _
    0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 0x60-0x6F: ` a-o (lowercase)
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x70-0x7F: p-z (lowercase)
};

static const u8 kAsciiToAtariScreenCode[128] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x00-0x0F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x10-0x1F
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 0x20-0x2F: space ! " # $ % & ' ( ) * + , - . /
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, // 0x30-0x3F: 0-9 : ; < = > ?
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 0x40-0x4F: @ A-O
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 0x50-0x5F: P-Z [ \ ] ^ _
    0xFF, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, // 0x60-0x6F: ` a-o
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x70-0x7F: p-z
};

static const u8 kAsciiPassthrough[128] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x00-0x0F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x10-0x1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 0x20-0x2F
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 0x30-0x3F
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // 0x40-0x4F
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, // 0x50-0x5F
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, // 0x60-0x6F
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0xFF, // 0x70-0x7F
};

CViewDataDump::CViewDataDump(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
							 CDebugSymbols *symbols, CViewDataMap *viewMemoryMap, CViewDisassembly *viewDisassembly)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->symbols = symbols;
	this->debugMemory = symbols->memory;
	this->debugInterface = symbols->debugInterface;
	this->dataAdapter = symbols->dataAdapter;
	this->viewMemoryMap = viewMemoryMap;
	this->viewDisassembly = viewDisassembly;
	
	editBoxHex = NULL;
	dataAddressEditBox = NULL;
	
	if (this->viewMemoryMap)
	{
		this->viewMemoryMap->SetDataDumpView(this);
	}
	
	SetDataAdapter(dataAdapter);
	
	fontDefaultCBM1 = viewC64->fontDefaultCBM1;
	fontDefaultCBM2 = viewC64->fontDefaultCBM2;
	fontCBM1 = viewC64->fontCBM1;
	fontCBM2 = viewC64->fontCBM2;
	fontAtari = viewC64->fontAtari;
	hasCustomChargenRom = viewC64->hasCustomChargenRom;

	selectedCharset = 0;
	charsetCombo = NULL;
	RebuildCharsetCombo();
	fontCharacters = fonts[0];

	fontSize = MT_UiScaled(7.0f);
	
	fontBytes = viewC64->fontDisassembly;
	
	dataShowStart = 0;
	dataShowEnd = 0;
	dataShowSize = 0;
	currentDataIndex = 0;
	
	numberOfBytesPerLine = 8;
	
	numberOfLines = 0;
	editCursorPositionX = 0;
	editCursorPositionY = 0;
	
	// FIXME: this must be taken from here and computed over sizeY
	numberOfCharactersToShow = 24;
	numberOfSpritesToShow = 16;
	
	showCharacters = true;
	showDataCharacters = true;
	showSprites = true;
		
	editBoxHex = new CGuiEditHex(this);
	editBoxHex->isCapitalLetters = false;
	isEditingValue = false;
	isCharacterEditing = false;

	isEditingValueAddr = false;

	selectionStartAddr = -1;
	selectionEndAddr = -1;
	isSelecting = false;
	autoScrollLastTime = 0;

	strTemp = new CSlrString();

	renderDataWithColors = false;

	// Character dirty-tracking init
	prevCharacterDataBuf = NULL;
	prevCharacterDataBufSize = 0;
	prevCharUseColors = false;
	prevCharColorD021 = prevCharColorD022 = prevCharColorD023 = prevCharColorD800 = 0;

	//
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));
	AddLayoutParameter(new CLayoutParameterInt("Bytes per line", &numberOfBytesPerLine));
	AddLayoutParameter(new CLayoutParameterBool("Characters ", &showDataCharacters));
	AddLayoutParameter(new CLayoutParameterBool("Character graphics", &showCharacters));
	AddLayoutParameter(new CLayoutParameterBool("Sprites", &showSprites));

	charsetCombo = new CLayoutParameterCombo("Charset", &selectedCharset, charsetComboItems, 0);
	RebuildCharsetCombo();
	AddLayoutParameter(charsetCombo);

	// init images for characters
	for (int i = 0; i < numberOfCharactersToShow; i++)
	{
		// alloc image that will store character pixels
		CImageData *imageData = new CImageData(16, 16, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		charactersImageData.push_back(imageData);
		
		/// init CSlrImage with empty image (will be deleted by loader)
		CImageData *emptyImageData = new CImageData(16, 16, IMG_TYPE_RGBA);
		emptyImageData->AllocImage(false, true);
		
		CSlrImage *imageCharacter = new CSlrImage(true, false);
		imageCharacter->LoadImage(emptyImageData, RESOURCE_PRIORITY_STATIC, false);
		imageCharacter->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		imageCharacter->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(imageCharacter, NULL);

		charactersImages.push_back(imageCharacter);
	}
	
	// init images for sprites
	for (int i = 0; i < numberOfSpritesToShow; i++)
	{
		// alloc image that will store sprite pixels
		CImageData *imageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		spritesImageData.push_back(imageData);
		
		/// init CSlrImage with empty image (will be deleted by loader)
		CImageData *emptyImageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		emptyImageData->AllocImage(false, true);
		
		CSlrImage *imageSprite = new CSlrImage(true, false);
		imageSprite->LoadImage(emptyImageData, RESOURCE_PRIORITY_STATIC, false);
		imageSprite->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		imageSprite->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(imageSprite, NULL);
		
		spritesImages.push_back(imageSprite);
	}
	
	previousClickTime = SYS_GetCurrentTimeInMillis();
	
	this->SetPosition(posX, posY, sizeX, sizeY, true);
}

CViewDataDump::~CViewDataDump()
{
	delete[] prevCharacterDataBuf;
}

void CViewDataDump::SetDataAdapter(CDebugDataAdapter *newDataAdapter)
{
	ClearSelection();
	this->dataAdapter = newDataAdapter;
	if (this->dataAddressEditBox)
		delete this->dataAddressEditBox;
	this->dataAddressEditBox = NULL;
	
	if (this->dataAdapter)
	{
		this->dataAddressEditBox = dataAdapter->CreateDataAddressEditBox(this);
	}
}

void CViewDataDump::SetPosition(float posX, float posY, float sizeX, float sizeY)
{
	RecalculateFontSizes();
	
	CGuiView::SetPosition(posX, posY, sizeX, sizeY);
}

void CViewDataDump::SetPosition(float posX, float posY, float sizeX, float sizeY, bool recalculateFontSizes)
{
	if (recalculateFontSizes)
	{
		this->SetPosition(posX, posY, sizeX, sizeY);
	}
	else
	{
		CGuiView::SetPosition(posX, posY, sizeX, sizeY);
	}
}

void CViewDataDump::RecalculateFontSizes()
{
	// calculate font sizes
	
	// workaround for broken layout
	if (fontSize < 0.01f)
	{
		fontSize = MT_UiScaled(5.0f);
	}
		
//	fontBytesSize = 10.0f * fontScale;
//	fontCharactersSize = 2.1f * fontScale;
//	fontCharactersWidth = 8.4f * fontScale;

	fontBytesSize = fontSize;
	fontCharactersSize = fontSize * 0.21f;
	fontCharactersWidth = fontSize * 0.84f;
	
	markerSizeX = fontSize*2.0f;
	markerSizeY = fontSize;

	gapAddress = fontSize;
	gapHexData = 0.5f*fontBytesSize;
	gapDataCharacters = 0.5f*fontBytesSize;
}

void CViewDataDump::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	RecalculateFontSizes();
}

void CViewDataDump::DoLogic()
{
}

bool CViewDataDump::FindDataPosition(float x, float y, int *dataPositionX, int *dataPositionY, int *dataPositionAddr)
{
	return FindDataPositionHex(x, y, dataPositionX, dataPositionY, dataPositionAddr)
		|| FindDataPositionCharacter(x, y, dataPositionX, dataPositionY, dataPositionAddr);
}

bool CViewDataDump::FindDataPositionHex(float x, float y, int *dataPositionX, int *dataPositionY, int *dataPositionAddr)
{
	float fontBytesSize40 = (float)(dataAddressEditBox->GetNumDigits()) * fontBytesSize;
	float fontBytesSize40_gap = fontBytesSize40 + gapAddress;
	float fontBytesSize20_gap = 2.0*fontBytesSize + gapHexData;

	float px = posX;
	float py = posY;

	int addr = dataShowStart;

	int dy = 0;
	while (true)
	{
		px = posX;
		int a = addr;

		px += fontBytesSize40_gap;

		float nextpy = py + fontCharactersWidth;

		// data bytes
		for (int dx = 0; dx < numberOfBytesPerLine; dx++)
		{
			float nextpx = px + 2.0*fontBytesSize;
			if (y >= py && y <= nextpy && x >= px && x <= nextpx)
			{
				*dataPositionX = dx;
				*dataPositionY = dy;
				*dataPositionAddr = a;

				return true;
			}

			a++;
			px += fontBytesSize20_gap;
		}

		addr += numberOfBytesPerLine;

		dy++;

		py += fontCharactersWidth;
		if (py+fontCharactersWidth > posEndY)
			break;
	}

	return false;
}

bool CViewDataDump::FindDataPositionCharacter(float x, float y, int *dataPositionX, int *dataPositionY, int *dataPositionAddr)
{
	if (!showDataCharacters)
		return false;

	float fontBytesSize40 = (float)(dataAddressEditBox->GetNumDigits()) * fontBytesSize;
	float fontBytesSize40_gap = fontBytesSize40 + gapAddress;
	float fontBytesSize20_gap = 2.0*fontBytesSize + gapHexData;
	float fontBytesSize05 = gapDataCharacters;

	float px = posX;
	float py = posY;

	int addr = dataShowStart;

	int dy = 0;
	while (true)
	{
		// Skip past address + hex bytes to get to character panel start
		px = posX + fontBytesSize40_gap + numberOfBytesPerLine * fontBytesSize20_gap + fontBytesSize05;
		int a = addr;

		float nextpy = py + fontCharactersWidth;

		for (int dx = 0; dx < numberOfBytesPerLine; dx++)
		{
			float nextpx = px + fontCharactersWidth;
			if (y >= py && y <= nextpy && x >= px && x <= nextpx)
			{
				*dataPositionX = dx;
				*dataPositionY = dy;
				*dataPositionAddr = a;

				return true;
			}

			a++;
			px += fontCharactersWidth;
		}

		addr += numberOfBytesPerLine;

		dy++;

		py += fontCharactersWidth;
		if (py+fontCharactersWidth > posEndY)
			break;
	}

	return false;
}

int CViewDataDump::GetAddrFromDataPosition(int dataPositionX, int dataPositionY)
{
	return dataShowStart + dataPositionY*numberOfBytesPerLine + dataPositionX + dataAdapter->GetDataOffset();
}

void CViewDataDump::RebuildCharsetCombo()
{
	int n = 0;
	charsetComboItems[n] = "C64 upper case";  fonts[n] = fontDefaultCBM1; n++;
	charsetComboItems[n] = "C64 lower case";  fonts[n] = fontDefaultCBM2; n++;
	if (hasCustomChargenRom)
	{
		charsetComboItems[n] = "C64 user ROM upper case";  fonts[n] = fontCBM1; n++;
		charsetComboItems[n] = "C64 user ROM lower case";  fonts[n] = fontCBM2; n++;
	}
	if (fontAtari != NULL)
	{
		charsetComboItems[n] = "Atari 800";  fonts[n] = fontAtari; n++;
	}
	if (charsetCombo)
	{
		charsetCombo->itemsCount = n;
	}
	if (selectedCharset >= n)
	{
		selectedCharset = 0;
	}
}

void CViewDataDump::RenderImGui()
{
	PreRenderImGui();

	// Sync user ROM font pointers (may change when custom ROM is loaded at runtime)
	if (fontCBM1 != viewC64->fontCBM1)
	{
		fontCBM1 = viewC64->fontCBM1;
		fontCBM2 = viewC64->fontCBM2;
		hasCustomChargenRom = viewC64->hasCustomChargenRom;
		RebuildCharsetCombo();
	}

	fontCharacters = fonts[selectedCharset];

	this->Render();
	CGuiView::PostRenderImGui();
}

void CViewDataDump::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;

	char *buf = SYS_GetCharBuf();
	
	float px = posX;
	float py = posY;
	
	int addrOffset = dataAdapter->GetDataOffset();
	int addr = dataShowStart;
		
	int dy = 0;
	
	int numberOfLines2 = numberOfLines/2;
	
	float cx = 0.0f;
	float cy = 0.0f;
	
	float fontBytesSize40 = fontBytesSize * (float)dataAddressEditBox->GetNumDigits();
	float fontBytesSize40_gap = fontBytesSize40 + gapAddress;
	float fontBytesSize20_gap = 2.0*fontBytesSize + gapHexData;
	float fontBytesSize05 = gapDataCharacters; //0.5f*fontBytesSize;
	
	while (true)
	{
		px = posX;
		int a = addr;
		
		if (isEditingValueAddr == false)
		{
//			if (isVisibleEditCursor == false)
//			{
//				sprintfHexCode16(buf, addr);
//			}
//			else
			{
				if (dy == editCursorPositionY)
				{
					int addrEdit = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);

					dataAdapter->GetAddressStringForCell(addrEdit, buf, MAX_STRING_LENGTH);
	
					BlitFilledRectangle(px, py, posZ, fontBytesSize40, fontBytesSize, 0.0f, 0.7f, 0.0f, 1.0f);
				}
				else
				{
					dataAdapter->GetAddressStringForCell(addr + addrOffset, buf, MAX_STRING_LENGTH);
				}
			}
			fontBytes->BlitText(buf, px, py, posZ, fontBytesSize);
			px += ((float)strlen(buf)) * fontBytesSize + gapAddress;
		}
		else
		{
			if (dy == numberOfLines2)
			{
//				fontBytes->BlitTextColor(editBoxHex->textWithCursor, px, py, posZ, fontBytesSize, 1.0f, 1.0f, 1.0f, 1.0f);
				dataAddressEditBox->Render(px, py, fontBytes, fontBytesSize);
				
				px += ((float)dataAddressEditBox->GetNumDigits()) * fontBytesSize + gapAddress;
			}
			else
			{
				dataAdapter->GetAddressStringForCell(addr + addrOffset, buf, MAX_STRING_LENGTH);
				fontBytes->BlitText(buf, px, py, posZ, fontBytesSize);
				px += ((float)strlen(buf)) * fontBytesSize + gapAddress;
			}
		}
		
		// data bytes
		for (int dx = 0; dx < numberOfBytesPerLine; dx++)
		{
			bool isAvailable;
			uint8 value;
			
			dataAdapter->AdapterReadByte(a, &value, &isAvailable);
			if (isAvailable)
			{
				CDebugMemoryCell *cell = debugMemory->GetMemoryCell(a);
				
				if (cell)
				{
					if (cell->isExecuteCode)
					{
						BlitFilledRectangle(px, py, posZ, markerSizeX, markerSizeY,
											colorExecuteCodeR, colorExecuteCodeG, colorExecuteCodeB, colorExecuteCodeA);
					}
					else if (cell->isExecuteArgument)
					{
						BlitFilledRectangle(px, py, posZ, markerSizeX, markerSizeY,
											colorExecuteArgumentR, colorExecuteArgumentG, colorExecuteArgumentB, colorExecuteArgumentA);
					}
					BlitFilledRectangle(px, py, posZ, markerSizeX, markerSizeY, cell->sr, cell->sg, cell->sb, cell->sa);

					// Selection highlight (hex panel)
					if (IsAddressSelected(a))
					{
						BlitFilledRectangle(px, py, posZ, fontBytesSize*2.0f, fontCharactersWidth, 0.55f, 0.55f, 1.0f, 1.0f);
					}

					//BlitTextColor(CSlrString *text, float posX, float posY, float posZ, float scale, float colorR, float colorG, float colorB, float alpha)

					// TODO: refactor refresh of memory map cell colors and skip visibility and update, change so debugmemory has a list of consumers, if no consumers are visible skip this update UpdateCellColors, if any consumer visible do for each cell once per frame
					if (this->viewMemoryMap)
					{
						if (viewMemoryMap->visible == false)
						{
							cell->UpdateCellColors(value, viewMemoryMap->showCurrentExecutePC, symbols->GetCurrentExecuteAddr());
						}
					}
					else
					{
						cell->UpdateCellColors(value, false, 0);
					}
					
					////////// refactor^^^^

					if (/*isVisibleEditCursor &&*/ editCursorPositionY == dy && editCursorPositionX == dx)
					{
						cx = px;
						cy = py;
						
						if (isEditingValue == false)
						{
							sprintfHexCode8(buf, value);

							CDebugMemoryCell *cell = debugMemory->GetMemoryCell(this->currentDataIndex);
	//						fontBytes->BlitTextColor(buf, px, py, posZ, fontBytesSize,

	//						fontBytesSize*2.0f, fontCharactersWidth, 0.3f,  1.0f, 0.3f, 0.5f, 1.3f);

							float r = cell->rr;
							float g = cell->rg;
							float b = cell->rb;
							float a = cell->ra;

							// prevent white color on white background
							float s = cell->rr + cell->rg + cell->rb;
							
	//						if (s > 2.7f)
	//						{
								r = cell->rr < 0.40f ? cell->rr : 0.4f;
								g = cell->rg < 0.40f ? cell->rg : 0.4f;
								b = cell->rb < 0.40f ? cell->rb : 0.4f;
								a = cell->ra;
	//						}
	//						else if (s > 1.2f)
	//						{
	//							r = cell->rr < 0.5f ? cell->rr : 0.6f;
	//							g = cell->rg < 0.5f ? cell->rg : 0.6f;
	//							b = cell->rb < 0.5f ? cell->rb : 0.6f;
	//							a = cell->ra;
	//						}

							if (isCharacterEditing)
							{
								// Outline only — cursor is active in character panel
								BlitRectangle(px, py, posZ, fontBytesSize*2.0f, fontCharactersWidth, 0.5f, 0.5f, 0.5f, 0.7f);
							}
							else
							{
								// Filled — cursor is active in hex panel
								BlitFilledRectangle(px, py, posZ, fontBytesSize*2.0f, fontCharactersWidth, r, g, b, a);
							}

	//
	//						viewC64->fontDisassemblyInverted->BlitTextColor(buf, px, py, posZ, fontBytesSize, cell->rb, cell->rg, cell->rr, 1.0f);
							viewC64->fontDisassemblyInverted->BlitTextColor(buf, px, py, posZ, fontBytesSize, 1.0f, 1.0f, 1.0f, 0.7f);
						}
						else
						{
	//						if (cell->isExecute)
							{
	//							LOGD("editHex->textWithCursor=%s", editHex->textWithCursor);
	//							fontBytes->BlitTextColor(editHex->textWithCursor, px, py, posZ, fontBytesSize, 1.0f, 1.0f, 1.0f, 1.0f);
	//							viewC64->fontDisassemblyInverted->BlitTextColor(editHex->textWithCursor, px, py, posZ, fontBytesSize, 1.0f, 1.0f, 1.0f, 1.0f);
								fontBytes->BlitTextColor(editBoxHex->textWithCursor, px, py, posZ, fontBytesSize, 1.0f, 1.0f, 1.0f, 1.0f);
							}
	//						else
	//						{
	//							fontBytes->BlitTextColor(editHex->textWithCursor, px, py, posZ, fontBytesSize, 0.8f, 0.8f, 0.8f, 1.0f);
	//						}
						}
						
						currentDataIndex = a;
					}
					else
					{
						sprintfHexCode8(buf, value);
	//					if (cell->isExecute)
						{
							fontBytes->BlitTextColor(buf, px, py, posZ, fontBytesSize, 1.0f, 1.0f, 1.0f, 1.0f);
						}
	//					else
	//					{
	//						fontBytes->BlitTextColor(buf, px, py, posZ, fontBytesSize, 0.8f, 0.8f, 0.8f, 1.0f);
	//					}
					}
					
					
				}
			}
			
			a++;
			px += fontBytesSize20_gap;
		}
				
		// data characters
		if (showDataCharacters)
		{
			px += fontBytesSize05;

			a = addr;
			for (int dx = 0; dx < numberOfBytesPerLine; dx++)
			{
				u8 value;
				bool isAvailable;

				dataAdapter->AdapterReadByte(a, &value, &isAvailable);

				if (isAvailable)
				{
					// Selection highlight (character panel)
					if (IsAddressSelected(a))
					{
						BlitFilledRectangle(px, py, posZ, fontCharactersWidth, fontCharactersWidth, 0.55f, 0.55f, 1.0f, 1.0f);
					}

					// Draw cursor on the character panel
					if (editCursorPositionY == dy && editCursorPositionX == dx)
					{
						if (isCharacterEditing)
						{
							// Filled — cursor is active in character panel
							BlitFilledRectangle(px, py, posZ, fontCharactersWidth, fontCharactersWidth, 1.0f, 0.3f, 0.3f, 0.5f);
						}
						else
						{
							// Outline only — cursor is active in hex panel
							BlitRectangle(px, py, posZ, fontCharactersWidth, fontCharactersWidth, 0.5f, 0.5f, 0.5f, 0.7f);
						}
					}

					fontCharacters->BlitChar((u16)value, px, py, posZ, fontCharactersSize);
				}

				px += fontCharactersWidth;
				a++;
			}
		}

		addr += numberOfBytesPerLine;
		
		dy++;
		
		py += fontCharactersWidth;
		if (py+fontCharactersWidth > posEndY)
			break;
	}
	
	dataShowEnd = addr;
	dataShowSize = dataShowEnd - dataShowStart;
	numberOfLines = dy;
	
	
	//
	// blit data cursor
	//
	
	//if (isVisibleEditCursor)
	{
		if (cx >= posX-5 && cx <= posEndX+5
			&& cy >= posY-5 && cy <= posEndY+5)
		{
//			BlitFilledRectangle(cx, cy, posZ, fontBytesSize*2.0f, fontCharactersWidth, 0.6f,  0.6f, 0.6f, 0.4f);
			BlitRectangle(cx, cy, posZ, fontBytesSize*2.0f, fontCharactersWidth, 0.3f,  1.0f, 0.3f, 0.5f, 1.3f);
		}
	}
//	else
//	{
//		dataAddr = dataShowStart;
//	}
	

	
	// get VIC colors
	u8 cD021, cD022, cD023;
	u8 cD800;
	u8 cD025, cD026, cD027;
	
	cD021 = viewC64->colorsToShow[1];
	cD022 = viewC64->colorsToShow[2];
	cD023 = viewC64->colorsToShow[3];
	cD800 = viewC64->colorToShowD800;
	
	cD025 = viewC64->colorsToShow[5];
	cD026 = viewC64->colorsToShow[6];
	cD027 = viewC64->colorsToShow[7];
	
	
	if (showCharacters)
	{
		// refresh texture of C64's character mode screen
		UpdateCharacters(renderDataWithColors, cD021, cD022, cD023, cD800);
		
		px += 5.0f;
		py = posY;
		
		const float characterSize = 32.0f;
		
		for (std::list<CSlrImage *>::iterator it = charactersImages.begin(); it != charactersImages.end(); it++)
		{
			CSlrImage *image = *it;
			
			const float characterTexStartX = 4.0/16.0;
			const float characterTexStartY = 4.0/16.0;
			const float characterTexEndX = (4.0+8.0)/16.0;
			const float characterTexEndY = (4.0+8.0)/16.0;
			
			//BlitFilledRectangle(px, py, posZ, 32, 32, 0.5, 0.5, 1.0f, 1.0f);
			
			Blit(image, px, py, posZ, characterSize, characterSize, characterTexStartX, characterTexStartY, characterTexEndX, characterTexEndY);
			
			py += characterSize + 10;
		}
		
		px += 40.0f;
	}
	else
	{
		px += 5.0f;
	}

	if (showSprites)
	{
		// render sprites
		UpdateSprites(renderDataWithColors, cD021, cD025, cD026, cD027);
		
		py = posY;
		
		const float scale = 1.9f;
		const float spriteSizeX = 24.0f * scale;
		const float spriteSizeY = 21.0f * scale;
		
		for (std::list<CSlrImage *>::iterator it = spritesImages.begin(); it != spritesImages.end(); it++)
		{
			CSlrImage *image = *it;
			
			const float spriteTexStartX = 4.0/32.0;
			const float spriteTexStartY = 4.0/32.0;
			const float spriteTexEndX = (4.0+24.0)/32.0;
			const float spriteTexEndY = (4.0+21.0)/32.0;
			
			//BlitFilledRectangle(px, py, posZ, 32, 32, 0.5, 0.5, 1.0f, 1.0f);
			
			Blit(image, px, py, posZ, spriteSizeX, spriteSizeY, spriteTexStartX, spriteTexStartY, spriteTexEndX, spriteTexEndY);
			
			py += spriteSizeY + 10;
		}
	}

	SYS_ReleaseCharBuf(buf);
}

void CViewDataDump::UpdateCharacters(bool useColors, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800)
{
	std::list<CSlrImage *>::iterator itImage = charactersImages.begin();
	std::list<CImageData *>::iterator itImageData =  charactersImageData.begin();

	int addr = currentDataIndex;

	if (addr < 0)
	{
		addr = 0;
	}
	if (addr >= dataAdapter->AdapterGetDataLength())
	{
		addr = dataAdapter->AdapterGetDataLength()-1;
	}

	int numChars = (int)charactersImages.size();
	int requiredBufSize = numChars * 8;

	// Reallocate tracking buffer if number of characters changed
	if (prevCharacterDataBufSize != requiredBufSize)
	{
		delete[] prevCharacterDataBuf;
		prevCharacterDataBuf = new uint8[requiredBufSize];
		memset(prevCharacterDataBuf, 0, requiredBufSize);
		prevCharacterDataBufSize = requiredBufSize;
	}

	// Check if global colors changed
	bool globalColorsChanged = (useColors != prevCharUseColors ||
								colorD021 != prevCharColorD021 ||
								colorD022 != prevCharColorD022 ||
								colorD023 != prevCharColorD023 ||
								colorD800 != prevCharColorD800);
	if (globalColorsChanged)
	{
		prevCharUseColors = useColors;
		prevCharColorD021 = colorD021;
		prevCharColorD022 = colorD022;
		prevCharColorD023 = colorD023;
		prevCharColorD800 = colorD800;
	}

	int charIndex = 0;
	while(itImage != charactersImages.end())
	{
		//LOGD("char#=%d dataAddr=%4.4x", charIndex, addr);

		CSlrImage *image = *itImage;
		CImageData *imageData = *itImageData;

		u8 characterData[8];

		for (int i = 0; i < 8; i++)
		{
			u8 v;
			dataAdapter->AdapterReadByte(addr, &v);
			characterData[i] = v;
			addr++;
		}

		uint8 *prevData = prevCharacterDataBuf + charIndex * 8;

		// Skip rebind if neither character data nor colors changed
		if (!globalColorsChanged && memcmp(characterData, prevData, 8) == 0)
		{
			itImage++;
			itImageData++;
			charIndex++;
			continue;
		}

		memcpy(prevData, characterData, 8);

		// TODO: make this generic
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			if (useColors == false)
			{
				ConvertCharacterDataToImage(characterData, imageData);
			}
			else
			{
				ConvertColorCharacterDataToImage(characterData, imageData, colorD021, colorD022, colorD023, colorD800,
												 (CDebugInterfaceC64*)debugInterface);
			}
		}
		else
		{
			ConvertCharacterDataToImage(characterData, imageData);
		}

		// re-bind image
		image->ReBindImageData(imageData);

		itImage++;
		itImageData++;
		charIndex++;
	}
}

void CViewDataDump::UpdateSprites(bool useColors, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027)
{
	std::list<CSlrImage *>::iterator itImage = spritesImages.begin();
	std::list<CImageData *>::iterator itImageData =  spritesImageData.begin();
	
	int addr = currentDataIndex;
	
//	int zi = 0;
	while(itImage != spritesImages.end())
	{
//		LOGD("sprite#=%d dataAddr=%4.4x", zi++, addr);
		
		CSlrImage *image = *itImage;
		CImageData *imageData = *itImageData;
		
		u8 spriteData[63];
		
		for (int i = 0; i < 63; i++)
		{
			u8 v;
			dataAdapter->AdapterReadByte(addr, &v);
			spriteData[i] = v;
			addr++;
		}
		
		// TODO: make this generic
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			if (useColors == false)
			{
				ConvertSpriteDataToImage(spriteData, imageData, 4);
			}
			else
			{
				ConvertColorSpriteDataToImage(spriteData, imageData,
											  colorD021, colorD025, colorD026, colorD027,
											  (CDebugInterfaceC64*)debugInterface, 4, 0);
			}
		}
		else
		{
			ConvertSpriteDataToImage(spriteData, imageData, 4);
		}
		
		addr++;
		
		// re-bind image
		image->ReBindImageData(imageData);
		
		itImage++;
		itImageData++;
	}
}


bool CViewDataDump::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
//	if (x >= posX && x <= posX+5*fontBytesSize)
//	{
//		isVisibleEditCursor = false;
//		return true;
//	}
	
	int dataPositionX;
	int dataPositionY;
	int dataPositionAddr;
	
	if (isEditingValue || isEditingValueAddr)
	{
		CancelEditingHexBox();
	}

	// Check which panel was clicked: hex or character
	bool foundHex = FindDataPositionHex(x, y, &dataPositionX, &dataPositionY, &dataPositionAddr);
	bool foundChar = false;
	if (!foundHex)
	{
		foundChar = FindDataPositionCharacter(x, y, &dataPositionX, &dataPositionY, &dataPositionAddr);
	}

	if (!foundHex && !foundChar)
	{
		guiMain->UnlockMutex();
		return true;
	}

	// Set editing mode based on which panel was clicked
	isCharacterEditing = foundChar;
	
//	if (isVisibleEditCursor && dataPositionX == editCursorPositionX && dataPositionY == editCursorPositionY)
//	{
//		isVisibleEditCursor = false;
//
//		if (isEditingValue)
//			editHex->FinalizeEntering(MTKEY_ENTER);
//
//		return true;
//	}

	if (guiMain->isControlPressed)
	{
		CDebugMemoryCell *cell = debugMemory->GetMemoryCell(dataPositionAddr);
		
		if (guiMain->isShiftPressed)
		{
			if (cell->readPC != -1)
			{
				if (viewDisassembly)
				{
					viewDisassembly->ScrollToAddress(cell->readPC);
				}
			}
			
			if (guiMain->isAltPressed)
			{
				if (debugInterface->snapshotsManager)
				{
					if (cell->readCycle != -1)
					{
						LOGM("============######################### RESTORE TO READ CYCLE=%d", cell->readCycle);
						debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->readCycle);
					}
				}
			}
		}
		else
		{
			if (cell->writePC != -1)
			{
				if (viewDisassembly)
				{
					viewDisassembly->ScrollToAddress(cell->writePC);
				}
			}
			
			if (guiMain->isAltPressed)
			{
				if (debugInterface->snapshotsManager)
				{
					if (cell->writeCycle != -1)
					{
						LOGM("============######################### RESTORE TO WRITE CYCLE=%d", cell->writeCycle);
						debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->writeCycle);
					}
				}
			}
		}
	}
	else
	{
		if (dataPositionAddr == previousClickAddr)
		{
			long time = SYS_GetCurrentTimeInMillis() - previousClickTime;
			if (time < c64SettingsDoubleClickMS)
			{
				// double click
				if (viewDisassembly)
				{
					viewDisassembly->ScrollToAddress(dataPositionAddr);
				}
			}
		}
	}
	
	previousClickTime = SYS_GetCurrentTimeInMillis();
	previousClickAddr = dataPositionAddr;
	
	editCursorPositionX = dataPositionX;
	editCursorPositionY = dataPositionY;

	isEditingValue = false;

	// Start selection
	selectionStartAddr = dataPositionAddr;
	selectionEndAddr = dataPositionAddr;
	isSelecting = true;
	autoScrollLastTime = 0;

//	isVisibleEditCursor = true;

	guiMain->UnlockMutex();
	return true;
}

bool CViewDataDump::DoScrollWheel(float deltaX, float deltaY)
{
	if (this->IsInside(guiMain->mousePosX, guiMain->mousePosY) == false)
	{
		return false;
	}
	
	LOGG("CViewDataDump::DoScrollWheel: %f %f", deltaX, deltaY);
	
	guiMain->LockMutex();
	
	int dy = fabs(round(deltaY));
	
	bool scrollUp = (deltaY > 0);
	
	for (int i = 0; i < dy; i++)
	{
		if (scrollUp)
		{
			ScrollDataUp();
		}
		else
		{
			ScrollDataDown();
		}
	}
	
	guiMain->UnlockMutex();
	return true;
	return false;
}

bool CViewDataDump::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (!isSelecting)
		return false;

	guiMain->LockMutex();

	int dataPositionX;
	int dataPositionY;
	int dataPositionAddr;

	bool found = FindDataPosition(x, y, &dataPositionX, &dataPositionY, &dataPositionAddr);

	if (found)
	{
		selectionEndAddr = dataPositionAddr;
		editCursorPositionX = dataPositionX;
		editCursorPositionY = dataPositionY;
	}
	else
	{
		// Auto-scroll when cursor is above or below the view
		long now = SYS_GetCurrentTimeInMillis();
		if (autoScrollLastTime == 0 || (now - autoScrollLastTime) >= 70)
		{
			if (y < posY)
			{
				// Cursor above view — scroll up
				if (dataShowStart > 0)
				{
					ScrollDataUp();
					// Extend selection to first byte of new top line
					selectionEndAddr = dataShowStart + dataAdapter->GetDataOffset();
					editCursorPositionX = 0;
					editCursorPositionY = 0;
					autoScrollLastTime = now;
				}
			}
			else if (y > posEndY)
			{
				// Cursor below view — scroll down
				int dataLength = dataAdapter->AdapterGetDataLength();
				if (dataShowStart + dataShowSize < dataLength)
				{
					ScrollDataDown();
					// Extend selection to last byte of new bottom line
					int lastLineAddr = dataShowStart + (numberOfLines - 1) * numberOfBytesPerLine;
					selectionEndAddr = lastLineAddr + numberOfBytesPerLine - 1 + dataAdapter->GetDataOffset();
					if (selectionEndAddr >= dataLength)
						selectionEndAddr = dataLength - 1;
					editCursorPositionX = numberOfBytesPerLine - 1;
					editCursorPositionY = numberOfLines - 1;
					autoScrollLastTime = now;
				}
			}
		}
	}

	guiMain->UnlockMutex();
	return true;
}

bool CViewDataDump::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	if (!isSelecting)
		return false;

	isSelecting = false;

	// If start == end, it was a click without drag — clear selection
	if (selectionStartAddr == selectionEndAddr)
	{
		selectionStartAddr = -1;
		selectionEndAddr = -1;
	}

	return true;
}

bool CViewDataDump::DoFinishTap(float x, float y)
{
	if (!isSelecting)
		return false;

	isSelecting = false;

	// Click without drag — clear selection
	if (selectionStartAddr == selectionEndAddr)
	{
		selectionStartAddr = -1;
		selectionEndAddr = -1;
	}

	return true;
}

bool CViewDataDump::GetSelectionRange(int *fromAddr, int *toAddr)
{
	if (selectionStartAddr < 0)
		return false;

	int s = selectionStartAddr;
	int e = selectionEndAddr;
	if (s > e)
	{
		int tmp = s;
		s = e;
		e = tmp;
	}

	// Clamp to valid data range
	int dataLength = dataAdapter->AdapterGetDataLength();
	int addrOffset = dataAdapter->GetDataOffset();
	if (s < addrOffset)
		s = addrOffset;
	if (e >= dataLength + addrOffset)
		e = dataLength + addrOffset - 1;

	*fromAddr = s;
	*toAddr = e;
	return true;
}

bool CViewDataDump::IsAddressSelected(int addr)
{
	if (selectionStartAddr < 0)
		return false;

	int s = selectionStartAddr;
	int e = selectionEndAddr;
	if (s > e)
	{
		int tmp = s;
		s = e;
		e = tmp;
	}
	return (addr >= s && addr <= e);
}

void CViewDataDump::ClearSelection()
{
	selectionStartAddr = -1;
	selectionEndAddr = -1;
	isSelecting = false;
}

void CViewDataDump::ScrollDataUp()
{
	if (isEditingValue || isEditingValueAddr)
		CancelEditingHexBox();

	int newDataAddr;
	newDataAddr = dataShowStart - numberOfBytesPerLine;
	if (newDataAddr < 0)
	{
		newDataAddr = 0;
	}
	
	dataShowStart = newDataAddr;
}

void CViewDataDump::ScrollDataPageUp()
{
	if (isEditingValue || isEditingValueAddr)
		CancelEditingHexBox();

	int newDataAddr;
	newDataAddr = dataShowStart - dataShowSize;
	if (newDataAddr < 0)
	{
		newDataAddr = 0;
	}
	
	dataShowStart = newDataAddr;
}

void CViewDataDump::ScrollDataDown()
{
	if (isEditingValue || isEditingValueAddr)
		CancelEditingHexBox();

	int newDataAddr;
	newDataAddr = dataShowStart + numberOfBytesPerLine;
	
	if (newDataAddr + dataShowSize > dataAdapter->AdapterGetDataLength())
	{
		newDataAddr = dataAdapter->AdapterGetDataLength() - dataShowSize;
	}
	
	dataShowStart = newDataAddr;
}

void CViewDataDump::ScrollDataPageDown()
{
	if (isEditingValue || isEditingValueAddr)
		CancelEditingHexBox();

	int newDataAddr;
	newDataAddr = dataShowEnd;
	
	if (newDataAddr + dataShowSize > dataAdapter->AdapterGetDataLength())
	{
		newDataAddr = dataAdapter->AdapterGetDataLength() - dataShowSize;
	}
	
	dataShowStart = newDataAddr;
}

void CViewDataDump::ScrollToAddress(int address)
{
	this->ScrollToAddress(address, true);
}

void CViewDataDump::ScrollToAddress(int address, bool updateDataShowStart)
{
	LOGG("CViewDataDump::ScrollToAddress: address=%4.4x", address);

	if (this->visible == false || numberOfBytesPerLine == 0)
		return;

	if (isEditingValue || isEditingValueAddr)
		return;

	if (address >= dataAdapter->AdapterGetDataLength())
	{
		address = dataAdapter->AdapterGetDataLength()-1;
	}
	
	int laddr = floor((double)(address / numberOfBytesPerLine)) * numberOfBytesPerLine;
	
	//LOGD("laddr=%4.4x", laddr);
	
	int saddr = laddr - (numberOfLines/2 * numberOfBytesPerLine);
	
	//LOGD("saddr=%4.4x", saddr);
	
	if (saddr < 0)
	{
		saddr = 0;
	}
	
	int saddr2 = saddr + (numberOfLines * numberOfBytesPerLine);
	
	if (saddr2 > dataAdapter->AdapterGetDataLength())
	{
		saddr = dataAdapter->AdapterGetDataLength() - dataShowSize;
	}
	
	int caddr = address - saddr;
	
	if (updateDataShowStart == false)
	{
		int caddr = address - dataShowStart;
		editCursorPositionY = floor((double)(caddr / numberOfBytesPerLine));
		editCursorPositionX = caddr - (numberOfBytesPerLine * editCursorPositionY);
//		LOGD("editCursorPositionX=%d / %d  |  editCursorPositionY=%d / %d", editCursorPositionX, numberOfBytesPerLine, editCursorPositionY, numberOfLines);
		
		// check if outside and then also scroll data start position
		if (editCursorPositionX < 0 || editCursorPositionX >= numberOfBytesPerLine
			|| editCursorPositionY < 0 || editCursorPositionY >= numberOfLines)
		{
			updateDataShowStart = true;
		}
	}
	
	if (updateDataShowStart)
	{
		dataShowStart = saddr;
		editCursorPositionY = floor((double)(caddr / numberOfBytesPerLine));
		editCursorPositionX = caddr - (numberOfBytesPerLine * editCursorPositionY);
	}
	else
	{
	}

	//isVisibleEditCursor = true;
	currentDataIndex = address;
}

bool CViewDataDump::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGG("CViewDataDump::keyDown=%4.4x", keyCode);
	
	u32 bareKey = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
	
	// change charset
	if ((keyCode == MTKEY_LSHIFT && isAlt) || (keyCode == MTKEY_LALT && isShift))
	{
		selectedCharset = (selectedCharset + 1) % charsetCombo->itemsCount;
		fontCharacters = fonts[selectedCharset];
		return false;
	}

	// Tab toggles between hex panel and character panel
	if (keyCode == MTKEY_TAB)
	{
		if (isEditingValue)
		{
			CancelEditingHexBox();
		}
		isCharacterEditing = !isCharacterEditing;
		return true;
	}

	CSlrKeyboardShortcut *keyboardShortcutMemory = guiMain->keyboardShortcuts->FindShortcut(KBZONE_MEMORY, bareKey, isShift, isAlt, isControl, isSuper);
	
	// check if editing value and go to address shortcut pressed
	if (isEditingValue && keyboardShortcutMemory == viewC64->mainMenuBar->kbsGoToAddress)
	{
		CancelEditingHexBox();
		
		// setting up the key shortcut to enter goto address will be handled later
	}
	
	if (isEditingValue)
	{
		editBoxHex->KeyDown(keyCode);
		return true;
	}
	
	if (isEditingValueAddr)
	{
		dataAddressEditBox->KeyDown(keyCode);
		return true;
	}

	// Character panel editing — type text mapped to screen codes
	if (isCharacterEditing)
	{
		// Don't intercept modifier-only keys, navigation, or shortcuts (Cmd/Ctrl+key)
		if (keyCode >= ' ' && keyCode < 128 && !isControl && !isSuper)
		{
			u8 screenCode = MapKeyToScreenCode(keyCode, isShift);
			if (screenCode != 0xFF)
			{
				int addr = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
				addr -= dataAdapter->GetDataOffset();
				dataAdapter->AdapterWriteByte(addr, screenCode);

				// Advance cursor to next byte (reuse arrow right logic for wrap/scroll)
				this->KeyDown(MTKEY_ARROW_RIGHT, false, false, false, false);
				return true;
			}
		}
	}

	// TODO: HOME should move to first item in current line, ctrl+HOME should move to beginning of document
	if (keyCode == MTKEY_HOME) // && isControl)
	{
		this->ScrollToAddress(0x0000);
		return true;
	}
	
	if (keyCode == MTKEY_END) // && isControl)
	{
		this->ScrollToAddress(dataAdapter->AdapterGetDataLength()-1);
		return true;
	}
	
	if (viewDisassembly != NULL)
	{
		CSlrKeyboardShortcut *keyboardShortcutDisassembly = guiMain->keyboardShortcuts->FindShortcut(KBZONE_DISASSEMBLY, bareKey, isShift, isAlt, isControl, isSuper);
		if (keyboardShortcutDisassembly == viewC64->mainMenuBar->kbsToggleTrackPC)
		{
			return viewDisassembly->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
	}
	
	//
	
	if (keyboardShortcutMemory == viewC64->mainMenuBar->kbsGoToAddress)
	{
		int adr = dataShowStart + dataShowSize/2 + dataAdapter->GetDataOffset();
		dataAddressEditBox->SetValue(adr);
		
		isEditingValue = false;
		isEditingValueAddr = true;
		
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		return true;
	}

	CSlrKeyboardShortcut *keyboardShortcutGlobal = guiMain->keyboardShortcuts->FindShortcut(KBZONE_GLOBAL, bareKey, isShift, isAlt, isControl, isSuper);

	if (keyboardShortcutGlobal == viewC64->mainMenuBar->kbsCopyToClipboard)
	{
		if (isCharacterEditing)
			this->CopyTextToClipboard();
		else
			this->CopyHexValuesToClipboard();
		return true;
	}

	if (keyboardShortcutGlobal == viewC64->mainMenuBar->kbsCopyAlternativeToClipboard)
	{
		this->CopyHexAddressToClipboard();
		return true;
	}

	if (keyboardShortcutGlobal == viewC64->mainMenuBar->kbsPasteFromClipboard)
	{
		if (isCharacterEditing)
			this->PasteTextFromClipboard();
		else
			this->PasteHexValuesFromClipboard();
		return true;
	}
	
	//
	
	// mimic shift+cursor up/down as page up/down
	if (keyCode == MTKEY_PAGE_DOWN)
	{
		keyCode = MTKEY_ARROW_DOWN;
		isShift = true;
	}
	else if (keyCode == MTKEY_PAGE_UP)
	{
		keyCode = MTKEY_ARROW_UP;
		isShift = true;
	}

	
//	// simple data show/scroll
//	if (isVisibleEditCursor == false)
//	{
//		if (keyCode == MTKEY_ARROW_DOWN)
//		{
//			if (isShift == false)
//			{
//				ScrollDataDown();
//			}
//			else
//			{
//				ScrollDataPageDown();
//			}
//			return true;
//		}
//		else if (keyCode == MTKEY_ARROW_UP)
//		{
//			if (isShift == false)
//			{
//				ScrollDataUp();
//			}
//			else
//			{
//				ScrollDataPageUp();
//			}
//			return true;
//		}
//		else if (keyCode == MTKEY_ARROW_LEFT || keyCode == MTKEY_ARROW_RIGHT)
//		{
//			return true;
//		}
//	}
//	else
	{
		// Clear selection on keyboard navigation
		// (cmd/ctrl+left/right is the global emulation rewind shortcut, leave it alone)
		if (keyCode == MTKEY_ARROW_DOWN || keyCode == MTKEY_ARROW_UP
			|| ((keyCode == MTKEY_ARROW_LEFT || keyCode == MTKEY_ARROW_RIGHT) && !isControl))
		{
			ClearSelection();
		}

		// show editing cursor
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (isShift == false)
			{
				if (editCursorPositionY < numberOfLines-1)
				{
					editCursorPositionY++;
				}
				else
				{
					ScrollDataDown();
				}
			}
			else
			{
				ScrollDataPageDown();
			}
			return true;
		}
		else if (keyCode == MTKEY_ARROW_UP)
		{
			if (isShift == false)
			{
				if (editCursorPositionY > 0)
				{
					editCursorPositionY--;
				}
				else
				{
					ScrollDataUp();
				}
			}
			else
			{
				ScrollDataPageUp();
			}
			return true;
		}
		else if (keyCode == MTKEY_ARROW_LEFT && !isControl)
		{
			// cmd/ctrl+left is the global "rewind emulation" shortcut; let it fall through
			if (editCursorPositionX > 0)
			{
				editCursorPositionX--;
			}
			else
			{
				if (editCursorPositionY > 0)
				{
					editCursorPositionX = numberOfBytesPerLine-1;
					editCursorPositionY--;
				}
				else
				{
					if (dataShowStart > 0)
					{
						ScrollDataUp();
						editCursorPositionX = numberOfBytesPerLine-1;
					}
				}
			}
			return true;
		}
		else if (keyCode == MTKEY_ARROW_RIGHT && !isControl)
		{
			// cmd/ctrl+right is the global "forward emulation" shortcut; let it fall through
			if (editCursorPositionX < numberOfBytesPerLine-1)
			{
				editCursorPositionX++;
			}
			else
			{
				if (editCursorPositionY < numberOfLines-1)
				{
					editCursorPositionX = 0;
					editCursorPositionY++;
				}
				else
				{
					if (dataShowStart + dataShowSize < dataAdapter->AdapterGetDataLength())
					{
						ScrollDataDown();
						editCursorPositionX = 0;
					}
				}
			}
			return true;
		}
		else if (keyCode == MTKEY_ENTER && !isCharacterEditing)
		{
			ClearSelection();
			int addr = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
			addr -= dataAdapter->GetDataOffset();
			u8 v;
			dataAdapter->AdapterReadByte(addr, &v);
			editBoxHex->SetValue(v, 2);
			isEditingValue = true;
			return true;
		}
		else if (!isCharacterEditing && ((keyCode >= '0' && keyCode <= '9') || (keyCode >= 'a' && keyCode <= 'f')))
		{
			// mimic start editing
			this->KeyDown(MTKEY_ENTER, false, false, false, false);
			editBoxHex->KeyDown(keyCode);
			return true;
		}
	}
	
	
	return false;
}

bool CViewDataDump::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDataDump::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (isEditingValue || isEditingValueAddr)
		return true;
	
	if (keyCode == MTKEY_ARROW_DOWN || keyCode == MTKEY_ARROW_UP || keyCode == MTKEY_ARROW_LEFT || keyCode == MTKEY_ARROW_RIGHT || keyCode == MTKEY_ENTER)
		return true;
	
	return false;
}

void CViewDataDump::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	if (isEditingValue)
	{
		int addr = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
		addr -= dataAdapter->GetDataOffset();
		u8 v = editHex->value;
		dataAdapter->AdapterWriteByte(addr, v);
		
		isEditingValue = false;
		
		// move to next value?
	}
}

void CViewDataDump::DataAddressEditBoxEnteredValue(CDataAddressEditBox *editBox, u32 lastKeyCode, bool isCancelled)
{
	if (isEditingValueAddr)
	{
		int addrOffset = dataAdapter->GetDataOffset();
		int addr = dataAddressEditBox->GetValue();

		// ux, when user entered value below data offset let us add it
		if (addr < addrOffset)
		{
			addr += addrOffset;
		}

		addr -= addrOffset;

		isEditingValueAddr = false;
		ScrollToAddress(addr);
	}
}

void CViewDataDump::PasteHexValuesFromClipboard()
{
	LOGG("CViewDataDump::PasteHexValuesFromClipboard");

	int addrCursor;
	int fromAddr, toAddr;
	if (GetSelectionRange(&fromAddr, &toAddr))
	{
		addrCursor = fromAddr - dataAdapter->GetDataOffset();
		ClearSelection();
	}
	else
	{
		addrCursor = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
		addrCursor -= dataAdapter->GetDataOffset();
	}

	int addr = addrCursor;
	bool onlyHexFound = true;
	bool addrFound = false;
	
	CSlrString *pasteStr = SYS_GetClipboardAsSlrString();
	pasteStr->DebugPrint("pasteStr=");
	
	std::list<u16> splitChars;
	splitChars.push_back(' ');
	splitChars.push_back(':');
	splitChars.push_back(',');
	splitChars.push_back('-');
	splitChars.push_back('\n');
	splitChars.push_back('\r');
	splitChars.push_back('\t');

	std::vector<CSlrString *> *strs = pasteStr->Split(splitChars);

	for (std::vector<CSlrString *>::iterator it = strs->begin(); it != strs->end(); it++)
	{
		CSlrString *hexVal = *it;
		
		if (hexVal->IsHexValue() == false)
		{
			onlyHexFound = false;
			continue;
		}
		
		int len = hexVal->GetLength();
		int val = hexVal->ToIntFromHex();
		
		if (hexVal->GetChar(0) == '$')
			len--;
		
		if (len == 4 || len == 3)
		{
			addr = val;
			addrFound = true;
		}
		else if (len == 2 || len == 1)
		{
			dataAdapter->AdapterWriteByte(addr, val);
			addr++;
		}
	}
	
	while (!strs->empty())
	{
		CSlrString *str = strs->back();
		
		strs->pop_back();
		delete str;
	}

	if (addr != addrCursor)
	{
//		if (addrFound)
//		{
//			ScrollToAddress(addr-1);
//		}
//		else
		{
			ScrollToAddress(addr);
		}
	}
	
	delete pasteStr;
}

void CViewDataDump::CopyHexValuesToClipboard()
{
	LOGG("CViewDataDump::CopyHexValuesToClipboard");

	int fromAddr, toAddr;
	if (GetSelectionRange(&fromAddr, &toAddr) && fromAddr != toAddr)
	{
		// Multi-byte selection: copy as space-separated hex
		int addrOffset = dataAdapter->GetDataOffset();
		int count = toAddr - fromAddr + 1;
		// Each byte: "XX " = 3 chars, plus null
		char *buf = new char[count * 3 + 1];
		int pos = 0;
		for (int a = fromAddr; a <= toAddr; a++)
		{
			u8 val;
			bool isAvailable;
			dataAdapter->AdapterReadByte(a - addrOffset, &val, &isAvailable);
			if (isAvailable)
			{
				if (pos > 0)
					buf[pos++] = ' ';
				sprintf(buf + pos, "%02X", val);
				pos += 2;
			}
		}
		buf[pos] = '\0';

		CSlrString *str = new CSlrString(buf);
		SYS_SetClipboardAsSlrString(str);
		delete str;

		char msgBuf[64];
		sprintf(msgBuf, "Copied %d bytes", count);
		viewC64->ShowMessageInfo(msgBuf);

		delete[] buf;
	}
	else
	{
		// Single byte at cursor
		int addrCursor = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
		addrCursor -= dataAdapter->GetDataOffset();

		u8 val;
		bool isAvailable;
		dataAdapter->AdapterReadByte(addrCursor, &val, &isAvailable);
		if (isAvailable)
		{
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "$%02X", val);
			CSlrString *str = new CSlrString(buf);
			SYS_SetClipboardAsSlrString(str);
			delete str;

			sprintf(buf, "Copied $%02X", val);
			viewC64->ShowMessageInfo(buf);

			SYS_ReleaseCharBuf(buf);
		}
		else
		{
			viewC64->ShowMessageError("No data available for copying.");
		}
	}
}

void CViewDataDump::CopyHexAddressToClipboard()
{
	LOGG("CViewDataDump::CopyHexAddressToClipboard");
	
	int addrCursor = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);

	char *buf = SYS_GetCharBuf();
	sprintf(buf, "$%04X", addrCursor);
	CSlrString *str = new CSlrString(buf);
	SYS_SetClipboardAsSlrString(str);
	delete str;
	
	sprintf(buf, "Copied $%04X", addrCursor);
	viewC64->ShowMessageInfo(buf);

	SYS_ReleaseCharBuf(buf);
}

//
bool CViewDataDump::DoRightClick(float x, float y)
{
	LOGD("CViewDataDump::DoRightClick: x=%f y=%f", x, y);
	int dataPositionX;
	int dataPositionY;
	int dataPositionAddr;

	guiMain->LockMutex();
	bool found = FindDataPosition(x, y, &dataPositionX, &dataPositionY, &dataPositionAddr);
	if (found == false)
	{
		guiMain->UnlockMutex();
		return CGuiView::DoRightClick(x, y);
	}

	previousClickTime = SYS_GetCurrentTimeInMillis();
	previousClickAddr = dataPositionAddr;
	
	editCursorPositionX = dataPositionX;
	editCursorPositionY = dataPositionY;

	guiMain->UnlockMutex();
	return CGuiView::DoRightClick(x, y);
}

bool CViewDataDump::HasContextMenuItems()
{
	return true;
}

void CViewDataDump::RenderContextMenuItems()
{
	CGuiView::RenderContextMenuItems();
	
	CDebugMemoryCell *cell = debugMemory->GetMemoryCell(currentDataIndex);
	
	u8 val;
	dataAdapter->AdapterReadByte(currentDataIndex, &val);
	
	localLabelText[0] = 0;
	CDebugSymbolsSegment *currentSegment = NULL;
	CDebugSymbolsCodeLabel *label = NULL;
	if (symbols && symbols->currentSegment)
	{
		currentSegment = symbols->currentSegment;
		label = currentSegment->FindLabel(currentDataIndex);
		if (label)
		{
			strcpy(localLabelText, label->GetLabelText());
		}
	}

	char *buf = SYS_GetCharBuf();
	char *buf2 = SYS_GetCharBuf();

	dataAdapter->GetAddressStringForCell(currentDataIndex, buf, MAX_STRING_LENGTH);
	ImGui::Text(buf);
	
	ImGui::SameLine();
		
	if (currentSegment)
	{
		// Per-address ID so ImGui's InputText internal edit state does not leak
		// between cells when the context menu popup is reopened at a new address.
		ImGui::PushID((int)currentDataIndex);
		if (ImGui::InputText("##dataDumpLabelText", localLabelText, MAX_STRING_LENGTH))
		{
			if (label != NULL)
			{
				// empty text?
				if (localLabelText[0] == 0)
				{
					currentSegment->RemoveCodeLabel(label, true);
					delete label;
				}
				else
				{
					label->SetText(localLabelText);
				}
			}
			else
			{
				// TODO: refactor this: fix ownership
				char *labelText = STRALLOC(localLabelText);
				currentSegment->AddCodeLabel(currentDataIndex, labelText);
			}
		}
		ImGui::PopID();
	}
	
	FUN_IntToBinaryStr(val, buf2);
	
	sprintf(buf, "%02X %3d %s", val, val, buf2);
	ImGui::Text(buf);
	
/*	if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Written by PC address: %04x", cell->writePC);
		ImGui::Text("Written on cycle: %d", cell->writeCycle);

		//volatile
		bool isExecuteCode;
		//volatile
		bool isExecuteArgument;
		
		bool isRead;
		bool isWrite;

		// last write PC & raster (where was PC & raster when cell was written)
		int writePC;
		int writeRasterLine, writeRasterCycle;
		u64 writeCycle;

		// last read PC & raster (where was PC & raster when cell was read)
		int readPC;
		int readRasterLine, readRasterCycle;
		u64 readCycle;
		
		// last execute cycle
		u64 executeCycle;

	}
		*/
	
	
	ImGui::Separator();
	
//	if (ImGui::CollapsingHeader("Breakpoints", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// breakpoints
		if (currentSegment && currentSegment->supportBreakpoints && currentSegment->breakpointsData)
		{
			bool supportsWriteBreakpoint, supportsReadBreakpoint;
			currentSegment->symbols->debugInterface->SupportsBreakpoints(&supportsWriteBreakpoint, &supportsReadBreakpoint);
			
			if (supportsWriteBreakpoint || supportsReadBreakpoint)
			{
				currentSegment->symbols->debugInterface->LockMutex();
				
				ImGui::Text("Breakpoint:");
				CDebugBreakpointData *memoryBreakpoint = (CDebugBreakpointData*)currentSegment->breakpointsData->GetBreakpoint(currentDataIndex);
				
				if (!memoryBreakpoint)
				{
					if (supportsWriteBreakpoint)
					{
						bool isWrite = false;
						if (ImGui::Checkbox("Write##memoryBreakpoint", &isWrite))
						{
							currentSegment->AddBreakpointMemory(currentDataIndex, MEMORY_BREAKPOINT_ACCESS_WRITE, DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL, 0xFF);
						}
					}
					
					if (supportsReadBreakpoint)
					{
						bool isRead = false;
						ImGui::SameLine();
						if (ImGui::Checkbox("Read##memoryBreakpoint", &isRead))
						{
							currentSegment->AddBreakpointMemory(currentDataIndex, MEMORY_BREAKPOINT_ACCESS_READ, DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL, 0xFF);
						}
					}
				}
				else
				{
					if (supportsWriteBreakpoint)
					{
						bool isWrite = IS_SET(memoryBreakpoint->dataAccess, MEMORY_BREAKPOINT_ACCESS_WRITE);
						if (ImGui::Checkbox("Write##memoryBreakpoint", &isWrite))
						{
							if (isWrite)
							{
								SET_BIT(memoryBreakpoint->dataAccess, MEMORY_BREAKPOINT_ACCESS_WRITE);
							}
							else
							{
								REMOVE_BIT(memoryBreakpoint->dataAccess, MEMORY_BREAKPOINT_ACCESS_WRITE);
							}
						}
					}

					if (supportsReadBreakpoint)
					{
						// TODO: read breakpoint in Vice is fired on STA (because it reads)
						bool isRead  = IS_SET(memoryBreakpoint->dataAccess, MEMORY_BREAKPOINT_ACCESS_READ);
						ImGui::SameLine();
						if (ImGui::Checkbox("Read##memoryBreakpoint", &isRead))
						{
							if (isRead)
							{
								SET_BIT(memoryBreakpoint->dataAccess, MEMORY_BREAKPOINT_ACCESS_READ);
							}
							else
							{
								REMOVE_BIT(memoryBreakpoint->dataAccess, MEMORY_BREAKPOINT_ACCESS_READ);
							}
						}
					}
					
					ImGui::SameLine();
					
	//				MEMORY_BREAKPOINT_EQUAL = 0,
	//				MEMORY_BREAKPOINT_NOT_EQUAL,
	//				MEMORY_BREAKPOINT_LESS,
	//				MEMORY_BREAKPOINT_LESS_OR_EQUAL,
	//				MEMORY_BREAKPOINT_GREATER,
	//				MEMORY_BREAKPOINT_GREATER_OR_EQUAL,

					ImGui::PushItemWidth(50);
					int comparison = memoryBreakpoint->comparison;
					if (ImGui::Combo("##memoryBreakpointComparison", &comparison, "==\0!=\0<\0<=\0>\0>=\0\0"))
					{
						memoryBreakpoint->comparison = (DataBreakpointComparison)comparison;
					}
					ImGui::PopItemWidth();
					
					ImGui::SameLine();

					ImGui::PushItemWidth(30);
					ImGuiInputTextFlags defaultHexInputFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;

					u8 val = memoryBreakpoint->value;
					ImGui::InputScalar("##memoryBreakpointValue", ImGuiDataType_::ImGuiDataType_U8, &val, NULL, NULL, "%02X", defaultHexInputFlags);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						memoryBreakpoint->value = val;
					}
					ImGui::PopItemWidth();
				}
				
				currentSegment->symbols->debugInterface->UnlockMutex();

				ImGui::Separator();
			}

		}
	}
	
//	if (ImGui::CollapsingHeader("Rewind", ImGuiTreeNodeFlags_DefaultOpen))
	
	if (currentSegment && currentSegment->supportBreakpoints && currentSegment->breakpointsData)
	{
		if (viewDisassembly)
		{
			// scroll/rewind
			if (ImGui::MenuItem("Scroll to last read", PLATFORM_STR_KEY_CTRL "+Shift+click", false, (cell->readPC != -1) ))
			{
				viewDisassembly->ScrollToAddress(cell->readPC);
			}

			if (ImGui::MenuItem("Scroll to last write", PLATFORM_STR_KEY_CTRL "+click", false, (cell->writePC != -1)))
			{
				viewDisassembly->ScrollToAddress(cell->writePC);
			}
		}

		if (debugInterface->snapshotsManager)
		{
			if (ImGui::MenuItem("Rewind to previous read", "Alt+" PLATFORM_STR_KEY_CTRL "+Shift+click", false, (cell->readCycle != -1) ))
			{
				LOGM("============######################### RESTORE TO READ CYCLE=%d", cell->readCycle);
				debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->readCycle);
			}

			if (ImGui::MenuItem("Rewind to previous write", "Alt+" PLATFORM_STR_KEY_CTRL "+click", false, (cell->writeCycle != -1)))
			{
				LOGM("============######################### RESTORE TO WRITE CYCLE=%d", cell->writeCycle);
				debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->writeCycle);
			}
			
			if (ImGui::MenuItem("Rewind to previous execute", "", false, (cell->executeCycle != -1)))
			{
				LOGM("============######################### RESTORE TO EXECUTE CYCLE=%d", cell->executeCycle);
				debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->executeCycle);
			}
		}

		ImGui::Separator();
	}

	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(buf2);
	
//	if (ImGui::CollapsingHeader("Config", ImGuiTreeNodeFlags_DefaultOpen))
}

void CViewDataDump::CancelEditingHexBox()
{
	if (isEditingValue)
	{
		editBoxHex->CancelEntering();
		isEditingValue = false;
	}
	
	if (isEditingValueAddr)
	{
		dataAddressEditBox->CancelEntering();
		isEditingValueAddr = false;
	}
}

u8 CViewDataDump::MapKeyToScreenCode(u32 keyCode, bool isShift)
{
	// Resolve shifted symbols (Shift+1 -> '!', etc.)
	u32 resolvedKey = SYS_GetShiftedKey(keyCode, isShift, false, false, false);

	// Bounds check — only printable ASCII
	if (resolvedKey < ' ' || resolvedKey >= 128)
		return 0xFF;

	// Select mapping table based on active charset font
	const u8 *table;
	if (fontCharacters == fontDefaultCBM1 || fontCharacters == fontCBM1)
	{
		table = kAsciiToC64ScreenCodeUppercase;
	}
	else if (fontCharacters == fontDefaultCBM2 || fontCharacters == fontCBM2)
	{
		table = kAsciiToC64ScreenCodeLowercase;
	}
	else if (fontCharacters == fontAtari)
	{
		table = kAsciiToAtariScreenCode;
	}
	else
	{
		table = kAsciiPassthrough;
	}

	return table[resolvedKey];
}

char CViewDataDump::MapScreenCodeToAscii(u8 screenCode)
{
	// Select the same mapping table as MapKeyToScreenCode
	const u8 *table;
	if (fontCharacters == fontDefaultCBM1 || fontCharacters == fontCBM1)
	{
		table = kAsciiToC64ScreenCodeUppercase;
	}
	else if (fontCharacters == fontDefaultCBM2 || fontCharacters == fontCBM2)
	{
		table = kAsciiToC64ScreenCodeLowercase;
	}
	else if (fontCharacters == fontAtari)
	{
		table = kAsciiToAtariScreenCode;
	}
	else
	{
		table = kAsciiPassthrough;
	}

	// Reverse lookup: find the first ASCII char that maps to this screencode
	// Prefer printable lowercase for letters when both upper and lower map to same code
	for (int i = 0x60; i < 128; i++)
	{
		if (table[i] == screenCode)
			return (char)i;
	}
	for (int i = 0x20; i < 0x60; i++)
	{
		if (table[i] == screenCode)
			return (char)i;
	}

	// No mapping found — return '.' as placeholder
	return '.';
}

void CViewDataDump::CopyTextToClipboard()
{
	LOGG("CViewDataDump::CopyTextToClipboard");

	int fromAddr, toAddr;
	if (GetSelectionRange(&fromAddr, &toAddr) && fromAddr != toAddr)
	{
		int addrOffset = dataAdapter->GetDataOffset();
		int count = toAddr - fromAddr + 1;
		char *buf = new char[count + 1];
		int pos = 0;
		for (int a = fromAddr; a <= toAddr; a++)
		{
			u8 val;
			bool isAvailable;
			dataAdapter->AdapterReadByte(a - addrOffset, &val, &isAvailable);
			if (isAvailable)
			{
				buf[pos++] = MapScreenCodeToAscii(val);
			}
		}
		buf[pos] = '\0';

		CSlrString *str = new CSlrString(buf);
		SYS_SetClipboardAsSlrString(str);
		delete str;

		char msgBuf[64];
		sprintf(msgBuf, "Copied %d chars", pos);
		viewC64->ShowMessageInfo(msgBuf);

		delete[] buf;
	}
	else
	{
		// Single char at cursor
		int addrCursor = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
		addrCursor -= dataAdapter->GetDataOffset();

		u8 val;
		bool isAvailable;
		dataAdapter->AdapterReadByte(addrCursor, &val, &isAvailable);
		if (isAvailable)
		{
			char buf[2];
			buf[0] = MapScreenCodeToAscii(val);
			buf[1] = '\0';
			CSlrString *str = new CSlrString(buf);
			SYS_SetClipboardAsSlrString(str);
			delete str;

			char msgBuf[64];
			sprintf(msgBuf, "Copied '%c'", buf[0]);
			viewC64->ShowMessageInfo(msgBuf);
		}
	}
}

void CViewDataDump::PasteTextFromClipboard()
{
	LOGG("CViewDataDump::PasteTextFromClipboard");

	int addrCursor;
	int fromAddr, toAddr;
	if (GetSelectionRange(&fromAddr, &toAddr))
	{
		addrCursor = fromAddr - dataAdapter->GetDataOffset();
		ClearSelection();
	}
	else
	{
		addrCursor = GetAddrFromDataPosition(editCursorPositionX, editCursorPositionY);
		addrCursor -= dataAdapter->GetDataOffset();
	}

	CSlrString *pasteStr = SYS_GetClipboardAsSlrString();
	int addr = addrCursor;

	for (int i = 0; i < pasteStr->GetLength(); i++)
	{
		u16 chr = pasteStr->GetChar(i);
		if (chr < 128)
		{
			u8 screenCode = MapKeyToScreenCode(chr, false);
			if (screenCode != 0xFF)
			{
				dataAdapter->AdapterWriteByte(addr, screenCode);
				addr++;
			}
		}
	}

	if (addr != addrCursor)
	{
		ScrollToAddress(addr);
	}

	delete pasteStr;
}

void CViewDataDump::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewDataDump::Deserialize(CByteBuffer *byteBuffer)
{
}

