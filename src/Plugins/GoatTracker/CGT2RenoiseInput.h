#ifndef _CGT2RenoiseInput_H_
#define _CGT2RenoiseInput_H_

#include "SYS_Defs.h"

class C64DebuggerPluginGoatTracker;

// Renoise keyboard-layout dispatcher. Owned by C64DebuggerPluginGoatTracker.
// Every GT2 view's KeyDown delegates here so a Renoise rebind takes effect
// regardless of whether an ImGui GT2 window has focus or the main GT2
// text-mode render view (which forwards keys straight into GT2's main loop)
// has focus.
//
// Adding a new Renoise rebind:
//   1. Search for clashes with the same key in src/Plugins/GoatTracker/gt2/*.c
//      — see [[feedback_gt2_renoise_keybind_clashes]].
//   2. Add a case to HandleKey()'s switch dispatching to a private member.
//   3. The private member returns true to consume the key (which suppresses
//      the underlying GT2 default), or false to fall through.
class CGT2RenoiseInput
{
public:
	CGT2RenoiseInput(C64DebuggerPluginGoatTracker *plugin);

	// Returns true if Renoise mode is active and this key has a binding.
	// When true, callers MUST NOT forward the key to GT2's main loop.
	bool HandleKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	bool HandleKeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	bool TriggerPlayFromCursor();
	// Song order-list operations (Renoise layout). Public for the GT2 tests.
	bool HandleInsertPattern();
	bool HandleDeletePattern();
	bool HandleDuplicatePattern();
	// Move the song order position (Ctrl+Up / Ctrl+Down): the pattern editor
	// follows to the previous / next patterns. Public for the GT2 tests.
	bool HandleSongPositionShortcut(int delta);
	// Step the current pattern number (Ctrl+Left / Ctrl+Right). Public for tests.
	bool HandleOrderListPatternNumberShortcut(int delta);
	// Space (playFromCursor=false) and Shift+Space (true). Public because the
	// toolbar's Play/Pause button routes through it: that button is tooltipped
	// as Space, so it has to BE Space rather than a second implementation that
	// drifts from it.
	bool HandlePlayStop(bool playFromCursor);

private:
	C64DebuggerPluginGoatTracker *plugin;

	bool HandleEditStepShortcut(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	bool HandleUIScaleShortcut(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	bool HandleTab(bool isShift);
	bool HandleEnterTriggerRow();
	bool HandleWriteModeToggle();
	bool HandleOctaveShortcut(int delta);
	bool HandleNoteOffShortcut();
	bool HandleDeleteClearNote();
	bool HandleClearWholeRow();
	bool HandleInstrumentShortcut(int delta);
	bool HandleMuteSoloShortcut(bool solo);
	// Bare-bracket pattern-number step — ports gpattern.c's prevpattern()
	// / nextpattern() bindings (delta = +1 / -1). Renoise also has
	// Shift+Left/Right doing the same thing via HandleMainTrackNavigation;
	// these are the legacy-equivalent extra bindings.
	bool HandlePatternNumberStep(int delta);
	// Shift+1 / Shift+2 / Shift+3 toggle of a specific SID voice mute —
	// ports gpattern.c:1174's per-digit binding by writing the same
	// gt2_voice_mute[] array the mixer UI uses.
	bool HandleChannelMuteShortcut(int channel);
	bool HandleUndoRedoShortcut(bool redo);
	bool HandleNewSong();
	bool HandleOpenSong();
	bool HandleSaveSong();
	bool HandleSaveSongAs();
};

#endif
