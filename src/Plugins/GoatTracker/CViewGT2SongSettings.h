#ifndef _CViewGT2SongSettings_H_
#define _CViewGT2SongSettings_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGT2UndoHistory.h"
#include <string>

class C64DebuggerPluginGoatTracker;

// Every control the Song Settings view draws. Its tooltip comes from
// GetControlTooltip() rather than a literal at the draw call, so the
// regression test reads exactly the text the user hovers -- the same
// arrangement CViewGT2Toolbar uses, and for the same reason: a control added
// without an explanation is the failure mode this catches.
enum GT2SongSettingsControl
{
	GT2_SETTINGS_NAME = 0,
	GT2_SETTINGS_AUTHOR,
	GT2_SETTINGS_COPYRIGHT,
	GT2_SETTINGS_SID_MODEL,
	GT2_SETTINGS_VIDEO_STANDARD,
	GT2_SETTINGS_CUSTOM_CLOCK,
	GT2_SETTINGS_SPEED_MULTIPLIER,
	GT2_SETTINGS_TEMPO,
	GT2_SETTINGS_HARD_RESTART,
	GT2_SETTINGS_FINE_VIBRATO,
	GT2_SETTINGS_OPTIMIZE_PULSE,
	GT2_SETTINGS_OPTIMIZE_REALTIME,
	GT2_SETTINGS_SID_ADDRESS,
	GT2_SETTINGS_INTERPOLATION,
	GT2_SETTINGS_RESID_DELAY,
	GT2_SETTINGS_BASE_PITCH,
	GT2_SETTINGS_DIVISIONS_PER_OCTAVE,
	GT2_SETTINGS_SCALA_FILE,
	GT2_SETTINGS_CONTROL_COUNT
};

// GT2's own validation, from goattrk2.c:400 onwards. Repeated here because the
// setters have to hold the same line at runtime that the config reader holds
// at startup -- a value the reader would have rejected must not be reachable
// through the UI either.
#define GT2_MAX_SPEED_MULTIPLIER 16
#define GT2_MIN_CUSTOM_CLOCK_RATE 100
#define GT2_MAX_RESID_DELAY 63
#define GT2_MAX_INTERPOLATION 3

// Song-level settings: what the .sng carries, what the SID header carries, and
// what the packed player is built with. Everything is reached through static
// accessors so the regression test can drive it with no window and no plugin
// instance -- see the song-settings design notes.
class CViewGT2SongSettings : public CGuiView
{
public:
	CViewGT2SongSettings(const char *name, float posX, float posY, float posZ,
						 float sizeX, float sizeY, C64DebuggerPluginGoatTracker *plugin);
	virtual ~CViewGT2SongSettings();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	// --- Song information. Written to the .sng and to the SID header. ---
	static std::string GetSongName();
	static void SetSongName(const char *value);
	static std::string GetAuthorName();
	static void SetAuthorName(const char *value);
	static std::string GetCopyrightName();
	static void SetCopyrightName(const char *value);

	// --- Chip and timing. These reach reSID only through ApplySoundSettings(). ---
	static int GetSidModel();              // 0 = 6581, 1 = 8580
	static void SetSidModel(int model);
	static bool IsNtsc();
	static void SetNtsc(bool value);
	static int GetCustomClockRate();       // 0 = the PAL/NTSC default
	static void SetCustomClockRate(int hz);
	static int GetSpeedMultiplier();       // 0 = 25Hz, 1..16 = calls per frame
	static void SetSpeedMultiplier(int value);

	// The song's default tempo, in ticks per row. Shared with the toolbar's
	// Tempo field -- both call GT2_GetSongTempo() / GT2_SetSongTempo().
	static int GetSongTempo();
	static void SetSongTempo(int tempo);

	// --- Player options. These end up in the packed tune. ---
	static int GetHardRestartAdsr();       // 16-bit AD/SR pair
	static void SetHardRestartAdsr(int value);
	static bool IsFineVibrato();
	static void SetFineVibrato(bool value);
	static bool IsPulseOptimization();
	static void SetPulseOptimization(bool value);
	static bool IsRealtimeOptimization();
	static void SetRealtimeOptimization(bool value);
	static int GetSidAddress();
	static void SetSidAddress(int address);

	// --- reSID emulation. ---
	static int GetInterpolation();         // bit 0 = interpolate, bit 1 = distortion
	static void SetInterpolation(int value);
	static int GetResidWriteDelay();       // 0..63 cycles
	static void SetResidWriteDelay(int cycles);

	// --- Tuning. ---
	static float GetBasePitch();           // A-4 in Hz, 0 = the built-in table
	static void SetBasePitch(float hz);
	static float GetDivisionsPerOctave();
	static void SetDivisionsPerOctave(float divisions);
	static std::string GetScalaTuningPath();
	// Reads a Scala .scl, adopts its note names and rebuilds the frequency
	// table. An empty path drops back to equal temperament.
	static void LoadScalaTuning(const char *path);
	static std::string GetTuningName();
	static int GetTuningStepCount();
	// Why the last LoadScalaTuning() did not take, or "" when it did.
	static std::string GetScalaLoadError();
	// A scale is loaded but nothing derives from it, because equal temperament
	// is still in force -- a Scala scale needs a base pitch to hang off.
	static bool IsScalaTuningInert();

	// Hands the current sidmodel / ntsc / multiplier / custom clock /
	// interpolation to reSID -- the five that only reach it through a full
	// re-init. A no-op while the GT2 engine is not up.
	static void ApplySoundSettings();

	// What the control does, and what changing it affects. Static like the
	// accessors: the test reads the strings without standing a view up.
	static std::string GetControlTooltip(int control);

	C64DebuggerPluginGoatTracker *plugin;

	// Reaches BeginEdit/CommitEdit, so the field-to-field undo handoff is
	// covered by a test rather than by reasoning about ImGui frame order.
	friend class CTestGT2SongSettings;

private:
	// One undo entry per field visit, not per keystroke: captured when a field
	// takes focus, committed when it loses it. The pending step remembers
	// WHICH control opened it, because ImGui can activate the next field in
	// the same frame it deactivates the previous one -- and, when the new
	// field is drawn earlier in the window, in that order.
	void BeginEdit(int control);
	void CommitEdit(int control);

	CGT2UndoSnapshot pendingSnapshot;
	int pendingSnapshotControl;   // GT2SongSettingsControl, or -1 for none

	// Fields whose setter re-initialises reSID are edited into these and
	// applied when the widget is released, so dragging a slider or typing a
	// clock rate costs one teardown instead of one per frame. See
	// ApplySoundSettings() for why that teardown is not free.
	int customClockEditValue;
	bool customClockEditing;
	int speedMultiplierEditValue;
	bool speedMultiplierEditing;

	// The Scala path is applied by a button next to the field, so the text has
	// to survive the frames in which ImGui reports no change -- a buffer
	// refilled from the global every frame would hand the button the OLD path.
	std::string scalaPathEditValue;
	bool scalaPathEditing;

	float basePitchEditValue;
	bool basePitchEditing;
	float divisionsEditValue;
	bool divisionsEditing;

	// Drops a step whose field stopped being drawn while it was open. See the
	// implementation for why it discards rather than commits.
	void DropOrphanedEdit();
};

#endif
