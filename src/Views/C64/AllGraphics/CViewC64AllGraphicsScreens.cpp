#include "CViewC64AllGraphics.h"
#include "CViewC64AllGraphicsScreens.h"
#include "CViewC64AllGraphicsScreensControl.h"
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

CViewC64AllGraphicsScreens::CViewC64AllGraphicsScreens(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface, CViewC64AllGraphicsScreensControl *viewControl)
: CGuiViewMovingPane(name, posX, posY, posZ, sizeX, sizeY,
					 // numDisplaysColumns = 8;
					 // numDisplaysRows = 0x10000/0x0400 / cnumDisplaysColumns;
					 8*320, 8*200)
{
	this->debugInterface = debugInterface;
	this->viewControl = viewControl;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	this->isSelectedItemScreen = false;

	this->selectedScreenId = 0;
	
	numScreenDisplays = 0x10000/0x0400;	// 0x40
	
	numVicDisplays = numScreenDisplays;	// 0x40
	vicDisplays = new CViewC64VicDisplay *[numVicDisplays];
	vicControl = new CViewC64VicControl *[numVicDisplays];
	for (int i = 0; i < numVicDisplays; i++)
	{
		vicDisplays[i] = new CViewC64VicDisplay("VicDisplay/CViewC64AllGraphicsScreens", 0, 0, posZ, 100, 100, debugInterface);
		vicDisplays[i]->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_NONE);
		
		vicDisplays[i]->showSpritesFrames = false;
		vicDisplays[i]->showSpritesGraphics = false;
		vicDisplays[i]->performIsTopWindowCheck = false;
		
		vicControl[i] = new CViewC64VicControl("VicDisplay/CViewC64AllGraphicsScreens", 0, 0, posZ, 100, 100, vicDisplays[i]);
		vicControl[i]->SetVisible(false);

		vicDisplays[i]->SetVicControlView(vicControl[i]);
		
		vicControl[i]->lstScreenAddresses->SetListLocked(true);
	}
	
	//
	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;
	
	minZoom = 0.125f;
	maxZoom = 60.0f;

	SetupVicDisplays();
}

CViewC64AllGraphicsScreens::~CViewC64AllGraphicsScreens()
{
}

void CViewC64AllGraphicsScreens::SetupVicDisplays()
{
	guiMain->LockMutex();
	
	{
		numVisibleDisplays = 0x10000/0x0400;
		numDisplaysColumns = 8;
		
		numDisplaysRows = (float)numVisibleDisplays / (float)numDisplaysColumns;
		
		float displaySizeX = 320.0f * currentZoom;
		float displaySizeY = 200.0f * currentZoom;

		int i = 0;
		float px = renderMapPosX;
		for(int x = 0; x < numDisplaysColumns; x++)
		{
			float py = renderMapPosY;
			for (int y = 0; y < numDisplaysRows; y++)
			{
//				LOGD("......px=%f py=%f", px, py);
				vicDisplays[i]->SetDisplayPosition(px, py, currentZoom, true);
				py += displaySizeY;
				
				vicControl[i]->btnModeText->SetOn(true);
				vicControl[i]->lstScreenAddresses->SetListLocked(true);
				vicControl[i]->lstScreenAddresses->SetElement(i, false, false);
				vicControl[i]->lstCharsetAddresses->SetListLocked(true);

				i++;
			}
			px += displaySizeX;
		}

//		btnModeBitmapColorsBlackWhite->SetVisible(true);
//		btnModeHires->SetVisible(true);
//		btnModeMulti->SetVisible(true);
//		lstScreenAddresses->SetVisible(false);
//		lstCharsetAddresses->SetVisible(true);
	}

	guiMain->UnlockMutex();
}

void CViewC64AllGraphicsScreens::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphicsScreens::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64AllGraphicsScreens::Render()
{
	SetupVicDisplays();
	
	u16 screenAddress;
	vicii_cycle_state_t *viciiState = &viewC64->viciiStateToShow;
	
	screenAddress = viciiState->vbank_phi2 + ((viciiState->regs[0x18] & 0xf0) << 6);
	screenAddress = (screenAddress & viciiState->vaddr_mask_phi2) | viciiState->vaddr_offset_phi2;
	
	u16 charsetAddress = (viciiState->regs[0x18] & 0x0E) << 10;
	charsetAddress = (charsetAddress + viciiState->vbank_phi1);
	charsetAddress &= viciiState->vaddr_mask_phi1;
	charsetAddress |= viciiState->vaddr_offset_phi1;
	
	if (viewControl->lstCharsetAddresses->isLocked)
	{
		charsetAddress = viewControl->lstCharsetAddresses->selectedElement * 0x0800;
	}
	else
	{
		bool updatePosition = true;

		if (viewControl->lstCharsetAddresses->IsInside(guiMain->mousePosX, guiMain->mousePosY))
			updatePosition = false;

		// update controls
		int addrItemNum = charsetAddress / 0x0800;
		viewControl->lstCharsetAddresses->SetElement(addrItemNum, updatePosition, false);
	}

//	LOGD("charsetAddress=%x", charsetAddress);
	
	// vic displays
	for (int i = 0; i < numVisibleDisplays; i++)
	{
		//		LOGD("Render VIC Display %d", i);
		vicDisplays[i]->viewVicControl->forceDataFromRam = viewC64->isDataDirectlyFromRAM;
		
		vicDisplays[i]->viewVicControl->btnModeBitmap->SetOn(false);
		vicDisplays[i]->viewVicControl->btnModeText->SetOn(true);
		int charsetAddrItemNum = charsetAddress / 0x0800;
		vicDisplays[i]->viewVicControl->lstCharsetAddresses->SetElement(charsetAddrItemNum, false, false);
		vicDisplays[i]->showGridLines = viewC64->viewC64VicControl->btnShowGrid->IsOn();

		vicDisplays[i]->viewVicControl->btnModeStandard->SetOn(true);
		vicDisplays[i]->viewVicControl->btnModeExtended->SetOn(false);

//		LOGD("forcedRenderScreenMode=%d", viewControl->forcedRenderScreenMode);
		switch(viewControl->forcedRenderScreenMode)
		{
			case VIEW_C64_ALL_GRAPHICS_FORCED_NONE:
				vicDisplays[i]->viewVicControl->forceGrayscaleColors = false;
				vicDisplays[i]->viewVicControl->btnModeHires->SetOn(!viewControl->btnModeMulti->IsOn());
				vicDisplays[i]->viewVicControl->btnModeMulti->SetOn(viewControl->btnModeMulti->IsOn());
				break;
			case VIEW_C64_ALL_GRAPHICS_FORCED_GRAY:
				LOGD("forced gray");
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
		
		// this below does not work as vic display may clear the state
//		if (IsInsideView(guiMain->mousePosX, guiMain->mousePosY) && vicDisplays[i]->IsInside(guiMain->mousePosX, guiMain->mousePosY))
//		{
//			viewC64->viewC64StateVIC->SetIsLockedState(true);
//		}
	}
	
 
	{
		// render selected outline and screen id
		if (selectedScreenId >=0 && selectedScreenId < 0x40)
		{
			CViewC64VicDisplay *vicDisplay = vicDisplays[selectedScreenId];
			BlitRectangle(vicDisplay->posX, vicDisplay->posY, posZ, vicDisplay->sizeX, vicDisplay->sizeY, 1.0, 0.0, 0.0f, 0.7f);
			
//				float px = 62;
//				float py = 220;
//
//				float ppx = vicDisplay->posX;
//				float ppy = vicDisplay->posY;
//
//				vicDisplay->showGridLines = viewC64->viewC64VicControl->btnShowGrid->IsOn();
//				vicDisplay->SetDisplayPosition(px, py, 0.650f, true);
//				vicDisplay->Render();
//				vicDisplay->SetDisplayPosition(ppx, ppy, 0.129f, true);
		}

	}

//	btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());

	CGuiView::Render();
}

void CViewC64AllGraphicsScreens::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphicsScreens::UpdateShowIOButton()
{
//	btnShowRAMorIO->SetOn( ! viewC64->isDataDirectlyFromRAM);
}

bool CViewC64AllGraphicsScreens::GetIsSelectedItem()
{
	return isSelectedItemScreen;
}

void CViewC64AllGraphicsScreens::SetIsSelectedItem(bool isSelected)
{
	isSelectedItemScreen = isSelected;
}

//@returns is consumed
bool CViewC64AllGraphicsScreens::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsScreens::DoTap:  x=%f y=%f", x, y);
	
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
			{
				vicDisplays[itemId]->DoNotTouchedMove(x, y);
			}
		}
		return true;
	}
	return CGuiViewMovingPane::DoTap(x, y);
}

bool CViewC64AllGraphicsScreens::DoRightClick(float x, float y)
{
	return CGuiViewMovingPane::DoRightClick(x, y);
}

bool CViewC64AllGraphicsScreens::DoNotTouchedMove(float x, float y)
{
//	LOGD("CViewC64AllGraphicsScreens::DoNotTouchedMove: x=%f y=%f", x, y);
	
	if (IsInsideView(x, y))
	{
		if (GetIsSelectedItem() == false)
		{
			ClearCursorPos();

			int itemId = GetItemIdAt(x, y);
			SetSelectedItemId(itemId);

			if (itemId != -1)
			{
				vicDisplays[itemId]->SetAutoScrollMode(AUTOSCROLL_DISASSEMBLY_TEXT_ADDRESS);
				vicDisplays[itemId]->DoNotTouchedMove(x, y);
			}
		}
		else
		{
			int itemId = GetItemIdAt(x, y);
			if (itemId != GetSelectedItemId())
			{
				ClearCursorPos();
			}
		}
	}
	else
	{
		ClearCursorPos();
	}

	return CGuiViewMovingPane::DoNotTouchedMove(x, y);
}

int CViewC64AllGraphicsScreens::GetSelectedItemId()
{
	return selectedScreenId;
}

void CViewC64AllGraphicsScreens::SetSelectedItemId(int itemId)
{
	selectedScreenId = itemId;
}

// TODO: refactor me to GetItemAt(x, y), SetSelectedItem(id)
int CViewC64AllGraphicsScreens::GetItemIdAt(float x, float y)
{
	float displaySizeX = 320.0f * currentZoom;
	float displaySizeY = 200.0f * currentZoom;

	int i = 0;
	float px = renderMapPosX;
	bool found = false;
	for(int dx = 0; dx < numDisplaysColumns; dx++)
	{
		float py = renderMapPosY;
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

void CViewC64AllGraphicsScreens::ClearCursorPos()
{
	for (int i = 0; i < numVicDisplays; i++)
	{
		vicDisplays[i]->ClearRasterCursorPos();
	}
}

void CViewC64AllGraphicsScreens::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
}

void CViewC64AllGraphicsScreens::ClearGraphicsForcedMode()
{
//	this->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
//	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
//	SetSwitchButtonDefaultColors(btnModeHires);
//	SetSwitchButtonDefaultColors(btnModeMulti);
}

bool CViewC64AllGraphicsScreens::DoFinishTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsScreens::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64AllGraphicsScreens::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsScreens::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64AllGraphicsScreens::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsScreens::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphicsScreens::ActivateView()
{
	LOGG("CViewC64AllGraphicsScreens::ActivateView()");
	UpdateShowIOButton();
}

void CViewC64AllGraphicsScreens::DeactivateView()
{
	LOGG("CViewC64AllGraphicsScreens::DeactivateView()");
}

