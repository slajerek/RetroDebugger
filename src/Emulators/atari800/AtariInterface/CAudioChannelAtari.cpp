#include "EmulatorsConfig.h"
#include "SYS_Types.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "CDebugInterfaceAtari.h"
#include "DebuggerDefs.h"
#include "CAudioChannelAtari.h"
#include "AtariWrapper.h"

extern "C" {
	extern int POKEYSND_stereo_enabled;
	void Sound_Callback(uint8 *buffer, unsigned int size);
}

CAudioChannelAtari::CAudioChannelAtari(CDebugInterfaceAtari *debugInterface)
: CAudioChannel("Atari")
{
	this->debugInterface = debugInterface;
	this->bypass = true;
	
	monoBuffer = new u16[ATARI_AUDIO_BUFFER_FRAMES];
}

void CAudioChannelAtari::FillBuffer(int *mixBuffer, u32 numSamples)
{
#if defined(RUN_ATARI)
	if (POKEYSND_stereo_enabled)
	{
		Sound_Callback((uint8*)monoBuffer, numSamples*4);
	}
	else
	{
		Sound_Callback((uint8*)monoBuffer, numSamples*2);
	}
#endif
	
//	memset(mixBuffer, 0, numSamples*4);

	// TODO: refactor me c64SettingsMuteSIDOnPause-> mute emulation sound on pause
	if (c64SettingsMuteSIDOnPause)
	{
		if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
		{
			memset(mixBuffer, 0, numSamples*4);
			return;
		}
	}

	if (POKEYSND_stereo_enabled)
	{
		u8 *src = (u8*)monoBuffer;
		u8 *dest = (u8*)mixBuffer;

		memcpy(dest, src, numSamples*4);
	}
	else
	{
		// one POKEY, mono
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
		}	}
}


