#include "CViewGT2Patterns.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2AudioMixer.h"
#include "CGT2RenoiseInput.h"
#include "CViewGT2Tables.h"
#include "CViewGT2Instrument.h"
#include "CViewC64.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "CGT2FontAtlas.h"
#include "CLayoutParameter.h"
#include "SYS_KeyCodes.h"
#include "imgui.h"
#include "imgui_internal.h"   // custom menu item with hover-colored description
#include <algorithm>
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsong.h"
#include "gorder.h"
#include "gtable.h"
#include "ginstr.h"
extern int gt2_engine_ready;
extern int gt2RenoiseEditStep;

extern int pattlen[MAX_PATT];
extern int epnum[MAX_CHN];
extern int eppos, epview, epcolumn, epchn;
extern int esnum, eschn, eseditpos;
extern int epmarkchn, epmarkstart, epmarkend;
extern int editmode, stepsize, cursorflash, eamode;
extern int menu;
extern int cursorcolortable[];
extern unsigned patterndispmode;
extern char *notename[];
extern int epoctave;
extern int recordmode;
extern int shiftpressed;
extern int stepsize;
extern unsigned keypreset;
extern unsigned char notekeytbl1[];
extern unsigned char notekeytbl2[];
extern unsigned char dmckeytbl[];
extern unsigned char jankokeytbl1[];
extern unsigned char jankokeytbl2[];
extern unsigned char customkeytbl1[];
extern unsigned char customkeytbl2[];
extern unsigned char renoisekeytbl1[];
extern unsigned char renoisekeytbl2[];
extern int virtualkeycode;
extern int einum;
extern int autoadvance;
void gt2advanceeditstep(void);
void splitpattern(void);
void joinpattern(void);
}

// Keyboard layout presets (from goattrk2.h)
#define KEY_TRACKER 0
#define KEY_DMC     1
#define KEY_JANKO   2
#define KEY_CUSTOM  3
#define KEY_RENOISE 4

// GT2 color constants (from gdisplay.h)
#define CNORMAL  8
#define CMUTE    3
#define CEDIT    10
#define CPLAYING 12
#define CCOMMAND 7
#define CTITLE   15

// Edit mode constant (from goattrk2.h)
#define EDIT_PATTERN 0


#ifndef WAVECMD
#define WAVECMD 0xf0
#endif
#ifndef WAVELASTCMD
#define WAVELASTCMD 0xfe
#endif

// Command Value Mode: when on, the speed-table commands (1,2,3,4,E) show and
// accept the 16-bit speed-table value directly instead of the table index.
// Toggled from the GT2 menu; persisted as GT2CommandValueMode.
int gt2CommandValueMode = 0;

// Visible Sustain Column: when on, draws a 1-char shadow of the CMD_SETSR
// sustain nibble after the instrument on main tracks ('.' when the row has
// no set-sustain command). Editable: typing a hex digit there writes
// CMD_SETSR with that high nibble and the release nibble of the cursor
// row's instrument. Toggled from the GT2 menu; persisted as
// GT2VisibleSustainColumn.
int gt2VisibleSustainColumn = 0;

// Cursor-row position in the pattern + pattern-list views.
//  0 = upper-third (Renoise-style — fewer rows above the bar than below)
//  1 = centred    (50% — equal split)
// Default is upper-third. Persisted as GT2PatternCursorCentered.
int gt2PatternCursorCentered = 0;

// Generate Echo persistent settings (saved with the rest of the GT2 config
// so they survive an app restart).
int gt2EchoSustainStep = 2;      // sustain decrement per echo entry (1..15)
int gt2EchoRowStep     = 2;      // row spacing between echoes (>=1)
// Channel target mask: bit 0 = ch1, bit 1 = ch2, bit 2 = ch3. 0 means
// "use the source's own channel only" (default — backward compat). When
// multiple bits are set, echoes cycle across those channels in ch1→ch2→ch3
// order; if the source channel is in the mask the cycle starts there,
// otherwise at the next selected channel after the source.
int gt2EchoChannelMask = 0;

CViewGT2Patterns::CViewGT2Patterns(const char *name, float posX, float posY, float posZ,
									float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
	this->eparpcol = -1;
	this->epInSustain = false;
	this->patternSelectionActive = false;
	this->patternSelectionDragging = false;
	this->patternSelectionMoved = false;
	this->patternSelectionStartTrack = 0;
	this->patternSelectionStartRow = 0;
	this->patternSelectionEndTrack = 0;
	this->patternSelectionEndRow = 0;
	this->patternSelectionStartFineField = 0;
	this->patternSelectionEndFineField = 0;
	this->patternSelectionFineMode = false;
	this->patternClickColInChn = 0;
	this->patternDragScrollAccum = 0.0f;
	this->patternClipboardWidth = 0;
	this->patternClipboardHeight = 0;
	this->pendingPatternUndoSnapshotActive = false;
	this->contextMenuOnCommand = false;
	this->commandValueEditing = false;
	this->commandValueEditChn = 0;
	this->commandValueEditRow = 0;
	this->commandValueEditValue = 0;
	this->commandValueDigit = 0;
	this->echoPopupRequested = false;
	this->channelRowSpill.resize(MAX_CHN);
	this->preservingRowSpill = false;
	// Persist the Renoise UI zoom per workspace. Hidden keeps the context menu
	// from writing the raw float while CGuiView still serializes the parameter.
	AddLayoutParameter(new CLayoutParameterFloat("GT2RenoiseUIScale", true, &gt2RenoiseUIScale));
}

CViewGT2Patterns::~CViewGT2Patterns()
{
}

bool CViewGT2Patterns::DeserializeLayout(CByteBuffer *byteBuffer, int version)
{
	// The global layout callback resets to 100% before any view deserializes.
	// Here the layout parameter has loaded a raw float, so clamp/snap it.
	bool ok = CGuiView::DeserializeLayout(byteBuffer, version);
	GT2_SetRenoiseUIScale(gt2RenoiseUIScale);
	return ok;
}

bool CViewGT2Patterns::IsWriteModeFrameVisible() const
{
	return keypreset == KEY_RENOISE && recordmode != 0;
}

bool CViewGT2Patterns::GetWriteModeFrameRect(ImVec2 windowPos, ImVec2 windowSize, ImVec2 *outMin, ImVec2 *outMax) const
{
	if (!IsWriteModeFrameVisible() || outMin == NULL || outMax == NULL)
		return false;

	*outMin = windowPos;
	*outMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
	return true;
}

int CViewGT2Patterns::GetPatternTrackIndex(int channel, int arpColumn) const
{
	if (channel < 0) channel = 0;
	if (channel >= MAX_CHN) channel = MAX_CHN - 1;
	int tracksPerChannel = 1 + numarpcolumns;
	int trackInChannel = arpColumn < 0 ? 0 : arpColumn + 1;
	if (trackInChannel < 0) trackInChannel = 0;
	if (trackInChannel >= tracksPerChannel) trackInChannel = tracksPerChannel - 1;
	return channel * tracksPerChannel + trackInChannel;
}

bool CViewGT2Patterns::GetPatternTrackInfo(int track, int *channel, int *arpColumn) const
{
	int tracksPerChannel = 1 + numarpcolumns;
	int totalTracks = MAX_CHN * tracksPerChannel;
	if (track < 0 || track >= totalTracks)
		return false;

	int ch = track / tracksPerChannel;
	int trackInChannel = track % tracksPerChannel;
	if (channel) *channel = ch;
	if (arpColumn) *arpColumn = trackInChannel == 0 ? -1 : trackInChannel - 1;
	return true;
}

int CViewGT2Patterns::GetPatternTrackFromGridColumn(int gridCol, int visibleChannels) const
{
	if (visibleChannels < 1) visibleChannels = 1;
	if (visibleChannels > MAX_CHN) visibleChannels = MAX_CHN;
	int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
	// chnBlockWidth carries a +1 col of slack to absorb the half-cellW
	// pixel offsets the renderer puts before instrument / sustain (see
	// Note→Instr / Instr→Sustain gaps). Logical columns themselves stay
	// in the tightened positions (note 4-6, instr 7-8, sustain 9, arp 9+).
	int chnBlockWidth = 14 + numarpcolumns * 4 + (gt2CommandValueMode ? 4 : 2) + sustainPad;
	int channel = gridCol / chnBlockWidth;
	if (channel < 0) channel = 0;
	if (channel >= visibleChannels) channel = visibleChannels - 1;
	int colInChn = gridCol - channel * chnBlockWidth;
	int arpStart = 9 + sustainPad;
	if (numarpcolumns > 0 && colInChn >= arpStart && colInChn < arpStart + numarpcolumns * 4)
	{
		int arpColumn = (colInChn - arpStart) / 4;
		if (arpColumn >= numarpcolumns) arpColumn = numarpcolumns - 1;
		return GetPatternTrackIndex(channel, arpColumn);
	}
	return GetPatternTrackIndex(channel, -1);
}

int CViewGT2Patterns::FieldsPerChannel() const
{
	return 3 + numarpcolumns;
}

int CViewGT2Patterns::FineFieldFromColInChn(int colInChn) const
{
	// Tightened layout (cols within a channel):
	//   row#(0-3), note(4-6), instr(7-8), [sustain(9) when enabled,]
	//   arp0..N, command(cmdBase..). The sustain column is not a
	//   fine-selection field, so a click on it lands on the main track
	//   without a specific fine sub-column highlight.
	int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
	if (colInChn < 7) return 0;                                   // note
	if (colInChn < 9) return 1;                                   // instr
	if (sustainPad && colInChn < 9 + sustainPad) return -1;       // sustain — excluded
	int arpStart = 9 + sustainPad;
	int cmdBase = arpStart + numarpcolumns * 4;
	if (numarpcolumns > 0 && colInChn < cmdBase)
	{
		int arp = (colInChn - arpStart) / 4;
		if (arp < 0) arp = 0;
		if (arp >= numarpcolumns) arp = numarpcolumns - 1;
		return 2 + arp;
	}
	return 2 + numarpcolumns;                                     // cmd
}

int CViewGT2Patterns::FineFieldFromFieldColumn(int fieldColumn) const
{
	int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
	if (fieldColumn < 4) return -1;                               // row number
	if (fieldColumn < 7) return 0;                                // note
	if (fieldColumn < 9) return 1;                                // instr
	if (sustainPad && fieldColumn < 9 + sustainPad) return -1;    // sustain
	int arpStart = 9 + sustainPad;
	int cmdBase = arpStart + numarpcolumns * 4;
	if (numarpcolumns > 0 && fieldColumn < cmdBase)
	{
		int arp = (fieldColumn - arpStart) / 4;
		if (arp < 0 || arp >= numarpcolumns) return -1;
		return 2 + arp;
	}
	return 2 + numarpcolumns;
}

void CViewGT2Patterns::BeginMousePatternSelection(int track, int fineField, int row)
{
	patternSelectionActive = true;
	patternSelectionDragging = true;
	patternSelectionMoved = false;
	patternSelectionStartTrack = track;
	patternSelectionStartRow = row;
	patternSelectionEndTrack = track;
	patternSelectionEndRow = row;
	patternSelectionStartFineField = fineField;
	patternSelectionEndFineField = fineField;
	patternSelectionFineMode = true;
}

void CViewGT2Patterns::UpdateMousePatternSelection(int track, int fineField, int row)
{
	if (!patternSelectionActive)
		BeginMousePatternSelection(track, fineField, row);
	patternSelectionMoved = patternSelectionMoved
		|| track != patternSelectionStartTrack
		|| row != patternSelectionStartRow
		|| fineField != patternSelectionStartFineField;
	patternSelectionEndTrack = track;
	patternSelectionEndRow = row;
	patternSelectionEndFineField = fineField;
}

void CViewGT2Patterns::ExtendSelectionWithShiftClick(int track, int fineField, int row)
{
	if (!patternSelectionActive)
	{
		// First shift-click without an existing selection — anchor at
		// the current cursor cell so the new selection runs from there
		// to the click target.
		int anchorTrack = GetPatternTrackIndex(epchn, eparpcol >= 0 ? eparpcol : -1);
		int anchorFine;
		if (eparpcol >= 0)            anchorFine = 2 + eparpcol;
		else if (epInSustain)         anchorFine = 1;
		else if (epcolumn == 0)       anchorFine = 0;
		else if (epcolumn == 1
		     || epcolumn == 2)        anchorFine = 1;
		else                          anchorFine = 2 + numarpcolumns;
		patternSelectionActive = true;
		patternSelectionFineMode = true;
		patternSelectionStartTrack = anchorTrack;
		patternSelectionStartRow = eppos;
		patternSelectionStartFineField = anchorFine;
	}
	patternSelectionEndTrack = track;
	patternSelectionEndRow = row;
	patternSelectionEndFineField = fineField;
	patternSelectionMoved = patternSelectionMoved
		|| track != patternSelectionStartTrack
		|| row != patternSelectionStartRow
		|| fineField != patternSelectionStartFineField;
	patternSelectionDragging = false;
}

void CViewGT2Patterns::BeginMousePatternSelection(int track, int row)
{
	patternSelectionActive = true;
	patternSelectionDragging = true;
	patternSelectionMoved = false;
	patternSelectionStartTrack = track;
	patternSelectionStartRow = row;
	patternSelectionEndTrack = track;
	patternSelectionEndRow = row;
	patternSelectionStartFineField = 0;
	patternSelectionEndFineField = 0;
	patternSelectionFineMode = false;
}

void CViewGT2Patterns::UpdateMousePatternSelection(int track, int row)
{
	if (!patternSelectionActive)
		BeginMousePatternSelection(track, row);
	patternSelectionMoved = patternSelectionMoved
		|| track != patternSelectionStartTrack
		|| row != patternSelectionStartRow;
	patternSelectionEndTrack = track;
	patternSelectionEndRow = row;
}

void CViewGT2Patterns::ClearPatternSelection()
{
	patternSelectionActive = false;
	patternSelectionDragging = false;
	patternSelectionMoved = false;
}

bool CViewGT2Patterns::HasPatternSelection() const
{
	return patternSelectionActive;
}

// Capture / restore / compare are the shared history's -- pattern edits, table
// edits and instrument loads all record the same snapshot type on the same
// stack, so Ctrl+Z takes back the last change whatever kind it was.
CViewGT2Patterns::PatternUndoSnapshot CViewGT2Patterns::CapturePatternUndoSnapshot() const
{
	return GT2UndoHistory()->Capture();
}

void CViewGT2Patterns::CaptureUndoViewState(CGT2UndoSnapshot *snapshot) const
{
	if (snapshot == NULL) return;
	snapshot->cursorArpColumn = eparpcol;
	snapshot->selectionActive = patternSelectionActive;
	snapshot->selectionStartTrack = patternSelectionStartTrack;
	snapshot->selectionStartRow = patternSelectionStartRow;
	snapshot->selectionEndTrack = patternSelectionEndTrack;
	snapshot->selectionEndRow = patternSelectionEndRow;
	snapshot->selectionStartFineField = patternSelectionStartFineField;
	snapshot->selectionEndFineField = patternSelectionEndFineField;
	snapshot->selectionFineMode = patternSelectionFineMode;
}

void CViewGT2Patterns::RestoreUndoViewState(const CGT2UndoSnapshot &snapshot)
{
	eparpcol = snapshot.cursorArpColumn;
	patternSelectionActive = snapshot.selectionActive;
	patternSelectionStartTrack = snapshot.selectionStartTrack;
	patternSelectionStartRow = snapshot.selectionStartRow;
	patternSelectionEndTrack = snapshot.selectionEndTrack;
	patternSelectionEndRow = snapshot.selectionEndRow;
	patternSelectionStartFineField = snapshot.selectionStartFineField;
	patternSelectionEndFineField = snapshot.selectionEndFineField;
	patternSelectionFineMode = snapshot.selectionFineMode;
}

void CViewGT2Patterns::OnUndoHistoryRestored()
{
	ClearAllChannelRowSpill();
}

bool CViewGT2Patterns::CommitPatternUndoSnapshotIfChanged(const PatternUndoSnapshot &before)
{
	// Value mode: drop any speed-table entry this edit left unreferenced, so
	// the table stays compact. The sweep is part of the same undo step, and it
	// is a pattern-editor concern -- table edits and instrument loads commit
	// straight to the history without it.
	SweepUnusedSpeedtableEntries();
	if (!GT2UndoHistory()->CommitIfChanged(before))
		return false;
	// Any pattern edit other than Insert/Remove Row invalidates the spill
	// stash; those two suppress this so a contiguous run can round-trip.
	if (!preservingRowSpill)
		ClearAllChannelRowSpill();
	return true;
}

bool CViewGT2Patterns::CanUndoPatternEdit() const
{
	return GT2UndoHistory()->CanUndo();
}

bool CViewGT2Patterns::CanRedoPatternEdit() const
{
	return GT2UndoHistory()->CanRedo();
}

bool CViewGT2Patterns::UndoPatternEdit()
{
	if (!GT2UndoHistory()->Undo())
		return false;
	OnUndoHistoryRestored();
	return true;
}

bool CViewGT2Patterns::RedoPatternEdit()
{
	if (!GT2UndoHistory()->Redo())
		return false;
	OnUndoHistoryRestored();
	return true;
}

void CViewGT2Patterns::ClearPatternUndoHistory()
{
	GT2UndoHistory()->Clear();
	pendingPatternUndoSnapshotActive = false;
	ClearAllChannelRowSpill();
}

void CViewGT2Patterns::BeginPatternUndoStep()
{
	if (pendingPatternUndoSnapshotActive)
		return;
	pendingPatternUndoSnapshot = CapturePatternUndoSnapshot();
	pendingPatternUndoSnapshotActive = true;
}

bool CViewGT2Patterns::CommitPatternUndoStep()
{
	if (!pendingPatternUndoSnapshotActive)
		return false;
	PatternUndoSnapshot before = pendingPatternUndoSnapshot;
	pendingPatternUndoSnapshotActive = false;
	return CommitPatternUndoSnapshotIfChanged(before);
}

void CViewGT2Patterns::CancelPatternUndoStep()
{
	pendingPatternUndoSnapshotActive = false;
}

void CViewGT2Patterns::GetPatternSelectionBounds(int *trackMin, int *trackMax, int *rowMin, int *rowMax) const
{
	if (trackMin) *trackMin = std::min(patternSelectionStartTrack, patternSelectionEndTrack);
	if (trackMax) *trackMax = std::max(patternSelectionStartTrack, patternSelectionEndTrack);
	if (rowMin) *rowMin = std::min(patternSelectionStartRow, patternSelectionEndRow);
	if (rowMax) *rowMax = std::max(patternSelectionStartRow, patternSelectionEndRow);
}

bool CViewGT2Patterns::IsPatternCellSelected(int channel, int patternRow, int fieldColumn) const
{
	if (!patternSelectionActive)
		return false;

	int rowMin, rowMax;
	GetPatternSelectionBounds(NULL, NULL, &rowMin, &rowMax);
	if (patternRow < rowMin || patternRow > rowMax)
		return false;

	if (patternSelectionFineMode)
	{
		// Fine-grained mode: highlight is a rectangle in
		// (channel*fieldsPerChannel + fineField, row) space, where fineField
		// indexes the visual sub-columns of one channel — note(0), instr(1),
		// arp0..N(2..1+N), cmd(2+N). A drag through only the note column
		// highlights notes only, not the whole main track.
		int cellFine = FineFieldFromFieldColumn(fieldColumn);
		int tracksPerChannel = 1 + numarpcolumns;
		int fieldsPerCh = FieldsPerChannel();
		int startCh = patternSelectionStartTrack / tracksPerChannel;
		int endCh   = patternSelectionEndTrack   / tracksPerChannel;
		int startAbs = startCh * fieldsPerCh + patternSelectionStartFineField;
		int endAbs   = endCh   * fieldsPerCh + patternSelectionEndFineField;
		int lo = std::min(startAbs, endAbs);
		int hi = std::max(startAbs, endAbs);
		// Sustain column doesn't own a fine field of its own — it shadows
		// the row's CMD_SETSR data. Treat it as a satellite of the note
		// column: when this channel's note is in the selection, light up
		// the sustain digit too. Same one-way pairing as note→instr.
		if (cellFine < 0)
		{
			if (gt2VisibleSustainColumn && fieldColumn == 9)
			{
				int noteAbs = channel * fieldsPerCh + 0;
				if (noteAbs >= lo && noteAbs <= hi)
					return true;
			}
			return false;
		}
		int cellAbs = channel * fieldsPerCh + cellFine;
		bool inRect = (cellAbs >= lo && cellAbs <= hi);
		// Note and instrument on a main track are conceptually paired (the
		// instrument plays the note), so any selection that touches a
		// channel's main note column also highlights that channel's
		// instrument cells. The reverse is not implied — selecting just the
		// instrument column stays narrow. Arp tracks are not affected.
		if (!inRect && cellFine == 1)
		{
			int noteAbs = channel * fieldsPerCh + 0;
			if (noteAbs >= lo && noteAbs <= hi)
				inRect = true;
		}
		if (!inRect)
			return false;
		// Trim the leading 1-space gutter of arp/cmd fields when the
		// selection stays inside a single track — that gutter only earns
		// its highlight when a multi-track selection needs the columns to
		// visually connect into one block.
		bool singleTrack = (patternSelectionStartTrack == patternSelectionEndTrack);
		if (singleTrack)
		{
			int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
			int arpStart = 9 + sustainPad;
			int cmdBaseColumn = arpStart + numarpcolumns * 4;
			bool isArpField = (cellFine >= 2 && cellFine < 2 + numarpcolumns);
			bool isCmdField = (cellFine == 2 + numarpcolumns);
			if (isArpField)
			{
				int arpK = cellFine - 2;
				if (fieldColumn == arpStart + arpK * 4)
					return false;     // leading space of arp k
			}
			else if (isCmdField)
			{
				if (fieldColumn == cmdBaseColumn)
					return false;     // leading space of command group
			}
		}
		return true;
	}

	// Legacy whole-track mode (Ctrl+A, tests, programmatic selections):
	// every track in the range highlights its full set of cells — main track
	// owns note + instr (left) + command (right); arp tracks own their 4-col
	// group between instr and command.
	int trackMin = std::min(patternSelectionStartTrack, patternSelectionEndTrack);
	int trackMax = std::max(patternSelectionStartTrack, patternSelectionEndTrack);
	int legacySustainPad = gt2VisibleSustainColumn ? 1 : 0;
	int legacyArpStart = 9 + legacySustainPad;
	int cmdBaseColumn = legacyArpStart + numarpcolumns * 4;
	for (int track = trackMin; track <= trackMax; track++)
	{
		int trackChannel, arpColumn;
		if (!GetPatternTrackInfo(track, &trackChannel, &arpColumn) || trackChannel != channel)
			continue;
		if (arpColumn < 0)
		{
			int cmdEnd = cmdBaseColumn + 1 + (gt2CommandValueMode ? 4 : 2);
			if ((fieldColumn >= 4 && fieldColumn <= 8)
				|| (fieldColumn >= cmdBaseColumn && fieldColumn <= cmdEnd))
				return true;
		}
		else
		{
			int firstColumn = legacyArpStart + arpColumn * 4;
			if (fieldColumn >= firstColumn && fieldColumn <= firstColumn + 3)
				return true;
		}
	}
	return false;
}

bool CViewGT2Patterns::CopyPatternSelection()
{
	if (!patternSelectionActive)
		return false;

	int trackMin, trackMax, rowMin, rowMax;
	GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
	patternClipboardWidth = trackMax - trackMin + 1;
	patternClipboardHeight = rowMax - rowMin + 1;
	patternClipboard.clear();
	patternClipboard.resize(patternClipboardWidth * patternClipboardHeight);

	// Fine-mode selections carry a kind (note / instrument / command) when
	// both corners of the rectangle land on the same sub-column. That makes
	// "copy only the command column across N rows" actually copy only the
	// command bytes — without this, a multi-row column selection would
	// silently fall back to whole-cell copy and the paste would clobber
	// notes + instruments. Mixed-kind selections (e.g. dragging from note
	// into cmd) keep fineKind = NONE so the paste stays whole-cell as
	// before. Arp sub-fields don't need a kind — the arp track index in
	// the per-cell loop already addresses the right slot.
	int columnFineKind = PFC_FINE_NONE;
	if (patternSelectionFineMode
		&& patternSelectionStartFineField == patternSelectionEndFineField)
	{
		int ff = patternSelectionStartFineField;
		if (ff == 0)                       columnFineKind = PFC_FINE_NOTE;
		else if (ff == 1)                  columnFineKind = PFC_FINE_INSTR;
		else if (ff == 2 + numarpcolumns)  columnFineKind = PFC_FINE_CMD;
	}

	for (int row = rowMin; row <= rowMax; row++)
	{
		for (int track = trackMin; track <= trackMax; track++)
		{
			PatternClipboardCell &cell = patternClipboard[(row - rowMin) * patternClipboardWidth + (track - trackMin)];
			cell.valid = false;
			cell.isArp = false;
			cell.fineKind = columnFineKind;
			memset(cell.data, 0, sizeof(cell.data));
			int channel, arpColumn;
			if (!GetPatternTrackInfo(track, &channel, &arpColumn))
				continue;
			int pattNum = epnum[channel];
			cell.isArp = arpColumn >= 0;
			if (row < 0 || row >= pattlen[pattNum])
				continue;
			cell.valid = true;
			if (cell.isArp)
				cell.data[0] = arpdata[pattNum][channel][row][arpColumn];
			else
				memcpy(cell.data, &pattern[pattNum][row * 4], 4);
		}
	}
	return true;
}

static u8 GT2PatternNoteForMainTrack(u8 note)
{
	if (note == 0) return REST;
	if ((note >= FIRSTNOTE && note <= LASTNOTE) || note == REST || note == KEYOFF || note == KEYON)
		return note;
	return REST;
}

static u8 GT2PatternNoteForArpTrack(u8 note)
{
	if (note >= FIRSTNOTE && note <= LASTNOTE) return note;
	if (note == KEYOFF) return KEYOFF;
	return 0;
}

void CViewGT2Patterns::FormatMainTrackNoteNameForDisplay(u8 note, char out[4])
{
	if (note == KEYOFF)
	{
		snprintf(out, 4, "OFF");
		return;
	}
	snprintf(out, 4, "%s", notename[note - FIRSTNOTE]);
}

bool CViewGT2Patterns::PastePatternClipboardAt(int track, int row)
{
	if (patternClipboard.empty() || patternClipboardWidth <= 0 || patternClipboardHeight <= 0)
		return false;

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = false;
	for (int y = 0; y < patternClipboardHeight; y++)
	{
		for (int x = 0; x < patternClipboardWidth; x++)
		{
			int dstTrack = track + x;
			int dstRow = row + y;
			int channel, arpColumn;
			if (!GetPatternTrackInfo(dstTrack, &channel, &arpColumn))
				continue;
			int pattNum = epnum[channel];
			if (dstRow < 0 || dstRow >= pattlen[pattNum])
				continue;
			const PatternClipboardCell &cell = patternClipboard[y * patternClipboardWidth + x];
			if (!cell.valid)
				continue;
			if (cell.fineKind != PFC_FINE_NONE)
			{
				// Fine-grained single-cell paste. Source kind decides which
				// bytes are written so we don't clobber surrounding fields.
				if (cell.fineKind == PFC_FINE_NOTE)
				{
					if (arpColumn >= 0)
						arpdata[pattNum][channel][dstRow][arpColumn] = GT2PatternNoteForArpTrack(cell.data[0]);
					else
					{
						pattern[pattNum][dstRow * 4]     = GT2PatternNoteForMainTrack(cell.data[0]);
						pattern[pattNum][dstRow * 4 + 1] = cell.data[1];   // instr
					}
				}
				else if (cell.fineKind == PFC_FINE_INSTR)
				{
					if (arpColumn < 0)
						pattern[pattNum][dstRow * 4 + 1] = cell.data[1];
				}
				else if (cell.fineKind == PFC_FINE_CMD)
				{
					if (arpColumn < 0)
					{
						pattern[pattNum][dstRow * 4 + 2] = cell.data[2];
						pattern[pattNum][dstRow * 4 + 3] = cell.data[3];
					}
				}
			}
			else if (arpColumn >= 0)
			{
				arpdata[pattNum][channel][dstRow][arpColumn] = GT2PatternNoteForArpTrack(cell.data[0]);
			}
			else if (cell.isArp)
			{
				pattern[pattNum][dstRow * 4] = GT2PatternNoteForMainTrack(cell.data[0]);
			}
			else
			{
				memcpy(&pattern[pattNum][dstRow * 4], cell.data, 4);
			}
			changed = true;
		}
	}
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::CopyPatternCell(int track, int row)
{
	int channel, arpColumn;
	if (!GetPatternTrackInfo(track, &channel, &arpColumn))
		return false;
	int pattNum = epnum[channel];
	if (row < 0 || row >= pattlen[pattNum])
		return false;

	patternClipboardWidth = 1;
	patternClipboardHeight = 1;
	patternClipboard.clear();
	patternClipboard.resize(1);
	PatternClipboardCell &cell = patternClipboard[0];
	cell.valid = true;
	cell.isArp = arpColumn >= 0;
	memset(cell.data, 0, sizeof(cell.data));
	if (cell.isArp)
		cell.data[0] = arpdata[pattNum][channel][row][arpColumn];
	else
		memcpy(cell.data, &pattern[pattNum][row * 4], 4);
	return true;
}

bool CViewGT2Patterns::ClearPatternCell(int track, int row)
{
	int channel, arpColumn;
	if (!GetPatternTrackInfo(track, &channel, &arpColumn))
		return false;
	int pattNum = epnum[channel];
	if (row < 0 || row >= pattlen[pattNum])
		return false;
	if (arpColumn >= 0)
	{
		arpdata[pattNum][channel][row][arpColumn] = 0;
	}
	else
	{
		pattern[pattNum][row * 4] = REST;
		pattern[pattNum][row * 4 + 1] = 0;
		pattern[pattNum][row * 4 + 2] = 0;
		pattern[pattNum][row * 4 + 3] = 0;
	}
	return true;
}

bool CViewGT2Patterns::CutPatternSelection()
{
	if (!CopyPatternSelection())
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	int trackMin, trackMax, rowMin, rowMax;
	GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
	bool changed = false;

	// Fine-mode single-cell cut: clear only the bytes that the clipboard
	// will carry so the surrounding fields on that row are preserved.
	bool singleCellFine = !patternClipboard.empty() && patternClipboard.front().fineKind != PFC_FINE_NONE
		&& trackMin == trackMax && rowMin == rowMax;
	if (singleCellFine)
	{
		int channel, arpColumn;
		if (GetPatternTrackInfo(trackMin, &channel, &arpColumn) && arpColumn < 0)
		{
			int pattNum = epnum[channel];
			if (rowMin >= 0 && rowMin < pattlen[pattNum])
			{
				int fk = patternClipboard.front().fineKind;
				if (fk == PFC_FINE_NOTE)
				{
					pattern[pattNum][rowMin * 4]     = REST;
					pattern[pattNum][rowMin * 4 + 1] = 0;
				}
				else if (fk == PFC_FINE_INSTR)
				{
					pattern[pattNum][rowMin * 4 + 1] = 0;
				}
				else if (fk == PFC_FINE_CMD)
				{
					pattern[pattNum][rowMin * 4 + 2] = 0;
					pattern[pattNum][rowMin * 4 + 3] = 0;
				}
				changed = true;
			}
		}
	}
	else
	{
		for (int row = rowMin; row <= rowMax; row++)
			for (int track = trackMin; track <= trackMax; track++)
				changed = ClearPatternCell(track, row) || changed;
	}
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::TransposePatternCell(int track, int row, int semitones)
{
	int channel, arpColumn;
	if (!GetPatternTrackInfo(track, &channel, &arpColumn))
		return false;
	int pattNum = epnum[channel];
	if (row < 0 || row >= pattlen[pattNum])
		return false;
	u8 *note = arpColumn >= 0 ? &arpdata[pattNum][channel][row][arpColumn] : &pattern[pattNum][row * 4];
	if (*note < FIRSTNOTE || *note > LASTNOTE)
		return false;
	int transposed = (int)*note + semitones;
	if (transposed < FIRSTNOTE) transposed = FIRSTNOTE;
	if (transposed > LASTNOTE) transposed = LASTNOTE;
	*note = (u8)transposed;
	return true;
}

bool CViewGT2Patterns::TransposePatternSelection(int semitones)
{
	if (!patternSelectionActive)
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	int trackMin, trackMax, rowMin, rowMax;
	GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
	bool changed = false;
	for (int row = rowMin; row <= rowMax; row++)
		for (int track = trackMin; track <= trackMax; track++)
			changed = TransposePatternCell(track, row, semitones) || changed;
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::SelectWholePattern()
{
	int maxRow = 0;
	for (int channel = 0; channel < MAX_CHN; channel++)
		if (pattlen[epnum[channel]] > maxRow)
			maxRow = pattlen[epnum[channel]];
	if (maxRow > 0)
		maxRow--;
	patternSelectionActive = true;
	patternSelectionDragging = false;
	patternSelectionMoved = true;
	patternSelectionStartTrack = 0;
	patternSelectionStartRow = 0;
	patternSelectionEndTrack = GetPatternTrackIndex(MAX_CHN - 1, numarpcolumns > 0 ? numarpcolumns - 1 : -1);
	patternSelectionEndRow = maxRow;
	// Whole-pattern selection spans every fine sub-column too: note of chan 0
	// through the command of the last channel.
	patternSelectionStartFineField = 0;
	patternSelectionEndFineField = FieldsPerChannel() - 1;
	patternSelectionFineMode = false;
	return true;
}

bool CViewGT2Patterns::CopyEffectsAtCursorOrSelection()
{
	effectClipboard.clear();
	int pattNum = epnum[epchn];
	if (patternSelectionActive)
	{
		int rowMin, rowMax;
		GetPatternSelectionBounds(NULL, NULL, &rowMin, &rowMax);
		for (int row = rowMin; row <= rowMax; row++)
		{
			if (row < 0 || row >= pattlen[pattNum])
				break;
			EffectClipboardCell cell;
			cell.command = pattern[pattNum][row * 4 + 2];
			cell.commandData = pattern[pattNum][row * 4 + 3];
			effectClipboard.push_back(cell);
		}
	}
	else if (eppos >= 0 && eppos < pattlen[pattNum])
	{
		EffectClipboardCell cell;
		cell.command = pattern[pattNum][eppos * 4 + 2];
		cell.commandData = pattern[pattNum][eppos * 4 + 3];
		effectClipboard.push_back(cell);
	}
	return !effectClipboard.empty();
}

bool CViewGT2Patterns::PasteEffectsAtCursor()
{
	if (effectClipboard.empty())
		return false;
	int pattNum = epnum[epchn];
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = false;
	for (int i = 0; i < (int)effectClipboard.size(); i++)
	{
		int row = eppos + i;
		if (row < 0 || row >= pattlen[pattNum])
			break;
		pattern[pattNum][row * 4 + 2] = effectClipboard[i].command;
		pattern[pattNum][row * 4 + 3] = effectClipboard[i].commandData;
		changed = true;
	}
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

std::vector<int> CViewGT2Patterns::GetCurrentChannelTracks() const
{
	std::vector<int> tracks;
	tracks.push_back(GetPatternTrackIndex(epchn, -1));
	for (int arpColumn = 0; arpColumn < numarpcolumns; arpColumn++)
		tracks.push_back(GetPatternTrackIndex(epchn, arpColumn));
	return tracks;
}

bool CViewGT2Patterns::ReverseRowsInTracks(const std::vector<int> &tracks, int rowMin, int rowMax)
{
	if (rowMax <= rowMin)
		return false;
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int left = rowMin;
		int right = rowMax;
		while (left < right)
		{
			PatternClipboardCell a;
			PatternClipboardCell b;
			ReadPatternBlockCell(tracks[i], left, &a);
			ReadPatternBlockCell(tracks[i], right, &b);
			WritePatternBlockCell(tracks[i], left, b.valid ? &b : NULL);
			WritePatternBlockCell(tracks[i], right, a.valid ? &a : NULL);
			left++;
			right--;
		}
	}
	return true;
}

bool CViewGT2Patterns::InvertSelectionOrPattern()
{
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = false;
	if (patternSelectionActive)
	{
		int trackMin, trackMax, rowMin, rowMax;
		GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
		std::vector<int> tracks;
		for (int track = trackMin; track <= trackMax; track++)
			tracks.push_back(track);
		changed = ReverseRowsInTracks(tracks, rowMin, rowMax);
	}
	else
	{
		changed = ReverseRowsInTracks(GetCurrentChannelTracks(), 0, pattlen[epnum[epchn]] - 1);
	}
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::ShrinkSelectionOrPattern()
{
	if (patternSelectionActive)
		return ShrinkPatternSelection();
	int pattLen = pattlen[epnum[epchn]];
	if (pattLen < 2)
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
		ShrinkPatternColumn(tracks[i], 0, pattLen - 1);
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::ExpandSelectionOrPattern()
{
	if (patternSelectionActive)
		return ExpandPatternSelection();
	int pattLen = pattlen[epnum[epchn]];
	if (pattLen < 2)
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
		ExpandPatternColumn(tracks[i], 0, pattLen - 1);
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::MakeHiFiVibratoPortaSpeed()
{
	int pattNum = epnum[epchn];
	if (eppos < 0 || eppos >= pattlen[pattNum])
		return false;
	unsigned char command = pattern[pattNum][eppos * 4 + 2];
	if (command != CMD_PORTAUP && command != CMD_PORTADOWN && command != CMD_VIBRATO && command != CMD_TONEPORTA)
		return false;
	int row = (command == CMD_TONEPORTA) ? eppos - 1 : eppos;
	for (; row >= 0; row--)
	{
		unsigned char noteByte = pattern[pattNum][row * 4];
		if (noteByte < FIRSTNOTE || noteByte > LASTNOTE)
			continue;
		int note = noteByte - FIRSTNOTE;
		if (note > MAX_NOTES - 1)
			note--;
		int pitch1 = freqtbllo[note] | (freqtblhi[note] << 8);
		int pitch2 = freqtbllo[note + 1] | (freqtblhi[note + 1] << 8);
		int delta = pitch2 - pitch1;
		int right = pattern[pattNum][eppos * 4 + 3] & 0x0f;
		int left = pattern[pattNum][eppos * 4 + 3] >> 4;
		while (left--) delta <<= 1;
		while (right--) delta >>= 1;
		if (command == CMD_VIBRATO && delta > 0xff)
			delta = 0xff;
		int pos = makespeedtable(delta, MST_RAW, 1);
		PatternUndoSnapshot before = CapturePatternUndoSnapshot();
		pattern[pattNum][eppos * 4 + 3] = pos + 1;
		CommitPatternUndoSnapshotIfChanged(before);
		return true;
	}
	return false;
}

bool CViewGT2Patterns::AdjustHighlightStep(int delta)
{
	int old = stepsize;
	stepsize += delta;
	if (stepsize < 2) stepsize = 2;
	if (stepsize > MAX_PATTROWS) stepsize = MAX_PATTROWS;
	return stepsize != old;
}

bool CViewGT2Patterns::CycleAutoadvanceMode()
{
	autoadvance++;
	if (autoadvance > 2) autoadvance = 0;
	if (keypreset == KEY_TRACKER && autoadvance == 1)
		autoadvance = 2;
	return true;
}

bool CViewGT2Patterns::SplitPatternAtCursor()
{
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	int oldSongLen = songlen[esnum][epchn];
	splitpattern();
	if (songlen[esnum][epchn] == oldSongLen && pattlen[epnum[epchn]] == before.patternLengths[epnum[epchn]])
		return false;
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::JoinPatternAtCursor()
{
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	int oldSongLen = songlen[esnum][epchn];
	joinpattern();
	if (songlen[esnum][epchn] != oldSongLen || pattlen[epnum[epchn]] != before.patternLengths[epnum[epchn]])
	{
		CommitPatternUndoSnapshotIfChanged(before);
		return true;
	}

	int joinPos = -1;
	int currentPattern = -1;
	int nextPattern = -1;
	for (int pos = 0; pos < songlen[esnum][epchn] - 1; pos++)
	{
		int a = songorder[esnum][epchn][pos];
		int b = songorder[esnum][epchn][pos + 1];
		if (a < 0 || a >= MAX_PATT || b < 0 || b >= MAX_PATT)
			continue;
		if (pattlen[a] + pattlen[b] > MAX_PATTROWS)
			continue;
		joinPos = pos;
		currentPattern = a;
		nextPattern = b;
		break;
	}
	if (joinPos < 0)
		return false;

	int writeRow = pattlen[currentPattern];
	for (int row = 0; row < pattlen[nextPattern]; row++, writeRow++)
	{
		memcpy(&pattern[currentPattern][writeRow * 4], &pattern[nextPattern][row * 4], 4);
	}
	pattern[currentPattern][writeRow * 4] = ENDPATT;
	pattern[currentPattern][writeRow * 4 + 1] = 0;
	pattern[currentPattern][writeRow * 4 + 2] = 0;
	pattern[currentPattern][writeRow * 4 + 3] = 0;
	countpatternlengths();

	int savedEditPos = eseditpos;
	eseditpos = joinPos + 1;
	deleteorder();
	eseditpos = savedEditPos;
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

// The pattern track the cursor currently sits in (main note column or arp
// column of the cursor's channel).
int CViewGT2Patterns::GetCursorPatternTrack() const
{
	return GetPatternTrackIndex(epchn, eparpcol >= 0 ? eparpcol : -1);
}

// Selection-or-cursor-cell variants. Each acts on the active pattern
// selection, or falls back to the single cell under the cursor.

bool CViewGT2Patterns::CopyAtCursor()
{
	if (patternSelectionActive)
		return CopyPatternSelection();
	int track = GetCursorPatternTrack();
	return CopyPatternCell(track, eppos);
}

bool CViewGT2Patterns::CutAtCursor()
{
	if (patternSelectionActive)
		return CutPatternSelection();
	int track = GetCursorPatternTrack();
	if (!CopyPatternCell(track, eppos))
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	ClearPatternCell(track, eppos);
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::PasteAtCursor()
{
	int track = GetCursorPatternTrack();
	return PastePatternClipboardAt(track, eppos);
}

bool CViewGT2Patterns::TransposeAtCursor(int semitones)
{
	if (patternSelectionActive)
		return TransposePatternSelection(semitones);
	int track = GetCursorPatternTrack();
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = TransposePatternCell(track, eppos, semitones);
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::EraseAtCursor()
{
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = false;
	if (patternSelectionActive)
	{
		int trackMin, trackMax, rowMin, rowMax;
		GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
		for (int row = rowMin; row <= rowMax; row++)
			for (int track = trackMin; track <= trackMax; track++)
				changed = ClearPatternCell(track, row) || changed;
	}
	else
	{
		int track = GetCursorPatternTrack();
		changed = ClearPatternCell(track, eppos);
	}
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

// Read / write a single selection-block cell at (track, row). A read of a
// row outside the pattern length yields an invalid (empty) cell; a write to
// such a row is dropped. A NULL / invalid cell writes an empty note.
void CViewGT2Patterns::ReadPatternBlockCell(int track, int row, PatternClipboardCell *out) const
{
	out->valid = false;
	out->isArp = false;
	memset(out->data, 0, sizeof(out->data));
	int channel, arpColumn;
	if (!GetPatternTrackInfo(track, &channel, &arpColumn))
		return;
	out->isArp = arpColumn >= 0;
	int pattNum = epnum[channel];
	if (row < 0 || row >= pattlen[pattNum])
		return;
	out->valid = true;
	if (out->isArp)
		out->data[0] = arpdata[pattNum][channel][row][arpColumn];
	else
		memcpy(out->data, &pattern[pattNum][row * 4], 4);
}

void CViewGT2Patterns::WritePatternBlockCell(int track, int row, const PatternClipboardCell *cell)
{
	int channel, arpColumn;
	if (!GetPatternTrackInfo(track, &channel, &arpColumn))
		return;
	int pattNum = epnum[channel];
	if (row < 0 || row >= pattlen[pattNum])
		return;
	if (arpColumn >= 0)
	{
		arpdata[pattNum][channel][row][arpColumn] = (cell && cell->valid) ? cell->data[0] : 0;
	}
	else if (cell && cell->valid)
	{
		memcpy(&pattern[pattNum][row * 4], cell->data, 4);
	}
	else
	{
		pattern[pattNum][row * 4]     = REST;
		pattern[pattNum][row * 4 + 1] = 0;
		pattern[pattNum][row * 4 + 2] = 0;
		pattern[pattNum][row * 4 + 3] = 0;
	}
}

// Shrink one track's rows [rowMin, rowMax]: keep every 2nd row, compact them
// to the top, empty the freed tail. Returns the number of rows kept.
int CViewGT2Patterns::ShrinkPatternColumn(int track, int rowMin, int rowMax)
{
	int height = rowMax - rowMin + 1;
	int keptCount = (height + 1) / 2;   // ceil(height / 2)
	std::vector<PatternClipboardCell> kept(keptCount);
	for (int k = 0; k < keptCount; k++)
		ReadPatternBlockCell(track, rowMin + k * 2, &kept[k]);
	for (int r = 0; r < height; r++)
		WritePatternBlockCell(track, rowMin + r, r < keptCount ? &kept[r] : NULL);
	return keptCount;
}

// Expand one track's rows [rowMin, rowMax]: double the row spacing, writing
// downward. Rows past the pattern end are dropped by WritePatternBlockCell.
void CViewGT2Patterns::ExpandPatternColumn(int track, int rowMin, int rowMax)
{
	int height = rowMax - rowMin + 1;
	// Snapshot the source column first — writes go downward and would
	// otherwise overwrite not-yet-read source rows.
	std::vector<PatternClipboardCell> src(height);
	for (int i = 0; i < height; i++)
		ReadPatternBlockCell(track, rowMin + i, &src[i]);
	for (int i = 0; i < height; i++)
	{
		WritePatternBlockCell(track, rowMin + i * 2, &src[i]);
		if (i < height - 1)
			WritePatternBlockCell(track, rowMin + i * 2 + 1, NULL);
	}
}

bool CViewGT2Patterns::ShrinkPatternSelection()
{
	if (!patternSelectionActive)
		return false;
	int trackMin, trackMax, rowMin, rowMax;
	GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
	int height = rowMax - rowMin + 1;
	if (height < 2)
		return false;

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	int keptCount = (height + 1) / 2;   // ceil(height / 2)

	for (int track = trackMin; track <= trackMax; track++)
		ShrinkPatternColumn(track, rowMin, rowMax);

	patternSelectionStartRow = rowMin;
	patternSelectionEndRow = rowMin + keptCount - 1;

	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::ExpandPatternSelection()
{
	if (!patternSelectionActive)
		return false;
	int trackMin, trackMax, rowMin, rowMax;
	GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
	int height = rowMax - rowMin + 1;
	if (height < 2)
		return false;

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();

	for (int track = trackMin; track <= trackMax; track++)
		ExpandPatternColumn(track, rowMin, rowMax);

	// Grow the selection to the doubled height, clamped to the pattern
	// length of the cursor's channel.
	int newRowMax = rowMin + 2 * height - 2;
	int lastRow = pattlen[epnum[epchn]] - 1;
	if (lastRow > MAX_PATTROWS - 1) lastRow = MAX_PATTROWS - 1;
	if (newRowMax > lastRow) newRowMax = lastRow;
	if (newRowMax < rowMin) newRowMax = rowMin;
	patternSelectionStartRow = rowMin;
	patternSelectionEndRow = newRowMax;

	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

// ---------------------------------------------------------------------------
// Whole-channel operations. These act on every track of the cursor's channel
// (main note column + arp columns), spanning every row of that channel's
// pattern, regardless of any active selection. Cut/Copy use a dedicated
// channel clipboard that is independent of the selection/cell clipboard.

bool CViewGT2Patterns::TransposeTrack(int semitones)
{
	int pattLen = pattlen[epnum[epchn]];
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = false;
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
		for (int row = 0; row < pattLen; row++)
			changed = TransposePatternCell(tracks[i], row, semitones) || changed;
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::CopyTrack()
{
	int pattLen = pattlen[epnum[epchn]];
	if (pattLen <= 0)
		return false;
	trackClipboard.clear();
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(tracks[i], &channel, &arpColumn))
			continue;
		PhraseClipboardColumn col;
		col.channel = channel;
		col.arpColumn = arpColumn;
		col.cells.resize(pattLen);
		for (int row = 0; row < pattLen; row++)
			ReadPatternBlockCell(tracks[i], row, &col.cells[row]);
		trackClipboard.push_back(col);
	}
	return true;
}

bool CViewGT2Patterns::CutTrack()
{
	if (!CopyTrack())
		return false;
	int pattLen = pattlen[epnum[epchn]];
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
		for (int row = 0; row < pattLen; row++)
			WritePatternBlockCell(tracks[i], row, NULL);
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::PasteTrack()
{
	if (trackClipboard.empty())
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	for (size_t i = 0; i < trackClipboard.size(); i++)
	{
		const PhraseClipboardColumn &col = trackClipboard[i];
		if (col.arpColumn >= numarpcolumns)
			continue;
		int track = GetPatternTrackIndex(epchn, col.arpColumn);
		for (int row = 0; row < (int)col.cells.size(); row++)
			WritePatternBlockCell(track, row, &col.cells[row]);
	}
	return CommitPatternUndoSnapshotIfChanged(before);
}

bool CViewGT2Patterns::ShrinkTrack()
{
	int pattLen = pattlen[epnum[epchn]];
	if (pattLen < 2)
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
		ShrinkPatternColumn(tracks[i], 0, pattLen - 1);
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::ExpandTrack()
{
	int pattLen = pattlen[epnum[epchn]];
	if (pattLen < 2)
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	std::vector<int> tracks = GetCurrentChannelTracks();
	for (size_t i = 0; i < tracks.size(); i++)
		ExpandPatternColumn(tracks[i], 0, pattLen - 1);
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

// ---------------------------------------------------------------------------
// Whole-phrase operations. The phrase is every track of every channel: each
// channel's main note column plus its arp columns, spanning the full length of
// that channel's pattern. Cut/Copy use a dedicated phrase clipboard.

// All tracks composing the current phrase. With dedupeSharedMainPatterns set,
// main columns of channels that point to the same pattern number collapse to a
// single entry, so a non-idempotent transform is not applied twice to shared
// pattern data. Arp data is per-channel and never deduplicated.
std::vector<int> CViewGT2Patterns::GetPhraseTracks(bool dedupeSharedMainPatterns) const
{
	std::vector<int> tracks;
	int tracksPerChannel = 1 + numarpcolumns;
	int totalTracks = MAX_CHN * tracksPerChannel;
	bool mainPatternSeen[MAX_PATT];
	memset(mainPatternSeen, 0, sizeof(mainPatternSeen));
	for (int t = 0; t < totalTracks; t++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(t, &channel, &arpColumn))
			continue;
		if (arpColumn < 0 && dedupeSharedMainPatterns)
		{
			int p = epnum[channel];
			if (p < 0 || p >= MAX_PATT)
				continue;
			if (mainPatternSeen[p])
				continue;
			mainPatternSeen[p] = true;
		}
		tracks.push_back(t);
	}
	return tracks;
}

bool CViewGT2Patterns::TransposePhrase(int semitones)
{
	std::vector<int> tracks = GetPhraseTracks(true);
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool changed = false;
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(tracks[i], &channel, &arpColumn))
			continue;
		int pattLen = pattlen[epnum[channel]];
		for (int row = 0; row < pattLen; row++)
			changed = TransposePatternCell(tracks[i], row, semitones) || changed;
	}
	if (changed)
		CommitPatternUndoSnapshotIfChanged(before);
	return changed;
}

bool CViewGT2Patterns::CopyPhrase()
{
	std::vector<int> tracks = GetPhraseTracks(false);
	phraseClipboard.clear();
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(tracks[i], &channel, &arpColumn))
			continue;
		int pattLen = pattlen[epnum[channel]];
		PhraseClipboardColumn col;
		col.channel = channel;
		col.arpColumn = arpColumn;
		col.cells.resize(pattLen);
		for (int row = 0; row < pattLen; row++)
			ReadPatternBlockCell(tracks[i], row, &col.cells[row]);
		phraseClipboard.push_back(col);
	}
	return !phraseClipboard.empty();
}

bool CViewGT2Patterns::CutPhrase()
{
	if (!CopyPhrase())
		return false;
	std::vector<int> tracks = GetPhraseTracks(false);
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(tracks[i], &channel, &arpColumn))
			continue;
		int pattLen = pattlen[epnum[channel]];
		for (int row = 0; row < pattLen; row++)
			WritePatternBlockCell(tracks[i], row, NULL);
	}
	CommitPatternUndoSnapshotIfChanged(before);
	return true;
}

bool CViewGT2Patterns::PastePhrase()
{
	if (phraseClipboard.empty())
		return false;
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	for (size_t i = 0; i < phraseClipboard.size(); i++)
	{
		const PhraseClipboardColumn &col = phraseClipboard[i];
		if (col.arpColumn >= numarpcolumns)   // arp column no longer exists
			continue;
		int track = GetPatternTrackIndex(col.channel, col.arpColumn);
		for (int row = 0; row < (int)col.cells.size(); row++)
			WritePatternBlockCell(track, row, &col.cells[row]);
	}
	return CommitPatternUndoSnapshotIfChanged(before);
}

bool CViewGT2Patterns::ShrinkPhrase()
{
	std::vector<int> tracks = GetPhraseTracks(true);
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool any = false;
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(tracks[i], &channel, &arpColumn))
			continue;
		int pattLen = pattlen[epnum[channel]];
		if (pattLen < 2)
			continue;
		ShrinkPatternColumn(tracks[i], 0, pattLen - 1);
		any = true;
	}
	if (any)
		CommitPatternUndoSnapshotIfChanged(before);
	return any;
}

bool CViewGT2Patterns::ExpandPhrase()
{
	std::vector<int> tracks = GetPhraseTracks(true);
	PatternUndoSnapshot before = CapturePatternUndoSnapshot();
	bool any = false;
	for (size_t i = 0; i < tracks.size(); i++)
	{
		int channel, arpColumn;
		if (!GetPatternTrackInfo(tracks[i], &channel, &arpColumn))
			continue;
		int pattLen = pattlen[epnum[channel]];
		if (pattLen < 2)
			continue;
		ExpandPatternColumn(tracks[i], 0, pattLen - 1);
		any = true;
	}
	if (any)
		CommitPatternUndoSnapshotIfChanged(before);
	return any;
}

bool CViewGT2Patterns::HandlePatternSelectionShortcut(u32 keyCode, bool isShift)
{
	if (isShift && (keyCode == 'v' || keyCode == 'V' || keyCode == SDLK_V))
	{
		if (patternClipboard.empty() || patternClipboardWidth <= 0 || patternClipboardHeight <= 0)
			return false;
		int currentTrack = GetPatternTrackIndex(epchn, eparpcol >= 0 ? eparpcol : -1);
		PastePatternClipboardAt(currentTrack, eppos);
		return true;
	}

	if (!patternSelectionActive)
		return false;
	if (isShift)
	{
		if (keyCode == 'c' || keyCode == 'C' || keyCode == SDLK_C) { CopyPatternSelection(); return true; }
		if (keyCode == 'x' || keyCode == 'X' || keyCode == SDLK_X) { CutPatternSelection(); return true; }
		if (keyCode == 'q' || keyCode == 'Q' || keyCode == SDLK_Q) { TransposePatternSelection(12); return true; }
		if (keyCode == 'a' || keyCode == 'A' || keyCode == SDLK_A) { TransposePatternSelection(-12); return true; }
		if (keyCode == 'w' || keyCode == 'W' || keyCode == SDLK_W) { TransposePatternSelection(1); return true; }
		if (keyCode == 's' || keyCode == 'S' || keyCode == SDLK_S) { TransposePatternSelection(-1); return true; }
	}
	bool isDeleteKey = (keyCode == MTKEY_DELETE || keyCode == SDLK_DELETE);
	bool isBackspaceKey = (keyCode == MTKEY_BACKSPACE || keyCode == SDLK_BACKSPACE);
	// Renoise convention: plain Delete clears only the cell under the cursor
	// even when a selection is active — selection-wide erase lives on
	// Shift+Delete via EraseAtCursor. Returning false lets the downstream
	// renoise handler (HandleDeleteClearNote) own the actual erase so it
	// also honors recordmode / arp columns / edit-step advance.
	if (isDeleteKey)
		return false;
	// Shift+Backspace is reserved for Remove Row and must not be swallowed
	// here as a selection clear.
	if (isBackspaceKey && !isShift)
	{
		PatternUndoSnapshot before = CapturePatternUndoSnapshot();
		int trackMin, trackMax, rowMin, rowMax;
		GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
		bool changed = false;
		for (int row = rowMin; row <= rowMax; row++)
			for (int track = trackMin; track <= trackMax; track++)
				changed = ClearPatternCell(track, row) || changed;
		if (changed)
			CommitPatternUndoSnapshotIfChanged(before);
		return true;
	}
	return false;
}

void CViewGT2Patterns::SetCursorFromPatternTrack(int track, int row)
{
	int channel, arpColumn;
	if (!GetPatternTrackInfo(track, &channel, &arpColumn))
		return;
	if (row < 0 || row > pattlen[epnum[channel]])
		return;
	editmode = EDIT_PATTERN;
	epchn = channel;
	eppos = row;
	eparpcol = arpColumn;
	if (arpColumn >= 0)
	{
		epcolumn = 5;
		epInSustain = false;
	}
	// epInSustain for main-track moves is decided by SetCursorColumnFromColInChn
	// which the mouse path calls right after this.
}

// A pattern row counts as "having content" if its main note column carries a
// note-column event (a pitched note, key-off or key-on) or any non-zero
// instrument / command / command-data byte.
bool CViewGT2Patterns::PatternRowHasContent(int pattNum, int row) const
{
	const u8 *cell = &pattern[pattNum][row * 4];
	u8 note = cell[0];
	bool hasNoteEvent = (note >= FIRSTNOTE && note <= LASTNOTE)
		|| note == KEYOFF || note == KEYON;
	return hasNoteEvent || cell[1] != 0 || cell[2] != 0 || cell[3] != 0;
}

// Jump the cursor to an absolute row, clamped to the current channel's pattern,
// and recentre the view (same follow behaviour as arrow-key navigation).
void CViewGT2Patterns::JumpToPatternRow(int row)
{
	int pattLen = pattlen[epnum[epchn]];
	if (row < 0) row = 0;
	if (row > pattLen) row = pattLen;
	eppos = row;
}

// Move the cursor to the nearest row above (direction -1) or below (+1) that
// has content in the current channel's main column. No wrap-around.
void CViewGT2Patterns::MoveToRowWithNote(int direction)
{
	int pattNum = epnum[epchn];
	int pattLen = pattlen[pattNum];
	for (int row = eppos + direction; row >= 0 && row < pattLen; row += direction)
	{
		if (PatternRowHasContent(pattNum, row))
		{
			eppos = row;
			return;
		}
	}
}

void CViewGT2Patterns::ClearAllChannelRowSpill()
{
	for (size_t c = 0; c < channelRowSpill.size(); c++)
		channelRowSpill[c].clear();
}

// Insert an empty row at the cursor across every track of the cursor's channel
// (main note column + arp columns). Rows below shift down; the row pushed off
// the pattern bottom is stashed so a later Remove Row can restore it.
void CViewGT2Patterns::InsertChannelRow()
{
	int pattNum = epnum[epchn];
	int pattLen = pattlen[pattNum];
	int atRow = eppos;
	if (pattLen < 1 || atRow < 0 || atRow >= pattLen)
		return;

	int trackCount = 1 + numarpcolumns;

	// Capture the bottom row of every channel track before it spills off.
	ChannelRowSpillEntry spill;
	spill.pattNum = pattNum;
	spill.arpCols = numarpcolumns;
	spill.cells.resize(trackCount);
	for (int i = 0; i < trackCount; i++)
	{
		int track = GetPatternTrackIndex(epchn, i == 0 ? -1 : i - 1);
		ReadPatternBlockCell(track, pattLen - 1, &spill.cells[i]);
	}

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();

	for (int i = 0; i < trackCount; i++)
	{
		int track = GetPatternTrackIndex(epchn, i == 0 ? -1 : i - 1);
		// Shift down, bottom-up so source rows are not clobbered.
		for (int row = pattLen - 1; row > atRow; row--)
		{
			PatternClipboardCell cell;
			ReadPatternBlockCell(track, row - 1, &cell);
			WritePatternBlockCell(track, row, &cell);
		}
		WritePatternBlockCell(track, atRow, NULL);   // new empty row
	}

	preservingRowSpill = true;
	bool changed = CommitPatternUndoSnapshotIfChanged(before);
	preservingRowSpill = false;

	if (changed)
		channelRowSpill[epchn].push_back(spill);
}

// Remove the cursor row across every track of the cursor's channel. Rows below
// shift up; the freed pattern-bottom row is refilled from the spill stash when
// a matching entry is available, otherwise left empty.
void CViewGT2Patterns::RemoveChannelRow()
{
	int pattNum = epnum[epchn];
	int pattLen = pattlen[pattNum];
	int atRow = eppos;
	if (pattLen < 1 || atRow < 0 || atRow >= pattLen)
		return;

	int trackCount = 1 + numarpcolumns;

	// Take a spilled row to refill the bottom, if one matches the current
	// pattern / arp layout; a stale stash is discarded.
	std::vector<ChannelRowSpillEntry> &stack = channelRowSpill[epchn];
	bool haveSpill = false;
	ChannelRowSpillEntry spill;
	if (!stack.empty())
	{
		if (stack.back().pattNum == pattNum && stack.back().arpCols == numarpcolumns)
		{
			spill = stack.back();
			haveSpill = true;
		}
		else
		{
			stack.clear();
		}
	}

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();

	for (int i = 0; i < trackCount; i++)
	{
		int track = GetPatternTrackIndex(epchn, i == 0 ? -1 : i - 1);
		// Shift up, top-down so source rows are not clobbered.
		for (int row = atRow; row < pattLen - 1; row++)
		{
			PatternClipboardCell cell;
			ReadPatternBlockCell(track, row + 1, &cell);
			WritePatternBlockCell(track, row, &cell);
		}
		const PatternClipboardCell *bottom = haveSpill ? &spill.cells[i] : NULL;
		WritePatternBlockCell(track, pattLen - 1, bottom);
	}

	preservingRowSpill = true;
	CommitPatternUndoSnapshotIfChanged(before);
	preservingRowSpill = false;

	if (haveSpill)
		stack.pop_back();
}

int CViewGT2Patterns::GetPatternCellBackgroundColor(int channel, int patternRow, int fieldColumn, int cursorBackgroundColor) const
{
	int backgroundColor = -1;

	if (IsPatternCellSelected(channel, patternRow, fieldColumn))
		backgroundColor = 1;

	int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
	int arpStart = 9 + sustainPad;
	int cmdBaseColumn = arpStart + numarpcolumns * 4;

	int cmdGroupEnd = cmdBaseColumn + 1 + (gt2CommandValueMode ? 4 : 2);
	if (channel == epmarkchn && fieldColumn >= 4 && fieldColumn <= cmdGroupEnd)
	{
		bool isMarked = epmarkstart <= epmarkend
			? patternRow >= epmarkstart && patternRow <= epmarkend
			: patternRow <= epmarkstart && patternRow >= epmarkend;
		if (isMarked) backgroundColor = 1;
	}

	if (editmode == EDIT_PATTERN && !eamode && channel == epchn && patternRow == eppos)
	{
		if (epInSustain && sustainPad)
		{
			// Sustain digit sits directly at column 9 (no leading space).
			if (fieldColumn == 9)
				backgroundColor = cursorBackgroundColor;
		}
		else if (eparpcol >= 0 && eparpcol < numarpcolumns)
		{
			int firstColumn = arpStart + 1 + eparpcol * 4;
			if (fieldColumn >= firstColumn && fieldColumn < firstColumn + 3)
				backgroundColor = cursorBackgroundColor;
		}
		else if (epcolumn == 0)
		{
			// note glyphs occupy cols 4..6.
			if (fieldColumn >= 4 && fieldColumn < 7)
				backgroundColor = cursorBackgroundColor;
		}
		else if (epcolumn <= 2)
		{
			// instrument hi/lo nibble at cols 7..8 (epcolumn 1→7, 2→8).
			if (fieldColumn == 6 + epcolumn)
				backgroundColor = cursorBackgroundColor;
		}
		else
		{
			// command area. In value mode a speed command's value field
			// spans 4 digits; the cursor highlights the active digit.
			if (gt2CommandValueMode && epcolumn >= 4 && CursorRowCommandUsesValueField())
			{
				if (fieldColumn == cmdBaseColumn + 2 + commandValueDigit)
					backgroundColor = cursorBackgroundColor;
			}
			else if (fieldColumn == cmdBaseColumn + (epcolumn - 2))
			{
				backgroundColor = cursorBackgroundColor;
			}
		}
	}

	return backgroundColor;
}

u8 CViewGT2Patterns::ApplyPatternCellBackground(u8 colorIndex, int backgroundColor) const
{
	if (backgroundColor < 0) return colorIndex;
	return (u8)((colorIndex & 0x0F) | ((backgroundColor & 0x0F) << 4));
}

static void DrawPatternTextGT2(CViewGT2Patterns *view, ImDrawList *dl, CGT2FontAtlas *fontAtlas,
							  float px, float py, int channel, int patternRow, int fieldColumn,
							  u8 colorIndex, const char *text, int cursorBackgroundColor,
							  float gapBeforePx = 0.0f)
{
	// If the caller offset this field by a half cell (note→instr or
	// instr→sustain gutter), and the field's first cell has a real
	// background colour (selection / cursor / mark), paint the gutter
	// with the same colour so a multi-cell selection reads as a
	// continuous strip instead of leaving black slivers between fields.
	if (gapBeforePx > 0.0f && text[0])
	{
		int backgroundColor = view->GetPatternCellBackgroundColor(channel, patternRow, fieldColumn, cursorBackgroundColor);
		if (backgroundColor >= 0)
		{
			u8 packed = view->ApplyPatternCellBackground(colorIndex, backgroundColor);
			ImU32 bgColor = fontAtlas->palette[(packed >> 4) & 0x0F];
			dl->AddRectFilled(
				ImVec2(px - gapBeforePx, py),
				ImVec2(px, py + GT2CellH()),
				bgColor);
		}
	}
	char ch[2] = { 0, 0 };
	for (int i = 0; text[i]; i++)
	{
		int backgroundColor = view->GetPatternCellBackgroundColor(channel, patternRow, fieldColumn + i, cursorBackgroundColor);
		ch[0] = text[i];
		DrawTextGT2(dl, fontAtlas, px + i * GT2CellW(), py,
					view->ApplyPatternCellBackground(colorIndex, backgroundColor), ch);
	}
}

void CViewGT2Patterns::RenderImGui()
{
	// Command Value Mode: per-frame transient-edit safety net (design #5.8).
	UpdateCommandValueEditLifecycle();

	// Handle pending autoload (checked here because DoFrame depends on VICE running)
	if (pluginGoatTracker && pluginGoatTracker->autoloadPending && gt2_engine_ready)
	{
		pluginGoatTracker->autoloadPending = false;
		char lastPath[512];
		lastPath[0] = 0;
		if (pluginGoatTracker->gt2Config)
			pluginGoatTracker->gt2Config->GetString("GoatTrackerSongPath", lastPath, sizeof(lastPath), "");
		LOGD("GT2 autoload: engine ready, loading '%s'", lastPath);
		if (strlen(lastPath) > 0)
		{
			pluginGoatTracker->LoadSongFromFile(lastPath);
		}
	}

	PreRenderImGui();
	if (!fontAtlas->TryLoad()) { PostRenderImGui(); return; }

	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetCursorScreenPos();

	// Compute visible rows and channels from window size
	ImVec2 avail = ImGui::GetContentRegionAvail();
	float windowW = avail.x;
	float windowH = avail.y;

	int visibleRows = (int)(windowH / GT2CellH()) - 1; // -1 for header row
	if (visibleRows < 1) visibleRows = 1;

	// Renoise-style fixed cursor row: the cursor row is pinned to a fixed
	// vertical position and the pattern scrolls under it. epview is always
	// derived from eppos — never free-scrolled — so the current row stays on
	// the bar. The bar sits at visibleRows/3 by default (fewer rows above
	// than below, like Renoise) and can be toggled to visibleRows/2 from
	// the GT2 settings menu.
	int centerRow = gt2PatternCursorCentered ? (visibleRows / 2) : (visibleRows / 3);
	epview = eppos - centerRow;

	// Channel block layout (logical columns): row#(0-2) note(4-6) instr(7-8),
	// [sustain digit at col 9 when enabled,] arp columns (4 each), command
	// group on the right. The renderer adds half-cellW pixel offsets before
	// instrument / sustain / arp / cmd so those fields don't visually touch
	// note. chnBlockWidth carries one extra col of slack to absorb the
	// cumulative offset; hit-test, fine-fields and cursor highlight stay
	// integer-column based.
	int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
	int arpStartCol = 9 + sustainPad;
	int chnBlockWidth = 14 + numarpcolumns * 4 + (gt2CommandValueMode ? 4 : 2) + sustainPad;
	int cmdBaseCol = arpStartCol + numarpcolumns * 4;
	float halfCellPx = 0.5f * GT2CellW();
	float instrPxOffset   = halfCellPx;                          // gap before instr
	float sustainPxOffset = 2.0f * halfCellPx;                   // + gap before sustain
	float arpPxOffset     = sustainPad ? 2.0f * halfCellPx       // sustain on: cumulative
	                                    : halfCellPx;            // sustain off: just instr gap
	float cmdPxOffset     = arpPxOffset;
	int visibleChannels = (int)(windowW / (chnBlockWidth * GT2CellW()));
	if (visibleChannels > MAX_CHN) visibleChannels = MAX_CHN;
	if (visibleChannels < 1) visibleChannels = 1;

	int cc = cursorcolortable[cursorflash];

	char textbuffer[256];

	// Forked from gdisplay.c:145-244
	for (int c = 0; c < visibleChannels; c++)
	{
		// Channel header (text-mode row 2 -> our row 0)
		sprintf(textbuffer, "CHN %d PATT.%02X", c+1, epnum[c]);
		DrawTextGT2(dl, fontAtlas,
					origin.x + GT2ColToPixel(c*chnBlockWidth),
					origin.y + GT2RowToPixel(0),
					CTITLE, textbuffer);

		// "S" header for the optional sustain column.
		if (gt2VisibleSustainColumn)
		{
			DrawTextGT2(dl, fontAtlas,
						origin.x + GT2ColToPixel(9 + c*chnBlockWidth) + sustainPxOffset,
						origin.y + GT2RowToPixel(0),
						CTITLE, "S");
		}

		// Arp column headers
		for (int a = 0; a < numarpcolumns; a++)
		{
			char arpHdr[5];
			sprintf(arpHdr, " A%d ", a+1);
			DrawTextGT2(dl, fontAtlas,
						origin.x + GT2ColToPixel(arpStartCol + a*4 + c*chnBlockWidth) + arpPxOffset,
						origin.y + GT2RowToPixel(0),
						CTITLE, arpHdr);
		}

		for (int d = 0; d < visibleRows; d++)
		{
			int p = epview+d;
			int color = CNORMAL;

			if ((epnum[c] == chn[c].pattnum) && (isplaying()))
			{
				int chnrow = chn[c].pattptr / 4;
				if (chnrow > pattlen[chn[c].pattnum]) chnrow = pattlen[chn[c].pattnum];
				if (chnrow == p) color = CPLAYING;
			}

			bool channelMuted = chn[c].mute;
			if (pluginGoatTracker && pluginGoatTracker->audioMixer)
				channelMuted = pluginGoatTracker->audioMixer->IsChannelEffectivelyMuted(c, MAX_CHN);
			if (channelMuted) color = CMUTE;
			if (p == eppos) color = CEDIT;

			if ((p < 0) || (p > pattlen[epnum[c]]))
			{
				sprintf(textbuffer, "             ");
			}
			else
			{
				if (!(patterndispmode & 1))
				{
					if (p < 100) sprintf(textbuffer, " %02d", p);
					else         sprintf(textbuffer, "%03d", p);
				}
				else
					sprintf(textbuffer, " %02X", p);

				if (pattern[epnum[c]][p*4] == ENDPATT)
				{
					sprintf(&textbuffer[3], " PATT. END");
					if (color == CNORMAL) color = CCOMMAND;
				}
				else
				{
					char mainNoteName[4];
					FormatMainTrackNoteNameForDisplay(pattern[epnum[c]][p*4], mainNoteName);
					sprintf(&textbuffer[3], " %s %02X%01X%02X",
						mainNoteName,
						pattern[epnum[c]][p*4+1],
						pattern[epnum[c]][p*4+2],
						pattern[epnum[c]][p*4+3]);
					if (patterndispmode & 2)
					{
						if (!pattern[epnum[c]][p*4+1])
							memset(&textbuffer[8], '.', 2);
						if (!pattern[epnum[c]][p*4+2])
							memset(&textbuffer[10], '.', 3);
					}
				}
			}

			// Row number field (3 chars, cols 0-2 of the channel block)
			// text-mode: printtext(2+c*chnBlockWidth, 3+d, ...) -> our row 1+d
			char rowNumBuf[4];
			rowNumBuf[0] = textbuffer[0];
			rowNumBuf[1] = textbuffer[1];
			rowNumBuf[2] = textbuffer[2];
			rowNumBuf[3] = '\0';

			u8 rowNumColor = (p % stepsize) ? (u8)CNORMAL : (u8)CCOMMAND;
			DrawTextGT2(dl, fontAtlas,
						origin.x + GT2ColToPixel(c*chnBlockWidth),
						origin.y + GT2RowToPixel(1+d),
						rowNumColor, rowNumBuf);

			// Note / instrument / arp / command fields. The command group is
			// drawn last, to the far right of the arp columns:
			//   note  instr  arp1 arp2 ...  command
			int color2 = (color == CNORMAL) ? CCOMMAND : color;

			bool isEndPatt = (p >= 0) && (p <= pattlen[epnum[c]])
							 && (pattern[epnum[c]][p*4] == ENDPATT);

			if (isEndPatt)
			{
				// "PATT. END" marker — textbuffer[3..12] drawn as one run.
				char tmp[12];
				memcpy(tmp, &textbuffer[3], 10);
				tmp[10] = '\0';
				DrawPatternTextGT2(this, dl, fontAtlas,
									origin.x + GT2ColToPixel(3+c*chnBlockWidth),
									origin.y + GT2RowToPixel(1+d),
									c, p, 3, (u8)color, tmp, cc);
			}
			else
			{
				// textbuffer[4..6]: note name (3 chars, no trailing space —
				// instrument sits immediately to the right at col 7).
				{
					char tmp[4];
					tmp[0] = textbuffer[4];
					tmp[1] = textbuffer[5];
					tmp[2] = textbuffer[6];
					tmp[3] = '\0';
					DrawPatternTextGT2(this, dl, fontAtlas,
										origin.x + GT2ColToPixel(4+c*chnBlockWidth),
										origin.y + GT2RowToPixel(1+d),
										c, p, 4, (u8)color2, tmp, cc);
				}

				// textbuffer[8..9]: instrument, drawn at logical cols 7..8
				// with a half-cellW pixel offset so it doesn't touch note.
				// The gutter between note and instr is back-filled by
				// DrawPatternTextGT2 when the instr cell carries a
				// selection / cursor / mark background.
				{
					char tmp[3];
					tmp[0] = textbuffer[8];
					tmp[1] = textbuffer[9];
					tmp[2] = '\0';
					DrawPatternTextGT2(this, dl, fontAtlas,
										origin.x + GT2ColToPixel(7+c*chnBlockWidth) + instrPxOffset,
										origin.y + GT2RowToPixel(1+d),
										c, p, 7, (u8)color, tmp, cc,
										instrPxOffset);
				}

				// Optional Sustain column — a single hex digit at logical
				// col 9 with a half-cellW pixel gap before it. The gutter
				// between instr and sustain is back-filled by
				// DrawPatternTextGT2 (sustainPxOffset - instrPxOffset).
				if (gt2VisibleSustainColumn && (p >= 0) && (p <= pattlen[epnum[c]]))
				{
					char susBuf[2];
					unsigned char rowCmd  = pattern[epnum[c]][p*4 + 2];
					unsigned char rowData = pattern[epnum[c]][p*4 + 3];
					if (rowCmd == CMD_SETSR)
					{
						unsigned susNib = (rowData >> 4) & 0x0F;
						susBuf[0] = (char)(susNib < 10 ? '0' + susNib : 'A' + susNib - 10);
					}
					else
						susBuf[0] = '.';
					susBuf[1] = '\0';
					DrawPatternTextGT2(this, dl, fontAtlas,
										origin.x + GT2ColToPixel(9 + c*chnBlockWidth) + sustainPxOffset,
										origin.y + GT2RowToPixel(1+d),
										c, p, 9, (u8)color, susBuf, cc,
										sustainPxOffset - instrPxOffset);
				}

				// Arp columns — between sustain (or instrument) and command.
				// Each is a 4-char field (leading space + 3-char note). The
				// cumulative pixel offset matches the offset already in
				// effect for the rest of the channel.
				if ((p >= 0) && (p <= pattlen[epnum[c]]))
				{
					for (int a = 0; a < numarpcolumns; a++)
					{
						unsigned char arpnote = arpdata[epnum[c]][c][p][a];
						char arpbuf[5];
						if (arpnote >= FIRSTNOTE && arpnote <= LASTNOTE)
							sprintf(arpbuf, " %s", notename[arpnote - FIRSTNOTE]);
						else if (arpnote == KEYOFF)
							sprintf(arpbuf, " OFF");
						else
							sprintf(arpbuf, " ...");

						DrawPatternTextGT2(this, dl, fontAtlas,
											origin.x + GT2ColToPixel(arpStartCol + a*4 + c*chnBlockWidth) + arpPxOffset,
											origin.y + GT2RowToPixel(1+d),
											c, p, arpStartCol + a*4, (u8)color, arpbuf, cc);
					}
				}

				// Command — far right, after the arp columns.
				bool realRow = (p >= 0) && (p <= pattlen[epnum[c]]);
				int cmdNib = realRow ? (pattern[epnum[c]][p*4+2] & 0x0F) : -1;
				bool emptyCmd = gt2CommandValueMode && realRow && cmdNib == 0;

				// command nibble (with a leading spacer column). textbuffer[10]
				// is the nibble glyph; an empty value-mode cell shows a dash.
				{
					char tmp[3];
					tmp[0] = ' ';
					tmp[1] = emptyCmd ? '-' : textbuffer[10];
					tmp[2] = '\0';
					DrawPatternTextGT2(this, dl, fontAtlas,
										origin.x + GT2ColToPixel(cmdBaseCol + c*chnBlockWidth) + cmdPxOffset,
										origin.y + GT2RowToPixel(1+d),
										c, p, cmdBaseCol, (u8)color2, tmp, cc);
				}

				// Command argument. In Command Value Mode a value-mode command
				// (1,2,3,4) shows the 16-bit speed-table value (4 hex digits);
				// a 1-byte command shows its 2 raw digits + "--" padding; an
				// empty cell shows "----". Otherwise the classic 2 raw digits.
				{
					char argBuf[6];
					if (gt2CommandValueMode)
					{
						if (!realRow)
						{
							argBuf[0] = argBuf[1] = argBuf[2] = argBuf[3] = ' ';
							argBuf[4] = '\0';
						}
						else if (IsValueModeCommand(cmdNib))
						{
							bool editingHere = commandValueEditing
								&& commandValueEditChn == c
								&& commandValueEditRow == p;
							if (editingHere)
								sprintf(argBuf, "%04X", commandValueEditValue & 0xFFFF);
							else if (pattern[epnum[c]][p*4+3] == 0)
								strcpy(argBuf, "----");
							else
								sprintf(argBuf, "%04X",
										GetSpeedtableValue(pattern[epnum[c]][p*4+3]));
						}
						else if (cmdNib == 0)
						{
							strcpy(argBuf, "----");   // empty cell
						}
						else
						{
							// 1-byte command: 2 raw digits + "--" padding
							argBuf[0] = textbuffer[11];
							argBuf[1] = textbuffer[12];
							argBuf[2] = '-';
							argBuf[3] = '-';
							argBuf[4] = '\0';
						}
					}
					else
					{
						argBuf[0] = textbuffer[11];
						argBuf[1] = textbuffer[12];
						argBuf[2] = '\0';
					}
					DrawPatternTextGT2(this, dl, fontAtlas,
										origin.x + GT2ColToPixel(cmdBaseCol + 2 + c*chnBlockWidth) + cmdPxOffset,
										origin.y + GT2RowToPixel(1+d),
										c, p, cmdBaseCol + 2, (u8)color, argBuf, cc);
				}
			}
		}
	}

	// Renoise-style centre bar: a translucent full-width strip marking the
	// current row, drawn on top of the pattern text.
	{
		float barY = origin.y + GT2RowToPixel(1 + centerRow);
		dl->AddRectFilled(
			ImVec2(origin.x, barY),
			ImVec2(origin.x + windowW, barY + GT2CellH()),
			IM_COL32(120, 150, 235, 55));
	}

	// Mouse click/drag handling — cursor follows the mouse, drag creates a 2D track selection.
	if ((ImGui::IsWindowHovered() || patternSelectionDragging) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		// Clamp the mouse x to the visible channel strip so dragging past the
		// right edge stays on the rightmost visible column instead of jumping
		// channels (mouseX outside the view yields huge gridCol values).
		float relX = mousePos.x - origin.x;
		float relMaxX = (float)(visibleChannels * chnBlockWidth) * GT2CellW() - 1.0f;
		if (relX < 0.0f) relX = 0.0f;
		if (relX > relMaxX) relX = relMaxX;
		int gridCol = GT2PixelToCol(relX);
		int gridRow = GT2PixelToRow(mousePos.y - origin.y);

		// Map mouse to a pattern row using the CURRENT epview. With the
		// centre-pinned scroll model (epview = eppos - centerRow), moving
		// eppos every frame shifts the view, which shifts the mouse-to-row
		// mapping, which would move eppos again — a feedback loop that ran
		// away faster the further the mouse was from the centre bar. So we
		// only move the cursor on the initial click; drag updates extend
		// the selection without touching eppos. Auto-scroll below moves
		// eppos when the mouse leaves the view, and the resulting epview
		// shift naturally drags the selection end with it next frame.
		bool mouseInContent = (gridRow >= 1);
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouseInContent)
		{
			int newRow = epview + (gridRow - 1);
			int newTrack = GetPatternTrackFromGridColumn(gridCol, visibleChannels);
			int newChn, newArpColumn;
			GetPatternTrackInfo(newTrack, &newChn, &newArpColumn);
			if (newRow >= 0 && newRow <= pattlen[epnum[newChn]])
			{
				int colInChn = gridCol - newChn * chnBlockWidth;
				int newFineField = FineFieldFromColInChn(colInChn);
				bool shiftHeld = ImGui::GetIO().KeyShift;
				if (shiftHeld)
				{
					// Shift-click: text-editor / Excel semantics. See
					// ExtendSelectionWithShiftClick for the full logic.
					ExtendSelectionWithShiftClick(newTrack, newFineField, newRow);
				}
				else
				{
					// Defer the cursor move to mouse-up so that building a
					// drag selection doesn't drag the cursor along too. A
					// click-without-drag still relocates the cursor — the
					// mouse-release handler does it when patternSelectionMoved
					// stays false.
					patternClickColInChn = colInChn;
					BeginMousePatternSelection(newTrack, newFineField, newRow);
				}
			}
		}
		else if (patternSelectionDragging)
		{
			// Clamp the mouse row into the visible content for the purpose
			// of extending the selection. When the user drags past the top
			// of the view, the selection end should track the topmost
			// visible row (which the auto-scroll below lowers as eppos
			// shrinks) instead of getting frozen at the row where the
			// pointer last sat inside the view. Same for the bottom edge.
			int clampedRow = gridRow;
			if (clampedRow < 1)             clampedRow = 1;
			if (clampedRow > visibleRows)   clampedRow = visibleRows;
			int newRow = epview + (clampedRow - 1);
			int newTrack = GetPatternTrackFromGridColumn(gridCol, visibleChannels);
			int newChn, newArpColumn;
			GetPatternTrackInfo(newTrack, &newChn, &newArpColumn);
			if (newRow < 0) newRow = 0;
			if (newRow > pattlen[epnum[newChn]]) newRow = pattlen[epnum[newChn]];
			int colInChn = gridCol - newChn * chnBlockWidth;
			int newFineField = FineFieldFromColInChn(colInChn);
			UpdateMousePatternSelection(newTrack, newFineField, newRow);
		}

		// Auto-scroll while dragging — only kicks in once the mouse leaves the
		// visible content (above the first content row or below the bottom of
		// the view). Speed is time-based and scales with overshoot so a small
		// nudge past the edge crawls, a deliberate drag accelerates. Previously
		// this ran one row per frame, which scrolled ~60 r/s at 60fps and
		// looked like a runaway as soon as the mouse touched the last row.
		if (patternSelectionDragging)
		{
			float dt = ImGui::GetIO().DeltaTime;
			float cellH      = GT2CellH();
			float deadZone   = cellH * 0.5f;            // ignore the first half-row past the edge
			float topEdge    = origin.y + cellH - deadZone;
			float bottomEdge = origin.y + windowH + deadZone;
			int dir = 0;
			float distOut = 0.0f;
			if (mousePos.y < topEdge)         { dir = -1; distOut = topEdge - mousePos.y; }
			else if (mousePos.y > bottomEdge) { dir = +1; distOut = mousePos.y - bottomEdge; }
			if (dir != 0)
			{
				// Slow, gentle acceleration. Base ~3 r/s right at the deadzone
				// edge; full speed only when the mouse is dragged well outside.
				float overshootRows = (cellH > 0.0f) ? distOut / cellH : 0.0f;
				float rowsPerSec = 3.0f + overshootRows * 3.0f;
				if (rowsPerSec > 30.0f) rowsPerSec = 30.0f;
				patternDragScrollAccum += dt * rowsPerSec;
				int whole = (int)patternDragScrollAccum;
				if (whole > 0)
				{
					patternDragScrollAccum -= (float)whole;
					int pattLen = pattlen[epnum[epchn]];
					int nextEppos = eppos + dir * whole;
					if (nextEppos < 0) nextEppos = 0;
					if (nextEppos > pattLen) nextEppos = pattLen;
					eppos = nextEppos;
				}
			}
			else
			{
				patternDragScrollAccum = 0.0f;
			}
		}
	}
	if (patternSelectionDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		patternSelectionDragging = false;
		patternDragScrollAccum = 0.0f;
		// Click-without-drag (selection never grew past the click cell):
		// move the cursor onto the clicked cell now. A drag selection leaves
		// the cursor where it was so building a selection doesn't relocate
		// the cursor as a side effect. The single-cell selection itself is
		// kept either way so cut / copy / paste act on that exact cell.
		if (!patternSelectionMoved)
		{
			SetCursorFromPatternTrack(patternSelectionStartTrack, patternSelectionStartRow);
			SetCursorColumnFromColInChn(patternClickColInChn);
		}
	}

	// Mouse wheel moves the cursor row; epview follows it (centred-row model).
	if (ImGui::IsWindowHovered())
	{
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f)
		{
			int scrollAmount = (int)(-wheel * 3);  // 3 rows per notch
			eppos += scrollAmount;
			int pattLen = pattlen[epnum[epchn]];
			if (eppos < 0) eppos = 0;
			if (eppos > pattLen) eppos = pattLen;
		}
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		int gridCol = GT2PixelToCol(mousePos.x - origin.x);
		int gridRow = GT2PixelToRow(mousePos.y - origin.y);
		contextMenuOnCommand = false;
		if (gridRow >= 1)
		{
			int newRow = epview + (gridRow - 1);
			int newTrack = GetPatternTrackFromGridColumn(gridCol, visibleChannels);
			int newChn, newArpColumn;
			GetPatternTrackInfo(newTrack, &newChn, &newArpColumn);
			if (newRow >= 0 && newRow <= pattlen[epnum[newChn]])
			{
				SetCursorFromPatternTrack(newTrack, newRow);
				SetCursorColumnFromColInChn(gridCol - newChn * chnBlockWidth);
				contextMenuOnCommand = (eparpcol < 0 && epcolumn >= 3);
			}
		}
		ImGui::OpenPopup("GT2PatternsContext");
	}
	RenderContextMenu();

	// "Generate Echo" popup — opened from the context menu via a deferred
	// request flag so OpenPopup happens outside the menu's BeginPopup scope.
	if (echoPopupRequested)
	{
		ImGui::OpenPopup("GT2GenerateEcho");
		echoPopupRequested = false;
	}
	RenderEchoPopup();

	ImVec2 frameMin, frameMax;
	if (GetWriteModeFrameRect(ImGui::GetWindowPos(), ImGui::GetWindowSize(), &frameMin, &frameMax))
	{
		dl->AddRect(
			frameMin,
			frameMax,
			IM_COL32(255, 0, 0, 255),
			0.0f, 0, 2.0f);
	}

	PostRenderImGui();
}

void CViewGT2Patterns::SetCursorColumnFromColInChn(int colInChn)
{
	int sustainPad = gt2VisibleSustainColumn ? 1 : 0;
	int arpStart = 9 + sustainPad;
	int cmdBase = arpStart + numarpcolumns * 4;
	// Sustain column (col 9 when enabled) — part of the main track, not
	// an arp / not an instr / cmd digit. We park epcolumn at 2 (instr-lo)
	// while we're in sustain; the dedicated epInSustain flag tells render
	// + key handlers which field actually owns the cursor.
	if (sustainPad && colInChn == 9)
	{
		eparpcol = -1;
		epInSustain = true;
		epcolumn = 2;
		return;
	}
	epInSustain = false;
	if (numarpcolumns > 0 && colInChn >= arpStart && colInChn < cmdBase)
	{
		int arpIdx = (colInChn - arpStart) / 4;
		if (arpIdx >= numarpcolumns) arpIdx = numarpcolumns - 1;
		eparpcol = arpIdx;
		epcolumn = 5; // park epcolumn at last normal col
		return;
	}
	eparpcol = -1;
	// Tightened layout: note cols 4..6, instr cols 7..8.
	if (colInChn <= 6)
		epcolumn = 0;
	else if (colInChn <= 8)
		epcolumn = 1 + (colInChn - 7);
	else if (colInChn >= cmdBase)
	{
		int rel = colInChn - cmdBase; // 0 spacer, 1 nibble, 2.. argument
		if (rel <= 1)
		{
			epcolumn = 3;
		}
		else if (gt2CommandValueMode && CursorRowCommandUsesValueField())
		{
			// value field — one cursor cell, 4 typed digits
			epcolumn = 4;
			commandValueDigit = rel - 2;
			if (commandValueDigit < 0) commandValueDigit = 0;
			if (commandValueDigit > 3) commandValueDigit = 3;
		}
		else
		{
			epcolumn = 4 + (rel - 2);
			if (epcolumn > 5) epcolumn = 5;
		}
	}
	else
		epcolumn = 1 + (colInChn - 7);
	if (epcolumn > 5) epcolumn = 5;
	if (epcolumn < 0) epcolumn = 0;
}

void CViewGT2Patterns::InsertPatternCommand(unsigned char cmdNibble)
{
	if (editmode != EDIT_PATTERN)
		return;
	int pn = epnum[epchn];
	if (eppos < 0 || eppos >= pattlen[pn])
		return;  // ENDPATT marker / out of range — not an editable row
	if (pattern[pn][eppos * 4] == ENDPATT)
		return;
	cmdNibble &= 0x0F;
	if ((pattern[pn][eppos * 4 + 2] & 0x0F) == cmdNibble)
		return;  // no change
	ChangeCommandNibble(epchn, eppos, cmdNibble);
}

// --- Command Value Mode -----------------------------------------------------

bool CViewGT2Patterns::IsSpeedCommand(int cmdNibble) const
{
	cmdNibble &= 0x0F;
	// GT2 commands that reference the speed table: portamento up/down, tone
	// portamento, vibrato, funktempo. Used for orphan reference counting.
	return cmdNibble == 1 || cmdNibble == 2 || cmdNibble == 3
		|| cmdNibble == 4 || cmdNibble == 14;
}

bool CViewGT2Patterns::IsValueModeCommand(int cmdNibble) const
{
	cmdNibble &= 0x0F;
	// Commands edited as a 4-digit speed-table value in Command Value Mode:
	// portamento up/down, tone portamento, vibrato. Funktempo (E) and the
	// table-pointer commands keep their classic 1-byte argument.
	return cmdNibble == 1 || cmdNibble == 2 || cmdNibble == 3 || cmdNibble == 4;
}

int CViewGT2Patterns::GetSpeedtableValue(int index1) const
{
	if (index1 <= 0 || index1 > MAX_TABLELEN)
		return 0;
	int pos = index1 - 1;
	return (ltable[STBL][pos] << 8) | rtable[STBL][pos];
}

int CViewGT2Patterns::FindSpeedtableEntry(int value16) const
{
	value16 &= 0xFFFF;
	if (value16 == 0)
		return -1;
	for (int pos = 0; pos < MAX_TABLELEN; pos++)
	{
		if (((ltable[STBL][pos] << 8) | rtable[STBL][pos]) == value16)
			return pos;
	}
	return -1;
}

int CViewGT2Patterns::CountSpeedtableRefs(int index1) const
{
	if (index1 <= 0)
		return 0;
	int count = 0;
	// Pattern commands
	for (int c = 0; c < MAX_PATT; c++)
	{
		for (int d = 0; d <= MAX_PATTROWS; d++)
		{
			if (IsSpeedCommand(pattern[c][d * 4 + 2])
				&& pattern[c][d * 4 + 3] == index1)
				count++;
		}
	}
	// Wavetable porta/vibrato/funktempo commands
	for (int c = 0; c < MAX_TABLELEN; c++)
	{
		if (ltable[WTBL][c] >= WAVECMD && ltable[WTBL][c] <= WAVELASTCMD)
		{
			if (IsSpeedCommand(ltable[WTBL][c] & 0x0F)
				&& rtable[WTBL][c] == index1)
				count++;
		}
	}
	// Instrument speed-table pointers
	for (int c = 1; c < MAX_INSTR; c++)
	{
		if (ginstr[c].ptr[STBL] == index1)
			count++;
	}
	return count;
}

void CViewGT2Patterns::SweepUnusedSpeedtableEntries()
{
	if (!gt2CommandValueMode)
		return;
	optimizetable(STBL);
}

void CViewGT2Patterns::CommitCommandValueEdit(int chn, int row, int value16)
{
	if (chn < 0 || chn >= MAX_CHN)
		return;
	int pn = epnum[chn];
	if (row < 0 || row >= pattlen[pn])
		return;
	unsigned char *cell = &pattern[pn][row * 4];
	if (cell[0] == ENDPATT)
		return;
	if (!IsValueModeCommand(cell[2]))
		return;  // command is not a value-mode command — nothing to commit
	value16 &= 0xFFFF;
	int oldIdx = cell[3];

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();

	int newIdx;
	if (value16 == 0)
	{
		newIdx = 0;
	}
	else
	{
		int existingPos = FindSpeedtableEntry(value16);
		if (existingPos >= 0)
		{
			newIdx = existingPos + 1;  // dedup: reuse the existing slot
		}
		else if (oldIdx != 0 && CountSpeedtableRefs(oldIdx) == 1)
		{
			// this command is the sole owner — overwrite the slot in place
			ltable[STBL][oldIdx - 1] = (unsigned char)(value16 >> 8);
			rtable[STBL][oldIdx - 1] = (unsigned char)(value16 & 0xFF);
			newIdx = oldIdx;
		}
		else
		{
			int freePos = findfreespeedtable();
			if (freePos < 0)
			{
				LOGD("GT2 Command Value Mode: speed table full");
				return;  // refuse — no free slot
			}
			ltable[STBL][freePos] = (unsigned char)(value16 >> 8);
			rtable[STBL][freePos] = (unsigned char)(value16 & 0xFF);
			newIdx = freePos + 1;
		}
	}
	cell[3] = (unsigned char)newIdx;
	CommitPatternUndoSnapshotIfChanged(before);  // sweeps orphaned entries
}

void CViewGT2Patterns::ChangeCommandNibble(int chn, int row, unsigned char newNibble)
{
	if (chn < 0 || chn >= MAX_CHN)
		return;
	int pn = epnum[chn];
	if (row < 0 || row >= pattlen[pn])
		return;
	unsigned char *cell = &pattern[pn][row * 4];
	if (cell[0] == ENDPATT)
		return;
	newNibble &= 0x0F;
	unsigned char oldNibble = (unsigned char)(cell[2] & 0x0F);

	PatternUndoSnapshot before = CapturePatternUndoSnapshot();

	cell[2] = newNibble;
	if (newNibble == 0)
		cell[3] = 0;  // stock GT2: clearing the command clears its data

	if (gt2CommandValueMode
		&& IsValueModeCommand(oldNibble) != IsValueModeCommand(newNibble))
	{
		// crossing the speed/non-speed boundary resets the argument so the
		// new command never inherits a meaningless byte
		cell[3] = 0;
	}

	CommitPatternUndoSnapshotIfChanged(before);  // sweep frees any orphan
}

bool CViewGT2Patterns::CursorRowCommandIsValueMode() const
{
	if (epchn < 0 || epchn >= MAX_CHN)
		return false;
	int pn = epnum[epchn];
	if (eppos < 0 || eppos >= pattlen[pn])
		return false;
	if (pattern[pn][eppos * 4] == ENDPATT)
		return false;
	return IsValueModeCommand(pattern[pn][eppos * 4 + 2]);
}

bool CViewGT2Patterns::CursorRowCommandUsesValueField() const
{
	// In value mode the 4-digit value field is used by speed commands and by
	// empty cells (no command) — so the cursor digit stays consistent while
	// navigating vertically across blank rows.
	if (epchn < 0 || epchn >= MAX_CHN)
		return false;
	int pn = epnum[epchn];
	if (eppos < 0 || eppos >= pattlen[pn])
		return false;
	if (pattern[pn][eppos * 4] == ENDPATT)
		return false;
	int cmd = pattern[pn][eppos * 4 + 2] & 0x0F;
	return IsValueModeCommand(cmd) || cmd == 0;
}

static int GT2_HexFromKey(u32 k)
{
	if (k >= '0' && k <= '9') return (int)(k - '0');
	if (k >= 'a' && k <= 'f') return (int)(k - 'a' + 10);
	if (k >= 'A' && k <= 'F') return (int)(k - 'A' + 10);
	return -1;
}

void CViewGT2Patterns::BeginCommandValueEdit()
{
	commandValueEditing = true;
	commandValueEditChn = epchn;
	commandValueEditRow = eppos;
	int pn = epnum[epchn];
	commandValueEditValue = GetSpeedtableValue(pattern[pn][eppos * 4 + 3]);
}

void CViewGT2Patterns::CommitPendingCommandValueEdit()
{
	if (!commandValueEditing)
		return;
	int chn = commandValueEditChn;
	int row = commandValueEditRow;
	int value = commandValueEditValue;
	commandValueEditing = false;
	CommitCommandValueEdit(chn, row, value);
}

void CViewGT2Patterns::CancelPendingCommandValueEdit()
{
	commandValueEditing = false;
}

void CViewGT2Patterns::AdvanceCommandValueCursor()
{
	// Advance the cursor after an entry, exactly like GT2 note entry: by the
	// edit step in the Renoise layout, by one row otherwise.
	if (keypreset == KEY_RENOISE)
	{
		gt2advanceeditstep();
	}
	else if (autoadvance < 2)
	{
		eppos++;
		if (eppos > pattlen[epnum[epchn]])
			eppos = 0;
	}
}

void CViewGT2Patterns::UpdateCommandValueEditLifecycle()
{
	if (!commandValueEditing)
		return;
	bool stillValid = gt2CommandValueMode
		&& recordmode
		&& editmode == EDIT_PATTERN && !eamode
		&& epchn == commandValueEditChn
		&& eppos == commandValueEditRow
		&& eparpcol < 0
		&& epcolumn >= 4
		&& CursorRowCommandIsValueMode();
	if (!stillValid)
		CommitPendingCommandValueEdit();
}

bool CViewGT2Patterns::HandleSustainColumnKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Only relevant when the cursor sits on the sustain column.
	if (!epInSustain || !gt2VisibleSustainColumn) return false;
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (isAlt || isControl || isSuper) return false;
	if (epchn < 0 || epchn >= MAX_CHN) return false;
	int pattNum = epnum[epchn];
	if (pattNum < 0 || pattNum >= MAX_PATT) return false;

	// DELETE: clear the row's CMD_SETSR + data (the only fields the sustain
	// column shadows). Native GT2's plain-DELETE handler would otherwise see
	// epcolumn=2 (parked at instr-lo while in sustain) and wipe the
	// instrument byte instead. Advance by edit step in recordmode, matching
	// every other recordmode write/delete.
	if (keyCode == MTKEY_DELETE)
	{
		if (!recordmode) return true;
		if (eppos < 0 || eppos >= pattlen[pattNum]) return true;
		BeginPatternUndoStep();
		pattern[pattNum][eppos * 4 + 2] = 0;
		pattern[pattNum][eppos * 4 + 3] = 0;
		CommitPatternUndoStep();
		gt2advanceeditstep();
		return true;
	}

	// Hex digit (0..9 / a..f / A..F): write CMD_SETSR with that high nibble
	// and the cursor row's effective instrument's release nibble. Anything
	// else falls through so the existing handlers (HandleEditStepShortcut
	// for backquote, HandleArpKey for arrows, Tab/Enter/Esc/Space, …) keep
	// working while the cursor sits on the sustain column.
	int hex = -1;
	if      (keyCode >= '0' && keyCode <= '9') hex = (int)(keyCode - '0');
	else if (keyCode >= 'a' && keyCode <= 'f') hex = 10 + (int)(keyCode - 'a');
	else if (keyCode >= 'A' && keyCode <= 'F') hex = 10 + (int)(keyCode - 'A');
	if (hex < 0) return false;
	if (!recordmode) return true;

	// Look up the row's effective instrument (carry-forward in the same
	// pattern when the row itself has no instrument byte).
	unsigned char instr = pattern[pattNum][eppos * 4 + 1];
	if (instr == 0)
	{
		for (int r = eppos - 1; r >= 0; r--)
		{
			unsigned char ii = pattern[pattNum][r * 4 + 1];
			if (ii != 0) { instr = ii; break; }
		}
	}
	int release = (instr > 0 && instr < MAX_INSTR) ? (ginstr[instr].sr & 0x0F) : 0;

	BeginPatternUndoStep();
	pattern[pattNum][eppos * 4 + 2] = CMD_SETSR;
	pattern[pattNum][eppos * 4 + 3] = (unsigned char)((hex << 4) | (release & 0x0F));
	CommitPatternUndoStep();
	gt2advanceeditstep();
	return true;
}

bool CViewGT2Patterns::HandleCommandValueKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (!gt2CommandValueMode)
		return false;

	bool ctrlLike = isControl || isSuper;

	// Undo / redo: cancel the in-progress edit, then let the undo run.
	if (ctrlLike && !isAlt
		&& (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z
			|| keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y))
	{
		CancelPendingCommandValueEdit();
		return false;
	}

	// Any other modified key: commit first, then let it proceed normally.
	if (ctrlLike || isAlt)
	{
		CommitPendingCommandValueEdit();
		return false;
	}

	if (editmode != EDIT_PATTERN || eamode)
	{
		CommitPendingCommandValueEdit();
		return false;
	}

	auto moveCommandCursorToNextChannel = [this]() {
		CommitPendingCommandValueEdit();
		epchn++;
		if (epchn >= MAX_CHN) epchn = 0;
		eparpcol = -1;
		epcolumn = 0;
		commandValueDigit = 0;
		if (eppos > pattlen[epnum[epchn]])
			eppos = pattlen[epnum[epchn]];
	};

	// Command nibble entry (epcolumn 3) — route through ChangeCommandNibble so
	// speed/non-speed transitions and orphan cleanup are handled, then advance
	// vertically by the edit step like note entry.
	if (eparpcol < 0 && epcolumn == 3 && recordmode && !isShift)
	{
		int hex = GT2_HexFromKey(keyCode);
		if (hex >= 0)
		{
			int pn = epnum[epchn];
			if (eppos >= 0 && eppos < pattlen[pn]
				&& pattern[pn][eppos * 4] != ENDPATT)
			{
				CommitPendingCommandValueEdit();
				ChangeCommandNibble(epchn, eppos, (unsigned char)hex);
				AdvanceCommandValueCursor();
				return true;
			}
		}
	}

	// Right from the command nibble enters either the 4-digit value field or the
	// first raw argument digit immediately, without queueing a stock GT2 event.
	if (eparpcol < 0 && epcolumn == 3 && keyCode == MTKEY_ARROW_RIGHT && !isShift)
	{
		if (epchn >= 0 && epchn < MAX_CHN)
		{
			int pn = epnum[epchn];
			if (eppos >= 0 && eppos < pattlen[pn]
				&& pattern[pn][eppos * 4] != ENDPATT)
			{
				epcolumn = 4;
				commandValueDigit = 0;
				return true;
			}
		}
	}

	// 1-byte command argument (epcolumn 4 = hi nibble, 5 = lo nibble).
	// Each typed digit is committed immediately and advances vertically by the
	// edit step, matching tracker note-entry flow.
	if (eparpcol < 0 && (epcolumn == 4 || epcolumn == 5) && recordmode && !isShift)
	{
		int pn = epnum[epchn];
		if (eppos >= 0 && eppos < pattlen[pn]
			&& pattern[pn][eppos * 4] != ENDPATT)
		{
			int cmd = pattern[pn][eppos * 4 + 2] & 0x0F;
			int hexv = GT2_HexFromKey(keyCode);
			if (hexv >= 0 && cmd != 0 && !IsValueModeCommand(cmd))
			{
				CommitPendingCommandValueEdit();
				PatternUndoSnapshot before = CapturePatternUndoSnapshot();
				unsigned char *cell = &pattern[pn][eppos * 4];
				if (epcolumn == 4)
					cell[3] = (unsigned char)((cell[3] & 0x0F) | (hexv << 4));
				else
					cell[3] = (unsigned char)((cell[3] & 0xF0) | hexv);
				CommitPatternUndoSnapshotIfChanged(before);
				AdvanceCommandValueCursor();
				return true;
			}
		}
	}

	// 1-byte command argument navigation. Keep this local to the ImGui pattern
	// view so arrows update the visible cursor immediately.
	if (eparpcol < 0 && (epcolumn == 4 || epcolumn == 5)
		&& (keyCode == MTKEY_ARROW_LEFT || keyCode == MTKEY_ARROW_RIGHT) && !isShift)
	{
		int pn = epnum[epchn];
		if (eppos >= 0 && eppos < pattlen[pn]
			&& pattern[pn][eppos * 4] != ENDPATT)
		{
			int cmd = pattern[pn][eppos * 4 + 2] & 0x0F;
			if (cmd != 0 && !IsValueModeCommand(cmd))
			{
				CommitPendingCommandValueEdit();
				if (keyCode == MTKEY_ARROW_LEFT)
				{
					epcolumn = epcolumn == 5 ? 4 : 3;
				}
				else if (epcolumn == 4)
				{
					epcolumn = 5;
				}
				else
				{
					moveCommandCursorToNextChannel();
				}
				return true;
			}
		}
	}

	// Cursor in the command value area of a value-field row (a speed command
	// or an empty cell). Empty cells own hex keys as no-op edits so stock GT2
	// autoadvance cannot move the cursor differently than Renoise edit step.
	bool onValueField = eparpcol < 0 && epcolumn >= 4
		&& CursorRowCommandUsesValueField();
	if (!onValueField)
	{
		CommitPendingCommandValueEdit();
		return false;
	}

	int hex = GT2_HexFromKey(keyCode);
	if (hex >= 0 && recordmode && !isShift)
	{
		if (CursorRowCommandIsValueMode())
		{
			if (commandValueEditing
				&& (commandValueEditChn != epchn || commandValueEditRow != eppos))
				CommitPendingCommandValueEdit();
			if (!commandValueEditing)
				BeginCommandValueEdit();
			int sh = (3 - commandValueDigit) * 4;
			commandValueEditValue = (commandValueEditValue & ~(0xF << sh)) | (hex << sh);
			// commit immediately — the value is persisted after every digit
			CommitCommandValueEdit(commandValueEditChn, commandValueEditRow,
							   commandValueEditValue);
			commandValueEditing = false;
		}
		else
		{
			CommitPendingCommandValueEdit();
		}
		AdvanceCommandValueCursor();
		return true;
	}

	if (keyCode == MTKEY_ARROW_LEFT && !isShift)
	{
		if (commandValueDigit > 0)
		{
			commandValueDigit--;
		}
		else
		{
			CommitPendingCommandValueEdit();
			epcolumn = 3;   // move to the command nibble
		}
		return true;
	}
	if (keyCode == MTKEY_ARROW_RIGHT && !isShift)
	{
		if (commandValueDigit < 3)
		{
			commandValueDigit++;
		}
		else
		{
			CommitPendingCommandValueEdit();
			epchn++;
			if (epchn >= MAX_CHN) epchn = 0;
			eparpcol = -1;
			epcolumn = 0;
			if (eppos > pattlen[epnum[epchn]])
				eppos = pattlen[epnum[epchn]];
		}
		return true;
	}
	if (keyCode == MTKEY_ENTER)
	{
		CommitPendingCommandValueEdit();
		return true;
	}
	if (keyCode == MTKEY_ESC)
	{
		CancelPendingCommandValueEdit();
		return true;
	}

	// Any other key (Up/Down/Tab/...): commit, then let it proceed.
	CommitPendingCommandValueEdit();
	return false;
}

// GT2 pattern command / effect set. Index == command nibble.
struct GT2CommandInfo
{
	const char *name;   // command name
	const char *brief;  // concise hint shown in the menu (what it does + arg)
	const char *help;   // full tooltip description
	const char *valueBrief;  // Command Value Mode override for commands 1..4
	const char *valueHelp;
};

static const GT2CommandInfo kGT2Commands[16] =
{
	{ "No command",             "clear this row's command",
	  "Removes any command from this row (command nibble set to 0).",
	  nullptr, nullptr },
	{ "Portamento Up",          "slide pitch up - XX = speedtable idx",
	  "Slides the note pitch upward. XX is a speed-table entry holding the slide speed.",
	  "slide pitch up - XXXX = speed value",
	  "Slides the note pitch upward. XXXX is the 16-bit speed-table value edited directly." },
	{ "Portamento Down",        "slide pitch down - XX = speedtable idx",
	  "Slides the note pitch downward. XX is a speed-table entry holding the slide speed.",
	  "slide pitch down - XXXX = speed value",
	  "Slides the note pitch downward. XXXX is the 16-bit speed-table value edited directly." },
	{ "Tone Portamento",        "glide to note - XX = speedtable idx",
	  "Glides from the previous note to the new one. XX is a speed-table entry; 00 reuses the last 1XX/2XX speed.",
	  "glide to note - XXXX = speed value",
	  "Glides from the previous note to the new one. XXXX is the 16-bit speed-table value edited directly; 0000 leaves no speed entry." },
	{ "Vibrato",                "pitch vibrato - XX = speedtable idx",
	  "Pitch vibrato. XX is a speed-table entry: left side = frequency, right side = amplitude.",
	  "pitch vibrato - XXXX = speed value",
	  "Pitch vibrato. XXXX is the 16-bit speed-table value edited directly: high byte = frequency, low byte = amplitude." },
	{ "Set Attack/Decay",       "ADSR attack/decay - XX = AD value",
	  "Writes the SID attack/decay register. XX is the raw AD byte (attack high nibble, decay low nibble).",
	  nullptr, nullptr },
	{ "Set Sustain/Release",    "ADSR sustain/release - XX = SR value",
	  "Writes the SID sustain/release register. XX is the raw SR byte (sustain high nibble, release low nibble).",
	  nullptr, nullptr },
	{ "Set Waveform",           "set waveform - XX = waveform byte",
	  "Sets the SID waveform / control byte directly. XX is the waveform value (e.g. 41 = pulse + gate).",
	  nullptr, nullptr },
	{ "Set Wavetable Pointer",  "jump wavetable - XX = position",
	  "Jumps the wavetable to position XX. 00 stops the wavetable.",
	  nullptr, nullptr },
	{ "Set Pulsetable Pointer", "jump pulsetable - XX = position",
	  "Jumps the pulsetable to position XX. 00 stops the pulsetable.",
	  nullptr, nullptr },
	{ "Set Filtertable Pointer","jump filtertable - XX = position",
	  "Jumps the filtertable to position XX. 00 stops the filtertable.",
	  nullptr, nullptr },
	{ "Set Filter Control",     "filter routing - XX = control byte",
	  "Sets filter routing. Low nibble = channels filtered, high nibble = passband. 00 turns the filter off.",
	  nullptr, nullptr },
	{ "Set Filter Cutoff",      "filter cutoff - XX = cutoff value",
	  "Sets the filter cutoff frequency. XX is an 8-bit cutoff value.",
	  nullptr, nullptr },
	{ "Set Master Volume",      "master volume - XX = volume",
	  "Sets the SID master volume. XX is the volume; the high nibble also drives the filter passband.",
	  nullptr, nullptr },
	{ "Funktempo",              "shuffle tempo - XX = speedtable idx",
	  "Shuffle / swing tempo. XX is a speed-table entry holding two tempo values that alternate every row.",
	  nullptr, nullptr },
	{ "Set Tempo",              "song tempo - XX = tempo",
	  "Sets the tempo. 03-7F sets all channels, 83-FF only the current channel (value minus 80). 00-01 recall the funktempo values.",
	  nullptr, nullptr },
};

const char *CViewGT2Patterns::GetCommandMenuBrief(int cmdNibble) const
{
	if (cmdNibble < 0 || cmdNibble >= 16)
		return "";
	const GT2CommandInfo &info = kGT2Commands[cmdNibble];
	if (gt2CommandValueMode && IsValueModeCommand(cmdNibble) && info.valueBrief)
		return info.valueBrief;
	return info.brief;
}

const char *CViewGT2Patterns::GetCommandMenuHelp(int cmdNibble) const
{
	if (cmdNibble < 0 || cmdNibble >= 16)
		return "";
	const GT2CommandInfo &info = kGT2Commands[cmdNibble];
	if (gt2CommandValueMode && IsValueModeCommand(cmdNibble) && info.valueHelp)
		return info.valueHelp;
	return info.help;
}

ImVec4 CViewGT2Patterns::GetCommandMenuDescriptionColor(bool hovered) const
{
	return ImGui::GetStyleColorVec4(hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
}

bool CViewGT2Patterns::CommandMenuDescriptionUsesNativeShortcut() const
{
	return false;
}

static bool GT2_CommandMenuItemWithDescription(const char *label,
	const char *description, bool selected, bool enabled, bool *outHovered)
{
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
	{
		if (outHovered) *outHovered = false;
		return false;
	}

	// Command descriptions need ImGui's menu-column placement, but ImGui::MenuItem
	// always draws its shortcut text as disabled gray. Render the row locally so
	// the description can switch color after Selectable() has resolved hover.
	if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
	{
		bool pressed = ImGui::MenuItem(label, NULL, selected, enabled);
		if (outHovered) *outHovered = ImGui::IsItemHovered();
		return pressed;
	}

	ImGuiContext &g = *GImGui;
	ImGuiStyle &style = g.Style;
	ImVec2 pos = window->DC.CursorPos;
	ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);

	bool pressed = false;
	ImGui::PushID(label);
	if (!enabled)
		ImGui::BeginDisabled();

	const ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SelectOnRelease
		| ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SetNavIdOnHover;
	const ImGuiMenuColumns *offsets = &window->DC.MenuColumns;
	float descriptionW = (description && description[0])
		? ImGui::CalcTextSize(description, NULL).x : 0.0f;
	float checkmarkW = IM_TRUNC(g.FontSize * 1.20f);
	float minW = window->DC.MenuColumns.DeclColumns(0.0f, labelSize.x,
		descriptionW, checkmarkW);
	float stretchW = ImMax(0.0f, ImGui::GetContentRegionAvail().x - minW);
	ImVec2 textPos(pos.x, pos.y + window->DC.CurrLineTextBaseOffset);

	pressed = ImGui::Selectable("", false,
		selectableFlags | ImGuiSelectableFlags_SpanAvailWidth,
		ImVec2(minW, labelSize.y));
	bool hovered = ImGui::IsItemHovered();
	if (outHovered) *outHovered = hovered;

	if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)
	{
		ImGui::RenderText(textPos + ImVec2(offsets->OffsetLabel, 0.0f), label);
		if (descriptionW > 0.0f)
		{
			ImGui::PushStyleColor(ImGuiCol_Text,
				ImGui::GetStyleColorVec4(hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled));
			ImGui::LogSetNextTextDecoration("(", ")");
			ImGui::RenderText(textPos + ImVec2(offsets->OffsetShortcut + stretchW, 0.0f),
				description, NULL, false);
			ImGui::PopStyleColor();
		}
		if (selected)
		{
			ImGui::RenderCheckMark(window->DrawList,
				textPos + ImVec2(offsets->OffsetMark + stretchW + g.FontSize * 0.40f,
					g.FontSize * 0.134f * 0.5f),
				ImGui::GetColorU32(ImGuiCol_Text), g.FontSize * 0.866f);
		}
	}

	IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label,
		g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable
		| (selected ? ImGuiItemStatusFlags_Checked : 0));
	if (!enabled)
		ImGui::EndDisabled();
	ImGui::PopID();

	return pressed;
}

// ADSR rate-to-time references — used by the AD / SR argument editor so
// the user can see what each nibble actually does. Copied here so this
// translation unit doesn't depend on the instrument-view file.
static const char *GT2_PatCmd_AttackTime[16] =
{
	"2 ms", "8 ms", "16 ms", "24 ms", "38 ms", "56 ms", "68 ms", "80 ms",
	"100 ms", "250 ms", "500 ms", "800 ms", "1 s", "3 s", "5 s", "8 s",
};
static const char *GT2_PatCmd_DecayRelTime[16] =
{
	"6 ms", "24 ms", "48 ms", "72 ms", "114 ms", "168 ms", "204 ms", "240 ms",
	"300 ms", "750 ms", "1.5 s", "2.4 s", "3 s", "9 s", "15 s", "24 s",
};

// Filter-control passband table for command B. The high nibble of the
// data byte selects bits 4..7 of SID $D418 (LP / BP / HP / 3OFF).
static const char *GT2_PatCmd_FilterPassbands[8] =
{
	"$80 off",      "$90 lowpass",  "$A0 bandpass", "$B0 LP+BP",
	"$C0 highpass", "$D0 HP+LP",    "$E0 HP+BP",    "$F0 LP+BP+HP",
};

// Hex-nibble picker: 16 small buttons in a row, the current value chip
// is highlighted in the context-menu accent colour. Returns true and
// fills *out when the user clicks a new value.
static bool RenderHexNibblePicker(const char *id, int current, int *out)
{
	ImVec4 sel  = ImVec4(0.30f, 1.00f, 0.30f, 0.80f);
	ImVec4 hov  = ImVec4(0.30f, 1.00f, 0.30f, 0.95f);
	ImVec4 act  = ImVec4(0.30f, 1.00f, 0.30f, 1.00f);
	ImVec4 selFg(0.05f, 0.05f, 0.05f, 1.00f);
	bool changed = false;
	for (int i = 0; i < 16; i++)
	{
		if (i > 0) ImGui::SameLine(0.0f, 2.0f);
		char b[16]; snprintf(b, sizeof(b), "%X##%s_%d", i, id, i);
		bool isSel = (i == current);
		if (isSel)
		{
			ImGui::PushStyleColor(ImGuiCol_Button,        sel);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  act);
			ImGui::PushStyleColor(ImGuiCol_Text,          selFg);
		}
		if (ImGui::SmallButton(b))
		{
			if (out) *out = i;
			changed = true;
		}
		if (isSel) ImGui::PopStyleColor(4);
	}
	return changed;
}

// Writes both the command nibble and the data byte atomically (one undo
// step). Caller has already confirmed the cursor row is editable.
static void GT2_WriteCommandAndData(CViewGT2Patterns *view,
	int channel, int row, unsigned char cmdNib, unsigned char dataByte)
{
	int pn = epnum[channel];
	if (pn < 0 || pn >= MAX_PATT) return;
	if (row < 0 || row >= pattlen[pn]) return;
	if (pattern[pn][row * 4] == ENDPATT) return;
	view->BeginPatternUndoStep();
	pattern[pn][row * 4 + 2] = (unsigned char)(cmdNib & 0x0F);
	pattern[pn][row * 4 + 3] = dataByte;
	view->CommitPatternUndoStep();
}

void CViewGT2Patterns::RenderPatternCommandEditor()
{
	int pn = epnum[epchn];
	bool rowEditable = (eppos >= 0 && eppos < pattlen[pn]
						&& pattern[pn][eppos * 4] != ENDPATT);
	unsigned char curCmd  = rowEditable ? (pattern[pn][eppos * 4 + 2] & 0x0F) : 0;
	unsigned char curData = rowEditable ? pattern[pn][eppos * 4 + 3] : 0;
	const GT2CommandInfo &info = kGT2Commands[curCmd];

	ImGui::TextUnformatted("Edit Effect");
	ImGui::Separator();
	if (!rowEditable)
	{
		ImGui::TextDisabled("Row not editable.");
		return;
	}
	ImGui::Text("Current: %X%02X   %s", (int)curCmd, (int)curData, info.name);
	ImGui::TextDisabled("%s", GetCommandMenuHelp((int)curCmd));
	ImGui::Spacing();

	// --- Command picker (4 columns × 4 rows of clickable chips) ---
	// Short labels — the full GT2CommandInfo.name is still in the
	// hover tooltip so the user can read the full title when the chip
	// is too narrow to spell it out. Labels start with the hex digit so
	// that the left-aligned text lines them up in a regular grid.
	static const char *kShortNames[16] =
	{
		"0  None",        "1  Porta Up",    "2  Porta Dn",    "3  Tone Porta",
		"4  Vibrato",     "5  AD",          "6  SR",          "7  Waveform",
		"8  WTbl Ptr",    "9  PTbl Ptr",    "A  FTbl Ptr",    "B  Filter Ctrl",
		"C  Cutoff",      "D  Mst Vol",     "E  Funktempo",   "F  Tempo",
	};
	ImGui::TextUnformatted("Command:");
	ImVec4 selBg (0.30f, 1.00f, 0.30f, 0.80f);
	ImVec4 selHov(0.30f, 1.00f, 0.30f, 0.95f);
	ImVec4 selAct(0.30f, 1.00f, 0.30f, 1.00f);
	ImVec4 selFg (0.05f, 0.05f, 0.05f, 1.00f);
	// Left-align button text so the leading hex digit lines up across
	// every chip in the grid — centred labels drifted around because
	// the strings differ in length and most got clipped.
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
	if (ImGui::BeginTable("##gt2pcmdpicker", 4,
		ImGuiTableFlags_SizingStretchSame))
	{
		for (int i = 0; i < 16; i++)
		{
			ImGui::TableNextColumn();
			char lab[40];
			snprintf(lab, sizeof(lab), "%s##pcmd%d", kShortNames[i], i);
			bool sel = (i == (int)curCmd);
			if (sel)
			{
				ImGui::PushStyleColor(ImGuiCol_Button,        selBg);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selHov);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  selAct);
				ImGui::PushStyleColor(ImGuiCol_Text,          selFg);
			}
			ImVec2 sz(-FLT_MIN, 0);
			if (ImGui::Button(lab, sz))
			{
				curCmd = (unsigned char)i;
				// Clear the data byte when the command changes to a
				// different kind, otherwise the previous numeric stays
				// confusing for the new command's argument semantics.
				if ((int)curData != 0 && i == 0)
					curData = 0;
				GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
			}
			if (sel) ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%X  %s\n%s",
					i, kGT2Commands[i].name, kGT2Commands[i].help);
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();

	ImGui::Spacing();
	ImGui::TextUnformatted("Argument:");

	// --- Argument editor (per command) ---
	if (curCmd == 0)
	{
		ImGui::TextDisabled("No argument for 'No command'.");
	}
	else if (curCmd == 5 || curCmd == 6)
	{
		// AD or SR — two 0..F nibble pickers, with timing hint.
		int hi = (curData >> 4) & 0x0F;
		int lo =  curData       & 0x0F;
		bool isAD = (curCmd == 5);
		ImGui::TextUnformatted(isAD ? "Attack:" : "Sustain:");
		int newHi = hi;
		if (RenderHexNibblePicker(isAD ? "pAtk" : "pSus", hi, &newHi))
		{
			curData = (unsigned char)((newHi << 4) | lo);
			GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
		}
		ImGui::TextDisabled("  (%s)", isAD ? GT2_PatCmd_AttackTime[hi]
		                                   : "level (0=silent .. F=loudest)");
		ImGui::TextUnformatted(isAD ? "Decay:" : "Release:");
		int newLo = lo;
		if (RenderHexNibblePicker(isAD ? "pDec" : "pRel", lo, &newLo))
		{
			curData = (unsigned char)((hi << 4) | newLo);
			GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
		}
		ImGui::TextDisabled("  (%s)", GT2_PatCmd_DecayRelTime[lo]);
	}
	else if (curCmd == 7)
	{
		// Set Waveform — high nibble = waveform mix, low nibble = control bits.
		int hi = (curData >> 4) & 0x0F;
		int lo =  curData       & 0x0F;
		ImGui::TextUnformatted("Waveform mix (high nibble):");
		int newHi = hi;
		if (RenderHexNibblePicker("pWf", hi, &newHi))
		{
			curData = (unsigned char)((newHi << 4) | lo);
			GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
		}
		ImGui::TextDisabled("  1=tri 2=saw 4=pulse 8=noise (mix bits)");
		ImGui::Spacing();
		ImGui::TextUnformatted("Control bits (low nibble):");
		bool gate = (lo & 0x01) != 0;
		bool sync = (lo & 0x02) != 0;
		bool ring = (lo & 0x04) != 0;
		bool test = (lo & 0x08) != 0;
		bool cBitsChanged = false;
		if (ImGui::Checkbox("gate##pwc_g", &gate)) cBitsChanged = true;
		ImGui::SameLine();
		if (ImGui::Checkbox("sync##pwc_s", &sync)) cBitsChanged = true;
		ImGui::SameLine();
		if (ImGui::Checkbox("ring##pwc_r", &ring)) cBitsChanged = true;
		ImGui::SameLine();
		if (ImGui::Checkbox("test##pwc_t", &test)) cBitsChanged = true;
		if (cBitsChanged)
		{
			int newLo = (gate ? 0x01 : 0) | (sync ? 0x02 : 0)
			          | (ring ? 0x04 : 0) | (test ? 0x08 : 0);
			curData = (unsigned char)((hi << 4) | newLo);
			GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
		}
	}
	else if (curCmd == 11)   // B — Set Filter Control
	{
		// Same passband + resonance + routing layout used in the filter
		// table's context editor, applied directly to the data byte.
		int hi = (curData >> 4) & 0x0F;
		int lo =  curData       & 0x0F;
		int passband = -1;
		if (hi >= 0x8) passband = hi - 0x8;     // 0..7 maps onto $80..$F0

		ImGui::TextUnformatted("Passband (high nibble):");
		if (ImGui::BeginTable("##gt2pcmdfp", 2,
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
		{
			for (int i = 0; i < 8; i++)
			{
				ImGui::TableNextColumn();
				bool sel = (i == passband);
				if (sel)
				{
					ImGui::PushStyleColor(ImGuiCol_Button,        selBg);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selHov);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  selAct);
					ImGui::PushStyleColor(ImGuiCol_Text,          selFg);
				}
				char lab[24]; snprintf(lab, sizeof(lab), "%s##pfp%d",
					GT2_PatCmd_FilterPassbands[i], i);
				if (ImGui::SmallButton(lab))
				{
					int newHi = 0x8 + i;
					curData = (unsigned char)((newHi << 4) | lo);
					GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
				}
				if (sel) ImGui::PopStyleColor(4);
			}
			ImGui::EndTable();
		}
		int resonance = (lo >> 0) & 0x0F;        // routing is bits 0..2,
		// resonance is the high nibble of the lower-half. The standard
		// GT2 routing byte layout is: high nibble = resonance, low
		// nibble = channel bitmask. But the filter-control argument
		// here uses the SAME byte as command BXY (resonance in the high
		// nibble of the low half? Let's clarify: the data byte's whole
		// low nibble (4 bits) carries the channel routing in bits 0..2,
		// and the high nibble of the data byte carries the passband.
		// Native GT2 doesn't pack a separate resonance nibble into this
		// command; the resonance comes from $D417 elsewhere. Skip the
		// resonance row to avoid presenting bogus controls.
		(void)resonance;
		ImGui::Spacing();
		ImGui::TextUnformatted("Routing (low nibble — channels to filter):");
		bool ch1 = (lo & 0x01) != 0;
		bool ch2 = (lo & 0x02) != 0;
		bool ch3 = (lo & 0x04) != 0;
		bool rChanged = false;
		if (ImGui::Checkbox("Ch 1##pfc1", &ch1)) rChanged = true;
		ImGui::SameLine();
		if (ImGui::Checkbox("Ch 2##pfc2", &ch2)) rChanged = true;
		ImGui::SameLine();
		if (ImGui::Checkbox("Ch 3##pfc3", &ch3)) rChanged = true;
		if (rChanged)
		{
			int newLo = (ch1 ? 0x01 : 0) | (ch2 ? 0x02 : 0) | (ch3 ? 0x04 : 0);
			curData = (unsigned char)((hi << 4) | newLo);
			GT2_WriteCommandAndData(this, epchn, eppos, curCmd, curData);
		}
	}
	else
	{
		// Commands without a dedicated semantic editor — show the
		// current data byte read-only and describe what the byte means.
		// (The argument is still editable in the pattern itself; the
		// in-context byte nudger turned out to be more noise than help.)
		ImGui::TextDisabled("Data byte: 0x%02X", (int)curData);
		const char *hint = nullptr;
		switch (curCmd)
		{
		case 1: case 2: case 3: case 4:
			hint = "Speed-table entry index (00 = none / reuse the last "
			       "1XX/2XX speed). Turn on 'Command Value Mode' from the "
			       "GT2 settings menu to edit the 16-bit speed value "
			       "directly on the row instead of going through the "
			       "speed-table index.";
			break;
		case 8: case 9: case 10:
			hint = "Table position (1-based). 00 = stop / no jump.";
			break;
		case 12:
			hint = "8-bit filter cutoff value.";
			break;
		case 13:
			hint = "Master volume. The high nibble of the byte also "
			       "controls the filter passband bits.";
			break;
		case 14:
			hint = "Speed-table entry that holds two alternating tempo "
			       "values (left = first row, right = second row).";
			break;
		case 15:
			hint = "03-7F sets tempo on all channels; 83-FF only the "
			       "current channel (value - 80). 00-01 recall the two "
			       "funktempo values.";
			break;
		default: break;
		}
		if (hint)
			ImGui::TextDisabled("%s", hint);
	}
}

void CViewGT2Patterns::RenderContextMenu()
{
	if (!ImGui::BeginPopup("GT2PatternsContext"))
		return;

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		ImGui::CloseCurrentPopup();

	// The Edit Effect block that used to open this menu now lives in its own
	// view -- GoatTracker -> GT2 Pattern Row -- which renders this exact call
	// and follows the cursor instead of a click target. Two copies of it on
	// screen is one too many, so the menu keeps only the actions below.
	//
	// Deliberately kept, not deleted: this is the other half of the shared-
	// renderer arrangement described in the pattern-row notes,
	// and uncommenting the block is how the menu gets it back.
	//
	//   if (contextMenuOnCommand)
	//   {
	//       RenderPatternCommandEditor();
	//       ImGui::Separator();
	//   }

	bool hasSelection = patternSelectionActive;
	bool canPasteSelection = !patternClipboard.empty() && patternClipboardWidth > 0 && patternClipboardHeight > 0;
	bool canPasteTrack = !trackClipboard.empty();
	bool canPastePattern = !phraseClipboard.empty();
	bool canPasteEffects = !effectClipboard.empty();
	const char *cmd = "Ctrl";
#if defined(MACOS) || defined(__APPLE__)
	cmd = "Cmd";
#endif
	char shortcut[32];

	if (ImGui::BeginMenu("Selection"))
	{
		if (ImGui::MenuItem("Cut", "Alt+F3", false, hasSelection))
			CutPatternSelection();
		if (ImGui::MenuItem("Copy", "Alt+F4", false, hasSelection))
			CopyPatternSelection();
		if (ImGui::MenuItem("Paste", "Alt+F5", false, canPasteSelection))
			PasteAtCursor();
		if (ImGui::MenuItem("Transpose Notes Down 1", "Alt+F1", false, hasSelection))
			TransposePatternSelection(-1);
		if (ImGui::MenuItem("Transpose Notes Up 1", "Alt+F2", false, hasSelection))
			TransposePatternSelection(1);
		if (ImGui::MenuItem("Transpose Notes Down 12", "Alt+F11", false, hasSelection))
			TransposePatternSelection(-12);
		if (ImGui::MenuItem("Transpose Notes Up 12", "Alt+F12", false, hasSelection))
			TransposePatternSelection(12);
		if (ImGui::MenuItem("Shrink", "Alt+F8", false, hasSelection))
			ShrinkPatternSelection();
		if (ImGui::MenuItem("Expand", "Alt+F9", false, hasSelection))
			ExpandPatternSelection();
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Track"))
	{
		if (ImGui::MenuItem("Cut", "Shift+F3"))
			CutTrack();
		if (ImGui::MenuItem("Copy", "Shift+F4"))
			CopyTrack();
		if (ImGui::MenuItem("Paste", "Shift+F5", false, canPasteTrack))
			PasteTrack();
		if (ImGui::MenuItem("Transpose Notes Down 1", "Shift+F1"))
			TransposeTrack(-1);
		if (ImGui::MenuItem("Transpose Notes Up 1", "Shift+F2"))
			TransposeTrack(1);
		if (ImGui::MenuItem("Transpose Notes Down 12", "Shift+F11"))
			TransposeTrack(-12);
		if (ImGui::MenuItem("Transpose Notes Up 12", "Shift+F12"))
			TransposeTrack(12);
		if (ImGui::MenuItem("Shrink", "Shift+F8"))
			ShrinkTrack();
		if (ImGui::MenuItem("Expand", "Shift+F9"))
			ExpandTrack();
		ImGui::EndMenu();
	}

	snprintf(shortcut, sizeof(shortcut), "%s+F3", cmd);
	if (ImGui::BeginMenu("Pattern"))
	{
		if (ImGui::MenuItem("Cut", shortcut))
			CutPhrase();
		snprintf(shortcut, sizeof(shortcut), "%s+F4", cmd);
		if (ImGui::MenuItem("Copy", shortcut))
			CopyPhrase();
		snprintf(shortcut, sizeof(shortcut), "%s+F5", cmd);
		if (ImGui::MenuItem("Paste", shortcut, false, canPastePattern))
			PastePhrase();
		snprintf(shortcut, sizeof(shortcut), "%s+F1", cmd);
		if (ImGui::MenuItem("Transpose Notes Down 1", shortcut))
			TransposePhrase(-1);
		snprintf(shortcut, sizeof(shortcut), "%s+F2", cmd);
		if (ImGui::MenuItem("Transpose Notes Up 1", shortcut))
			TransposePhrase(1);
		snprintf(shortcut, sizeof(shortcut), "%s+F11", cmd);
		if (ImGui::MenuItem("Transpose Notes Down 12", shortcut))
			TransposePhrase(-12);
		snprintf(shortcut, sizeof(shortcut), "%s+F12", cmd);
		if (ImGui::MenuItem("Transpose Notes Up 12", shortcut))
			TransposePhrase(12);
		snprintf(shortcut, sizeof(shortcut), "%s+F8", cmd);
		if (ImGui::MenuItem("Shrink", shortcut))
			ShrinkPhrase();
		snprintf(shortcut, sizeof(shortcut), "%s+F9", cmd);
		if (ImGui::MenuItem("Expand", shortcut))
			ExpandPhrase();
		ImGui::EndMenu();
	}

	ImGui::Separator();
	if (ImGui::MenuItem("Copy Effects", "Shift+E"))
		CopyEffectsAtCursorOrSelection();
	if (ImGui::MenuItem("Paste Effects", "Shift+R", false, canPasteEffects))
		PasteEffectsAtCursor();
	if (ImGui::MenuItem("Make HiFi Vib/Porta Speed", "Shift+H"))
		MakeHiFiVibratoPortaSpeed();
	if (ImGui::MenuItem("Invert Selection/Pattern", "Shift+I"))
		InvertSelectionOrPattern();
	if (ImGui::MenuItem("Join Pattern", "Shift+J"))
		JoinPatternAtCursor();
	if (ImGui::MenuItem("Split Pattern", "Shift+K"))
		SplitPatternAtCursor();
	if (ImGui::MenuItem("Select All Pattern", "Shift+L / Ctrl+A"))
		SelectWholePattern();
	if (ImGui::MenuItem("Edit Step +1", "`", false, keypreset == KEY_RENOISE))
		gt2RenoiseEditStep++;
	if (ImGui::MenuItem("Edit Step -1", "~", false, keypreset == KEY_RENOISE))
		gt2RenoiseEditStep = std::max(0, gt2RenoiseEditStep - 1);
	if (ImGui::MenuItem("Increase Highlight Step", "Shift+M"))
		AdjustHighlightStep(1);
	if (ImGui::MenuItem("Decrease Highlight Step", "Shift+N"))
		AdjustHighlightStep(-1);
	if (ImGui::MenuItem("Shrink Pattern", "Shift+O"))
		ShrinkSelectionOrPattern();
	if (ImGui::MenuItem("Expand Pattern", "Shift+P"))
		ExpandSelectionOrPattern();
	if (ImGui::MenuItem("Cycle Autoadvance", "Shift+Z"))
		CycleAutoadvanceMode();

	ImGui::Separator();
	if (ImGui::MenuItem("Generate Echo..."))
		echoPopupRequested = true;

	ImGui::EndPopup();
}

// Builds one EchoSource per real note that the next Generate Echo will
// cascade from. With an active selection we scan every main-note track of
// the selected tracks for real notes in the selected rows; otherwise the
// cursor cell is the sole source. Instruments missing on the source row
// are resolved by walking back through the same pattern, matching how
// GT2 carries the last-set instrument forward on each channel.
int CViewGT2Patterns::CollectEchoSources(std::vector<EchoSource> *out) const
{
	if (out) out->clear();
	if (!gt2_engine_ready) return 0;

	auto pushSource = [&](int ch, int row) -> bool
	{
		if (ch < 0 || ch >= MAX_CHN) return false;
		int pn = epnum[ch];
		if (pn < 0 || pn >= MAX_PATT) return false;
		int len = pattlen[pn];
		if (row < 0 || row >= len) return false;
		const unsigned char *patt = pattern[pn];
		unsigned char note = patt[row * 4 + 0];
		// KEYOFF acts as its own source kind — the cascade copies KEYOFF
		// (no instrument, no CMD_SETSR) and uses a placeholder sustain so
		// the cascade length matches a max-sustain note, but every clash
		// resolves against it as "lowest sustain" (see GenerateEcho).
		if (note == KEYOFF)
		{
			if (out)
			{
				EchoSource s;
				s.channel  = ch;
				s.row      = row;
				s.note     = KEYOFF;
				s.instr    = 0;
				s.sustain  = 15;     // placeholder cascade length only
				s.release  = 0;
				s.isKeyOff = true;
				out->push_back(s);
			}
			return true;
		}
		if (note < FIRSTNOTE || note > LASTNOTE) return false;
		unsigned char instr = patt[row * 4 + 1];
		if (instr == 0)
		{
			for (int rr = row - 1; rr >= 0; rr--)
			{
				unsigned char ii = patt[rr * 4 + 1];
				if (ii != 0) { instr = ii; break; }
			}
		}
		if (instr == 0 || instr >= MAX_INSTR) return false;
		int sustain = (ginstr[instr].sr >> 4) & 0x0F;
		if (sustain <= 0) return false;
		if (out)
		{
			EchoSource s;
			s.channel  = ch;
			s.row      = row;
			s.note     = note;
			s.instr    = instr;
			s.sustain  = sustain;
			s.release  = ginstr[instr].sr & 0x0F;
			s.isKeyOff = false;
			out->push_back(s);
		}
		return true;
	};

	int count = 0;
	if (HasPatternSelection())
	{
		int trackMin, trackMax, rowMin, rowMax;
		GetPatternSelectionBounds(&trackMin, &trackMax, &rowMin, &rowMax);
		for (int t = trackMin; t <= trackMax; t++)
		{
			int ch, arp;
			if (!GetPatternTrackInfo(t, &ch, &arp)) continue;
			if (arp != -1) continue;            // arp columns can't echo
			for (int r = rowMin; r <= rowMax; r++)
				if (pushSource(ch, r)) count++;
		}
	}
	else
	{
		if (pushSource(epchn, eppos)) count++;
	}
	return count;
}

void CViewGT2Patterns::RenderEchoPopup()
{
	if (!ImGui::BeginPopup("GT2GenerateEcho"))
		return;

	ImGui::TextUnformatted("Generate Echo");
	ImGui::Separator();
	ImGui::TextDisabled("Cascades each source note forward via cmd 6 (set");
	ImGui::TextDisabled("sustain/release), dropping sustain per step.");
	ImGui::TextDisabled("Echo cells from later sources may overwrite echo");
	ImGui::TextDisabled("cells from earlier ones; real notes are kept.");
	ImGui::Spacing();

	std::vector<EchoSource> sources;
	CollectEchoSources(&sources);
	bool selectionMode = HasPatternSelection();

	if (sources.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f),
			selectionMode
				? "Selection has no real notes (with usable sustain)."
				: "Place the cursor on a real note (with an instrument)");
		if (!selectionMode)
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f),
				"before opening Generate Echo.");
	}
	else if (selectionMode)
	{
		ImGui::Text("Selection: %d source note(s) will echo",
			(int)sources.size());
		const EchoSource &first = sources.front();
		char noteBuf[4] = "...";
		FormatMainTrackNoteNameForDisplay(first.note, noteBuf);
		ImGui::Text("First: ch%d row %d  note %s  instr %02X  S=%X R=%X",
			first.channel + 1, first.row, noteBuf, first.instr,
			first.sustain, first.release);
	}
	else
	{
		const EchoSource &s = sources.front();
		char noteBuf[4] = "...";
		FormatMainTrackNoteNameForDisplay(s.note, noteBuf);
		ImGui::Text("Cursor: ch%d row %d  note %s  instr %02X  S=%X R=%X",
			s.channel + 1, s.row, noteBuf, s.instr, s.sustain, s.release);
	}
	ImGui::Spacing();

	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragInt("Sustain step", &gt2EchoSustainStep, 0.1f, 1, 15, "%d");
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragInt("Row step",     &gt2EchoRowStep,     0.1f, 1, MAX_PATTROWS, "%d");

	if (gt2EchoSustainStep < 1) gt2EchoSustainStep = 1;
	if (gt2EchoSustainStep > 15) gt2EchoSustainStep = 15;
	if (gt2EchoRowStep < 1) gt2EchoRowStep = 1;
	if (gt2EchoRowStep > MAX_PATTROWS) gt2EchoRowStep = MAX_PATTROWS;

	// Target-channel checkboxes — leave them all off to keep the legacy
	// "echo stays on the source's channel" behaviour. Pick any combination
	// to spread the cascade across channels in a ch1→ch2→ch3 cycle. The
	// source channel itself can stay unchecked, e.g. to render a melody on
	// ch2 and route every echo to a free ch3.
	ImGui::Spacing();
	ImGui::TextUnformatted("Target channels (cycle ch1→ch2→ch3):");
	bool maskChanged = false;
	for (int c = 0; c < MAX_CHN; c++)
	{
		if (c > 0) ImGui::SameLine();
		char lbl[16]; snprintf(lbl, sizeof(lbl), "Ch %d##echoCh%d", c + 1, c);
		bool on = (gt2EchoChannelMask & (1 << c)) != 0;
		if (ImGui::Checkbox(lbl, &on))
		{
			if (on) gt2EchoChannelMask |=  (1 << c);
			else    gt2EchoChannelMask &= ~(1 << c);
			maskChanged = true;
		}
	}
	(void)maskChanged;

	ImGui::Spacing();
	ImGui::BeginDisabled(sources.empty());
	if (ImGui::Button("Generate"))
	{
		int written = GenerateEchoAtCursor();
		LOGD("GT2 GenerateEcho: wrote %d echo row(s) from %d source(s)",
			written, (int)sources.size());
		PLUGIN_GoatTrackerSaveSettings();
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		// Still persist sliders / channel mask the user adjusted — they
		// reflect intent for the next run even when this one is cancelled.
		PLUGIN_GoatTrackerSaveSettings();
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

int CViewGT2Patterns::GenerateEchoAtCursor()
{
	std::vector<EchoSource> sources;
	if (CollectEchoSources(&sources) <= 0)
		return 0;

	// Process sources in ascending row order so later (lower-down) notes'
	// echoes can overwrite earlier notes' echoes. Channel is a tiebreaker
	// only — within one row, processing order is irrelevant because each
	// source writes onto its own channel.
	std::sort(sources.begin(), sources.end(),
		[](const EchoSource &a, const EchoSource &b)
		{
			if (a.row != b.row) return a.row < b.row;
			return a.channel < b.channel;
		});

	// Per-channel record of the sustain value (1..15) we wrote into each
	// cell during THIS call. 0 = no note echo placed. Used to resolve
	// clashes between two note echoes that decay onto the same row: the
	// louder one wins. echoIsOff[][] marks cells that hold an OFF echo
	// (KEYOFF with no command). OFF echo loses every clash — it only
	// fills blank cells, and any note echo (sustain >= 1) overwrites it.
	int  echoSustainAt[MAX_CHN][MAX_PATTROWS];
	bool echoIsOff    [MAX_CHN][MAX_PATTROWS];
	memset(echoSustainAt, 0, sizeof(echoSustainAt));
	memset(echoIsOff,     0, sizeof(echoIsOff));

	BeginPatternUndoStep();
	int wroteCount = 0;

	for (const EchoSource &s : sources)
	{
		// Build the per-source cycle of target channels from the user's
		// channel mask. An empty mask falls back to the source's own
		// channel (legacy behaviour). The cycle starts AT the source's
		// position when the source channel is in the mask, otherwise at
		// the next selected channel after the source (wrapping around).
		int cycle[MAX_CHN];
		int cycleLen = 0;
		for (int c = 0; c < MAX_CHN; c++)
			if (gt2EchoChannelMask & (1 << c)) cycle[cycleLen++] = c;
		if (cycleLen == 0)
		{
			cycle[0] = s.channel;
			cycleLen = 1;
		}
		int startIdx = 0;
		{
			int srcSlot = -1;
			for (int i = 0; i < cycleLen; i++)
				if (cycle[i] == s.channel) { srcSlot = i; break; }
			if (srcSlot >= 0)
			{
				startIdx = srcSlot;
			}
			else
			{
				for (int i = 0; i < cycleLen; i++)
					if (cycle[i] > s.channel) { startIdx = i; break; }
				// else startIdx stays 0 (wrap past end of channel range)
			}
		}

		int currentSustain = s.sustain;
		for (int k = 0; ; k++)
		{
			int row = s.row + (k + 1) * gt2EchoRowStep;
			currentSustain -= gt2EchoSustainStep;
			if (currentSustain <= 0) break;
			if (row >= MAX_PATTROWS) break;

			int targetCh = cycle[(startIdx + k) % cycleLen];
			int targetPn = epnum[targetCh];
			if (targetPn < 0 || targetPn >= MAX_PATT) continue;
			if (row >= pattlen[targetPn]) continue;

			unsigned char *patt = pattern[targetPn];
			int rb = row * 4;
			int existingEchoSustain = echoSustainAt[targetCh][row];
			bool existingIsOff       = echoIsOff[targetCh][row];
			unsigned char n = patt[rb + 0];
			bool blank = ((n == REST) || (n == INSTRCHG))
				&& patt[rb + 1] == 0 && patt[rb + 2] == 0
				&& patt[rb + 3] == 0;
			bool cellHasEcho = (existingEchoSustain > 0) || existingIsOff;
			if (!blank && !cellHasEcho)
				continue;   // pre-existing real user content — keep
			if (s.isKeyOff)
			{
				// OFF is the floor — only fills truly blank cells. Any
				// existing echo (note or OFF) keeps the spot.
				if (!blank || cellHasEcho)
					continue;
				patt[rb + 0] = KEYOFF;
				patt[rb + 1] = 0;
				patt[rb + 2] = 0;
				patt[rb + 3] = 0;
				echoIsOff[targetCh][row] = true;
				wroteCount++;
				continue;
			}
			// Note echo: louder-sustain wins against an existing note
			// echo (ties keep what was written first), and any note echo
			// overwrites an OFF echo since OFF has the lowest sustain.
			if (existingEchoSustain > 0 && currentSustain <= existingEchoSustain)
				continue;

			patt[rb + 0] = s.note;
			patt[rb + 1] = s.instr;
			patt[rb + 2] = CMD_SETSR;
			patt[rb + 3] = (unsigned char)((currentSustain << 4) | s.release);
			echoSustainAt[targetCh][row] = currentSustain;
			echoIsOff[targetCh][row]     = false;
			wroteCount++;
		}
	}

	if (wroteCount > 0)
		CommitPatternUndoStep();
	else
		CancelPatternUndoStep();
	return wroteCount;
}

// Resolve a raw key + active keymap (KEY_TRACKER / DMC / JANKO / CUSTOM /
// RENOISE) to a GT2 note byte (FIRSTNOTE..LASTNOTE) at the current octave.
// Returns -1 if the key isn't a mapped note key for the active keymap. Mirrors
// the keymap branch of gpattern.c::patterncommands() and is shared by both
// the arp-track and main-track note-entry handlers so we only walk the
// keymap tables once.
static int GT2_ResolveNoteFromKey(u32 keyCode)
{
	if (shiftpressed) return -1;
	int newnote = -1;
	switch (keypreset)
	{
	case KEY_TRACKER:
		for (int c = 0; c < 15; c++)
			if (keyCode == notekeytbl1[c])
				newnote = FIRSTNOTE + c + epoctave * 12;
		for (int c = 0; c < 17; c++)
			if (keyCode == notekeytbl2[c])
				newnote = FIRSTNOTE + c + (epoctave + 1) * 12;
		break;
	case KEY_DMC:
		for (int c = 0; c < 16; c++)
			if (keyCode == dmckeytbl[c])
				newnote = FIRSTNOTE + c + epoctave * 12;
		break;
	case KEY_JANKO:
		for (int c = 0; c < 17; c++)
			if (keyCode == jankokeytbl1[c])
				newnote = FIRSTNOTE + c + epoctave * 12;
		for (int c = 0; c < 18; c++)
			if (keyCode == jankokeytbl2[c])
				newnote = FIRSTNOTE + c + (epoctave + 1) * 12;
		break;
	case KEY_CUSTOM:
		for (int c = 0; c < 15; c++)
			if (keyCode == customkeytbl1[c])
				newnote = FIRSTNOTE + c + epoctave * 12;
		for (int c = 0; c < 17; c++)
			if (keyCode == customkeytbl2[c])
				newnote = FIRSTNOTE + c + (epoctave + 1) * 12;
		break;
	case KEY_RENOISE:
		for (int c = 0; c < 15; c++)
			if (keyCode == renoisekeytbl1[c])
				newnote = FIRSTNOTE + c + epoctave * 12;
		for (int c = 0; c < 17; c++)
			if (keyCode == renoisekeytbl2[c])
				newnote = FIRSTNOTE + c + (epoctave + 1) * 12;
		break;
	}
	if (newnote > LASTNOTE) newnote = -1;
	return newnote;
}

bool CViewGT2Patterns::HandleMainTrackNoteEntry(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Only fires on the main note column, when the user is actually editing
	// patterns. We must not steal these keys for the instrument column, the
	// command column, the arp columns (HandleArpKey owns those), or anywhere
	// the legacy GT2 modal state (eamode/menu) would otherwise eat input.
	if (isAlt || isControl || isSuper) return false;
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (eparpcol >= 0) return false;
	if (epInSustain) return false;
	if (epcolumn != 0) return false;

	int newnote = GT2_ResolveNoteFromKey(keyCode);
	if (keyCode == MTKEY_BACKSPACE) newnote = REST;
	if (keyCode == MTKEY_DELETE)    newnote = KEYOFF;
	if (newnote < 0) return false;

	int pattNum = epnum[epchn];

	// Mirror gpattern.c:443 — only write while in record mode AND within the
	// pattern's playable rows. Outside that range we still preview the note
	// (it lets users audition before extending the pattern), matching native.
	if (recordmode && eppos < pattlen[pattNum])
	{
		PatternUndoSnapshot before = CapturePatternUndoSnapshot();
		pattern[pattNum][eppos * 4 + 0] = (unsigned char)newnote;
		if (newnote < REST)
		{
			pattern[pattNum][eppos * 4 + 1] = (unsigned char)einum;
		}
		else
		{
			pattern[pattNum][eppos * 4 + 1] = 0;
		}
		// Shift+REST also clears the command bytes, matching gpattern.c:456.
		// Useful for stomping a sustained note dead with one keystroke.
		if (isShift && newnote == REST)
		{
			pattern[pattNum][eppos * 4 + 2] = 0;
			pattern[pattNum][eppos * 4 + 3] = 0;
		}
		CommitPatternUndoSnapshotIfChanged(before);
	}

	if (recordmode)
	{
		if (keypreset == KEY_RENOISE)
		{
			gt2advanceeditstep();
		}
		else if (autoadvance < 2)
		{
			eppos++;
			if (eppos > pattlen[pattNum])
				eppos = 0;
		}
	}

	// Preview the note even outside recordmode and even outside the
	// playable range — matches native behaviour and keeps audition working
	// during non-destructive scrolling.
	playtestnote(newnote, einum, epchn);
	return true;
}

bool CViewGT2Patterns::HandlePatternHexEntry(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// gpattern.c:1209 writes a typed hex digit into the instrument byte
	// (epcolumn 1/2) or the command byte and its argument (3/4/5). Under
	// KEY_RENOISE that code is unreachable: the pattern view's fall-through
	// is GT2_ForwardKeyDown(), which returns without queueing anything for
	// the Renoise layout, so the digit was simply dropped. Note entry kept
	// working because HandleMainTrackNoteEntry() had already been moved here.
	if (isShift || isAlt || isControl || isSuper) return false;
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (eparpcol >= 0) return false;      // arp columns are HandleArpKey's
	if (epInSustain) return false;        // sustain has its own handler
	if (!recordmode) return false;        // native edits only in record mode
	if (epcolumn < 1 || epcolumn > 5) return false;

	// Command Value Mode owns the command area and deliberately treats some
	// of it as a no-op; it runs before this and must keep those columns.
	if (gt2CommandValueMode && epcolumn >= 3) return false;

	int hex = GT2_HexFromKey(keyCode);
	if (hex < 0) return false;

	int pn = epnum[epchn];
	if (eppos >= 0 && eppos < pattlen[pn])
	{
		PatternUndoSnapshot before = CapturePatternUndoSnapshot();
		unsigned char *cell = &pattern[pn][eppos * 4];
		switch (epcolumn)
		{
			case 1:
				cell[1] = (unsigned char)(((cell[1] & 0x0f) | (hex << 4)) & (MAX_INSTR - 1));
				break;
			case 2:
				cell[1] = (unsigned char)(((cell[1] & 0xf0) | hex) & (MAX_INSTR - 1));
				break;
			case 3:
				cell[2] = (unsigned char)hex;
				// Clearing the command clears its argument with it.
				if (cell[2] == 0) cell[3] = 0;
				break;
			case 4:
				cell[3] = (unsigned char)((cell[3] & 0x0f) | (hex << 4));
				if (cell[2] == 0) cell[3] = 0;
				break;
			case 5:
				cell[3] = (unsigned char)((cell[3] & 0xf0) | hex);
				if (cell[2] == 0) cell[3] = 0;
				break;
		}
		CommitPatternUndoSnapshotIfChanged(before);
	}

	// Native advances even when the row was out of range, so do the same.
	AdvanceCommandValueCursor();
	return true;
}

bool CViewGT2Patterns::HandlePatternEnterKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode != MTKEY_ENTER) return false;
	if (isAlt || isControl || isSuper) return false;
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (eparpcol >= 0) return false;
	if (epInSustain) return false;

	// Shift+Enter on the note column = KEYON.  gpattern.c:266.
	if (isShift && epcolumn == 0)
	{
		int pattNum = epnum[epchn];
		if (recordmode && eppos < pattlen[pattNum])
		{
			PatternUndoSnapshot before = CapturePatternUndoSnapshot();
			pattern[pattNum][eppos * 4 + 0] = (unsigned char)KEYON;
			pattern[pattNum][eppos * 4 + 1] = 0;
			CommitPatternUndoSnapshotIfChanged(before);
		}
		if (recordmode)
		{
			if (keypreset == KEY_RENOISE)
				gt2advanceeditstep();
			else if (autoadvance < 2)
			{
				eppos++;
				if (eppos > pattlen[pattNum]) eppos = 0;
			}
		}
		return true;
	}

	// Bare Enter on the instr column with an instrument byte set in the
	// current row = jump to that instrument's editor. gpattern.c:273.
	if (!isShift && (epcolumn == 1 || epcolumn == 2))
	{
		int pattNum = epnum[epchn];
		if (eppos >= 0 && eppos < pattlen[pattNum])
		{
			unsigned char instr = pattern[pattNum][eppos * 4 + 1];
			if (instr != 0)
			{
				gotoinstr(instr);
				if (pluginGoatTracker && pluginGoatTracker->viewInstrument)
					pluginGoatTracker->viewInstrument->visible = true;
				return true;
			}
		}
	}

	return false;
}

bool CViewGT2Patterns::HandleMainTrackNavigation(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Restores the bare-arrow / Home / End / Shift+arrow navigation that
	// gpattern.c's KEY_LEFT / KEY_RIGHT / KEY_UP / KEY_DOWN / KEY_HOME /
	// KEY_END branches used to provide while the legacy key forward was
	// alive. Only fires on the main track (eparpcol < 0, not sustain) and
	// in EDIT_PATTERN — arp/sustain boundary nav is HandleArpKey's job,
	// and outside EDIT_PATTERN this is irrelevant.
	if (isAlt || isControl || isSuper) return false;
	if (editmode != EDIT_PATTERN) return false;
	if (eamode || menu) return false;
	if (eparpcol >= 0) return false;

	// Sustain reserves LEFT/RIGHT for HandleArpKey (sub-column boundary nav
	// between sustain and instr-lo / arp-0 / cmd-hi). But the rest of the
	// navigation set — row up/down, Home/End, and the shift-arrow pattern
	// number step — has no reason to be blocked just because the cursor
	// happens to be parked on sustain.
	bool isHorizontalArrow = (keyCode == MTKEY_ARROW_LEFT || keyCode == MTKEY_ARROW_RIGHT);
	if (epInSustain && isHorizontalArrow && !isShift) return false;

	int pattNum = epnum[epchn];

	// Shift+Left / Shift+Right step this channel's pattern number, matching
	// gpattern.c:1099/1122. Clamp to MAX_PATT-1 / 0; clamp eppos to the new
	// pattern's playable length so we don't end up past END.
	if (isShift && keyCode == MTKEY_ARROW_RIGHT)
	{
		if (epnum[epchn] < MAX_PATT - 1) epnum[epchn]++;
		if (eppos > pattlen[epnum[epchn]]) eppos = pattlen[epnum[epchn]];
		return true;
	}
	if (isShift && keyCode == MTKEY_ARROW_LEFT)
	{
		if (epnum[epchn] > 0) epnum[epchn]--;
		if (eppos > pattlen[epnum[epchn]]) eppos = pattlen[epnum[epchn]];
		return true;
	}

	if (isShift) return false;  // other Shift+arrow combos: leave to other handlers

	if (keyCode == MTKEY_ARROW_RIGHT)
	{
		// epcolumn=2 (instr-lo) RIGHT is HandleArpKey's job when sustain/arp
		// columns exist. When there's neither sustain nor arps it falls
		// here — step to cmd-hi (epcolumn=3).
		if (epcolumn == 2)
		{
			epcolumn = 3;
			return true;
		}
		epcolumn++;
		if (epcolumn > 5)
		{
			epcolumn = 0;
			epchn++;
			if (epchn >= MAX_CHN) epchn = 0;
			if (eppos > pattlen[epnum[epchn]]) eppos = pattlen[epnum[epchn]];
		}
		return true;
	}
	if (keyCode == MTKEY_ARROW_LEFT)
	{
		// epcolumn=3 (cmd-hi) LEFT goes to instr-lo when there's no sustain
		// or arp gutter between (HandleArpKey handles the in-gutter cases).
		if (epcolumn == 3)
		{
			epcolumn = 2;
			return true;
		}
		epcolumn--;
		if (epcolumn < 0)
		{
			epcolumn = 5;
			epchn--;
			if (epchn < 0) epchn = MAX_CHN - 1;
			if (eppos > pattlen[epnum[epchn]]) eppos = pattlen[epnum[epchn]];
		}
		return true;
	}
	if (keyCode == MTKEY_ARROW_UP)
	{
		eppos--;
		if (eppos < 0) eppos = pattlen[epnum[epchn]];
		SeekPlayerToCursorIfPlaying();
		return true;
	}
	if (keyCode == MTKEY_ARROW_DOWN)
	{
		eppos++;
		if (eppos > pattlen[epnum[epchn]]) eppos = 0;
		SeekPlayerToCursorIfPlaying();
		return true;
	}
	if (keyCode == MTKEY_HOME)
	{
		eppos = 0;
		SeekPlayerToCursorIfPlaying();
		return true;
	}
	if (keyCode == MTKEY_END)
	{
		eppos = pattlen[pattNum];
		SeekPlayerToCursorIfPlaying();
		return true;
	}
	return false;
}

void CViewGT2Patterns::SeekPlayerToCursorIfPlaying()
{
	// Cursor-leads-playback. The first cut just rewrote pattptr — that
	// made the *display* jump but left audio at the song's tempo, because
	// gplay.c::playroutine only reads a new pattern row when
	// `cptr->tick == cptr->gatetimer` (one new-note tick every ~20 ms at
	// PAL tempo). Without bumping tick too, pattptr just changed *where*
	// the next note read would happen, not *when*.
	//
	// Fix: also force `cptr->tick = cptr->gatetimer` so the next
	// playroutine call on the audio thread fires GETNEWNOTES immediately
	// for that channel, reads pattern[cptr->pattnum][eppos*4] right then.
	// Combined with key auto-repeat, holding Down stacks extra new-note
	// fires on top of the player's normal tempo → audible fast-forward.
	// Up rewinds eppos and the same mechanism re-fires the earlier row.
	if (!isplaying()) return;
	for (int c = 0; c < MAX_CHN; c++)
	{
		int pattNum = epnum[c];
		if (pattNum < 0 || pattNum >= MAX_PATT) continue;
		int newPattptr = eppos * 4;
		int maxPattptr = pattlen[pattNum] * 4;
		if (newPattptr < 0) newPattptr = 0;
		if (newPattptr > maxPattptr) newPattptr = maxPattptr;
		chn[c].pattptr = (unsigned)newPattptr;
		chn[c].advance = 0;
		// Trip the new-note latch so the audio thread reads the jumped
		// row on its very next playroutine call instead of waiting out
		// the rest of this gate cycle.
		chn[c].tick = chn[c].gatetimer;
	}
}

bool CViewGT2Patterns::HandleArpKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (isAlt || isControl || isSuper)
		return false;

	// Cursor sub-column navigation. The cursor walks left-to-right through:
	//   note, instr-hi, instr-lo, [sustain], arp0, arp1, ..., cmd, ...
	// Native GT2 only knows about the main 6 (note/instr/cmd nibbles); we
	// intercept at the instr↔sustain↔arp↔cmd boundaries. Channel wrap stays
	// native (Right at cmd-lo / Left at note).
	if (editmode == EDIT_PATTERN && !isShift)
	{
		bool sustainOn = (gt2VisibleSustainColumn != 0);
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (eparpcol >= 0)
			{
				eparpcol++;
				if (eparpcol >= numarpcolumns)
				{
					// past the last arp → command nibble, same channel
					eparpcol = -1;
					epcolumn = 3;
				}
				return true;
			}
			if (epInSustain)
			{
				// sustain → first arp (or command nibble when no arps)
				epInSustain = false;
				if (numarpcolumns > 0)
					eparpcol = 0;
				else
					epcolumn = 3;
				return true;
			}
			if (epcolumn == 2 && sustainOn)
			{
				// instrument low → sustain column
				epInSustain = true;
				return true;
			}
			if (epcolumn == 2 && numarpcolumns > 0)
			{
				// instrument low → first arp column
				eparpcol = 0;
				return true;
			}
		}
		else if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (eparpcol > 0)
			{
				eparpcol--;
				return true;
			}
			if (eparpcol == 0)
			{
				// first arp → sustain (if shown) else instrument low
				eparpcol = -1;
				if (sustainOn)
					epInSustain = true;
				else
					epcolumn = 2;
				return true;
			}
			if (epInSustain)
			{
				// sustain → instrument low
				epInSustain = false;
				epcolumn = 2;
				return true;
			}
			if (epcolumn == 3 && numarpcolumns > 0)
			{
				// command nibble → last arp column
				eparpcol = numarpcolumns - 1;
				return true;
			}
			if (epcolumn == 3 && sustainOn && numarpcolumns == 0)
			{
				// command nibble → sustain (no arps in between)
				epInSustain = true;
				return true;
			}
		}
	}

	// When cursor is in an arp column, handle note entry directly.
	if (eparpcol >= 0 && editmode == EDIT_PATTERN)
	{
		int newnote = GT2_ResolveNoteFromKey(keyCode);
		if (keyCode == MTKEY_BACKSPACE)
			newnote = 0;
		if (keyCode == MTKEY_DELETE)
			newnote = KEYOFF;

		if (newnote >= 0)
		{
			int pattNum = epnum[epchn];

			// Look up the base note at this row BEFORE we advance eppos.
			// We retrigger it (not the arp note) so the user hears the
			// whole arp chord cycling, not just the typed arp note alone.
			int baseAtRow = 0;
			if (eppos >= 0 && eppos <= pattlen[pattNum])
			{
				unsigned char nb = pattern[pattNum][eppos * 4];
				if (nb >= FIRSTNOTE && nb <= LASTNOTE)
					baseAtRow = nb;
			}

			// Match main-track gate: write+advance only while in-range
			// (gpattern.c:388 uses `eppos < pattlen` not `<=`).
			if ((recordmode) && (eppos < pattlen[pattNum]))
			{
				PatternUndoSnapshot before = CapturePatternUndoSnapshot();
				if (newnote == 0)
					arpdata[pattNum][epchn][eppos][eparpcol] = 0;
				else
					arpdata[pattNum][epchn][eppos][eparpcol] = (unsigned char)newnote;

				CHN *cptr = &chn[epchn];
				if (newnote == 0 || newnote == KEYOFF)
					cptr->arpcolnotes[eparpcol] = 0;
				else
					cptr->arpcolnotes[eparpcol] = (unsigned char)(newnote - FIRSTNOTE);
				CommitPatternUndoSnapshotIfChanged(before);
			}
			if (recordmode)
			{
				if (keypreset == KEY_RENOISE)
				{
					gt2advanceeditstep();
				}
				else if (autoadvance < 2)
				{
					eppos++;
					if (eppos > pattlen[epnum[epchn]])
						eppos = 0;
				}
			}

			// Audio preview: trigger the base note so the arp pool —
			// which now includes the just-typed arp note — cycles audibly.
			// The player thread's per-tick rebuildarp (gplay.c:1008)
			// will rebuild arpnotes[] from cptr->note + arpcolnotes[],
			// arpcount becomes >=2, and the cycling override at
			// gplay.c:1033 alternates SID freq between base and arp note.
			// Fallback to the arp note alone if the row has no base.
			int testNote = baseAtRow ? baseAtRow
				: ((newnote >= FIRSTNOTE && newnote <= LASTNOTE) ? newnote : 0);
			if (testNote)
				playtestnote(testNote, einum, epchn);
			return true;
		}

		if (keyCode == MTKEY_ARROW_UP)
		{
			eppos--;
			if (eppos < 0) eppos = pattlen[epnum[epchn]];
			// Same cursor-leads-playback hook as the main track's Up/Down
			// in HandleMainTrackNavigation — without this, holding Up/Down
			// while the cursor sits in an arp column would only scroll the
			// pattern display and leave audio at the song's tempo.
			SeekPlayerToCursorIfPlaying();
			return true;
		}
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			eppos++;
			if (eppos > pattlen[epnum[epchn]]) eppos = 0;
			SeekPlayerToCursorIfPlaying();
			return true;
		}
		if (keyCode == SDLK_BACKSLASH)
		{
			if (epoctave < 7) epoctave++;
			return true;
		}
		if (keyCode == SDLK_SLASH)
		{
			if (epoctave > 0) epoctave--;
			return true;
		}
	}

	return false;
}

bool CViewGT2Patterns::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Command Value Mode owns the command field while editing — must run
	// before renoiseInput, which would otherwise swallow Tab/Enter/Esc.
	if (HandleCommandValueKey(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	// Sustain column edit — only fires when the cursor is parked on it.
	if (HandleSustainColumnKey(keyCode, isShift, isAlt, isControl, isSuper))
		return true;
	// Hex digits on the instrument / command columns. Runs before the
	// modifier blocks below, but claims bare keys only, so Shift+1..3 (mute)
	// and the Ctrl/Shift shortcuts still reach them.
	if (HandlePatternHexEntry(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	if (!isShift && !isAlt && (isControl || isSuper))
	{
		if (keyCode == 'a' || keyCode == 'A' || keyCode == SDLK_A)
			return SelectWholePattern();
		if (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z)
		{
			UndoPatternEdit();
			return true;
		}
		if (keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y)
		{
			RedoPatternEdit();
			return true;
		}
	}

	if (isShift && !isAlt && !isControl && !isSuper)
	{
		if (keyCode == 'l' || keyCode == 'L' || keyCode == SDLK_L)
			return SelectWholePattern();
		if (keyCode == 'e' || keyCode == 'E' || keyCode == SDLK_E)
			return CopyEffectsAtCursorOrSelection();
		if (keyCode == 'r' || keyCode == 'R' || keyCode == SDLK_R)
			return PasteEffectsAtCursor();
		if (keyCode == 'h' || keyCode == 'H' || keyCode == SDLK_H)
			return MakeHiFiVibratoPortaSpeed();
		if (keyCode == 'i' || keyCode == 'I' || keyCode == SDLK_I)
			return InvertSelectionOrPattern();
		if (keyCode == 'j' || keyCode == 'J' || keyCode == SDLK_J)
			return JoinPatternAtCursor();
		if (keyCode == 'k' || keyCode == 'K' || keyCode == SDLK_K)
			return SplitPatternAtCursor();
		if (keyCode == 'm' || keyCode == 'M' || keyCode == SDLK_M)
			return AdjustHighlightStep(1);
		if (keyCode == 'n' || keyCode == 'N' || keyCode == SDLK_N)
			return AdjustHighlightStep(-1);
		if (keyCode == 'o' || keyCode == 'O' || keyCode == SDLK_O)
			return ShrinkSelectionOrPattern();
		if (keyCode == 'p' || keyCode == 'P' || keyCode == SDLK_P)
			return ExpandSelectionOrPattern();
		if (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z)
			return CycleAutoadvanceMode();
		if (keyCode == 'c' || keyCode == 'C' || keyCode == SDLK_C)
			return patternSelectionActive ? CopyPatternSelection() : CopyPhrase();
		if (keyCode == 'x' || keyCode == 'X' || keyCode == SDLK_X)
			return patternSelectionActive ? CutPatternSelection() : CutPhrase();
		if (keyCode == 'v' || keyCode == 'V' || keyCode == SDLK_V)
			return patternSelectionActive ? PasteAtCursor() : PastePhrase();
		// Shift+Q/A/W/S: transpose. Without an active selection they used
		// to fall through to native GT2 which transposes the marked OR whole
		// pattern silently — destructive. TransposeAtCursor already prefers
		// the selection when active and falls back to the cursor cell.
		if (keyCode == 'q' || keyCode == 'Q' || keyCode == SDLK_Q)
			return TransposeAtCursor(12);
		if (keyCode == 'a' || keyCode == 'A' || keyCode == SDLK_A)
			return TransposeAtCursor(-12);
		if (keyCode == 'w' || keyCode == 'W' || keyCode == SDLK_W)
			return TransposeAtCursor(1);
		if (keyCode == 's' || keyCode == 'S' || keyCode == SDLK_S)
			return TransposeAtCursor(-1);
	}

	if (!isAlt && !isShift && (isControl || isSuper))
	{
		if (keyCode == 'c' || keyCode == 'C' || keyCode == SDLK_C)
			return CopyAtCursor();
		if (keyCode == 'x' || keyCode == 'X' || keyCode == SDLK_X)
			return CutAtCursor();
		if (keyCode == 'v' || keyCode == 'V' || keyCode == SDLK_V)
			return PasteAtCursor();
	}

	if (!isAlt && !isControl && !isSuper && HandlePatternSelectionShortcut(keyCode, isShift))
		return true;

	// Pattern-view Enter overlays (Shift+Enter → KEYON, Enter on instr
	// col → gotoinstr) — must run BEFORE renoiseInput so the dispatcher's
	// HandleEnterTriggerRow doesn't claim bare Enter on the instr column.
	if (HandlePatternEnterKey(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	if (pluginGoatTracker && pluginGoatTracker->renoiseInput
		&& pluginGoatTracker->renoiseInput->HandleKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	if (HandleArpKey(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	// Main-track note entry — keymap-aware note write + preview without
	// bouncing through native GT2. Runs after HandleArpKey so arp-track
	// entry is preserved exactly as before.
	if (HandleMainTrackNoteEntry(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	// Main-track cursor navigation (arrows / Home / End / Shift+arrows).
	// Replaces what gpattern.c handled before we cut the native key
	// forward. Runs after note entry so the keymap can claim a key
	// before we treat it as bare-arrow nav.
	if (HandleMainTrackNavigation(keyCode, isShift, isAlt, isControl, isSuper))
		return true;

	// Any non-modifier forwarded to GT2 leaves the arp column. Pure modifiers
	// only update GT2's key state; they must not move the Renoise track cursor.
	if (!GT2_IsModifierKey(keyCode))
	{
		eparpcol = -1;
	}

	GT2_ForwardKeyDown(keyCode);
	return true;
}

bool CViewGT2Patterns::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2Patterns::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
