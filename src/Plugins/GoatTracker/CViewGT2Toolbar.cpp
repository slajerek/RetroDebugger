#include "CViewGT2Toolbar.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2RenoiseInput.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2Tables.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "IconsFontAwesome_c.h"
#include "imgui.h"
#include "imgui_internal.h"   // ArrowButtonEx (explicit-size arrow button)

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gorder.h"
#include "gsong.h"
#include "gsid.h"
extern int eamode, menu, followplay, songinit;
extern int stepsize;            // GT2 step size; also the row-highlight interval
extern int epoctave;            // GT2 keyboard octave, clamped to 0..7
extern int gt2RenoiseEditStep;  // Renoise-layout cursor advance
extern unsigned keypreset;
extern unsigned multiplier;     // player frame-rate multiple (-S), 0 = 25Hz mode
}
#include <cstring>
#include <string>

extern bool gt2RenoiseFollowTrack;
extern bool gt2MetronomeEnabled;

#define KEY_RENOISE 4

static bool GT2ToolbarButton(ImGuiToolbar &toolbar, const char *icon, const char *tooltip, bool active = false, bool enabled = true)
{
	if (active)
	{
		// Toggled-on state: a bright, saturated fill so it is clearly
		// distinguishable from the default (untoggled) button shade.
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.60f, 1.00f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.72f, 1.00f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.14f, 0.48f, 0.86f, 1.0f));
	}
	bool pressed = toolbar.Button(icon, tooltip, enabled);
	if (active)
	{
		ImGui::PopStyleColor(3);
	}
	return pressed;
}

// A toolbar tooltip: what the control does, then its keyboard shortcut on its
// own line when it has one. Controls with no shortcut pass NULL, and the line
// is left out rather than saying "none" -- there are only three of them and
// the silence is the answer.
//
// The command-modifier word comes from GT2_CmdKey(), so it reads "Cmd" on
// macOS and "Ctrl" elsewhere; it is never hardcoded. Keep every string here in
// step with the "Renoise Shortcuts" reference in
// C64DebuggerPluginGoatTracker::RenderMainMenuImGui() and with the pattern
// context menu -- those are the other two places the same bindings are named.
static std::string GT2ToolbarTip(const char *what, const char *shortcut)
{
	std::string tip = what;
	if (shortcut != NULL && shortcut[0] != 0)
	{
		tip += "\nShortcut: ";
		tip += shortcut;
	}
	return tip;
}

CViewGT2Toolbar::CViewGT2Toolbar(const char *name, float posX, float posY, float posZ,
								 float sizeX, float sizeY, C64DebuggerPluginGoatTracker *plugin)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->plugin = plugin;
	imGuiNoScrollbar = true;
}

CViewGT2Toolbar::~CViewGT2Toolbar()
{
}

bool CViewGT2Toolbar::TriggerPlayPause()
{
	// This button is tooltipped as Space, so it delegates to Space rather than
	// reimplementing it. Parity is then structural: whatever Space does --
	// including which flavour of stop gt2KeepPlayingOnStop selects -- this
	// button does too, and a later change to one cannot drift from the other.
	//
	// It previously had its own copy of the transport logic, which had already
	// drifted in two ways: while playing it ran TriggerStop() (a full player
	// reset) where Space only stops, and when starting it synced espos only
	// when eseditpos != espos[eschn] where Space always syncs.
	if (pluginGoatTracker != NULL && pluginGoatTracker->renoiseInput != NULL)
		return pluginGoatTracker->renoiseInput->HandlePlayStop(false);

	// No renoise dispatcher yet (very early init): fall back to the same
	// sequence HandlePlayStop runs, so behaviour still matches.
	if (eamode || menu) return false;

	if (GT2_IsTransportActive())
	{
		GT2_StopSong();
		return true;
	}

	for (int c = 0; c < MAX_CHN; c++)
	{
		if (eseditpos < songlen[esnum][c]) espos[c] = eseditpos;
		if (esend[c] <= espos[c]) esend[c] = 0;
	}
	initsongpos(esnum, PLAY_POS, 0);
	followplay = gt2RenoiseFollowTrack ? 1 : 0;
	return true;
}

bool CViewGT2Toolbar::TriggerStop()
{
	// Total player reset: stop transport, wipe every channel's runtime
	// state (gate / arp position / current note / current instr / tempo
	// counters), and zero the SID register mirror so the next sid->clock
	// silences any leftover envelopes. Equivalent to the player state you
	// get right after a fresh `loadsong()` — song / instrument / pattern
	// DATA is left intact (we don't call clearsong), but no notes are
	// sustaining, no arps are mid-cycle, no funktable is mid-swap.
	stopsong();
	initchannels();
	memset(sidreg, 0, NUMSIDREGS);
	followplay = 0;
	return true;
}

bool CViewGT2Toolbar::TriggerUndo()
{
	// One shared timeline -- see CGT2UndoHistory.h. The button no longer picks
	// a history by edit mode; it steps back through the last change wherever
	// in the editor it was made.
	return plugin && plugin->viewPatterns && plugin->viewPatterns->UndoPatternEdit();
}

bool CViewGT2Toolbar::TriggerRedo()
{
	return plugin && plugin->viewPatterns && plugin->viewPatterns->RedoPatternEdit();
}

bool CViewGT2Toolbar::ToggleLoopCurrentPattern()
{
	gt2LoopCurrentPattern = gt2LoopCurrentPattern ? 0 : 1;
	return IsLoopCurrentPatternEnabled();
}

bool CViewGT2Toolbar::ToggleFollowPattern()
{
	gt2RenoiseFollowTrack = !gt2RenoiseFollowTrack;
	if (gt2RenoiseFollowTrack)
	{
		if (IsPlaybackActive()) followplay = 1;
	}
	else
	{
		followplay = 0;
	}
	return gt2RenoiseFollowTrack;
}

bool CViewGT2Toolbar::ToggleMetronome()
{
	gt2MetronomeEnabled = !gt2MetronomeEnabled;
	return gt2MetronomeEnabled;
}

void CViewGT2Toolbar::AdjustOctave(int delta)
{
	SetOctave(epoctave + delta);
}

void CViewGT2Toolbar::SetOctave(int octave)
{
	if (octave < 0) octave = 0;
	if (octave > 7) octave = 7;
	epoctave = octave;
}

int CViewGT2Toolbar::GetOctaveEditValue() const
{
	return epoctave + 1;
}

void CViewGT2Toolbar::SetOctaveEditValue(int octave)
{
	SetOctave(octave - 1);
}

bool CViewGT2Toolbar::IsPlaybackActive() const
{
	return GT2_IsTransportActive();
}

bool CViewGT2Toolbar::CanUndo() const
{
	return plugin && plugin->viewPatterns && plugin->viewPatterns->CanUndoPatternEdit();
}

bool CViewGT2Toolbar::CanRedo() const
{
	return plugin && plugin->viewPatterns && plugin->viewPatterns->CanRedoPatternEdit();
}

bool CViewGT2Toolbar::IsLoopCurrentPatternEnabled() const
{
	return gt2LoopCurrentPattern != 0;
}

bool CViewGT2Toolbar::IsFollowPatternEnabled() const
{
	return gt2RenoiseFollowTrack;
}

bool CViewGT2Toolbar::IsMetronomeEnabled() const
{
	return gt2MetronomeEnabled;
}

int CViewGT2Toolbar::GetSongTempo() const
{
	return GT2_GetSongTempo();
}

void CViewGT2Toolbar::SetSongTempo(int tempo)
{
	GT2_SetSongTempo(tempo);
}

void CViewGT2Toolbar::AdjustSongTempo(int delta)
{
	SetSongTempo(GetSongTempo() + delta);
}

std::string CViewGT2Toolbar::GetControlTooltip(int control) const
{
	// Renoise-layout bindings are named only under that preset, because
	// CGT2RenoiseInput::HandleKey ignores them elsewhere. Undo/redo and the
	// highlight step are handled in CViewGT2Patterns with no preset gate, so
	// they are named unconditionally.
	const bool renoise = (keypreset == KEY_RENOISE);
	const char *cmd = GT2_CmdKey();
	char sc[224];

	switch (control)
	{
	case GT2_TOOLBAR_PLAY:
		return GT2ToolbarTip("Play the current pattern from row 0.",
			renoise ? "Space   (Shift+Space plays from the cursor row)" : NULL);

	case GT2_TOOLBAR_PAUSE:
		// The icon says pause, but while the song runs this stops it -- so the
		// tooltip says what actually happens. Which flavour of stop depends on
		// "Keep Playing on Stop": off routes to TriggerStop(), on leaves the
		// voices ringing (see TriggerPlayPause).
		return GT2ToolbarTip(gt2KeepPlayingOnStop
				? "Stop playback, letting whatever is sounding ring out.\n"
				  "Pressing it again starts the current pattern from row 0."
				: "Stop playback.\n"
				  "Pressing it again starts the current pattern from row 0.",
			renoise ? "Space" : NULL);

	case GT2_TOOLBAR_LOOP:
		return GT2ToolbarTip("Loop the current pattern.\n"
							 "At the end of a pattern every channel restarts it instead of\n"
							 "advancing through the song order.", NULL);

	case GT2_TOOLBAR_STOP:
		return GT2ToolbarTip("Stop and reset the player: silences the SID, clears gates,\n"
							 "arpeggios and tempo counters, and turns Follow off.\n"
							 "Song data is untouched. Space only stops playback; this also\n"
							 "resets the player state.", NULL);

	case GT2_TOOLBAR_FOLLOW:
		return GT2ToolbarTip("Follow playback: scroll the pattern editor to the row being\n"
							 "played. Turning it on while the song runs starts following\n"
							 "immediately.", NULL);

	case GT2_TOOLBAR_METRONOME:
		return GT2ToolbarTip("Metronome.\n"
							 "Not implemented yet -- the toggle is remembered but silent.", NULL);

	case GT2_TOOLBAR_UNDO:
		snprintf(sc, sizeof(sc), "%s+Z", cmd);
		return GT2ToolbarTip("Undo the last change anywhere in the editor -- a pattern edit, a\n"
							 "table edit, an instrument edit or an instrument load. One shared\n"
							 "history, so it steps back through them in the order you made them.", sc);

	case GT2_TOOLBAR_REDO:
		snprintf(sc, sizeof(sc), "%s+Y", cmd);
		return GT2ToolbarTip("Redo the change that was last undone.", sc);

	case GT2_TOOLBAR_TEMPO:
		return GT2ToolbarTip("Song tempo: how many player ticks each pattern row lasts.\n"
							 "Lower is faster; 6 is GT2's default. Saved with the song as the\n"
							 "default tempo, on the same scale as the F command -- an F in a\n"
							 "pattern still overrides it from the row it sits on.", NULL);

	case GT2_TOOLBAR_TEMPO_DEC:
		return GT2ToolbarTip("Song tempo -1 (faster).", NULL);

	case GT2_TOOLBAR_TEMPO_INC:
		return GT2ToolbarTip("Song tempo +1 (slower).", NULL);

	case GT2_TOOLBAR_HIGHLIGHT:
		return GT2ToolbarTip("Row highlight interval: every Nth row number is drawn highlighted\n"
							 "in the pattern editor, so bars stay countable. Display only -- it\n"
							 "does not affect playback or note entry.", "Shift+M / Shift+N");

	case GT2_TOOLBAR_HIGHLIGHT_DEC:
		return GT2ToolbarTip("Highlight rows more often (interval -1).", "Shift+N");

	case GT2_TOOLBAR_HIGHLIGHT_INC:
		return GT2ToolbarTip("Highlight rows less often (interval +1).", "Shift+M");

	// The Step and Oct widgets are only drawn under KEY_RENOISE, and their
	// bindings only fire there too -- but the gate is written out anyway, so
	// the accessor answers honestly whoever asks and whatever the preset.
	case GT2_TOOLBAR_EDIT_STEP:
		snprintf(sc, sizeof(sc), "`  +1,   ~  -1,   %s+0...%s+9 sets 0-9,   Alt+= doubles,   Alt+- halves", cmd, cmd);
		return GT2ToolbarTip("Edit step: how many rows the cursor moves down after you enter a\n"
							 "note or a value. 0 keeps the cursor on the same row.",
			renoise ? sc : NULL);

	case GT2_TOOLBAR_EDIT_STEP_DEC:
		return GT2ToolbarTip("Edit step -1.", renoise ? "~" : NULL);

	case GT2_TOOLBAR_EDIT_STEP_INC:
		return GT2ToolbarTip("Edit step +1.", renoise ? "`" : NULL);

	case GT2_TOOLBAR_OCTAVE:
		snprintf(sc, sizeof(sc), "Num* / Num/ ,   %s+] / %s+[", cmd, cmd);
		return GT2ToolbarTip("Octave the computer keyboard plays and records in, 1-8.\n"
							 "The note keys are laid out over two octaves starting here.",
			renoise ? sc : NULL);

	case GT2_TOOLBAR_OCTAVE_DEC:
		snprintf(sc, sizeof(sc), "Num/   or   %s+[", cmd);
		return GT2ToolbarTip("Octave down.", renoise ? sc : NULL);

	case GT2_TOOLBAR_OCTAVE_INC:
		snprintf(sc, sizeof(sc), "Num*   or   %s+]", cmd);
		return GT2ToolbarTip("Octave up.", renoise ? sc : NULL);
	}
	return std::string();
}

void CViewGT2Toolbar::RenderImGui()
{
	PreRenderImGui();
	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
		ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
		ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));

	if (toolbar.BeginToolbar("gt2Toolbar"))
	{
		// Tooltip text -- including which shortcut, if any, drives each control
		// -- lives in GetControlTooltip(), so the test can read the same
		// strings the user hovers.
		const bool playing = IsPlaybackActive();
		std::string playPauseTip = GetControlTooltip(playing ? GT2_TOOLBAR_PAUSE : GT2_TOOLBAR_PLAY);
		if (GT2ToolbarButton(toolbar, playing ? ICON_FA_PAUSE : ICON_FA_PLAY, playPauseTip.c_str()))
		{
			TriggerPlayPause();
		}

		std::string loopTip = GetControlTooltip(GT2_TOOLBAR_LOOP);
		if (GT2ToolbarButton(toolbar, ICON_FA_REPEAT, loopTip.c_str(), IsLoopCurrentPatternEnabled()))
		{
			ToggleLoopCurrentPattern();
		}

		std::string stopTip = GetControlTooltip(GT2_TOOLBAR_STOP);
		if (GT2ToolbarButton(toolbar, ICON_FA_STOP, stopTip.c_str()))
		{
			TriggerStop();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x * 4.0f, ImGui::GetStyle().ItemSpacing.y));
		std::string followTip = GetControlTooltip(GT2_TOOLBAR_FOLLOW);
		if (GT2ToolbarButton(toolbar, ICON_FA_LOCATION_ARROW, followTip.c_str(), IsFollowPatternEnabled()))
		{
			ToggleFollowPattern();
		}
		ImGui::PopStyleVar();

		std::string metronomeTip = GetControlTooltip(GT2_TOOLBAR_METRONOME);
		if (GT2ToolbarButton(toolbar, ICON_FA_BELL, metronomeTip.c_str(), IsMetronomeEnabled()))
		{
			ToggleMetronome();
		}

		// New section: Undo / Redo
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x * 4.0f, ImGui::GetStyle().ItemSpacing.y));
		std::string undoTip = GetControlTooltip(GT2_TOOLBAR_UNDO);
		if (GT2ToolbarButton(toolbar, ICON_FA_UNDO, undoTip.c_str(), false, CanUndo()))
		{
			TriggerUndo();
		}
		ImGui::PopStyleVar();
		std::string redoTip = GetControlTooltip(GT2_TOOLBAR_REDO);
		if (GT2ToolbarButton(toolbar, ICON_FA_SHARE, redoTip.c_str(), false, CanRedo()))
		{
			TriggerRedo();
		}

		// New section: row-highlight interval and (Renoise) edit step.
		// Each value gets ImGui ArrowButtons (decrement / increment) on its
		// left, then the number field. The label, both arrows and the field
		// all carry a tooltip -- the arrows name the one direction they move.
		// Arrow buttons: full textbox height, narrow width, tight gap.
		const ImVec2 kArrowSize(ImGui::GetFontSize() + 2.0f, ImGui::GetFrameHeight());
		const ImVec2 kArrowGap(1.0f * uiScale, ImGui::GetStyle().ItemSpacing.y);

		// Song tempo. Unlike Hl / Step / Oct this is real song data -- it is
		// saved in the .sng -- so an edit is an undo step. The arrows are one
		// step per click; the field would otherwise push an entry per
		// keystroke, so its step opens when the field takes focus and closes
		// when it loses it. The close is an unconditional Commit, not a
		// Cancel: CommitIfChanged() records nothing when the value did not move,
		// and a Cancel here would throw away an arrow click made while the
		// field happened to be focused.
		CViewGT2Tables *tablesView = plugin ? plugin->viewTables : NULL;
		std::string tempoTip = GetControlTooltip(GT2_TOOLBAR_TEMPO);
		std::string tempoDecTip = GetControlTooltip(GT2_TOOLBAR_TEMPO_DEC);
		std::string tempoIncTip = GetControlTooltip(GT2_TOOLBAR_TEMPO_INC);

		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x * 4.0f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Tempo");
		ImGui::SetItemTooltip("%s", tempoTip.c_str());
		ImGui::SameLine();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, kArrowGap);
		if (ImGui::ArrowButtonEx("##gt2tempo_dec", ImGuiDir_Left,  kArrowSize))
		{
			if (tablesView) tablesView->BeginTableUndoStep();
			AdjustSongTempo(-1);
			if (tablesView) tablesView->CommitTableUndoStep();
		}
		ImGui::SetItemTooltip("%s", tempoDecTip.c_str());
		ImGui::SameLine();
		if (ImGui::ArrowButtonEx("##gt2tempo_inc", ImGuiDir_Right, kArrowSize))
		{
			if (tablesView) tablesView->BeginTableUndoStep();
			AdjustSongTempo(1);
			if (tablesView) tablesView->CommitTableUndoStep();
		}
		ImGui::SetItemTooltip("%s", tempoIncTip.c_str());
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(46.0f * uiScale);
		int tempoEditValue = GetSongTempo();
		if (ImGui::InputInt("##gt2tempo", &tempoEditValue, 0, 0)) SetSongTempo(tempoEditValue);
		if (ImGui::IsItemActivated() && tablesView) tablesView->BeginTableUndoStep();
		if (ImGui::IsItemDeactivated() && tablesView) tablesView->CommitTableUndoStep();
		ImGui::SetItemTooltip("%s", tempoTip.c_str());

		std::string hlTip = GetControlTooltip(GT2_TOOLBAR_HIGHLIGHT);
		std::string hlDecTip = GetControlTooltip(GT2_TOOLBAR_HIGHLIGHT_DEC);
		std::string hlIncTip = GetControlTooltip(GT2_TOOLBAR_HIGHLIGHT_INC);

		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x * 2.0f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Hl");
		ImGui::SetItemTooltip("%s", hlTip.c_str());
		ImGui::SameLine();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, kArrowGap);
		if (ImGui::ArrowButtonEx("##gt2hl_dec", ImGuiDir_Left,  kArrowSize)) stepsize--;
		ImGui::SetItemTooltip("%s", hlDecTip.c_str());
		ImGui::SameLine();
		if (ImGui::ArrowButtonEx("##gt2hl_inc", ImGuiDir_Right, kArrowSize)) stepsize++;
		ImGui::SetItemTooltip("%s", hlIncTip.c_str());
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(46.0f * uiScale);
		ImGui::InputInt("##gt2hl", &stepsize, 0, 0);
		if (stepsize < 1) stepsize = 1;   // 0 would divide-by-zero in the row highlight
		ImGui::SetItemTooltip("%s", hlTip.c_str());

		if (keypreset == KEY_RENOISE)
		{
			std::string stepTip = GetControlTooltip(GT2_TOOLBAR_EDIT_STEP);
			std::string stepDecTip = GetControlTooltip(GT2_TOOLBAR_EDIT_STEP_DEC);
			std::string stepIncTip = GetControlTooltip(GT2_TOOLBAR_EDIT_STEP_INC);

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Step");
			ImGui::SetItemTooltip("%s", stepTip.c_str());
			ImGui::SameLine();
			bool stepChanged = false;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, kArrowGap);
			if (ImGui::ArrowButtonEx("##gt2step_dec", ImGuiDir_Left,  kArrowSize)) { gt2RenoiseEditStep--; stepChanged = true; }
			ImGui::SetItemTooltip("%s", stepDecTip.c_str());
			ImGui::SameLine();
			if (ImGui::ArrowButtonEx("##gt2step_inc", ImGuiDir_Right, kArrowSize)) { gt2RenoiseEditStep++; stepChanged = true; }
			ImGui::SetItemTooltip("%s", stepIncTip.c_str());
			ImGui::PopStyleVar();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(46.0f * uiScale);
			if (ImGui::InputInt("##gt2step", &gt2RenoiseEditStep, 0, 0)) stepChanged = true;
			if (gt2RenoiseEditStep < 0) gt2RenoiseEditStep = 0;
			if (stepChanged) PLUGIN_GoatTrackerSaveSettings();
			ImGui::SetItemTooltip("%s", stepTip.c_str());

			std::string octTip = GetControlTooltip(GT2_TOOLBAR_OCTAVE);
			std::string octDecTip = GetControlTooltip(GT2_TOOLBAR_OCTAVE_DEC);
			std::string octIncTip = GetControlTooltip(GT2_TOOLBAR_OCTAVE_INC);

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Oct");
			ImGui::SetItemTooltip("%s", octTip.c_str());
			ImGui::SameLine();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, kArrowGap);
			if (ImGui::ArrowButtonEx("##gt2oct_dec", ImGuiDir_Left,  kArrowSize)) AdjustOctave(-1);
			ImGui::SetItemTooltip("%s", octDecTip.c_str());
			ImGui::SameLine();
			if (ImGui::ArrowButtonEx("##gt2oct_inc", ImGuiDir_Right, kArrowSize)) AdjustOctave(1);
			ImGui::SetItemTooltip("%s", octIncTip.c_str());
			ImGui::PopStyleVar();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(46.0f * uiScale);
			int octaveEditValue = GetOctaveEditValue();
			if (ImGui::InputInt("##gt2oct", &octaveEditValue, 0, 0)) SetOctaveEditValue(octaveEditValue);
			ImGui::SetItemTooltip("%s", octTip.c_str());
		}

		toolbar.EndToolbar();
	}
	ImGui::PopStyleVar(2);

	PostRenderImGui();
}

bool CViewGT2Toolbar::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// While a toolbar input field (Hl / Step / Oct) is being edited, keystrokes
	// belong to ImGui.
	if (ImGui::GetIO().WantTextInput)
		return false;

	// Past that, the toolbar owns no keyboard of its own. It is a control
	// strip: clicking a button here — bumping the octave, arming Follow —
	// must not cost the user the pattern editor's keys. Same principle as
	// the Instrument / Tables / Instrument-list views, which hand every key
	// they do not claim themselves to whatever actually owns editing in that
	// context (there: native GT2, via
	// GT2_HandleRenoiseOrForwardKeyDownToNative). Here that owner is
	// CViewGT2Patterns.
	//
	// GT2_HandleRenoiseOrForwardKeyDown is NOT that owner and cannot stand in
	// for it: it sees only the Renoise dispatcher's global bindings, and its
	// native fallback (GT2_ForwardKeyDown) is a deliberate no-op under
	// KEY_RENOISE. Note entry, hex digits on the instrument / command
	// columns, cursor navigation, the selection shortcuts and the
	// Shift+letter pattern ops all live in CViewGT2Patterns::KeyDown, so
	// routing there silently dropped every one of them.
	if (plugin && plugin->viewPatterns)
		return plugin->viewPatterns->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);

	return GT2_HandleRenoiseOrForwardKeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2Toolbar::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (ImGui::GetIO().WantTextInput)
		return false;
	// Mirror KeyDown's delegation, so a note started while the toolbar holds
	// focus is released by the same code that started it.
	if (plugin && plugin->viewPatterns)
		return plugin->viewPatterns->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2Toolbar::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
