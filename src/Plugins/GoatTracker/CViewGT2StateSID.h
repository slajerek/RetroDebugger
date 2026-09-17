#ifndef _CViewGT2StateSID_H_
#define _CViewGT2StateSID_H_

#include "CViewBaseStateSID.h"

// GoatTracker 2 SID state view -- the same view the C64 emulator uses, bound
// to GT2's own SID instead of VICE's.
//
// GT2 drives its own reSID on its own thread, so the register file comes from
// gsid.cpp's sidreg[] and the oscilloscope data from the per-voice buffers
// CGT2VoiceWaveforms fills (the same ones CViewGT2Oscilloscope draws).
//
// Register writes are reverse-mapped onto the player's own state by
// CGT2SidRegisters -- see the comment there for why poking sidreg[] directly
// would not survive a single frame.
class CViewGT2StateSID : public CViewBaseStateSID
{
public:
	CViewGT2StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual int GetNumSids();
	virtual u8 GetSidRegister(int sidNum, int registerNum);
	virtual void SetSidRegister(int sidNum, int registerNum, u8 value);
	virtual u16 GetSidBaseAddress(int sidNum);
	virtual int GetNumRegisters();
	virtual bool IsRegisterWriteEffective(int sidNum, int registerNum);

	virtual CWaveformData *GetChannelWaveform(int sidNum, int voice);
	virtual CWaveformData *GetMixWaveform(int sidNum);
	virtual void UpdateWaveformsMuteStatus();

	virtual void RenderImGui();
};

#endif
