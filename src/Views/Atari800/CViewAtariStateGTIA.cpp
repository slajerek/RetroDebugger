#include "C64D_Version.h"
#if defined(RUN_ATARI)

extern "C" {
#include "gtia.h"
}

#endif

#include "CViewAtariStateGTIA.h"
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
#include "CDebugInterfaceAtari.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CLayoutParameter.h"

#if defined(RUN_ATARI)

CViewAtariStateGTIA::CViewAtariStateGTIA(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;
	
	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewAtariStateGTIA::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewAtariStateGTIA::DoLogic()
{
}

void CViewAtariStateGTIA::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewAtariStateGTIA::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;

	this->RenderState(posX, posY, posZ, fontBytes, fontSize, 1);
}


void CViewAtariStateGTIA::RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId)
{
	char buf[256];
	
	sprintf(buf, "GTIA");
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	py += fontSize*0.5f;

//	char *space = "    ";
	char *space = "  ";
	
	sprintf(buf, "HPOSP0 %02X%sHPOSP1 %02X%sHPOSP2 %02X%sHPOSP3 %02X",
			GTIA_HPOSP0, space, GTIA_HPOSP1, space, GTIA_HPOSP2, space, GTIA_HPOSP3);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	sprintf(buf, "HPOSM0 %02X%sHPOSM1 %02X%sHPOSM2 %02X%sHPOSM3 %02X",
			GTIA_HPOSM0, space, GTIA_HPOSM1, space, GTIA_HPOSM2, space, GTIA_HPOSM3);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	

	sprintf(buf, "SIZEP0 %02X%sSIZEP1 %02X%sSIZEP2 %02X%sSIZEP3 %02X%sSIZEM  %02X",
			GTIA_SIZEP0, space, GTIA_SIZEP1, space, GTIA_SIZEP2, space, GTIA_SIZEP3, space, GTIA_SIZEM);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, "GRAFP0 %02X%sGRAFP1 %02X%sGRAFP2 %02X%sGRAFP3 %02X%sGRAFM  %02X",
			GTIA_GRAFP0, space, GTIA_GRAFP1, space, GTIA_GRAFP2, space, GTIA_GRAFP3, space, GTIA_GRAFM);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

//	sprintf(buf, "D010   %02X    D011   %02X    COLMP0 %02X     COLMP1 %02X",
//			0x00, 0x00, GTIA_COLPM0, GTIA_COLPM1, GTIA_COLPM2, GTIA_COLPM3);
//	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	

	
	
	sprintf(buf, "COLPM0 %02X%sCOLPM1 %02X%sCOLPM2 %02X%sCOLPM3 %02X",
			GTIA_COLPM0, space, GTIA_COLPM1, space, GTIA_COLPM2, space, GTIA_COLPM3);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, "COLPF0 %02X%sCOLPF1 %02X%sCOLPF2 %02X%sCOLPF3 %02X%sCOLBK  %02X",
			GTIA_COLPF0, space, GTIA_COLPF1, space, GTIA_COLPF2, space, GTIA_COLPF3, space, GTIA_COLBK);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, "PRIOR  %02X%sVDELAY %02X%sGRACTL %02X%sCONSOL %02X",
			GTIA_PRIOR, space, GTIA_VDELAY, space, GTIA_GRACTL, space,
			GTIA_GetByte(GTIA_OFFSET_CONSOL, TRUE));
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
	sprintf(buf, "TRIG0  %02X%sTRIG1  %02X%sTRIG2  %02X%sTRIG3  %02X",
			GTIA_TRIG[0] & GTIA_TRIG_latch[0], space, GTIA_TRIG[1] & GTIA_TRIG_latch[1], space,
			GTIA_TRIG[2] & GTIA_TRIG_latch[2], space, GTIA_TRIG[3] & GTIA_TRIG_latch[3]);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
}


bool CViewAtariStateGTIA::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}

	/*
	// check if tap register
	if (showRegistersOnly)
	{
		float px = posX;
		
		if (x >= posX && x < posX+190)
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
					LOGD("CViewAtariStateGTIA::DoTap: tapped register %02x", i);
					
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
	*/
	showRegistersOnly = !showRegistersOnly;
	
	guiMain->UnlockMutex();
	return false;
}

void CViewAtariStateGTIA::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	/*
	if (editingRegisterValueIndex != -1)
	{
		byte v = editHex->value;
		debugInterface->SetCiaRegister(editingCIAIndex, editingRegisterValueIndex, v);
		
		editHex->SetCursorPos(0);
	}
	 */

}

bool CViewAtariStateGTIA::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	/*
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
	 */
	return false;
}

bool CViewAtariStateGTIA::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewAtariStateGTIA::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

#else
//dummy

CViewAtariStateGTIA::CViewAtariStateGTIA(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CGuiView(posX, posY, posZ, sizeX, sizeY) {}
void CViewAtariStateGTIA::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY) {}
void CViewAtariStateGTIA::DoLogic() {}
void CViewAtariStateGTIA::Render() {}
void CViewAtariStateGTIA::RenderImGui() {}
void CViewAtariStateGTIA::RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId) {}
bool CViewAtariStateGTIA::DoTap(float x, float y) { return false; }
void CViewAtariStateGTIA::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled) {}
bool CViewAtariStateGTIA::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper) { return false; }
bool CViewAtariStateGTIA::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper) { return false; }
void CViewAtariStateGTIA::RenderFocusBorder() {}

#endif
