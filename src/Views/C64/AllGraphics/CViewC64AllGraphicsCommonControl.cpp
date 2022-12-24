#include "CViewC64AllGraphics.h"
#include "VID_Main.h"
#include "VID_ImageBinding.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64VicControl.h"
#include "CViewDataDump.h"
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CGuiMain.h"
#include "CViewDataDump.h"
#include "CGuiLockableList.h"
#include "CSlrString.h"
#include "CViewC64StateVIC.h"
#include "CViewC64.h"

// TODO: colour leds are just rectangles with hitbox and lot of copy pasted code, replace them to proper buttons!

CViewC64AllGraphics::CViewC64AllGraphics(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;

	this->isSelectedItemBitmap = false;
	this->isSelectedItemScreen = false;
	this->isSelectedItemCharset = false;
	this->isSelectedItemSprite = false;
	this->isSelectedItemColor = false;

	this->selectedScreenId = 0;
	this->selectedBitmapId = 0;
	this->selectedSpriteId = 0;
	this->selectedCharsetId = 0;
	
	this->charsetScale = 0.3287f;
	this->charsetSizeX = 256.0f * charsetScale;
	this->charsetSizeY = 64.0f * charsetScale;
	this->charsetScaleB = 0.7f;
	this->charsetSizeXB = 256.0f * charsetScaleB;
	this->charsetSizeYB = 64.0f * charsetScaleB;
	this->spriteScale = 0.435f;
	this->spriteSizeX = 24.0f * spriteScale;
	this->spriteSizeY = 21.0f * spriteScale;
	this->spriteScaleB = 2.3f;
	this->spriteSizeXB = 24.0f * spriteScaleB;
	this->spriteSizeYB = 21.0f * spriteScaleB;

	this->charsetsOffsetY = 50.0f;
	
	numBitmapDisplays = 0x10000/0x2000;	//8
	numScreenDisplays = 0x10000/0x0400;	// 0x40
	
	numVicDisplays = numScreenDisplays;	// 0x40
	vicDisplays = new CViewC64VicDisplay *[numVicDisplays];
	vicControl = new CViewC64VicControl *[numVicDisplays];
	for (int i = 0; i < numVicDisplays; i++)
	{
		vicDisplays[i] = new CViewC64VicDisplay("VicDisplay/CViewC64AllGraphics", 0, 0, posZ, 100, 100, debugInterface);
		vicDisplays[i]->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_NONE);
		
		vicDisplays[i]->showSpritesFrames = false;
		vicDisplays[i]->showSpritesGraphics = false;
		
		vicControl[i] = new CViewC64VicControl("VicDisplay/CViewC64AllGraphics", 0, 0, posZ, 100, 100, vicDisplays[i]);
		vicControl[i]->visible = false;

		vicDisplays[i]->SetVicControlView(vicControl[i]);
		
		vicControl[i]->lstScreenAddresses->SetListLocked(true);
	}
	
	//
	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;

	float px = posX + 350;
	float py = posY + 15;
	float buttonSizeX = 31.0f;//28.0f;
	float buttonSizeY = 10.0f;
	float gap = 5.0f;
	float mgap = 2.5f;

	btnModeBitmapColorsBlackWhite = new CGuiButtonSwitch(NULL, NULL, NULL,
										   px, py, posZ, buttonSizeX, buttonSizeY,
										   new CSlrString("B/W"),
										   FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										   font, fontScale,
										   1.0, 1.0, 1.0, 1.0,
										   1.0, 1.0, 1.0, 1.0,
										   0.3, 0.3, 0.3, 1.0,
										   this);
	btnModeBitmapColorsBlackWhite->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
	this->AddGuiElement(btnModeBitmapColorsBlackWhite);

	px += buttonSizeX + gap;

	btnModeHires = new CGuiButtonSwitch(NULL, NULL, NULL,
														px, py, posZ, buttonSizeX, buttonSizeY,
														new CSlrString("HIRES"),
														FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
														font, fontScale,
														1.0, 1.0, 1.0, 1.0,
														1.0, 1.0, 1.0, 1.0,
														0.3, 0.3, 0.3, 1.0,
														this);
	btnModeHires->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeHires);
	this->AddGuiElement(btnModeHires);

	px += buttonSizeX + gap;
	
	btnModeMulti = new CGuiButtonSwitch(NULL, NULL, NULL,
										px, py, posZ, buttonSizeX, buttonSizeY,
										new CSlrString("MULTI"),
										FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										font, fontScale,
										1.0, 1.0, 1.0, 1.0,
										1.0, 1.0, 1.0, 1.0,
										0.3, 0.3, 0.3, 1.0,
										this);
	btnModeMulti->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeMulti);
	this->AddGuiElement(btnModeMulti);
	
	//
	px = posX + 350;
	py += buttonSizeY + gap;
	float by = py;
	
	btnShowBitmaps = new CGuiButtonSwitch(NULL, NULL, NULL,
										px, py, posZ, buttonSizeX, buttonSizeY,
										new CSlrString("BITMAPS"),
										FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										font, fontScale,
										1.0, 1.0, 1.0, 1.0,
										1.0, 1.0, 1.0, 1.0,
										0.3, 0.3, 0.3, 1.0,
										this);
	btnShowBitmaps->SetOn(false);
	SetLockableButtonDefaultColors(btnShowBitmaps);
	this->AddGuiElement(btnShowBitmaps);

	py += buttonSizeY + mgap;
	
	btnShowScreens = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("SCREENS"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										  font, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
	btnShowScreens->SetOn(true);
	SetLockableButtonDefaultColors(btnShowScreens);
	this->AddGuiElement(btnShowScreens);

	py += buttonSizeY + mgap;
	
	btnShowCharsets = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("CHARSETS"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										  font, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
	btnShowCharsets->SetOn(false);
	SetLockableButtonDefaultColors(btnShowCharsets);
	this->AddGuiElement(btnShowCharsets);

	py += buttonSizeY + mgap;
	
	btnShowSprites = new CGuiButtonSwitch(NULL, NULL, NULL,
										px, py, posZ, buttonSizeX, buttonSizeY,
										new CSlrString("SPRITES"),
										FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										font, fontScale,
										1.0, 1.0, 1.0, 1.0,
										1.0, 1.0, 1.0, 1.0,
										0.3, 0.3, 0.3, 1.0,
										this);
	btnShowSprites->SetOn(false);
	SetLockableButtonDefaultColors(btnShowSprites);
	this->AddGuiElement(btnShowSprites);

	py += buttonSizeY + mgap;
	
	btnShowColor = new CGuiButtonSwitch(NULL, NULL, NULL,
										px, py, posZ, buttonSizeX, buttonSizeY,
										new CSlrString("COLOR"),
										FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										font, fontScale,
										1.0, 1.0, 1.0, 1.0,
										1.0, 1.0, 1.0, 1.0,
										0.3, 0.3, 0.3, 1.0,
										this);
	btnShowColor->SetOn(false);
	SetLockableButtonDefaultColors(btnShowColor);
	this->AddGuiElement(btnShowColor);

	// list of screen addresses
	px = posX + 350 + buttonSizeX + gap;
	py = by;
	
	char **txtScreenAddresses = new char *[0x40];
	
	u16 addr = 0x0000;
	for (int i = 0; i < 0x40; i++)
	{
		char *txtAddr = new char[5];
		sprintf(txtAddr, "%04x", addr);
		addr += 0x0400;
		
		txtScreenAddresses[i] = txtAddr;
	}
	
	float lstFontSize = 4.0f;
	
	this->lstScreenAddresses = new CGuiLockableList(px, py, posZ+0.01, lstFontSize*6.5f, 65.0f, lstFontSize,
													NULL, 0, false,
													viewC64->fontDisassembly,
													guiMain->theme->imgBackground, 1.0f,
													this);
	this->lstScreenAddresses->name = "AllGraphics::lstScreenAddresses";
	this->lstScreenAddresses->Init(txtScreenAddresses, 0x40, true);
	this->lstScreenAddresses->SetGaps(0.0f, -0.25f);
	this->AddGuiElement(this->lstScreenAddresses);

	// list of charset addresses
	px = posX + 350 + buttonSizeX + gap;
	py = by; // + 5.0f;
	
	char **txtCharsetAddresses = new char *[0x40];
	
	addr = 0x0000;
	for (int i = 0; i < 0x20; i++)
	{
		char *txtCharsetAddr = new char[5];
		sprintf(txtCharsetAddr, "%04x", addr);
		addr += 0x0800;
		
		txtCharsetAddresses[i] = txtCharsetAddr;
	}

	//																					  61.0f
	this->lstCharsetAddresses = new CGuiLockableList(px, py, posZ+0.01, lstFontSize*6.5f, 65.0f, lstFontSize,
													NULL, 0, false,
													 viewC64->fontDisassembly,
													guiMain->theme->imgBackground, 1.0f,
													this);
	this->lstCharsetAddresses->name = "AllGraphics::lstCharsetAddresses";
	this->lstCharsetAddresses->Init(txtCharsetAddresses, 0x20, true);
	this->lstCharsetAddresses->SetGaps(0.0f, -0.25f);
	this->AddGuiElement(this->lstCharsetAddresses);

	//
	px = posX + 386 + buttonSizeX + gap;
	py = by + 55;
	btnShowRAMorIO = new CGuiButtonSwitch(NULL, NULL, NULL,
												  px, py, posZ, buttonSizeX, buttonSizeY,
												  new CSlrString("ROM"),
												  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
												  font, fontScale,
												  1.0, 1.0, 1.0, 1.0,
												  1.0, 1.0, 1.0, 1.0,
												  0.3, 0.3, 0.3, 1.0,
												  this);
	btnShowRAMorIO->SetOn(true);
	SetSwitchButtonDefaultColors(btnShowRAMorIO);
	this->AddGuiElement(btnShowRAMorIO);

	py -= buttonSizeY + mgap;
	btnShowGrid = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("GRID"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										  font, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
	btnShowGrid->SetOn(true);
	SetSwitchButtonDefaultColors(btnShowGrid);
	this->AddGuiElement(btnShowGrid);
	
	// sprites sheet, init images for sprites
	for (int i = 0; i < (0x10000/0x40); i++)
	{
		// alloc image that will store sprite pixels
		CImageData *imageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		spritesImageData.push_back(imageData);
		
		/// init CSlrImage with empty image (will be deleted by loader)
		// TODO: fixme on GUI branch and use LoadForRebinding...
		CImageData *emptyImageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		emptyImageData->AllocImage(false, true);
		
		CSlrImage *imageSprite = new CSlrImage(true, false);
		imageSprite->LoadImage(emptyImageData, RESOURCE_PRIORITY_STATIC, false);
		imageSprite->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		imageSprite->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(imageSprite, NULL);
		
		spritesImages.push_back(imageSprite);
		
		delete emptyImageData;
	}

	// charsets sheet, init images for charsets
	for (int i = 0; i < (0x10000/0x800); i++)
	{
		// alloc image that will store charset pixels
		CImageData *imageData = new CImageData(256, 64, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		charsetsImageData.push_back(imageData);
		
		/// init CSlrImage with empty image (will be deleted by loader)
		// TODO: fixme on GUI branch and use LoadForRebinding...
		CImageData *emptyImageData = new CImageData(256, 64, IMG_TYPE_RGBA);
		emptyImageData->AllocImage(false, true);
		
		CSlrImage *charsetImage = new CSlrImage(true, false);
		charsetImage->LoadImage(emptyImageData, RESOURCE_PRIORITY_STATIC, false);
		charsetImage->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		charsetImage->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(charsetImage, NULL);
		
		charsetsImages.push_back(charsetImage);
		
		delete emptyImageData;
	}
	
	// alloc image that will store charset pixels
	colorImageData = new CImageData(64, 32, IMG_TYPE_RGBA);
	colorImageData->AllocImage(false, true);
	
	/// init CSlrImage with empty image (will be deleted by loader)
	// TODO: fixme on GUI branch and use LoadForRebinding...
	CImageData *emptyImageData = new CImageData(64, 32, IMG_TYPE_RGBA);
	emptyImageData->AllocImage(false, true);
	
	colorImage = new CSlrImage(true, false);
	colorImage->LoadImage(emptyImageData, RESOURCE_PRIORITY_STATIC, false);
	colorImage->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	colorImage->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(colorImage, NULL);
	
	delete emptyImageData;

	this->SetMode(VIEW_C64_ALL_GRAPHICS_MODE_SCREENS);
}

CViewC64AllGraphics::~CViewC64AllGraphics()
{
}

void CViewC64AllGraphics::SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn)
{
	btn->buttonSwitchOffColorR = 0.0f;
	btn->buttonSwitchOffColorG = 0.0f;
	btn->buttonSwitchOffColorB = 0.0f;
	btn->buttonSwitchOffColorA = 1.0f;
	
	btn->buttonSwitchOffColor2R = 0.3f;
	btn->buttonSwitchOffColor2G = 0.3f;
	btn->buttonSwitchOffColor2B = 0.3f;
	btn->buttonSwitchOffColor2A = 1.0f;
	
	btn->buttonSwitchOnColorR = 0.0f;
	btn->buttonSwitchOnColorG = 0.7f;
	btn->buttonSwitchOnColorB = 0.0f;
	btn->buttonSwitchOnColorA = 1.0f;
	
	btn->buttonSwitchOnColor2R = 0.3f;
	btn->buttonSwitchOnColor2G = 0.3f;
	btn->buttonSwitchOnColor2B = 0.3f;
	btn->buttonSwitchOnColor2A = 1.0f;
	
}

void CViewC64AllGraphics::SetLockableButtonDefaultColors(CGuiButtonSwitch *btn)
{
	btn->buttonSwitchOffColorR = 0.0f;
	btn->buttonSwitchOffColorG = 0.0f;
	btn->buttonSwitchOffColorB = 0.0f;
	btn->buttonSwitchOffColorA = 1.0f;
	
	btn->buttonSwitchOffColor2R = 0.3f;
	btn->buttonSwitchOffColor2G = 0.3f;
	btn->buttonSwitchOffColor2B = 0.3f;
	btn->buttonSwitchOffColor2A = 1.0f;
	
	btn->buttonSwitchOnColorR = 0.7f;
	btn->buttonSwitchOnColorG = 0.0f;
	btn->buttonSwitchOnColorB = 0.0f;
	btn->buttonSwitchOnColorA = 1.0f;
	
	btn->buttonSwitchOnColor2R = 0.3f;
	btn->buttonSwitchOnColor2G = 0.3f;
	btn->buttonSwitchOnColor2B = 0.3f;
	btn->buttonSwitchOnColor2A = 1.0f;
	
}

void CViewC64AllGraphics::SetMode(int newMode)
{
	guiMain->LockMutex();
	this->displayMode = newMode;
	
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		numVisibleDisplays = numBitmapDisplays;
		numDisplaysColumns = 2;
		
		numDisplaysRows = (float)numVisibleDisplays / (float)numDisplaysColumns;

		float px = posX;
		float py = posY;
		float displaySizeX = (sizeX / (float)numDisplaysColumns) * 0.6f;
		float displaySizeY = sizeY / (float)numDisplaysRows;
		
		int i = 0;
		px = 0.0f;
		for(int x = 0; x < numDisplaysColumns; x++)
		{

			py = 0.0f;
			for (int y = 0; y < numDisplaysRows; y++)
			{
//				LOGD("......px=%f py=%f", px, py);
				vicDisplays[i]->SetDisplayPosition(px, py, 0.45f, true);
				py += displaySizeY;
				
				vicControl[i]->btnModeBitmap->SetOn(true);
				vicControl[i]->lstBitmapAddresses->SetListLocked(true);
				vicControl[i]->lstBitmapAddresses->SetElement(i, false, false);

				i++;
			}
			px += displaySizeX;
		}
		
		btnModeBitmapColorsBlackWhite->SetVisible(true);
		btnModeHires->SetVisible(true);
		btnModeMulti->SetVisible(true);
		lstScreenAddresses->SetVisible(true);
		lstCharsetAddresses->SetVisible(false);
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		numVisibleDisplays = numScreenDisplays;
		numDisplaysColumns = 8;
		
		numDisplaysRows = (float)numVisibleDisplays / (float)numDisplaysColumns;
		
		float px = posX;
		float py = posY;
		float displaySizeX = (sizeX / (float)numDisplaysColumns) * 0.585f;
		float displaySizeY = sizeY / (float)numDisplaysRows * 0.60f;
		
		int i = 0;
		px = 0.0f;
		for(int x = 0; x < numDisplaysColumns; x++)
		{
			
			py = 0.0f;
			for (int y = 0; y < numDisplaysRows; y++)
			{
//				LOGD("......px=%f py=%f", px, py);
				vicDisplays[i]->SetDisplayPosition(px, py, 0.129f, true);
				py += displaySizeY;
				
				vicControl[i]->btnModeText->SetOn(true);
				vicControl[i]->lstScreenAddresses->SetListLocked(true);
				vicControl[i]->lstScreenAddresses->SetElement(i, false, false);
				vicControl[i]->lstCharsetAddresses->SetListLocked(true);

				i++;
			}
			px += displaySizeX;
		}

		btnModeBitmapColorsBlackWhite->SetVisible(true);
		btnModeHires->SetVisible(true);
		btnModeMulti->SetVisible(true);
		lstScreenAddresses->SetVisible(false);
		lstCharsetAddresses->SetVisible(true);
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		btnModeBitmapColorsBlackWhite->SetVisible(true);
		btnModeHires->SetVisible(true);
		btnModeMulti->SetVisible(true);
		lstScreenAddresses->SetVisible(false);
		lstCharsetAddresses->SetVisible(false);
		
		switch(forcedRenderScreenMode)
		{
			case VIEW_C64_ALL_GRAPHICS_FORCED_GRAY:
				viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
				break;
			case VIEW_C64_ALL_GRAPHICS_FORCED_HIRES:
				viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
				break;
			case VIEW_C64_ALL_GRAPHICS_FORCED_MULTI:
				viewC64->viewC64MemoryDataDump->renderDataWithColors = true;
				break;
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		btnModeBitmapColorsBlackWhite->SetVisible(true);
		btnModeHires->SetVisible(true);
		btnModeMulti->SetVisible(true);
		lstScreenAddresses->SetVisible(false);
		lstCharsetAddresses->SetVisible(false);
		
		switch(forcedRenderScreenMode)
		{
			case VIEW_C64_ALL_GRAPHICS_FORCED_GRAY:
				viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
				break;
			case VIEW_C64_ALL_GRAPHICS_FORCED_HIRES:
				viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
				break;
			case VIEW_C64_ALL_GRAPHICS_FORCED_MULTI:
				viewC64->viewC64MemoryDataDump->renderDataWithColors = true;
				break;
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		btnModeBitmapColorsBlackWhite->SetVisible(true);
		btnModeHires->SetVisible(true);
		btnModeMulti->SetVisible(true);
		lstScreenAddresses->SetVisible(false);
		lstCharsetAddresses->SetVisible(false);
	}

	guiMain->UnlockMutex();
}

void CViewC64AllGraphics::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphics::RenderImGui()
{
//	PreRenderImGui();
//	Render();
//	PostRenderImGui();
}

void CViewC64AllGraphics::Render()
{
	float ledX = posX + fontSize * 82.1725f;
	float ledY = posY + fontSize * 5.5;
	
	char ledBuf[8] = { 'D', '0', '2', '0', 0x00 };
	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS
		|| displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		u16 screenAddress;
		vicii_cycle_state_t *viciiState = &viewC64->viciiStateToShow;
		
		screenAddress = viciiState->vbank_phi2 + ((viciiState->regs[0x18] & 0xf0) << 6);
		screenAddress = (screenAddress & viciiState->vaddr_mask_phi2) | viciiState->vaddr_offset_phi2;
		
		if (lstScreenAddresses->isLocked)
		{
			screenAddress = lstScreenAddresses->selectedElement * 0x0400;
		}
		else
		{
			bool updatePosition = true;
			
			if (lstScreenAddresses->IsInside(guiMain->mousePosX, guiMain->mousePosY))
				updatePosition = false;
			
			// update controls
			int addrItemNum = screenAddress / 0x0400;
			lstScreenAddresses->SetElement(addrItemNum, updatePosition, false);
		}
		
		u16 charsetAddress = (viciiState->regs[0x18] & 0x0E) << 10;
		charsetAddress = (charsetAddress + viciiState->vbank_phi1);
		charsetAddress &= viciiState->vaddr_mask_phi1;
		charsetAddress |= viciiState->vaddr_offset_phi1;
		
		if (lstCharsetAddresses->isLocked)
		{
			charsetAddress = lstCharsetAddresses->selectedElement * 0x0800;
		}
		else
		{
			bool updatePosition = true;
			
			if (lstCharsetAddresses->IsInside(guiMain->mousePosX, guiMain->mousePosY))
				updatePosition = false;
			
			// update controls
			int addrItemNum = charsetAddress / 0x0800;
			lstCharsetAddresses->SetElement(addrItemNum, updatePosition, false);
		}

		//
		if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_NONE)
		{
			u8 mc;
			
			mc = (viciiState->regs[0x16] & 0x10) >> 4;
			
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeHires->SetOn(!mc);
			btnModeMulti->SetOn(mc);
		}
		
		// TODO: move me to button press
		
		// vic displays
		for (int i = 0; i < numVisibleDisplays; i++)
		{
			//		LOGD("Render VIC Display %d", i);
			vicDisplays[i]->viewVicControl->forceDataFromRam = viewC64->isDataDirectlyFromRAM;
			
			if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
			{
				vicDisplays[i]->viewVicControl->btnModeText->SetOn(false);
				vicDisplays[i]->viewVicControl->btnModeBitmap->SetOn(true);
				int screenAddrItemNum = screenAddress / 0x0400;
				vicDisplays[i]->viewVicControl->lstScreenAddresses->SetElement(screenAddrItemNum, false, false);
				vicDisplays[i]->showGridLines = viewC64->viewC64VicControl->btnShowGrid->IsOn();
			}
			else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
			{
				vicDisplays[i]->viewVicControl->btnModeBitmap->SetOn(false);
				vicDisplays[i]->viewVicControl->btnModeText->SetOn(true);
				int charsetAddrItemNum = charsetAddress / 0x0800;
				vicDisplays[i]->viewVicControl->lstCharsetAddresses->SetElement(charsetAddrItemNum, false, false);
				vicDisplays[i]->showGridLines = false;
			}

			vicDisplays[i]->viewVicControl->btnModeStandard->SetOn(true);
			vicDisplays[i]->viewVicControl->btnModeExtended->SetOn(false);

//			LOGD("forcedRenderScreenMode=%d", forcedRenderScreenMode);
			switch(forcedRenderScreenMode)
			{
				case VIEW_C64_ALL_GRAPHICS_FORCED_NONE:
					vicDisplays[i]->viewVicControl->forceGrayscaleColors = false;
					vicDisplays[i]->viewVicControl->btnModeHires->SetOn(!btnModeMulti->IsOn());
					vicDisplays[i]->viewVicControl->btnModeMulti->SetOn(btnModeMulti->IsOn());
					break;
				case VIEW_C64_ALL_GRAPHICS_FORCED_GRAY:
					vicDisplays[i]->viewVicControl->forceGrayscaleColors = true;
					vicDisplays[i]->viewVicControl->btnModeHires->SetOn(true);
					vicDisplays[i]->viewVicControl->btnModeMulti->SetOn(false);
					break;
				case VIEW_C64_ALL_GRAPHICS_FORCED_HIRES:
					vicDisplays[i]->viewVicControl->forceGrayscaleColors = false;
					vicDisplays[i]->viewVicControl->btnModeHires->SetOn(true);
					vicDisplays[i]->viewVicControl->btnModeMulti->SetOn(false);
					break;
				case VIEW_C64_ALL_GRAPHICS_FORCED_MULTI:
					vicDisplays[i]->viewVicControl->forceGrayscaleColors = false;
					vicDisplays[i]->viewVicControl->btnModeHires->SetOn(false);
					vicDisplays[i]->viewVicControl->btnModeMulti->SetOn(true);
					break;
			}
			
			vicDisplays[i]->Render();
		}
		
		if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
		{
			// render selected bitmap outline only
			if (selectedBitmapId >=0 && selectedBitmapId < 0x40)
			{
				CViewC64VicDisplay *vicDisplay = vicDisplays[selectedBitmapId];
				BlitRectangle(vicDisplay->posX, vicDisplay->posY, posZ, vicDisplay->sizeX, vicDisplay->sizeY, 1.0, 0.0, 0.0f, 0.7f);
			}

			// render color leds for bitmap. todo: convert them to proper buttons
			if (btnModeHires->IsOn())
			{
				float px = ledX;
				float py = ledY;
				
				// D021
				int i = 0x01;
				u8 color = viewC64->colorsToShow[i];
				bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
			}
			else if (btnModeMulti->IsOn())
			{
				float px = ledX;
				float py = ledY;
				
				// D021
				int i = 0x01;
				u8 color = viewC64->colorsToShow[i];
				bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				py += fontSize + step;

				/* D800
				color = viewC64->colorToShowD800;
				isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				 */
			}
		}
		else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
		{
			// render selected outline and screen id
			if (selectedScreenId >=0 && selectedScreenId < 0x40)
			{
				CViewC64VicDisplay *vicDisplay = vicDisplays[selectedScreenId];
				BlitRectangle(vicDisplay->posX, vicDisplay->posY, posZ, vicDisplay->sizeX, vicDisplay->sizeY, 1.0, 0.0, 0.0f, 0.7f);
				
				float px = 62;
				float py = 220;
				
				float ppx = vicDisplay->posX;
				float ppy = vicDisplay->posY;
				
				vicDisplay->showGridLines = viewC64->viewC64VicControl->btnShowGrid->IsOn();
				vicDisplay->SetDisplayPosition(px, py, 0.650f, true);
				vicDisplay->Render();
				vicDisplay->SetDisplayPosition(ppx, ppy, 0.129f, true);
			}
			
			// render color leds for screens when in hires
			if (btnModeHires->IsOn())
			{
				float px = ledX;
				float py = ledY;
				
				// D021
				int i = 0x01;
				u8 color = viewC64->colorsToShow[i];
				bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				py += fontSize + step;

				/* D800
				color = viewC64->colorToShowD800;
				isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				 */
			}
			else if (btnModeMulti->IsOn())
			{
				float px = ledX;
				float py = ledY;
				
				// D021-D023
				for (int i = 0x01; i < 0x04; i++)
				{
					u8 color = viewC64->colorsToShow[i];
					bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
					RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
					py += fontSize + step;
				}

				/* D800
				u8 color = viewC64->colorToShowD800;
				bool isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				 */
			}
						
		}
		
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		// get VIC colors
		u8 cD021 = viewC64->colorsToShow[1];
		u8 cD022 = viewC64->colorsToShow[2];
		u8 cD023 = viewC64->colorsToShow[3];
		u8 cD800 = viewC64->colorToShowD800;
		
		// render charsets
		UpdateCharsets(viewC64->viewC64MemoryDataDump->renderDataWithColors, cD021, cD022, cD023, cD800);
		
		float startX = posX + 0.0f;
		float startY = posY + charsetsOffsetY;
		float px = startX;
		float py = startY;

		int countY = 0;
		int charsetId = 0;
		CSlrImage *imageSelectedCharset = NULL;
		for (std::vector<CSlrImage *>::iterator it = charsetsImages.begin(); it != charsetsImages.end(); it++)
		{
			CSlrImage *image = *it;
			
			Blit(image, px, py, posZ, charsetSizeX, charsetSizeY);
			
			if (charsetId == selectedCharsetId)
			{
				BlitRectangle(px, py, posZ, charsetSizeX, charsetSizeY, 1.0, 0.0, 0.0f, 0.7f);
				imageSelectedCharset = image;
			}
			
			py += charsetSizeY;
			
			countY++;
			
			if (countY == 8)
			{
				countY = 0;
				py = startY;
				px += charsetSizeX;
			}
			
			charsetId++;
		}
		py = startY + charsetSizeY * 8 + 23.0f;
		
		px = (4.0f * charsetSizeX - charsetSizeXB) / 2.0f;
		if (imageSelectedCharset)
		{
//			u16 charsetAddr = selectedCharsetId * 0x0800;
			Blit(imageSelectedCharset, px, py, posZ, charsetSizeXB, charsetSizeYB);
			BlitRectangle(px, py, posZ, charsetSizeXB, charsetSizeYB, 1.0, 0.0, 0.0f, 0.7f);
			
//			float fontSize = 4.0f;
//			px = (32.0f * charsetSizeX - charsetSizeXB) / 2.0f;
		}
		//
		if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_NONE)
		{
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeHires->SetOn(! (viewC64->viewC64MemoryDataDump->renderDataWithColors) );
			btnModeMulti->SetOn(  (viewC64->viewC64MemoryDataDump->renderDataWithColors) );
		}
		
		// render color leds for charsets when in multi
		if (btnModeMulti->IsOn())
		{
			px = ledX;
			py = ledY;
			
			// D021-D023
			for (int i = 0x01; i < 0x04; i++)
			{
				u8 color = viewC64->colorsToShow[i];
				bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				py += fontSize + step;
			}

			// D800
			u8 color = viewC64->colorToShowD800;
			bool isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
			RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		// get VIC colors
		u8 cD021 = viewC64->colorsToShow[1];
		u8 cD025 = viewC64->colorsToShow[5];
		u8 cD026 = viewC64->colorsToShow[6];
		u8 cD027 = viewC64->colorsToShow[7];
		
		// render sprites
		UpdateSprites(viewC64->viewC64MemoryDataDump->renderDataWithColors, cD021, cD025, cD026, cD027);
		
		float startX = posX + 0.0f;
		float px = startX;
		float py = posY;
		
		const float spriteTexStartX = 4.0/32.0;
		const float spriteTexStartY = 4.0/32.0;
		const float spriteTexEndX = (4.0+24.0)/32.0;
		const float spriteTexEndY = (4.0+21.0)/32.0;
		
		int countX = 0;
		int spriteId = 0;
		CSlrImage *imageSelectedSprite = NULL;
		for (std::vector<CSlrImage *>::iterator it = spritesImages.begin(); it != spritesImages.end(); it++)
		{
			CSlrImage *image = *it;
			
			Blit(image, px, py, posZ, spriteSizeX, spriteSizeY, spriteTexStartX, spriteTexStartY, spriteTexEndX, spriteTexEndY);
			
			if (spriteId == selectedSpriteId)
			{
				BlitRectangle(px, py, posZ, spriteSizeX, spriteSizeY, 1.0, 0.0, 0.0f, 0.7f);
				imageSelectedSprite = image;
			}
			
			px += spriteSizeX;
			
			countX++;
			
			if (countX == 32)
			{
				countX = 0;
				px = startX;
				py += spriteSizeY;
			}
			
			spriteId++;
		}
		py += spriteSizeY - 3.0f;
		
		px = (32.0f * spriteSizeX - spriteSizeXB) / 2.0f;
		if (imageSelectedSprite)
		{
//			u16 spriteAddr = selectedSpriteId * 0x0040;
			Blit(imageSelectedSprite, px, py, posZ, spriteSizeXB, spriteSizeYB, spriteTexStartX, spriteTexStartY, spriteTexEndX, spriteTexEndY);
			BlitRectangle(px, py, posZ, spriteSizeXB, spriteSizeYB, 1.0, 0.0, 0.0f, 0.7f);
			
			//			float fontSize = 4.0f;
			//			px = (32.0f * spriteSizeX - fontSize) / 2.0f;
		}
		
		//
		if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_NONE)
		{
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeHires->SetOn(! (viewC64->viewC64MemoryDataDump->renderDataWithColors) );
			btnModeMulti->SetOn(  (viewC64->viewC64MemoryDataDump->renderDataWithColors) );
		}
		
		// render color leds for sprites
		if (btnModeMulti->IsOn())
		{
			px = ledX;
			py = ledY;
			
			// D021
			int i = 0x01;
			u8 color = viewC64->colorsToShow[i];
			bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
			RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
			py += fontSize + step;

			// D025-D027
			for (int i = 0x05; i < 0x08; i++)
			{
				u8 color = viewC64->colorsToShow[i];
				bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
				py += fontSize + step; //gap + ledSizeY + gap;
			}
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
	{
		vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
		u8 *screen_ptr, *color_ram_ptr, *chargen_ptr, *bitmap_low_ptr, *bitmap_high_ptr;
		u8 colors[0x0F];
		
		viewC64->viewC64VicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

		u8 fgcolor;
		u8 fgColorR, fgColorG, fgColorB;
		
		for (int i = 0; i < 25; i++)
		{
			for (int j = 0; j < 40; j++)
			{
				fgcolor = color_ram_ptr[(i * 40) + j] & 0xf;
				debugInterface->GetCBMColor(fgcolor, &fgColorR, &fgColorG, &fgColorB);
				
				//LOGD("i=%d j=%d %d | %d %d %d", i, j, fgcolor, fgColorR, fgColorG, fgColorB);
				colorImageData->SetPixelResultRGBA(j, i, fgColorR, fgColorG, fgColorB, 255);
			}
		}
		
		colorImage->ReBindImageData(colorImageData);
		
		// nearest neighbour
		// TODO: the image is v-flipped, this has been fixed in GUI branch
		const float itex = 40.0f/64.0f;
//		const float itey = 1.0f - (25.0f/32.0f);
		const float itey = 25.0f/32.0f;

		float s = 1.0f;
		float px = 10.0f;
		float py = 80.0f;
		float sx = 320.0f * s;
		float sy = 200.0f * s;
		Blit(colorImage, px, py, posZ, sx, sy, 0, 1, itex, itey);
		BlitRectangle(px, py, posZ, sx, sy, 1.0, 0.0, 0.0f, 0.7f);

	}

	btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());

	CGuiView::Render();
}

void CViewC64AllGraphics::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphics::UpdateSprites(bool useColors, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027)
{
	std::vector<CSlrImage *>::iterator itImage = spritesImages.begin();
	std::vector<CImageData *>::iterator itImageData = spritesImageData.begin();
	
	int addr = 0x0000;
	CDataAdapter *dataAdapter = viewC64->debugInterfaceC64->dataAdapterC64DirectRam;
	
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
		
		if (useColors == false)
		{
			ConvertSpriteDataToImage(spriteData, imageData, 4);
		}
		else
		{
			ConvertColorSpriteDataToImage(spriteData, imageData, colorD021, colorD025, colorD026, colorD027,
										  viewC64->debugInterfaceC64, 4, 255);
		}
		
		addr++;
		
		// re-bind image
		image->ReBindImageData(imageData);
		
		itImage++;
		itImageData++;
	}
}

void CViewC64AllGraphics::UpdateCharsets(bool useColors, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800)
{
	std::vector<CSlrImage *>::iterator itImage = charsetsImages.begin();
	std::vector<CImageData *>::iterator itImageData = charsetsImageData.begin();
	
	int addr = 0x0000;
	CDataAdapter *dataAdapter = viewC64->debugInterfaceC64->dataAdapterC64DirectRam;
	
	//	int zi = 0;
	while(itImage != charsetsImages.end())
	{
		//		LOGD("charsets#=%d dataAddr=%4.4x", zi++, addr);
		
		CSlrImage *image = *itImage;
		CImageData *imageData = *itImageData;
		
		u8 charsetData[0x800];
		
		for (int i = 0; i < 0x800; i++)
		{
			u8 v;
			dataAdapter->AdapterReadByte(addr, &v);
			charsetData[i] = v;
			addr++;
		}
		
		if (useColors == false)
		{
			CopyHiresCharsetToImage(charsetData, imageData, 32, 0, 1, viewC64->debugInterfaceC64);
		}
		else
		{
			CopyMultiCharsetToImage(charsetData, imageData, 32, colorD021, colorD022, colorD023,
									colorD800, viewC64->debugInterfaceC64);
		}
		
		// re-bind image
		image->ReBindImageData(imageData);
		
		itImage++;
		itImageData++;
	}
}

void CViewC64AllGraphics::UpdateShowIOButton()
{
	btnShowRAMorIO->SetOn( ! viewC64->isDataDirectlyFromRAM);
}

bool CViewC64AllGraphics::GetIsSelectedItem()
{
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		return isSelectedItemBitmap;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		return isSelectedItemScreen;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		return isSelectedItemCharset;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		return isSelectedItemSprite;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
	{
		return isSelectedItemColor;
	}
	return false;
}

void CViewC64AllGraphics::SetIsSelectedItem(bool isSelected)
{
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		isSelectedItemBitmap = isSelected;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		isSelectedItemScreen = isSelected;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		isSelectedItemCharset = isSelected;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		isSelectedItemSprite = isSelected;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
	{
		isSelectedItemColor = isSelected;
	}
}

//@returns is consumed
bool CViewC64AllGraphics::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphics::DoTap:  x=%f y=%f", x, y);
	
	if (GetIsSelectedItem() == false)
	{
		int itemId = GetItemIdAt(x, y);
		if (itemId != -1)
		{
			SetIsSelectedItem(true);
			SetSelectedItemId(itemId);
//			ClearRasterCursorPos();
			return true;
		}
	}
	else
	{
		int itemId = GetItemIdAt(x, y);
		if (itemId != -1)
		{
			int selectedItemId = GetSelectedItemId();
			if (itemId == selectedItemId)
			{
				SetIsSelectedItem(false);
			}
			else
			{
				SetSelectedItemId(itemId);
			}
			
			// show the cursor again and scroll to address
			if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS
				|| displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
			{
				vicDisplays[itemId]->DoNotTouchedMove(x, y);
			}
		}
		return true;
	}
	
	guiMain->LockMutex();
	
	// TODO: note this is copy pasted code from C64ViewStateVIC, needs to be generalized
	//       idea is to sync values with VIC state view. Leds should be replaced with proper buttons and callbacks.
	//		 the below is just a temporary POC made in a few minutes
	
	float ledX = posX + fontSize * 82.1725f;
	float ledY = posY + fontSize * 5.5;
	
	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		// TODO: warning! copy pasted code
		// render color leds for bitmap. todo: convert them to proper buttons
		if (btnModeHires->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					viewC64->viewC64StateVIC->forceColors[i] = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}
		}
		else if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					viewC64->viewC64StateVIC->forceColors[i] = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;

			/* D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				else
				{
					viewC64->viewC64StateVIC->forceColorD800 = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}*/
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		// render color leds for screens when in hires
		if (btnModeHires->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					viewC64->viewC64StateVIC->forceColors[i] = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;

			/* D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				else
				{
					viewC64->viewC64StateVIC->forceColorD800 = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}*/
		}
		else if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			// D021-D023
			for (int i = 1; i < 4; i++)
			{
				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
				{
					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
					{
						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
					}
					else
					{
						viewC64->viewC64StateVIC->forceColors[i] = -1;
					}
					guiMain->UnlockMutex();
					return true;
				}

				py += fontSize + step;
			}

			/* D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				else
				{
					viewC64->viewC64StateVIC->forceColorD800 = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}*/
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		// render color leds for charsets when in multi
		if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			// D021-D023
			for (int i = 1; i < 4; i++)
			{
				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
				{
					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
					{
						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
					}
					else
					{
						viewC64->viewC64StateVIC->forceColors[i] = -1;
					}
					guiMain->UnlockMutex();
					return true;
				}

				py += fontSize + step;
			}

			// D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				else
				{
					viewC64->viewC64StateVIC->forceColorD800 = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		// render color leds for sprites
		if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					viewC64->viewC64StateVIC->forceColors[i] = -1;
				}
				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;

			// D025-D027
			for (int i = 5; i < 8; i++)
			{
				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
				{
					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
					{
						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
					}
					else
					{
						viewC64->viewC64StateVIC->forceColors[i] = -1;
					}
					guiMain->UnlockMutex();
					return true;
				}

				py += fontSize + step;
			}
		}
	}
//	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
//	{
//	}

	
	guiMain->UnlockMutex();
	
	return CGuiView::DoTap(x, y);
}

bool CViewC64AllGraphics::DoRightClick(float x, float y)
{
	guiMain->LockMutex();
	
	// TODO: note this is copy pasted code from C64ViewStateVIC, needs to be generalized
	//       idea is to sync values with VIC state view. Leds should be replaced with proper buttons and callbacks.
	//		 the below is just a temporary POC made in few minutes
	
	float ledX = posX + fontSize * 82.1725f;
	float ledY = posY + fontSize * 5.5;
	
	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		// TODO: warning! copy pasted code
		// render color leds for bitmap. todo: convert them to proper buttons
		if (btnModeHires->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				
				viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}
		}
		else if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;

			/* D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				viewC64->viewC64StateVIC->forceColorD800 = (viewC64->viewC64StateVIC->forceColorD800 + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}*/
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		// render color leds for screens when in hires
		if (btnModeHires->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;

			/* D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				viewC64->viewC64StateVIC->forceColorD800 = (viewC64->viewC64StateVIC->forceColorD800 + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}*/
		}
		else if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			// D021-D023
			for (int i = 1; i < 4; i++)
			{
				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
				{
					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
					{
						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
					}
					viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

					guiMain->UnlockMutex();
					return true;
				}

				py += fontSize + step;
			}

			/* D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				viewC64->viewC64StateVIC->forceColorD800 = (viewC64->viewC64StateVIC->forceColorD800 + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}*/
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		// render color leds for charsets when in multi
		if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			// D021-D023
			for (int i = 1; i < 4; i++)
			{
				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
				{
					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
					{
						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
					}
					viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

					guiMain->UnlockMutex();
					return true;
				}

				py += fontSize + step;
			}

			// D800
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
				{
					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
				}
				viewC64->viewC64StateVIC->forceColorD800 = (viewC64->viewC64StateVIC->forceColorD800 + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}
		}
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		// render color leds for sprites
		if (btnModeMulti->IsOn())
		{
			float px = ledX;
			float py = ledY;

			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				// D021
				int i = 0x01;
				
				if (viewC64->viewC64StateVIC->forceColors[i] == -1)
				{
					viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
				}
				viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

				guiMain->UnlockMutex();
				return true;
			}

			py += fontSize + step;

			// D025-D027
			for (int i = 5; i < 8; i++)
			{
				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
				{
					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
					{
						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
					}
					viewC64->viewC64StateVIC->forceColors[i] = (viewC64->viewC64StateVIC->forceColors[i] + 1) & 0x0F;

					guiMain->UnlockMutex();
					return true;
				}

				py += fontSize + step;
			}
		}
	}
//	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
//	{
//	}

	
	guiMain->UnlockMutex();
	
	return CGuiView::CGuiElement::DoRightClick(x, y);
}

bool CViewC64AllGraphics::DoNotTouchedMove(float x, float y)
{
//	LOGD("CViewC64AllGraphics::DoNotTouchedMove: x=%f y=%f", x, y);
	
	if (GetIsSelectedItem() == false)
	{
		ClearRasterCursorPos();

		int itemId = GetItemIdAt(x, y);
		SetSelectedItemId(itemId);

		if (itemId != -1)
		{
			if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
			{
				vicDisplays[itemId]->SetAutoScrollMode(AUTOSCROLL_DISASSEMBLY_BITMAP_ADDRESS);
				vicDisplays[itemId]->DoNotTouchedMove(x, y);
			}
			else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
			{
				vicDisplays[itemId]->SetAutoScrollMode(AUTOSCROLL_DISASSEMBLY_TEXT_ADDRESS);
				vicDisplays[itemId]->DoNotTouchedMove(x, y);
			}
		}
	}
	else
	{
		int itemId = GetItemIdAt(x, y);
		if (itemId != GetSelectedItemId())
		{
			ClearRasterCursorPos();
		}
	}
	
	return false;
}

int CViewC64AllGraphics::GetSelectedItemId()
{
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		return selectedBitmapId;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		return selectedScreenId;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		return selectedCharsetId;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		return selectedSpriteId;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
	{
		return -1;
	}
	return -1;
}

void CViewC64AllGraphics::SetSelectedItemId(int itemId)
{
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		selectedBitmapId = itemId;
		//u16 bitmapAddr = selectedBitmapId * 0x2000;
		//viewC64->viewC64MemoryDataDump->ScrollToAddress(bitmapAddr);
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		selectedScreenId = itemId;
		//u16 screenAddr = selectedScreenId * 0x0400;
		//viewC64->viewC64MemoryDataDump->ScrollToAddress(screenAddr);
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		selectedCharsetId = itemId;
		u16 charsetAddr = selectedCharsetId * 0x0800;
		viewC64->viewC64MemoryDataDump->ScrollToAddress(charsetAddr);
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		selectedSpriteId = itemId;
		u16 spriteAddr = selectedSpriteId * 0x0040;
		viewC64->viewC64MemoryDataDump->ScrollToAddress(spriteAddr);
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_COLOR)
	{
		// there's only one color screen
	}
}



// TODO: refactor me to GetItemAt(x, y), SetSelectedItem(id)
int CViewC64AllGraphics::GetItemIdAt(float x, float y)
{
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS)
	{
		float px = posX;
		float py = posY;
		float displaySizeX = (sizeX / (float)numDisplaysColumns) * 0.6f;
		float displaySizeY = sizeY / (float)numDisplaysRows;
		
		int i = 0;
		px = 0.0f;
		bool found = false;
		for(int dx = 0; dx < numDisplaysColumns; dx++)
		{
			py = 0.0f;
			for (int dy = 0; dy < numDisplaysRows; dy++)
			{
				if (x >= px && x <= px + displaySizeX
					&& y >= py && y <= py + displaySizeY)
				{
					found = true;
					break;
				}
				py += displaySizeY;
				i++;
			}
			
			if (found)
				break;
			
			px += displaySizeX;
		}
		
		if (i < 0x08)
		{
			return i;
		}
		return -1;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SCREENS)
	{
		float px = posX;
		float py = posY;
		float displaySizeX = (sizeX / (float)numDisplaysColumns) * 0.585f;
		float displaySizeY = sizeY / (float)numDisplaysRows * 0.60f;
		
		int i = 0;
		px = 0.0f;
		bool found = false;
		for(int dx = 0; dx < numDisplaysColumns; dx++)
		{
			py = 0.0f;
			for (int dy = 0; dy < numDisplaysRows; dy++)
			{
				if (x >= px && x <= px + displaySizeX
					&& y >= py && y <= py + displaySizeY)
				{
					found = true;
					break;
				}
				py += displaySizeY;
				i++;
			}
			
			if (found)
				break;
			
			px += displaySizeX;
		}
		
		if (i < 0x40)
		{
			return i;
		}
		return -1;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
	{
		// find charset over cursor
		int charsetId = 0;
		
		float startX = posX + 0.0f;
		float startY = posY + charsetsOffsetY;
		float px = startX;
		float py = startY;
		
		int countY = 0;
		for (std::vector<CSlrImage *>::iterator it = charsetsImages.begin(); it != charsetsImages.end(); it++)
		{
			if (x >= px && x <= px + charsetSizeX
				&& y >= py && y <= py + charsetSizeY)
			{
				break;
			}
			
			py += charsetSizeY;
			
			countY++;
			if (countY == 8)
			{
				countY = 0;
				py = startY;
				px += charsetSizeX;
			}
			
			charsetId++;
		}
		
		if (charsetId < (0x10000/0x800))
		{
			return charsetId;
		}
		return -1;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		// find sprite over cursor
		int spriteId = 0;
		
		float startX = posX + 0.0f;
		float px = startX;
		float py = posY;
		
		int countX = 0;
		for (std::vector<CSlrImage *>::iterator it = spritesImages.begin(); it != spritesImages.end(); it++)
		{
			if (x >= px && x <= px + spriteSizeX
				&& y >= py && y <= py + spriteSizeY)
			{
				break;
			}
			
			px += spriteSizeX;
			
			countX++;
			if (countX == 32)
			{
				countX = 0;
				px = startX;
				py += spriteSizeY;
			}
			
			spriteId++;
		}
		
		if (spriteId < (0x10000/0x40))
		{
			return spriteId;
		}
		return -1;
	}
	else if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		return -1;
	}
	return -1;
}

void CViewC64AllGraphics::ClearRasterCursorPos()
{
	for (int i = 0; i < numVicDisplays; i++)
	{
		vicDisplays[i]->ClearRasterCursorPos();
	}
}

bool CViewC64AllGraphics::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewC64AllGraphics::ButtonPressed(CGuiButton *button)
{
	LOGD("CViewC64AllGraphics::ButtonPressed");
	
	if (button == btnModeBitmapColorsBlackWhite)
	{
		if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES
			|| displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
		{
			viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
		}
		if (forcedRenderScreenMode != VIEW_C64_ALL_GRAPHICS_FORCED_GRAY)
		{
			btnModeMulti->SetOn(false);
			btnModeHires->SetOn(false);
			btnModeBitmapColorsBlackWhite->SetOn(true);
			forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_GRAY;
			SetLockableButtonDefaultColors(btnModeBitmapColorsBlackWhite);
		}
		else
		{
			ClearGraphicsForcedMode();
		}
		return true;
	}
	else if (button == btnModeHires)
	{
		if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES
			|| displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
		{
			// is it already set?
			if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_HIRES)
			{
				// switch to multi
				viewC64->viewC64MemoryDataDump->renderDataWithColors = true;

				btnModeBitmapColorsBlackWhite->SetOn(false);
				btnModeHires->SetOn(false);
				btnModeMulti->SetOn(true);
				forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_MULTI;
				SetLockableButtonDefaultColors(btnModeMulti);
				return true;
			}
			
			viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
		}
		if (forcedRenderScreenMode != VIEW_C64_ALL_GRAPHICS_FORCED_HIRES)
		{
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeMulti->SetOn(false);
			btnModeHires->SetOn(true);
			forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_HIRES;
			SetLockableButtonDefaultColors(btnModeHires);
		}
		else
		{
			ClearGraphicsForcedMode();
		}
		return true;
	}
	else if (button == btnModeMulti)
	{
		if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES
			|| displayMode == VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS)
		{
			if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_MULTI)
			{
				// switch to hires
				viewC64->viewC64MemoryDataDump->renderDataWithColors = false;
				
				btnModeBitmapColorsBlackWhite->SetOn(false);
				btnModeMulti->SetOn(false);
				btnModeHires->SetOn(true);
				forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_HIRES;
				SetLockableButtonDefaultColors(btnModeHires);
				return true;
			}
			
			viewC64->viewC64MemoryDataDump->renderDataWithColors = true;
		}
		if (forcedRenderScreenMode != VIEW_C64_ALL_GRAPHICS_FORCED_MULTI)
		{
			btnModeBitmapColorsBlackWhite->SetOn(false);
			btnModeHires->SetOn(false);
			btnModeMulti->SetOn(true);
			forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_MULTI;
			SetLockableButtonDefaultColors(btnModeMulti);
		}
		else
		{
			ClearGraphicsForcedMode();
		}
		return true;
	}
	else if (button == btnShowBitmaps)
	{
		btnShowBitmaps->SetOn(true);
		btnShowScreens->SetOn(false);
		btnShowCharsets->SetOn(false);
		btnShowSprites->SetOn(false);
		btnShowColor->SetOn(false);
		SetLockableButtonDefaultColors(btnShowBitmaps);
		this->SetMode(VIEW_C64_ALL_GRAPHICS_MODE_BITMAPS);
		return true;
	}
	else if (button == btnShowScreens)
	{
		btnShowBitmaps->SetOn(false);
		btnShowScreens->SetOn(true);
		btnShowCharsets->SetOn(false);
		btnShowSprites->SetOn(false);
		btnShowColor->SetOn(false);
		SetLockableButtonDefaultColors(btnShowScreens);
		this->SetMode(VIEW_C64_ALL_GRAPHICS_MODE_SCREENS);
		return true;
	}
	else if (button == btnShowCharsets)
	{
		btnShowBitmaps->SetOn(false);
		btnShowScreens->SetOn(false);
		btnShowCharsets->SetOn(true);
		btnShowSprites->SetOn(false);
		btnShowColor->SetOn(false);
		SetLockableButtonDefaultColors(btnShowCharsets);
		this->SetMode(VIEW_C64_ALL_GRAPHICS_MODE_CHARSETS);
		return true;
	}
	else if (button == btnShowSprites)
	{
		btnShowBitmaps->SetOn(false);
		btnShowScreens->SetOn(false);
		btnShowCharsets->SetOn(false);
		btnShowSprites->SetOn(true);
		btnShowColor->SetOn(false);
		SetLockableButtonDefaultColors(btnShowSprites);
		this->SetMode(VIEW_C64_ALL_GRAPHICS_MODE_SPRITES);
		return true;
	}
	else if (button == btnShowColor)
	{
		btnShowBitmaps->SetOn(false);
		btnShowScreens->SetOn(false);
		btnShowCharsets->SetOn(false);
		btnShowSprites->SetOn(false);
		btnShowColor->SetOn(true);
		SetLockableButtonDefaultColors(btnShowColor);
		this->SetMode(VIEW_C64_ALL_GRAPHICS_MODE_COLOR);
		return true;
	}
	else if (button == btnShowGrid)
	{
		viewC64->viewC64VicControl->btnShowGrid->DoSwitch();
		return true;
	}

	return false;
}

bool CViewC64AllGraphics::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	LOGD("CViewC64AllGraphics::ButtonSwitchChanged");
	
	if (button == btnShowRAMorIO)
	{
		viewC64->SwitchIsDataDirectlyFromRam(!btnShowRAMorIO->IsOn());
	}
	return false;
}

bool CViewC64AllGraphics::ListElementPreSelect(CGuiList *listBox, int elementNum)
{
	LOGD("CViewC64AllGraphics::ListElementPreSelect");
	guiMain->LockMutex();
	
	CGuiLockableList *list = (CGuiLockableList*)listBox;
	
	if (list->isLocked)
	{
		// click on the same element - unlock
		if (list->selectedElement == elementNum)
		{
			list->SetListLocked(false);
			guiMain->UnlockMutex();
			return true;
		}
	}
	
	list->SetListLocked(true);
	
	guiMain->UnlockMutex();
	
	return true;
}

void CViewC64AllGraphics::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		ClearGraphicsForcedMode();
	}
}

void CViewC64AllGraphics::ClearGraphicsForcedMode()
{
	this->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
	SetSwitchButtonDefaultColors(btnModeHires);
	SetSwitchButtonDefaultColors(btnModeMulti);
}

bool CViewC64AllGraphics::DoFinishTap(float x, float y)
{
	LOGG("CViewC64AllGraphics::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64AllGraphics::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphics::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64AllGraphics::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphics::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64AllGraphics::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64AllGraphics::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64AllGraphics::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64AllGraphics::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64AllGraphics::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewC64AllGraphics::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64AllGraphics::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64AllGraphics::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64AllGraphics::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64AllGraphics::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64AllGraphics::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphics::ActivateView()
{
	LOGG("CViewC64AllGraphics::ActivateView()");
	UpdateShowIOButton();
}

void CViewC64AllGraphics::DeactivateView()
{
	LOGG("CViewC64AllGraphics::DeactivateView()");
}

