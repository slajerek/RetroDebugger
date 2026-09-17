#include "CTestGT2SongSettings.h"
#include "CViewGT2SongSettings.h"
#include "CGT2UndoHistory.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "GT2ViewCommon.h"
#include "SYS_Funct.h"
#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "gplay.h"
#include "goattrk2.h"

extern unsigned residdelay;
extern unsigned customclockrate;
extern float equaldivisionsperoctave;
extern int tuningcount;
extern char scalatuningfilepath[];
extern char specialnotenames[186];
}

#define GT2_FREQTABLE_NOTES 96

// Everything this test writes to, so a failure part-way through still leaves
// the editor exactly as it found it.
struct GT2SettingsBackup
{
	char scalaPath[MAX_PATHNAME];
	char noteNames[186];
	char songName[MAX_STR];
	char authorName[MAX_STR];
	char copyrightName[MAX_STR];
	unsigned sidModel;
	unsigned ntscFlag;
	unsigned speedMultiplier;
	unsigned customClock;
	unsigned hardRestart;
	unsigned fineVibrato;
	unsigned useFineVibrato;
	unsigned pulseOpt;
	unsigned realtimeOpt;
	unsigned sidAddr;
	unsigned interpolation;
	unsigned writeDelay;
	unsigned keyPreset;
	float basePitchValue;
	float divisionsValue;
	int tuningSteps;
	unsigned char reservedInstrumentAd;
	unsigned char reservedInstrumentWtbl;
	// GT2_SetSongTempo() writes the live channels too, so restoring only the
	// reserved slot would leave every later test in the suite playing at this
	// test's last tempo.
	unsigned char channelTempo[MAX_CHN];
	unsigned char freqLo[GT2_FREQTABLE_NOTES];
	unsigned char freqHi[GT2_FREQTABLE_NOTES];
};

static void GT2SettingsSave(GT2SettingsBackup *b)
{
	strncpy(b->scalaPath, scalatuningfilepath, MAX_PATHNAME);
	// readscalatuningfile() rewrites this whether or not the file names its
	// degrees, so it has to come back too.
	memcpy(b->noteNames, specialnotenames, sizeof(b->noteNames));
	strncpy(b->songName, songname, MAX_STR);
	strncpy(b->authorName, authorname, MAX_STR);
	strncpy(b->copyrightName, copyrightname, MAX_STR);
	b->sidModel = sidmodel;
	b->ntscFlag = ntsc;
	b->speedMultiplier = multiplier;
	b->customClock = customclockrate;
	b->hardRestart = adparam;
	b->fineVibrato = finevibrato;
	b->useFineVibrato = usefinevib;
	b->pulseOpt = optimizepulse;
	b->realtimeOpt = optimizerealtime;
	b->sidAddr = sidaddress;
	b->interpolation = interpolate;
	b->writeDelay = residdelay;
	b->keyPreset = keypreset;
	b->basePitchValue = basepitch;
	b->divisionsValue = equaldivisionsperoctave;
	b->tuningSteps = tuningcount;
	b->reservedInstrumentAd = ginstr[MAX_INSTR-1].ad;
	b->reservedInstrumentWtbl = ginstr[MAX_INSTR-1].ptr[WTBL];
	for (int c = 0; c < MAX_CHN; c++)
		b->channelTempo[c] = chn[c].tempo;
	memcpy(b->freqLo, freqtbllo, GT2_FREQTABLE_NOTES);
	memcpy(b->freqHi, freqtblhi, GT2_FREQTABLE_NOTES);
}

static void GT2SettingsRestore(const GT2SettingsBackup *b)
{
	strncpy(scalatuningfilepath, b->scalaPath, MAX_PATHNAME);
	memcpy(specialnotenames, b->noteNames, sizeof(b->noteNames));
	strncpy(songname, b->songName, MAX_STR);
	strncpy(authorname, b->authorName, MAX_STR);
	strncpy(copyrightname, b->copyrightName, MAX_STR);
	sidmodel = b->sidModel;
	ntsc = b->ntscFlag;
	multiplier = b->speedMultiplier;
	customclockrate = b->customClock;
	adparam = b->hardRestart;
	finevibrato = b->fineVibrato;
	usefinevib = b->useFineVibrato;
	optimizepulse = b->pulseOpt;
	optimizerealtime = b->realtimeOpt;
	sidaddress = b->sidAddr;
	interpolate = b->interpolation;
	residdelay = b->writeDelay;
	keypreset = b->keyPreset;
	basepitch = b->basePitchValue;
	equaldivisionsperoctave = b->divisionsValue;
	tuningcount = b->tuningSteps;
	ginstr[MAX_INSTR-1].ad = b->reservedInstrumentAd;
	ginstr[MAX_INSTR-1].ptr[WTBL] = b->reservedInstrumentWtbl;
	for (int c = 0; c < MAX_CHN; c++)
		chn[c].tempo = b->channelTempo[c];
	memcpy(freqtbllo, b->freqLo, GT2_FREQTABLE_NOTES);
	memcpy(freqtblhi, b->freqHi, GT2_FREQTABLE_NOTES);

	// Restoring the globals is not enough. Every setter that touches the chip
	// model, the timing or the multiplier has already pushed those values into
	// reSID through gtsound_init(), which also latched gsound.c's `framerate`
	// and `snd_bpmtempo`. Without this, a suite run leaves the GT2 player
	// clocked at whatever this test finished with, and the next GT2 test --
	// ArpGate, ArpParity, the export tests -- inherits it.
	CViewGT2SongSettings::ApplySoundSettings();
}

void CTestGT2SongSettings::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	GT2SettingsBackup backup;
	GT2SettingsSave(&backup);

	int step = 0;
	char msg[512];

	// --- Test 1: the three .sng strings round-trip and stop at 31 characters ---
	step++;
	{
		CViewGT2SongSettings::SetSongName("MOON RIDER");
		CViewGT2SongSettings::SetAuthorName("COMPOSER 42");
		CViewGT2SongSettings::SetCopyrightName("2026 NOBODY");

		bool nameOk = CViewGT2SongSettings::GetSongName() == "MOON RIDER";
		bool authorOk = CViewGT2SongSettings::GetAuthorName() == "COMPOSER 42";
		bool copyrightOk = CViewGT2SongSettings::GetCopyrightName() == "2026 NOBODY";

		// MAX_STR is 32 bytes including the terminator, and the editor reads
		// these with strlen() -- an over-long name must be cut, not smeared
		// into whatever follows.
		const char *tooLong = "0123456789012345678901234567890123456789";
		CViewGT2SongSettings::SetSongName(tooLong);
		std::string clamped = CViewGT2SongSettings::GetSongName();
		bool clampedOk = (clamped.size() == MAX_STR - 1)
			&& (strncmp(clamped.c_str(), tooLong, MAX_STR - 1) == 0)
			&& (songname[MAX_STR - 1] == 0);

		bool ok = nameOk && authorOk && copyrightOk && clampedOk;
		snprintf(msg, sizeof(msg),
				 "name=%d author=%d copyright=%d clampedTo%d=%d",
				 nameOk, authorOk, copyrightOk, (int)MAX_STR - 1, clampedOk);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 2: renaming the song is undoable ---
	// The names are song data, so the rule the instrument-load fix established
	// holds for them too: Ctrl+Z takes back the last change, whatever kind it was.
	step++;
	{
		GT2UndoHistory()->Clear();
		CViewGT2SongSettings::SetSongName("BEFORE");

		CGT2UndoSnapshot before = GT2UndoHistory()->Capture();
		CViewGT2SongSettings::SetSongName("AFTER");
		bool recorded = GT2UndoHistory()->CommitIfChanged(before);

		bool undone = GT2UndoHistory()->Undo()
			&& CViewGT2SongSettings::GetSongName() == "BEFORE";
		bool redone = GT2UndoHistory()->Redo()
			&& CViewGT2SongSettings::GetSongName() == "AFTER";

		// A field that was focused and left alone must not cost an entry.
		CGT2UndoSnapshot unchanged = GT2UndoHistory()->Capture();
		bool noEntryWithoutChange = !GT2UndoHistory()->CommitIfChanged(unchanged);

		GT2UndoHistory()->Clear();

		bool ok = recorded && undone && redone && noEntryWithoutChange;
		snprintf(msg, sizeof(msg), "recorded=%d undone=%d redone=%d noEntryWhenUnchanged=%d",
				 recorded, undone, redone, noEntryWithoutChange);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 3: SID model and video standard ---
	step++;
	{
		CViewGT2SongSettings::SetSidModel(0);
		bool is6581 = CViewGT2SongSettings::GetSidModel() == 0 && sidmodel == 0;
		CViewGT2SongSettings::SetSidModel(1);
		bool is8580 = CViewGT2SongSettings::GetSidModel() == 1 && sidmodel == 1;
		// Anything non-zero is 8580; the global is a single bit downstream
		// (greloc.c ORs it into the SID header flags).
		CViewGT2SongSettings::SetSidModel(7);
		bool normalized = CViewGT2SongSettings::GetSidModel() == 1 && sidmodel == 1;

		CViewGT2SongSettings::SetNtsc(true);
		bool ntscOn = CViewGT2SongSettings::IsNtsc() && ntsc == 1;
		CViewGT2SongSettings::SetNtsc(false);
		bool palOn = !CViewGT2SongSettings::IsNtsc() && ntsc == 0;

		bool ok = is6581 && is8580 && normalized && ntscOn && palOn;
		snprintf(msg, sizeof(msg), "6581=%d 8580=%d normalized=%d ntsc=%d pal=%d",
				 is6581, is8580, normalized, ntscOn, palOn);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 4: custom SID clock rejects the values GT2's config reader rejects ---
	step++;
	{
		CViewGT2SongSettings::SetCustomClockRate(1000000);
		bool accepted = CViewGT2SongSettings::GetCustomClockRate() == 1000000;
		// goattrk2.c:414 turns anything under 100 into "use the default"
		// rather than clamping it up -- a clock that slow is a typo.
		CViewGT2SongSettings::SetCustomClockRate(99);
		bool rejectedLow = CViewGT2SongSettings::GetCustomClockRate() == 0;
		CViewGT2SongSettings::SetCustomClockRate(-5);
		bool rejectedNegative = CViewGT2SongSettings::GetCustomClockRate() == 0;
		CViewGT2SongSettings::SetCustomClockRate(0);
		bool zeroIsDefault = CViewGT2SongSettings::GetCustomClockRate() == 0;

		bool ok = accepted && rejectedLow && rejectedNegative && zeroIsDefault;
		snprintf(msg, sizeof(msg), "accepted=%d rejectedLow=%d rejectedNegative=%d zero=%d",
				 accepted, rejectedLow, rejectedNegative, zeroIsDefault);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 5: speed multiplier clamps, and carries fine vibrato with it ---
	step++;
	{
		CViewGT2SongSettings::SetSpeedMultiplier(100);
		bool clampedHigh = CViewGT2SongSettings::GetSpeedMultiplier() == GT2_MAX_SPEED_MULTIPLIER;
		CViewGT2SongSettings::SetSpeedMultiplier(-1);
		bool clampedLow = CViewGT2SongSettings::GetSpeedMultiplier() == 0;

		// goattrk2.c:409 applies fine vibrato below 2x when finevibrato is 1.
		// Moving the multiplier has to move the derived flag the same way a
		// restart would.
		finevibrato = 1;
		CViewGT2SongSettings::SetSpeedMultiplier(1);
		bool derivedOnAt1x = usefinevib == 1;
		CViewGT2SongSettings::SetSpeedMultiplier(4);
		bool derivedOffAt4x = usefinevib == 0;

		// With the setting off, the multiplier cannot switch it on.
		finevibrato = 0;
		CViewGT2SongSettings::SetSpeedMultiplier(1);
		bool staysOffWhenDisabled = usefinevib == 0;

		bool ok = clampedHigh && clampedLow && derivedOnAt1x && derivedOffAt4x
			&& staysOffWhenDisabled;
		snprintf(msg, sizeof(msg),
				 "clampedHigh=%d clampedLow=%d derivedOn1x=%d derivedOff4x=%d staysOffWhenDisabled=%d",
				 clampedHigh, clampedLow, derivedOnAt1x, derivedOffAt4x, staysOffWhenDisabled);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 6: tempo is the toolbar's tempo, not a second copy ---
	step++;
	{
		multiplier = 1;
		ginstr[MAX_INSTR-1].ptr[WTBL] = 0;

		CViewGT2SongSettings::SetSongTempo(9);
		bool storedInReservedSlot = ginstr[MAX_INSTR-1].ad == 9;
		bool agreesWithShared = CViewGT2SongSettings::GetSongTempo() == GT2_GetSongTempo()
			&& GT2_GetSongTempo() == 9;
		// The player runs at ad-1, so the live channels carry 8.
		bool reachedLiveChannels = chn[0].tempo == 8 && chn[1].tempo == 8 && chn[2].tempo == 8;

		CViewGT2SongSettings::SetSongTempo(1);
		bool clampedLow = CViewGT2SongSettings::GetSongTempo() == GT2_SONG_TEMPO_MIN;
		CViewGT2SongSettings::SetSongTempo(999);
		bool clampedHigh = CViewGT2SongSettings::GetSongTempo() == GT2_SONG_TEMPO_MAX;

		// A wavetable pointer in the reserved slot means it is a real
		// instrument again, not a tempo override.
		ginstr[MAX_INSTR-1].ptr[WTBL] = 1;
		bool overrideDisabled = CViewGT2SongSettings::GetSongTempo() == 6;
		ginstr[MAX_INSTR-1].ptr[WTBL] = 0;

		bool ok = storedInReservedSlot && agreesWithShared && reachedLiveChannels
			&& clampedLow && clampedHigh && overrideDisabled;
		snprintf(msg, sizeof(msg),
				 "reservedSlot=%d agreesWithToolbar=%d live=%d clampLow=%d clampHigh=%d wtblDisables=%d",
				 storedInReservedSlot, agreesWithShared, reachedLiveChannels,
				 clampedLow, clampedHigh, overrideDisabled);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 7: hard restart ADSR and SID base address are 16-bit ---
	step++;
	{
		CViewGT2SongSettings::SetHardRestartAdsr(0x0f00);
		bool defaultOk = CViewGT2SongSettings::GetHardRestartAdsr() == 0x0f00 && adparam == 0x0f00;
		CViewGT2SongSettings::SetHardRestartAdsr(0x12345);
		bool maskedOk = CViewGT2SongSettings::GetHardRestartAdsr() == 0x2345;

		CViewGT2SongSettings::SetSidAddress(0xd400);
		bool addrOk = CViewGT2SongSettings::GetSidAddress() == 0xd400 && sidaddress == 0xd400;
		CViewGT2SongSettings::SetSidAddress(0x1d420);
		bool addrMaskedOk = CViewGT2SongSettings::GetSidAddress() == 0xd420;

		bool ok = defaultOk && maskedOk && addrOk && addrMaskedOk;
		snprintf(msg, sizeof(msg), "hr=%d hrMasked=%d addr=%d addrMasked=%d",
				 defaultOk, maskedOk, addrOk, addrMaskedOk);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 8: fine vibrato writes BOTH globals, and stays a valid mode ---
	// GT2's own title-row toggle writes usefinevib alone, so a session that
	// used it wrote the old finevibrato back to the config on exit and lost
	// the change. The switch here has to survive a restart -- WITHOUT ever
	// putting a value other than 0 or 1 into finevibrato: gsong.c hands that
	// global straight to makespeedtable() as its `mode` when loading a song,
	// where 2 means MST_FUNKTEMPO and would mangle every vibrato parameter in
	// the file.
	step++;
	{
		multiplier = 1;

		CViewGT2SongSettings::SetFineVibrato(true);
		bool onOk = CViewGT2SongSettings::IsFineVibrato()
			&& finevibrato == 1 && usefinevib == 1;
		CViewGT2SongSettings::SetFineVibrato(false);
		bool offOk = !CViewGT2SongSettings::IsFineVibrato()
			&& finevibrato == 0 && usefinevib == 0;

		// Never a value makespeedtable() would read as a different mode.
		CViewGT2SongSettings::SetFineVibrato(true);
		bool stillAValidMode = finevibrato <= 1;

		// A restart re-derives usefinevib from finevibrato (goattrk2.c:409).
		// Replay that here: the stored setting must survive it.
		unsigned storedSetting = finevibrato;
		usefinevib = 0;
		if ((finevibrato == 1) && (multiplier < 2)) usefinevib = 1;
		if (finevibrato > 1) usefinevib = 1;
		bool survivesRestart = (storedSetting == 1) && (usefinevib == 1)
			&& CViewGT2SongSettings::IsFineVibrato();

		// Above 2x the derived flag drops, but the setting itself does not:
		// the checkbox must keep showing what the user chose.
		CViewGT2SongSettings::SetSpeedMultiplier(8);
		bool settingSurvivesMultiplier = CViewGT2SongSettings::IsFineVibrato()
			&& finevibrato == 1 && usefinevib == 0;

		bool ok = onOk && offOk && stillAValidMode && survivesRestart
			&& settingSurvivesMultiplier;
		snprintf(msg, sizeof(msg),
				 "on=%d off=%d validMode=%d survivesRestart=%d settingSurvivesMultiplier=%d (finevibrato=%u)",
				 onOk, offOk, stillAValidMode, survivesRestart, settingSurvivesMultiplier, finevibrato);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 9: packer optimizations and reSID knobs ---
	step++;
	{
		CViewGT2SongSettings::SetPulseOptimization(false);
		bool pulseOff = !CViewGT2SongSettings::IsPulseOptimization() && optimizepulse == 0;
		CViewGT2SongSettings::SetPulseOptimization(true);
		bool pulseOn = CViewGT2SongSettings::IsPulseOptimization() && optimizepulse == 1;

		CViewGT2SongSettings::SetRealtimeOptimization(false);
		bool realtimeOff = !CViewGT2SongSettings::IsRealtimeOptimization() && optimizerealtime == 0;
		CViewGT2SongSettings::SetRealtimeOptimization(true);
		bool realtimeOn = CViewGT2SongSettings::IsRealtimeOptimization() && optimizerealtime == 1;

		CViewGT2SongSettings::SetInterpolation(3);
		bool interpolationOk = CViewGT2SongSettings::GetInterpolation() == 3;
		CViewGT2SongSettings::SetInterpolation(99);
		bool interpolationClamped = CViewGT2SongSettings::GetInterpolation() == GT2_MAX_INTERPOLATION;
		CViewGT2SongSettings::SetInterpolation(-1);
		bool interpolationFloored = CViewGT2SongSettings::GetInterpolation() == 0;

		CViewGT2SongSettings::SetResidWriteDelay(32);
		bool delayOk = CViewGT2SongSettings::GetResidWriteDelay() == 32 && residdelay == 32;
		CViewGT2SongSettings::SetResidWriteDelay(1000);
		bool delayClamped = CViewGT2SongSettings::GetResidWriteDelay() == GT2_MAX_RESID_DELAY;
		CViewGT2SongSettings::SetResidWriteDelay(-1);
		bool delayFloored = CViewGT2SongSettings::GetResidWriteDelay() == 0;

		bool ok = pulseOff && pulseOn && realtimeOff && realtimeOn
			&& interpolationOk && interpolationClamped && interpolationFloored
			&& delayOk && delayClamped && delayFloored;
		snprintf(msg, sizeof(msg),
				 "pulse=%d/%d realtime=%d/%d interp=%d/%d/%d delay=%d/%d/%d",
				 pulseOff, pulseOn, realtimeOff, realtimeOn,
				 interpolationOk, interpolationClamped, interpolationFloored,
				 delayOk, delayClamped, delayFloored);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 10: tuning rebuilds the frequency table, and 0 puts it back ---
	// calculatefreqtable() derives every note from basepitch, so at 0 it would
	// fill the table with zeroes. GT2 never hits that because it only calls it
	// once at startup; a settings field can be tuned back to default, so the
	// built-in table has to be restorable.
	step++;
	{
		tuningcount = 0;
		unsigned char originalLo[GT2_FREQTABLE_NOTES];
		unsigned char originalHi[GT2_FREQTABLE_NOTES];
		memcpy(originalLo, freqtbllo, GT2_FREQTABLE_NOTES);
		memcpy(originalHi, freqtblhi, GT2_FREQTABLE_NOTES);

		CViewGT2SongSettings::SetBasePitch(440.0f);
		bool pitchStored = CViewGT2SongSettings::GetBasePitch() == 440.0f;
		bool tableChanged = memcmp(freqtbllo, originalLo, GT2_FREQTABLE_NOTES) != 0
			|| memcmp(freqtblhi, originalHi, GT2_FREQTABLE_NOTES) != 0;
		// Not silence: a table of zeroes would "differ" too.
		bool tableIsMusical = false;
		for (int i = 0; i < GT2_FREQTABLE_NOTES; i++)
			if (freqtbllo[i] != 0 || freqtblhi[i] != 0) { tableIsMusical = true; break; }

		CViewGT2SongSettings::SetBasePitch(0.0f);
		bool tableRestored = memcmp(freqtbllo, originalLo, GT2_FREQTABLE_NOTES) == 0
			&& memcmp(freqtblhi, originalHi, GT2_FREQTABLE_NOTES) == 0;

		CViewGT2SongSettings::SetBasePitch(-10.0f);
		bool negativeFloored = CViewGT2SongSettings::GetBasePitch() == 0.0f;

		// A zero divisor would be a division by zero in calculatefreqtable().
		CViewGT2SongSettings::SetDivisionsPerOctave(0.0f);
		bool divisionsFloored = CViewGT2SongSettings::GetDivisionsPerOctave() >= 1.0f;
		CViewGT2SongSettings::SetDivisionsPerOctave(12.0f);
		bool divisionsStored = CViewGT2SongSettings::GetDivisionsPerOctave() == 12.0f;

		bool ok = pitchStored && tableChanged && tableIsMusical && tableRestored
			&& negativeFloored && divisionsFloored && divisionsStored;
		snprintf(msg, sizeof(msg),
				 "stored=%d changed=%d musical=%d restored=%d negFloored=%d divFloored=%d divStored=%d",
				 pitchStored, tableChanged, tableIsMusical, tableRestored,
				 negativeFloored, divisionsFloored, divisionsStored);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 11: every control explains itself, and shortcut hints are honest ---
	step++;
	{
		bool allDescribed = true;
		int firstUndescribed = -1;
		const unsigned presets[2] = { KEY_RENOISE, KEY_TRACKER };
		for (int p = 0; p < 2; p++)
		{
			keypreset = presets[p];
			for (int c = 0; c < GT2_SETTINGS_CONTROL_COUNT; c++)
			{
				std::string tip = CViewGT2SongSettings::GetControlTooltip(c);
				if (tip.empty())
				{
					allDescribed = false;
					if (firstUndescribed < 0) firstUndescribed = c;
				}
			}
		}

		// SHIFT+F5..F8 reach native GT2 only outside the Renoise layout --
		// under KEY_RENOISE the dispatcher spends them on paste / shrink /
		// expand, so naming them there would be a lie.
		keypreset = KEY_TRACKER;
		bool namedOutsideRenoise =
			CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_SID_MODEL).find("Shift+F8") != std::string::npos
			&& CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_SPEED_MULTIPLIER).find("Shift+F5") != std::string::npos
			&& CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_HARD_RESTART).find("Shift+F7") != std::string::npos;

		keypreset = KEY_RENOISE;
		bool hiddenUnderRenoise =
			CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_SID_MODEL).find("Shift+F8") == std::string::npos
			&& CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_SPEED_MULTIPLIER).find("Shift+F5") == std::string::npos
			&& CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_HARD_RESTART).find("Shift+F7") == std::string::npos;

		// Controls with no binding print no shortcut line at all rather than
		// a "none" the reader has to parse.
		bool unboundStaySilent =
			CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_NAME).find("Shortcut:") == std::string::npos
			&& CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_TEMPO).find("Shortcut:") == std::string::npos
			&& CViewGT2SongSettings::GetControlTooltip(GT2_SETTINGS_SCALA_FILE).find("Shortcut:") == std::string::npos;

		bool ok = allDescribed && namedOutsideRenoise && hiddenUnderRenoise && unboundStaySilent;
		snprintf(msg, sizeof(msg),
				 "described=%d (firstBad=%d) namedOutsideRenoise=%d hiddenUnderRenoise=%d unboundSilent=%d",
				 allDescribed, firstUndescribed, namedOutsideRenoise, hiddenUnderRenoise, unboundStaySilent);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 12: the Scala loader refuses what would overrun GT2's arrays ---
	// readscalatuningfile() scanf's the step count with no bound and then
	// writes tuning[i] for every i below it, into a double[96]; it also
	// strcat's two note-name characters per step into a char[186], which
	// overflows at 93 steps. Those were only ever reachable from a config file
	// read once at startup until this view put a Load button on them.
	step++;
	{
		char scalaDir[512];
		snprintf(scalaDir, sizeof(scalaDir), "%s", "/tmp");

		char goodPath[600], hugePath[600], truncatedPath[600], emptyPath[600];
		snprintf(goodPath, sizeof(goodPath), "%s/gt2-test-good.scl", scalaDir);
		snprintf(hugePath, sizeof(hugePath), "%s/gt2-test-huge.scl", scalaDir);
		snprintf(truncatedPath, sizeof(truncatedPath), "%s/gt2-test-truncated.scl", scalaDir);
		snprintf(emptyPath, sizeof(emptyPath), "%s/gt2-test-empty.scl", scalaDir);

		FILE *f = fopen(goodPath, "wt");
		// No degree names in any fixture: a named degree makes
		// readscalatuningfile() call setspecialnotenames(), which mallocs a
		// fresh notename[] table for the WHOLE application and leaks the old
		// one. There is no way to put that back, so the test does not go there.
		if (f) { fprintf(f, "! test\nTest Scale\n 3\n!\n 100.0\n 200.0\n 2/1\n"); fclose(f); }
		f = fopen(hugePath, "wt");
		if (f)
		{
			fprintf(f, "! huge\nHuge Scale\n 4096\n!\n");
			for (int i = 0; i < 4096; i++) fprintf(f, " %d.0\n", i + 1);
			fclose(f);
		}
		f = fopen(truncatedPath, "wt");
		if (f) { fprintf(f, "! truncated\nTruncated\n 8\n!\n 100.0\n 200.0\n"); fclose(f); }
		f = fopen(emptyPath, "wt");
		if (f) { fprintf(f, "! nothing but comments\n"); fclose(f); }

		// A well-formed file loads.
		CViewGT2SongSettings::LoadScalaTuning(goodPath);
		bool goodLoaded = CViewGT2SongSettings::GetTuningStepCount() == 3
			&& CViewGT2SongSettings::GetScalaLoadError().empty();

		// A file declaring more steps than the arrays hold is refused OUTRIGHT
		// -- not clamped after the fact, which would already be too late.
		CViewGT2SongSettings::LoadScalaTuning(hugePath);
		bool hugeRefused = CViewGT2SongSettings::GetTuningStepCount() == 0
			&& !CViewGT2SongSettings::GetScalaLoadError().empty();

		// A file that ends before its declared steps do leaves tuning[] partly
		// stale, so it must not count as loaded either.
		CViewGT2SongSettings::LoadScalaTuning(truncatedPath);
		bool truncatedRefused = CViewGT2SongSettings::GetTuningStepCount() == 0
			&& !CViewGT2SongSettings::GetScalaLoadError().empty();

		CViewGT2SongSettings::LoadScalaTuning(emptyPath);
		bool emptyRefused = CViewGT2SongSettings::GetTuningStepCount() == 0;

		CViewGT2SongSettings::LoadScalaTuning("/tmp/gt2-test-does-not-exist.scl");
		bool missingRefused = CViewGT2SongSettings::GetTuningStepCount() == 0
			&& !CViewGT2SongSettings::GetScalaLoadError().empty();

		// A scale with no base pitch cannot be applied: calculatefreqtable()
		// derives every note from basepitch, so at 0 it would write 96 zeroes
		// and the editor would go silent. The table must be untouched and the
		// view must say the scale is inert.
		unsigned char builtInLo[GT2_FREQTABLE_NOTES];
		memcpy(builtInLo, freqtbllo, GT2_FREQTABLE_NOTES);
		CViewGT2SongSettings::SetBasePitch(0.0f);
		CViewGT2SongSettings::LoadScalaTuning(goodPath);
		bool tableNotZeroed = memcmp(freqtbllo, builtInLo, GT2_FREQTABLE_NOTES) == 0;
		bool reportedInert = CViewGT2SongSettings::IsScalaTuningInert();

		// With a base pitch it does apply.
		CViewGT2SongSettings::SetBasePitch(440.0f);
		bool appliedWithPitch = !CViewGT2SongSettings::IsScalaTuningInert()
			&& memcmp(freqtbllo, builtInLo, GT2_FREQTABLE_NOTES) != 0;

		// Clearing the path drops the scale.
		CViewGT2SongSettings::LoadScalaTuning("");
		bool clearedOk = CViewGT2SongSettings::GetTuningStepCount() == 0
			&& CViewGT2SongSettings::GetScalaLoadError().empty();
		CViewGT2SongSettings::SetBasePitch(0.0f);

		remove(goodPath); remove(hugePath); remove(truncatedPath); remove(emptyPath);

		bool ok = goodLoaded && hugeRefused && truncatedRefused && emptyRefused
			&& missingRefused && tableNotZeroed && reportedInert && appliedWithPitch
			&& clearedOk;
		snprintf(msg, sizeof(msg),
				 "good=%d hugeRefused=%d truncatedRefused=%d emptyRefused=%d missingRefused=%d "
				 "tableNotZeroed=%d inert=%d appliedWithPitch=%d cleared=%d",
				 goodLoaded, hugeRefused, truncatedRefused, emptyRefused, missingRefused,
				 tableNotZeroed, reportedInert, appliedWithPitch, clearedOk);
		StepCompleted(step, ok, msg);
		if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
	}

	// --- Test 13: moving between two fields does not lose the second edit ---
	// ImGui activates the newly clicked field and deactivates the old one in
	// the SAME frame, and which happens first depends only on which is drawn
	// first. A single shared "a step is open" flag loses the second edit's
	// history when the new field wins that race, so the pending step records
	// which control owns it.
	step++;
	{
		CViewGT2SongSettings *settingsView =
			(pluginGoatTracker != NULL) ? pluginGoatTracker->viewSongSettings : NULL;

		if (settingsView == NULL)
		{
			ReportRequiredGap("no GT2 Song Settings view instance, so the "
							  "field-to-field undo handoff was never exercised");
			StepCompleted(step, true, "no view instance -- handoff not exercised");
		}
		else
		{
			GT2UndoHistory()->Clear();
			settingsView->pendingSnapshotControl = -1;

			CViewGT2SongSettings::SetSongName("NAME0");
			CViewGT2SongSettings::SetAuthorName("AUTHOR0");

			// Author is being edited...
			settingsView->BeginEdit(GT2_SETTINGS_AUTHOR);
			CViewGT2SongSettings::SetAuthorName("AUTHOR1");

			// ...and the user clicks Name, which is drawn first, so its
			// activation is seen BEFORE the author field's deactivation.
			settingsView->BeginEdit(GT2_SETTINGS_NAME);
			settingsView->CommitEdit(GT2_SETTINGS_AUTHOR);
			CViewGT2SongSettings::SetSongName("NAME1");
			settingsView->CommitEdit(GT2_SETTINGS_NAME);

			// Both edits must be on the timeline, newest first.
			bool nameUndone = GT2UndoHistory()->Undo()
				&& CViewGT2SongSettings::GetSongName() == "NAME0"
				&& CViewGT2SongSettings::GetAuthorName() == "AUTHOR1";
			bool authorUndone = GT2UndoHistory()->Undo()
				&& CViewGT2SongSettings::GetAuthorName() == "AUTHOR0";
			bool bothRedone = GT2UndoHistory()->Redo() && GT2UndoHistory()->Redo()
				&& CViewGT2SongSettings::GetSongName() == "NAME1"
				&& CViewGT2SongSettings::GetAuthorName() == "AUTHOR1";

			settingsView->pendingSnapshotControl = -1;
			GT2UndoHistory()->Clear();

			bool ok = nameUndone && authorUndone && bothRedone;
			snprintf(msg, sizeof(msg), "nameUndone=%d authorUndone=%d bothRedone=%d",
					 nameUndone, authorUndone, bothRedone);
			StepCompleted(step, ok, msg);
			if (!ok) { GT2SettingsRestore(&backup); TestCompleted(false, msg); return; }
		}
	}

	GT2SettingsRestore(&backup);
	TestCompleted(true, "GT2 song settings: metadata, chip/timing, player options and tuning all verified");
}
