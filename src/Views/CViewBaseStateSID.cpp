#include "CViewBaseStateSID.h"

#include "stb_sprintf.h"
#include "SYS_Main.h"
#include "SYS_KeyCodes.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "CSlrFileFromOS.h"
#include "CViewC64.h"
#include "C64SettingsStorage.h"
#include "C64SIDFrequencies.h"
#include "CViewWaveform.h"
#include "CWaveformData.h"
#include "CLayoutParameter.h"
#include "CByteBuffer.h"

#define ONE_SID_STATE_DEFAULT_SIZE_X 235.0f

CViewBaseStateSID::CViewBaseStateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
									 const char *recentsSettingsName)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	recentlyOpened = new CRecentlyOpenedFiles(new CSlrString(recentsSettingsName), this);

	fontBytes = viewC64->fontDisassembly;
	fontBytesSize = 7.0f;

	oneSidStateSizeX = 170.0f;

	selectedSidNumber = 0;

	sidRegsFileExtensions.push_back(new CSlrString("sidregs"));
	sidRegsFileExtensions.push_back(new CSlrString("bin"));

	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;
	editingSIDIndex = -1;

	showAllSidChips = true;
	AddLayoutParameter(new CLayoutParameterBool("Show all SID chips", &showAllSidChips));

	font = viewC64->fontDefaultCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	buttonSizeX = 25.0f;
	buttonSizeY = 8.0f;

	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		for (int i = 0; i < 3; i++)
			viewChannelWaveform[sidNum][i] = NULL;
		viewMixWaveform[sidNum] = NULL;

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

		cachedSidBaseAddress[sidNum] = 0;
	}

	consumeTapBackground = false;
	buttonSizeY = 10.0f;

	cachedNumSids = -1;
}

CViewBaseStateSID::~CViewBaseStateSID()
{
}

// The setup below reaches the backend, so it cannot run while the base
// constructor is still executing -- the virtuals would dispatch to this class,
// not to the subclass.
void CViewBaseStateSID::InitStateSID()
{
	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		for (int i = 0; i < 3; i++)
		{
			CWaveformData *waveform = GetChannelWaveform(sidNum, i);
			if (waveform != NULL)
			{
				viewChannelWaveform[sidNum][i] =
					new CViewWaveform("CViewBaseStateSID::CViewWaveform", 0, 0, 0, 0, 0, waveform);
			}
		}

		CWaveformData *mixWaveform = GetMixWaveform(sidNum);
		if (mixWaveform != NULL)
		{
			viewMixWaveform[sidNum] =
				new CViewWaveform("CViewBaseStateSID::CViewWaveform", 0, 0, 0, 0, 0, mixWaveform);
		}
	}

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
	this->SelectSid(0);
	UpdateSidButtonsState();
}

//
// Backend surface defaults
//

int CViewBaseStateSID::GetNumRegisters()
{
	return SID_STATE_NUM_REGISTERS;
}

bool CViewBaseStateSID::IsRegisterWritable()
{
	return true;
}

bool CViewBaseStateSID::IsRegisterWriteEffective(int sidNum, int registerNum)
{
	return true;
}

CWaveformData *CViewBaseStateSID::GetChannelWaveform(int sidNum, int voice)
{
	return NULL;
}

CWaveformData *CViewBaseStateSID::GetMixWaveform(int sidNum)
{
	return NULL;
}

void CViewBaseStateSID::UpdateWaveformsMuteStatus()
{
}

void CViewBaseStateSID::SetReceiveChannelsData(int sidNum, bool isReceiving)
{
}

void CViewBaseStateSID::OnRegisterEdited()
{
}

bool CViewBaseStateSID::HasWaveforms()
{
	return viewChannelWaveform[0][0] != NULL;
}

//
// Layout
//

void CViewBaseStateSID::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);

	float sx = ONE_SID_STATE_DEFAULT_SIZE_X * (showAllSidChips ? (float)GetNumSids() : 1.0f);

	fontBytesSize = 7.0f/sx * sizeX;
	UpdateWaveformsPosition();
}

void CViewBaseStateSID::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	UpdateWaveformsPosition();
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewBaseStateSID::SetVisible(bool isVisible)
{
	CGuiElement::SetVisible(isVisible);

	for (int sidNum = 0; sidNum < GetNumSids(); sidNum++)
	{
		SetReceiveChannelsData(sidNum, isVisible);
	}
}

void CViewBaseStateSID::UpdateWaveformsPosition()
{
	oneSidStateSizeX = ONE_SID_STATE_DEFAULT_SIZE_X/7.0f * fontBytesSize;

	float wsx = fontBytesSize*10.0f;
	float wsy = fontBytesSize*3.5f;
	float wgx = fontBytesSize*23.0f;
	float wgy = fontBytesSize*9.0f;

	float px = posX;
	float py = posY;

	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->SetPosition(px, py);
		px += buttonSizeX + 5.0f;
	}

	px = posX + wgx;
	py += buttonSizeY;

	for (int chanNum = 0; chanNum < 3; chanNum++)
	{
		for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
		{
			if (viewChannelWaveform[sidNum][chanNum] == NULL)
				continue;

			float pxs;
			if (showAllSidChips == false)
				pxs = px;
			else
				pxs = px + (float)sidNum * oneSidStateSizeX;

			viewChannelWaveform[sidNum][chanNum]->SetPosition(pxs, py, posZ, wsx, wsy);
		}
		py += wgy;
	}

	py += fontBytesSize*1;

	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		if (viewMixWaveform[sidNum] == NULL)
			continue;

		float pxs;
		if (showAllSidChips == false)
			pxs = px;
		else
			pxs = px + (float)sidNum * oneSidStateSizeX;

		viewMixWaveform[sidNum]->SetPosition(pxs, py, posZ, wsx, wsy);
	}
}

//
// Chip selection
//

void CViewBaseStateSID::UpdateSidButtonsState()
{
	guiMain->LockMutex();

	int numSids = GetNumSids();

	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		btnsSelectSID[sidNum]->visible = false;
	}

	// The chip selector only means anything when there is more than one chip
	// and the view is showing a single one at a time.
	if (showAllSidChips == false && numSids > 1)
	{
		for (int sidNum = 0; sidNum < numSids && sidNum < SID_STATE_MAX_SIDS; sidNum++)
		{
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "%04X", GetSidBaseAddress(sidNum));

			CSlrString *str = new CSlrString(buf);
			btnsSelectSID[sidNum]->SetText(str);
			delete str;
			SYS_ReleaseCharBuf(buf);

			btnsSelectSID[sidNum]->visible = true;
		}
	}

	if (selectedSidNumber >= numSids)
	{
		SelectSid(0);
	}

	guiMain->UnlockMutex();
}

void CViewBaseStateSID::SelectSid(int sidNum)
{
	guiMain->LockMutex();

	if (this->visible)
	{
		if (showAllSidChips == false)
		{
			SetReceiveChannelsData(this->selectedSidNumber, false);
			SetReceiveChannelsData(sidNum, true);
		}
		else
		{
			for (int i = 0; i < GetNumSids(); i++)
			{
				SetReceiveChannelsData(i, true);
			}
		}
	}

	this->selectedSidNumber = sidNum;

	for (int i = 0; i < SID_STATE_MAX_SIDS; i++)
	{
		btnsSelectSID[i]->SetOn(false);
	}

	btnsSelectSID[this->selectedSidNumber]->SetOn(true);

	guiMain->UnlockMutex();
}

bool CViewBaseStateSID::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		if (button == btnsSelectSID[sidNum])
		{
			SelectSid(sidNum);
			return true;
		}
	}

	return false;
}

//
// Rendering
//

void CViewBaseStateSID::RenderImGui()
{
	// Rebuild the chip buttons only when the backend's configuration actually
	// changed -- on the C64 that is the user switching stereo SID settings.
	int numSids = GetNumSids();
	bool configurationChanged = (cachedNumSids != numSids);
	for (int sidNum = 0; sidNum < numSids && sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		u16 baseAddress = GetSidBaseAddress(sidNum);
		if (cachedSidBaseAddress[sidNum] != baseAddress)
		{
			cachedSidBaseAddress[sidNum] = baseAddress;
			configurationChanged = true;
		}
	}

	if (configurationChanged)
	{
		cachedNumSids = numSids;
		UpdateSidButtonsState();
	}

	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewBaseStateSID::Render()
{
	this->RenderStateSID(posX, posY + buttonSizeY, posZ, fontBytes, fontBytesSize);

	if (showAllSidChips == false)
	{
		for (int i = 0; i < 3; i++)
		{
			if (viewChannelWaveform[selectedSidNumber][i] != NULL)
				viewChannelWaveform[selectedSidNumber][i]->Render();
		}
		if (viewMixWaveform[selectedSidNumber] != NULL)
			viewMixWaveform[selectedSidNumber]->Render();
	}
	else
	{
		int numSids = GetNumSids();
		for (int sidNum = 0; sidNum < numSids && sidNum < SID_STATE_MAX_SIDS; sidNum++)
		{
			for (int i = 0; i < 3; i++)
			{
				if (viewChannelWaveform[sidNum][i] != NULL)
					viewChannelWaveform[sidNum][i]->Render();
			}
			if (viewMixWaveform[sidNum] != NULL)
				viewMixWaveform[sidNum]->Render();
		}
	}

	CGuiView::Render();
}

void CViewBaseStateSID::RenderStateSID(float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize)
{
	char buf[256];

	int numRegisters = GetNumRegisters();

	int startSid = 0;
	int endSid = GetNumSids();
	if (!showAllSidChips)
	{
		startSid = selectedSidNumber;
		endSid = selectedSidNumber + 1;
	}
	if (endSid > SID_STATE_MAX_SIDS)
		endSid = SID_STATE_MAX_SIDS;

	for (int sidNum = startSid; sidNum < endSid; sidNum++)
	{
		float px = posX;
		float py = posY;

		px += oneSidStateSizeX * (float)sidNum;

		uint16 sidBase = GetSidBaseAddress(sidNum);

		if (showRegistersOnly)
		{
			float fs2 = fontSize;

			float plx = px + fontSize*15;
			float ply = py;
			for (int i = 0; i < numRegisters; i++)
			{
				if (editingSIDIndex == sidNum && editingRegisterValueIndex == i)
				{
					sprintf(buf, "%04x", sidBase+i);
					fontBytes->BlitText(buf, plx, ply, posZ, fs2);
					fontBytes->BlitTextColor(editHex->textWithCursor, plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
				}
				else
				{
					u8 v = GetSidRegister(sidNum, i);
					sprintf(buf, "%04x %02x", sidBase+i, v);
					fontBytes->BlitText(buf, plx, ply, posZ, fs2);
				}

				ply += fs2;

				if (i == 0x06 || i == 0x0D || i == 0x14)
				{
					ply += fs2;
				}
			}

			continue;
		}

		uint8 reg_freq_lo, reg_freq_hi, reg_pw_lo, reg_pw_hi, reg_ad, reg_sr, reg_ctrl, reg_res_filter, reg_volume, reg_filter_lo, reg_filter_hi;

		reg_res_filter = GetSidRegister(sidNum, 0x17);
		reg_volume  = GetSidRegister(sidNum, 0x18);
		reg_filter_lo = GetSidRegister(sidNum, 0x15);
		reg_filter_hi = GetSidRegister(sidNum, 0x16);

		for (int voice = 0; voice < 3; voice++)
		{
			int voiceBase = voice * 0x07;

			reg_freq_lo = GetSidRegister(sidNum, voiceBase + 0x00);
			reg_freq_hi = GetSidRegister(sidNum, voiceBase + 0x01);
			reg_pw_lo = GetSidRegister(sidNum, voiceBase + 0x02);
			reg_pw_hi = GetSidRegister(sidNum, voiceBase + 0x03);
			reg_ctrl = GetSidRegister(sidNum, voiceBase + 0x04);
			reg_ad = GetSidRegister(sidNum, voiceBase + 0x05);
			reg_sr = GetSidRegister(sidNum, voiceBase + 0x06);

			uint16 freq = (reg_freq_hi << 8) | reg_freq_lo;

			sprintf(buf, "Voice #%d", (voice+1));
			fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

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
	}
}

void CViewBaseStateSID::PrintSidWaveform(uint8 wave, char *buf)
{
	if (wave & 0xf0) {
		if (wave & 0x10) strcat(buf, "Triangle ");
		if (wave & 0x20) strcat(buf, "Sawtooth ");
		if (wave & 0x40) strcat(buf, "Rectangle ");
		if (wave & 0x80) strcat(buf, "Noise");
	} else
		strcat(buf, "None");
}

void CViewBaseStateSID::DoLogic()
{
}

//
// Input
//

bool CViewBaseStateSID::DoTap(float x, float y)
{
	if (!IsInsideView(x, y))
		return false;

	guiMain->LockMutex();

	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}

	int numRegisters = GetNumRegisters();

	if (showRegistersOnly && IsRegisterWritable())
	{
		float fontSize = fontBytesSize;

		float px = posX;

		editingSIDIndex = -1;

		if (showAllSidChips == false)
		{
			editingSIDIndex = selectedSidNumber;
		}
		else
		{
			int numSids = GetNumSids();
			for (int sidNum = 0; sidNum < numSids && sidNum < SID_STATE_MAX_SIDS; sidNum++)
			{
				float chipLeft = posX + oneSidStateSizeX * (float)sidNum;
				if (x >= chipLeft && x < chipLeft + oneSidStateSizeX)
				{
					px = chipLeft;
					editingSIDIndex = sidNum;
					break;
				}
			}
		}

		if (editingSIDIndex != -1)
		{
			float fs2 = fontSize;

			float plx = px + fontSize*15;
			float plex = plx + fontSize * 7.0f;
			float ply = posY + fontSize*2;
			for (int i = 0; i < numRegisters; i++)
			{
				if (x >= plx && x <= plex
					&& y >= ply && y <= ply+fontSize)
				{
					editingRegisterValueIndex = i;

					u8 v = GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
					editHex->SetValue(v, 2);

					guiMain->UnlockMutex();
					return true;
				}

				ply += fs2;

				if (i == 0x06 || i == 0x0D || i == 0x14)
				{
					ply += fs2;
				}
			}
		}
	}

	guiMain->UnlockMutex();

	int numSids = GetNumSids();
	for (int sidNum = 0; sidNum < numSids && sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		if (showAllSidChips == false)
		{
			sidNum = selectedSidNumber;
		}

		for (int i = 0; i < 3; i++)
		{
			if (viewChannelWaveform[sidNum][i] == NULL)
				continue;

			if (viewChannelWaveform[sidNum][i]->IsInside(x, y))
			{
				viewChannelWaveform[sidNum][i]->waveform->isMuted = !viewChannelWaveform[sidNum][i]->waveform->isMuted;

				UpdateWaveformsMuteStatus();
				return true;
			}
		}

		if (viewMixWaveform[sidNum] != NULL && viewMixWaveform[sidNum]->IsInside(x,y))
		{
			viewMixWaveform[sidNum]->waveform->isMuted = !viewMixWaveform[sidNum]->waveform->isMuted;
			for (int i = 0; i < 3; i++)
			{
				if (viewChannelWaveform[sidNum][i] != NULL)
					viewChannelWaveform[sidNum][i]->waveform->isMuted = viewMixWaveform[sidNum]->waveform->isMuted;
			}

			UpdateWaveformsMuteStatus();
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

void CViewBaseStateSID::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;

	if (editingRegisterValueIndex != -1)
	{
		u8 v = editHex->value;
		SetSidRegister(editingSIDIndex, editingRegisterValueIndex, v);
		OnRegisterEdited();
		editHex->SetCursorPos(0);
	}
}

bool CViewBaseStateSID::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editingRegisterValueIndex != -1)
	{
		int numRegisters = GetNumRegisters();

		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < numRegisters-1)
			{
				editingRegisterValueIndex++;
				u8 v = GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > 0x08)
			{
				editingRegisterValueIndex -= 0x08;
				u8 v = GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < numRegisters-0x08)
			{
				editingRegisterValueIndex += 0x08;
				u8 v = GetSidRegister(editingSIDIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}

		editHex->KeyDown(keyCode);
		return true;
	}

	return false;
}

bool CViewBaseStateSID::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewBaseStateSID::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewBaseStateSID::RenderFocusBorder()
{
}

//
// Import / export
//

bool CViewBaseStateSID::HasContextMenuItems()
{
	return true;
}

void CViewBaseStateSID::RenderContextMenuItems()
{
	char *menuId = SYS_GetCharBuf();

	if (ImGui::MenuItem("Export SID registers"))
	{
		CSlrString *defaultFileName = new CSlrString("registers");

		CSlrString *windowTitle = new CSlrString("Export SID registers");
		viewC64->ShowDialogSaveFile(this, &sidRegsFileExtensions, defaultFileName, c64SettingsDefaultSnapshotsFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
	}
	if (IsRegisterWritable() && ImGui::MenuItem("Import SID registers"))
	{
		CSlrString *windowTitle = new CSlrString("Import SID registers");
		viewC64->ShowDialogOpenFile(this, &sidRegsFileExtensions, NULL, windowTitle);
		delete windowTitle;
	}

	sprintf(menuId, "Recent##%s", this->name);
	recentlyOpened->RenderImGuiMenu(menuId);
	SYS_ReleaseCharBuf(menuId);

	ImGui::Separator();
}

void CViewBaseStateSID::SystemDialogFileOpenSelected(CSlrString *path)
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

void CViewBaseStateSID::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	ImportSidRegs(filePath);
}

void CViewBaseStateSID::SystemDialogFileSaveSelected(CSlrString *path)
{
	this->ExportSidRegs(path);

	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	viewC64->ShowMessageInfo(str);
	delete str;

	recentlyOpened->Add(path);
}

// The on-disk format is a flat SID_STATE_MAX_SIDS * 0x20 byte dump, matching
// what CSidData has always written, so a .sidregs file is interchangeable
// between backends.
#define SID_STATE_EXPORT_STRIDE 0x20

bool CViewBaseStateSID::ImportSidRegs(CSlrString *path)
{
	if (!IsRegisterWritable())
		return false;

	CSlrFile *file = new CSlrFileFromOS(path);
	if (!file->Exists())
	{
		guiMain->ShowMessageBox("Error", "Import SID registers failed. Can't open file.");
		delete file;
		return false;
	}

	CByteBuffer *byteBuffer = new CByteBuffer(file);

	int numSids = GetNumSids();
	int numRegisters = GetNumRegisters();

	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		for (int registerNum = 0; registerNum < SID_STATE_EXPORT_STRIDE; registerNum++)
		{
			if (byteBuffer->IsEof())
			{
				delete byteBuffer;
				delete file;
				return true;
			}

			u8 v = byteBuffer->GetU8();

			if (sidNum < numSids && registerNum < numRegisters)
			{
				SetSidRegister(sidNum, registerNum, v);
			}
		}
	}

	OnRegisterEdited();

	delete byteBuffer;
	delete file;

	return true;
}

bool CViewBaseStateSID::ExportSidRegs(CSlrString *path)
{
	CByteBuffer *byteBuffer = new CByteBuffer();

	int numSids = GetNumSids();
	int numRegisters = GetNumRegisters();

	for (int sidNum = 0; sidNum < SID_STATE_MAX_SIDS; sidNum++)
	{
		for (int registerNum = 0; registerNum < SID_STATE_EXPORT_STRIDE; registerNum++)
		{
			u8 v = 0;
			if (sidNum < numSids && registerNum < numRegisters)
			{
				v = GetSidRegister(sidNum, registerNum);
			}
			byteBuffer->PutU8(v);
		}
	}

	byteBuffer->storeToFile(path);
	delete byteBuffer;

	return true;
}

// Layout
void CViewBaseStateSID::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewBaseStateSID::Deserialize(CByteBuffer *byteBuffer)
{
}
