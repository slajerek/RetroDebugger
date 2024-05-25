#include "CViewC64AllGraphicsCharsets.h"
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
#include "CViewC64AllGraphicsCharsetsControl.h"

CViewC64AllGraphicsCharsets::CViewC64AllGraphicsCharsets(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiViewMovingPane(name, posX, posY, posZ, sizeX, sizeY,
					 // cols=32 *8
					 // rows=8  *8
					 32*8, 8*8)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;

	this->isSelectedItemCharset = false;

	this->selectedCharsetId = 0;
	
	this->charsetScale = 0.3287f;
	this->charsetSizeX = 256.0f * charsetScale;
	this->charsetSizeY = 64.0f * charsetScale;
	this->charsetScaleB = 0.7f;
	this->charsetSizeXB = 256.0f * charsetScaleB;
	this->charsetSizeYB = 64.0f * charsetScaleB;

	this->charsetsOffsetY = 1.0f;
	
	//
	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;

	// charsets sheet, init images for charsets
	for (int i = 0; i < (0x10000/0x800); i++)
	{
		// alloc image that will store charset pixels
		CImageData *imageData = new CImageData(256, 64, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		charsetsImageData.push_back(imageData);
		
		CSlrImage *imageCharset = new CSlrImage(true, false);
		imageCharset->LoadImageForRebinding(imageData, RESOURCE_PRIORITY_STATIC);
		imageCharset->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		imageCharset->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(imageCharset, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);

		charsetsImages.push_back(imageCharset);
	}
	
	minZoom = 0.25f;
	maxZoom = 60.0f;
}

CViewC64AllGraphicsCharsets::~CViewC64AllGraphicsCharsets()
{
}

void CViewC64AllGraphicsCharsets::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphicsCharsets::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64AllGraphicsCharsets::Render()
{
	charsetScale = currentZoom;
	this->charsetSizeX = 256.0f * charsetScale;
	this->charsetSizeY = 64.0f * charsetScale;

	{
		// get VIC colors
		u8 cD021 = viewC64->colorsToShow[1];
		u8 cD022 = viewC64->colorsToShow[2];
		u8 cD023 = viewC64->colorsToShow[3];
		u8 cD800 = viewC64->colorToShowD800;
		
		// render charsets
		UpdateCharsets(viewC64->viewC64MemoryDataDump->renderDataWithColors, cD021, cD022, cD023, cD800);
		
		float startX = renderMapPosX;
		float startY = renderMapPosY;
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
		
		
//		py = startY + charsetSizeY * 8 + 23.0f;
//
////		px = (4.0f * charsetSizeX - charsetSizeXB) / 2.0f;
//		px = posX + (sizeX/2.0f - charsetSizeXB/2.0f);
//		if (imageSelectedCharset)
//		{
////			u16 charsetAddr = selectedCharsetId * 0x0800;
//			Blit(imageSelectedCharset, px, py, posZ, charsetSizeXB, charsetSizeYB);
//			BlitRectangle(px, py, posZ, charsetSizeXB, charsetSizeYB, 1.0, 0.0, 0.0f, 0.7f);
//
////			float fontSize = 4.0f;
////			px = (32.0f * charsetSizeX - charsetSizeXB) / 2.0f;
//		}
//		//
//		if (forcedRenderScreenMode == VIEW_C64_ALL_GRAPHICS_FORCED_NONE)
//		{
////			btnModeBitmapColorsBlackWhite->SetOn(false);
////			btnModeHires->SetOn(! (viewC64->viewC64MemoryDataDump->renderDataWithColors) );
////			btnModeMulti->SetOn(  (viewC64->viewC64MemoryDataDump->renderDataWithColors) );
//		}
//
////		// render color leds for charsets when in multi
////		if (btnModeMulti->IsOn())
////		{
////			px = ledX;
////			py = ledY;
////
////			// D021-D023
////			for (int i = 0x01; i < 0x04; i++)
////			{
////				u8 color = viewC64->colorsToShow[i];
////				bool isForced = (viewC64->viewC64StateVIC->forceColors[i] != -1);
////				RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
////				py += fontSize + step;
////			}
////
////			// D800
////			u8 color = viewC64->colorToShowD800;
////			bool isForced = (viewC64->viewC64StateVIC->forceColorD800 != -1);
////			RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isForced, color, fontSize, viewC64->debugInterfaceC64);
////		}
		
		
	}
	
	//btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());

	CGuiView::Render();
}

void CViewC64AllGraphicsCharsets::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphicsCharsets::UpdateCharsets(bool useColors, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800)
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

//void CViewC64AllGraphicsCharsets::UpdateShowIOButton()
//{
////	btnShowRAMorIO->SetOn( ! viewC64->isDataDirectlyFromRAM);
//}

bool CViewC64AllGraphicsCharsets::GetIsSelectedItem()
{
	return isSelectedItemCharset;
}

void CViewC64AllGraphicsCharsets::SetIsSelectedItem(bool isSelected)
{
	isSelectedItemCharset = isSelected;
}

//@returns is consumed
bool CViewC64AllGraphicsCharsets::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsCharsets::DoTap:  x=%f y=%f", x, y);
	
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


//	{
//		// render color leds for charsets when in multi
//		if (btnModeMulti->IsOn())
//		{
//			float px = ledX;
//			float py = ledY;
//
//			// D021-D023
//			for (int i = 1; i < 4; i++)
//			{
//				if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
//				{
//					if (viewC64->viewC64StateVIC->forceColors[i] == -1)
//					{
//						viewC64->viewC64StateVIC->forceColors[i] = viewC64->colorsToShow[i];
//					}
//					else
//					{
//						viewC64->viewC64StateVIC->forceColors[i] = -1;
//					}
//					guiMain->UnlockMutex();
//					return true;
//				}
//
//				py += fontSize + step;
//			}
//
//			// D800
//			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
//			{
//				if (viewC64->viewC64StateVIC->forceColorD800 == -1)
//				{
//					viewC64->viewC64StateVIC->forceColorD800 = viewC64->colorToShowD800;
//				}
//				else
//				{
//					viewC64->viewC64StateVIC->forceColorD800 = -1;
//				}
//				guiMain->UnlockMutex();
//				return true;
//			}
//		}
//	}
	
	guiMain->UnlockMutex();
	
	return CGuiViewMovingPane::DoTap(x, y);
}

bool CViewC64AllGraphicsCharsets::DoRightClick(float x, float y)
{
	guiMain->LockMutex();
	
	guiMain->UnlockMutex();
	
	return CGuiViewMovingPane::DoRightClick(x, y);
}

bool CViewC64AllGraphicsCharsets::DoNotTouchedMove(float x, float y)
{
//	LOGD("CViewC64AllGraphicsCharsets::DoNotTouchedMove: x=%f y=%f", x, y);

	if (IsInsideView(x, y))
	{
		if (GetIsSelectedItem() == false)
		{
			ClearCursorPos();

			int itemId = GetItemIdAt(x, y);
			SetSelectedItemId(itemId);
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

int CViewC64AllGraphicsCharsets::GetSelectedItemId()
{
	return selectedCharsetId;
}

void CViewC64AllGraphicsCharsets::SetSelectedItemId(int itemId)
{
	selectedCharsetId = itemId;
	u16 charsetAddr = selectedCharsetId * 0x0800;
	viewC64->viewC64MemoryDataDump->ScrollToAddress(charsetAddr);
}

// TODO: refactor me to GetItemAt(x, y), SetSelectedItem(id)
int CViewC64AllGraphicsCharsets::GetItemIdAt(float x, float y)
{
	// find charset over cursor
	int charsetId = 0;
	
	float startX = renderMapPosX;
	float startY = renderMapPosY;
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

void CViewC64AllGraphicsCharsets::ClearCursorPos()
{
}

void CViewC64AllGraphicsCharsets::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
}

void CViewC64AllGraphicsCharsets::ClearGraphicsForcedMode()
{
	this->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
//	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
//	SetSwitchButtonDefaultColors(btnModeHires);
//	SetSwitchButtonDefaultColors(btnModeMulti);
}

bool CViewC64AllGraphicsCharsets::DoFinishTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsCharsets::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64AllGraphicsCharsets::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsCharsets::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64AllGraphicsCharsets::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsCharsets::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewC64AllGraphicsCharsets::HasContextMenuItems()
{
	return true;
}

void CViewC64AllGraphicsCharsets::RenderContextMenuItems()
{
	bool isVisible = viewC64->viewC64AllGraphicsCharsetsControl->visible;
	if (ImGui::MenuItem("All Charsets controller", NULL, &isVisible))
	{
		viewC64->viewC64AllGraphicsCharsetsControl->SetVisible(isVisible);
	}
}

// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphicsCharsets::ActivateView()
{
	LOGG("CViewC64AllGraphicsCharsets::ActivateView()");
//	UpdateShowIOButton();
}

void CViewC64AllGraphicsCharsets::DeactivateView()
{
	LOGG("CViewC64AllGraphicsCharsets::DeactivateView()");
}

