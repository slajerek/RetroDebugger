#ifndef _CViewGT2Patterns_H_
#define _CViewGT2Patterns_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGT2UndoHistory.h"
#include "imgui.h"
#include <vector>

class CGT2FontAtlas;

class CViewGT2Patterns : public CGuiView
{
	friend class CTestGT2Patterns;   // regression tests reach internal helpers
public:
	// The "Edit Effect" block: pick a command, then dial in its argument (AD
	// nibbles, waveform mix bits, filter passband + resonance + routing,
	// cutoff value, ...) with clickable / nudge widgets. Each write goes
	// through one pattern undo step, so every action is reversible.
	//
	// It opens at the top of the pattern context menu when the right-click
	// lands on a command column, and it IS the whole of CViewGT2PatternRow --
	// hence public, so the menu and that view cannot drift apart. Reads the
	// live cursor (epchn / eppos) and refuses a row it cannot edit.
	void RenderPatternCommandEditor();

	CViewGT2Patterns(const char *name, float posX, float posY, float posZ,
					  float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2Patterns();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool DeserializeLayout(CByteBuffer *byteBuffer, int version);

	// Arp-column nav + note entry. Returns true if the key was consumed.
	// Called from this view's KeyDown AND from CViewC64GoatTracker::KeyDown
	// (the main text-mode view) so navigation and note entry work
	// regardless of which window has focus.
	bool HandleArpKey(u32 keyCode, bool isShift, bool isAlt = false, bool isControl = false, bool isSuper = false);
	// Main-track note entry — mirrors gpattern.c's KEY_TRACKER/DMC/JANKO/
	// CUSTOM/RENOISE keymap branch, but writes directly to pattern[][] +
	// triggers playtestnote() preview without bouncing the key through
	// native GT2's gconsole loop. Returns true iff the key resolved to a
	// note (or REST/KEYOFF) and was consumed for the main note column.
	bool HandleMainTrackNoteEntry(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// Hex entry on the instrument and command columns -- mirrors
	// gpattern.c:1209's hexnybble branch. It has to live here because
	// GT2_ForwardKeyDown() is a deliberate no-op under KEY_RENOISE, so under
	// the Renoise layout the native handler never saw the key at all.
	bool HandlePatternHexEntry(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// Main-track cursor navigation — arrow keys / Home / End / Shift+Left|Right.
	// Replaces what gpattern.c used to do via the now-removed native key
	// forward for KEY_RENOISE: epcolumn ± with channel wrap, eppos ± with
	// pattern-length clamp, Home/End to row 0 / last row, and Shift+arrows
	// to step the cursor channel's pattern number.
	bool HandleMainTrackNavigation(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// Pattern-view Enter binding overlay. Two cases that have to run BEFORE
	// the renoise dispatcher's HandleEnterTriggerRow consumes bare Enter:
	//   * Shift+Enter on the note column   → write KEYON (gpattern.c:266).
	//   * Bare Enter on the instr column with an instr byte present →
	//     gotoinstr(byte) — switch to that instrument's editor
	//     (gpattern.c:273).
	// Returns true iff one of these cases fired.
	bool HandlePatternEnterKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// Cursor-leads-playback (Renoise-style fast-forward / rewind by
	// holding Down / Up during playback). Called from HandleMainTrackNavigation
	// after Up/Down/Home/End update eppos; no-ops when playback is stopped.
	// Public so tests can exercise the seek behaviour without driving a key
	// event through the full KeyDown chain.
	void SeekPlayerToCursorIfPlaying();
	// Public so the shared dispatchers (CViewC64GoatTracker, GT2ViewCommon)
	// can run sustain-column edits before the key falls through to
	// renoiseInput / HandleArpKey / native GT2.
	bool HandleSustainColumnKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	bool IsWriteModeFrameVisible() const;
	bool GetWriteModeFrameRect(ImVec2 windowPos, ImVec2 windowSize, ImVec2 *outMin, ImVec2 *outMax) const;
	int GetPatternCellBackgroundColor(int channel, int patternRow, int fieldColumn, int cursorBackgroundColor) const;
	u8 ApplyPatternCellBackground(u8 colorIndex, int backgroundColor) const;
	int GetPatternTrackIndex(int channel, int arpColumn) const;
	int GetPatternTrackFromGridColumn(int gridCol, int visibleChannels) const;
	// Fine-grained selection — note/instr/arp k/cmd treated as separate
	// selectable sub-columns within a channel.
	void BeginMousePatternSelection(int track, int fineField, int row);
	void UpdateMousePatternSelection(int track, int fineField, int row);
	// Shift+click selection extension — text-editor / Excel semantics.
	// If no selection is active, anchor at the current cursor cell, then
	// set the END to the clicked cell. If a selection already exists,
	// pivot around its existing START (so repeated shift-clicks grow /
	// shrink around the original anchor). Selection stays fine-mode but
	// does NOT enter drag state (no follow-on mouse-move extension).
	void ExtendSelectionWithShiftClick(int track, int fineField, int row);
	// Legacy whole-track selection — convenience overloads that select the
	// entire main column (note + instr + cmd) when the track is main, or the
	// arp column when it's an arp track. Used by older tests and any path
	// that wants the pre-sub-column selection behavior.
	void BeginMousePatternSelection(int track, int row);
	void UpdateMousePatternSelection(int track, int row);
	// Maps a channel-local column offset (gridCol - channel*chnBlockWidth)
	// to its fine field index — note/instr/arp k/cmd. Used for both mouse
	// drag start/end and per-cell highlight checks.
	int  FineFieldFromColInChn(int colInChn) const;
	int  FineFieldFromFieldColumn(int fieldColumn) const;
	int  FieldsPerChannel() const;
	void ClearPatternSelection();
	bool HasPatternSelection() const;
	bool IsPatternCellSelected(int channel, int patternRow, int fieldColumn) const;
	bool CopyPatternSelection();
	bool CutPatternSelection();
	bool PastePatternClipboardAt(int track, int row);
	bool TransposePatternSelection(int semitones);
	bool SelectWholePattern();
	bool CopyEffectsAtCursorOrSelection();
	bool PasteEffectsAtCursor();
	bool InvertSelectionOrPattern();
	bool ShrinkSelectionOrPattern();
	bool ExpandSelectionOrPattern();
	bool MakeHiFiVibratoPortaSpeed();
	bool AdjustHighlightStep(int delta);
	bool CycleAutoadvanceMode();
	bool SplitPatternAtCursor();
	bool JoinPatternAtCursor();
	// Selection-or-cursor-cell variants: act on the active pattern selection,
	// or fall back to the single cell under the cursor when there is none.
	bool CopyAtCursor();
	bool CutAtCursor();
	bool PasteAtCursor();
	bool TransposeAtCursor(int semitones);
	bool EraseAtCursor();
	// Block resize on the active selection.
	bool ShrinkPatternSelection();
	bool ExpandPatternSelection();
	// Whole-track ops: act on the cursor's column (main note column or arp
	// column), spanning every row of the pattern, regardless of any selection.
	bool TransposeTrack(int semitones);
	bool CutTrack();
	bool CopyTrack();
	bool PasteTrack();
	bool ShrinkTrack();
	bool ExpandTrack();
	// Whole-phrase ops: act on every track of every channel (each channel's
	// main note column and arp columns) of the current per-channel patterns.
	bool TransposePhrase(int semitones);
	bool CutPhrase();
	bool CopyPhrase();
	bool PastePhrase();
	bool ShrinkPhrase();
	bool ExpandPhrase();
	// Cursor row navigation (Renoise layout).
	void JumpToPatternRow(int row);
	void MoveToRowWithNote(int direction);
	// Structural row edits on every track of the cursor's channel.
	void InsertChannelRow();
	void RemoveChannelRow();
	bool HandlePatternSelectionShortcut(u32 keyCode, bool isShift);
	bool CanUndoPatternEdit() const;
	bool CanRedoPatternEdit() const;
	bool UndoPatternEdit();
	bool RedoPatternEdit();
	void ClearPatternUndoHistory();
	void BeginPatternUndoStep();
	bool CommitPatternUndoStep();
	void CancelPatternUndoStep();
	// Called by the shared history (CGT2UndoHistory): the arp column and the
	// pattern selection live here, not in GT2's globals, and the row-spill
	// stash is only meaningful for the edit that filled it.
	void CaptureUndoViewState(CGT2UndoSnapshot *snapshot) const;
	void RestoreUndoViewState(const CGT2UndoSnapshot &snapshot);
	void OnUndoHistoryRestored();

	CGT2FontAtlas *fontAtlas;
	int eparpcol;  // -1 = cursor in normal columns, 0+ = arp column index
	// true when the cursor sits on the optional sustain column. Only ever
	// true while gt2VisibleSustainColumn is on; eparpcol must be -1 to be
	// in sustain (it's part of the main track, between instr and arp/cmd).
	bool epInSustain;

private:
	struct PatternClipboardCell
	{
		bool valid;
		bool isArp;
		u8 data[4];
		// fineKind: which subset of `data` is meaningful for a paste.
		//   -1 (FINE_NONE) — legacy whole-cell (multi-cell selections,
		//                    or arp cells where data[0] is the arp byte).
		//    0 (FINE_NOTE) — single-cell main note click: data[0]=note,
		//                    data[1]=instr (cmd/data untouched on paste).
		//    1 (FINE_INSTR)— single-cell instrument click: data[1]=instr.
		//    2 (FINE_CMD)  — single-cell command click: data[2]=cmd,
		//                    data[3]=cmd data.
		int fineKind;
	};
	// PFC_FINE_NONE is 0 so std::vector<PatternClipboardCell>::resize value-
	// initialization (zero-init) keeps legacy whole-cell semantics by default.
	enum { PFC_FINE_NONE = 0, PFC_FINE_NOTE = 1, PFC_FINE_INSTR = 2, PFC_FINE_CMD = 3 };
	struct PhraseClipboardColumn
	{
		int channel;
		int arpColumn;   // -1 = main note column
		std::vector<PatternClipboardCell> cells;
	};
	struct EffectClipboardCell
	{
		u8 command;
		u8 commandData;
	};
	// One pattern row across every track of a channel, pushed off the pattern
	// bottom by Insert Row so a later Remove Row can pull it back.
	struct ChannelRowSpillEntry
	{
		int pattNum;
		int arpCols;
		std::vector<PatternClipboardCell> cells;   // index 0 = main, 1.. = arp
	};
	// The undo snapshot is shared with the table editor and the instrument
	// loader -- one timeline, one snapshot type. See CGT2UndoHistory.h.
	typedef CGT2UndoSnapshot PatternUndoSnapshot;

	bool GetPatternTrackInfo(int track, int *channel, int *arpColumn) const;
	void GetPatternSelectionBounds(int *trackMin, int *trackMax, int *rowMin, int *rowMax) const;
	bool CopyPatternCell(int track, int row);
	bool ClearPatternCell(int track, int row);
	bool TransposePatternCell(int track, int row, int semitones);
	void ReadPatternBlockCell(int track, int row, PatternClipboardCell *out) const;
	void WritePatternBlockCell(int track, int row, const PatternClipboardCell *cell);
	int GetCursorPatternTrack() const;
	int ShrinkPatternColumn(int track, int rowMin, int rowMax);
	void ExpandPatternColumn(int track, int rowMin, int rowMax);
	std::vector<int> GetCurrentChannelTracks() const;
	bool ReverseRowsInTracks(const std::vector<int> &tracks, int rowMin, int rowMax);
	void RenderContextMenu();
	static void FormatMainTrackNoteNameForDisplay(u8 note, char out[4]);
	// Maps an in-channel display column to the keyboard cursor sub-column,
	// setting eparpcol + epcolumn. Shared by left-click and right-click.
	void SetCursorColumnFromColInChn(int colInChn);
	// Writes a command nibble (0..15) into the cursor's pattern cell.
	void InsertPatternCommand(unsigned char cmdNibble);
	const char *GetCommandMenuBrief(int cmdNibble) const;
	const char *GetCommandMenuHelp(int cmdNibble) const;
	ImVec4 GetCommandMenuDescriptionColor(bool hovered) const;
	bool CommandMenuDescriptionUsesNativeShortcut() const;
	// --- Command Value Mode (speed-table value editing) ---
	bool IsSpeedCommand(int cmdNibble) const;       // 1,2,3,4,E — reference the speed table
	bool IsValueModeCommand(int cmdNibble) const;   // 1,2,3,4 — edited as a 4-digit value
	int  GetSpeedtableValue(int index1) const;      // 16-bit value of a 1-based index
	int  FindSpeedtableEntry(int value16) const;    // 0-based pos of value16, or -1
	int  CountSpeedtableRefs(int index1) const;     // refs in patterns+wavetable+instruments
	void SweepUnusedSpeedtableEntries();            // optimizetable(STBL), value mode only
	void CommitCommandValueEdit(int chn, int row, int value16);
	void ChangeCommandNibble(int chn, int row, unsigned char newNibble);
	bool CursorRowCommandIsValueMode() const;       // cursor cell holds a value-mode command
	bool CursorRowCommandUsesValueField() const;    // value-mode command OR empty (4-digit field)
	bool HandleCommandValueKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	void BeginCommandValueEdit();
	void CommitPendingCommandValueEdit();
	void CancelPendingCommandValueEdit();
	void UpdateCommandValueEditLifecycle();         // per-frame transient-state safety net
	void AdvanceCommandValueCursor();               // advance the cursor by the edit step
	std::vector<int> GetPhraseTracks(bool dedupeSharedMainPatterns) const;
	bool PatternRowHasContent(int pattNum, int row) const;
	void ClearAllChannelRowSpill();
	void SetCursorFromPatternTrack(int track, int row);
	PatternUndoSnapshot CapturePatternUndoSnapshot() const;
	bool CommitPatternUndoSnapshotIfChanged(const PatternUndoSnapshot &before);

	bool patternSelectionActive;
	bool patternSelectionDragging;
	bool patternSelectionMoved;
	// In-channel column the mouse went down on, remembered until release so a
	// click-without-drag can move the cursor on mouse-up. A drag suppresses
	// the cursor move so building a selection doesn't relocate the cursor.
	int patternClickColInChn;
	// Fine field index (within a channel) of the drag start / end. Each
	// channel's columns enumerate left-to-right as note(0), instr(1),
	// arp0(2)..arpN(1+N), cmd(2+N) — so cells can be selected as
	// individual columns instead of the whole main track at once.
	int patternSelectionStartFineField;
	int patternSelectionEndFineField;
	// false → IsPatternCellSelected uses the legacy whole-track rule
	// (set by the 2-arg Begin/Update overloads and SelectWholePattern).
	// true  → uses the fine-field rectangle (set by mouse drag).
	bool patternSelectionFineMode;
	// Fractional row accumulator for time-based auto-scroll while drag-selecting
	// past the top/bottom of the visible content. Reset whenever the mouse
	// returns inside the view or the drag ends.
	float patternDragScrollAccum;
	int patternSelectionStartTrack;
	int patternSelectionStartRow;
	int patternSelectionEndTrack;
	int patternSelectionEndRow;
	std::vector<PatternClipboardCell> patternClipboard;
	int patternClipboardWidth;
	int patternClipboardHeight;
	// Dedicated clipboard for whole-channel cut/copy/paste (main + arp tracks),
	// kept separate from the selection/cell clipboard above.
	std::vector<PhraseClipboardColumn> trackClipboard;
	std::vector<EffectClipboardCell> effectClipboard;
	// Dedicated clipboard for whole-phrase cut/copy/paste, separate from the
	// track and selection/cell clipboards.
	std::vector<PhraseClipboardColumn> phraseClipboard;
	// Per-channel stash of rows pushed off the pattern bottom by Insert Row,
	// so a later Remove Row can restore them. Index = channel.
	std::vector<std::vector<ChannelRowSpillEntry>> channelRowSpill;
	bool preservingRowSpill;
	PatternUndoSnapshot pendingPatternUndoSnapshot;
	bool pendingPatternUndoSnapshotActive;
	// True when the context menu was opened on a command/effect column, so
	// RenderContextMenu shows the Command / Effect picker as its first row.
	bool contextMenuOnCommand;
	// Command Value Mode transient edit state (design doc #5.8).
	bool commandValueEditing;
	int commandValueEditChn;
	int commandValueEditRow;
	int commandValueEditValue;   // 16-bit working buffer
	int commandValueDigit;       // 0..3 typing / cursor digit

	// "Generate Echo" tool — cascades the cursor note forward with a Set
	// Sustain/Release command (CMD_SETSR) whose sustain nibble drops by a
	// configurable step. Original sustain + release come from the cursor's
	// instrument. Only empty cells are written; stops when sustain hits 0
	// or the pattern ends.
	bool echoPopupRequested;     // menu set this; we OpenPopup next frame
	// One source note that will spawn an echo cascade on its channel.
	struct EchoSource
	{
		int channel;
		int row;
		unsigned char note;
		unsigned char instr;
		int sustain;       // 0..15, from the instrument (15 placeholder for KEYOFF)
		int release;       // 0..15, from the instrument (0 for KEYOFF)
		bool isKeyOff;     // true → cascade copies KEYOFF only, no CMD_SETSR
	};
	void RenderEchoPopup();
	// Collects every source note that the next Generate Echo would cascade
	// from: the cursor cell with no selection, or every real note inside the
	// main note tracks of the active selection. Returns count.
	int  CollectEchoSources(std::vector<EchoSource> *out) const;
	int  GenerateEchoAtCursor();        // returns number of echo cells written
};

#endif
