#include "CGT2RenoiseInput.h"
#include "GT2ViewCommon.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2AudioMixer.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2Tables.h"
#include "CViewC64.h"
#include "SYS_KeyCodes.h"
#include <SDL3/SDL.h>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gfile.h"
#include "ginstr.h"
#include "gplay.h"   // triggerpatternrow
#include "gorder.h"
#include "gsid.h"
#include "gsong.h"
void gt2advanceeditstep(void);
extern int pattlen[MAX_PATT];
extern int epnum[MAX_CHN];
extern int eppos, epview, epcolumn, epchn;
extern int epoctave;
extern int gt2RenoiseEditStep;
extern int editmode, recordmode;
extern int followplay;
extern unsigned keypreset;
extern int eamode;   // editadsr modal
extern int menu;     // help screen modal
extern char loadedsongfilename[MAX_FILENAME];
extern int numarpcolumns;  // c64d-only, declared in CViewGT2Patterns.cpp's TU
extern unsigned char pattused[MAX_PATT];
}
extern bool gt2RenoiseFollowTrack;
extern bool gt2RenoiseBulkPatternNumberChange;

// Mirrors values from gt2/goattrk2.h. We don't include goattrk2.h directly
// because it pulls BME struct typedefs that clash with c64d types — same
// reason CViewGT2Patterns.cpp redeclares them locally.
#define EDIT_PATTERN  0
#define EDIT_ORDERLIST 1
#define EDIT_INSTRUMENT 2
#define EDIT_TABLES   3
#define KEY_RENOISE   4
#define VISIBLEPATTROWS 31
#define GT2_PAGE_ROWS 8    // rows moved by Page Up/Down — matches GT2 PGUPDNREPEAT

// The single channel a non-bulk order-list op acts on (defined further below).
static int GT2_OrderListChannel();

static bool GT2_HasModifier(bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (isShift || isAlt || isControl || isSuper) return true;
	SDL_Keymod mods = SDL_GetModState();
	return (mods & (SDL_KMOD_SHIFT | SDL_KMOD_ALT | SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
}

static bool GT2_IsCapsLockOn()
{
	return (SDL_GetModState() & SDL_KMOD_CAPS) != 0;
}

CGT2RenoiseInput::CGT2RenoiseInput(C64DebuggerPluginGoatTracker *plugin)
: plugin(plugin)
{
}

bool CGT2RenoiseInput::HandleKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keypreset != KEY_RENOISE) return false;
	if (HandleEditStepShortcut(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	if (HandleUIScaleShortcut(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	if ((keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z)
		&& !isShift && !isAlt && (isControl || isSuper))
	{
		HandleUndoRedoShortcut(false);
		return true;
	}
	if ((keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y)
		&& !isShift && !isAlt && (isControl || isSuper))
	{
		HandleUndoRedoShortcut(true);
		return true;
	}
	if ((keyCode == 'n' || keyCode == 'N' || keyCode == SDLK_N)
		&& !isShift && !isAlt && (isControl || isSuper))
	{
		return HandleNewSong();
	}
	if ((keyCode == 'o' || keyCode == 'O' || keyCode == SDLK_O)
		&& !isShift && !isAlt && (isControl || isSuper))
	{
		return HandleOpenSong();
	}
	if ((keyCode == 's' || keyCode == 'S' || keyCode == SDLK_S)
		&& isShift && !isAlt && (isControl || isSuper))
	{
		return HandleSaveSongAs();
	}
	if ((keyCode == 's' || keyCode == 'S' || keyCode == SDLK_S)
		&& !isShift && !isAlt && (isControl || isSuper))
	{
		return HandleSaveSong();
	}
	if ((keyCode == 'k' || keyCode == 'K' || keyCode == SDLK_K)
		&& !isShift && !isAlt && (isControl || isSuper))
	{
		return HandleDuplicatePattern();
	}

	bool altOnly = isAlt && !isShift && !isControl && !isSuper;
	bool shiftOnly = isShift && !isAlt && !isControl && !isSuper;
	bool ctrlOnly = (isControl || isSuper) && !isShift && !isAlt;
	bool noMods = !isShift && !isAlt && !isControl && !isSuper;

	switch (keyCode)
	{
	case MTKEY_TAB:
		// Modifier policy is per-binding; TAB takes plain or Shift only.
		if (isAlt || isControl || isSuper) return false;
		return HandleTab(isShift);
	case MTKEY_ENTER:
		if (isShift || isAlt || isControl || isSuper) return false;
		return HandleEnterTriggerRow();
	case MTKEY_ESC:
		// Esc must NEVER fall through to native GT2 — bare Esc there calls
		// quit() (exits the app) and Shift+Esc calls gt_clear() (wipes the
		// song). Swallow unconditionally, and only toggle write mode on the
		// bare press; modifier variants are intentional no-ops.
		if (!isShift && !isAlt && !isControl && !isSuper)
			HandleWriteModeToggle();
		return true;
	case MTKEY_SPACEBAR:
		if (isAlt || isControl || isSuper) return false;
		return HandlePlayStop(isShift);
	case MTKEY_NUM_MULTIPLY:
		if (isShift || isAlt || isControl || isSuper) return false;
		return HandleOctaveShortcut(1);
	case MTKEY_NUM_DIVIDE:
		if (isShift || isAlt || isControl || isSuper) return false;
		return HandleOctaveShortcut(-1);
	case MTKEY_NUM_MINUS:
		if (isShift || isAlt || isControl || isSuper) return false;
		return HandleInstrumentShortcut(-1);
	case MTKEY_NUM_PLUS:
		if (isShift || isAlt || isControl || isSuper) return false;
		return HandleInstrumentShortcut(1);
	case MTKEY_CAPS_LOCK:
		if (GT2_HasModifier(isShift, isAlt, isControl, isSuper)) return false;
		if (!GT2_IsCapsLockOn()) return true;
		return HandleNoteOffShortcut();
	case 'a':
		if (isShift || isAlt || isControl || isSuper) return false;
		return HandleNoteOffShortcut();
	case MTKEY_DELETE:
		// Ctrl/Cmd+Delete removes the pattern entry from the song order list.
		if ((isControl || isSuper) && !isShift && !isAlt)
			return HandleDeletePattern();
		if (isControl || isSuper) return false;
		if (isShift && !isAlt)
		{
			// Clear: erase the selection, or the cursor cell if none.
			if (plugin && plugin->viewPatterns) plugin->viewPatterns->EraseAtCursor();
			return true;
		}
		if (isAlt && !isShift) return HandleClearWholeRow();
		if (!isAlt && !isShift) return HandleDeleteClearNote();
		return false;
	case MTKEY_INSERT:
		// Ctrl/Cmd+Insert inserts a fresh pattern into the song order list;
		// bare Insert inserts an empty row into the cursor's channel.
		if (ctrlOnly) return HandleInsertPattern();
		if (noMods && plugin && plugin->viewPatterns) { plugin->viewPatterns->InsertChannelRow(); return true; }
		return false;
	case MTKEY_BACKSPACE:
		// Shift+Backspace removes the cursor row from the cursor's channel.
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->RemoveChannelRow(); return true; }
		return false;

	// F-key pattern ops. Alt+F-key = selection ops (fall back to the cursor
	// cell when there is no active selection). Shift+F-key = whole-track ops
	// on the cursor's column. Ctrl/Cmd+F-key = whole-phrase ops (all channels).
	case MTKEY_F1:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeAtCursor(-1); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeTrack(-1);    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposePhrase(-1);   return true; }
		// Bare F1 in native GT2 = initsong(PLAY_BEGINNING) — accidental
		// playback start. Renoise users use Space for transport.
		return true;
	case MTKEY_F2:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeAtCursor(1); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeTrack(1);    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposePhrase(1);   return true; }
		// Bare F2 in native GT2 = initsong(PLAY_POS).
		return true;
	case MTKEY_F11:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeAtCursor(-12); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeTrack(-12);    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposePhrase(-12);   return true; }
		if (noMods    && plugin && plugin->viewPatterns) { plugin->viewPatterns->JumpToPatternRow(32);   return true; }
		return false;
	case MTKEY_F12:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeAtCursor(12); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposeTrack(12);    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->TransposePhrase(12);   return true; }
		if (noMods    && plugin && plugin->viewPatterns) { plugin->viewPatterns->JumpToPatternRow(48);  return true; }
		return false;
	case MTKEY_F3:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->CutAtCursor(); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->CutTrack();    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->CutPhrase();   return true; }
		// Bare F3 in native GT2 = initsong(PLAY_PATTERN).
		return true;
	case MTKEY_F4:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->CopyAtCursor(); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->CopyTrack();    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->CopyPhrase();   return true; }
		// Bare F4 in native GT2 = stopsong(). Swallow.
		return true;
	case MTKEY_F5:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->PasteAtCursor(); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->PasteTrack();    return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->PastePhrase();   return true; }
		// Bare F5: native GT2 sets editmode = EDIT_PATTERN (a no-op in
		// Renoise) — swallow for consistency with F6..F8 below.
		return true;
	case MTKEY_F6:
	case MTKEY_F7:
		// Native GT2: F6 flips editmode to EDIT_ORDERLIST, F7 to
		// EDIT_INSTRUMENT/TABLES. Both are unreachable in Renoise (those
		// live in their own ImGui windows), so a bare press just desyncs
		// the input handlers. Swallow to keep cursor/space/escape sane.
		return true;
	case MTKEY_F8:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->ShrinkPatternSelection(); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->ShrinkTrack();            return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->ShrinkPhrase();           return true; }
		// Bare F8 in native GT2 flips editmode to EDIT_NAMES. Swallow.
		return true;
	case MTKEY_F9:
		if (altOnly   && plugin && plugin->viewPatterns) { plugin->viewPatterns->ExpandPatternSelection(); return true; }
		if (shiftOnly && plugin && plugin->viewPatterns) { plugin->viewPatterns->ExpandTrack();            return true; }
		if (ctrlOnly  && plugin && plugin->viewPatterns) { plugin->viewPatterns->ExpandPhrase();           return true; }
		if (noMods    && plugin && plugin->viewPatterns) { plugin->viewPatterns->JumpToPatternRow(0);      return true; }
		return false;
	case MTKEY_F10:
		if (noMods    && plugin && plugin->viewPatterns) { plugin->viewPatterns->JumpToPatternRow(16);     return true; }
		// Shift+F10 in native GT2 = load() — opens a song-load dialog.
		if (shiftOnly) return true;
		return altOnly;

	case SDLK_BACKSLASH:
		if (isShift || isAlt || (isControl && isSuper))
			return editmode == EDIT_PATTERN && !eamode && !menu;
		return HandleMuteSoloShortcut(isControl || isSuper);
	case SDLK_RIGHTBRACKET:
		// Ctrl+] → octave up (Renoise binding).
		if (ctrlOnly) return HandleOctaveShortcut(1);
		// Bare ] (and the user-decision-twins ) and >) → step this channel's
		// pattern number forward, mirroring gpattern.c:82's nextpattern().
		if (noMods) return HandlePatternNumberStep(1);
		return false;
	case SDLK_LEFTBRACKET:
		if (ctrlOnly) return HandleOctaveShortcut(-1);
		if (noMods) return HandlePatternNumberStep(-1);
		return false;
	// Native bare ( / ) and < / > were prevpattern / nextpattern aliases too.
	// User asked to keep both Renoise (Ctrl+brackets for octave) and the
	// native bare-bracket-step-pattern bindings — so these route to the
	// same shortcut. Modifiers fall through so Shift+9/0 etc. typing in
	// fields still works.
	case '9':
	case '0':
		// Skip — '9' '0' are the bare digit keycodes the same SDL events
		// produce. We only want the shifted forms ( ) here, which arrive
		// as their own ASCII codes below.
		return false;
	case '(':
		if (noMods || (isShift && !isAlt && !isControl && !isSuper))
			return HandlePatternNumberStep(-1);
		return false;
	case ')':
		if (noMods || (isShift && !isAlt && !isControl && !isSuper))
			return HandlePatternNumberStep(1);
		return false;
	case '<':
	case ',':
		if (isShift && !isAlt && !isControl && !isSuper)
			return HandlePatternNumberStep(-1);
		return false;
	case '>':
	case '.':
		if (isShift && !isAlt && !isControl && !isSuper)
			return HandlePatternNumberStep(1);
		return false;
	// Shift+1 / Shift+2 / Shift+3 — channel mute toggle for ch0/ch1/ch2,
	// mirroring gpattern.c:1174's Shift+digit binding. Writes to
	// gt2_voice_mute (same source-of-truth the mixer UI uses) so the
	// mute light in the mixer stays in sync.
	case '1':
	case '!':
		if (isShift && !isAlt && !isControl && !isSuper)
			return HandleChannelMuteShortcut(0);
		return false;
	case '2':
	case '@':
		if (isShift && !isAlt && !isControl && !isSuper)
			return HandleChannelMuteShortcut(1);
		return false;
	case '3':
	case '#':
		if (isShift && !isAlt && !isControl && !isSuper)
			return HandleChannelMuteShortcut(2);
		return false;
	case MTKEY_ARROW_RIGHT:
		if (isShift || isAlt || !(isControl || isSuper)) return false;
		return HandleOrderListPatternNumberShortcut(1);
	case MTKEY_ARROW_LEFT:
		if (isShift || isAlt || !(isControl || isSuper)) return false;
		return HandleOrderListPatternNumberShortcut(-1);
	case MTKEY_ARROW_UP:
		if (isShift && (isControl || isSuper) && !isAlt)
		{
			if (plugin && plugin->viewPatterns) plugin->viewPatterns->MoveToRowWithNote(-1);
			return true;
		}
		// Ctrl/Cmd+Up — step the song order position to the previous patterns.
		if (ctrlOnly) return HandleSongPositionShortcut(-1);
		if (isShift || !isAlt || isControl || isSuper) return false;
		return HandleInstrumentShortcut(-1);
	case MTKEY_ARROW_DOWN:
		if (isShift && (isControl || isSuper) && !isAlt)
		{
			if (plugin && plugin->viewPatterns) plugin->viewPatterns->MoveToRowWithNote(1);
			return true;
		}
		// Ctrl/Cmd+Down — step the song order position to the next patterns.
		if (ctrlOnly) return HandleSongPositionShortcut(1);
		if (isShift || !isAlt || isControl || isSuper) return false;
		return HandleInstrumentShortcut(1);
	// Shift+Ctrl/Cmd+PageUp/PageDown — alternate keys for prev/next row w/ note.
	// Plain PageUp/PageDown page the cursor, clamped (stock GT2 wraps around).
	case MTKEY_PAGE_UP:
		if (isShift && (isControl || isSuper) && !isAlt)
		{
			if (plugin && plugin->viewPatterns) plugin->viewPatterns->MoveToRowWithNote(-1);
			return true;
		}
		if (noMods && plugin && plugin->viewPatterns)
		{
			plugin->viewPatterns->JumpToPatternRow(eppos - GT2_PAGE_ROWS);
			return true;
		}
		return false;
	case MTKEY_PAGE_DOWN:
		if (isShift && (isControl || isSuper) && !isAlt)
		{
			if (plugin && plugin->viewPatterns) plugin->viewPatterns->MoveToRowWithNote(1);
			return true;
		}
		if (noMods && plugin && plugin->viewPatterns)
		{
			plugin->viewPatterns->JumpToPatternRow(eppos + GT2_PAGE_ROWS);
			return true;
		}
		return false;
	}
	return false;
}

bool CGT2RenoiseInput::TriggerPlayFromCursor()
{
	return HandlePlayStop(true);
}

bool CGT2RenoiseInput::HandleEditStepShortcut(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	bool command = isControl || isSuper;
	bool isBackquoteKey = keyCode == SDLK_GRAVE || keyCode == '`' || keyCode == '~';
	if (!command && !isAlt && isBackquoteKey)
	{
		if (isShift)
		{
			if (gt2RenoiseEditStep > 0)
				gt2RenoiseEditStep--;
			else
				gt2RenoiseEditStep = 0;
		}
		else
		{
			gt2RenoiseEditStep++;
		}
		return true;
	}

	if (command && !isShift && !isAlt)
	{
		if (keyCode >= '0' && keyCode <= '9')
		{
			gt2RenoiseEditStep = (int)(keyCode - '0');
			return true;
		}
		// Ctrl+= / Ctrl+- are UI zoom (HandleUIScaleShortcut), not edit step.
	}
	else if (isAlt && !isShift && !command)
	{
		if (keyCode == SDLK_EQUALS)
		{
			gt2RenoiseEditStep *= 2;
			return true;
		}
		if (keyCode == SDLK_MINUS)
		{
			gt2RenoiseEditStep /= 2;
			if (gt2RenoiseEditStep < 0) gt2RenoiseEditStep = 0;
			return true;
		}
	}
	return false;
}

bool CGT2RenoiseInput::HandleUIScaleShortcut(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Renoise-layout UI zoom: Ctrl/Cmd + main-row '=' / '-'. Numpad excluded.
	bool command = isControl || isSuper;
	if (!command || isShift || isAlt)
		return false;

	if (keyCode == SDLK_EQUALS)
	{
		GT2_StepRenoiseUIScale(+1);
		return true;
	}
	if (keyCode == SDLK_MINUS)
	{
		GT2_StepRenoiseUIScale(-1);
		return true;
	}
	return false;
}

bool CGT2RenoiseInput::HandleOrderListPatternNumberShortcut(int delta)
{
	if (eamode || menu) return false;
	if (esnum < 0 || esnum >= MAX_SONGS) return false;
	if (eseditpos < 0 || eseditpos > MAX_SONGLEN + 1) return false;
	if (!plugin || !plugin->viewPatterns) return false;

	// Bulk Pattern Number Change on: shift every channel by the channel count
	// so the whole block moves as a unit. Off: shift only the cursor's channel
	// by one. Works from both the order-list view and the pattern editor.
	int singleChannel = GT2_OrderListChannel();
	int step = gt2RenoiseBulkPatternNumberChange ? MAX_CHN : 1;
	int firstChannel = gt2RenoiseBulkPatternNumberChange ? 0 : singleChannel;
	int lastChannel = gt2RenoiseBulkPatternNumberChange ? (MAX_CHN - 1) : singleChannel;
	int changed = 0;
	int skipped = 0;

	plugin->viewPatterns->BeginPatternUndoStep();
	for (int c = firstChannel; c <= lastChannel; c++)
	{
		if (c < 0 || c >= MAX_CHN)
		{
			skipped++;
			continue;
		}
		if (eseditpos >= songlen[esnum][c])
		{
			skipped++;
			continue;
		}

		int current = songorder[esnum][c][eseditpos];
		if (current >= MAX_PATT)
		{
			skipped++;
			continue;
		}

		int next = current + delta * step;
		if (next < 0 || next >= MAX_PATT)
		{
			skipped++;
			continue;
		}

		songorder[esnum][c][eseditpos] = (unsigned char)next;
		epnum[c] = next;   // pull the pattern editor onto the new pattern
		changed++;
	}
	plugin->viewPatterns->CommitPatternUndoStep();

	if (skipped > 0 && viewC64 != NULL)
	{
		viewC64->ShowMessageError("GT2 pattern number shortcut skipped %d channel(s)", skipped);
	}
	return changed > 0 || skipped > 0;
}

// ---------------------------------------------------------------------------
// Song order-list operations. These edit songorder/songlen at the current
// order position eseditpos and are wrapped in a CViewGT2Patterns undo step,
// whose snapshot now also covers songorder/songlen so they are undoable.

// Fill out[] with up to `count` distinct pattern numbers that are free — not
// referenced by any subtune's order list — skipping any listed in exclude[].
// findusedpatterns() must have been called first to populate pattused[].
// (pattlen == 0 is the wrong test: clearpattern leaves defaultpatternlength
// blank rows, so freshly cleared patterns have a non-zero length.)
static bool GT2_FindEmptyPatterns(int count, int *out, const int *exclude, int excludeCount)
{
	int found = 0;
	for (int p = 0; p < MAX_PATT && found < count; p++)
	{
		if (pattused[p])
			continue;
		bool skip = false;
		for (int e = 0; e < excludeCount; e++)
			if (exclude[e] == p) { skip = true; break; }
		if (skip)
			continue;
		out[found++] = p;
	}
	return found == count;
}

// The single channel a non-bulk order-list op acts on: the order-list cursor
// channel when the order-list view drives editing, else the pattern channel.
static int GT2_OrderListChannel()
{
	return (editmode == EDIT_ORDERLIST) ? eschn : epchn;
}

bool CGT2RenoiseInput::HandleInsertPattern()
{
	if (eamode || menu) return true;
	if (esnum < 0 || esnum >= MAX_SONGS) return true;
	if (eseditpos < 0 || eseditpos > MAX_SONGLEN + 1) return true;
	if (!plugin || !plugin->viewPatterns) return true;

	bool bulk = gt2RenoiseBulkPatternNumberChange;
	int first = bulk ? 0 : GT2_OrderListChannel();
	int last  = bulk ? MAX_CHN - 1 : GT2_OrderListChannel();

	findusedpatterns();

	// Every channel in range gets a row. When the cursor is on a real entry the
	// row is inserted there; otherwise it is appended — which also recovers a
	// fully empty order list ("+" must always be able to add the first row).
	int targets[MAX_CHN];
	int targetCount = 0;
	for (int c = first; c <= last; c++)
	{
		if (c < 0 || c >= MAX_CHN) continue;
		targets[targetCount++] = c;
	}
	if (targetCount == 0)
		return true;

	int empties[MAX_CHN];
	if (!GT2_FindEmptyPatterns(targetCount, empties, NULL, 0))
	{
		if (viewC64) viewC64->ShowMessageError("GT2 Insert Pattern: no empty pattern available");
		return true;
	}

	plugin->viewPatterns->BeginPatternUndoStep();
	int savedEschn = eschn;
	int savedEseditpos = eseditpos;
	for (int i = 0; i < targetCount; i++)
	{
		int c = targets[i];
		eschn = c;
		int insertAt = savedEseditpos + 1;
		if (insertAt < songlen[esnum][c])
			eseditpos = insertAt;                  // insert after the cursor
		else
			eseditpos = songlen[esnum][c] + 1;     // append (also recovers empty)
		// The picked pattern may hold orphaned data — blank it so the inserted
		// row is a fresh defaultpatternlength pattern.
		clearpattern(empties[i]);
		memset(arpdata[empties[i]], 0, sizeof(arpdata[0]));
		insertorder((unsigned char)empties[i]);
		epnum[c] = empties[i];
	}
	eschn = savedEschn;
	// Move cursor onto the row we just inserted (right after the original cursor).
	int newCursor = savedEseditpos + 1;
	int refLen = songlen[esnum][savedEschn];
	if (refLen <= 0)
		newCursor = 0;
	else if (newCursor >= refLen)
		newCursor = refLen - 1;
	eseditpos = newCursor;
	countpatternlengths();
	plugin->viewPatterns->CommitPatternUndoStep();
	return true;
}

bool CGT2RenoiseInput::HandleDeletePattern()
{
	if (eamode || menu) return true;
	if (esnum < 0 || esnum >= MAX_SONGS) return true;
	if (eseditpos < 0 || eseditpos > MAX_SONGLEN + 1) return true;
	if (!plugin || !plugin->viewPatterns) return true;

	bool bulk = gt2RenoiseBulkPatternNumberChange;
	int first = bulk ? 0 : GT2_OrderListChannel();
	int last  = bulk ? MAX_CHN - 1 : GT2_OrderListChannel();

	int targets[MAX_CHN];
	int targetCount = 0;
	for (int c = first; c <= last; c++)
	{
		if (c < 0 || c >= MAX_CHN) continue;
		if (eseditpos >= songlen[esnum][c]) continue;
		if (songlen[esnum][c] <= 1) continue;   // keep at least one row, like Renoise
		targets[targetCount++] = c;
	}
	if (targetCount == 0)
	{
		if (viewC64) viewC64->ShowMessageError("GT2 Delete Pattern: the pattern list must keep at least one row");
		return true;
	}

	plugin->viewPatterns->BeginPatternUndoStep();
	int savedEschn = eschn;
	int savedEseditpos = eseditpos;
	for (int i = 0; i < targetCount; i++)
	{
		eschn = targets[i];
		deleteorder();
		eseditpos = savedEseditpos;   // deleteorder may bump it past a deleted tail entry
	}
	eschn = savedEschn;
	for (int i = 0; i < targetCount; i++)
	{
		int c = targets[i];
		if (eseditpos < songlen[esnum][c] && songorder[esnum][c][eseditpos] < MAX_PATT)
			epnum[c] = songorder[esnum][c][eseditpos];
	}
	plugin->viewPatterns->CommitPatternUndoStep();
	return true;
}

bool CGT2RenoiseInput::HandleDuplicatePattern()
{
	if (eamode || menu) return true;
	if (esnum < 0 || esnum >= MAX_SONGS) return true;
	if (eseditpos < 0 || eseditpos > MAX_SONGLEN + 1) return true;
	if (!plugin || !plugin->viewPatterns) return true;

	bool bulk = gt2RenoiseBulkPatternNumberChange;
	int first = bulk ? 0 : GT2_OrderListChannel();
	int last  = bulk ? MAX_CHN - 1 : GT2_OrderListChannel();

	findusedpatterns();

	int targets[MAX_CHN];
	int srcs[MAX_CHN];
	int targetCount = 0;
	for (int c = first; c <= last; c++)
	{
		if (c < 0 || c >= MAX_CHN) continue;
		if (eseditpos >= songlen[esnum][c]) continue;
		int src = songorder[esnum][c][eseditpos];
		if (src >= MAX_PATT) continue;   // a marker (loop/transpose/repeat), not a pattern
		targets[targetCount] = c;
		srcs[targetCount] = src;
		targetCount++;
	}
	if (targetCount == 0)
	{
		if (viewC64) viewC64->ShowMessageError("GT2 Duplicate Pattern: no pattern at this position");
		return true;
	}

	int dsts[MAX_CHN];
	if (!GT2_FindEmptyPatterns(targetCount, dsts, srcs, targetCount))
	{
		if (viewC64) viewC64->ShowMessageError("GT2 Duplicate Pattern: no empty pattern available");
		return true;
	}

	plugin->viewPatterns->BeginPatternUndoStep();
	int savedEschn = eschn;
	for (int i = 0; i < targetCount; i++)
	{
		int c = targets[i], src = srcs[i], dst = dsts[i];
		memcpy(pattern[dst], pattern[src], sizeof(pattern[0]));
		memcpy(arpdata[dst][c], arpdata[src][c], sizeof(arpdata[0][0]));
		eschn = c;
		insertorder((unsigned char)dst);
		// insertorder put dst at eseditpos and shifted the original down to
		// eseditpos+1; swap so the original stays put and the clone follows.
		unsigned char tmp = songorder[esnum][c][eseditpos];
		songorder[esnum][c][eseditpos]     = songorder[esnum][c][eseditpos + 1];
		songorder[esnum][c][eseditpos + 1] = tmp;
	}
	eschn = savedEschn;
	countpatternlengths();
	plugin->viewPatterns->CommitPatternUndoStep();
	return true;
}

// Move the song order position by delta and pull the pattern editor onto the
// patterns at the new position (epnum[]). Renoise-style prev/next pattern.
bool CGT2RenoiseInput::HandleSongPositionShortcut(int delta)
{
	if (eamode || menu)
		return true;
	if (esnum < 0 || esnum >= MAX_SONGS)
		return true;

	int maxlen = 0;
	for (int c = 0; c < MAX_CHN; c++)
		if (songlen[esnum][c] > maxlen)
			maxlen = songlen[esnum][c];
	if (maxlen < 1)
		return true;

	int newpos = eseditpos + delta;
	if (newpos < 0) newpos = 0;
	if (newpos > maxlen - 1) newpos = maxlen - 1;
	eseditpos = newpos;

	bool playing = isplaying();
	int firstLiveChannel = -1;
	bool liveOrderChannelUpdated = false;
	for (int c = 0; c < MAX_CHN; c++)
	{
		if (eseditpos < songlen[esnum][c])
		{
			int e = songorder[esnum][c][eseditpos];
			if (e < MAX_PATT)   // a plain pattern, not a repeat / transpose marker
			{
				epnum[c] = e;
				if (playing)
				{
					if (firstLiveChannel < 0) firstLiveChannel = c;
					if (c == eschn) liveOrderChannelUpdated = true;
					espos[c] = eseditpos;
					if (esend[c] <= espos[c]) esend[c] = 0;
					chn[c].pattnum = (unsigned char)e;
					chn[c].songptr = (unsigned char)(eseditpos + 1);
					if (chn[c].pattptr >= (unsigned)(pattlen[e] * 4))
					{
						int lastRow = pattlen[e] > 0 ? pattlen[e] - 1 : 0;
						chn[c].pattptr = (unsigned)(lastRow * 4);
					}
				}
			}
		}
	}
	if (playing && !liveOrderChannelUpdated && firstLiveChannel >= 0)
		eschn = firstLiveChannel;
	if (eppos > pattlen[epnum[epchn]])
		eppos = pattlen[epnum[epchn]];
	return true;
}

bool CGT2RenoiseInput::HandleKeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keypreset != KEY_RENOISE) return false;
	if (keyCode != MTKEY_CAPS_LOCK) return false;
	if (GT2_HasModifier(isShift, isAlt, isControl, isSuper)) return false;
	if (GT2_IsCapsLockOn()) return false;
	return HandleNoteOffShortcut();
}

bool CGT2RenoiseInput::HandleTab(bool isShift)
{
	// Pattern-editing context only. When the user is clicked into Instrument,
	// Tables, SongInfo, … the global `editmode` reflects that and Tab here
	// would stomp pattern track focus from a window where Tab means something
	// else (or where native handlers want it).
	if (editmode != EDIT_PATTERN) return false;
	if (!plugin || !plugin->viewPatterns) return true;

	// Renoise-style track navigation. Each SID channel contains:
	//   - 1 main track (the note column with attached instr/cmd digit cols)
	//   - numarpcolumns arp tracks (additional note columns for chord/arp)
	// TAB walks the global track sequence, skipping digit columns entirely:
	//   ch0 main → ch0 arp0 → ch0 arp1 → ... → ch1 main → ch1 arp0 → ...
	// Wraps at MAX_CHN. Shift+TAB walks the same sequence in reverse.
	int &eparpcol = plugin->viewPatterns->eparpcol;

	if (!isShift)
	{
		if (eparpcol == -1)
		{
			// Currently in main track (regardless of which digit column).
			// Step into first arp of same channel, or — if channel has no
			// arps — wrap to next channel's main track.
			if (numarpcolumns > 0)
			{
				eparpcol = 0;
			}
			else
			{
				epchn++;
				if (epchn >= MAX_CHN) epchn = 0;
				epcolumn = 0;
			}
		}
		else if (eparpcol + 1 < numarpcolumns)
		{
			eparpcol++;
		}
		else
		{
			// Last arp of this channel → next channel's main track.
			eparpcol = -1;
			epchn++;
			if (epchn >= MAX_CHN) epchn = 0;
			epcolumn = 0;
		}
	}
	else
	{
		if (eparpcol > 0)
		{
			eparpcol--;
		}
		else if (eparpcol == 0)
		{
			// First arp → main track of same channel.
			eparpcol = -1;
			epcolumn = 0;
		}
		else
		{
			// Main track → previous channel's last track (arp if any, else main).
			epchn--;
			if (epchn < 0) epchn = MAX_CHN - 1;
			epcolumn = 0;
			eparpcol = (numarpcolumns > 0) ? (numarpcolumns - 1) : -1;
		}
	}

	if (eppos > pattlen[epnum[epchn]]) eppos = pattlen[epnum[epchn]];
	return true;
}

bool CGT2RenoiseInput::HandleEnterTriggerRow()
{
	// Renoise "Trigger Row": play the row currently under the cursor on all
	// channels.
	//
	// Pattern-editing context only. Enter in Instrument view follows table
	// pointers, Enter in Tables view confirms a value, Enter in SongInfo
	// commits a name — those go via the view's own KeyDown or native GT2
	// handlers, not via this renoise pattern-row trigger.
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;

	// Queue just the current row. Do not enter PLAY_PATTERN, because that is
	// transport playback from this row rather than Renoise's one-row trigger.
	triggerpatternrow(eppos);
	gt2advanceeditstep();
	epview = eppos - VISIBLEPATTROWS / 2;
	return true;
}

bool CGT2RenoiseInput::HandleWriteModeToggle()
{
	// Pattern-editing context only. Esc in Instrument / Tables / SongInfo
	// should not toggle the pattern record-mode flag; in those views Esc
	// has other meanings (close popup, cancel name edit, …).
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	recordmode ^= 1;
	return true;
}

bool CGT2RenoiseInput::HandlePlayStop(bool playFromCursor)
{
	// Renoise: Space always plays/stops, Shift+Space always plays-from-cursor.
	// Native GT2's editmode is no longer consulted — it was a global that
	// drifted out of sync with whichever Renoise window the user was actually
	// looking at (the original Space-toggles-recordmode bug).
	if (eamode || menu) return false;

	if (!playFromCursor && GT2_IsTransportActive())
	{
		// Honours gt2KeepPlayingOnStop: off (default) cuts the sound as stock
		// GT2 does, on lets whatever is sounding ring out.
		GT2_StopSong();
		return true;
	}

	if (playFromCursor)
	{
		if (lastsonginit != PLAY_PATTERN)
		{
			if (eseditpos != espos[eschn])
			{
				for (int c = 0; c < MAX_CHN; c++)
				{
					if (eseditpos < songlen[esnum][c]) espos[c] = eseditpos;
					if (esend[c] <= espos[c]) esend[c] = 0;
				}
			}
			initsongpos(esnum, PLAY_POS, eppos);
		}
		else
		{
			initsongpos(esnum, PLAY_PATTERN, eppos);
		}
	}
	else
	{
		for (int c = 0; c < MAX_CHN; c++)
		{
			if (eseditpos < songlen[esnum][c]) espos[c] = eseditpos;
			if (esend[c] <= espos[c]) esend[c] = 0;
		}
		initsongpos(esnum, PLAY_POS, 0);
	}
	followplay = gt2RenoiseFollowTrack ? 1 : 0;
	return true;
}

bool CGT2RenoiseInput::HandleOctaveShortcut(int delta)
{
	if (eamode || menu) return false;

	if (delta > 0)
	{
		if (epoctave < 7) epoctave++;
	}
	else if (delta < 0)
	{
		if (epoctave > 0) epoctave--;
	}
	return true;
}

bool CGT2RenoiseInput::HandlePatternNumberStep(int delta)
{
	// Mirrors gpattern.c::prevpattern() / nextpattern() (gpattern.c:1297).
	// Only meaningful while editing patterns — outside EDIT_PATTERN the
	// epnum[]/epchn cursor model isn't what the user is interacting with.
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (delta > 0)
	{
		if (epnum[epchn] < MAX_PATT - 1) epnum[epchn]++;
	}
	else if (delta < 0)
	{
		if (epnum[epchn] > 0) epnum[epchn]--;
	}
	if (eppos > pattlen[epnum[epchn]]) eppos = pattlen[epnum[epchn]];
	return true;
}

bool CGT2RenoiseInput::HandleChannelMuteShortcut(int channel)
{
	// Mirrors gpattern.c:1174's Shift+1/2/3 binding. Toggles
	// gt2_voice_mute[c] — the same array the mixer UI binds to, so the
	// mute light on the mixer strip stays in sync.
	if (eamode || menu) return false;
	if (channel < 0 || channel >= 3) return false;
	gt2_voice_mute[channel] = gt2_voice_mute[channel] ? 0 : 1;
	return true;
}

bool CGT2RenoiseInput::HandleNoteOffShortcut()
{
	// Pattern-editing context only. CapsLock / `a` outside the pattern view
	// means whatever native GT2 / the focused view says it means — e.g. `a`
	// as a hex digit in Instrument/Tables fields. Without this gate, typing
	// `a` in those views would stamp KEYOFF into the pattern at the cursor.
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (!plugin || !plugin->viewPatterns) return false;

	int pattNum = epnum[epchn];
	int eparpcol = plugin->viewPatterns->eparpcol;
	if (eparpcol >= 0)
	{
		if (eparpcol >= numarpcolumns) return false;

		if (recordmode && eppos < pattlen[pattNum])
		{
			plugin->viewPatterns->BeginPatternUndoStep();
			arpdata[pattNum][epchn][eppos][eparpcol] = KEYOFF;
			chn[epchn].arpcolnotes[eparpcol] = 0;
			plugin->viewPatterns->CommitPatternUndoStep();
		}
		if (recordmode)
		{
			gt2advanceeditstep();
			epview = eppos - VISIBLEPATTROWS / 2;
		}
		return true;
	}

	if (epcolumn != 0) return false;

	if (recordmode && eppos < pattlen[pattNum])
	{
		plugin->viewPatterns->BeginPatternUndoStep();
		pattern[pattNum][eppos * 4] = KEYOFF;
		pattern[pattNum][eppos * 4 + 1] = 0;
		plugin->viewPatterns->CommitPatternUndoStep();
	}
	if (recordmode)
	{
		gt2advanceeditstep();
		epview = eppos - VISIBLEPATTROWS / 2;
	}
	playtestnote(KEYOFF, 0, epchn);
	return true;
}

bool CGT2RenoiseInput::HandleDeleteClearNote()
{
	// Pattern-editing context only. Delete in Instrument/Tables clears the
	// field value via native handlers; we must not intercept it here.
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (!plugin || !plugin->viewPatterns) return false;

	int pattNum = epnum[epchn];
	int eparpcol = plugin->viewPatterns->eparpcol;
	if (eparpcol >= 0)
	{
		if (eparpcol >= numarpcolumns) return false;

		if (recordmode && eppos < pattlen[pattNum])
		{
			plugin->viewPatterns->BeginPatternUndoStep();
			arpdata[pattNum][epchn][eppos][eparpcol] = 0;
			chn[epchn].arpcolnotes[eparpcol] = 0;
			plugin->viewPatterns->CommitPatternUndoStep();
		}
		if (recordmode)
		{
			gt2advanceeditstep();
			epview = eppos - VISIBLEPATTROWS / 2;
		}
		return true;
	}

	// Delete erases the single field group under the cursor: epcolumn==0 →
	// note (which also wipes instr/cmd to match GT2's "fresh note" semantics);
	// epcolumn 1/2 → instrument byte only; epcolumn 3/4/5 → command + data
	// bytes. We always consume the key so it doesn't fall back into the
	// selection-wide erase that the user explicitly does NOT want from Delete.
	if (recordmode && eppos < pattlen[pattNum])
	{
		plugin->viewPatterns->BeginPatternUndoStep();
		if (epcolumn == 0)
		{
			pattern[pattNum][eppos * 4]     = REST;
			pattern[pattNum][eppos * 4 + 1] = 0;
			pattern[pattNum][eppos * 4 + 2] = 0;
			pattern[pattNum][eppos * 4 + 3] = 0;
		}
		else if (epcolumn == 1 || epcolumn == 2)
		{
			pattern[pattNum][eppos * 4 + 1] = 0;
		}
		else // 3, 4, 5 — command nibble or data nibbles
		{
			pattern[pattNum][eppos * 4 + 2] = 0;
			pattern[pattNum][eppos * 4 + 3] = 0;
		}
		plugin->viewPatterns->CommitPatternUndoStep();
	}
	if (recordmode)
	{
		gt2advanceeditstep();
		epview = eppos - VISIBLEPATTROWS / 2;
	}
	return true;
}

bool CGT2RenoiseInput::HandleClearWholeRow()
{
	// Clear Whole Row (Alt+Delete): zero the current row across all channels
	// and their arp columns. An empty note (REST) is left in every channel's
	// note column. Like HandleDeleteClearNote, this only writes in recordmode.
	// Pattern-editing context only — Alt+Delete in Instrument/Tables means
	// whatever the focused view (or native) bound it to.
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (!plugin || !plugin->viewPatterns) return false;

	if (recordmode)
	{
		plugin->viewPatterns->BeginPatternUndoStep();
		for (int c = 0; c < MAX_CHN; c++)
		{
			int pn = epnum[c];
			if (eppos >= pattlen[pn]) continue;
			pattern[pn][eppos * 4]     = REST;
			pattern[pn][eppos * 4 + 1] = 0;
			pattern[pn][eppos * 4 + 2] = 0;
			pattern[pn][eppos * 4 + 3] = 0;
			for (int a = 0; a < numarpcolumns; a++)
				arpdata[pn][c][eppos][a] = 0;
		}
		plugin->viewPatterns->CommitPatternUndoStep();

		gt2advanceeditstep();
		epview = eppos - VISIBLEPATTROWS / 2;
	}
	return true;
}

bool CGT2RenoiseInput::HandleUndoRedoShortcut(bool redo)
{
	if (!plugin) return false;
	// One timeline for the whole editor, so the shortcut does NOT pick a
	// history by editmode any more. It used to, and that is what made an
	// instrument load unreachable from the pattern editor: the load was
	// recorded while the tables were current, Ctrl+Z in the pattern editor
	// looked at the other history, and there was nothing there. See
	// CGT2UndoHistory.h.
	if (!plugin->viewPatterns) return false;
	if (redo)
		plugin->viewPatterns->RedoPatternEdit();
	else
		plugin->viewPatterns->UndoPatternEdit();
	return true;
}

bool CGT2RenoiseInput::HandleNewSong()
{
	stopsong();
	clearsong(1, 1, 1, 1, 1);
	if (plugin && plugin->viewPatterns)
		plugin->viewPatterns->ClearPatternUndoHistory();
	if (plugin && plugin->viewTables)
		plugin->viewTables->ClearTableUndoHistory();
	return true;
}

bool CGT2RenoiseInput::HandleOpenSong()
{
	if (!plugin) return false;
	plugin->OpenLoadSongDialog();
	return true;
}

bool CGT2RenoiseInput::HandleSaveSong()
{
	if (!plugin) return false;

	// Quick save: if a song file is already loaded, save directly to the
	// same path without showing a dialog (Renoise-style Ctrl+S behavior).
	if (strlen(loadedsongfilename) > 0)
	{
		plugin->SaveSongToFile(loadedsongfilename);
	}
	else
	{
		plugin->OpenSaveSongDialog();
	}
	return true;
}

bool CGT2RenoiseInput::HandleSaveSongAs()
{
	if (!plugin) return false;
	plugin->OpenSaveSongDialog();
	return true;
}

bool CGT2RenoiseInput::HandleInstrumentShortcut(int delta)
{
	if (eamode || menu) return false;

	if (delta < 0)
	{
		previnstr();
	}
	else if (delta > 0)
	{
		nextinstr();
	}
	return true;
}

bool CGT2RenoiseInput::HandleMuteSoloShortcut(bool solo)
{
	if (eamode || menu) return false;
	if (!plugin || !plugin->audioMixer) return false;
	if (epchn < 0 || epchn >= MAX_CHN || epchn >= plugin->audioMixer->numActiveChannels) return false;

	GT2MixerChannel &channel = plugin->audioMixer->channels[epchn];
	if (solo)
	{
		channel.solo = !channel.solo;
	}
	else
	{
		channel.mute = !channel.mute;
	}
	plugin->audioMixer->ApplyVoiceMutes(gt2_voice_mute, MAX_CHN);
	return true;
}
