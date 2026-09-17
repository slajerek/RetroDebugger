#ifndef _CGT2UndoHistory_H_
#define _CGT2UndoHistory_H_

#include "SYS_Defs.h"
#include <string>
#include <vector>

// ONE undo timeline for the whole GT2 editor.
//
// Pattern edits, table edits and instrument loads all rewrite overlapping
// blocks of the same globals: pattern[][] carries table pointers, an
// instrument load reshuffles the shared ltable/rtable pool and renumbers those
// pointers, a table insert renumbers them too. Two separate histories
// therefore could not coexist -- whichever was written last had to wipe the
// other, or an undo from the stale one would restore a snapshot that no longer
// agreed with the pool. That is what the editor used to do, and it is why
// Ctrl+Z in the pattern editor could not take back an instrument load: the
// load was recorded on the table history, it had wiped the pattern history on
// its way in, and Ctrl+Z picked a history by editmode, so the entry that held
// the previous instrument was unreachable from the pattern editor.
//
// So there is one stack, holding one snapshot type that covers every block any
// GT2 edit can touch plus the cursor state of both editors. Ctrl+Z anywhere
// steps back through the changes in the order they were made, whatever kind
// each one was.
//
// The views keep their own pending-step bookkeeping (CViewGT2Patterns holds a
// single pending snapshot, CViewGT2Tables counts nesting depth because an
// instrument load opens a step inside a step) -- only the recorded history is
// shared.
struct CGT2UndoSnapshot
{
	CGT2UndoSnapshot();

	// Song data: the union of what a pattern edit, a table edit and an
	// instrument load can rewrite.
	std::vector<u8> patternData;
	std::vector<u8> arpData;
	std::vector<u8> arpColumnNotes;
	std::vector<int> patternLengths;
	std::vector<int> patternNumbers;
	std::vector<u8> songOrderData;
	std::vector<int> songLengths;
	std::vector<u8> leftTableData;    // ltable[][] -- shared wave/pulse/filter/speed pool
	std::vector<u8> rightTableData;   // rtable[][]
	std::vector<u8> instrumentData;   // ginstr[]

	// The three strings savesong() writes alongside the music data. They are
	// song data like everything above, so Ctrl+Z takes back a rename the same
	// way it takes back a note.
	std::string songName;
	std::string authorName;
	std::string copyrightName;

	// Pattern editor cursor and selection.
	int cursorRow;
	int cursorView;
	int cursorColumn;
	int cursorChannel;
	int cursorArpColumn;
	bool selectionActive;
	int selectionStartTrack;
	int selectionStartRow;
	int selectionEndTrack;
	int selectionEndRow;
	int selectionStartFineField;
	int selectionEndFineField;
	bool selectionFineMode;

	// Table editor cursor and mark.
	std::vector<int> tableViews;
	int tableNum;
	int tablePos;
	int tableColumn;
	int tableLock;
	int tableMarkNum;
	int tableMarkStart;
	int tableMarkEnd;
};

class CGT2UndoHistory
{
public:
	CGT2UndoHistory();

	CGT2UndoSnapshot Capture() const;
	void Restore(const CGT2UndoSnapshot &snapshot);
	// Cursor and selection ride along with the data, but do not make history
	// by themselves -- navigating is not an undoable edit.
	static bool HaveSameData(const CGT2UndoSnapshot &a, const CGT2UndoSnapshot &b);

	// Records `before` as an entry unless nothing actually changed.
	bool CommitIfChanged(const CGT2UndoSnapshot &before);

	bool CanUndo() const;
	bool CanRedo() const;
	bool Undo();
	bool Redo();
	void Clear();

private:
	void Push(const CGT2UndoSnapshot &snapshot);

	std::vector<CGT2UndoSnapshot> undoStack;
	std::vector<CGT2UndoSnapshot> redoStack;
};

// The editor's state is global, so its history is too.
CGT2UndoHistory *GT2UndoHistory();

#endif
