#ifndef _CVIEWC64STATESID_H_
#define _CVIEWC64STATESID_H_

#include "CViewBaseStateSID.h"
#include "EmulatorsConfig.h"

class CDebugInterfaceC64;
class CSidData;

// VICE-backed SID state view. Everything visual lives in CViewBaseStateSID;
// this supplies register access, the waveform buffers VICE fills, and the
// VICE-specific register import/export path.
class CViewC64StateSID : public CViewBaseStateSID
{
public:
	CViewC64StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	virtual ~CViewC64StateSID();

	CDebugInterfaceC64 *debugInterface;

	virtual int GetNumSids();
	virtual u8 GetSidRegister(int sidNum, int registerNum);
	virtual void SetSidRegister(int sidNum, int registerNum, u8 value);
	virtual u16 GetSidBaseAddress(int sidNum);

	virtual CWaveformData *GetChannelWaveform(int sidNum, int voice);
	virtual CWaveformData *GetMixWaveform(int sidNum);
	virtual void UpdateWaveformsMuteStatus();
	virtual void SetReceiveChannelsData(int sidNum, bool isReceiving);
	virtual void OnRegisterEdited();

	// VICE keeps its own bulk register store: it holds the sound-engine mutex
	// and sets c64d_skip_sound_run_sound_in_sound_store across the restore, so
	// a whole register file can be poked without a burst of audio artefacts.
	CSidData *sidData;
	virtual bool ImportSidRegs(CSlrString *path);
	virtual bool ExportSidRegs(CSlrString *path);
};

#endif
