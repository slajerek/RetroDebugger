#include "CViewGT2StateSID.h"
#include "CGT2SidRegisters.h"
#include "CGT2VoiceWaveforms.h"
#include "CGT2AudioMixer.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewWaveform.h"
#include "CWaveformData.h"

extern "C" {
#include "gcommon.h"
#include "gsid.h"
}

CViewGT2StateSID::CViewGT2StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CViewBaseStateSID(name, posX, posY, posZ, sizeX, sizeY, "recents-sidregs-gt2")
{
	InitStateSID();
}

int CViewGT2StateSID::GetNumSids()
{
	return 1;
}

int CViewGT2StateSID::GetNumRegisters()
{
	// GT2 keeps $00..$18 only -- it never reads OSC3/ENV3 back from the chip,
	// so there is nothing truthful to show for $19..$1C.
	return GT2_NUM_SID_REGISTERS;
}

u8 CViewGT2StateSID::GetSidRegister(int sidNum, int registerNum)
{
	return GT2_GetSidRegister(registerNum);
}

void CViewGT2StateSID::SetSidRegister(int sidNum, int registerNum, u8 value)
{
	GT2_SetSidRegister(registerNum, value);
}

bool CViewGT2StateSID::IsRegisterWriteEffective(int sidNum, int registerNum)
{
	return GT2_IsSidRegisterWriteEffective(registerNum);
}

u16 CViewGT2StateSID::GetSidBaseAddress(int sidNum)
{
	return 0xD400;
}

CWaveformData *CViewGT2StateSID::GetChannelWaveform(int sidNum, int voice)
{
	if (voice < 0 || voice >= MAX_CHN)
		return NULL;
	return gt2VoiceWaveform[voice];
}

CWaveformData *CViewGT2StateSID::GetMixWaveform(int sidNum)
{
	return gt2MixWaveform;
}

void CViewGT2StateSID::UpdateWaveformsMuteStatus()
{
	// Route mute through the same array and mixer call the GT2 Mixer view and
	// the Renoise mute shortcut use, so all three stay in agreement.
	for (int voice = 0; voice < MAX_CHN; voice++)
	{
		if (viewChannelWaveform[0][voice] == NULL)
			continue;
		gt2_voice_mute[voice] = viewChannelWaveform[0][voice]->waveform->isMuted ? 1 : 0;
	}

	if (pluginGoatTracker != NULL && pluginGoatTracker->audioMixer != NULL)
	{
		pluginGoatTracker->audioMixer->ApplyVoiceMutes(gt2_voice_mute, MAX_CHN);
	}
}

void CViewGT2StateSID::RenderImGui()
{
	// GT2's audio thread runs independently of the VICE frame chain, so the
	// waveform snapshot has to be pulled from the render side -- exactly as
	// CViewGT2Oscilloscope does. Calling it twice in a frame is harmless.
	GT2_VoiceWaveforms_UpdatePerFrame();

	// Mute can also be changed from the Mixer view or the Renoise shortcut;
	// mirror the shared array back into the waveforms so this view agrees.
	for (int voice = 0; voice < MAX_CHN; voice++)
	{
		if (viewChannelWaveform[0][voice] == NULL)
			continue;
		viewChannelWaveform[0][voice]->waveform->isMuted = gt2_voice_mute[voice] ? true : false;
	}

	CViewBaseStateSID::RenderImGui();
}
