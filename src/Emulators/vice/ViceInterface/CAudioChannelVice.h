#ifndef _CAudioChannelVice_H_
#define _CAudioChannelVice_H_

#include "CAudioChannel.h"
#include "CDebugInterfaceVice.h"

class CAudioChannelVice : public CAudioChannel
{
public:
	CAudioChannelVice(CDebugInterfaceVice *debugInterface);
	
	CDebugInterfaceVice *debugInterface;
		
	virtual void FillBuffer(int *mixBuffer, u32 numSamples);
};

#endif

