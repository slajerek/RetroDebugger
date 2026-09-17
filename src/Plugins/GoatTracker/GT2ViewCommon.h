#ifndef _GT2ViewCommon_H_
#define _GT2ViewCommon_H_

#include "SYS_Defs.h"

// Shared keyboard/mouse forwarding for all GT2 ImGui views.
// Include this in each GT2 view .cpp and call GT2_ForwardKeyDown etc.

class CViewC64GoatTracker;
class CGuiView;

// Set by plugin init, used by all GT2 views to forward events
extern CViewC64GoatTracker *gt2MainView;

// Number of song channels. Today this is the compile-time MAX_CHN (3); it is
// routed through one accessor so a future SID-stereo change (6 channels) only
// has to update this single place.
int GT2_NumChannels();

// Transport stop policy.
//
// Default (false) is stock GT2 behaviour: stopping goes through stopsong(),
// whose PLAY_STOP pass walks every channel clearing wave and ptr[WTBL]
// (gplay.c:383-385), so the sound is cut dead the moment you press Space.
//
// With this on, stopping leaves the voices exactly as they are and only stops
// the sequencer, so whatever was sounding rings out and decays naturally --
// the same way a row triggered with Enter does.
extern bool gt2KeepPlayingOnStop;

// True while the transport is actually running.
//
// Not the same as isplaying(). PLAY_STOP means "a stop has been requested but
// playroutine() has not processed it yet"; isplaying() still reports that as
// playing, so a second press of Space would stop again instead of starting.
// In the running app the player thread clears PLAY_STOP within a frame, which
// is the only reason that was never visible. Both Space and the toolbar's
// Play/Pause button ask this instead.
bool GT2_IsTransportActive();

// Stop transport, honouring gt2KeepPlayingOnStop. Use this for every
// user-facing "stop playing" action. Call stopsong() directly only where a
// hard reset is the whole point -- New Song, and the toolbar's explicit
// Stop-and-reset button.
void GT2_StopSong();

void GT2_ForwardKeyDown(u32 keyCode);
void GT2_ForwardKeyUp(u32 keyCode);
// Same as GT2_ForwardKeyDown but ignores the keypreset gate. Used by
// views (Instrument, Tables, …) whose field editing relies on native
// GT2 handlers (gt2/ginstr.c, gt2/gtable.c) — those handlers are the
// design source of truth for that context, regardless of which keyboard
// preset the user picked.
void GT2_ForwardKeyDownToNative(u32 keyCode);
bool GT2_HandleRenoiseOrForwardKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
// Variant that lets renoiseInput see the key first (so undo/redo, mute,
// etc. still work) but falls back to GT2_ForwardKeyDownToNative — i.e.
// unconditionally to native GT2 — when the renoise handler didn't
// consume it. For views that delegate field editing to native GT2.
bool GT2_HandleRenoiseOrForwardKeyDownToNative(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
bool GT2_IsModifierKey(u32 keyCode);
void GT2_ForwardMouseDown(float viewX, float viewY, float viewW, float viewH, float clickX, float clickY);
void GT2_ForwardMouseUp(float viewX, float viewY, float viewW, float viewH, float clickX, float clickY);

// When a GT2 view uses ImGui::BeginChild internally, the click that gives the
// child window focus only updates ImGui's NavWindow to the child — CGuiView's
// PreRenderImGui checks ImGui::IsWindowFocused() against the OUTER window and
// so misses the click. Without this propagation, the engine never marks the
// outer view as focusedView, so GT2 shortcuts (Space, Ctrl+arrow, undo/redo,
// etc.) never reach renoiseInput while the user is interacting with sliders
// or buttons in the child. Call this right before PostRenderImGui() in any
// GT2 view that opens BeginChild blocks.
void GT2_PropagateChildWindowFocus(CGuiView *view);

// The song's default tempo, in player ticks per pattern row.
//
// GT2's song format has no tempo field. The default tempo lives in the `ad`
// byte of the RESERVED instrument slot MAX_INSTR-1 (63) -- the one
// GT2_LAST_INSTR keeps out of the instrument editor -- on the same scale as the
// F (Set Tempo) command: the player runs at ad-1. It counts only while that
// slot carries no wavetable pointer, which is the flag GT2 uses to tell a tempo
// override from a real instrument. gplay.c reads it at PLAY_BEGINNING and
// greloc.c exports it as DEFAULTTEMPO.
//
// Two views show this number -- the toolbar's Tempo field and the Song
// Settings view -- so the reserved-slot rule lives here once and both call it.
//
// Values below GT2_SONG_TEMPO_MIN are not tempos: the player treats tempo < 2
// as a funktable[] index, which is the E command's shuffle, so Set clamps.
#define GT2_SONG_TEMPO_MIN 3
#define GT2_SONG_TEMPO_MAX 127

int GT2_GetSongTempo();
void GT2_SetSongTempo(int tempo);

#endif
