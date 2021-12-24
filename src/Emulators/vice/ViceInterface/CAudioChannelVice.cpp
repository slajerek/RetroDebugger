#include "SYS_Types.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "DebuggerDefs.h"

extern "C" {
	void sdl_callback(void *userdata, uint8 *stream, int len);
}

#include "CAudioChannelVice.h"

CAudioChannelVice::CAudioChannelVice(CDebugInterfaceVice *debugInterface)
{
	this->debugInterface = debugInterface;
	sprintf(this->name, "c64");
}

void CAudioChannelVice::FillBuffer(int *mixBuffer, u32 numSamples)
{
	sdl_callback(NULL, (uint8*)mixBuffer, numSamples);

//	memset(mixBuffer, 0, numSamples*4);

	
	if (c64SettingsMuteSIDOnPause)
	{
		if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
		{
			memset(mixBuffer, 0, numSamples*4);
		}
	}
}


