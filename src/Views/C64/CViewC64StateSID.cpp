extern "C" {
#include "sid.h"
}
#include "stb_sprintf.h"

#include "CViewC64StateSID.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "CDebugInterfaceC64.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "C64SettingsStorage.h"
#include "VID_ImageBinding.h"
#include "C64SIDFrequencies.h"
#include "CViewC64SidTrackerHistory.h"
#include "CDebugInterfaceVice.h"
#include "CViewWaveform.h"
#include "CWaveformData.h"
#include "CSlrFileFromOS.h"
#include "CLayoutParameter.h"
#include "CByteBuffer.h"

#define ONE_SID_STATE_DEFAULT_SIZE_X 235.0f

CViewC64StateSID::CViewC64StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	recentlyOpened = new CRecentlyOpenedFiles(new CSlrString("recents-sidregs"), this);
	
	// local sid data for store/restore purposes
	sidData = new CSidData();

	fontBytes = viewC64->fontDisassembly;
	fontBytesSize = 7.0f;
//	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontBytesSize));

	oneSidStateSizeX = 170.0f;
	
	selectedSidNumber = 0;
	
	//
	sidRegsFileExtensions.push_back(new CSlrString("sidregs"));
	sidRegsFileExtensions.push_back(new CSlrString("bin"));
	
	//
	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;
	editingSIDIndex = -1;
	
	// waveforms
	showAllSidChips = true;
	AddLayoutParameter(new CLayoutParameterBool("Show all SID chips", &showAllSidChips));

	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	buttonSizeX = 25.0f;
	buttonSizeY = 8.0f;
	
	for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
	{
		for (int i = 0; i < 3; i++)
		{
			viewChannelWaveform[sidNum][i] = new CViewWaveform("CViewC64StateSID::CViewWaveform", 0, 0, 0, 0, 0, debugInterface->sidChannelWaveform[sidNum][i]);
		}
		viewMixWaveform[sidNum] = new CViewWaveform("CViewC64StateSID::CViewWaveform", 0, 0, 0, 0, 0, debugInterface->sidMixWaveform[sidNum]);
		
		// button
		btnsSelectSID[sidNum] = new CGuiButtonSwitch(NULL, NULL, NULL,
											   0, 0, posZ, buttonSizeX, buttonSizeY,
											   new CSlrString("D400"),
											   FONT_ALIGN_CENTER, buttonSizeX/2, 2.5,
											   font, fontScale,
											   1.0, 1.0, 1.0, 1.0,
											   1.0, 1.0, 1.0, 1.0,
											   0.3, 0.3, 0.3, 1.0,
											   this);
		btnsSelectSID[sidNum]->SetOn(false);
		
		btnsSelectSID[sidNum]->buttonSwitchOffColorR = 0.0f;
		btnsSelectSID[sidNum]->buttonSwitchOffColorG = 0.0f;
		btnsSelectSID[sidNum]->buttonSwitchOffColorB = 0.0f;
		btnsSelectSID[sidNum]->buttonSwitchOffColorA = 1.0f;

		btnsSelectSID[sidNum]->buttonSwitchOffColor2R = 0.3f;
		btnsSelectSID[sidNum]->buttonSwitchOffColor2G = 0.3f;
		btnsSelectSID[sidNum]->buttonSwitchOffColor2B = 0.3f;
		btnsSelectSID[sidNum]->buttonSwitchOffColor2A = 1.0f;

		btnsSelectSID[sidNum]->buttonSwitchOnColorR = 0.0f;
		btnsSelectSID[sidNum]->buttonSwitchOnColorG = 0.0f;
		btnsSelectSID[sidNum]->buttonSwitchOnColorB = 0.7f;
		btnsSelectSID[sidNum]->buttonSwitchOnColorA = 1.0f;

		btnsSelectSID[sidNum]->buttonSwitchOnColor2R = 0.3f;
		btnsSelectSID[sidNum]->buttonSwitchOnColor2G = 0.3f;
		btnsSelectSID[sidNum]->buttonSwitchOnColor2B = 0.3f;
		btnsSelectSID[sidNum]->buttonSwitchOnColor2A = 1.0f;

		this->AddGuiElement(btnsSelectSID[sidNum]);
	}
	
	consumeTapBackground = false;
	
	buttonSizeY = 10.0f;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	this->SelectSid(0);
	
}

void CViewC64StateSID::UpdateWaveformsPosition()
{
	oneSidStateSizeX = ONE_SID_STATE_DEFAULT_SIZE_X/7.0f * fontBytesSize;

	// waveforms
	float wsx = fontBytesSize*10.0f;
	float wsy = fontBytesSize*3.5f;
	float wgx = fontBytesSize*23.0f;
	float wgy = fontBytesSize*9.0f;
	
	float px = posX;
	float py = posY;
	
	for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->SetPosition(px, py);
		
		px += buttonSizeX + 5.0f;
	}
	
	px = posX + wgx;
	py += buttonSizeY;
	
	for (int chanNum = 0; chanNum < 3; chanNum++)
	{
		for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
		{
			float pxs;
			if (showAllSidChips == false)
			{
				pxs = px;
			}
			else
			{
				pxs = px + (float)sidNum * oneSidStateSizeX;
			}
			viewChannelWaveform[sidNum][chanNum]->SetPosition(pxs, py, posZ, wsx, wsy);
		}
		py += wgy;
	}
	
	py += fontBytesSize*1;
	
	for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
	{
		float pxs;
		
		if (showAllSidChips == false)
		{
			pxs = px;
		}
		else
		{
			pxs = px + (float)sidNum * oneSidStateSizeX;
		}
		viewMixWaveform[sidNum]->SetPosition(pxs, py, posZ, wsx, wsy);
	}
}

void CViewC64StateSID::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	float sx = ONE_SID_STATE_DEFAULT_SIZE_X * (showAllSidChips ? (float)debugInterface->GetNumSids() : 1.0f);
	
	fontBytesSize = 7.0f/sx * sizeX;
	UpdateWaveformsPosition();
}

void CViewC64StateSID::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	LOGD("CViewC64StateSID::LayoutParameterChanged: renderHorizontal=%s", STRBOOL(showAllSidChips));
	
	UpdateWaveformsPosition();
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64StateSID::SetVisible(bool isVisible)
{
	CGuiElement::SetVisible(isVisible);

	// TODO: MAKE ME HAPPY
	for (int sidNum = 0; sidNum < debugInterface->GetNumSids(); sidNum++)
	{
		viewC64->debugInterfaceC64->SetSIDReceiveChannelsData(sidNum, isVisible);
	}
}

void CViewC64StateSID::UpdateSidButtonsState()
{
	guiMain->LockMutex();
	
	for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->visible = false;
	}
	
	if (showAllSidChips == false)
	{
		if (c64SettingsSIDStereo >= 1)
		{
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "%04X", c64SettingsSIDStereoAddress);
			
			CSlrString *str = new CSlrString(buf);
			btnsSelectSID[1]->SetText(str);
			delete str;
			SYS_ReleaseCharBuf(buf);
			
			btnsSelectSID[0]->visible = true;
			btnsSelectSID[1]->visible = true;
		}
		
		if (c64SettingsSIDStereo >= 2)
		{
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "%04X", c64SettingsSIDTripleAddress);
			
			CSlrString *str = new CSlrString(buf);
			btnsSelectSID[2]->SetText(str);
			delete str;
			SYS_ReleaseCharBuf(buf);

			btnsSelectSID[2]->visible = true;
		}
	}

	if (selectedSidNumber > c64SettingsSIDStereo)
	{
		SelectSid(0);
	}
	
	guiMain->UnlockMutex();
}

void CViewC64StateSID::SelectSid(int sidNum)
{
	guiMain->LockMutex();
	
	if (this->visible)
	{
		if (showAllSidChips == false)
		{
			viewC64->debugInterfaceC64->SetSIDReceiveChannelsData(this->selectedSidNumber, false);
			viewC64->debugInterfaceC64->SetSIDReceiveChannelsData(sidNum, true);
		}
		else
		{
			for (int sidNum = 0; sidNum < viewC64->debugInterfaceC64->GetNumSids(); sidNum++)
			{
				viewC64->debugInterfaceC64->SetSIDReceiveChannelsData(sidNum, true);
			}
		}
	}
	
	this->selectedSidNumber = sidNum;
	
	for (int i = 0; i < C64_MAX_NUM_SIDS; i++)
	{
		btnsSelectSID[i]->SetOn(false);
	}

	btnsSelectSID[this->selectedSidNumber]->SetOn(true);

	guiMain->UnlockMutex();
}

bool CViewC64StateSID::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
	{
		if (button == btnsSelectSID[sidNum])
		{
			SelectSid(sidNum);
			return true;
		}
	}
	
	return false;
}

void CViewC64StateSID::RenderImGui()
{
	// FIXME: call once
	UpdateSidButtonsState();

	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64StateSID::Render()
{
//	if (viewC64->debugInterface->GetSettingIsWarpSpeed() == true)
//		return;

	// calc trigger pos, render waveform
	if (showAllSidChips == false)
	{
		this->RenderStateSID(selectedSidNumber, posX, posY + buttonSizeY, posZ, fontBytes, fontBytesSize);
	
		for (int i = 0; i < 3; i++)
		{
			viewChannelWaveform[selectedSidNumber][i]->Render();
		}
	
		viewMixWaveform[selectedSidNumber]->Render();
	}
	else
	{
		// show all sids at once
		int numSids = debugInterface->GetNumSids();
		for (int sidNum = 0; sidNum < numSids; sidNum++)
		{
			this->RenderStateSID(sidNum, posX, posY + buttonSizeY, posZ, fontBytes, fontBytesSize);
			
			for (int i = 0; i < 3; i++)
			{
				viewChannelWaveform[sidNum][i]->Render();
			}
			
			viewMixWaveform[sidNum]->Render();
		}
	}

	CGuiView::Render();
}

void CViewC64StateSID::RenderStateSID(int sidNum, float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize)
{
	char buf[256];

	for (int sidNum = 0; sidNum < debugInterface->GetNumSids(); sidNum++)
	{
		float px = posX;
		float py = posY;

		px += oneSidStateSizeX * (float)sidNum;
		
		if (showAllSidChips == false)
		{
			sidNum = selectedSidNumber;
		}
		
		uint16 sidBase = GetSidAddressByChipNum(sidNum);
		
		if (showRegistersOnly)
		{
			float fs2 = fontSize;
			
			float plx = px + fontSize*15;
			float ply = py;
			for (int i = 0; i < 0x1D; i++)
			{
				if (editingSIDIndex == sidNum && editingRegisterValueIndex == i)
				{
					sprintf(buf, "%04x", sidBase+i);
					fontBytes->BlitText(buf, plx, ply, posZ, fs2);
					fontBytes->BlitTextColor(editHex->textWithCursor, plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
				}
				else
				{
					u8 v = debugInterface->GetSidRegister(sidNum, i);
					sprintf(buf, "%04x %02x", sidBase+i, v);
					fontBytes->BlitText(buf, plx, ply, posZ, fs2);
				}
				
				ply += fs2;
				
				if (i == 0x06 || i == 0x0D || i == 0x14)
				{
					ply += fs2;
				}
				//			else if (i == 0x14)
				//			{
				//				ply = py;
				//				plx += fs2 * 9;
				//			}
			}
			
			if (showAllSidChips == false)
				return;
			
			continue;
		}
		
		uint8 reg_freq_lo, reg_freq_hi, reg_pw_lo, reg_pw_hi, reg_ad, reg_sr, reg_ctrl, reg_res_filter, reg_volume, reg_filter_lo, reg_filter_hi;
		
		reg_res_filter = sid_peek(sidBase + 0x17);
		reg_volume  = sid_peek(sidBase + 0x18);
		reg_filter_lo = sid_peek(sidBase + 0x15);
		reg_filter_hi = sid_peek(sidBase + 0x16);
		
		for (int voice = 0; voice < 3; voice++)
		{
			uint16 voiceBase = sidBase + voice * 0x07;
			
			reg_freq_lo = sid_peek(voiceBase + 0x00);
			reg_freq_hi = sid_peek(voiceBase + 0x01);
			reg_pw_lo = sid_peek(voiceBase + 0x02);
			reg_pw_hi = sid_peek(voiceBase + 0x03);
			reg_ctrl = sid_peek(voiceBase + 0x04);
			reg_ad = sid_peek(voiceBase + 0x05);
			reg_sr = sid_peek(voiceBase + 0x06);
			
			uint16 freq = (reg_freq_hi << 8) | reg_freq_lo;
			
			sprintf(buf, "Voice #%d", (voice+1));
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			//		sprintf(buf, " Frequency  : %04x", freq);
			const sid_frequency_t *sidFrequencyData = SidFrequencyToNote(freq);
			sprintf(buf, " Frequency  : %04x %s", freq, sidFrequencyData->name);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			
			sprintf(buf, " Pulse Width: %04x", ((reg_pw_hi & 0x0f) << 8) | reg_pw_lo);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			sprintf(buf, " Env. (ADSR): %1.1x %1.1x %1.1x %1.1x",
					reg_ad >> 4, reg_ad & 0x0f,
					reg_sr >> 4, reg_sr & 0x0f);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			sprintf(buf, " Waveform   : ");
			PrintSidWaveform(reg_ctrl, buf);
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			sprintf(buf, " Gate       : %s  Ring mod.: %s", reg_ctrl & 0x01 ? "On " : "Off", reg_ctrl & 0x04 ? "On" : "Off");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			sprintf(buf, " Test bit   : %s  Synchron.: %s", reg_ctrl & 0x08 ? "On " : "Off", reg_ctrl & 0x02 ? "On" : "Off");
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			if (voice == 2)
			{
				sprintf(buf, " Filter     : %s  Mute     : %s", reg_res_filter & (1 << voice) ? "On " : "Off", reg_volume & 0x80 ? "Yes" : "No");
			}
			else
			{
				sprintf(buf, " Filter     : %s", reg_res_filter & (1 << voice) ? "On " : "Off");
			}
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
			py += fontSize;
		}
		
		sprintf(buf, "Filters/Volume");
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, " Frequency: %04x", (reg_filter_hi << 3) | (reg_filter_lo & 0x07));
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, " Resonance: %1.1x", reg_res_filter >> 4);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, " Mode     : ");
		if (reg_volume & 0x70)
		{
			if (reg_volume & 0x10) strcat(buf, "LP ");
			if (reg_volume & 0x20) strcat(buf, "BP ");
			if (reg_volume & 0x40) strcat(buf, "HP");
		}
		else
		{
			strcat(buf, "None");
		}
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, " Volume   : %1.1x", reg_volume & 0x0f);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		if (showAllSidChips == false)
			return;
	}

}

void CViewC64StateSID::PrintSidWaveform(uint8 wave, char *buf)
{
	if (wave & 0xf0) {
		if (wave & 0x10) strcat(buf, "Triangle ");
		if (wave & 0x20) strcat(buf, "Sawtooth ");
		if (wave & 0x40) strcat(buf, "Rectangle ");
		if (wave & 0x80) strcat(buf, "Noise");
	} else
		strcat(buf, "None");
}


void CViewC64StateSID::DoLogic()
{
}

bool CViewC64StateSID::DoTap(float x, float y)
{
	if (!IsInsideView(x, y))
		return false;
	
	guiMain->LockMutex();
	
	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}
	
	// check if tap register
	if (showRegistersOnly)
	{
		float fontSize = fontBytesSize;

		float px = posX;
		
		editingSIDIndex = -1;
		
		if (showAllSidChips == false)
		{
			editingSIDIndex = selectedSidNumber; //-1;
		}
		else
		{
			// check SID 1
			if (x >= posX && x < posX+oneSidStateSizeX)
			{
				editingSIDIndex = 0;
			}
			else if (x >= posX+oneSidStateSizeX && x < posX+oneSidStateSizeX*2.0f
				&& debugInterface->GetNumSids() > 1)
			{
				px += oneSidStateSizeX;
				editingSIDIndex = 1;
			}
			else if (x >= posX+oneSidStateSizeX*2.0f && x < posX+oneSidStateSizeX*3.0f
				&& debugInterface->GetNumSids() > 2)
			{
				px += 2.0f * oneSidStateSizeX;
				editingSIDIndex = 2;
			}
		}
				
		if (editingSIDIndex != -1)
		{
			float fs2 = fontSize;
			float sx = fs2 * 9;
			
			float plx = px + fontSize*15;
			float plex = plx + fontSize * 7.0f;
			float ply = posY + fontSize*2;
			for (int i = 0; i < 0x1D; i++)
			{
				if (x >= plx && x <= plex
					&& y >= ply && y <= ply+fontSize)
				{
					LOGD("CViewC64StateSID::DoTap: tapped register %02x", i);
					
					editingRegisterValueIndex = i;
					
					u8 v = debugInterface->GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
					editHex->SetValue(v, 2);
					
					guiMain->UnlockMutex();
					return true;
				}
				
				ply += fs2;
				
				if (i == 0x06 || i == 0x0D || i == 0x14)
				{
					ply += fs2;
				}
	//			else if (i == 0x14)
	//			{
	//				ply = py;
	//				plx += fs2 * 9;
	//			}
			}
		}
	}
	
	guiMain->UnlockMutex();

	for (int sidNum = 0; sidNum < debugInterface->GetNumSids(); sidNum++)
	{
		if (showAllSidChips == false)
		{
			sidNum = selectedSidNumber;
		}
		
		for (int i = 0; i < 3; i++)
		{
			if (viewChannelWaveform[sidNum][i]->IsInside(x, y))
			{
				viewChannelWaveform[sidNum][i]->waveform->isMuted = !viewChannelWaveform[sidNum][i]->waveform->isMuted;
				
				debugInterface->UpdateWaveformsMuteStatus();
				return true;
			}
		}

		if (viewMixWaveform[sidNum]->IsInside(x,y))
		{
			viewMixWaveform[sidNum]->waveform->isMuted = !viewMixWaveform[sidNum]->waveform->isMuted;
			viewChannelWaveform[sidNum][0]->waveform->isMuted = viewMixWaveform[sidNum]->waveform->isMuted;
			viewChannelWaveform[sidNum][1]->waveform->isMuted = viewMixWaveform[sidNum]->waveform->isMuted;
			viewChannelWaveform[sidNum][2]->waveform->isMuted = viewMixWaveform[sidNum]->waveform->isMuted;

			debugInterface->UpdateWaveformsMuteStatus();
			return true;
		}
		
		if (showAllSidChips == false)
			break;
	}
	
	if (CGuiView::DoTap(x, y))
		return true;
	
	showRegistersOnly = !showRegistersOnly;

	return false;
}

void CViewC64StateSID::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	if (editingRegisterValueIndex != -1)
	{
		u8 v = editHex->value;
		debugInterface->SetSidRegister(editingSIDIndex, editingRegisterValueIndex, v);
		viewC64->viewC64SidTrackerHistory->UpdateHistoryWithCurrentSidData();
		editHex->SetCursorPos(0);
	}
}


bool CViewC64StateSID::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editingRegisterValueIndex != -1)
	{
		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = debugInterface->GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < 0x1C)
			{
				editingRegisterValueIndex++;
				u8 v = debugInterface->GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > 0x08)
			{
				editingRegisterValueIndex -= 0x08;
				u8 v = debugInterface->GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < 0x1D-0x08)
			{
				editingRegisterValueIndex += 0x08;
				u8 v = debugInterface->GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		editHex->KeyDown(keyCode);
		
		if (keyCode == MTKEY_ENTER)
		{
			editingRegisterValueIndex = -1;
		}
		return true;
	}
	return false;
}

bool CViewC64StateSID::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}


bool CViewC64StateSID::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64StateSID::RenderFocusBorder()
{
	//	CGuiView::RenderFocusBorder();
	//
}

bool CViewC64StateSID::HasContextMenuItems()
{
	return true;
}

void CViewC64StateSID::RenderContextMenuItems()
{
	if (ImGui::MenuItem("Export SID registers"))
	{
		CSlrString *defaultFileName = new CSlrString("registers");

		CSlrString *windowTitle = new CSlrString("Export SID registers");
		viewC64->ShowDialogSaveFile(this, &sidRegsFileExtensions, defaultFileName, c64SettingsDefaultSnapshotsFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
	}
	if (ImGui::MenuItem("Import SID registers"))
	{
		CSlrString *windowTitle = new CSlrString("Import SID registers");
		windowTitle->DebugPrint("windowTitle=");
		viewC64->ShowDialogOpenFile(this, &sidRegsFileExtensions, NULL, windowTitle);
		delete windowTitle;
	}
	recentlyOpened->RenderImGuiMenu("Recent##CViewC64StateSID");
	
	ImGui::Separator();
}

void CViewC64StateSID::SystemDialogFileOpenSelected(CSlrString *path)
{
	bool ret = this->ImportSidRegs(path);
	if (!ret)
	{
		return;
	}
	
	CSlrString *str = path->GetFileNameComponentFromPath();
		
	char *buf = str->GetStdASCII();
	char *buf2 = SYS_GetCharBuf();
	sprintf(buf2, "%s imported", buf);
	viewC64->ShowMessageSuccess(buf2);
	SYS_ReleaseCharBuf(buf2);
	delete [] buf;
	delete str;
	
	recentlyOpened->Add(path);
}

void CViewC64StateSID::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	ImportSidRegs(filePath);
}

bool CViewC64StateSID::ImportSidRegs(CSlrString *path)
{
	CSlrFile *file = new CSlrFileFromOS(path);
	if (!file->Exists())
	{
		guiMain->ShowMessageBox("Error", "Import SID registers failed. Can't open file.");
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer(file);
	
	sidData->Deserialize(byteBuffer);
	sidData->RestoreSids();

	delete byteBuffer;
	delete file;

	return true;
}

void CViewC64StateSID::SystemDialogFileSaveSelected(CSlrString *path)
{
	this->ExportSidRegs(path);
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	viewC64->ShowMessageInfo(str);
	delete str;

	recentlyOpened->Add(path);
}

bool CViewC64StateSID::ExportSidRegs(CSlrString *path)
{
	LOGM("CViewC64StateSID::ExportSidRegs");
	
	path->DebugPrint("ExportSidRegs path=");

	CByteBuffer *byteBuffer = new CByteBuffer();
	sidData->PeekFromSids();
	sidData->Serialize(byteBuffer);
	byteBuffer->storeToFile(path);
	delete byteBuffer;
	
	LOGM("CViewC64StateSID::ExportSidRegs: file saved");
	
	return true;
}

// Layout
void CViewC64StateSID::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64StateSID::Deserialize(CByteBuffer *byteBuffer)
{
}

