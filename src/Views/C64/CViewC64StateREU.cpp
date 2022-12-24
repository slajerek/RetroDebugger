extern "C" {
#include "reu.h"
#include "c64.h"
}
#include "CViewC64StateREU.h"
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
#include "CLayoutParameter.h"

CViewC64StateREU::CViewC64StateREU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontBytes = viewC64->fontDisassembly;
	
	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateREU::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateREU::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64StateREU::DoLogic()
{
}

void CViewC64StateREU::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64StateREU::Render()
{
	this->RenderStateREU(posX, posY, posZ, fontBytes, fontSize);
}

/// render states
extern "C" {
	int reu_cart_enabled(void);

	BYTE reu_read_without_sideeffects(WORD addr);
	void c64d_reu_io2_store(WORD addr, BYTE value);
}


void CViewC64StateREU::RenderStateREU(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize)
{
	char buf[256];
	
//	LOGD("reu_cart_enabled()=%d", reu_cart_enabled());
	if (reu_cart_enabled() == 0)
	{
		fontBytes->BlitText("REU not enabled", px, py, posZ, fontSize);
		return;
	}
	
	if (showRegistersOnly)
	{
		float fs2 = fontSize;
		
		float plx = px;
		float ply = py;
		
		for (int i = 0; i < 0x0B; i++)
		{
			if (editingRegisterValueIndex == i)
			{
				sprintf(buf, "df%02x", i);
				fontBytes->BlitText(buf, plx, ply, posZ, fs2);
				fontBytes->BlitTextColor(editHex->textWithCursor, plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				u8 v = reu_read_without_sideeffects(i);
				sprintf(buf, "df%02x %02x", i, v);
				fontBytes->BlitText(buf, plx, ply, posZ, fs2);
			}
			
			ply += fs2;
			
			if (i % 0x08 == 0x07)
			{
				ply = py;
				plx += fs2 * 9;
			}
		}
		
		return;
	}
	
	sprintf(buf, "REU STATUS: %02x", reu_read_without_sideeffects(0x00));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, "   COMMAND: %02x", reu_read_without_sideeffects(0x01));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, " BASE ADDR:   %04x", reu_read_without_sideeffects(0x03) << 8 | reu_read_without_sideeffects(0x02));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	sprintf(buf, "  RAM ADDR: %02x%04x", reu_read_without_sideeffects(0x06), reu_read_without_sideeffects(0x05) << 8 | reu_read_without_sideeffects(0x04));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

//	sprintf(buf, "      BANK: %02x", reu_read_without_sideeffects(0x06));
//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, " BLOCK LEN: %04x", reu_read_without_sideeffects(0x08) << 8 | reu_read_without_sideeffects(0x07));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, " INTERRUPT: %02x", reu_read_without_sideeffects(0x09));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, " ADDR CTRL: %02x", reu_read_without_sideeffects(0x0A));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	py += fontSize;
	
}


bool CViewC64StateREU::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}

	// check if tap register
	if (showRegistersOnly)
	{
		float px = posX;
		float fs2 = fontSize;
		float sx = fs2 * 9;
		
		float plx = posX;	//+ fontSize * 5.0f
		float plex = posX + fontSize * 7.0f;
		float ply = posY + fontSize;
		for (int i = 0; i < 0x10; i++)
		{
			if (x >= plx && x <= plex
				&& y >= ply && y <= ply+fontSize)
			{
				LOGD("CViewC64StateREU::DoTap: tapped register %02x", i);
				
				editingRegisterValueIndex = i;

				u8 v = reu_read_without_sideeffects(editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				
				guiMain->UnlockMutex();
				return true;
			}
			
			ply += fs2;
			
			if (i % 0x08 == 0x07)
			{
				ply = posY + fontSize;
				plx += sx;
				plex += sx;
			}
		}
	}
	
	showRegistersOnly = !showRegistersOnly;
	
	guiMain->UnlockMutex();
	return false;
}

void CViewC64StateREU::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	if (editingRegisterValueIndex != -1)
	{
		u8 v = editHex->value;
		c64d_reu_io2_store((u16)editingRegisterValueIndex, v);
		
		editHex->SetCursorPos(0);
	}

}

//
bool CViewC64StateREU::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editingRegisterValueIndex != -1)
	{
		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = reu_read_without_sideeffects(editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < 0x0F)
			{
				editingRegisterValueIndex++;
				u8 v = reu_read_without_sideeffects(editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > 0x08)
			{
				editingRegisterValueIndex -= 0x08;
				u8 v = reu_read_without_sideeffects(editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < 0x10-0x08)
			{
				editingRegisterValueIndex += 0x08;
				u8 v = reu_read_without_sideeffects(editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		editHex->KeyDown(keyCode);
		return true;
	}
	return false;
}

bool CViewC64StateREU::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64StateREU::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

// Layout
void CViewC64StateREU::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64StateREU::Deserialize(CByteBuffer *byteBuffer)
{
}

