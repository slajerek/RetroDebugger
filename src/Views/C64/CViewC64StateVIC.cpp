extern "C" {
#include "viciitypes.h"
}

#include "CViewC64StateVIC.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "CViewDataDump.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CViewDataDump.h"
#include "CViewC64VicDisplay.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CDebugInterfaceVice.h"
#include "C64Tools.h"
#include "C64SettingsStorage.h"
#include "CLayoutParameter.h"
#include "VID_Main.h"

CViewC64StateVIC::CViewC64StateVIC(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	isLockedState = false;
	previousIsLockedStateFrameNum = 0;

	fontSize = 7.0f;
	hasManualFontSize = false;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;
	
	isVertical = false;
	AddLayoutParameter(new CLayoutParameterBool("Vertical", &isVertical));

	showRegistersOnly = false;
	
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;
	numValuesPerColumn = 0x0C;
	
	showSprites = true;

	spritesImageData = new std::vector<CImageData *>();
	spritesImages = new std::vector<CSlrImage *>();
	
	// init images for sprites
	for (int i = 0; i < 0x0F; i++)
	{
		// alloc image that will store sprite pixels
		CImageData *imageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		imageData->AllocImage(false, true);
		
		spritesImageData->push_back(imageData);
		
		/// init CSlrImage with empty image (will be deleted by loader)
		CImageData *emptyImageData = new CImageData(32, 32, IMG_TYPE_RGBA);
		emptyImageData->AllocImage(false, true);
		
		CSlrImage *imageSprite = new CSlrImage(true, false);
		imageSprite->LoadImage(emptyImageData, RESOURCE_PRIORITY_STATIC, false);
		imageSprite->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
		imageSprite->resourcePriority = RESOURCE_PRIORITY_STATIC;
		VID_PostImageBinding(imageSprite, NULL);
		
		spritesImages->push_back(imageSprite);
	}
	
	// do not force colors
	for (int i = 0; i < 0x0F; i++)
	{
		forceColors[i] = -1;
	}
	forceColorD800 = -1;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateVIC::SetPosition(float posX, float posY)
{
	CGuiView::SetPosition(posX, posY, posZ, fontSize*62, fontSize*29);
}

void CViewC64StateVIC::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	bool sizeChanged = (fabs(sizeX - this->sizeX) > 0.5f || fabs(sizeY - this->sizeY) > 0.5f);

	if (hasManualFontSize && !sizeChanged)
	{
		// Size unchanged (startup/layout restore): keep manually set font size
	}
	else
	{
		// Auto-scale: 59 chars wide, 32 rows tall
		fontSize = fmin(sizeX / 57.0f, sizeY / 32.0f);

		if (sizeChanged)
			hasManualFontSize = false;
	}

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateVIC::SetPosition(float posX, float posY, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateVIC::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	if (layoutParameter != NULL)
	{
		hasManualFontSize = true;
	}
	else
	{
		float autoFontSize = fmin(sizeX / 57.0f, sizeY / 32.0f);

		if (fabs(fontSize - autoFontSize) > 0.01f)
		{
			hasManualFontSize = true;
		}
	}

	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64StateVIC::DoLogic()
{
}

void CViewC64StateVIC::UpdateSpritesImages()
{
	this->UpdateVICSpritesImages(&(viewC64->viciiStateToShow),
								 spritesImageData, spritesImages,
								 viewC64->viewC64MemoryDataDump->renderDataWithColors);
}

void CViewC64StateVIC::RenderColorRectangle(float px, float py, float ledSizeX, float ledSizeY, float gap, bool isLocked, u8 color)
{
	RenderColorRectangleWithHexCode(px, py, ledSizeX, ledSizeY, gap, isLocked, color, fontSize, viewC64->debugInterfaceC64);
}

void CViewC64StateVIC::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64StateVIC::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;
	
	//BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0.2f, 1.0f, 1.0f, 1.0f);
	
	
	if (isLockedState)
	{
		BlitFilledRectangle(posX, posY, posZ, fontSize*56.6, fontSize*26, 0.2f, 0.0f, 0.0f, 1.0f);
	}

	this->RenderStateVIC(&(viewC64->viciiStateToShow),
							posX, posY, posZ, isVertical, showSprites, fontBytes, fontSize,
							showRegistersOnly,
							spritesImageData, spritesImages,
							viewC64->viewC64MemoryDataDump->renderDataWithColors);
	
	// render colors
	if (isVertical == false)
	{
		// ghostbyte info on the right side, aligned with raster line
		{
			vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
			float gbX = posX + fontSize * 37.0f;
			float gbY = posY;

			char gbBuf[64];
			uint8 val = viciiState->ghostbyteValue;
			sprintf(gbBuf, "%04x: %02x %c%c%c%c%c%c%c%c", viciiState->ghostbyteAddr, val,
				(val & 0x80) ? '1' : '0', (val & 0x40) ? '1' : '0',
				(val & 0x20) ? '1' : '0', (val & 0x10) ? '1' : '0',
				(val & 0x08) ? '1' : '0', (val & 0x04) ? '1' : '0',
				(val & 0x02) ? '1' : '0', (val & 0x01) ? '1' : '0');
			viewC64->fontDisassembly->BlitText(gbBuf, gbX, gbY, posZ, fontSize);
		}

		float ledX = posX + fontSize * 37.0f;
		float ledY = posY + fontSize * 4.5;
		
		char buf[8] = { 'D', '0', '2', '0', 0x00 };
		float ledSizeX = fontSize*4.0f;
		float gap = fontSize * 0.1f;
		float step = fontSize * 0.75f;
		float ledSizeY = fontSize + gap + gap;
		
		float px = ledX;
		float py = ledY;
		float py2 = py + fontSize + gap;
		
		// D020-D023
		for (int i = 0x00; i < 0x04; i++)
		{
			buf[3] = 0x30 + i;
			viewC64->fontDisassembly->BlitText(buf, px, py, posZ, fontSize);
			
			u8 color = viewC64->colorsToShow[i];
			bool isForced = (this->forceColors[i] != -1);
			RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isForced, color);
			
			px += ledSizeX + step;
		}

		px = ledX;
		py += ledSizeY + fontSize + step;
		py2 = py + fontSize + gap;
		
		// D024-D027
		for (int i = 0x04; i < 0x07; i++)
		{
			buf[3] = 0x30 + i;
			viewC64->fontDisassembly->BlitText(buf, px, py, posZ, fontSize);
			
			u8 color = viewC64->colorsToShow[i];
			bool isForced = (this->forceColors[i] != -1);
			RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isForced, color);
			
			px += ledSizeX + step;
		}
		
		// D800
		viewC64->fontDisassembly->BlitText("RAM", px, py, posZ, fontSize);

		u8 color = viewC64->colorToShowD800;
		bool isForced = (this->forceColorD800 != -1);
		RenderColorRectangle(px, py2, ledSizeX, ledSizeY, gap, isForced, color);

		
		// sprite colors
		px = posX + fontSize * 10.5f;
		py = posY + fontSize * 12.75f;
		step = fontSize * 6;
		
		// D027-D02E
		for (int i = 0x07; i < 0x0F; i++)
		{
			u8 color = viewC64->colorsToShow[i];
			bool isForced = (this->forceColors[i] != -1);
			RenderColorRectangle(px, py, ledSizeX, ledSizeY, gap, isForced, color);
			
			px += step;
		}

	}

}

bool CViewC64StateVIC::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}
	
	// check if tap register
	if (showRegistersOnly)
	{
		float fs2 = fontSize;
		float sx = fs2 * 9;
		
		float plx = posX;	//+ fontSize * 5.0f
		float plex = posX + fontSize * 7.0f;
		float ply = posY;
		for (int i = 0; i < 0x2F; i++)
		{
			if (x >= plx && x <= plex
				&& y >= ply && y <= ply+fontSize)
			{
				LOGD("CViewC64StateVIC::DoTap: tapped register D0%02x", i);
				
				editHex->SetValue(viewC64->viciiStateToShow.regs[i], 2);
				editingRegisterValueIndex = i;

				guiMain->UnlockMutex();
				return true;
			}
			
			ply += fs2;
			
			if (i % numValuesPerColumn == numValuesPerColumn-1)
			{
				ply = posY;
				plx += sx;
				plex += sx;
			}
		}
	}
	
	
	// lock / unlock
	if (isVertical == false)
	{
		float ledX = posX + fontSize * 37.0f;
		float ledY = posY + fontSize * 4.5;
		float ledSizeX = fontSize*4.0f;
		float gap = fontSize * 0.1f;
		float step = fontSize * 0.75f;
		float ledSizeY = fontSize + gap + gap;
		float ledSizeY2 = ledSizeY + fontSize + step;
		
		float px = ledX;
		float py = ledY;
		float py2 = py + fontSize + gap;
		
		// D020-D023
		for (int i = 0x00; i < 0x04; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				LOGD("clicked %02x", i);
				
				if (this->forceColors[i] == -1)
				{
					this->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					this->forceColors[i] = -1;
				}
				
				guiMain->UnlockMutex();
				return true;
			}
			
			px += ledSizeX + step;
		}
		
		px = ledX;
		py += ledSizeY + fontSize + step;
		py2 = py + fontSize + gap;
		
		// D024-D027
		for (int i = 0x04; i < 0x07; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				LOGD("clicked %02x", i);
				
				if (this->forceColors[i] == -1)
				{
					this->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					this->forceColors[i] = -1;
				}
				
				guiMain->UnlockMutex();
				return true;
			}

			px += ledSizeX + step;
		}
		
		// D800
		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			LOGD("clicked d800");
			
			if (this->forceColorD800 == -1)
			{
				this->forceColorD800 = viewC64->colorToShowD800;
			}
			else
			{
				this->forceColorD800 = -1;
			}

			guiMain->UnlockMutex();
			return true;
		}
		
		
		// sprite colors
		px = posX + fontSize * 10.5f;
		py = posY + fontSize * 12.75f;
		step = fontSize * 6;
		
		// D027-D02E
		for (int i = 0x07; i < 0x0F; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				LOGD("clicked %02x", i);

				if (this->forceColors[i] == -1)
				{
					this->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					this->forceColors[i] = -1;
				}
				
				guiMain->UnlockMutex();
				return true;
			}
			
			px += step;
		}
		
	}
	
	// replace mode of display
	showRegistersOnly = !showRegistersOnly;
	editingRegisterValueIndex = -1;
	
	guiMain->UnlockMutex();
	return false;
}

bool CViewC64StateVIC::DoRightClick(float x, float y)
{
	// lock / unlock
	if (isVertical == false)
	{
		float ledX = posX + fontSize * 37.0f;
		float ledY = posY + fontSize * 4.5;
		float ledSizeX = fontSize*4.0f;
		float gap = fontSize * 0.1f;
		float step = fontSize * 0.75f;
		float ledSizeY = fontSize + gap + gap;
		float ledSizeY2 = ledSizeY + fontSize + step;
		
		float px = ledX;
		float py = ledY;
		float py2 = py + fontSize + gap;
		
		// D020-D023
		for (int i = 0x00; i < 0x04; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				LOGD("clicked %02x", i);
				
				if (this->forceColors[i] == -1)
				{
					this->forceColors[i] = viewC64->colorsToShow[i];
				}
				
				this->forceColors[i] = (this->forceColors[i] + 1) & 0x0F;
				
				return true;
			}
			
			px += ledSizeX + step;
		}
		
		px = ledX;
		py += ledSizeY + fontSize + step;
		py2 = py + fontSize + gap;
		
		// D024-D027
		for (int i = 0x04; i < 0x07; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				LOGD("clicked %02x", i);
				
				if (this->forceColors[i] == -1)
				{
					this->forceColors[i] = viewC64->colorsToShow[i];
				}
				
				this->forceColors[i] = (this->forceColors[i] + 1) & 0x0F;
				
				return true;
			}
			
			px += ledSizeX + step;
		}
		
		// D800
		if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
		{
			LOGD("clicked d800");
			
			if (this->forceColorD800 == -1)
			{
				this->forceColorD800 = viewC64->colorToShowD800;
			}
			
			this->forceColorD800 = (this->forceColorD800 + 1) & 0x0F;
			
			return true;
		}
		
		
		// sprite colors
		px = posX + fontSize * 10.5f;
		py = posY + fontSize * 12.75f;
		step = fontSize * 6;
		
		// D027-D02E
		for (int i = 0x07; i < 0x0F; i++)
		{
			if (x >= px && x <= px + ledSizeX && y >= py && y <= py + ledSizeY2)
			{
				LOGD("clicked %02x", i);
				
				if (this->forceColors[i] == -1)
				{
					this->forceColors[i] = viewC64->colorsToShow[i];
				}
				else
				{
					this->forceColors[i] = (this->forceColors[i] + 1) & 0x0F;
				}
				
				return true;
			}
			
			px += step;
		}
		
	}
	
	return false;
}

// TODO: change all %02x %04x into sprintfHexCode8WithoutZeroEnding(bufPtr, ...);
/// render states
extern "C" {
	const char *fetch_phi1_type(int addr);
}

void CViewC64StateVIC::RenderStateVIC(vicii_cycle_state_t *viciiState,
										   float posX, float posY, float posZ, bool isVertical, bool showSprites, CSlrFont *fontBytes, float fontSize,
										   bool showRegistersOnly,
										   std::vector<CImageData *> *spritesImageData,
										   std::vector<CSlrImage *> *spritesImages, bool renderDataWithColors)
{
	char buf[256];
	char buf2[256];
	float px = posX;
	float py = posY;
	int i, bits, bits2;
	int video_mode, m_mcm, m_bmm, m_ecm, v_bank, v_vram;
	
	video_mode = ((viciiState->regs[0x11] & 0x60) | (viciiState->regs[0x16] & 0x10)) >> 4;
	
	m_ecm = (video_mode & 4) >> 2;  /* 0 standard, 1 extended */
	m_bmm = (video_mode & 2) >> 1;  /* 0 text, 1 bitmap */
	m_mcm = video_mode & 1;         /* 0 hires, 1 multi */
	
	v_bank = viciiState->vbank_phi1;
	v_vram = ((viciiState->regs[0x18] >> 4) * 0x0400) + viciiState->vbank_phi2;
	
	if (showRegistersOnly)
	{
		float fs2 = fontSize; // * 0.75f;
		
		float plx = px;
		float ply = py;
		for (int i = 0; i < 0x2F; i++)
		{
			if (editingRegisterValueIndex == i)
			{
				sprintf(buf, "D0%02x", i);
				fontBytes->BlitText(buf, plx, ply, posZ, fs2);
				fontBytes->BlitTextColor(editHex->textWithCursor, plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				sprintf(buf, "D0%02x %02x", i, viciiState->regs[i]);
				fontBytes->BlitText(buf, plx, ply, posZ, fs2);
			}
			
			ply += fs2;
			
			if (i % numValuesPerColumn == numValuesPerColumn-1)
			{
				ply = py;
				plx += fs2 * 9;
			}
		}
		
		if (showSprites == true)
		{
			if (isVertical == false)
			{
				py += fontSize;
			}
			
			py += fontSize * 5.5f;
		}
		else
		{
			py = posY;
			px = posX + fontSize * 29.0f;
		}
		
		if (isVertical == false)
		{
			py += fontSize * 6.0f;
		}
		else
		{
			py += fontSize * 7.0f;
		}
		
		py += fontSize * 0.5f;
		
		
		//
	}
	else
	{
		// show descriptions
		static const char *mode_name[] =
		{
			"Standard Text",
			"Multicolor Text",
			"Hires Bitmap",
			"Multicolor Bitmap",
			"Extended Text",
			"Illegal Text",
			"Invalid Bitmap 1",
			"Invalid Bitmap 2"
		};
		
		//	sprintf(buf, "Raster cycle/line: %d/%d IRQ: %d", viciiState->raster_cycle, viciiState->raster_line, viciiState->raster_irq_line);
		//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		if (isVertical == false)
		{
			sprintf(buf, "Raster line       : %04x", viciiState->raster_line);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}
		
		sprintf(buf, "IRQ raster line   : %04x", viciiState->raster_irq_line);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		//	if (isVertical == false)
		//	{
		//		uint8 irqFlags = viciiState->irq_status;// | 0x70;
		//		sprintf(buf, "Interrupt status  : "); PrintVicInterrupts(irqFlags, buf);
		//		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		//	}
		uint8 irqMask = viciiState->regs[0x1a];
		sprintf(buf, "Enabled interrupts: "); PrintVicInterrupts(irqMask, buf);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		//	sprintf(buf, "Scroll X/Y: %d/%d, RC %d, Idle: %d, %dx%d", viciiState->regs[0x16] & 0x07, viciiState->regs[0x11] & 0x07,
		//			viciiState->rc, viciiState->idle_state,
		//			39 + ((viciiState->regs[0x16] >> 3) & 1), 24 + ((viciiState->regs[0x11] >> 3) & 1));
		//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		
		sprintf(buf, "X scroll          : %d", viciiState->regs[0x16] & 0x07);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, "Y scroll          : %d", viciiState->regs[0x11] & 0x07);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "Border            : %dx%d", 38 + (((viciiState->regs[0x16] >> 3) & 1) << 1), 24 + ((viciiState->regs[0x11] >> 3) & 1));
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		if (showSprites == true)
		{
			py += fontSize * 0.5f;
		}
		else
		{
			py = posY;
			px = posX + fontSize * 29.0f;
		}
		
		
		sprintf(buf, "Display mode      : %s", mode_name[video_mode]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		//	sprintf(buf, "Mode: %s (ECM/BMM/MCM=%d/%d/%d)", mode_name[video_mode], m_ecm, m_bmm, m_mcm);
		//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "Sequencer state   : %s", viciiState->idle_state ? "Idle" : "Display");
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "Row counter       : %d", viciiState->rc);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		const int FIRST_DMA_LINE = 0x30;
		const int LAST_DMA_LINE = 0xf7;
		uint8 yScroll = viciiState->regs[0x11] & 0x07;
		bool isBadLine = viciiState->raster_line >= FIRST_DMA_LINE && viciiState->raster_line <= LAST_DMA_LINE && ((viciiState->raster_line & 7) == yScroll);
		
		
		sprintf(buf, "Bad line state    : %s", isBadLine ? "Yes" : "No");
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "VC %03x VCBASE %03x VMLI %2d Phi1 %02x", viciiState->vc, viciiState->vcbase, viciiState->vmli, viciiState->last_read_phi1);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		//	sprintf(buf, "Colors: Border: %x BG: %x ", viciiState->regs[0x20], viciiState->regs[0x21]);
		//	if (m_ecm)
		//	{
		//		sprintf(buf2, "BG1: %x BG2: %x BG3: %x\n", viciiState->regs[0x22], viciiState->regs[0x23], viciiState->regs[0x24]);
		//		strcat(buf, buf2);
		//	}
		//	else if (m_mcm && !m_bmm)
		//	{
		//		sprintf(buf2, "MC1: %x MC2: %x\n", viciiState->regs[0x22], viciiState->regs[0x23]);
		//		strcat(buf, buf2);
		//	}
		//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		
		if (isVertical == false)
		{
			sprintf(buf, "Video base        : %04x, ", v_vram);
			if (m_bmm)
			{
				i = ((viciiState->regs[0x18] >> 3) & 1) * 0x2000 + v_bank;
				sprintf(buf2, "Bitmap  %04x (%s)", i, fetch_phi1_type(i));
				strcat(buf, buf2);
			}
			else
			{
				i = (((viciiState->regs[0x18] >> 1) & 0x7) * 0x0800) + v_bank;
				sprintf(buf2, "Charset %04x (%s)", i, fetch_phi1_type(i));
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}
		else
		{
			sprintf(buf, "Video base        : %04x", v_vram);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			if (m_bmm)
			{
				i = ((viciiState->regs[0x18] >> 3) & 1) * 0x2000 + v_bank;
				sprintf(buf, "            Bitmap: %04x (%s)", i, fetch_phi1_type(i));
			}
			else
			{
				i = (((viciiState->regs[0x18] >> 1) & 0x7) * 0x800) + v_bank;
				sprintf(buf, "           Charset: %04x (%s)", i, fetch_phi1_type(i));
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		}
		
		py += fontSize * 0.5f;
		
	}
	
	if (showSprites)
	{
		/// sprites
		int numPasses = 1;
		int step = 8;
		
		if (isVertical)
		{
			numPasses = 2;
			step = 4;
		}
		
	 // get VIC sprite colors
		uint8 cD021 = viciiState->regs[0x21];
		uint8 cD025 = viciiState->regs[0x25];
		uint8 cD026 = viciiState->regs[0x26];
		
		float fss = fontSize * 0.25f;
		
		//bool isEnabled[8] = { false };
		for (int passNum = 0; passNum < numPasses; passNum++)
		{
			int startId = passNum * step;
			int endId = (passNum+1) * step;
			
			sprintf(buf, "         ");
			
			if (isVertical)
			{
				for (int z = startId; z < endId; z++)
				{
					sprintf(buf2, "#%d    ", z);
					strcat(buf, buf2);
				}
				fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			}
			else
			{
				for (int z = startId; z < endId; z++)
				{
					sprintf(buf2, "%d     ", z);
					strcat(buf, buf2);
				}
				fontBytes->BlitText(buf, px, py-fss, posZ, fontSize); py += fontSize;
			}
			
			sprintf(buf, "Enabled: ");
			bits = viciiState->regs[0x15];
			for (i = startId; i < endId; i++)
			{
				/*
				 if (((bits >> i) & 1))
				 {
					isEnabled[i] = true;
				 }
				 else
				 {
					isEnabled[i] = false;
				 }
				 */
				
				sprintf(buf2, "%s", ((bits >> i) & 1) ? "Yes   " : "No    ");
				strcat(buf, buf2);
			}
			
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			/////////////"         "
			sprintf(buf, "DMA/dis: ");
			bits = viciiState->sprite_dma;
			bits2 = viciiState->sprite_display_bits;
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%c/%c   ", ((bits >> i) & 1) ? 'D' : ' ', ((bits2 >> i) & 1) ? 'd' : ' ');
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			/////////////"         "
			sprintf(buf, "Pointer: ");
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%02x    ", viciiState->sprite[i].pointer);
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			
			/////////////"         "
			sprintf(buf, "MC:      ");
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%02x    ", viciiState->sprite[i].mc);
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			/////////////"         "
			if (isVertical == false)
			{
				sprintf(buf, "MCBASE:  ");
				for (i = startId; i < endId; i++)
				{
					sprintf(buf2, "%02x    ", viciiState->sprite[i].mcbase);
					strcat(buf, buf2);
				}
				fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			}
			
			/////////////"         "
			sprintf(buf, "X-Pos:   ");
			for (i = startId; i < endId; i++)
			{
				if (c64SettingsShowPositionsInHex)
				{
					sprintf(buf2, "%-4x  ", viciiState->sprite[i].x);
				}
				else
				{
					sprintf(buf2, "%-4d  ", viciiState->sprite[i].x);
				}
				
				/*
				 int x = viciiState->regs[0 + (i << 1)];
				 
				 bits = viciiState->regs[0x10];
				 int e = ((bits >> i) & 1);
				 
				 if (e != 0)
				 {
					x += 256;
				 }
				 LOGD(" .. #%d s.x=%d x=%d", i, viciiState->sprite[i].x, x);
				 */
				
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			
			/////////////"         "
			sprintf(buf, "Y-Pos:   ");
			for (i = startId; i < endId; i++)
			{
				if (c64SettingsShowPositionsInHex)
				{
					sprintf(buf2, "%-4x  ", viciiState->regs[1 + (i << 1)]);
				}
				else
				{
					sprintf(buf2, "%-4d  ", viciiState->regs[1 + (i << 1)]);
				}
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			/////////////"         "
			sprintf(buf, "X-Exp:   ");
			bits = viciiState->regs[0x1d];
			
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%s", ((bits >> i) & 1) ? "Yes   " : "No    ");
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			/////////////"         "
			sprintf(buf, "Y-Exp:   ");
			bits = viciiState->regs[0x17];
			
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%s", ((bits >> i) & 1) ? (viciiState->sprite[i].exp_flop ? "YES*  " : "Yes   ") : "No    ");
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			sprintf(buf, "Mode   : ");
			bits = viciiState->regs[0x1c];
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%s", ((bits >> i) & 1) ? "Multi " : "Std.  ");
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			sprintf(buf, "Prio.  : ");
			bits = viciiState->regs[0x1b];
			for (i = startId; i < endId; i++)
			{
				sprintf(buf2, "%s", ((bits >> i) & 1) ? "Back  " : "Fore  ");
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			sprintf(buf, "Data   : ");
			for (i = startId; i < endId; i++)
			{
				int addr = v_bank + viciiState->sprite[i].pointer * 64;
				sprintf(buf2, "%04x  ", addr);
				strcat(buf, buf2);
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			py += fontSize * 0.25f;
			
			
			//
			// draw sprites
			//
			px += 9*fontSize;
			
			const float spriteSizeX = 6*fontSize;
			const float spriteSizeY = (21.0f * spriteSizeX) / 24.0f;
			
			const float spriteTexStartX = 4.0/32.0;
			const float spriteTexStartY = 4.0/32.0;
			const float spriteTexEndX = (4.0+24.0)/32.0;
			const float spriteTexEndY = (4.0+21.0)/32.0;
			
			//
			// UpdateSpritesImages is run from CViewC64::Render and sprites images are refreshed on the beginning of each UI render frame
			for (int zi = startId; zi < endId; zi++)
			{
				CSlrImage *image = (*spritesImages)[zi];
				
				Blit(image, px, py, posZ, spriteSizeX, spriteSizeY, spriteTexStartX, spriteTexStartY, spriteTexEndX, spriteTexEndY);
				px += spriteSizeX;
			}
			
			px = posX;
			py += 5.5f*fontSize;
		}
	}	
}

void CViewC64StateVIC::PrintVicInterrupts(uint8 flags, char *buf)
{
	if (flags & 0x1F)
	{
		if (flags & 0x01) strcat(buf, "Raster ");
		if (flags & 0x02) strcat(buf, "Spr-Data ");
		if (flags & 0x04) strcat(buf, "Spr-Spr ");
		if (flags & 0x08) strcat(buf, "Lightpen");
	}
	else
	{
		strcat(buf, "None");
	}
}

void CViewC64StateVIC::UpdateVICSpritesImages(vicii_cycle_state_t *viciiState,
												   std::vector<CImageData *> *spritesImageData,
												   std::vector<CSlrImage *> *spritesImages, bool renderDataWithColors)
{
	int v_bank = viciiState->vbank_phi1;
	uint8 cD021 = viciiState->regs[0x21];
	uint8 cD025 = viciiState->regs[0x25];
	uint8 cD026 = viciiState->regs[0x26];
	
	for (int zi = 0; zi < 8; zi++)
	{
		CSlrImage *image = (*spritesImages)[zi];
		CImageData *imageData = (*spritesImageData)[zi];
		
		int addr = v_bank + viciiState->sprite[zi].pointer * 64;
		
		//LOGD("sprite#=%d dataAddr=%04x", zi, addr);
		uint8 spriteData[63];
		
		for (int i = 0; i < 63; i++)
		{
			uint8 v;
			debugInterface->dataAdapterC64DirectRam->AdapterReadByte(addr, &v);
			spriteData[i] = v;
			addr++;
		}
		
		bool isColor = false;
		if (viciiState->regs[0x1c] & (1<<zi))
		{
			isColor = true;
		}
		if (isColor == false)
		{
			if (renderDataWithColors)
			{
				uint8 spriteColor = viciiState->regs[0x27+zi];
				ConvertSpriteDataToImage(spriteData, imageData, cD021, spriteColor, this->debugInterface, 4);
			}
			else
			{
				ConvertSpriteDataToImage(spriteData, imageData, 4);
			}
		}
		else
		{
			uint8 spriteColor = viciiState->regs[0x27+zi];
			ConvertColorSpriteDataToImage(spriteData, imageData,
										  cD021, cD025, cD026, spriteColor,
										  this->debugInterface, 4, 0);
		}
		
		// re-bind image
		image->ReBindImageData(imageData);
	}
}

//
void CViewC64StateVIC::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	if (editingRegisterValueIndex != -1)
	{
		u8 v = editHex->value;
		debugInterface->SetVicRegister(editingRegisterValueIndex, v);
		
		editHex->SetCursorPos(0);
	}
}


bool CViewC64StateVIC::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editingRegisterValueIndex != -1)
	{
		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = debugInterface->GetVicRegister(&(viewC64->viciiStateToShow), editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < 0x2F)
			{
				editingRegisterValueIndex++;
				u8 v = debugInterface->GetVicRegister(&(viewC64->viciiStateToShow), editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > numValuesPerColumn)
			{
				editingRegisterValueIndex -= numValuesPerColumn;
				u8 v = debugInterface->GetVicRegister(&(viewC64->viciiStateToShow), editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < 0x2F-numValuesPerColumn)
			{
				editingRegisterValueIndex += numValuesPerColumn;
				u8 v = debugInterface->GetVicRegister(&(viewC64->viciiStateToShow), editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		editHex->KeyDown(keyCode);
		return true;
	}
	return false;
}

bool CViewC64StateVIC::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editingRegisterValueIndex != -1)
	{
		return true;
	}
	return false;
}

void CViewC64StateVIC::RenderFocusBorder()
{
	//
//	CGuiView::RenderFocusBorder();
	return;
}

u64 previousIsLockedStateFrameNum;

void CViewC64StateVIC::SetIsLockedState(bool newIsLockedState)
{
	u64 currentFrameNum = VID_GetCurrentFrameNumber();
	
//	LOGD("PRE  SetIsLockedState isLockedState=%s previousIsLockedStateFrameNum=%d currentFrameNum=%d newIsLockedState=%s", STRBOOL(isLockedState), previousIsLockedStateFrameNum, currentFrameNum, STRBOOL(newIsLockedState));
	if (newIsLockedState == false)
	{
		if (previousIsLockedStateFrameNum < currentFrameNum)
		{
			previousIsLockedStateFrameNum = currentFrameNum;
			isLockedState = newIsLockedState;
		}
	}
	else
	{
		previousIsLockedStateFrameNum = currentFrameNum;
		isLockedState = newIsLockedState;
	}
//	LOGD("POST SetIsLockedState isLockedState=%s previousIsLockedStateFrameNum=%d currentFrameNum=%d newIsLockedState=%s", STRBOOL(isLockedState), previousIsLockedStateFrameNum, currentFrameNum, STRBOOL(newIsLockedState));
}

bool CViewC64StateVIC::GetIsLockedState()
{
//	LOGD("GetIsLockedState: %s", STRBOOL(isLockedState));
	return isLockedState;
}

// Layout
void CViewC64StateVIC::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64StateVIC::Deserialize(CByteBuffer *byteBuffer)
{
}

