#include "SYS_Types.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "CDebugInterfaceNes.h"
#include "DebuggerDefs.h"
#include "CAudioChannelNes.h"

#define NES_AUDIO_BUFFER_FRAMES 512

CAudioChannelNes::CAudioChannelNes(CDebugInterfaceNes *debugInterface)
{
	this->debugInterface = debugInterface;
	sprintf(this->name, "nes");
	this->bypass = true;
	
	monoBuffer = new i16[NES_AUDIO_BUFFER_FRAMES];
}

void nesd_audio_callback(i16 *monoBuffer, int numSamples);

void CAudioChannelNes::FillBuffer(int *mixBuffer, u32 numSamples)
{
//	LOGD("FillBuffer: %d", numSamples);
	nesd_audio_callback(monoBuffer, numSamples);
	
	// TODO: refactor me c64SettingsMuteSIDOnPause-> mute emulation sound on pause
	if (c64SettingsMuteSIDOnPause)
	{
		if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
		{
			memset(mixBuffer, 0, numSamples*4);
			return;
		}
	}

	int amount = numSamples;
	
	u8 *src = (u8*)monoBuffer;
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


