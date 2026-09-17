#include "CViewGT2PatternList.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "CGT2FontAtlas.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewGT2Patterns.h"
#include "CGT2RenoiseInput.h"
#include "CViewC64.h"
#include "imgui.h"
#include "SYS_KeyCodes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsong.h"
#include "gorder.h"

extern int songlen[MAX_SONGS][MAX_CHN];
extern unsigned char songorder[MAX_SONGS][MAX_CHN][MAX_SONGLEN+2];
extern int esnum;
extern int eschn;
extern int eseditpos;
extern int epnum[MAX_CHN];
extern int eppos;
extern int epchn;
extern int pattlen[MAX_PATT];
extern int editmode;
extern int eamode;
extern int cursorflash;
extern int cursorcolortable[];
extern unsigned char pattused[MAX_PATT];
}

// GT2 color constants (from gdisplay.h)
#define CNORMAL  8
#define CEDIT    10
#define CCOMMAND 7
#define CTITLE   15

#define EDIT_ORDERLIST 1

// Column layout (in character cells)
#define PL_BLOCK_COL    0   // block index, 2 hex digits
#define PL_SEP_COL      2   // " | "
#define PL_CHAN_COL     5   // first real-pattern channel column; 3 cells each

static int GT2PatternList_HexFromKey(u32 keyCode)
{
	if (keyCode >= '0' && keyCode <= '9') return (int)(keyCode - '0');
	if (keyCode >= 'a' && keyCode <= 'f') return (int)(keyCode - 'a' + 10);
	if (keyCode >= 'A' && keyCode <= 'F') return (int)(keyCode - 'A' + 10);
	return -1;
}

// Row clipboard for cut/copy/paste — raw per-channel order-list values.
static unsigned char s_clipboard[MAX_SONGLEN + 2][MAX_CHN];
static int s_clipboardLen = 0;

// Fill out[] with `count` distinct free patterns — not referenced by any
// subtune's order list — skipping any listed in exclude[]. findusedpatterns()
// must run first to populate pattused[]. Returns true if `count` were found.
static bool PL_FindEmptyPatterns(int count, int *out, const int *exclude, int excludeCount)
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

CViewGT2PatternList::CViewGT2PatternList(const char *name, float posX, float posY, float posZ,
										 float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
	this->scrollPos = 0;
	this->editCol = 0;
	this->selAnchor = -1;
	this->selEnd = -1;
	this->selecting = false;
	this->dragScrollPos = 0;
	imGuiNoScrollbar = true;
}

CViewGT2PatternList::~CViewGT2PatternList()
{
}

int CViewGT2PatternList::MaxOrderLen() const
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return 0;
	int numCh = GT2_NumChannels();
	int maxlen = 0;
	for (int c = 0; c < numCh && c < MAX_CHN; c++)
	{
		if (songlen[esnum][c] > maxlen)
			maxlen = songlen[esnum][c];
	}
	return maxlen;
}

// Pull the GT2 pattern editor (epnum[]) onto the patterns at the cursor row,
// so editing or moving here updates an open GT2 Patterns view.
void CViewGT2PatternList::SyncPatternEditorToCursor()
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int numCh = GT2_NumChannels();
	for (int c = 0; c < numCh && c < MAX_CHN; c++)
	{
		if (eseditpos >= 0 && eseditpos < songlen[esnum][c])
		{
			int e = songorder[esnum][c][eseditpos];
			if (e < REPEAT)   // a plain pattern, not a repeat/transpose marker
				epnum[c] = e;
		}
	}
	if (epchn >= 0 && epchn < MAX_CHN && eppos > pattlen[epnum[epchn]])
		eppos = pattlen[epnum[epchn]];
}

void CViewGT2PatternList::ClearSelection()
{
	selAnchor = -1;
	selEnd = -1;
	selecting = false;
}

void CViewGT2PatternList::MoveCursor(int delta)
{
	eseditpos += delta;
	int maxlen = MaxOrderLen();
	if (eseditpos < 0) eseditpos = 0;
	if (eseditpos > maxlen) eseditpos = maxlen;
	editCol = 0;
	ClearSelection();
	SyncPatternEditorToCursor();
}

// True when the cursor sits on a real, plain-pattern order position that the
// block index can be applied to (not the end row or a repeat/transpose marker).
bool CViewGT2PatternList::CanEditBlock() const
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return false;
	if (eseditpos < 0 || eseditpos >= songlen[esnum][0])
		return false;
	return songorder[esnum][0][eseditpos] < REPEAT;
}

// Write a block index to the cursor row: every channel's pattern becomes
// block * numChannels + c. Clamped, wrapped in one undo step.
void CViewGT2PatternList::WriteBlock(int block)
{
	int numCh = GT2_NumChannels();
	if (numCh < 1)
		return;
	int maxBlock = (MAX_PATT - numCh) / numCh;
	if (block < 0) block = 0;
	if (block > maxBlock) block = maxBlock;

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	for (int c = 0; c < numCh && c < MAX_CHN; c++)
	{
		if (eseditpos < songlen[esnum][c])
			songorder[esnum][c][eseditpos] = (unsigned char)(block * numCh + c);
	}
	SyncPatternEditorToCursor();
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

// Edit one hex nibble of the block index at the cursor row.
void CViewGT2PatternList::EditBlockNibble(int hexValue)
{
	if (!CanEditBlock())
		return;
	int numCh = GT2_NumChannels();
	int curBlock = songorder[esnum][0][eseditpos] / numCh;
	int newBlock = (editCol == 0)
		? ((hexValue << 4) | (curBlock & 0x0F))
		: ((curBlock & 0xF0) | hexValue);
	WriteBlock(newBlock);
	editCol++;
	if (editCol > 1)
		MoveCursor(1);   // also resets editCol to 0
}

// Step the cursor row's block index by delta (Ctrl+Left / Ctrl+Right). One
// block step shifts every channel's real pattern number by numChannels.
void CViewGT2PatternList::AdjustBlock(int delta)
{
	if (!CanEditBlock())
		return;
	int numCh = GT2_NumChannels();
	WriteBlock(songorder[esnum][0][eseditpos] / numCh + delta);
}

// The row range the next operation acts on: the mouse selection if there is
// one, otherwise just the cursor row. Clamped to the order list.
void CViewGT2PatternList::SelectionRange(int *lo, int *hi) const
{
	int a, b;
	if (selAnchor >= 0)
	{
		a = (selAnchor < selEnd) ? selAnchor : selEnd;
		b = (selAnchor < selEnd) ? selEnd : selAnchor;
	}
	else
	{
		a = b = eseditpos;
	}
	int slen = (esnum >= 0 && esnum < MAX_SONGS) ? songlen[esnum][0] : 0;
	if (a < 0) a = 0;
	if (b > slen - 1) b = slen - 1;
	*lo = a;
	*hi = b;
}

// Ctrl+C — copy the selected rows (raw per-channel order values) to the row
// clipboard. The copies are references: pasting points at the same patterns.
void CViewGT2PatternList::CopySelection()
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int lo, hi;
	SelectionRange(&lo, &hi);
	s_clipboardLen = 0;
	for (int i = lo; i <= hi && s_clipboardLen <= MAX_SONGLEN; i++)
	{
		for (int c = 0; c < MAX_CHN; c++)
			s_clipboard[s_clipboardLen][c] =
				(i < songlen[esnum][c]) ? songorder[esnum][c][i] : 0;
		s_clipboardLen++;
	}
}

// Delete order rows [lo, hi] on every channel, but never empty the list.
// Not undo-wrapped — the caller owns the undo step.
void CViewGT2PatternList::DeleteRowRange(int lo, int hi)
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int slen = songlen[esnum][0];
	if (lo < 0) lo = 0;
	if (hi > slen - 1) hi = slen - 1;
	if (hi < lo)
		return;
	int count = hi - lo + 1;
	if (count > slen - 1)
		count = slen - 1;          // keep at least one row, like Renoise
	if (count < 1)
		return;
	int savedEschn = eschn;
	for (int n = 0; n < count; n++)
	{
		for (int c = 0; c < MAX_CHN; c++)
		{
			if (lo < songlen[esnum][c])
			{
				eschn = c;
				eseditpos = lo;
				deleteorder();
			}
		}
	}
	eschn = savedEschn;
}

// Ctrl+X — copy then delete the selected rows.
void CViewGT2PatternList::CutSelection()
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	CopySelection();
	int lo, hi;
	SelectionRange(&lo, &hi);

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	DeleteRowRange(lo, hi);
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->CommitPatternUndoStep();

	eseditpos = lo;
	int maxlen = MaxOrderLen();
	if (eseditpos > maxlen) eseditpos = maxlen;
	if (eseditpos < 0) eseditpos = 0;
	editCol = 0;
	ClearSelection();
	SyncPatternEditorToCursor();
}

// Ctrl+V — insert the clipboard rows at the cursor (references, not new data).
void CViewGT2PatternList::PasteRows()
{
	if (esnum < 0 || esnum >= MAX_SONGS || s_clipboardLen <= 0)
		return;

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	int savedEschn = eschn;
	int pos = eseditpos;
	for (int n = 0; n < s_clipboardLen; n++)
	{
		for (int c = 0; c < MAX_CHN; c++)
		{
			int target = pos + n;
			eschn = c;
			// insertorder is a no-op at exactly songlen; songlen+1 appends.
			eseditpos = (target < songlen[esnum][c]) ? target : songlen[esnum][c] + 1;
			insertorder(s_clipboard[n][c]);
		}
	}
	eschn = savedEschn;
	countpatternlengths();
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->CommitPatternUndoStep();

	eseditpos = pos;
	editCol = 0;
	ClearSelection();
	SyncPatternEditorToCursor();
}

// Duplicate — clone every selected row's patterns into fresh empty patterns and
// insert the clones right after the selection. With no selection the cursor row
// is duplicated.
void CViewGT2PatternList::DuplicateSelection()
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int lo, hi;
	SelectionRange(&lo, &hi);
	if (hi < lo)
		return;
	int rows = hi - lo + 1;

	findusedpatterns();

	// Collect the source patterns of the selected rows (plain patterns only).
	int srcs[MAX_PATT];
	int srcRow[MAX_PATT];
	int srcChn[MAX_PATT];
	int n = 0;
	for (int r = lo; r <= hi && n < MAX_PATT; r++)
	{
		for (int c = 0; c < MAX_CHN && n < MAX_PATT; c++)
		{
			if (r >= songlen[esnum][c])
				continue;
			int src = songorder[esnum][c][r];
			if (src >= MAX_PATT)            // a marker, not a pattern
				continue;
			srcs[n] = src;
			srcRow[n] = r;
			srcChn[n] = c;
			n++;
		}
	}
	if (n == 0)
		return;

	int dsts[MAX_PATT];
	if (!PL_FindEmptyPatterns(n, dsts, srcs, n))
	{
		if (viewC64)
			viewC64->ShowMessageError("GT2 Duplicate: not enough empty patterns");
		return;
	}

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->BeginPatternUndoStep();

	for (int i = 0; i < n; i++)
	{
		memcpy(pattern[dsts[i]], pattern[srcs[i]], sizeof(pattern[0]));
		memcpy(arpdata[dsts[i]], arpdata[srcs[i]], sizeof(arpdata[0]));
	}

	// Clone values per (row, channel): the fresh pattern where one was made,
	// otherwise a verbatim copy of the original entry (e.g. markers).
	int cloneVal[MAX_SONGLEN + 2][MAX_CHN];
	for (int r = 0; r < rows; r++)
		for (int c = 0; c < MAX_CHN; c++)
			cloneVal[r][c] = ((lo + r) < songlen[esnum][c])
				? songorder[esnum][c][lo + r] : 0;
	for (int i = 0; i < n; i++)
		cloneVal[srcRow[i] - lo][srcChn[i]] = dsts[i];

	int savedEschn = eschn;
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < MAX_CHN; c++)
		{
			int pos = hi + 1 + r;
			eschn = c;
			// insertorder is a no-op at exactly songlen; songlen+1 appends.
			eseditpos = (pos < songlen[esnum][c]) ? pos : songlen[esnum][c] + 1;
			insertorder((unsigned char)cloneVal[r][c]);
		}
	}
	eschn = savedEschn;
	countpatternlengths();

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->CommitPatternUndoStep();

	eseditpos = hi + 1;
	int maxlen = MaxOrderLen();
	if (eseditpos > maxlen) eseditpos = maxlen;
	editCol = 0;
	ClearSelection();
	SyncPatternEditorToCursor();
}

// Renumber the song's blocks per newBlockOfOld[oldBlock] (-1 = unused), moving
// the pattern data to match. All relocation reads come from a pre-op snapshot,
// so an already-overwritten block is never re-read as a source.
void CViewGT2PatternList::RenumberBlocks(const int *newBlockOfOld)
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int numCh = GT2_NumChannels();
	if (numCh < 1)
		return;

	size_t patBytes = sizeof(pattern);
	size_t arpBytes = sizeof(arpdata);
	unsigned char *tmpPat = (unsigned char *)malloc(patBytes);
	unsigned char *tmpArp = (unsigned char *)malloc(arpBytes);
	if (!tmpPat || !tmpArp)
	{
		free(tmpPat);
		free(tmpArp);
		return;
	}
	memcpy(tmpPat, pattern, patBytes);
	memcpy(tmpArp, arpdata, arpBytes);

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->BeginPatternUndoStep();

	int newBlockCount = 0;
	for (int b = 0; b < MAX_PATT; b++)
	{
		int nb = newBlockOfOld[b];
		if (nb < 0)
			continue;
		if (nb + 1 > newBlockCount)
			newBlockCount = nb + 1;
		for (int c = 0; c < numCh; c++)
		{
			int op = b * numCh + c;
			int np = nb * numCh + c;
			if (op < 0 || op >= MAX_PATT || np < 0 || np >= MAX_PATT)
				continue;
			memcpy(pattern[np], tmpPat + (size_t)op * sizeof(pattern[0]),
				   sizeof(pattern[0]));
			memcpy(arpdata[np], tmpArp + (size_t)op * sizeof(arpdata[0]),
				   sizeof(arpdata[0]));
		}
	}
	free(tmpPat);
	free(tmpArp);

	// Rewrite this subtune's order list to the new block numbers.
	for (int c = 0; c < MAX_CHN; c++)
	{
		for (int i = 0; i < songlen[esnum][c]; i++)
		{
			int e = songorder[esnum][c][i];
			if (e >= REPEAT)               // a marker — leave it untouched
				continue;
			int b = e / numCh;
			if (b >= 0 && b < MAX_PATT && newBlockOfOld[b] >= 0)
				songorder[esnum][c][i] =
					(unsigned char)(newBlockOfOld[b] * numCh + c);
		}
	}

	// Free the patterns above the compacted range, unless another subtune
	// still references them.
	countpatternlengths();
	findusedpatterns();
	for (int p = newBlockCount * numCh; p < MAX_PATT; p++)
	{
		if (pattused[p])
			continue;
		memset(pattern[p], 0, sizeof(pattern[0]));
		pattern[p][0] = ENDPATT;
		memset(arpdata[p], 0, sizeof(arpdata[0]));
	}
	countpatternlengths();

	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
		pluginGoatTracker->viewPatterns->CommitPatternUndoStep();

	int maxlen = MaxOrderLen();
	if (eseditpos > maxlen) eseditpos = maxlen;
	if (eseditpos < 0) eseditpos = 0;
	editCol = 0;
	ClearSelection();
	SyncPatternEditorToCursor();
}

// Remove unused — drop blocks not referenced by the order list and compact the
// rest down by ascending block number (e.g. 00 03 04 05 -> 00 01 02 03).
void CViewGT2PatternList::RemoveUnusedPatterns()
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int numCh = GT2_NumChannels();
	if (numCh < 1)
		return;

	bool used[MAX_PATT];
	for (int b = 0; b < MAX_PATT; b++)
		used[b] = false;
	for (int c = 0; c < MAX_CHN; c++)
		for (int i = 0; i < songlen[esnum][c]; i++)
		{
			int e = songorder[esnum][c][i];
			if (e >= REPEAT)
				continue;
			int b = e / numCh;
			if (b >= 0 && b < MAX_PATT)
				used[b] = true;
		}

	int newBlockOfOld[MAX_PATT];
	int next = 0;
	for (int b = 0; b < MAX_PATT; b++)
		newBlockOfOld[b] = used[b] ? next++ : -1;

	RenumberBlocks(newBlockOfOld);
}

// Sort — renumber blocks so the order list reads 0, 1, 2, ... in sequence
// (e.g. 00 03 01 02 -> 00 01 02 03). The playback order is unchanged.
void CViewGT2PatternList::SortPatterns()
{
	if (esnum < 0 || esnum >= MAX_SONGS)
		return;
	int numCh = GT2_NumChannels();
	if (numCh < 1)
		return;

	int newBlockOfOld[MAX_PATT];
	for (int b = 0; b < MAX_PATT; b++)
		newBlockOfOld[b] = -1;
	int next = 0;
	// Assign new numbers in order-list appearance order; channel 0 drives it,
	// other channels only catch blocks channel 0 never references.
	for (int c = 0; c < MAX_CHN; c++)
		for (int i = 0; i < songlen[esnum][c]; i++)
		{
			int e = songorder[esnum][c][i];
			if (e >= REPEAT)
				continue;
			int b = e / numCh;
			if (b >= 0 && b < MAX_PATT && newBlockOfOld[b] < 0)
				newBlockOfOld[b] = next++;
		}

	RenumberBlocks(newBlockOfOld);
}

void CViewGT2PatternList::RenderImGui()
{
	PreRenderImGui();
	if (!fontAtlas->TryLoad()) { PostRenderImGui(); return; }
	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);

	// Toolbar: insert / remove / duplicate the cursor row. With Bulk Pattern
	// Number Change on these act on the whole block (every channel) at the
	// cursor position; Duplicate clones the row's patterns into fresh ones.
	if (pluginGoatTracker && pluginGoatTracker->renoiseInput)
	{
		ImGuiStyle &style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
			ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
			ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));
		CGT2RenoiseInput *ri = pluginGoatTracker->renoiseInput;
		if (ImGui::Button("+", ImVec2(26.0f * uiScale, 0))) ri->HandleInsertPattern();
		ImGui::SetItemTooltip("Insert a new row at the cursor");
		ImGui::SameLine();
		if (ImGui::Button("-", ImVec2(26.0f * uiScale, 0))) ri->HandleDeletePattern();
		ImGui::SetItemTooltip("Remove the cursor row");
		ImGui::SameLine();
		if (ImGui::Button("Duplicate")) ri->HandleDuplicatePattern();
		ImGui::SetItemTooltip("Duplicate the cursor row into fresh patterns");
		ImGui::PopStyleVar(2);
	}

	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetCursorScreenPos();

	ImVec2 avail = ImGui::GetContentRegionAvail();
	float windowW = avail.x;
	float windowH = avail.y;

	int visibleRows = (int)(windowH / GT2CellH()) - 1;   // -1 for the title row
	if (visibleRows < 1) visibleRows = 1;

	int numCh = GT2_NumChannels();
	int cc = cursorcolortable[cursorflash];
	int maxlen = MaxOrderLen();

	// Renoise-style fixed cursor row: the cursor row (eseditpos) is pinned to
	// a fixed vertical position and the list scrolls under it. scrollPos is
	// always derived from eseditpos; rows above 0 / past the list end render
	// blank. Bar position follows the pattern view's setting — upper-third
	// (default) or centre, toggled in GT2 settings. While a drag-select is
	// in progress the scroll is frozen so the row under the mouse stays
	// stable.
	extern int gt2PatternCursorCentered;
	int centerRow = gt2PatternCursorCentered ? (visibleRows / 2) : (visibleRows / 3);
	if (selecting)
		scrollPos = dragScrollPos;
	else
		scrollPos = eseditpos - centerRow;

	char tb[64];

	sprintf(tb, "PATTERN LIST (SUBTUNE %02X)", esnum);
	DrawTextGT2(dl, fontAtlas,
				origin.x + GT2ColToPixel(0), origin.y + GT2RowToPixel(0),
				CTITLE, tb);

	for (int d = 0; d < visibleRows; d++)
	{
		int p = scrollPos + d;
		float rowY = origin.y + GT2RowToPixel(1 + d);
		if (p < 0 || p > maxlen)
			continue;

		// --- Block index column (editable) ---
		int e0 = songorder[esnum][0][p];
		bool plain0 = (p < songlen[esnum][0]) && (e0 < REPEAT);
		char blockStr[4];
		if (plain0) sprintf(blockStr, "%02X", e0 / numCh);
		else        sprintf(blockStr, "--");

		u8 blockFg = (p == eseditpos) ? (u8)CEDIT : (u8)CNORMAL;
		for (int n = 0; n < 2; n++)
		{
			// Compose the blinking cursor background into the active nibble
			// instead of drawing an opaque rect over the glyph.
			u8 packed = (u8)(blockFg & 0x0F);
			if (p == eseditpos && !eamode && n == editCol)
				packed = (u8)((blockFg & 0x0F) | ((cc & 0x0F) << 4));
			char ch[2] = { blockStr[n], 0 };
			DrawTextGT2(dl, fontAtlas,
						origin.x + GT2ColToPixel(PL_BLOCK_COL + n), rowY, packed, ch);
		}

		DrawTextGT2(dl, fontAtlas,
					origin.x + GT2ColToPixel(PL_SEP_COL), rowY, CCOMMAND, " | ");

		// --- Real per-channel patterns (read-only) ---
		for (int c = 0; c < numCh && c < MAX_CHN; c++)
		{
			int e = songorder[esnum][c][p];
			if (p >= songlen[esnum][c])
				sprintf(tb, "   ");
			else if (e < REPEAT)
				sprintf(tb, "%02X ", e);
			else if (e == LOOPSONG)
				sprintf(tb, "RST");
			else if (e >= TRANSUP)
				sprintf(tb, "+%01X ", e & 0x0f);
			else if (e >= TRANSDOWN)
				sprintf(tb, "-%01X ", 16 - (e & 0x0f));
			else
				sprintf(tb, "R%01X ", (e + 1) & 0x0f);
			DrawTextGT2(dl, fontAtlas,
						origin.x + GT2ColToPixel(PL_CHAN_COL + c*3), rowY, CNORMAL, tb);
		}
	}

	// Dim the real-pattern columns so they read as read-only ("greyer").
	dl->AddRectFilled(
		ImVec2(origin.x + GT2ColToPixel(PL_CHAN_COL), origin.y + GT2RowToPixel(1)),
		ImVec2(origin.x + GT2ColToPixel(PL_CHAN_COL + numCh*3),
			   origin.y + GT2RowToPixel(1 + visibleRows)),
		IM_COL32(0, 0, 0, 115));

	// Highlight the mouse-selected row range.
	if (selAnchor >= 0)
	{
		int selLo = (selAnchor < selEnd) ? selAnchor : selEnd;
		int selHi = (selAnchor < selEnd) ? selEnd : selAnchor;
		for (int d = 0; d < visibleRows; d++)
		{
			int p = scrollPos + d;
			if (p < selLo || p > selHi)
				continue;
			float rowY = origin.y + GT2RowToPixel(1 + d);
			dl->AddRectFilled(
				ImVec2(origin.x, rowY),
				ImVec2(origin.x + windowW, rowY + GT2CellH()),
				IM_COL32(80, 120, 200, 80));
		}
	}

	// Renoise-style centre bar: a translucent strip marking the current row.
	{
		float barY = origin.y + GT2RowToPixel(1 + centerRow);
		dl->AddRectFilled(
			ImVec2(origin.x, barY),
			ImVec2(origin.x + windowW, barY + GT2CellH()),
			IM_COL32(120, 150, 235, 55));
	}

	// Mouse — click moves the edit cursor, drag selects a contiguous row range.
	// The scroll is frozen during a drag so the row under the mouse is stable;
	// dragging past the top/bottom edge auto-scrolls.
	if ((ImGui::IsWindowHovered() || selecting) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		int gridRow = GT2PixelToRow(mousePos.y - origin.y);

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && gridRow >= 1)
		{
			int p = scrollPos + (gridRow - 1);
			if (p < 0) p = 0;
			if (p > maxlen) p = maxlen;
			eseditpos = p;
			editCol = 0;
			editmode = EDIT_ORDERLIST;
			selAnchor = p;
			selEnd = p;
			selecting = true;
			dragScrollPos = scrollPos;
			SyncPatternEditorToCursor();
		}
		else if (selecting)
		{
			if (mousePos.y < origin.y + GT2RowToPixel(1) && dragScrollPos > 0)
				dragScrollPos--;
			else if (mousePos.y > origin.y + GT2RowToPixel(1 + visibleRows)
					 && dragScrollPos + visibleRows <= maxlen)
				dragScrollPos++;

			int p = dragScrollPos + (gridRow - 1);
			if (p < 0) p = 0;
			if (p > maxlen) p = maxlen;
			selEnd = p;
			eseditpos = p;
			editCol = 0;
			SyncPatternEditorToCursor();
		}
	}
	if (selecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		selecting = false;
		if (selAnchor == selEnd)   // a plain click, not a drag — no range
		{
			selAnchor = -1;
			selEnd = -1;
		}
	}

	// Right-click moves the cursor to the clicked row (unless it lands inside
	// an existing selection) so the context menu acts on what was clicked.
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		int gridRow = GT2PixelToRow(ImGui::GetIO().MousePos.y - origin.y);
		if (gridRow >= 1)
		{
			int p = scrollPos + (gridRow - 1);
			if (p < 0) p = 0;
			if (p > maxlen) p = maxlen;
			int selLo = (selAnchor < selEnd) ? selAnchor : selEnd;
			int selHi = (selAnchor < selEnd) ? selEnd : selAnchor;
			if (selAnchor < 0 || p < selLo || p > selHi)
			{
				eseditpos = p;
				editCol = 0;
				editmode = EDIT_ORDERLIST;
				ClearSelection();
				SyncPatternEditorToCursor();
			}
		}
	}

	// Right-click context menu.
	if (ImGui::BeginPopupContextWindow("GT2PatternListCtx"))
	{
		CGT2RenoiseInput *ri = pluginGoatTracker ? pluginGoatTracker->renoiseInput : NULL;
		bool hasSel = (selAnchor >= 0);
		if (ImGui::MenuItem("Cut", "Ctrl+X"))   CutSelection();
		if (ImGui::MenuItem("Copy", "Ctrl+C"))  CopySelection();
		if (ImGui::MenuItem("Paste", "Ctrl+V", false, s_clipboardLen > 0))
			PasteRows();
		ImGui::Separator();
		if (ImGui::MenuItem("Insert", "Insert"))  { if (ri) { ri->HandleInsertPattern(); ClearSelection(); } }
		if (ImGui::MenuItem("Delete"))  { if (ri) { ri->HandleDeletePattern(); ClearSelection(); } }
		if (ImGui::MenuItem(hasSel ? "Duplicate selection" : "Duplicate", "Ctrl+D"))
			DuplicateSelection();
		ImGui::Separator();
		if (ImGui::MenuItem("Remove unused")) RemoveUnusedPatterns();
		if (ImGui::MenuItem("Sort"))          SortPatterns();
		ImGui::EndPopup();
	}

	PostRenderImGui();
}

bool CViewGT2PatternList::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	bool noMods = !isShift && !isAlt && !isControl && !isSuper;
	bool ctrlOnly = (isControl || isSuper) && !isShift && !isAlt;
	if (noMods)
	{
		int hex = GT2PatternList_HexFromKey(keyCode);
		if (hex >= 0)                     { EditBlockNibble(hex); return true; }
		if (keyCode == MTKEY_ARROW_UP)    { MoveCursor(-1);       return true; }
		if (keyCode == MTKEY_ARROW_DOWN)  { MoveCursor(1);        return true; }
		if (keyCode == MTKEY_INSERT)
		{
			if (pluginGoatTracker && pluginGoatTracker->renoiseInput)
			{
				pluginGoatTracker->renoiseInput->HandleInsertPattern();
				ClearSelection();
			}
			return true;
		}
	}
	if (ctrlOnly)
	{
		if (keyCode == 'x' || keyCode == 'X' || keyCode == SDLK_X) { CutSelection();       return true; }
		if (keyCode == 'c' || keyCode == 'C' || keyCode == SDLK_C) { CopySelection();      return true; }
		if (keyCode == 'v' || keyCode == 'V' || keyCode == SDLK_V) { PasteRows();          return true; }
		if (keyCode == 'd' || keyCode == 'D' || keyCode == SDLK_D) { DuplicateSelection(); return true; }
		// Ctrl+Left/Right step the block index (each step shifts every
		// channel's pattern by numChannels).
		if (keyCode == MTKEY_ARROW_LEFT)  { AdjustBlock(-1);        return true; }
		if (keyCode == MTKEY_ARROW_RIGHT) { AdjustBlock(1);         return true; }
	}
	return GT2_HandleRenoiseOrForwardKeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2PatternList::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2PatternList::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
