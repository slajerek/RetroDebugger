#include "CViewGT2SongSettings.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewGT2Patterns.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "gplay.h"
#include "gsound.h"
#include "goattrk2.h"

// Not listed in goattrk2.h, but defined in goattrk2.c alongside the ones that
// are. The config reader and writer touch all of them.
extern unsigned residdelay;
extern unsigned customclockrate;
extern float equaldivisionsperoctave;
extern char scalatuningfilepath[MAX_PATHNAME];
extern char specialnotenames[186];
extern char tuningname[64];
extern int tuningcount;
}

// The frequency table calculatefreqtable() overwrites. Only the first 96
// entries are notes; the arrays themselves are 128 bytes.
#define GT2_FREQTABLE_NOTES 96

// calculatefreqtable() derives every note from basepitch, so with basepitch at
// 0 it would fill the table with zeroes rather than restore anything. GT2 gets
// away with that because it only ever calls it once, at startup, behind
// `if (basepitch > 0.0f)`. A settings view can be tuned back to 0, so the
// built-in table -- a static initialiser in gplay.c, with no other copy
// anywhere -- is saved before the first override and put back when the user
// returns to it.
static bool gt2DefaultFreqTableSaved = false;
static unsigned char gt2DefaultFreqTblLo[GT2_FREQTABLE_NOTES];
static unsigned char gt2DefaultFreqTblHi[GT2_FREQTABLE_NOTES];

static void GT2SaveDefaultFreqTable()
{
	if (gt2DefaultFreqTableSaved)
		return;
	memcpy(gt2DefaultFreqTblLo, freqtbllo, GT2_FREQTABLE_NOTES);
	memcpy(gt2DefaultFreqTblHi, freqtblhi, GT2_FREQTABLE_NOTES);
	gt2DefaultFreqTableSaved = true;
}

// Rebuild the note frequencies from whatever the tuning settings now say, or
// put the built-in table back when they say "default".
static void GT2RefreshFreqTable()
{
	GT2SaveDefaultFreqTable();

	// `basepitch > 0` is the ONLY condition, exactly as goattrk2.c:424 has it,
	// and a loaded Scala scale does not widen it. calculatefreqtable() opens
	// with `basefreq = basepitch * ...` and both of its branches -- the equal
	// division one AND the tuning[] one -- multiply that through, so at
	// basepitch 0 it does not fall back to anything, it writes 96 zeroes and
	// the editor goes silent. A scale with no base pitch to hang off is inert
	// here for the same reason it is inert in GT2 itself; the view says so
	// rather than letting the user find out by hearing nothing.
	if (basepitch > 0.0f)
	{
		calculatefreqtable();
		return;
	}

	memcpy(freqtbllo, gt2DefaultFreqTblLo, GT2_FREQTABLE_NOTES);
	memcpy(freqtblhi, gt2DefaultFreqTblHi, GT2_FREQTABLE_NOTES);
}

// True when a scale is loaded but cannot be applied, because the note table is
// still the built-in one and nothing derives from the scale.
// Why the last Load did not take, or NULL. Static string literals only.
static const char *gt2ScalaLoadError = NULL;

static bool GT2ScalaTuningIsInert()
{
	return tuningcount > 0 && basepitch <= 0.0f;
}

// readscalatuningfile() trusts its input completely: it scanf's the step count
// with no bound and then writes tuning[i] for every i below it, into a
// `double tuning[96]` -- and strcat's two note-name characters per step into a
// `char specialnotenames[186]`, which overflows at 93 steps even for a
// perfectly legitimate large scale. Until now that only mattered to a
// hand-edited config read once at startup. A Load button in the UI turns it
// into "open this file" and puts it in reach of anything the user drags in, so
// the header is parsed here first and a file that would overrun is refused.
//
// Skips '!' comments and blank lines exactly as the GT2 reader does, then
// checks the declared step count against what the arrays hold AND against how
// many degree lines the file actually carries.
//
// Counting them here is the only way to catch a truncated file: the reader
// sets `tuningcount` straight from the header before it reads a single degree,
// so after the fact that global agrees with the header no matter how early the
// file ran out -- and it `return`s mid-loop on EOF, leaving the rest of
// `tuning[]` holding whatever the previous scale left there.
#define GT2_MAX_SCALA_STEPS 93

static bool GT2ScalaFileIsSafe(const char *path, int *stepCountOut, const char **errorOut)
{
	*stepCountOut = 0;
	*errorOut = NULL;

	FILE *handle = fopen(path, "rt");
	if (handle == NULL)
	{
		*errorOut = "cannot open file";
		return false;
	}

	char line[MAX_PATHNAME];
	int headerLinesSeen = 0;
	int stepCount = -1;
	int degreeLines = 0;

	while (fgets(line, sizeof(line), handle) != NULL)
	{
		if (line[0] == 0 || line[0] == '!' || line[0] == 13 || line[0] == 10)
			continue;

		headerLinesSeen++;
		if (headerLinesSeen == 1)
			continue;                       // the scale's name
		if (headerLinesSeen == 2)
		{
			if (sscanf(line, "%d", &stepCount) != 1)
				stepCount = -1;
			continue;                       // the step count
		}
		degreeLines++;
	}
	fclose(handle);

	if (stepCount < 1)
	{
		*errorOut = "no step count in header";
		return false;
	}
	if (stepCount > GT2_MAX_SCALA_STEPS)
	{
		*errorOut = "too many steps";
		return false;
	}
	if (degreeLines < stepCount)
	{
		*errorOut = "file ends before its steps do";
		return false;
	}

	*stepCountOut = stepCount;
	return true;
}

static std::string GT2SettingsTip(const char *what, const char *shortcut)
{
	std::string tip = what;
	if (shortcut != NULL && shortcut[0] != 0)
	{
		tip += "\nShortcut: ";
		tip += shortcut;
	}
	return tip;
}

// GT2's name buffers are MAX_STR bytes including the terminator.
static void GT2StoreName(char *dest, const char *value)
{
	if (value == NULL)
		value = "";
	size_t length = strlen(value);
	if (length > MAX_STR - 1)
		length = MAX_STR - 1;
	memcpy(dest, value, length);
	dest[length] = 0;
}

CViewGT2SongSettings::CViewGT2SongSettings(const char *name, float posX, float posY, float posZ,
										   float sizeX, float sizeY, C64DebuggerPluginGoatTracker *plugin)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->plugin = plugin;
	this->pendingSnapshotControl = -1;
	this->customClockEditValue = 0;
	this->customClockEditing = false;
	this->speedMultiplierEditValue = 1;
	this->speedMultiplierEditing = false;
	this->scalaPathEditing = false;
	this->basePitchEditValue = 0.0f;
	this->basePitchEditing = false;
	this->divisionsEditValue = 12.0f;
	this->divisionsEditing = false;

	// Take the copy of the built-in frequency table HERE, not lazily on the
	// first override. The plugin constructs its views at the end of Init() and
	// only then starts the GT2 thread, so at this moment gtmain() has not run
	// and cannot have called calculatefreqtable() for a basepitch restored
	// from goattrk2.cfg. Left to the first SetBasePitch(), a config that
	// already carried a custom pitch would have us save the OVERRIDDEN table
	// as "default" and never be able to get back to equal temperament.
	GT2SaveDefaultFreqTable();
}

CViewGT2SongSettings::~CViewGT2SongSettings()
{
}

//
// Song information
//

std::string CViewGT2SongSettings::GetSongName()      { return songname; }
std::string CViewGT2SongSettings::GetAuthorName()    { return authorname; }
std::string CViewGT2SongSettings::GetCopyrightName() { return copyrightname; }

void CViewGT2SongSettings::SetSongName(const char *value)      { GT2StoreName(songname, value); }
void CViewGT2SongSettings::SetAuthorName(const char *value)    { GT2StoreName(authorname, value); }
void CViewGT2SongSettings::SetCopyrightName(const char *value) { GT2StoreName(copyrightname, value); }

//
// Chip and timing
//

void CViewGT2SongSettings::ApplySoundSettings()
{
	// gtsound_init() tears reSID down and builds it again. GT2's own SHIFT+F8
	// and the click on its title row call it exactly like this; what is new is
	// the caller's thread.
	//
	// What keeps the audio thread out of the freed object is gtsound_uninit()
	// clearing `initted` as its first act (gsound.c:241) -- gtsound_mixer(),
	// which is what snd_mixdata() calls into, returns immediately while that is
	// 0 -- and then sleeping 50 ms to let any call already inside it drain
	// before `buffer` is freed and sid_init() replaces the SID. The mixer
	// pointer itself is cleared afterwards and is not the guard.
	//
	// That 50 ms is spent on the calling thread, so this is called once per
	// committed change -- a released slider, a clicked toggle -- and never
	// once per drag frame. It is still a visible hitch on a deliberate click.
	//
	// Known limitation: gsound.c has no lock, and GT2's own thread can call
	// gtsound_init() too (SHIFT+F5/F6/F8, goattrk2.c:1126-1152). Two calls
	// landing together would race on `buffer` and on sid_init(). It needs both
	// hands at once and neither path is reachable without a deliberate user
	// action, so it is documented rather than papered over with a lock that
	// only one of the two callers would take.
	if (!gt2_engine_ready)
		return;
	gtsound_init(b, mr, writer, hardsid, sidmodel, ntsc, multiplier, catweasel,
				 interpolate, customclockrate);
}

int CViewGT2SongSettings::GetSidModel()
{
	return (int)(sidmodel & 1);
}

void CViewGT2SongSettings::SetSidModel(int model)
{
	// greloc.c:2110 puts this bit in the SID header, so it travels with the
	// exported tune rather than staying an editor preference.
	sidmodel = (model != 0) ? 1u : 0u;
	ApplySoundSettings();
}

bool CViewGT2SongSettings::IsNtsc()
{
	return ntsc != 0;
}

void CViewGT2SongSettings::SetNtsc(bool value)
{
	ntsc = value ? 1u : 0u;
	ApplySoundSettings();
}

int CViewGT2SongSettings::GetCustomClockRate()
{
	return (int)customclockrate;
}

void CViewGT2SongSettings::SetCustomClockRate(int hz)
{
	// goattrk2.c:414 rejects anything under 100 Hz outright rather than
	// clamping it -- a clock that slow is a typo, not a setting.
	if (hz < GT2_MIN_CUSTOM_CLOCK_RATE)
		hz = 0;
	customclockrate = (unsigned)hz;
	ApplySoundSettings();
}

int CViewGT2SongSettings::GetSpeedMultiplier()
{
	return (int)multiplier;
}

void CViewGT2SongSettings::SetSpeedMultiplier(int value)
{
	if (value < 0) value = 0;
	if (value > GT2_MAX_SPEED_MULTIPLIER) value = GT2_MAX_SPEED_MULTIPLIER;
	multiplier = (unsigned)value;

	// Fine vibrato only applies below 2x: goattrk2.c:409 turns it on for
	// `finevibrato == 1` only while the multiplier is under 2, because at
	// higher multipliers the player already runs often enough. Re-derive the
	// flag so a multiplier change moves it the same way a restart would.
	usefinevib = (finevibrato == 1 && multiplier < 2) ? 1u : 0u;

	ApplySoundSettings();
}

int CViewGT2SongSettings::GetSongTempo()
{
	return GT2_GetSongTempo();
}

void CViewGT2SongSettings::SetSongTempo(int tempo)
{
	GT2_SetSongTempo(tempo);
}

//
// Player options
//

int CViewGT2SongSettings::GetHardRestartAdsr()
{
	return (int)(adparam & 0xffff);
}

void CViewGT2SongSettings::SetHardRestartAdsr(int value)
{
	adparam = ((unsigned)value) & 0xffff;
}

bool CViewGT2SongSettings::IsFineVibrato()
{
	// finevibrato is the setting; usefinevib is derived from it and from the
	// multiplier. Report the setting, so the checkbox shows what the user
	// chose rather than what the current multiplier happens to allow.
	return finevibrato != 0;
}

void CViewGT2SongSettings::SetFineVibrato(bool value)
{
	// finevibrato is NOT a free-form flag: gsong.c passes it straight to
	// makespeedtable() as the `mode` argument when a song is loaded, where 0
	// is MST_NOFINEVIB and 1 is MST_FINEVIB. Writing 2 -- which goattrk2.c:410
	// implies is a legal "force on" -- would select MST_FUNKTEMPO instead and
	// silently reinterpret every vibrato parameter in the next loaded file.
	// So it stays 0 or 1, and the derived flag is derived.
	finevibrato = value ? 1u : 0u;

	// GT2's own title-row toggle writes usefinevib alone, which is why a
	// session that used it wrote the OLD finevibrato back to the config on
	// exit and lost the change. Write the setting, then re-derive exactly as
	// goattrk2.c:409 does at startup.
	usefinevib = (finevibrato == 1 && multiplier < 2) ? 1u : 0u;
}

bool CViewGT2SongSettings::IsPulseOptimization()
{
	return optimizepulse != 0;
}

void CViewGT2SongSettings::SetPulseOptimization(bool value)
{
	optimizepulse = value ? 1u : 0u;
}

bool CViewGT2SongSettings::IsRealtimeOptimization()
{
	return optimizerealtime != 0;
}

void CViewGT2SongSettings::SetRealtimeOptimization(bool value)
{
	optimizerealtime = value ? 1u : 0u;
}

int CViewGT2SongSettings::GetSidAddress()
{
	return (int)(sidaddress & 0xffff);
}

void CViewGT2SongSettings::SetSidAddress(int address)
{
	sidaddress = ((unsigned)address) & 0xffff;
}

//
// reSID emulation
//

int CViewGT2SongSettings::GetInterpolation()
{
	return (int)interpolate;
}

void CViewGT2SongSettings::SetInterpolation(int value)
{
	if (value < 0) value = 0;
	if (value > GT2_MAX_INTERPOLATION) value = GT2_MAX_INTERPOLATION;
	interpolate = (unsigned)value;
	ApplySoundSettings();
}

int CViewGT2SongSettings::GetResidWriteDelay()
{
	return (int)residdelay;
}

void CViewGT2SongSettings::SetResidWriteDelay(int cycles)
{
	// gsid.cpp reads residdelay on every write, so this one needs no re-init.
	if (cycles < 0) cycles = 0;
	if (cycles > GT2_MAX_RESID_DELAY) cycles = GT2_MAX_RESID_DELAY;
	residdelay = (unsigned)cycles;
}

//
// Tuning
//

float CViewGT2SongSettings::GetBasePitch()
{
	return basepitch;
}

void CViewGT2SongSettings::SetBasePitch(float hz)
{
	if (hz < 0.0f) hz = 0.0f;
	basepitch = hz;
	GT2RefreshFreqTable();
}

float CViewGT2SongSettings::GetDivisionsPerOctave()
{
	return equaldivisionsperoctave;
}

void CViewGT2SongSettings::SetDivisionsPerOctave(float divisions)
{
	// calculatefreqtable() divides by this, so zero or negative is not a
	// tuning, it is a division by zero waiting to happen.
	if (divisions < 1.0f) divisions = 1.0f;
	equaldivisionsperoctave = divisions;
	GT2RefreshFreqTable();
}

std::string CViewGT2SongSettings::GetScalaTuningPath()
{
	return scalatuningfilepath;
}

void CViewGT2SongSettings::LoadScalaTuning(const char *path)
{
	if (path == NULL)
		path = "";

	size_t length = strlen(path);
	if (length > MAX_PATHNAME - 1)
		length = MAX_PATHNAME - 1;
	memcpy(scalatuningfilepath, path, length);
	scalatuningfilepath[length] = 0;

	// readscalatuningfile() only ever grows tuningcount, so clear it first --
	// otherwise an unreadable file leaves the previous scale in place while
	// the path field claims the new one is loaded.
	tuningcount = 0;
	tuningname[0] = 0;
	gt2ScalaLoadError = NULL;

	if (scalatuningfilepath[0] != 0)
	{
		int declaredSteps = 0;
		const char *error = NULL;
		if (!GT2ScalaFileIsSafe(scalatuningfilepath, &declaredSteps, &error))
		{
			gt2ScalaLoadError = error;
			GT2RefreshFreqTable();
			return;
		}

		readscalatuningfile();

		// Belt and braces. The pre-scan already refused a short file, so this
		// can only fire if the reader disagreed with it -- but tuningcount is
		// what indexes tuning[], so it is worth one comparison.
		if (tuningcount != declaredSteps)
		{
			tuningcount = 0;
			tuningname[0] = 0;
			gt2ScalaLoadError = "unreadable scale";
			GT2RefreshFreqTable();
			return;
		}

		// The .scl carries note names too, and the reader writes them into
		// specialnotenames[] as a side effect. Apply them only on a load that
		// worked -- on a failed one they are still the PREVIOUS scale's, and
		// re-applying those while the path field shows a new file would be a
		// quiet lie.
		if (specialnotenames[1] != 0)
			setspecialnotenames();
	}

	GT2RefreshFreqTable();
}

std::string CViewGT2SongSettings::GetScalaLoadError()
{
	return gt2ScalaLoadError != NULL ? gt2ScalaLoadError : "";
}

bool CViewGT2SongSettings::IsScalaTuningInert()
{
	return GT2ScalaTuningIsInert();
}

std::string CViewGT2SongSettings::GetTuningName()
{
	return tuningname;
}

int CViewGT2SongSettings::GetTuningStepCount()
{
	return tuningcount;
}

//
// Undo
//

void CViewGT2SongSettings::BeginEdit(int control)
{
	if (pendingSnapshotControl == control)
		return;

	// Clicking straight from one field into another activates the new one and
	// deactivates the old one in the same frame, and the order depends only on
	// which is drawn first. If the new field wins that race, close the
	// outstanding step here rather than letting the loser's Commit throw this
	// capture away -- otherwise the second edit would silently make no history.
	if (pendingSnapshotControl != -1)
		GT2UndoHistory()->CommitIfChanged(pendingSnapshot);

	pendingSnapshot = GT2UndoHistory()->Capture();
	pendingSnapshotControl = control;
}

void CViewGT2SongSettings::DropOrphanedEdit()
{
	// CommitEdit() only ever runs from IsItemDeactivated(), which needs the
	// field to be drawn. Hiding the window mid-edit -- the menu item, or the
	// hide-all block when GoatTracker is closed -- therefore leaves the step
	// open forever, and the next Commit would push a snapshot taken before
	// everything that happened in the meantime: one Ctrl+Z would then roll
	// back unrelated pattern, table and instrument work.
	//
	// So an orphaned step is DISCARDED, not committed. By the time we notice,
	// the snapshot can no longer be attributed to the field that opened it.
	// The cost is losing undo for one rename in a rare case; committing would
	// cost the user work they never touched.
	if (pendingSnapshotControl != -1 && !ImGui::IsAnyItemActive())
		pendingSnapshotControl = -1;

	// Same idea for the deferred values: their pending edit is unattributable
	// now, so let the fields re-seed from the globals.
	if (!ImGui::IsAnyItemActive())
	{
		customClockEditing = false;
		speedMultiplierEditing = false;
		scalaPathEditing = false;
		basePitchEditing = false;
		divisionsEditing = false;
	}
}

void CViewGT2SongSettings::CommitEdit(int control)
{
	if (pendingSnapshotControl != control)
		return;
	pendingSnapshotControl = -1;
	// CommitIfChanged() records nothing when the value did not move, so this
	// is unconditional -- a field that was focused and left alone costs no
	// undo entry.
	GT2UndoHistory()->CommitIfChanged(pendingSnapshot);
}

//
// Tooltips
//

std::string CViewGT2SongSettings::GetControlTooltip(int control)
{
	// SHIFT+F5..F8 reach native GT2 only while the Renoise dispatcher is not
	// in the way: under KEY_RENOISE those combinations are paste / shrink /
	// expand instead (CGT2RenoiseInput::HandleKey), so naming them there would
	// be a lie.
	const bool nativeFunctionKeys = (keypreset != KEY_RENOISE);

	switch (control)
	{
	case GT2_SETTINGS_NAME:
		return GT2SettingsTip("Song title. Stored in the .sng and written into the SID header on export. 31 characters.", NULL);

	case GT2_SETTINGS_AUTHOR:
		return GT2SettingsTip("Composer. Stored in the .sng and written into the SID header on export. 31 characters.", NULL);

	case GT2_SETTINGS_COPYRIGHT:
		return GT2SettingsTip("Copyright / release line. Stored in the .sng and written into the SID header on export. 31 characters.", NULL);

	case GT2_SETTINGS_SID_MODEL:
		return GT2SettingsTip("Which SID revision the editor emulates: 6581 (filter is darker, waveform mixing is dirtier) or 8580.\nAlso set in the exported SID header, so it travels with the tune.",
			nativeFunctionKeys ? "Shift+F8" : NULL);

	case GT2_SETTINGS_VIDEO_STANDARD:
		return GT2SettingsTip("Machine timing: PAL runs the player 50 times a second, NTSC 60.\nChanges the pitch of every note and the song's real duration, and is written into the exported SID header.", NULL);

	case GT2_SETTINGS_CUSTOM_CLOCK:
		return GT2SettingsTip("SID clock in cycles per second, overriding the PAL/NTSC default.\n0 uses the default. Values under 100 are rejected.", NULL);

	case GT2_SETTINGS_SPEED_MULTIPLIER:
		return GT2SettingsTip("How many times per frame the player runs. 1x is normal; 2x and up give finer vibrato and faster effects at the cost of raster time.\n0 is the 25 Hz mode -- the player runs every second frame.",
			nativeFunctionKeys ? "Shift+F5 slower, Shift+F6 faster" : NULL);

	case GT2_SETTINGS_TEMPO:
		return GT2SettingsTip("The song's default tempo, in player ticks per pattern row. 6 is GT2's default.\nAt 6 ticks on PAL with 4 rows to the beat that is 125 BPM. An F command in a pattern overrides it from the row it sits on.", NULL);

	case GT2_SETTINGS_HARD_RESTART:
		return GT2SettingsTip("Hard restart ADSR, as one 16-bit AD/SR pair. The player writes it to the voice for one tick before a new note so the envelope restarts cleanly.\n0F00 is the default.",
			nativeFunctionKeys ? "Shift+F7" : NULL);

	case GT2_SETTINGS_FINE_VIBRATO:
		return GT2SettingsTip("Read vibrato parameters at fine resolution when loading a song, giving slower and smoother movement.\nIt only takes effect below a 2x multiplier -- above that the player already runs often enough -- so the FV indicator can be off while this stays on.", NULL);

	case GT2_SETTINGS_OPTIMIZE_PULSE:
		return GT2SettingsTip("Let the packer skip pulse-width writes that would not change the register. Saves raster time; turn it off if a routine outside the player also writes pulse.", NULL);

	case GT2_SETTINGS_OPTIMIZE_REALTIME:
		return GT2SettingsTip("Let the packer skip realtime effect writes that would not change the register. Same trade as pulse skipping.", NULL);

	case GT2_SETTINGS_SID_ADDRESS:
		return GT2SettingsTip("Base address the packed player writes its SID registers to. D400 is the normal chip.", NULL);

	case GT2_SETTINGS_INTERPOLATION:
		return GT2SettingsTip("reSID resampling quality for the editor's own playback. Interpolation removes aliasing; distortion models the 6581's non-linear output.\nAffects what you hear here, never the exported tune.", NULL);

	case GT2_SETTINGS_RESID_DELAY:
		return GT2SettingsTip("Random delay, in cycles, added to reSID register writes to imitate a badline pushing the player around.\n0 is off. Affects what you hear here, never the exported tune.", NULL);

	case GT2_SETTINGS_BASE_PITCH:
		return GT2SettingsTip("Pitch of A-4 in Hz. 0 uses GT2's built-in frequency table, which sits close to 440 Hz.\nAnything else rebuilds the whole note table -- and a Scala scale needs a value here before it does anything.", NULL);

	case GT2_SETTINGS_DIVISIONS_PER_OCTAVE:
		return GT2SettingsTip("Equal divisions per octave. 12 is normal tuning; 8.2019143 gives Bohlen-Pierce.\nOnly used when a base pitch is set and no Scala scale is loaded.", NULL);

	case GT2_SETTINGS_SCALA_FILE:
		return GT2SettingsTip("Path to a Scala .scl scale. Its steps replace the equal-division tuning and its note names replace the note column's.\nNeeds an A-4 pitch above -- GT2 derives every note from that, so with no pitch set the scale is loaded but not used.\nClear the field and load again to drop it.", NULL);
	}

	return "";
}

//
// Rendering
//

void CViewGT2SongSettings::RenderImGui()
{
	PreRenderImGui();

	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
		ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
		ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));

	const float fieldWidth = 220.0f * uiScale;
	char buf[MAX_PATHNAME];

	DropOrphanedEdit();

	//
	// Song information
	//
	ImGui::SeparatorText("Song");

	ImGui::PushItemWidth(fieldWidth);

	GT2StoreName(buf, songname);
	if (ImGui::InputText("Name##gt2settings", buf, MAX_STR))
		SetSongName(buf);
	if (ImGui::IsItemActivated()) BeginEdit(GT2_SETTINGS_NAME);
	if (ImGui::IsItemDeactivated()) CommitEdit(GT2_SETTINGS_NAME);
	ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_NAME).c_str());

	GT2StoreName(buf, authorname);
	if (ImGui::InputText("Author##gt2settings", buf, MAX_STR))
		SetAuthorName(buf);
	if (ImGui::IsItemActivated()) BeginEdit(GT2_SETTINGS_AUTHOR);
	if (ImGui::IsItemDeactivated()) CommitEdit(GT2_SETTINGS_AUTHOR);
	ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_AUTHOR).c_str());

	GT2StoreName(buf, copyrightname);
	if (ImGui::InputText("Copyright##gt2settings", buf, MAX_STR))
		SetCopyrightName(buf);
	if (ImGui::IsItemActivated()) BeginEdit(GT2_SETTINGS_COPYRIGHT);
	if (ImGui::IsItemDeactivated()) CommitEdit(GT2_SETTINGS_COPYRIGHT);
	ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_COPYRIGHT).c_str());

	ImGui::PopItemWidth();

	//
	// Chip and timing
	//
	ImGui::SeparatorText("Chip & timing");

	ImGui::PushItemWidth(fieldWidth);

	{
		static const char *sidModelNames[] = { "6581", "8580" };
		int model = GetSidModel();
		if (ImGui::Combo("SID model##gt2settings", &model, sidModelNames, 2))
			SetSidModel(model);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_SID_MODEL).c_str());
	}

	{
		static const char *videoNames[] = { "PAL (50 Hz)", "NTSC (60 Hz)" };
		int video = IsNtsc() ? 1 : 0;
		if (ImGui::Combo("Video standard##gt2settings", &video, videoNames, 2))
			SetNtsc(video != 0);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_VIDEO_STANDARD).c_str());
	}

	{
		// Applied on release, not on keystroke: every change re-inits reSID.
		if (!customClockEditing)
			customClockEditValue = GetCustomClockRate();
		ImGui::InputInt("Custom SID clock (Hz)##gt2settings", &customClockEditValue, 0, 0);
		if (ImGui::IsItemActivated())
			customClockEditing = true;
		if (ImGui::IsItemDeactivated())
		{
			customClockEditing = false;
			SetCustomClockRate(customClockEditValue);
		}
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_CUSTOM_CLOCK).c_str());
		if (GetCustomClockRate() == 0)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(default)");
		}
	}

	{
		// Same deal: the slider moves freely, reSID is rebuilt once, on release.
		if (!speedMultiplierEditing)
			speedMultiplierEditValue = GetSpeedMultiplier();
		ImGui::SliderInt("Speed multiplier##gt2settings", &speedMultiplierEditValue,
						 0, GT2_MAX_SPEED_MULTIPLIER,
						 speedMultiplierEditValue == 0 ? "25 Hz" : "%dx");
		if (ImGui::IsItemActivated())
			speedMultiplierEditing = true;
		if (ImGui::IsItemDeactivated())
		{
			speedMultiplierEditing = false;
			SetSpeedMultiplier(speedMultiplierEditValue);
		}
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_SPEED_MULTIPLIER).c_str());
	}

	{
		// GT2 keeps the default tempo in the reserved instrument slot's `ad`
		// byte, and only honours it while that slot carries no wavetable
		// pointer. With a pointer there the slot is a real instrument again,
		// the stored tempo is ignored, and a field that still accepted numbers
		// would be lying about what it does.
		const bool tempoOverrideAvailable = !ginstr[MAX_INSTR-1].ptr[WTBL];
		if (!tempoOverrideAvailable)
			ImGui::BeginDisabled();

		// No step arrows: the field opens an undo step when it takes focus and
		// closes it when it loses focus, so typing "12" is one entry rather
		// than one per keystroke. The toolbar's Tempo field owns the arrows.
		int tempo = GetSongTempo();
		if (ImGui::InputInt("Tempo (ticks/row)##gt2settings", &tempo, 0, 0))
			SetSongTempo(tempo);
		if (ImGui::IsItemActivated()) BeginEdit(GT2_SETTINGS_TEMPO);
		if (ImGui::IsItemDeactivated()) CommitEdit(GT2_SETTINGS_TEMPO);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_TEMPO).c_str());

		// The BPM this works out to, so the number means something musically.
		int framerate = IsNtsc() ? 60 : 50;
		int mult = GetSpeedMultiplier();
		float ticksPerSecond = mult ? (float)(framerate * mult) : (float)framerate / 2.0f;
		float bpm = (ticksPerSecond * 60.0f) / ((float)GetSongTempo() * 4.0f);
		ImGui::SameLine();
		ImGui::TextDisabled("= %.1f BPM at 4 rows/beat", bpm);

		if (!tempoOverrideAvailable)
		{
			ImGui::EndDisabled();
			ImGui::TextDisabled("instrument %d has a wavetable pointer, so it is an instrument,"
								" not a tempo override", MAX_INSTR - 1);
		}
	}

	ImGui::PopItemWidth();

	//
	// Player options
	//
	ImGui::SeparatorText("Player");

	ImGui::PushItemWidth(fieldWidth);

	{
		ImU16 hardRestart = (ImU16)GetHardRestartAdsr();
		if (ImGui::InputScalar("Hard restart ADSR##gt2settings", ImGuiDataType_U16, &hardRestart,
							   NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal))
			SetHardRestartAdsr((int)hardRestart);
		// No undo wrap: adparam is a config value, not song data, so it is not
		// in CGT2UndoSnapshot and CommitIfChanged() would record nothing.
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_HARD_RESTART).c_str());
	}

	{
		ImU16 address = (ImU16)GetSidAddress();
		if (ImGui::InputScalar("SID base address##gt2settings", ImGuiDataType_U16, &address,
							   NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal))
			SetSidAddress((int)address);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_SID_ADDRESS).c_str());
	}

	ImGui::PopItemWidth();

	{
		bool fineVibrato = IsFineVibrato();
		if (ImGui::Checkbox("Fine vibrato##gt2settings", &fineVibrato))
			SetFineVibrato(fineVibrato);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_FINE_VIBRATO).c_str());
	}

	{
		bool pulse = IsPulseOptimization();
		if (ImGui::Checkbox("Pulse optimization##gt2settings", &pulse))
			SetPulseOptimization(pulse);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_OPTIMIZE_PULSE).c_str());
	}

	{
		bool realtime = IsRealtimeOptimization();
		if (ImGui::Checkbox("Realtime effect optimization##gt2settings", &realtime))
			SetRealtimeOptimization(realtime);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_OPTIMIZE_REALTIME).c_str());
	}

	//
	// reSID emulation
	//
	ImGui::SeparatorText("Emulation (editor playback only)");

	ImGui::PushItemWidth(fieldWidth);

	{
		static const char *interpolationNames[] = {
			"Off", "Interpolate", "Distortion", "Distortion + interpolate"
		};
		int interpolation = GetInterpolation();
		if (interpolation < 0 || interpolation > GT2_MAX_INTERPOLATION)
			interpolation = 0;
		if (ImGui::Combo("reSID quality##gt2settings", &interpolation, interpolationNames, 4))
			SetInterpolation(interpolation);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_INTERPOLATION).c_str());
	}

	{
		int delay = GetResidWriteDelay();
		if (ImGui::SliderInt("Write delay (cycles)##gt2settings", &delay, 0, GT2_MAX_RESID_DELAY))
			SetResidWriteDelay(delay);
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_RESID_DELAY).c_str());
	}

	ImGui::PopItemWidth();

	//
	// Tuning
	//
	ImGui::SeparatorText("Tuning");

	ImGui::PushItemWidth(fieldWidth);

	{
		// Applied on release. The audio thread reads freqtbllo[n] and
		// freqtblhi[n] as two separate byte loads (gplay.c:608), so rebuilding
		// the table on every keystroke gives a note triggered mid-rebuild a
		// chance at a mismatched low/high pair. Once per committed value keeps
		// that window to a single edit instead of one per character.
		if (!basePitchEditing)
			basePitchEditValue = GetBasePitch();
		ImGui::InputFloat("A-4 pitch (Hz)##gt2settings", &basePitchEditValue, 0.0f, 0.0f, "%.4f");
		if (ImGui::IsItemActivated())
			basePitchEditing = true;
		if (ImGui::IsItemDeactivated())
		{
			basePitchEditing = false;
			SetBasePitch(basePitchEditValue);
		}
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_BASE_PITCH).c_str());
		if (GetBasePitch() == 0.0f)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(built-in table)");
		}
	}

	{
		if (!divisionsEditing)
			divisionsEditValue = GetDivisionsPerOctave();
		ImGui::InputFloat("Divisions per octave##gt2settings", &divisionsEditValue, 0.0f, 0.0f, "%.7f");
		if (ImGui::IsItemActivated())
			divisionsEditing = true;
		if (ImGui::IsItemDeactivated())
		{
			divisionsEditing = false;
			SetDivisionsPerOctave(divisionsEditValue);
		}
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_DIVISIONS_PER_OCTAVE).c_str());
	}

	ImGui::PopItemWidth();

	{
		if (!scalaPathEditing)
			scalaPathEditValue = GetScalaTuningPath();
		strncpy(buf, scalaPathEditValue.c_str(), MAX_PATHNAME - 1);
		buf[MAX_PATHNAME - 1] = 0;
		ImGui::PushItemWidth(fieldWidth * 1.6f);
		bool submitted = ImGui::InputText("Scala .scl##gt2settings", buf, MAX_PATHNAME,
										  ImGuiInputTextFlags_EnterReturnsTrue);
		if (ImGui::IsItemEdited())
			scalaPathEditValue = buf;
		if (ImGui::IsItemActivated())
			scalaPathEditing = true;
		if (ImGui::IsItemDeactivated())
			scalaPathEditing = false;
		ImGui::SetItemTooltip("%s", GetControlTooltip(GT2_SETTINGS_SCALA_FILE).c_str());
		ImGui::PopItemWidth();
		ImGui::SameLine();
		// Clicking Load takes focus off the field, so read the edit buffer
		// rather than whatever the widget last wrote back.
		if (ImGui::Button("Load##gt2settingsscala") || submitted)
			LoadScalaTuning(scalaPathEditValue.c_str());

		std::string scalaError = GetScalaLoadError();
		if (!scalaError.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f),
							   "not loaded: %s", scalaError.c_str());
		}
		else if (GetTuningStepCount() > 0)
		{
			ImGui::TextDisabled("%s -- %d steps", GetTuningName().c_str(), GetTuningStepCount());
			if (IsScalaTuningInert())
			{
				// Saying nothing here would mean the scale is loaded, listed,
				// and inaudible -- GT2 derives every note from the base pitch,
				// and at 0 there is nothing for the scale to scale.
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f),
								   "scale is not in use: set an A-4 pitch above to apply it");
			}
		}
		else
		{
			ImGui::TextDisabled("no scale loaded, using equal divisions");
		}
	}

	ImGui::TextDisabled("These settings are written to goattrk2.cfg when RetroDebugger exits.");

	ImGui::PopStyleVar(2);

	PostRenderImGui();
}

//
// Keyboard
//

bool CViewGT2SongSettings::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Typing a song title must not double as note entry.
	if (ImGui::GetIO().WantTextInput)
		return false;

	// Past that this window owns no keyboard of its own -- it is a settings
	// panel, and clicking a checkbox in it must not cost the user the pattern
	// editor's keys. Delegate exactly as CViewGT2Toolbar does, and for the
	// same reason: GT2_HandleRenoiseOrForwardKeyDown sees only the Renoise
	// dispatcher's global bindings and its native fallback is a no-op under
	// KEY_RENOISE, so routing there would silently drop note entry, cursor
	// movement and every Shift+letter pattern op.
	if (plugin && plugin->viewPatterns)
		return plugin->viewPatterns->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);

	return GT2_HandleRenoiseOrForwardKeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2SongSettings::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (ImGui::GetIO().WantTextInput)
		return false;
	// Mirror KeyDown's delegation, so a note started while this window holds
	// focus is released by the code that started it.
	if (plugin && plugin->viewPatterns)
		return plugin->viewPatterns->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2SongSettings::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
