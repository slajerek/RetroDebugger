#include "CTestGT2ArpGate.h"
#include "SYS_Main.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CAudioChannelGoatTracker.h"
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsong.h"
#include "gsid.h"
#include "gpattern.h"
#include "goattrk2.h"
}

// ---------------------------------------------------------------------
// What this test pins down (2026-09-07, both reported in one session):
//
//  1. Jam mode, cursor in an arp column: typing a note re-plays the row's
//     base note through playtestnote(). The base note must get its normal
//     hard restart — gate off + HR ADSR for `gatetimer` ticks — exactly as
//     it does without arp columns. The old "trigger instrument when arp
//     notes exist but the channel is silent" block re-raised the gate in
//     the very tick playtestnote() dropped it, so the SID never saw a gate
//     edge. Combined with the HR ADSR (AD=$0F/SR=$00) reaching a gate-on
//     voice through the audio mixer's register flush, reSID's envelope
//     decayed to zero and held there (`hold_zero`) until a real gate edge:
//     the channel went silent until SPACE reset the channels.
//
//  2. ENTER (triggerpatternrow) on a row with a base note while arp notes
//     are active must hard-restart the channel like any other new note.
//
//  3. Song mode: KEYOFF in the main track releases the gate (ADSR release)
//     but the arp cycle keeps running — base note included — until the
//     arp columns are cleared or a new base note arrives.
//
//  4. A plain note with no arp columns takes its frequency from the
//     wavetable like stock GT2: a wavetable row with a relative note
//     offset (+12) must be audible, not overwritten by the arp code's
//     per-tick frequency write.
// ---------------------------------------------------------------------

namespace
{

const int NOTE_C4 = 48;
const int NOTE_A4 = 57;
const int NOTE_C5 = 60;
const int NOTE_D5 = 62;
const int NOTE_F5 = 65;

#define SILENT_PATT_A 2
#define SILENT_PATT_B 3

struct SavedState
{
	std::vector<unsigned char> pattern, arpdata, ltable, rtable, songorder;
	std::vector<unsigned char> ginstr, chn, sidreg;
	int pattlen[MAX_PATT];
	int songlen[MAX_SONGS][MAX_CHN];
	int epnum[MAX_CHN];
	int numarpcolumns, songinit, lastsonginit, followplay;
	unsigned adparam;

	void Save()
	{
		pattern.assign((unsigned char *)::pattern, (unsigned char *)::pattern + sizeof ::pattern);
		arpdata.assign((unsigned char *)::arpdata, (unsigned char *)::arpdata + sizeof ::arpdata);
		ltable.assign((unsigned char *)::ltable, (unsigned char *)::ltable + sizeof ::ltable);
		rtable.assign((unsigned char *)::rtable, (unsigned char *)::rtable + sizeof ::rtable);
		songorder.assign((unsigned char *)::songorder, (unsigned char *)::songorder + sizeof ::songorder);
		ginstr.assign((unsigned char *)::ginstr, (unsigned char *)::ginstr + sizeof ::ginstr);
		chn.assign((unsigned char *)::chn, (unsigned char *)::chn + sizeof ::chn);
		sidreg.assign(::sidreg, ::sidreg + NUMSIDREGS);
		memcpy(pattlen, ::pattlen, sizeof pattlen);
		memcpy(songlen, ::songlen, sizeof songlen);
		memcpy(epnum, ::epnum, sizeof epnum);
		numarpcolumns = ::numarpcolumns;
		songinit = ::songinit;
		lastsonginit = ::lastsonginit;
		followplay = ::followplay;
		adparam = ::adparam;
	}

	void Restore()
	{
		memcpy(::pattern, pattern.data(), sizeof ::pattern);
		memcpy(::arpdata, arpdata.data(), sizeof ::arpdata);
		memcpy(::ltable, ltable.data(), sizeof ::ltable);
		memcpy(::rtable, rtable.data(), sizeof ::rtable);
		memcpy(::songorder, songorder.data(), sizeof ::songorder);
		memcpy(::ginstr, ginstr.data(), sizeof ::ginstr);
		memcpy(::chn, chn.data(), sizeof ::chn);
		memcpy(::sidreg, sidreg.data(), NUMSIDREGS);
		memcpy(::pattlen, pattlen, sizeof pattlen);
		memcpy(::songlen, songlen, sizeof songlen);
		memcpy(::epnum, epnum, sizeof epnum);
		::numarpcolumns = numarpcolumns;
		::songinit = songinit;
		::lastsonginit = lastsonginit;
		::followplay = followplay;
		::adparam = adparam;
	}
};

unsigned short freq_of(int noteIndex)
{
	return (unsigned short)(freqtbllo[noteIndex] | (freqtblhi[noteIndex] << 8));
}

unsigned short sid_freq(int channel)
{
	return (unsigned short)(sidreg[0x00 + 7 * channel] | (sidreg[0x01 + 7 * channel] << 8));
}

bool sid_gate_on(int channel)
{
	return (sidreg[0x04 + 7 * channel] & 0x01) != 0;
}

bool sid_adsr_is(int channel, unsigned char ad, unsigned char sr)
{
	return sidreg[0x05 + 7 * channel] == ad && sidreg[0x06 + 7 * channel] == sr;
}

void fill_rest_pattern(int patt, int rows)
{
	memset(pattern[patt], 0, sizeof pattern[patt]);
	for (int r = 0; r < rows; r++)
		pattern[patt][r * 4] = REST;
	pattern[patt][rows * 4] = ENDPATT;
	pattlen[patt] = rows;
}

// Instrument 1: the user's report. AD $31, SR $F6, first wave $09
// (test+gate), hard restart with gatetimer 2, wavetable at position 3:
//   3: 11 00   triangle+gate, play the current note
//   4: FF 00   end (jump to 0 stops the wavetable)
// Instrument 2: same shell, wavetable at position 5 with a relative +12:
//   5: 11 0C
//   6: FF 05   loop back to 5
void build_song(void)
{
	memset(pattern, 0, sizeof pattern);
	memset(arpdata, 0, sizeof arpdata);
	memset(ginstr, 0, sizeof ginstr);
	memset(songorder, 0, sizeof songorder);
	memset(ltable, 0, sizeof ltable);
	memset(rtable, 0, sizeof rtable);
	for (int p = 0; p < MAX_PATT; p++)
		pattlen[p] = 0;
	for (int s = 0; s < MAX_SONGS; s++)
		for (int c = 0; c < MAX_CHN; c++)
			songlen[s][c] = 0;

	INSTR *i1 = &ginstr[1];
	i1->ad = 0x31;
	i1->sr = 0xf6;
	i1->firstwave = 0x09;
	i1->gatetimer = 0x02;
	i1->ptr[WTBL] = 3;
	ltable[WTBL][2] = 0x11; rtable[WTBL][2] = 0x00;
	ltable[WTBL][3] = 0xff; rtable[WTBL][3] = 0x00;

	INSTR *i2 = &ginstr[2];
	*i2 = *i1;
	i2->ptr[WTBL] = 5;
	ltable[WTBL][4] = 0x11; rtable[WTBL][4] = 0x0c;
	ltable[WTBL][5] = 0xff; rtable[WTBL][5] = 0x05;

	adparam = 0x0f00;
}

void start_jam(void)
{
	initchannels();
	songinit = PLAY_STOPPED;
	memset(sidreg, 0, NUMSIDREGS);
}

// Runs `ticks` playroutine() calls, ORing which of the given note
// frequencies showed up on `channel`. Returns a bitmask over `notes`.
unsigned run_collect(int channel, int ticks, const int *notes, int noteCount)
{
	unsigned seen = 0;
	for (int t = 0; t < ticks; t++)
	{
		playroutine();
		unsigned short f = sid_freq(channel);
		for (int n = 0; n < noteCount; n++)
			if (f == freq_of(notes[n])) seen |= (1u << n);
	}
	return seen;
}

} // namespace

void CTestGT2ArpGate::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;

	static char msg[512];
	SavedState saved;
	saved.Save();

	// The plugin's mixer thread calls playroutine() on the same globals.
	bool audioWasPlaying = false;
	if (pluginGoatTracker && pluginGoatTracker->audioChannel)
	{
		audioWasPlaying = true;
		pluginGoatTracker->audioChannel->Stop();
		SYS_Sleep(50);
	}

	bool ok = true;
	msg[0] = '\0';

#define FAIL(...) do { snprintf(msg, sizeof msg, __VA_ARGS__); ok = false; } while (0)

	// ---- Case 1: jam mode, base note re-played while an arp column is active ----
	build_song();
	numarpcolumns = 1;
	start_jam();

	playtestnote(FIRSTNOTE + NOTE_A4, 1, 1);
	for (int t = 0; t < 4; t++) playroutine();   // 3 ticks to TICK0, 1 wavetable tick
	if (ok && !(sidreg[0x0b] == 0x11 && sid_adsr_is(1, 0x31, 0xf6) && sid_freq(1) == freq_of(NOTE_A4)))
		FAIL("case 1 setup: plain A-4 not sounding (wave=$%02X ad=$%02X sr=$%02X freq=$%04X)",
		     sidreg[0x0b], sidreg[0x0c], sidreg[0x0d], sid_freq(1));

	// The editor's arp-column entry: cache the arp note, re-play the base.
	chn[1].arpcolnotes[0] = NOTE_C5;
	playtestnote(FIRSTNOTE + NOTE_A4, 1, 1);
	for (int t = 0; ok && t < 2; t++)
	{
		playroutine();
		if (sid_gate_on(1) || !sid_adsr_is(1, 0x0f, 0x00))
			FAIL("case 1: HR window tick %d under arp — gate bit %d (want 0), ad/sr=$%02X/$%02X (want $0F/$00)",
			     t, sid_gate_on(1) ? 1 : 0, sidreg[0x0c], sidreg[0x0d]);
	}
	if (ok)
	{
		playroutine();   // TICK0: instrument reload, gate on
		if (!sid_gate_on(1) || !sid_adsr_is(1, 0x31, 0xf6))
			FAIL("case 1: TICK0 under arp — gate bit %d (want 1), ad/sr=$%02X/$%02X (want $31/$F6)",
			     sid_gate_on(1) ? 1 : 0, sidreg[0x0c], sidreg[0x0d]);
	}
	if (ok)
	{
		const int notes[2] = { NOTE_A4, NOTE_C5 };
		unsigned seen = run_collect(1, 4, notes, 2);
		if (seen != 0x3)
			FAIL("case 1: after TICK0 the cycle must alternate A-4/C-5, seen mask=%u", seen);
		if (!sid_gate_on(1))
			FAIL("case 1: gate dropped while cycling");
	}

	// ---- Case 2: ENTER (triggerpatternrow) on a base-note row with arp active ----
	if (ok)
	{
		fill_rest_pattern(1, 8);
		fill_rest_pattern(SILENT_PATT_A, 8);
		fill_rest_pattern(SILENT_PATT_B, 8);
		pattern[1][0] = FIRSTNOTE + NOTE_A4;
		pattern[1][1] = 1;
		arpdata[1][1][0][0] = FIRSTNOTE + NOTE_C5;
		epnum[0] = SILENT_PATT_A;
		epnum[1] = 1;
		epnum[2] = SILENT_PATT_B;

		triggerpatternrow(0);
		if (chn[1].gate != 0xfe || !sid_adsr_is(1, 0x0f, 0x00) || chn[1].newnote != FIRSTNOTE + NOTE_A4)
			FAIL("case 2: ENTER under arp — gate=$%02X (want $FE), ad/sr=$%02X/$%02X (want $0F/$00), newnote=$%02X",
			     chn[1].gate, sidreg[0x0c], sidreg[0x0d], chn[1].newnote);
	}
	for (int t = 0; ok && t < 2; t++)
	{
		playroutine();
		if (sid_gate_on(1))
			FAIL("case 2: HR window tick %d — gate bit set", t);
	}
	if (ok)
	{
		playroutine();   // TICK0
		const int notes[2] = { NOTE_A4, NOTE_C5 };
		unsigned seen = run_collect(1, 4, notes, 2);
		if (!sid_gate_on(1) || !sid_adsr_is(1, 0x31, 0xf6) || seen != 0x3)
			FAIL("case 2: after TICK0 — gate bit %d, ad/sr=$%02X/$%02X, seen mask=%u (want 1, $31/$F6, 3)",
			     sid_gate_on(1) ? 1 : 0, sidreg[0x0c], sidreg[0x0d], seen);
	}

	// ---- Case 3: song mode, KEYOFF in the main track keeps the arp cycling in release ----
	if (ok)
	{
		build_song();
		numarpcolumns = 2;
		fill_rest_pattern(0, 8);
		fill_rest_pattern(SILENT_PATT_A, 8);
		pattern[0][0] = FIRSTNOTE + NOTE_F5;
		pattern[0][1] = 1;
		arpdata[0][0][0][0] = FIRSTNOTE + NOTE_C5;
		arpdata[0][0][0][1] = FIRSTNOTE + NOTE_D5;
		pattern[0][4] = KEYOFF;
		for (int c = 0; c < MAX_CHN; c++)
		{
			songorder[0][c][0] = (c == 0) ? 0 : SILENT_PATT_A;
			songorder[0][c][1] = 0xff;
			songorder[0][c][2] = 0x00;
			songlen[0][c] = 1;
		}
		memset(sidreg, 0, NUMSIDREGS);
		initchannels();
		initsong(0, PLAY_BEGINNING);

		// Row 0 must sound as a 3-note chord first.
		const int chord[3] = { NOTE_F5, NOTE_C5, NOTE_D5 };
		unsigned seen = 0;
		int t;
		for (t = 0; t < 40 && chn[0].pattptr < 8; t++)
		{
			playroutine();
			if (chn[0].gate == 0xff)
			{
				unsigned short f = sid_freq(0);
				for (int n = 0; n < 3; n++)
					if (f == freq_of(chord[n])) seen |= (1u << n);
			}
		}
		if (chn[0].pattptr < 8)
			FAIL("case 3: KEYOFF row never fetched (pattptr=%u after %d ticks)", chn[0].pattptr, t);
		else if (seen != 0x7)
			FAIL("case 3: row 0 chord incomplete before KEYOFF, seen mask=%u (want 7)", seen);
		else if (chn[0].gate != 0xfe)
			FAIL("case 3: KEYOFF fetched but gate=$%02X (want $FE — ADSR release)", chn[0].gate);

		if (ok)
		{
			seen = 0;
			for (t = 0; t < 6; t++)
			{
				playroutine();
				if (sid_gate_on(0))
				{
					FAIL("case 3: gate bit set %d ticks after KEYOFF (want release)", t + 1);
					break;
				}
				unsigned short f = sid_freq(0);
				for (int n = 0; n < 3; n++)
					if (f == freq_of(chord[n])) seen |= (1u << n);
			}
			if (ok && seen != 0x7)
				FAIL("case 3: arp must keep cycling F-5/C-5/D-5 during release, seen mask=%u (want 7)", seen);
		}
	}

	// ---- Case 4: plain note honours a wavetable relative offset ----
	if (ok)
	{
		build_song();
		numarpcolumns = 0;
		start_jam();
		playtestnote(FIRSTNOTE + NOTE_C4, 2, 0);
		for (int t = 0; t < 4; t++) playroutine();   // TICK0 at 3, wavetable row "11 0C" at 4
		if (sid_freq(0) != freq_of(NOTE_C4 + 12))
			FAIL("case 4: wavetable +12 offset lost — freq=$%04X (want $%04X for C-5, base C-4 is $%04X)",
			     sid_freq(0), freq_of(NOTE_C4 + 12), freq_of(NOTE_C4));
		playroutine();
		if (ok && sid_freq(0) != freq_of(NOTE_C4 + 12))
			FAIL("case 4: wavetable +12 offset lost on the loop tick — freq=$%04X", sid_freq(0));
	}

#undef FAIL

	saved.Restore();
	if (audioWasPlaying && pluginGoatTracker && pluginGoatTracker->audioChannel)
		pluginGoatTracker->audioChannel->Start();

	if (ok)
		TestCompleted(true, "GT2 arp gate ownership: 4/4 cases passed");
	else
		TestCompleted(false, msg);
}

void CTestGT2ArpGate::Cancel()
{
	isRunning = false;
}
