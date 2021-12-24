#include "SYS_Types.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "DebuggerDefs.h"
#include "CAudioChannelGoatTracker.h"

#define GT2_AUDIO_BUFFER_FRAMES 1024

extern "C" {
	void gtsound_mixer(Sint32 *dest, unsigned samples);
	void snd_mixdata(Uint8 *dest, unsigned bytes);
};

CAudioChannelGoatTracker::CAudioChannelGoatTracker(C64DebuggerPluginGoatTracker *plugin)
{
	this->plugin = plugin;
	sprintf(this->name, "gt2");
	this->bypass = true;
	
	stereoBuffer = new i32[GT2_AUDIO_BUFFER_FRAMES];
}

void CAudioChannelGoatTracker::FillBuffer(int *mixBuffer, u32 numSamples)
{
//	LOGD("CAudioChannelGoatTracker::FillBuffer: %d", numSamples);
	snd_mixdata((u8*)stereoBuffer, numSamples*2);
//	gtsound_mixer(mixBuffer, numSamples);
	
//	// TODO: refactor me c64SettingsMuteSIDOnPause-> mute emulation sound on pause
//	if (c64SettingsMuteSIDOnPause)
//	{
//		if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
//		{
//			memset(mixBuffer, 0, numSamples*4);
//			return;
//		}
//	}

	int amount = numSamples;
	
	u8 *src = (u8*)stereoBuffer;
	u8 *dest = (u8*)mixBuffer;

	for (int i = 0; i < amount; i++)
	{
		uint8 s1, s2;
		
		s1 = *src;
		src++;
		s2 = *src;
		src++;
		
		// L
		*dest = s1;
		dest++;
		*dest = s2;
		dest++;
		
		// R
		*dest = s1;
		dest++;
		*dest = s2;
		dest++;
		
//		LOGD("%d %d", i, (i16)(s1 << 8 | s2));
	}

//	// remix mono to stereo
//	u16 *inPtr = monoBuffer;
//	u16 *outPtr = (u16*)mixBuffer;
//	for (int i = 0; i < numSamples; i++)
//	{
//		u16 s = *inPtr++;
//		
//		*outPtr++ = s;
//		*outPtr++ = s;
//	}
}


