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
#include "CViewMemoryMap.h"
#include "CDebugInterfaceC64.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "C64SettingsStorage.h"
#include "VID_ImageBinding.h"
#include "C64SIDFrequencies.h"
#include "CViewC64SidTrackerHistory.h"
#include "CViewWaveform.h"
#include "CLayoutParameter.h"

// TODO: calculate this based on fontSize!
#define ONE_SID_STATE_SIZE_X	170.0f


CViewC64StateSID::CViewC64StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	fontBytes = viewC64->fontDisassembly;
	fontBytesSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontBytesSize));

	selectedSidNumber = 0;
	
	//
	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;
	editingSIDIndex = -1;
	
	// waveforms
	renderHorizontal = false;
	AddLayoutParameter(new CLayoutParameterBool("Horizontal", &renderHorizontal));

	waveformPos = 0;

	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	buttonSizeX = 25.0f;
	buttonSizeY = 8.0f;
	
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
	{
		for (int i = 0; i < 3; i++)
		{
			sidChannelWaveform[sidNum][i] = new CViewWaveform(0, 0, 0, 0, 0, SID_WAVEFORM_LENGTH);
		}
		sidMixWaveform[sidNum] = new CViewWaveform(0, 0, 0, 0, 0, SID_WAVEFORM_LENGTH);
		
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

void CViewC64StateSID::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	// waveforms
	float wsx = fontBytesSize*10.0f;
	float wsy = fontBytesSize*3.5f;
	float wgx = fontBytesSize*23.0f;
	float wgy = fontBytesSize*9.0f;
	
	float px = posX;
	float py = posY;
	
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->SetPosition(px, py);
		
		px += buttonSizeX + 5.0f;
	}
	
	px = posX + wgx;
	py += buttonSizeY;
	
	for (int chanNum = 0; chanNum < 3; chanNum++)
	{
		for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
		{
			float pxs;
			if (renderHorizontal == false)
			{
				pxs = px;
			}
			else
			{
				pxs = px + (float)sidNum * ONE_SID_STATE_SIZE_X;
			}
			sidChannelWaveform[sidNum][chanNum]->SetPosition(pxs, py, posZ, wsx, wsy);
		}
		py += wgy;
	}
	
	py += fontBytesSize*1;
	
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
	{
		float pxs;
		
		if (renderHorizontal == false)
		{
			pxs = px;
		}
		else
		{
			pxs = px + (float)sidNum * ONE_SID_STATE_SIZE_X;
		}
		sidMixWaveform[sidNum]->SetPosition(pxs, py, posZ, wsx, wsy);
	}
}

void CViewC64StateSID::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	// waveforms
	float wsx = fontBytesSize*10.0f;
	float wsy = fontBytesSize*3.5f;
	float wgx = fontBytesSize*23.0f;
	float wgy = fontBytesSize*9.0f;
	
	float px = posX;
	float py = posY;
	
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->SetPosition(px, py);
		
		px += buttonSizeX + 5.0f;
	}
	
	px = posX + wgx;
	py += buttonSizeY;
	
	for (int chanNum = 0; chanNum < 3; chanNum++)
	{
		for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
		{
			float pxs;
			if (renderHorizontal == false)
			{
				pxs = px;
			}
			else
			{
				pxs = px + (float)sidNum * ONE_SID_STATE_SIZE_X;
			}
			sidChannelWaveform[sidNum][chanNum]->SetPosition(pxs, py, posZ, wsx, wsy);
		}
		py += wgy;
	}
	
	py += fontBytesSize*1;
	
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
	{
		float pxs;
		
		if (renderHorizontal == false)
		{
			pxs = px;
		}
		else
		{
			pxs = px + (float)sidNum * ONE_SID_STATE_SIZE_X;
		}
		sidMixWaveform[sidNum]->SetPosition(pxs, py, posZ, wsx, wsy);
	}
	
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
	
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->visible = false;
	}
	
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
		if (renderHorizontal == false)
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
	
	for (int i = 0; i < MAX_NUM_SIDS; i++)
	{
		btnsSelectSID[i]->SetOn(false);
	}

	btnsSelectSID[this->selectedSidNumber]->SetOn(true);

	guiMain->UnlockMutex();
}

bool CViewC64StateSID::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	for (int sidNum = 0; sidNum < MAX_NUM_SIDS; sidNum++)
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

extern "C" {
void c64d_sid_receive_channels_data(int sidNum, int isOn);
}

void CViewC64StateSID::Render()
{
//	if (viewC64->debugInterface->GetSettingIsWarpSpeed() == true)
//		return;

	c64d_sid_receive_channels_data(selectedSidNumber, 1);
	
	if (renderHorizontal == false)
	{
		this->RenderStateSID(selectedSidNumber, posX, posY + buttonSizeY, posZ, fontBytes, fontBytesSize);
	
		sidChannelWaveform[selectedSidNumber][0]->CalculateWaveform();
		sidChannelWaveform[selectedSidNumber][1]->CalculateWaveform();
		sidChannelWaveform[selectedSidNumber][2]->CalculateWaveform();
		sidMixWaveform[selectedSidNumber]->CalculateWaveform();
	
		for (int i = 0; i < 3; i++)
		{
			sidChannelWaveform[selectedSidNumber][i]->Render();
		}
	
		sidMixWaveform[selectedSidNumber]->Render();
	}
	else
	{
		// show all sids at once
		int numSids = debugInterface->GetNumSids();
		for (int sidNum = 0; sidNum < numSids; sidNum++)
		{
			this->RenderStateSID(sidNum, posX, posY + buttonSizeY, posZ, fontBytes, fontBytesSize);
			
			sidChannelWaveform[sidNum][0]->CalculateWaveform();
			sidChannelWaveform[sidNum][1]->CalculateWaveform();
			sidChannelWaveform[sidNum][2]->CalculateWaveform();
			sidMixWaveform[sidNum]->CalculateWaveform();
			
			for (int i = 0; i < 3; i++)
			{
				sidChannelWaveform[sidNum][i]->Render();
			}
			
			sidMixWaveform[sidNum]->Render();
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

		px += ONE_SID_STATE_SIZE_X * (float)sidNum;
		
		if (renderHorizontal == false)
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
			
			if (renderHorizontal == false)
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
		
		if (renderHorizontal == false)
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
		
		if (renderHorizontal == false)
		{
			editingSIDIndex = selectedSidNumber; //-1;
		}
		else
		{
			// check SID 1
			if (x >= posX && x < posX+ONE_SID_STATE_SIZE_X)
			{
				editingSIDIndex = 0;
			}
			else if (x >= posX+ONE_SID_STATE_SIZE_X && x < posX+ONE_SID_STATE_SIZE_X*2.0f
				&& debugInterface->GetNumSids() > 1)
			{
				px += ONE_SID_STATE_SIZE_X;
				editingSIDIndex = 1;
			}
			else if (x >= posX+ONE_SID_STATE_SIZE_X*2.0f && x < posX+ONE_SID_STATE_SIZE_X*3.0f
				&& debugInterface->GetNumSids() > 2)
			{
				px += 2.0f * ONE_SID_STATE_SIZE_X;
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
		if (renderHorizontal == false)
		{
			sidNum = selectedSidNumber;
		}
		
		for (int i = 0; i < 3; i++)
		{
			if (sidChannelWaveform[sidNum][i]->IsInside(x, y))
			{
				sidChannelWaveform[sidNum][i]->isMuted = !sidChannelWaveform[sidNum][i]->isMuted;
				
				viewC64->debugInterfaceC64->SetSIDMuteChannels(sidNum,
															sidChannelWaveform[sidNum][0]->isMuted,
															sidChannelWaveform[sidNum][1]->isMuted,
															sidChannelWaveform[sidNum][2]->isMuted, false);

				if (sidChannelWaveform[sidNum][0]->isMuted
					&& sidChannelWaveform[sidNum][1]->isMuted
					&& sidChannelWaveform[sidNum][2]->isMuted)
				{
					sidMixWaveform[sidNum]->isMuted = true;
				}
				else if (!sidChannelWaveform[sidNum][0]->isMuted
						 || !sidChannelWaveform[sidNum][1]->isMuted
						 || !sidChannelWaveform[sidNum][2]->isMuted)
				{
					sidMixWaveform[sidNum]->isMuted = false;
				}
				return true;
			}
		}

		if (sidMixWaveform[sidNum]->IsInside(x,y))
		{
			sidMixWaveform[sidNum]->isMuted = !sidMixWaveform[sidNum]->isMuted;
			sidChannelWaveform[sidNum][0]->isMuted = sidMixWaveform[sidNum]->isMuted;
			sidChannelWaveform[sidNum][1]->isMuted = sidMixWaveform[sidNum]->isMuted;
			sidChannelWaveform[sidNum][2]->isMuted = sidMixWaveform[sidNum]->isMuted;

			viewC64->debugInterfaceC64->SetSIDMuteChannels(sidNum,
														sidChannelWaveform[sidNum][0]->isMuted,
														sidChannelWaveform[sidNum][1]->isMuted,
														sidChannelWaveform[sidNum][2]->isMuted, false);
		}
		
		if (renderHorizontal == false)
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


void CViewC64StateSID::AddWaveformData(int sidNumber, int v1, int v2, int v3, short mix)
{
	//	LOGD("CViewC64StateSID::AddWaveformData: sid#%d, %d %d %d %d", sidNumber, v1, v2, v3, mix);
	
	// sid channels
	sidChannelWaveform[sidNumber][0]->waveformData[waveformPos] = v1;
	sidChannelWaveform[sidNumber][1]->waveformData[waveformPos] = v2;
	sidChannelWaveform[sidNumber][2]->waveformData[waveformPos] = v3;
	
	// mix channel
	sidMixWaveform[sidNumber]->waveformData[waveformPos] = mix;
	
	waveformPos++;
	
	if (waveformPos == SID_WAVEFORM_LENGTH)
	{
//		guiMain->LockRenderMutex();
//		sidChannelWaveform[sidNumber][0]->CalculateWaveform();
//		sidChannelWaveform[sidNumber][1]->CalculateWaveform();
//		sidChannelWaveform[sidNumber][2]->CalculateWaveform();
//		sidMixWaveform[sidNumber]->CalculateWaveform();
//		guiMain->UnlockRenderMutex();
		
		waveformPos = 0;
	}
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

// Layout
void CViewC64StateSID::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64StateSID::Deserialize(CByteBuffer *byteBuffer)
{
}

