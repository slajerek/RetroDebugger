#ifndef _CVIEWC64USTATESID_H_
#define _CVIEWC64USTATESID_H_

#include "CViewBaseStateSID.h"

class CDebugInterfaceC64U;

// C64 Ultimate SID state view. The Ultimate reports its SID registers through
// a logical state cache that mirrors $D400-$D41F; there is no write path and
// no per-voice audio, so this backend is read-only and supplies no waveforms.
class CViewC64UStateSID : public CViewBaseStateSID
{
public:
	CViewC64UStateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64U *debugInterface);

	CDebugInterfaceC64U *debugInterface;

	virtual int GetNumSids();
	virtual u8 GetSidRegister(int sidNum, int registerNum);
	virtual void SetSidRegister(int sidNum, int registerNum, u8 value);
	virtual u16 GetSidBaseAddress(int sidNum);
	virtual int GetNumRegisters();
	virtual bool IsRegisterWritable();
};

#endif
