#include "CGT2SidRegisters.h"

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsid.h"

// playroutine()'s filter state. gplay.c defines these at file scope with
// external linkage but declares them in no header, so they are named here.
extern unsigned char filterctrl;
extern unsigned char filtertype;
extern unsigned char filtercutoff;
extern unsigned char filterptr;
}

// Voice registers occupy $00..$14 as three 7-byte blocks; $15..$18 are the
// shared filter/volume registers.
#define GT2_VOICE_REG_END 0x15

static bool GT2SidRegisterToVoice(int registerNum, int *voiceOut, int *offsetOut)
{
	if (registerNum < 0 || registerNum >= GT2_VOICE_REG_END)
		return false;

	int voice = registerNum / 7;
	if (voice >= MAX_CHN)
		return false;

	*voiceOut = voice;
	*offsetOut = registerNum % 7;
	return true;
}

u8 GT2_GetSidRegister(int registerNum)
{
	if (registerNum < 0 || registerNum >= GT2_NUM_SID_REGISTERS)
		return 0;

	return sidreg[registerNum];
}

void GT2_SetSidRegister(int registerNum, u8 value)
{
	if (registerNum < 0 || registerNum >= GT2_NUM_SID_REGISTERS)
		return;

	int voice, offset;
	if (GT2SidRegisterToVoice(registerNum, &voice, &offset))
	{
		CHN *cptr = &chn[voice];

		switch (offset)
		{
			case 0x00:
				cptr->freq = (cptr->freq & 0xff00) | value;
				break;
			case 0x01:
				cptr->freq = (cptr->freq & 0x00ff) | ((unsigned short)value << 8);
				break;
			case 0x02:
				// playroutine() emits pulse & 0xfe, so bit 0 is not
				// representable; keep the stored value consistent with what
				// will actually be heard.
				cptr->pulse = (cptr->pulse & 0xff00) | (value & 0xfe);
				break;
			case 0x03:
				cptr->pulse = (cptr->pulse & 0x00ff) | ((unsigned short)value << 8);
				break;
			case 0x04:
				// sidreg = wave & gate, where gate is a MASK: 0xff passes the
				// waveform through, 0xfe forces the gate bit low on keyoff.
				// Opening the mask makes the typed value reach the SID intact.
				cptr->wave = value;
				cptr->gate = 0xff;
				break;
			case 0x05:
			case 0x06:
				// ADSR: playroutine() writes these only on note init, hard
				// restart or an ADSR command, never once per frame, so the
				// register file itself is the durable home for the value.
				sidreg[registerNum] = value;
				break;
			default:
				break;
		}
		return;
	}

	switch (registerNum)
	{
		case 0x15:
			// playroutine() hard-codes sidreg[0x15] = 0x00 every frame; GT2
			// does not use the filter cutoff low bits at all.
			break;
		case 0x16:
			filtercutoff = value;
			break;
		case 0x17:
			filterctrl = value;
			break;
		case 0x18:
			filtertype = value & 0xf0;
			masterfader = value & 0x0f;
			break;
		default:
			break;
	}
}

bool GT2_IsSidRegisterWriteEffective(int registerNum)
{
	if (registerNum < 0 || registerNum >= GT2_NUM_SID_REGISTERS)
		return false;

	int voice, offset;
	if (GT2SidRegisterToVoice(registerNum, &voice, &offset))
	{
		// A muted channel has playroutine() emit only the test bit and skip
		// the frequency/pulse/waveform writes entirely (gplay.c:1179-1180),
		// so nothing typed into $00..$04 can be heard.
		if (chn[voice].mute && offset <= 0x04)
			return false;
		return true;
	}

	// The cutoff low byte is never emitted, and a running filter table
	// rebuilds cutoff/ctrl/type every frame from the table data.
	if (registerNum == 0x15)
		return false;
	if (registerNum >= 0x16 && registerNum <= 0x18 && filterptr != 0)
		return false;

	return true;
}
