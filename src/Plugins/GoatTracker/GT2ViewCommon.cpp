#include "GT2ViewCommon.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewC64GoatTracker.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2Instrument.h"
#include "CGT2RenoiseInput.h"
#include "CGuiEvent.h"
#include "CGuiView.h"
#include "CGuiMain.h"
#include "SYS_Defs.h"
#include "SYS_KeyCodes.h"
#include "imgui.h"

extern "C" {
#include "gcommon.h"
#include "gconsole.h"
#include "goattrk2.h"
#include "gplay.h"
#include "gsong.h"
}

CViewC64GoatTracker *gt2MainView = NULL;

bool gt2KeepPlayingOnStop = false;

bool GT2_IsTransportActive()
{
	return songinit != PLAY_STOPPED && songinit != PLAY_STOP;
}

void GT2_StopSong()
{
	if (!gt2KeepPlayingOnStop)
	{
		stopsong();
		return;
	}

	if (songinit == PLAY_STOPPED)
		return;

	// Deliberately not stopsong(). That sets songinit = PLAY_STOP, and
	// playroutine()'s init pass then walks every channel zeroing wave and
	// ptr[WTBL] -- which is precisely what silences the song.
	//
	// Going straight to PLAY_STOPPED skips that pass, so the channel state
	// survives and the wavetable, pulse program and ADSR all carry on to
	// their natural end. Nothing keeps playing the song, because every path
	// that reads it is already gated on this flag: the pattern fetch
	// (gplay.c:1059 and 1110) and the order-list advance (gplay.c:1197).
	// incrementtime() stops the clock for the same reason.
	songinit = PLAY_STOPPED;

	// What the init pass would have set on its way through PLAY_STOP, so
	// everything downstream sees the same state it would after a hard stop.
	lastsonginit = PLAY_STOP;
	followplay = 0;
}

int GT2_NumChannels()
{
	return MAX_CHN;
}

void GT2_ForwardKeyDown(u32 keyCode)
{
	// Renoise overlays own their keys end-to-end. Pattern view forwards
	// here when nothing matched — under KEY_RENOISE this must be a no-op,
	// otherwise the key leaks through to native gpattern.c (e.g. Space →
	// recordmode toggle, the original bug that triggered the routing
	// rewrite). Views that genuinely delegate field editing to native GT2
	// (Instrument, Tables) MUST use GT2_ForwardKeyDownToNative instead;
	// this function is the "I'm an overlay and shouldn't reach native"
	// fallback.
	if (keypreset == KEY_RENOISE) return;
	GT2_ForwardKeyDownToNative(keyCode);
}

void GT2_ForwardKeyDownToNative(u32 keyCode)
{
	// Unconditional native-side delivery. Bypasses the KEY_RENOISE gate.
	// Use only from views whose field editing is implemented by native
	// GT2 (gt2/ginstr.c, gt2/gtable.c, …) — those handlers ARE the design
	// in their context, no matter which keyboard preset is active.
	if (!gt2MainView) return;
	CGuiEventKeyboard *ev = new CGuiEventKeyboard(GUI_EVENT_KEYBOARD_KEY_DOWN, keyCode, keyCode);
	gt2MainView->AddEvent(ev);
}

void GT2_ForwardKeyUp(u32 keyCode)
{
	if (pluginGoatTracker && pluginGoatTracker->renoiseInput
		&& pluginGoatTracker->renoiseInput->HandleKeyUp(keyCode, false, false, false, false))
	{
		return;
	}

	// Same rationale as GT2_ForwardKeyDown — native GT2 must not see the
	// matching key-up either, or its sticky-key state (notesonkbd, etc.)
	// drifts out of sync with what Renoise actually played.
	if (keypreset == KEY_RENOISE) return;
	if (!gt2MainView) return;
	CGuiEventKeyboard *ev = new CGuiEventKeyboard(GUI_EVENT_KEYBOARD_KEY_UP, keyCode, keyCode);
	gt2MainView->AddEvent(ev);
}

bool GT2_HandleRenoiseOrForwardKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Global undo / redo — one shared history, works from every GT2 view
	// regardless of the keyboard preset.
	if ((isControl || isSuper) && !isAlt && pluginGoatTracker && pluginGoatTracker->viewPatterns)
	{
		if (!isShift && (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z))
		{
			pluginGoatTracker->viewPatterns->UndoPatternEdit();
			return true;
		}
		if (keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y)
		{
			pluginGoatTracker->viewPatterns->RedoPatternEdit();
			return true;
		}
	}

	// Sustain column edit (when the cursor is parked there) — must run
	// before renoiseInput / HandleArpKey / native GT2 so the hex digit
	// goes to CMD_SETSR instead of the instrument byte.
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns
		&& pluginGoatTracker->viewPatterns->HandleSustainColumnKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	if (pluginGoatTracker && pluginGoatTracker->renoiseInput
		&& pluginGoatTracker->renoiseInput->HandleKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Enter on an instrument's table-pointer field navigates the ImGui
	// table view instead of dropping into the native GT2 legacy editor.
	if (keyCode == MTKEY_ENTER && pluginGoatTracker && pluginGoatTracker->viewInstrument
		&& pluginGoatTracker->viewInstrument->HandleInstrumentTablePointerEnter(
			isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns
		&& pluginGoatTracker->viewPatterns->HandleArpKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns && !GT2_IsModifierKey(keyCode))
	{
		pluginGoatTracker->viewPatterns->eparpcol = -1;
	}

	GT2_ForwardKeyDown(keyCode);
	return true;
}

bool GT2_HandleRenoiseOrForwardKeyDownToNative(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Renoise sees the key first — Undo/Redo, transport, mute/solo must
	// keep working from instrument/tables views too. If nothing in the
	// Renoise stack consumed it, forward UNCONDITIONALLY to native GT2:
	// instrument/tables field editing is implemented over there and
	// that's intentional. Mirrors the head of GT2_HandleRenoiseOrForwardKeyDown
	// minus the final GT2_ForwardKeyDown gate.
	if ((isControl || isSuper) && !isAlt && pluginGoatTracker && pluginGoatTracker->viewPatterns)
	{
		if (!isShift && (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z))
		{
			pluginGoatTracker->viewPatterns->UndoPatternEdit();
			return true;
		}
		if (keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y)
		{
			pluginGoatTracker->viewPatterns->RedoPatternEdit();
			return true;
		}
	}
	if (pluginGoatTracker && pluginGoatTracker->renoiseInput
		&& pluginGoatTracker->renoiseInput->HandleKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	GT2_ForwardKeyDownToNative(keyCode);
	return true;
}

bool GT2_IsModifierKey(u32 keyCode)
{
	return keyCode == MTKEY_LSHIFT
		|| keyCode == MTKEY_RSHIFT
		|| keyCode == MTKEY_LALT
		|| keyCode == MTKEY_RALT
		|| keyCode == MTKEY_LCONTROL
		|| keyCode == MTKEY_RCONTROL
		|| keyCode == MTKEY_LSUPER
		|| keyCode == MTKEY_RSUPER;
}

void GT2_ForwardMouseDown(float viewX, float viewY, float viewW, float viewH, float clickX, float clickY)
{
	if (!gt2MainView) return;
	// Convert ImGui window coords to GT2 pixel coords (full 800x592 screen)
	float xp = ((clickX - viewX) / viewW) * (float)(MAX_COLUMNS * 8);
	float yp = ((clickY - viewY) / viewH) * (float)(MAX_ROWS * 16);
	CGuiEventMouse *ev = new CGuiEventMouse(GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN, (unsigned int)xp, (unsigned int)yp);
	gt2MainView->AddEvent(ev);
}

void GT2_ForwardMouseUp(float viewX, float viewY, float viewW, float viewH, float clickX, float clickY)
{
	if (!gt2MainView) return;
	float xp = ((clickX - viewX) / viewW) * (float)(MAX_COLUMNS * 8);
	float yp = ((clickY - viewY) / viewH) * (float)(MAX_ROWS * 16);
	CGuiEventMouse *ev = new CGuiEventMouse(GUI_EVENT_MOUSE_LEFT_BUTTON_UP, (unsigned int)xp, (unsigned int)yp);
	gt2MainView->AddEvent(ev);
}

void GT2_PropagateChildWindowFocus(CGuiView *view)
{
	if (view == NULL) return;
	// ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) returns true if
	// the outer window OR any of its currently-open child windows holds
	// focus. CGuiView::PreRenderImGui only checks the outer window, so
	// without this nudge the engine's focusedView never advances to a GT2
	// view whose interactive widgets all live inside BeginChild blocks
	// (mixer channel strips, instrument knob row, etc.). Without that, the
	// next KeyDown dispatched by CGuiMain goes to whatever view WAS focused
	// before, so Space / Ctrl+arrow shortcuts silently route to the wrong
	// renoiseInput target.
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
	{
		guiMain->SetInternalViewFocus(view);
	}
}

int GT2_GetSongTempo()
{
	// GT2 has no tempo field in the song header. The song's default tempo lives
	// in the RESERVED instrument slot MAX_INSTR-1 (63) -- the one
	// GT2_LAST_INSTR keeps out of the instrument editor -- in its `ad` byte, on
	// the same scale as the F (Set Tempo) command: the player runs at ad-1.
	// It counts only while that slot carries no wavetable pointer, which is the
	// flag GT2 uses to tell "tempo override" from "a real instrument".
	// gplay.c reads it at PLAY_BEGINNING and greloc.c exports it as
	// DEFAULTTEMPO; both spell the condition exactly this way.
	if (ginstr[MAX_INSTR-1].ad >= 2 && !ginstr[MAX_INSTR-1].ptr[WTBL])
		return ginstr[MAX_INSTR-1].ad;

	// No override: initchannels() starts every channel at 6, or 6*multiplier
	// when the player was started at a multiple of the frame rate.
	return multiplier ? (int)(6 * multiplier) : 6;
}

void GT2_SetSongTempo(int tempo)
{
	if (tempo < GT2_SONG_TEMPO_MIN) tempo = GT2_SONG_TEMPO_MIN;
	if (tempo > GT2_SONG_TEMPO_MAX) tempo = GT2_SONG_TEMPO_MAX;
	ginstr[MAX_INSTR-1].ad = (unsigned char)tempo;

	// The slot is only read when playback starts, so on its own this would not
	// be heard until the next restart. Push the value into the live channels
	// too -- the same thing the F command's all-channels branch does -- so the
	// field behaves like a tempo control rather than a preference. A pattern
	// carrying its own F command still wins on the next row that has one.
	unsigned char live = (unsigned char)tempo;
	if (live >= 3) live--;
	for (int c = 0; c < MAX_CHN; c++)
		chn[c].tempo = live;
}
