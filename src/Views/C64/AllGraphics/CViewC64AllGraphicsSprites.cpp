#include "CViewC64AllGraphicsSprites.h"
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
#include "CViewC64AllGraphics.h"

CViewC64AllGraphicsSprites::CViewC64AllGraphicsSprites(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiViewMovingPane(name, posX, posY, posZ, sizeX, sizeY,
					 // 32 x 32 sprites
					 32*24, 32*24*2) //32*21)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->consumeTapBackground = false;
	this->allowFocus = false;
	
	forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;

	this->selectedSpriteId = 0;
	
	this->spriteScale = 1.0f;
	this->spriteSizeX = 24.0f * spriteScale;
	this->spriteSizeY = 21.0f * spriteScale;
	
	// selected sprite
	this->spriteScaleB = 2.3f;
	this->spriteSizeXB = 24.0f * spriteScaleB;
	this->spriteSizeYB = 21.0f * spriteScaleB;

	//
	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	fontSize = fontHeight;

	// sprites sheet, init images for sprites
	// 32 x 32 sprites
	for (int i = 0; i < (0x10000/0x40); i++)
	{
		// alloc image that will store sprite pixels
		CImageData *imageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		spritesImageData.push_back(imageData);
		
		CSlrImage *imageSprite = new CSlrImage(true, false);
		imageSprite->LoadImageForRebinding(imageData, RESOURCE_PRIORITY_STATIC);
		imageSprite->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		imageSprite->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(imageSprite, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);

		spritesImages.push_back(imageSprite);
	}
	
	minZoom = 0.25f;
	maxZoom = 60.0f;
}

CViewC64AllGraphicsSprites::~CViewC64AllGraphicsSprites()
{
}

void CViewC64AllGraphicsSprites::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64AllGraphicsSprites::RenderImGui()
{
	PreRenderImGui();
	
	VID_SetClipping(posX, posY, sizeX, sizeY);
	
	this->spriteScale = currentZoom;
	this->spriteSizeX = 24.0f * spriteScale;
	this->spriteSizeY = 21.0f * spriteScale;

//	LOGD("currentZoom=%f spriteScale=%f", currentZoom, spriteScale);

	Render();
	
	VID_ResetClipping();
	
	PostRenderImGui();
}

void CViewC64AllGraphicsSprites::Render()
{
	float startX = renderMapPosX;
	float startY = renderMapPosY;
	
	// get VIC colors
	u8 cD021 = viewC64->colorsToShow[1];
	u8 cD025 = viewC64->colorsToShow[5];
	u8 cD026 = viewC64->colorsToShow[6];
	u8 cD027 = viewC64->colorsToShow[7];
	
	// render sprites
	UpdateSprites(viewC64->viewC64MemoryDataDump->renderDataWithColors, cD021, cD025, cD026, cD027);
	
	float px = startX;
	float py = startY;
	
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
	
	// render selected sprite

	/*
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
	 
	 */

//	btnShowGrid->SetOn(viewC64->viewC64VicControl->btnShowGrid->IsOn());

	CGuiView::Render();
}

void CViewC64AllGraphicsSprites::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewC64AllGraphicsSprites::UpdateSprites(bool useColors, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027)
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
		
		image->ReBindImage();
		
		itImage++;
		itImageData++;
	}
}
 
//@returns is consumed
bool CViewC64AllGraphicsSprites::DoTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsSprites::DoTap:  x=%f y=%f", x, y);
	
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
	

	guiMain->UnlockMutex();
	
	return CGuiViewMovingPane::DoTap(x, y);
}

bool CViewC64AllGraphicsSprites::DoRightClick(float x, float y)
{
	guiMain->LockMutex();
	
	guiMain->UnlockMutex();
	
	return CGuiViewMovingPane::DoRightClick(x, y);
}

bool CViewC64AllGraphicsSprites::DoNotTouchedMove(float x, float y)
{
	LOGG("CViewC64AllGraphicsSprites::DoNotTouchedMove: x=%f y=%f", x, y);
	
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

bool CViewC64AllGraphicsSprites::GetIsSelectedItem()
{
	return isSelectedItemSprite;
}

void CViewC64AllGraphicsSprites::SetIsSelectedItem(bool isSelected)
{
	isSelectedItemSprite = isSelected;
}

int CViewC64AllGraphicsSprites::GetSelectedItemId()
{
	return selectedSpriteId;
}

void CViewC64AllGraphicsSprites::SetSelectedItemId(int itemId)
{
	selectedSpriteId = itemId;
	u16 spriteAddr = selectedSpriteId * 0x0040;
	viewC64->viewC64MemoryDataDump->ScrollToAddress(spriteAddr);
}

void CViewC64AllGraphicsSprites::ClearCursorPos()
{
	//
}

// TODO: refactor me to GetItemAt(x, y), SetSelectedItem(id)
int CViewC64AllGraphicsSprites::GetItemIdAt(float x, float y)
{
	// find sprite over cursor
	int spriteId = 0;
	
	float startX = renderMapPosX;
	float startY = renderMapPosY;
	float px = startX;
	float py = startY;
	
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

void CViewC64AllGraphicsSprites::UpdateRenderDataWithColors()
{
	// ctrl+k shortcut was pressed
//	if (displayMode == VIEW_C64_ALL_GRAPHICS_MODE_SPRITES)
	{
		ClearGraphicsForcedMode();
	}
}

void CViewC64AllGraphicsSprites::ClearGraphicsForcedMode()
{
	this->forcedRenderScreenMode = VIEW_C64_ALL_GRAPHICS_FORCED_NONE;
//	SetSwitchButtonDefaultColors(btnModeBitmapColorsBlackWhite);
//	SetSwitchButtonDefaultColors(btnModeHires);
//	SetSwitchButtonDefaultColors(btnModeMulti);
}




bool CViewC64AllGraphicsSprites::DoFinishTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsSprites::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64AllGraphicsSprites::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsSprites::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64AllGraphicsSprites::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64AllGraphicsSprites::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


// bug: this event is not called when layout is set, and button state is updated on keyboard shortcut only
void CViewC64AllGraphicsSprites::ActivateView()
{
	LOGG("CViewC64AllGraphicsSprites::ActivateView()");
//	UpdateShowIOButton();
}

void CViewC64AllGraphicsSprites::DeactivateView()
{
	LOGG("CViewC64AllGraphicsSprites::DeactivateView()");
}

