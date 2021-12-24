#ifndef _ATARIAUDIOCHANNEL_H_
#define _ATARIAUDIOCHANNEL_H_

#include "CAudioChannel.h"
#include "CDebugInterfaceAtari.h"

class CAudioChannelAtari : public CAudioChannel
{
public:
	CAudioChannelAtari(CDebugInterfaceAtari *debugInterface);
	
	CDebugInterfaceAtari *debugInterface;
	virtual void FillBuffer(int *mixBuffer, u32 numSamples);
	
	u16 *monoBuffer;
};

#endif

