#ifndef _AUDIOCHANNELGOATTRACKER_H_
#define _AUDIOCHANNELGOATTRACKER_H_

#include "CAudioChannel.h"
#include "C64DebuggerPluginGoatTracker.h"

class CAudioChannelGoatTracker : public CAudioChannel
{
public:
	CAudioChannelGoatTracker(C64DebuggerPluginGoatTracker *plugin);
	
	C64DebuggerPluginGoatTracker *plugin;
	virtual void FillBuffer(int *mixBuffer, u32 numSamples);
	
	i32 *stereoBuffer;
};

#endif

