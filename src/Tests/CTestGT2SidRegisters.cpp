#include "CTestGT2SidRegisters.h"
#include "CGT2SidRegisters.h"
#include "CViewGT2StateSID.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CAudioChannelGoatTracker.h"
#include "SYS_Funct.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsid.h"

extern unsigned char filterctrl;
extern unsigned char filtertype;
extern unsigned char filtercutoff;
extern unsigned char filterptr;
}

// Everything a step here can disturb, so the rest of the suite still sees the
// editor exactly as this test found it.
struct GT2SidRegBackup
{
	CHN channels[MAX_CHN];
	unsigned char sidRegs[GT2_NUM_SID_REGISTERS];
	unsigned char filterCtrl;
	unsigned char filterType;
	unsigned char filterCutoff;
	unsigned char filterPtr;
	unsigned char masterFader;
	unsigned char voiceMute[3];
	int songInit;
};

static void GT2SidRegSave(GT2SidRegBackup *b)
{
	memcpy(b->channels, chn, sizeof(b->channels));
	memcpy(b->sidRegs, sidreg, sizeof(b->sidRegs));
	b->filterCtrl = filterctrl;
	b->filterType = filtertype;
	b->filterCutoff = filtercutoff;
	b->filterPtr = filterptr;
	b->masterFader = masterfader;
	for (int v = 0; v < 3; v++)
		b->voiceMute[v] = gt2_voice_mute[v];
	b->songInit = songinit;
}

static void GT2SidRegRestore(const GT2SidRegBackup *b)
{
	memcpy(chn, b->channels, sizeof(b->channels));
	memcpy(sidreg, b->sidRegs, sizeof(b->sidRegs));
	filterctrl = b->filterCtrl;
	filtertype = b->filterType;
	filtercutoff = b->filterCutoff;
	filterptr = b->filterPtr;
	masterfader = b->masterFader;
	for (int v = 0; v < 3; v++)
		gt2_voice_mute[v] = b->voiceMute[v];
	songinit = b->songInit;
}

// Puts every channel in a state where one playroutine() pass is fully
// deterministic. Called again before each pass, because the tick counter
// counts down and would otherwise reach TICK0 part-way through the test --
// at which point sequencer() would advance the song and hand out new notes.
//
//  - tick > 1 keeps the pass on the WAVEEXEC path;
//  - a zero wavetable/pulsetable pointer skips both table interpreters
//    (gplay.c:728 guards on cptr->ptr[WTBL]), which would otherwise rewrite
//    wave, freq and pulse from table data;
//  - no pending command means vibrato and portamento leave freq alone;
//  - unmuted, because a muted channel emits only the test bit.
//
// It deliberately touches none of freq / pulse / wave / gate -- those are the
// ghost variables under test.
static void GT2SidRegQuiesceChannels()
{
	for (int v = 0; v < MAX_CHN; v++)
	{
		chn[v].mute = 0;
		chn[v].tick = 8;
		chn[v].tempo = 6;
		chn[v].ptr[WTBL] = 0;
		chn[v].ptr[PTBL] = 0;
		chn[v].command = 0;
		chn[v].cmddata = 0;
		chn[v].newcommand = 0;
		chn[v].newcmddata = 0;
		chn[v].newnote = 0;
		chn[v].vibtime = 0;
		chn[v].vibdelay = 0;
		chn[v].gatetimer = 0;
	}
}

void CTestGT2SidRegisters::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	// Headless runs do not stand the plugin up on their own.
	if (pluginGoatTracker == NULL)
	{
		PLUGIN_GoatTrackerInit();
	}

	if (pluginGoatTracker == NULL)
	{
		TestCompleted(false, "GoatTracker plugin could not be initialized");
		return;
	}

	// The plugin's audio thread calls playroutine() on the same chn[] and
	// sidreg[] globals this test drives; without stopping it, a step can see
	// the player's own writes interleaved with its own.
	bool audioWasPlaying = false;
	if (pluginGoatTracker->audioChannel)
	{
		audioWasPlaying = true;
		pluginGoatTracker->audioChannel->Stop();
		SYS_Sleep(50);
	}

	GT2SidRegBackup backup;
	GT2SidRegSave(&backup);

	// A stopped song with no filter program running is the state in which a
	// manual register edit is supposed to be at its most durable.
	songinit = PLAY_STOPPED;
	filterptr = 0;

	GT2SidRegQuiesceChannels();

	int step = 0;
	char msg[512];

	#define GT2SIDREG_FAIL_IF(cond, text) \
		if (cond) { GT2SidRegRestore(&backup); \
			if (audioWasPlaying && pluginGoatTracker->audioChannel) pluginGoatTracker->audioChannel->Start(); \
			TestCompleted(false, text); return; }

	// --- 1: reads come straight from the live register file ---
	step++;
	{
		sidreg[0x04] = 0x41;
		sidreg[0x18] = 0x0f;
		bool ok = GT2_GetSidRegister(0x04) == 0x41
			   && GT2_GetSidRegister(0x18) == 0x0f;

		// Out-of-range must be inert rather than reading past the array.
		ok = ok && GT2_GetSidRegister(-1) == 0
				&& GT2_GetSidRegister(GT2_NUM_SID_REGISTERS) == 0;

		snprintf(msg, sizeof(msg), "read $04=%02x $18=%02x", GT2_GetSidRegister(0x04), GT2_GetSidRegister(0x18));
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 2: a frequency write survives playroutine() ---
	step++;
	{
		GT2_SetSidRegister(0x00, 0x34);
		GT2_SetSidRegister(0x01, 0x12);

		bool ghostOk = (chn[0].freq == 0x1234);

		GT2SidRegQuiesceChannels();
		playroutine();

		bool survived = (sidreg[0x00] == 0x34 && sidreg[0x01] == 0x12);

		bool ok = ghostOk && survived;
		snprintf(msg, sizeof(msg), "chn0.freq=%04x after playroutine $00=%02x $01=%02x",
				 chn[0].freq, sidreg[0x00], sidreg[0x01]);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 3: poking sidreg[] directly does NOT survive -- the reason the
	//        ghost mapping has to exist at all ---
	step++;
	{
		sidreg[0x00] = 0xAA;
		sidreg[0x01] = 0xBB;

		GT2SidRegQuiesceChannels();
		playroutine();

		// The player rebuilt both bytes from chn[0].freq, which step 2 left
		// at 0x1234, so the direct poke is gone.
		bool clobbered = (sidreg[0x00] == 0x34 && sidreg[0x01] == 0x12);

		snprintf(msg, sizeof(msg), "direct poke overwritten: $00=%02x $01=%02x (expected 34/12)",
				 sidreg[0x00], sidreg[0x01]);
		StepCompleted(step, clobbered, msg);
		GT2SIDREG_FAIL_IF(!clobbered, msg);
	}

	// --- 4: pulse width, including the bit the player masks off ---
	step++;
	{
		GT2_SetSidRegister(0x02, 0xFF);
		GT2_SetSidRegister(0x03, 0x0A);

		GT2SidRegQuiesceChannels();
		playroutine();

		// playroutine() emits pulse & 0xfe, so bit 0 can never be set.
		bool ok = (sidreg[0x02] == 0xFE && sidreg[0x03] == 0x0A);
		snprintf(msg, sizeof(msg), "pulse $02=%02x (expected fe) $03=%02x", sidreg[0x02], sidreg[0x03]);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 5: the control register, where gate is a mask and not a flag ---
	step++;
	{
		// Start from a closed gate so the write has to open the mask itself.
		chn[0].gate = 0xfe;
		GT2_SetSidRegister(0x04, 0x41);   // pulse + gate on

		GT2SidRegQuiesceChannels();
		playroutine();

		bool ok = (sidreg[0x04] == 0x41);
		snprintf(msg, sizeof(msg), "ctrl $04=%02x (expected 41) wave=%02x gate=%02x",
				 sidreg[0x04], chn[0].wave, chn[0].gate);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 6: ADSR is event-driven, so the register file itself holds it ---
	step++;
	{
		GT2_SetSidRegister(0x05, 0x2A);
		GT2_SetSidRegister(0x06, 0xF8);

		GT2SidRegQuiesceChannels();
		playroutine();

		bool ok = (sidreg[0x05] == 0x2A && sidreg[0x06] == 0xF8);
		snprintf(msg, sizeof(msg), "adsr $05=%02x $06=%02x", sidreg[0x05], sidreg[0x06]);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 7: voices 2 and 3 land in their own 7-byte blocks ---
	step++;
	{
		GT2_SetSidRegister(0x07, 0x11);   // voice 2 freq lo
		GT2_SetSidRegister(0x0E, 0x22);   // voice 3 freq lo

		bool ghostOk = ((chn[1].freq & 0xff) == 0x11)
					&& ((chn[2].freq & 0xff) == 0x22);

		GT2SidRegQuiesceChannels();
		playroutine();

		bool ok = ghostOk && sidreg[0x07] == 0x11 && sidreg[0x0E] == 0x22;
		snprintf(msg, sizeof(msg), "voice2 $07=%02x voice3 $0E=%02x", sidreg[0x07], sidreg[0x0E]);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 8: filter cutoff / resonance / mode+volume ---
	step++;
	{
		GT2_SetSidRegister(0x16, 0x80);   // cutoff high
		GT2_SetSidRegister(0x17, 0xF1);   // resonance + routing
		GT2_SetSidRegister(0x18, 0x1C);   // low-pass + volume 12

		GT2SidRegQuiesceChannels();
		playroutine();

		bool ok = sidreg[0x16] == 0x80
			   && sidreg[0x17] == 0xF1
			   && sidreg[0x18] == 0x1C
			   && filtertype == 0x10
			   && masterfader == 0x0C;
		snprintf(msg, sizeof(msg), "$16=%02x $17=%02x $18=%02x filtertype=%02x masterfader=%02x",
				 sidreg[0x16], sidreg[0x17], sidreg[0x18], filtertype, masterfader);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 9: $15 is never emitted, and the mapping says so ---
	step++;
	{
		GT2_SetSidRegister(0x15, 0x07);

		GT2SidRegQuiesceChannels();
		playroutine();

		// playroutine() hard-codes it to zero every frame.
		bool zeroed = (sidreg[0x15] == 0x00);
		bool reportedIneffective = !GT2_IsSidRegisterWriteEffective(0x15);

		bool ok = zeroed && reportedIneffective;
		snprintf(msg, sizeof(msg), "$15=%02x (expected 00) reportedIneffective=%d",
				 sidreg[0x15], reportedIneffective);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 10: a running filter table makes filter writes ineffective ---
	step++;
	{
		bool effectiveWhenIdle = GT2_IsSidRegisterWriteEffective(0x16)
							  && GT2_IsSidRegisterWriteEffective(0x17)
							  && GT2_IsSidRegisterWriteEffective(0x18);

		filterptr = 1;
		bool ineffectiveWhenRunning = !GT2_IsSidRegisterWriteEffective(0x16)
								   && !GT2_IsSidRegisterWriteEffective(0x17)
								   && !GT2_IsSidRegisterWriteEffective(0x18);
		filterptr = 0;

		bool ok = effectiveWhenIdle && ineffectiveWhenRunning;
		snprintf(msg, sizeof(msg), "effectiveWhenIdle=%d ineffectiveWhenRunning=%d",
				 effectiveWhenIdle, ineffectiveWhenRunning);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 11: a muted channel cannot show a typed value ---
	step++;
	{
		chn[1].mute = 1;

		// $07..$0B belong to voice 2; ADSR still applies because the player
		// leaves those alone whether the channel is muted or not.
		bool voiceRegsIneffective = !GT2_IsSidRegisterWriteEffective(0x07)
								 && !GT2_IsSidRegisterWriteEffective(0x0B);
		bool adsrStillEffective = GT2_IsSidRegisterWriteEffective(0x0C);
		bool otherVoiceUnaffected = GT2_IsSidRegisterWriteEffective(0x00);

		chn[1].mute = 0;

		bool ok = voiceRegsIneffective && adsrStillEffective && otherVoiceUnaffected;
		snprintf(msg, sizeof(msg), "muted voiceRegs=%d adsrStillEffective=%d otherVoice=%d",
				 voiceRegsIneffective, adsrStillEffective, otherVoiceUnaffected);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 12: out-of-range writes are inert ---
	step++;
	{
		CHN before = chn[0];
		unsigned char cutoffBefore = filtercutoff;

		GT2_SetSidRegister(-1, 0xFF);
		GT2_SetSidRegister(GT2_NUM_SID_REGISTERS, 0xFF);
		GT2_SetSidRegister(0x1C, 0xFF);

		bool ok = memcmp(&before, &chn[0], sizeof(CHN)) == 0
			   && filtercutoff == cutoffBefore;
		snprintf(msg, sizeof(msg), "out-of-range writes inert=%d", ok);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	// --- 13: the view reports GT2's shape to the generic base ---
	step++;
	{
		CViewGT2StateSID *view = pluginGoatTracker->viewStateSID;
		bool exists = (view != NULL);

		bool ok = exists
			   && view->GetNumSids() == 1
			   && view->GetNumRegisters() == GT2_NUM_SID_REGISTERS
			   && view->GetSidBaseAddress(0) == 0xD400
			   && view->IsRegisterWritable();

		snprintf(msg, sizeof(msg), "view exists=%d numSids=%d numRegs=%d",
				 exists, exists ? view->GetNumSids() : -1,
				 exists ? view->GetNumRegisters() : -1);
		StepCompleted(step, ok, msg);
		GT2SIDREG_FAIL_IF(!ok, msg);
	}

	#undef GT2SIDREG_FAIL_IF

	GT2SidRegRestore(&backup);
	if (audioWasPlaying && pluginGoatTracker->audioChannel)
		pluginGoatTracker->audioChannel->Start();

	TestCompleted(true, "GT2 SID registers: ghost mapping survives playroutine(), effectiveness reported correctly");
}
