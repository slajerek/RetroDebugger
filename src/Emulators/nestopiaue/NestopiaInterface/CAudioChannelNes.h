#ifndef _NESAUDIOCHANNEL_H_
#define _NESAUDIOCHANNEL_H_

#include "CAudioChannel.h"
#include "CDebugInterfaceNes.h"

class CAudioChannelNes : public CAudioChannel
{
public:
	CAudioChannelNes(CDebugInterfaceNes *debugInterface);
	
	CDebugInterfaceNes *debugInterface;
	virtual void FillBuffer(int *mixBuffer, u32 numSamples);
	
	i16 *monoBuffer;
};

#endif

