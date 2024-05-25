#include "CViewC64AllGraphicsBitmaps.h"
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
#include "CViewC64AllGraphicsBitmapsControl.h"
#include "CViewC64.h"

CViewC64AllGraphicsBitmaps::CViewC64AllGraphicsBitmaps(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface, CViewC64AllGraphicsBitmapsControl *viewControl)
: CGuiViewMovingPane(name, posX, posY, posZ, sizeX, sizeY,
					 // numDisplaysColumns = 2;  numDisplaysRows = 0x10000/0x2000 / numDisplaysColumns;
					 2*320, 8*200)
{
	this->debugInterface = debugInterface;
	this->viewControl = viewControl;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	this->isSelectedItemBitmap = false;

	this->selectedBitmapId = 0;
	
	numBitmapDisplays = 0x10000/0x2000;	//8
	
	numVicDisplays = numBitmapDisplays;	// 0x40
	vicDisplays = new CViewC64VicDisplay *[numVicDisplays];
	vicControl = new CViewC64VicControl *[numVicDisplays];
	for (int i = 0; i < numVicDisplays; i++)
	{
		vicDisplays[i] = new CViewC64VicDisplay("VicDisplay/CViewC64AllGraphicsBitmaps", 0, 0, posZ, 100, 100, debugInterface);
		vicDisplays[i]->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_NONE);
		
		vicDisplays[i]->showSpritesFrames = false;
		vicDisplays[i]->showSpritesGraphics = false;
		vicDisplays[i]->performIsTopWindowCheck = false;

		vicControl[i] = new CViewC64VicControl("VicDisplay/CViewC64AllGraphicsBitmaps", 0, 0, posZ, 100, 100, vicDisplays[i]);
		vicControl[i]->SetVisible(false);

		vicDisplays[i]->SetVicControlView(vicControl[i]);
		
		vicControl[i]->lstScreenAddresses->SetListLocked(true);
	}
	
	//
	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;
	
	minZoom = 0.25f;
	maxZoom = 60.0f;

	SetupVicDisplays();
}

CViewC64AllGraphicsBitmaps::~CViewC64AllGraphicsBitmaps()
{
}

void CViewC64AllGraphicsBitmaps::SetupVicDisplays()
{
	guiMain->LockMutex();

	numVisibleDisplays = numBitmapDisplays;

	numDisplaysColumns = 2;
	numDisplaysRows = (float)numVisibleDisplays / (float)numDisplaysColumns;

	float displaySizeX = 320.0f * currentZoom;
	float displaySizeY = 200.0f * currentZoom;
	
//	LOGD("renderMapPosX=%f renderMapPosY=%f", renderMapPosX, renderMapPosY);
	
	int i = 0;
	float px = renderMapPosX;
	for(int x = 0; x < numDisplaysColumns; x++)
	{
		float py = renderMapPosY;
		for (int y = 0; y < numDisplaysRows; y++)
		{
//			LOGD("......px=%f py=%f", px, py);
			vicDisplays[i]->SetDisplayPosition(px, py, currentZoom, true);
			py += displaySizeY;
			
			vicControl[i]->btnModeBitmap->SetOn(true);
			vicControl[i]->lstBitmapAddresses->SetListLocked(true);
			vicControl[i]->lstBitmapAddresses->SetElement(i, false, false);

			i++;
		}
		px += displaySizeX;
	}
	guiMain->UnlockMutex();
}

void CViewC64AllGraphicsBitmaps::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphicsBitmaps::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64AllGraphicsBitmaps::Render()
{
	SetupVicDisplays();
	
	{
		u16 screenAddress;
		vicii_cycle_state_t *viciiState = &viewC64->viciiStateToShow;
		
		screenAddress = viciiState->vbank_phi2 + ((viciiState->regs[0x18] & 0xf0) << 6);
		screenAddress = (screenAddress & viciiState->vaddr_mask_phi2) | viciiState->vaddr_offset_phi2;

		if (viewControl->lstScreenAddresses->isLocked)
		{
			screenAddress = viewControl->lstScreenAddresses->selectedElement * 0x0400;
		}
		else
		{
			bool updatePosition = true;

			if (viewControl->lstScreenAddresses->IsInside(guiMain->mousePosX, guiMain->mousePosY))
				updatePosition = false;

			// update controls
			int addrItemNum = screenAddress / 0x0400;
			viewControl->lstScreenAddresses->SetElement(addrItemNum, updatePosition, false);
		}
		
		// vic displays
		for (int i = 0; i < numVisibleDisplays; i++)
		{
			//		LOGD("Render VIC Display %d", i);
			vicDisplays[i]->viewVicControl->forceDataFromRam = viewC64->isDataDirectlyFromRAM;
			
			vicDisplays[i]->viewVicControl->btnModeText->SetOn(false);
			vicDisplays[i]->viewVicControl->btnModeBitmap->SetOn(true);
			int screenAddrItemNum = screenAddress / 0x0400;
			vicDisplays[i]->viewVicControl->lstScreenAddresses->SetElement(screenAddrItemNum, false, false);
			vicDisplays[i]->showGridLines = viewC64->viewC64VicControl->btnShowGrid->IsOn();

			vicDisplays[i]->viewVicControl->btnModeStandard->SetOn(true);
			vicDisplays[i]->viewVicControl->btnModeExtended->SetOn(false);

//			LOGD("forcedRenderScreenMode=%d", forcedRenderScreenMode);
			switch(viewControl->forcedRenderScreenMode)
			{
				case VIEW_C64_ALL_GRAPHICS_FORCED_NONE:
					vicDisplays[i]->viewVicControl->forceGrayscaleColors = false;
					vicDisplays[i]->viewVicControl->btnModeHires->SetOn(!viewControl->btnModeMulti->IsOn());
					vicDisplays[i]->viewVicControl->btnModeMulti->SetOn(viewControl->btnModeMulti->IsOn());
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
		
		{
			// render selected bitmap outline only
			if (selectedBitmapId >=0 && selectedBitmapId < 0x40)
			{
				CViewC64VicDisplay *vicDisplay = vicDisplays[selectedBitmapId];
				BlitRectangle(vicDisplay->posX, vicDisplay->posY, posZ, vicDisplay->sizeX, vicDisplay->sizeY, 1.0, 0.0, 0.0f, 0.7f);
			}
		}
	}

//	btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());

	CGuiView::Render();
}

void CViewC64AllGraphicsBitmaps::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphicsBitmaps::UpdateShowIOButton()
{
//	btnShowRAMorIO->SetOn( ! viewC64->isDataDirectlyFromRAM);
}

bool CViewC64AllGraphicsBitmaps::GetIsSelectedItem()
{
	return isSelectedItemBitmap;
}

void CViewC64AllGraphicsBitmaps::SetIsSelectedItem(bool isSelected)
{
	isSelectedItemBitmap = isSelected;
}

//@returns is consumed
bool CViewC64AllGraphicsBitmaps::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsBitmaps::DoTap:  x=%f y=%f", x, y);
	
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
			
			{
				vicDisplays[itemId]->DoNotTouchedMove(x, y);
			}
		}
		return true;
	}
	
	return CGuiViewMovingPane::DoTap(x, y);
}

bool CViewC64AllGraphicsBitmaps::DoRightClick(float x, float y)
{
	return CGuiViewMovingPane::DoRightClick(x, y);
}

bool CViewC64AllGraphicsBitmaps::DoNotTouchedMove(float x, float y)
{
//	LOGD("CViewC64AllGraphicsBitmaps::DoNotTouchedMove: x=%f y=%f", x, y);

	if (IsInsideView(x, y))
	{
		if (GetIsSelectedItem() == false)
		{
			ClearCursorPos();

			int itemId = GetItemIdAt(x, y);
			SetSelectedItemId(itemId);

			if (itemId != -1)
			{
				vicDisplays[itemId]->SetAutoScrollMode(AUTOSCROLL_DISASSEMBLY_BITMAP_ADDRESS);
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

int CViewC64AllGraphicsBitmaps::GetSelectedItemId()
{
	return selectedBitmapId;
}

void CViewC64AllGraphicsBitmaps::SetSelectedItemId(int itemId)
{
	selectedBitmapId = itemId;
		//	//u16 bitmapAddr = selectedBitmapId * 0x2000;
		//	//viewC64->viewC64MemoryDataDump->ScrollToAddress(bitmapAddr);
}

// TODO: refactor me to GetItemAt(x, y)   (no id), SetSelectedItem( pointer )
int CViewC64AllGraphicsBitmaps::GetItemIdAt(float x, float y)
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
	
	if (i < 0x08)
	{
		return i;
	}
	return -1;
}

void CViewC64AllGraphicsBitmaps::ClearCursorPos()
{
	for (int i = 0; i < numVicDisplays; i++)
	{
		vicDisplays[i]->ClearRasterCursorPos();
	}
}

void CViewC64AllGraphicsBitmaps::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
}

void CViewC64AllGraphicsBitmaps::ClearGraphicsForcedMode()
{
	viewControl->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
//	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
//	SetSwitchButtonDefaultColors(btnModeHires);
//	SetSwitchButtonDefaultColors(btnModeMulti);
}

bool CViewC64AllGraphicsBitmaps::DoFinishTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsBitmaps::DoFinishTap: %f %f", x, y);
	return CGuiViewMovingPane::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64AllGraphicsBitmaps::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsBitmaps::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiViewMovingPane::DoDoubleTap(x, y);
}

bool CViewC64AllGraphicsBitmaps::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsBitmaps::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewC64AllGraphicsBitmaps::HasContextMenuItems()
{
	return true;
}

void CViewC64AllGraphicsBitmaps::RenderContextMenuItems()
{
	bool isVisible = viewControl->visible;
	if (ImGui::MenuItem("All Bitmaps controller", NULL, &isVisible))
	{
		viewControl->SetVisible(isVisible);
	}
}

// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphicsBitmaps::ActivateView()
{
	LOGG("CViewC64AllGraphicsBitmaps::ActivateView()");
	UpdateShowIOButton();
}

void CViewC64AllGraphicsBitmaps::DeactivateView()
{
	LOGG("CViewC64AllGraphicsBitmaps::DeactivateView()");
}

