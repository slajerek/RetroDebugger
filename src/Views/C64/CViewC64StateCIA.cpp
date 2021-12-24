extern "C" {
#include "cia.h"
#include "c64.h"
}
#include "CViewC64StateCIA.h"
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

CViewC64StateCIA::CViewC64StateCIA(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	renderCIA1 = true;
	renderCIA2 = true;
	
	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;
	
	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;
	editingCIAIndex = -1;

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateCIA::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64StateCIA::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64StateCIA::DoLogic()
{
}

void CViewC64StateCIA::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;
	
	float px = posX;
	if (renderCIA1)
	{
		this->RenderStateCIA(px, posY, posZ, fontBytes, fontSize, 1);
		px += 190;
	}
	
	if (renderCIA2)
	{
		this->RenderStateCIA(px, posY, posZ, fontBytes, fontSize, 2);
	}
}

/// render states
extern "C" {
	BYTE c64d_ciacore_peek(cia_context_t *cia_context, WORD addr);
}

void CViewC64StateCIA::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


void CViewC64StateCIA::RenderStateCIA(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId)
{
	char buf[256];
	cia_context_t *cia_context;
	char addr;
	
	if (ciaId == 1)
	{
		cia_context = machine_context.cia1;
		addr = 'C';
	}
	else
	{
		cia_context = machine_context.cia2;
		addr = 'D';
	}
	
	sprintf(buf, "CIA %d:", ciaId);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	

	if (showRegistersOnly)
	{
		float fs2 = fontSize;
		
		float plx = px;
		float ply = py;
		for (int i = 0; i < 0x10; i++)
		{
			if (editingCIAIndex == ciaId && editingRegisterValueIndex == i)
			{
				sprintf(buf, "D%c%02x", addr, i);
				fontBytes->BlitText(buf, plx, ply, posZ, fs2);
				fontBytes->BlitTextColor(editHex->textWithCursor, plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				u8 v = c64d_ciacore_peek(cia_context, i);
				sprintf(buf, "D%c%02x %02x", addr, i, v);
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
	
	uint8 cra = c64d_ciacore_peek(cia_context, 0x0e);
	
	sprintf(buf, "ICR: %02x CTRLA: %02x CTRLB: %02x",
			c64d_ciacore_peek(cia_context, 0x0d), c64d_ciacore_peek(cia_context, 0x0e), c64d_ciacore_peek(cia_context, 0x0f));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	sprintf(buf, "Port A:  %02x DDR: %02x", c64d_ciacore_peek(cia_context, 0x00), c64d_ciacore_peek(cia_context, 0x02));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	sprintf(buf, "Port B:  %02x DDR: %02x", c64d_ciacore_peek(cia_context, 0x01), c64d_ciacore_peek(cia_context, 0x03));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	sprintf(buf, "Serial data : %02x %s", c64d_ciacore_peek(cia_context, 0x0c), cra & 0x40 ? "Output" : "Input");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	sprintf(buf, "Timer A  : %s", cra & 1 ? "On" : "Off");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	sprintf(buf, " Counter : %04x", c64d_ciacore_peek(cia_context, 0x04) + (c64d_ciacore_peek(cia_context, 0x05) << 8));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	sprintf(buf, " Run mode: %s", cra & 8 ? "One-shot" : "Continuous");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	sprintf(buf, " Input   : %s", cra & 0x20 ? "CNT" : "Phi2");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	sprintf(buf, " Output  : ");
	if (cra & 2)
		if (cra & 4)
			strcat(buf, "PB6 Toggle");
		else
			strcat(buf, "PB6 Pulse");
		else
			strcat(buf, "None");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	//py+=fontSize;
	
	
	//	sprintf(buf, "Timer B  : %s", crb & 1 ? "On" : "Off");
	//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	//	sprintf(buf, " Counter : %04x", c64d_ciacore_peek(cia_context, 0x06) + (c64d_ciacore_peek(cia_context, 0x07) << 8));
	//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	//	sprintf(buf, " Run mode: %s", crb & 8 ? "One-shot" : "Continuous");
	//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	//	sprintf(buf, " Input   : ");
	//	if (crb & 0x40)
	//		if (crb & 0x20)
	//			strcat(buf, "Timer A underflow (CNT high)");
	//		else
	//			strcat(buf, "Timer A underflow");
	//		else
	//			if (crb & 0x20)
	//				strcat(buf, "CNT");
	//			else
	//				strcat(buf, "Phi2");
	//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	//
	//	sprintf(buf, " Output  : ");
	//	if (crb & 2)
	//		if (crb & 4)
	//			strcat(buf, "PB7 Toggle");
	//		else
	//			strcat(buf, "PB7 Pulse");
	//		else
	//			strcat(buf, "None");
	//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	
	uint8 tod_hr = c64d_ciacore_peek(cia_context, 0x0b);
	uint8 tod_min = c64d_ciacore_peek(cia_context, 0x0a);
	uint8 tod_sec = c64d_ciacore_peek(cia_context, 0x09);
	uint8 tod_10ths = c64d_ciacore_peek(cia_context, 0x08);
	
	sprintf(buf, "TOD      : %1.1x%1.1x:%1.1x%1.1x:%1.1x%1.1x.%1.1x %s",
			(tod_hr >> 4) & 1, tod_hr & 0x0f,
			(tod_min >> 4) & 7, tod_min & 0x0f,
			(tod_sec >> 4) & 7, tod_sec & 0x0f,
			tod_10ths & 0x0f, tod_hr & 0x80 ? "PM" : "AM");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	py += fontSize;
	
}


bool CViewC64StateCIA::DoTap(float x, float y)
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
		editingCIAIndex = -1;
		
		if (renderCIA1)
		{
			if (x >= posX && x < posX+190)
			{
				editingCIAIndex = 1;
//				px = posX;
			}
		}
		
		if (renderCIA2)
		{
			if (x >= posX+190 && x < posX+190+190)
			{
				editingCIAIndex = 2;
				px = posX+190;
			}
		}

		if (editingCIAIndex != -1)
		{
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
					LOGD("CViewC64StateCIA::DoTap: tapped register %02x", i);
					
					editingRegisterValueIndex = i;

					u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
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
	}
	
	showRegistersOnly = !showRegistersOnly;
	
	guiMain->UnlockMutex();
	return false;
}

void CViewC64StateCIA::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	if (editingRegisterValueIndex != -1)
	{
		u8 v = editHex->value;
		debugInterface->SetCiaRegister(editingCIAIndex, editingRegisterValueIndex, v);
		
		editHex->SetCursorPos(0);
	}

}

//
extern "C" {
	cia_context_t *c64d_get_cia_context(int ciaId);
}

bool CViewC64StateCIA::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editingRegisterValueIndex != -1)
	{
		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < 0x0F)
			{
				editingRegisterValueIndex++;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > 0x08)
			{
				editingRegisterValueIndex -= 0x08;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < 0x10-0x08)
			{
				editingRegisterValueIndex += 0x08;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		editHex->KeyDown(keyCode);
		return true;
	}
	return false;
}

bool CViewC64StateCIA::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64StateCIA::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

// Layout
void CViewC64StateCIA::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64StateCIA::Deserialize(CByteBuffer *byteBuffer)
{
}

