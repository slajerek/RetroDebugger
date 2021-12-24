#include "CViewC64Sprite.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "CDebugInterfaceVice.h"
#include "CViewVicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "SYS_Funct.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "C64SettingsStorage.h"
#include "CSlrFileFromOS.h"

CViewC64Sprite::CViewC64Sprite(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewVicEditor *vicEditor)
: CGuiWindow(name, posX, posY, posZ, sizeX, sizeY, new CSlrString("Sprite"), NULL)
{
	this->vicEditor = vicEditor;
	
	viciiState = NULL;
	
	int w = 32;
	int h = 32;
	imageDataSprite = new CImageData(w, h, IMG_TYPE_RGBA);
	imageDataSprite->AllocImage(false, true);
	
	imageSprite = new CSlrImage(true, false);
	imageSprite->LoadImage(imageDataSprite, RESOURCE_PRIORITY_STATIC, false);
	imageSprite->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	imageSprite->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(imageSprite, NULL);

	spriteRasterX = -1;
	spriteRasterY = -1;
	
	//
	font = viewC64->fontCBMShifted;
	fontScale = 1;
	fontWidth = font->GetCharWidth('@', fontScale);
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	float buttonSizeX;
	float buttonSizeY = 10.0f;
	float px;
	float py;
	
	buttonSizeX = 30.0f;
	px = 2.5f; //posX + 2.5f;
	py = 5.0f + 28.0f + 3.0f; //posY + 5.0f + 28.0f + 3.0f;
	
	float gpx = 1.9f;
	
	btnIsMultiColor = new CGuiButtonSwitch(NULL, NULL, NULL,
											  px, py, posZ, buttonSizeX, buttonSizeY,
											  new CSlrString("Multi"),
											  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
											  font, fontScale,
											  1.0, 1.0, 1.0, 1.0,
											  1.0, 1.0, 1.0, 1.0,
											  0.3, 0.3, 0.3, 1.0,
											  this);
	btnIsMultiColor->SetOn(false);
	this->AddGuiElement(btnIsMultiColor);
	
	px += buttonSizeX + gpx;
	
	btnIsStretchX = new CGuiButtonSwitch(NULL, NULL, NULL,
										 px, py, posZ, buttonSizeX, buttonSizeY,
										 new CSlrString("Horiz"),
										 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										 font, fontScale,
										 1.0, 1.0, 1.0, 1.0,
										 1.0, 1.0, 1.0, 1.0,
										 0.3, 0.3, 0.3, 1.0,
										 this);
	btnIsStretchX->SetOn(false);
	this->AddGuiElement(btnIsStretchX);
	
	px += buttonSizeX + gpx;
	
	btnIsStretchY = new CGuiButtonSwitch(NULL, NULL, NULL,
										 px, py, posZ, buttonSizeX, buttonSizeY,
										 new CSlrString("Vert"),
										 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
										 font, fontScale,
										 1.0, 1.0, 1.0, 1.0,
										 1.0, 1.0, 1.0, 1.0,
										 0.3, 0.3, 0.3, 1.0,
										 this);
	btnIsStretchY->SetOn(false);
	this->AddGuiElement(btnIsStretchY);
	

	//
	buttonSizeX = 25.0f;
	px = sizeX - buttonSizeX - 3.0f;	//posX +
	py += buttonSizeY + 6.0f;
	btnScanForSprites = new CGuiButtonSwitch(NULL, NULL, NULL,
											  px, py, posZ, buttonSizeX, buttonSizeY,
											  new CSlrString("Scan"),
											  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
											  font, fontScale,
											  1.0, 1.0, 1.0, 1.0,
											  1.0, 1.0, 1.0, 1.0,
											  0.3, 0.3, 0.3, 1.0,
											  this);
	btnScanForSprites->SetOn(true);
	this->AddGuiElement(btnScanForSprites);
	
	//
	
	prevSpriteId = 0;
	selectedColor = -1;
	isSpriteLocked = false;
	
	spriteFileExtensions.push_back(new CSlrString("sprite"));
	spriteFileExtensions.push_back(new CSlrString("bin"));
	spriteFileExtensions.push_back(new CSlrString("data"));

	// setup icons in frame
	viewFrame->AddBarToolBox(this);
	
	imgIconImport = RES_GetImage("/gfx/icon_raw_import", true);
	viewFrame->AddBarIcon(imgIconImport);
	
	imgIconExport = RES_GetImage("/gfx/icon_raw_export", true);
	viewFrame->AddBarIcon(imgIconExport);
	
	this->UpdatePosition();

}

void CViewC64Sprite::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewC64Sprite::SetPosition: %f %f", posX, posY);
	CGuiWindow::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64Sprite::DoLogic()
{
	CGuiWindow::DoLogic();
}


void CViewC64Sprite::Render()
{
//	LOGD("CViewC64Sprite::Render: pos=%f %f", posX, posY);
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;

	
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, viewFrame->barColorR, viewFrame->barColorG, viewFrame->barColorB, 1);

	float px = posX + 3.0f;
	float py = posY + 3.0f;
	
	const float spriteSizeX = 34.0f;
	const float spriteSizeY = (21.0f * spriteSizeX) / 24.0f;

	C64Sprite *sprite = vicEditor->layerVirtualSprites->FindSpriteByRasterPos(spriteRasterX, spriteRasterY);
	
	int spriteId = 0;
	
	int spc = (spriteRasterX + 0x18)/8.0f;
	int spy = spriteRasterY + 0x32;
	
	viciiState = NULL;
	
	if (spy >= 0 && spy < 312
		&& spc >= 0 && spc < 64)
	{
		viciiState = c64d_get_vicii_state_for_raster_cycle(spy+2, spc);
	}
	
	if (viciiState != NULL)
	{
		paintColorD021	= viciiState->regs[0x21];
		paintColorD025	= viciiState->regs[0x25];
		paintColorD026	= viciiState->regs[0x26];
		
		if (sprite != NULL)
		{
			paintColorSprite = viciiState->regs[0x27 + (sprite->spriteId)];
			
			prevSpriteId = sprite->spriteId;
			
			spriteId = sprite->spriteId;

			int addr1 = sprite->pointerAddr;

			int v_bank = viciiState->vbank_phi1;
			int addr2 = v_bank + viciiState->sprite[sprite->spriteId].pointer * 64;
			
			float py2 = py + 46;
			
			char buf[32];
			sprintf(buf, "%d %04x (%04x)", spriteId, addr1, addr2);
			viewC64->fontDisassembly->BlitText(buf, px, py2, -1, 5);

			int sprx = sprite->posX + 0x18;
			int spry = sprite->posY + 0x32;
			
			py2 += 8.0f;
			
			if (sprx < 0)
				sprx += 504;
			
			sprintf(buf, "%02x    %3d %3d", viciiState->sprite[sprite->spriteId].pointer, sprx, spry);
			viewC64->fontDisassembly->BlitText(buf, px, py2, -1, 5);
			
			int addr = addr2;
			
			for (int i = 0; i < 63; i++)
			{
				u8 v = debugInterface->GetByteFromRamC64(addr);
				currentSpriteData[i] = v;
				addr++;
			}
			
			const float spriteTexStartX = 4.0/32.0;
			const float spriteTexStartY = 4.0/32.0;
			const float spriteTexEndX = (4.0+24.0)/32.0;
			const float spriteTexEndY = (4.0+21.0)/32.0;
			
			bool isColor = false;
			if (viciiState->regs[0x1c] & (1 << (sprite->spriteId)))
			{
				isColor = true;
			}
			if (isColor == false)
			{
				//			if (renderDataWithColors)
				//			{
				//				uint8 spriteColor = viciiState->regs[0x27+(sprite->spriteId)];
				//				ConvertSpriteDataToImage(spriteData, imageData, cD021, spriteColor, this);
				//			}
				//			else
				{
					ConvertSpriteDataToImage(currentSpriteData, imageDataSprite, 4);
				}
			}
			else
			{
				ConvertColorSpriteDataToImage(currentSpriteData, imageDataSprite,
											  paintColorD021, paintColorD025, paintColorD026, paintColorSprite,
											  debugInterface, 4, 0);
			}
			
			// re-bind image
			imageSprite->ReBindImageData(imageDataSprite);
			
			float cD021r, cD021g, cD021b;
			debugInterface->GetFloatCBMColor(paintColorD021, &cD021r, &cD021g, &cD021b);

			BlitFilledRectangle(px, py, posZ, spriteSizeX, spriteSizeY, cD021r, cD021g, cD021b, 1.0f);
			
			Blit(imageSprite, px, py, posZ, spriteSizeX, spriteSizeY, spriteTexStartX, spriteTexStartY, spriteTexEndX, spriteTexEndY);
			
			// buttons
			btnIsMultiColor->SetEnabled(true);
			btnIsStretchX->SetEnabled(true);
			btnIsStretchY->SetEnabled(true);
			
			if ((viciiState->regs[0x1C] >> sprite->spriteId) & 1)
			{
				btnIsMultiColor->SetOn(true);
			}
			else
			{
				btnIsMultiColor->SetOn(false);
			}

			if ((viciiState->regs[0x1D] >> sprite->spriteId) & 1)
			{
				btnIsStretchX->SetOn(true);
			}
			else
			{
				btnIsStretchX->SetOn(false);
			}

			if ((viciiState->regs[0x17] >> sprite->spriteId) & 1)
			{
				btnIsStretchY->SetOn(true);
			}
			else
			{
				btnIsStretchY->SetOn(false);
			}
		}
		else
		{
			paintColorSprite	= viciiState->regs[0x27 + prevSpriteId];
			
			btnIsMultiColor->SetEnabled(false);
			btnIsStretchX->SetEnabled(false);
			btnIsStretchY->SetEnabled(false);
			btnIsMultiColor->SetOn(false);
			btnIsStretchX->SetOn(false);
			btnIsStretchY->SetOn(false);
		}

	}
	
	BlitRectangle(px, py, posZ, spriteSizeX, spriteSizeY, 0.0, 0.0, 0.0f, 1.0f);

	// render color leds
	float fontSize = 5.0f;
	
	// render color LEDs
	char buf[8] = { 'D', '0', '2', '0', 0x00 };

	float ledX = posX + spriteSizeX + 5.0f + 3.0f;
	float ledY = posY + 2.0f;
	
	//char buf[8] = { 'D', '0', '2', '0', 0x00 };
	float ledSizeX = fontSize*5.0f; //4.0f;
	float gap = fontSize * 0.1f;
	float step = fontSize * 0.75f;
	float ledSizeY = fontSize + gap + gap + 2.0f;
	
	px = ledX;
	py = ledY;
	float py2 = py + fontSize + gap;
	
	bool isSelected;
	
	// 0 00 = D021
	// 1 01 = D025
	// 2 10 = D027+sprId
	// 3 11 = D026

	// D021
	sprintfHexCode4WithoutZeroEnding(&(buf[3]), 0x21);
	viewC64->fontDisassembly->BlitText(buf, px, py, posZ, fontSize);
	
	isSelected = (selectedColor == 0);
	RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isSelected, paintColorD021, debugInterface);
	
	px += ledSizeX + step;
	
	//
	sprintfHexCode4WithoutZeroEnding(&(buf[3]), 0x25);
	viewC64->fontDisassembly->BlitText(buf, px, py, posZ, fontSize);
	
	isSelected = (selectedColor == 1);
	RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isSelected, paintColorD025, debugInterface);

	py += ledSizeY + gap + fontSize + gap + 3.0f;
	py2 = py + fontSize + gap;
	px = ledX;

	//
	sprintfHexCode4WithoutZeroEnding(&(buf[3]), 0x27 + prevSpriteId);
	viewC64->fontDisassembly->BlitText(buf, px, py, posZ, fontSize);
	
	isSelected = (selectedColor == 2);
	RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isSelected, paintColorSprite, debugInterface);

	px += ledSizeX + step;
	
	//
	sprintfHexCode4WithoutZeroEnding(&(buf[3]), 0x26);
	viewC64->fontDisassembly->BlitText(buf, px, py, posZ, fontSize);

	isSelected = (selectedColor == 3);
	RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isSelected, paintColorD026, debugInterface);
	
	///
	
	CGuiWindow::Render();

	if (isSpriteLocked == false)
	{
		BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
	}
	else
	{
		BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0.5f, 0, 0, 1, 1);
	}
	
	
}

bool CViewC64Sprite::DoTap(float x, float y)
{
	LOGG("CViewC64Sprite::DoTap: %f %f", x, y);

	if (this->IsInsideView(x, y))
	{
		// check tap on colors
		float px = posX + 5.0f;
		float py = posY + 5.0f;
		
		const float spriteSizeX = 34.0f;
		const float spriteSizeY = (21.0f * spriteSizeX) / 24.0f;

		// render color leds
		float fontSize = 5.0f;
		
		// render color LEDs
		float ledX = posX + spriteSizeX + 5.0f + 3.0f;
		float ledY = posY + 2.0f;
		
		//char buf[8] = { 'D', '0', '2', '0', 0x00 };
		float ledSizeX = fontSize*5.0f; //4.0f;
		float gap = fontSize * 0.1f;
		float step = fontSize * 0.75f;
		float ledSizeY = fontSize + gap + gap + 2.0f;
		
		px = ledX;
		py = ledY;
		float py2 = py + fontSize + gap;
		
		float sy = (ledSizeY + fontSize + step + gap);
		
		// 0 00 = D021
		// 1 01 = D025
		// 2 10 = D027+sprId
		// 3 11 = D026

		// D021
		if (x >= px && x <= px + ledSizeX
			&& y >= py && y <= py + sy)
		{
			LOGD("D021");
			selectedColor = 0;
			
			UpdateSelectedColorInPalette();
			return true;
		}
		
		px += ledSizeX + step;
		
		if (x >= px && x <= px + ledSizeX
				 && y >= py && y <= py + sy)
		{
			LOGD("D025");
			selectedColor = 1;
			
			UpdateSelectedColorInPalette();
			return true;
		}
		
		py += ledSizeY + gap + fontSize + gap + 3.0f;
		py2 = py + fontSize + gap;
		px = ledX;

		if (x >= px && x <= px + ledSizeX
			&& y >= py && y <= py + sy)
		{
			LOGD("D027+spr");
			selectedColor = 2;

			UpdateSelectedColorInPalette();
			return true;
		}

		px += ledSizeX + step;

		if (x >= px && x <= px + ledSizeX
			&& y >= py && y <= py + sy)
		{
			LOGD("D026");
			selectedColor = 3;

			UpdateSelectedColorInPalette();
			return true;
		}
		
		selectedColor = -1;
		return true;
	}
	
	
	return CGuiWindow::DoTap(x, y);
}

bool CViewC64Sprite::DoFinishTap(float x, float y)
{
	// render thread is updating switch buttons, we need to lock render first
	guiMain->LockMutex();
	
	bool ret = CGuiWindow::DoFinishTap(x, y);
	
	guiMain->UnlockMutex();
	
	return ret;
}



bool CViewC64Sprite::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiWindow::DoMove(x, y, distX, distY, diffX, diffY);
}


void CViewC64Sprite::UpdateSelectedColorInPalette()
{
	if (selectedColor == -1)
		return;

	C64Sprite *sprite = vicEditor->layerVirtualSprites->FindSpriteByRasterPos(spriteRasterX, spriteRasterY);
	if (!sprite)
		return;

	int spc = (spriteRasterX + 0x18)/8.0f;
	int spy = spriteRasterY + 0x32;

	if (spy >= 0 && spy < 312
		&& spc >= 0 && spc < 64)
	{
		viciiState = c64d_get_vicii_state_for_raster_cycle(spy, spc);
		
		// 0 00 = D021
		// 1 01 = D025
		// 2 10 = D027+sprId
		// 3 11 = D026

		if (selectedColor == 0)
		{
			vicEditor->viewPalette->SetColorLMB(viciiState->regs[0x21]);
		}
		else if (selectedColor == 1)
		{
			vicEditor->viewPalette->SetColorLMB(viciiState->regs[0x25]);
		}
		else if (selectedColor == 1)
		{
			vicEditor->viewPalette->SetColorLMB(viciiState->regs[0x27 + sprite->spriteId]);
		}
		else if (selectedColor == 3)
		{
			vicEditor->viewPalette->SetColorLMB(viciiState->regs[0x26]);
		}
	}
	
}

u8 CViewC64Sprite::GetPaintColorByNum(u8 colorNum)
{
	switch(colorNum)
	{
		case 0:
			return paintColorD021;
		case 1:
			return paintColorD025;
		case 2:
			return paintColorSprite;
		case 3:
			return paintColorD026;
	}
	
	return paintColorSprite;
}

bool CViewC64Sprite::DoRightClick(float x, float y)
{
	LOGI("CViewC64Sprite::DoRightClick: %f %f", x, y);
	
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiWindow::DoRightClick(x, y);
}


bool CViewC64Sprite::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewC64Sprite::KeyDown: %d", keyCode);
	
	if ((keyCode == '1' && isShift && !isAlt && !isControl)
		|| keyCode == '!')
	{
		selectedColor = 1;
		return true;
	}
	else if ((keyCode == '2' && isShift && !isAlt && !isControl)
			 || keyCode == '@')
	{
		selectedColor = 2;
		return true;
	}
	else if ((keyCode == '3' && isShift && !isAlt && !isControl)
			 || keyCode == '#')
	{
		selectedColor = 3;
		return true;
	}

	if (isSpriteLocked)
	{
		int step = (guiMain->isShiftPressed ? 4 : 1);
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			MoveSelectedSprite(-step, 0);
			return true;
		}
		else if (keyCode == MTKEY_ARROW_RIGHT)
		{
			MoveSelectedSprite(+step, 0);
			return true;
		}
		else if (keyCode == MTKEY_ARROW_UP)
		{
			MoveSelectedSprite(0, -step);
			return true;
		}
		else if (keyCode == MTKEY_ARROW_DOWN)
		{
			MoveSelectedSprite(0, +step);
			return true;
		}
	}
	
	return false;
}

bool CViewC64Sprite::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64Sprite::MoveSelectedSprite(int deltaX, int deltaY)
{
	if (isSpriteLocked == false)
		return;
	
	guiMain->LockMutex();
	C64Sprite *sprite = vicEditor->layerVirtualSprites->FindSpriteByRasterPos(spriteRasterX, spriteRasterY);
	
	if (sprite == NULL)
	{
		LOGError("CViewC64Sprite::MoveSelectedSprite");
		guiMain->UnlockMutex();
		return;
	}
	
	vicEditor->StoreUndo();

	if (deltaX != 0)
	{
		int sprx = sprite->posX + 0x18;
		vicEditor->layerVirtualSprites->ReplacePosX(sprite->posX, sprite->posY, sprite->spriteId, sprx + deltaX);
		spriteRasterX += deltaX;
	}
	
	if (deltaY != 0)
	{
		int spry = sprite->posY + 0x32;
		vicEditor->layerVirtualSprites->ReplacePosY(sprite->posX, sprite->posY, sprite->spriteId, spry + deltaY);
		spriteRasterY += deltaY;
	}
	guiMain->UnlockMutex();
}

void CViewC64Sprite::PaletteColorChanged(u8 colorId, u8 newColorValue)
{
	LOGD("CViewC64Sprite::PaletteColorChanged");
	
	guiMain->LockMutex();
	if (colorId == VICEDITOR_COLOR_SOURCE_LMB && selectedColor != -1)
	{
		C64Sprite *sprite = vicEditor->layerVirtualSprites->FindSpriteByRasterPos(spriteRasterX, spriteRasterY);
		
		if (sprite != NULL)
		{
			vicEditor->StoreUndo();
			
			if (selectedColor == 0)
			{
				vicEditor->viewVicDisplayMain->debugInterface->SetByteC64(0xD021, newColorValue);
				guiMain->UnlockMutex();
				return;
			}
			
			int paintResult = vicEditor->layerVirtualSprites->ReplaceColor(spriteRasterX, spriteRasterY, sprite->spriteId, selectedColor, newColorValue);
			
			if (paintResult == PAINT_RESULT_ERROR)
			{
				guiMain->ShowMessage("Error in Sprite replace color");
			}
		}
	}
	
	guiMain->UnlockMutex();

}

bool CViewC64Sprite::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	LOGD("CViewC64Sprite::ButtonSwitchChanged");
	
	guiMain->LockMutex();
	C64Sprite *sprite = vicEditor->layerVirtualSprites->FindSpriteByRasterPos(spriteRasterX, spriteRasterY);
	
	if (sprite != NULL)
	{
		if (button == btnIsMultiColor)
		{
			vicEditor->StoreUndo();
			vicEditor->layerVirtualSprites->ReplaceMultiColor(spriteRasterX, spriteRasterY, sprite->spriteId, btnIsMultiColor->IsOn());
		}
		else if (button == btnIsStretchX)
		{
			vicEditor->StoreUndo();
			vicEditor->layerVirtualSprites->ReplaceStretchX(spriteRasterX, spriteRasterY, sprite->spriteId, btnIsStretchX->IsOn());
		}
		else if (button == btnIsStretchY)
		{
			vicEditor->StoreUndo();
			vicEditor->layerVirtualSprites->ReplaceStretchY(spriteRasterX, spriteRasterY, sprite->spriteId, btnIsStretchY->IsOn());
		}
	}
	
	guiMain->UnlockMutex();
	return true;
}

//
void CViewC64Sprite::ToolBoxIconPressed(CSlrImage *imgIcon)
{
	if (imgIcon == this->imgIconExport)
	{
		CSlrString *defaultFileName = new CSlrString("sprite");
		
		CSlrString *windowTitle = new CSlrString("Export Sprite");
		viewC64->ShowDialogSaveFile(this, &spriteFileExtensions, defaultFileName, c64SettingsDefaultSnapshotsFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
	}
	else if (imgIcon == this->imgIconImport)
	{
		CSlrString *windowTitle = new CSlrString("Import Sprite");
		windowTitle->DebugPrint("windowTitle=");
		viewC64->ShowDialogOpenFile(this, &spriteFileExtensions, NULL, windowTitle);
		delete windowTitle;
	}
}

void CViewC64Sprite::SystemDialogFileOpenSelected(CSlrString *path)
{
	int importSpriteAddr = this->ImportSprite(path);
	
	if (importSpriteAddr < 0)
	{
		return;
	}
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	
	char *buf = str->GetStdASCII();
	char *buf2 = SYS_GetCharBuf();
	sprintf(buf2, "%s imported at $%04x", buf, importSpriteAddr);
	guiMain->ShowMessage(buf2);
	SYS_ReleaseCharBuf(buf2);
	delete [] buf;
	delete str;
}

// https://csdb.dk/forums/?roomid=7&topicid=125812
// SpritePad format

//		v1:
//		1 byte    - 0-3 background
//		1 byte    - 0-3 multicolour 1
//		1 byte    - 0-3 multicolour 2
//
//		repeated for each sprite:
//		63 bytes  - sprite data
//		1 byte    - flags 0-3 colour, 4 overlay, 7 multi
//
//		v2:
//		3 bytes   - magic "SPD"
//		1 byte    - version (1)
//		1 byte    - number of sprites - 1
//		1 byte    - number of animations - 1
//		1 byte    - 0-3 background
//		1 byte    - 0-3 multicolour 1
//		1 byte    - 0-3 multicolour 2
//
//		repeated for each sprite:
//		63 bytes  - sprite data
//		1 byte    - flags 0-3 colour, 4 overlay, 7 multi
//
//		animation settings split into 4 arrays:
//		n bytes   - animation starts
//		n bytes   - animation ends
//		n bytes   - timers
//		n bytes   - flags 4 ping-pong, 5 overlay, 7 valid

int CViewC64Sprite::ImportSprite(CSlrString *path)
{
	guiMain->LockMutex();
	
	CSlrFile *file = new CSlrFileFromOS(path);
	if (file->Exists())
	{
		C64Sprite *sprite = vicEditor->layerVirtualSprites->FindSpriteByRasterPos(spriteRasterX, spriteRasterY);

		if (sprite == NULL)
		{
			guiMain->ShowMessage("Sprite not selected");
			delete file;
			guiMain->UnlockMutex();
			return -1;
		}
		
		int addr = sprite->pointerAddr;
		
		CByteBuffer *byteBuffer = new CByteBuffer(file, false);
		
		for (int i = 0; i < 64; i++)
		{
			if (byteBuffer->IsEof())
			{
				guiMain->ShowMessage("Sprite file corrupted");
				delete byteBuffer;
				delete file;
				guiMain->UnlockMutex();
				return -1;
			}
			u8 v = byteBuffer->GetU8();
			debugInterfaceVice->SetByteToRamC64(addr++, v);
		}
		
		int paintResult;
		
		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		u8 colorD021 = byteBuffer->GetU8();
		// skip D021
		
		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		u8 colorD025 = byteBuffer->GetU8();
		
		paintResult = vicEditor->layerVirtualSprites->ReplaceColor(spriteRasterX, spriteRasterY, sprite->spriteId, 1, colorD025);

		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		u8 colorD026 = byteBuffer->GetU8();

		paintResult = vicEditor->layerVirtualSprites->ReplaceColor(spriteRasterX, spriteRasterY, sprite->spriteId, 3, colorD026);

		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		u8 colorSprite = byteBuffer->GetU8();

		paintResult = vicEditor->layerVirtualSprites->ReplaceColor(spriteRasterX, spriteRasterY, sprite->spriteId, 2, colorSprite);

		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		bool isMultiColor = byteBuffer->GetBool();
		vicEditor->layerVirtualSprites->ReplaceMultiColor(spriteRasterX, spriteRasterY, sprite->spriteId, isMultiColor);

		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		bool isStretchX = byteBuffer->GetBool();
		vicEditor->layerVirtualSprites->ReplaceStretchX(spriteRasterX, spriteRasterY, sprite->spriteId, isStretchX);

		if (byteBuffer->IsEof()) { delete byteBuffer; delete file; return sprite->pointerAddr; }
		bool isStretchY = byteBuffer->GetBool();
		vicEditor->layerVirtualSprites->ReplaceStretchY(spriteRasterX, spriteRasterY, sprite->spriteId, isStretchY);
		
		delete byteBuffer;
		delete file;
		guiMain->UnlockMutex();
		return sprite->pointerAddr;
	}

	delete file;
	guiMain->UnlockMutex();
	return -1;
}


void CViewC64Sprite::SystemDialogFileOpenCancelled()
{
}

void CViewC64Sprite::SystemDialogFileSaveSelected(CSlrString *path)
{
	this->ExportSprite(path);
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	guiMain->ShowMessage(str);
	delete str;
}

void CViewC64Sprite::SystemDialogFileSaveCancelled()
{
}

void CViewC64Sprite::ExportSprite(CSlrString *path)
{
//	ConvertColorSpriteDataToImage(currentSpriteData, imageDataSprite, paintColorD021, paintColorD025, paintColorD026, paintColorSprite,
//	//								  debugInterface, 4);
//	
	char *cPath = path->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);

	u8 valTrue = TRUE;
	u8 valFalse = FALSE;
	
	FILE *fp = fopen(cPath, "wb");
	if (fp)
	{
		fwrite(currentSpriteData, 64, 1, fp);
		
		fwrite(&paintColorD021, 1, 1, fp);
		fwrite(&paintColorD025, 1, 1, fp);
		fwrite(&paintColorD026, 1, 1, fp);
		fwrite(&paintColorSprite, 1, 1, fp);

		if (btnIsMultiColor->IsOn())
		{
			fwrite(&valTrue, 1, 1, fp);
		}
		else
		{
			fwrite(&valFalse, 1, 1, fp);
		}
		
		if (btnIsStretchX->IsOn())
		{
			fwrite(&valTrue, 1, 1, fp);
		}
		else
		{
			fwrite(&valFalse, 1, 1, fp);
		}

		if (btnIsStretchY->IsOn())
		{
			fwrite(&valTrue, 1, 1, fp);
		}
		else
		{
			fwrite(&valFalse, 1, 1, fp);
		}

		fclose(fp);
	}
	delete [] cPath;

}


