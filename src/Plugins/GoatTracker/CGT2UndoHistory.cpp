#include "CGT2UndoHistory.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewGT2Patterns.h"
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsong.h"
#include "gtable.h"

extern unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
extern unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
extern int pattlen[MAX_PATT];
extern int epnum[MAX_CHN];
extern int eppos, epview, epcolumn, epchn;
}

static const size_t kGT2UndoLimit = 32;

// GT2's name buffers are MAX_STR bytes including the terminator, and the rest
// of the editor reads them with strlen(). Restoring one has to leave it
// NUL-terminated even if the snapshot somehow carries a longer string.
static void GT2CopySongString(char *dest, const std::string &value)
{
	size_t length = value.size();
	if (length > MAX_STR - 1)
		length = MAX_STR - 1;
	memcpy(dest, value.c_str(), length);
	dest[length] = 0;
}

CGT2UndoSnapshot::CGT2UndoSnapshot()
: cursorRow(0)
, cursorView(0)
, cursorColumn(0)
, cursorChannel(0)
, cursorArpColumn(-1)
, selectionActive(false)
, selectionStartTrack(0)
, selectionStartRow(0)
, selectionEndTrack(0)
, selectionEndRow(0)
, selectionStartFineField(0)
, selectionEndFineField(0)
, selectionFineMode(false)
, tableNum(0)
, tablePos(0)
, tableColumn(0)
, tableLock(1)
, tableMarkNum(-1)
, tableMarkStart(0)
, tableMarkEnd(0)
{
}

CGT2UndoHistory::CGT2UndoHistory()
{
}

CGT2UndoHistory *GT2UndoHistory()
{
	static CGT2UndoHistory history;
	return &history;
}

CGT2UndoSnapshot CGT2UndoHistory::Capture() const
{
	CGT2UndoSnapshot snapshot;

	const u8 *patternBegin = &pattern[0][0];
	const u8 *arpBegin = &arpdata[0][0][0][0];
	const u8 *songOrderBegin = &songorder[0][0][0];
	const u8 *leftTableBegin = &ltable[0][0];
	const u8 *rightTableBegin = &rtable[0][0];
	const u8 *instrumentBegin = reinterpret_cast<const u8 *>(&ginstr[0]);

	snapshot.patternData.assign(patternBegin, patternBegin + sizeof(pattern));
	snapshot.arpData.assign(arpBegin, arpBegin + sizeof(arpdata));
	for (int c = 0; c < MAX_CHN; c++)
		for (int a = 0; a < MAX_ARP_COLS; a++)
			snapshot.arpColumnNotes.push_back(chn[c].arpcolnotes[a]);
	snapshot.patternLengths.assign(pattlen, pattlen + MAX_PATT);
	snapshot.patternNumbers.assign(epnum, epnum + MAX_CHN);
	snapshot.songOrderData.assign(songOrderBegin, songOrderBegin + sizeof(songorder));
	snapshot.songLengths.assign(&songlen[0][0], &songlen[0][0] + MAX_SONGS * MAX_CHN);
	snapshot.leftTableData.assign(leftTableBegin, leftTableBegin + sizeof(ltable));
	snapshot.rightTableData.assign(rightTableBegin, rightTableBegin + sizeof(rtable));
	snapshot.instrumentData.assign(instrumentBegin, instrumentBegin + sizeof(ginstr));

	snapshot.songName = songname;
	snapshot.authorName = authorname;
	snapshot.copyrightName = copyrightname;

	snapshot.cursorRow = eppos;
	snapshot.cursorView = epview;
	snapshot.cursorColumn = epcolumn;
	snapshot.cursorChannel = epchn;

	snapshot.tableViews.assign(etview, etview + MAX_TABLES);
	snapshot.tableNum = etnum;
	snapshot.tablePos = etpos;
	snapshot.tableColumn = etcolumn;
	snapshot.tableLock = etlock;
	snapshot.tableMarkNum = etmarknum;
	snapshot.tableMarkStart = etmarkstart;
	snapshot.tableMarkEnd = etmarkend;

	// The arp column and the pattern selection live in the view, not in GT2's
	// globals, so the view fills them in.
	if (pluginGoatTracker != NULL && pluginGoatTracker->viewPatterns != NULL)
		pluginGoatTracker->viewPatterns->CaptureUndoViewState(&snapshot);

	return snapshot;
}

void CGT2UndoHistory::Restore(const CGT2UndoSnapshot &snapshot)
{
	if (snapshot.patternData.size() == sizeof(pattern))
		memcpy(&pattern[0][0], snapshot.patternData.data(), sizeof(pattern));
	if (snapshot.arpData.size() == sizeof(arpdata))
		memcpy(&arpdata[0][0][0][0], snapshot.arpData.data(), sizeof(arpdata));
	if (snapshot.arpColumnNotes.size() == MAX_CHN * MAX_ARP_COLS)
	{
		int noteIndex = 0;
		for (int c = 0; c < MAX_CHN; c++)
			for (int a = 0; a < MAX_ARP_COLS; a++)
				chn[c].arpcolnotes[a] = snapshot.arpColumnNotes[noteIndex++];
	}
	if (snapshot.patternLengths.size() == MAX_PATT)
		memcpy(pattlen, snapshot.patternLengths.data(), sizeof(int) * MAX_PATT);
	if (snapshot.patternNumbers.size() == MAX_CHN)
		memcpy(epnum, snapshot.patternNumbers.data(), sizeof(int) * MAX_CHN);
	if (snapshot.songOrderData.size() == sizeof(songorder))
		memcpy(&songorder[0][0][0], snapshot.songOrderData.data(), sizeof(songorder));
	if (snapshot.songLengths.size() == MAX_SONGS * MAX_CHN)
		memcpy(&songlen[0][0], snapshot.songLengths.data(), sizeof(int) * MAX_SONGS * MAX_CHN);
	if (snapshot.leftTableData.size() == sizeof(ltable))
		memcpy(&ltable[0][0], snapshot.leftTableData.data(), sizeof(ltable));
	if (snapshot.rightTableData.size() == sizeof(rtable))
		memcpy(&rtable[0][0], snapshot.rightTableData.data(), sizeof(rtable));
	if (snapshot.instrumentData.size() == sizeof(ginstr))
		memcpy(&ginstr[0], snapshot.instrumentData.data(), sizeof(ginstr));

	GT2CopySongString(songname, snapshot.songName);
	GT2CopySongString(authorname, snapshot.authorName);
	GT2CopySongString(copyrightname, snapshot.copyrightName);

	eppos = snapshot.cursorRow;
	epview = snapshot.cursorView;
	epcolumn = snapshot.cursorColumn;
	epchn = snapshot.cursorChannel;

	if (snapshot.tableViews.size() == MAX_TABLES)
		memcpy(etview, snapshot.tableViews.data(), sizeof(int) * MAX_TABLES);
	etnum = snapshot.tableNum;
	etpos = snapshot.tablePos;
	etcolumn = snapshot.tableColumn;
	etlock = snapshot.tableLock;
	etmarknum = snapshot.tableMarkNum;
	etmarkstart = snapshot.tableMarkStart;
	etmarkend = snapshot.tableMarkEnd;

	if (pluginGoatTracker != NULL && pluginGoatTracker->viewPatterns != NULL)
		pluginGoatTracker->viewPatterns->RestoreUndoViewState(snapshot);
}

bool CGT2UndoHistory::HaveSameData(const CGT2UndoSnapshot &a, const CGT2UndoSnapshot &b)
{
	return a.patternData == b.patternData
		&& a.arpData == b.arpData
		&& a.patternLengths == b.patternLengths
		&& a.patternNumbers == b.patternNumbers
		&& a.songOrderData == b.songOrderData
		&& a.songLengths == b.songLengths
		&& a.leftTableData == b.leftTableData
		&& a.rightTableData == b.rightTableData
		&& a.instrumentData == b.instrumentData
		&& a.songName == b.songName
		&& a.authorName == b.authorName
		&& a.copyrightName == b.copyrightName;
}

void CGT2UndoHistory::Push(const CGT2UndoSnapshot &snapshot)
{
	undoStack.push_back(snapshot);
	if (undoStack.size() > kGT2UndoLimit)
		undoStack.erase(undoStack.begin());
	redoStack.clear();
}

bool CGT2UndoHistory::CommitIfChanged(const CGT2UndoSnapshot &before)
{
	CGT2UndoSnapshot after = Capture();
	if (HaveSameData(before, after))
		return false;
	Push(before);
	return true;
}

bool CGT2UndoHistory::CanUndo() const
{
	return !undoStack.empty();
}

bool CGT2UndoHistory::CanRedo() const
{
	return !redoStack.empty();
}

bool CGT2UndoHistory::Undo()
{
	if (undoStack.empty())
		return false;
	CGT2UndoSnapshot current = Capture();
	CGT2UndoSnapshot previous = undoStack.back();
	undoStack.pop_back();
	redoStack.push_back(current);
	if (redoStack.size() > kGT2UndoLimit)
		redoStack.erase(redoStack.begin());
	Restore(previous);
	return true;
}

bool CGT2UndoHistory::Redo()
{
	if (redoStack.empty())
		return false;
	CGT2UndoSnapshot current = Capture();
	CGT2UndoSnapshot next = redoStack.back();
	redoStack.pop_back();
	undoStack.push_back(current);
	if (undoStack.size() > kGT2UndoLimit)
		undoStack.erase(undoStack.begin());
	Restore(next);
	return true;
}

void CGT2UndoHistory::Clear()
{
	undoStack.clear();
	redoStack.clear();
}
