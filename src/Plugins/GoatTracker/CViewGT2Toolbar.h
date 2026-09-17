#ifndef _CViewGT2Toolbar_H_
#define _CViewGT2Toolbar_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "ImGuiToolbar.h"
// GT2_SONG_TEMPO_MIN/MAX and the shared tempo accessors the Tempo field wraps.
#include "GT2ViewCommon.h"
#include <string>

class C64DebuggerPluginGoatTracker;

// Every control the toolbar draws. Its tooltip is built by
// CViewGT2Toolbar::GetControlTooltip() rather than written inline at the draw
// call, so the regression test can read exactly the text the user hovers --
// which is what keeps the shortcut hints from drifting away from the bindings
// they name.
enum GT2ToolbarControl
{
	GT2_TOOLBAR_PLAY = 0,
	GT2_TOOLBAR_PAUSE,            // same button as PLAY, shown while the song runs
	GT2_TOOLBAR_LOOP,
	GT2_TOOLBAR_STOP,
	GT2_TOOLBAR_FOLLOW,
	GT2_TOOLBAR_METRONOME,
	GT2_TOOLBAR_UNDO,
	GT2_TOOLBAR_REDO,
	GT2_TOOLBAR_TEMPO,
	GT2_TOOLBAR_TEMPO_DEC,
	GT2_TOOLBAR_TEMPO_INC,
	GT2_TOOLBAR_HIGHLIGHT,
	GT2_TOOLBAR_HIGHLIGHT_DEC,
	GT2_TOOLBAR_HIGHLIGHT_INC,
	GT2_TOOLBAR_EDIT_STEP,
	GT2_TOOLBAR_EDIT_STEP_DEC,
	GT2_TOOLBAR_EDIT_STEP_INC,
	GT2_TOOLBAR_OCTAVE,
	GT2_TOOLBAR_OCTAVE_DEC,
	GT2_TOOLBAR_OCTAVE_INC,
	GT2_TOOLBAR_CONTROL_COUNT
};

class CViewGT2Toolbar : public CGuiView
{
public:
	CViewGT2Toolbar(const char *name, float posX, float posY, float posZ,
					  float sizeX, float sizeY, C64DebuggerPluginGoatTracker *plugin);
	virtual ~CViewGT2Toolbar();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	bool TriggerPlayPause();
	bool TriggerStop();
	bool TriggerUndo();
	bool TriggerRedo();
	bool ToggleLoopCurrentPattern();
	bool ToggleFollowPattern();
	bool ToggleMetronome();
	void AdjustOctave(int delta);
	// The song's default tempo. Thin wrappers over GT2_GetSongTempo() /
	// GT2_SetSongTempo() (GT2ViewCommon.h), which is where the reserved-slot
	// rule lives -- the Song Settings view shows the same number.
	int GetSongTempo() const;
	void SetSongTempo(int tempo);
	void AdjustSongTempo(int delta);
	void SetOctave(int octave);
	int GetOctaveEditValue() const;
	void SetOctaveEditValue(int octave);
	bool CanUndo() const;
	bool CanRedo() const;
	bool IsPlaybackActive() const;
	bool IsLoopCurrentPatternEnabled() const;
	bool IsFollowPatternEnabled() const;
	bool IsMetronomeEnabled() const;

	// What the control does, then "Shortcut: <keys>" on its own line when it
	// has one. Renoise-layout bindings are named only under that preset.
	std::string GetControlTooltip(int control) const;

	C64DebuggerPluginGoatTracker *plugin;
	ImGuiToolbar toolbar;
};

#endif
