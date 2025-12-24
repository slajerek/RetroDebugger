#include "CViewC64AllGraphicsBitmapsControl.h"
#include "ViewC64AllGraphicsDefs.h"
#include "VID_Main.h"
#include "VID_ImageBinding.h"
#include "CDebugInterfaceC64.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64VicControl.h"
#include "CViewDataDump.h"
#include "CViewDataMap.h"
#include "C64Tools.h"
#include "CGuiMain.h"
#include "CViewDataDump.h"
#include "CGuiLockableList.h"
#include "CSlrString.h"
#include "CViewC64StateVIC.h"
#include "CViewC64.h"

// TODO: colour leds are just rectangles with hitbox and lot of copy pasted code, replace them to proper buttons!

CViewC64AllGraphicsBitmapsControl::CViewC64AllGraphicsBitmapsControl(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;

	//
	font = viewC64->fontCBMShifted;
	fontScale = 2.5f;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;

	float startX = 5;
	float startY = 5;
	
	float px = startX;
	float py = startY;
	float buttonSizeX = 5.96 * fontSize;
	float buttonSizeY = 2.0f * fontSize;
	float gap = 0.625f * fontSize;
	float mgap = 0.3125 * fontSize;
	float buttonTextOffsetY = 0.67*fontSize;

	btnModeBitmapColorsBlackWhite = new CGuiButtonSwitch(NULL, NULL, NULL,
										   px, py, posZ, buttonSizeX, buttonSizeY,
										   new CSlrString("B/W"),
										   FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
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
														FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
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
										FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
										font, fontScale,
										1.0, 1.0, 1.0, 1.0,
										1.0, 1.0, 1.0, 1.0,
										0.3, 0.3, 0.3, 1.0,
										this);
	btnModeMulti->SetOn(false);
	SetSwitchButtonDefaultColors(btnModeMulti);
	this->AddGuiElement(btnModeMulti);
	
	//
	px += buttonSizeX*1.8f + gap;

	char **txtScreenAddresses = new char *[0x40];
	
	u16 addr = 0x0000;
	for (int i = 0; i < 0x40; i++)
	{
		char *txtAddr = new char[5];
		sprintf(txtAddr, "%04x", addr);
		addr += 0x0400;
		
		txtScreenAddresses[i] = txtAddr;
	}
	
	float lstFontSize = fontSize;//4.0f;
	
	this->lstScreenAddresses = new CGuiLockableList(px, py, posZ+0.01, lstFontSize*6.5f, 65.0f/8.0f * fontSize,
													lstFontSize,
													NULL, 0, false,
													viewC64->fontDisassembly,
													guiMain->theme->imgBackground, 1.0f,
													this);
	this->lstScreenAddresses->name = "AllGraphics::lstScreenAddresses";
	this->lstScreenAddresses->Init(txtScreenAddresses, 0x40, true);
	this->lstScreenAddresses->SetGaps(0.0f, -0.25f);
	this->AddGuiElement(this->lstScreenAddresses);
	
	//
	
	//
	px = startX;
	py += buttonSizeY + gap;
	
	btnShowRAMorIO = new CGuiButtonSwitch(NULL, NULL, NULL,
												  px, py, posZ, buttonSizeX, buttonSizeY,
												  new CSlrString("ROM"),
												  FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
												  font, fontScale,
												  1.0, 1.0, 1.0, 1.0,
												  1.0, 1.0, 1.0, 1.0,
												  0.3, 0.3, 0.3, 1.0,
												  this);
	btnShowRAMorIO->SetOn(true);
	SetSwitchButtonDefaultColors(btnShowRAMorIO);
	this->AddGuiElement(btnShowRAMorIO);

	px += buttonSizeX + gap;

	btnShowGrid = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("GRID"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, buttonTextOffsetY,
										  font, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
	btnShowGrid->SetOn(true);
	SetSwitchButtonDefaultColors(btnShowGrid);
	this->AddGuiElement(btnShowGrid);
	
	SetupMode();
}

void CViewC64AllGraphicsBitmapsControl::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	float fs = 1.0f;
	
	float ledX = posX + fs * 20.1725f;
	float ledY = posY + 0.75f * fs;
	
	float ledSizeX = fs*4.0f;
	float gap = fs * 0.1f;
	float step = fs * 0.75f;
	float ledSizeY = fs + gap + gap;
	float ledSizeY2 = fs + step;
	
	float startX = 5;
	float startY = 5;
	
	float px = startX;
	float py = startY;
	
	float buttonSizeX = 5.96 * fs;
	float buttonSizeY = 2.0f * fs;
	
	float lstFontSize = fs;//4.0f;
	
	px += buttonSizeX + gap;
	px += buttonSizeX + gap;
	px += buttonSizeX*2.0f + gap;
	px += lstFontSize*6.5f;
	
	// we need to scale this
	float scale = (sizeX / px) * 0.1f;
	
	fontScale = 2.5f * scale;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;
	
	buttonSizeX = 5.96 * fontSize;
	buttonSizeY = 2.0f * fontSize;
	gap = 0.1f * fontSize;
	startX = posX + 5.0f;
	startY = posY + 5.0f;

	ledX = posX + fontSize * 20.1725f;
	ledY = posY + 0.75f * fontSize;

	ledSizeX = fontSize*4.0f;
	gap = fontSize * 0.1f;

	//
	px = startX;
	py = startY;
	
	btnModeBitmapColorsBlackWhite->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	btnModeBitmapColorsBlackWhite->SetFont(font, fontScale);
	
	px += buttonSizeX + gap;
	
	btnModeHires->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	btnModeHires->SetFont(font, fontScale);
	
	px += buttonSizeX + gap;
	
	btnModeMulti->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	btnModeMulti->SetFont(font, fontScale);
	
	//
	px += buttonSizeX*2.0f + gap;
	
	lstFontSize = fontSize;//4.0f;
	
	lstScreenAddresses->SetPosition(px, py, posZ, lstFontSize*6.5f, 65.0f/8.0f * fontSize);
	lstScreenAddresses->fontSize = lstFontSize;
	
	//
	
	//
	px = startX;
	py += buttonSizeY + gap;
	
	btnShowRAMorIO->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	btnShowRAMorIO->SetFont(font, fontScale);
	
	px += buttonSizeX + gap;
	
	btnShowGrid->SetPosition(px, py, posZ, buttonSizeX, buttonSizeY);
	btnShowGrid->SetFont(font, fontScale);
}

void CViewC64AllGraphicsBitmapsControl::SetSwitchButtonDefaultColors(CGuiButtonSwitch *btn)
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

void CViewC64AllGraphicsBitmapsControl::SetLockableButtonDefaultColors(CGuiButtonSwitch *btn)
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

void CViewC64AllGraphicsBitmapsControl::SetupMode()
{
	guiMain->LockMutex();
	
	btnModeBitmapColorsBlackWhite->SetVisible(true);
	btnModeHires->SetVisible(true);
	btnModeMulti->SetVisible(true);
	lstScreenAddresses->SetVisible(true);

	guiMain->UnlockMutex();
}

void CViewC64AllGraphicsBitmapsControl::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphicsBitmapsControl::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64AllGraphicsBitmapsControl::Render()
{
	float ledX = posX + fontSize * 19.3f;
	float ledY = posY + 0.75f * fontSize;

	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

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

	//
	if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_NONE)
	{
		u8 mc;

		mc = (viciiState->regs[0x16] & 0x10) >> 4;

		btnModeBitmapColorsBlackWhite->SetOn(false);
		btnModeHires->SetOn(!mc);
		btnModeMulti->SetOn(mc);
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

	btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());
	UpdateShowIOButton();
	
	CGuiView::Render();
}

void CViewC64AllGraphicsBitmapsControl::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphicsBitmapsControl::UpdateShowIOButton()
{
	btnShowRAMorIO->SetOn( ! viewC64->isDataDirectlyFromRAM);
}

//@returns is consumed
bool CViewC64AllGraphicsBitmapsControl::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsBitmapsControl::DoTap:  x=%f y=%f", x, y);
	
	guiMain->LockMutex();

	// TODO: note this is copy pasted code from C64ViewStateVIC, needs to be generalized
	//       idea is to sync values with VIC state view. Leds should be replaced with proper buttons and callbacks.
	//		 the below is just a temporary POC made in a few minutes

	float ledX = posX + fontSize * 19.3f;
	float ledY = posY + 0.75f * fontSize;

	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

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

	guiMain->UnlockMutex();
	
	return CGuiView::DoTap(x, y);
}

bool CViewC64AllGraphicsBitmapsControl::DoRightClick(float x, float y)
{
	guiMain->LockMutex();

	// TODO: note this is copy pasted code from C64ViewStateVIC, needs to be generalized
	//       idea is to sync values with VIC state view. Leds should be replaced with proper buttons and callbacks.
	//		 the below is just a temporary POC made in few minutes

	float ledX = posX + fontSize * 20.1725f;
	float ledY = posY + 0.75f * fontSize;

	float ledSizeX = fontSize*4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap;
	float ledSizeY2 = fontSize + step;

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

	guiMain->UnlockMutex();
	
	return CGuiView::CGuiElement::DoRightClick(x, y);
}

bool CViewC64AllGraphicsBitmapsControl::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewC64AllGraphicsBitmapsControl::ButtonPressed(CGuiButton *button)
{
	LOGD("CViewC64AllGraphicsBitmapsControl::ButtonPressed");
	
	if (button == btnModeBitmapColorsBlackWhite)
	{
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
	else if (button == btnShowGrid)
	{
		viewC64->viewC64VicControl->btnShowGrid->DoSwitch();
		return true;
	}

	return false;
}

bool CViewC64AllGraphicsBitmapsControl::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	LOGD("CViewC64AllGraphicsBitmapsControl::ButtonSwitchChanged");
	
	if (button == btnShowRAMorIO)
	{
		viewC64->SwitchIsDataDirectlyFromRam();
		return true;
	}
	return false;
}

bool CViewC64AllGraphicsBitmapsControl::ListElementPreSelect(CGuiList *listBox, int elementNum)
{
	LOGD("CViewC64AllGraphicsBitmapsControl::ListElementPreSelect");
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

void CViewC64AllGraphicsBitmapsControl::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
//	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
//	{
//		ClearGraphicsForcedMode();
//	}
}

void CViewC64AllGraphicsBitmapsControl::ClearGraphicsForcedMode()
{
	this->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
	SetSwitchButtonDefaultColors(btnModeHires);
	SetSwitchButtonDefaultColors(btnModeMulti);
}

// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphicsBitmapsControl::ActivateView()
{
	LOGG("CViewC64AllGraphicsBitmapsControl::ActivateView()");
	UpdateShowIOButton();
}

void CViewC64AllGraphicsBitmapsControl::DeactivateView()
{
	LOGG("CViewC64AllGraphicsBitmapsControl::DeactivateView()");
}

CViewC64AllGraphicsBitmapsControl::~CViewC64AllGraphicsBitmapsControl()
{
}

