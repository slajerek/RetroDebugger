#ifndef _CSourceBuffer_h_
#define _CSourceBuffer_h_

// RD-021: an editable source file of the workspace. The text is kept as one
// UTF-8 string (what the editor widget edits); undo and redo are whole-text
// snapshots, one per edit burst. Save writes a temporary file next to the
// target and renames it over, so a failed write never truncates the file.
// The file on disk is watched by its modification time: an external change
// reloads a clean buffer and marks a dirty one as conflicting.

#include "SYS_Defs.h"
#include <string>
#include <vector>

class CSourceBuffer
{
public:
	explicit CSourceBuffer(const std::string &path);

	std::string path;

	bool Load();						// from disk; false when it cannot be read
	bool Save(std::string *error = NULL);
	const std::string &GetText() const { return text; }
	// the editor changed the text: a new undo step when the last edit is older
	// than kUndoBurstMs (keystrokes in a row are one step)
	void SetText(const std::string &newText, u64 nowMs);
	bool IsDirty() const { return text != savedText; }
	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }
	bool Undo();
	bool Redo();

	// the file watcher's poll: CHANGED when the file on disk is newer than
	// what was loaded or saved. A clean buffer is reloaded; a dirty one keeps
	// its edits and IsConflicting() until saved (overwriting) or reverted.
	enum class DiskState { Unchanged, Reloaded, Conflict, Missing };
	DiskState CheckDisk();
	bool IsConflicting() const { return conflicting; }
	bool Revert();						// the file's text again, the edits gone

	// 1-based line and the byte offset of its start; for search and navigation
	int NumLines() const;
	std::string GetLine(int line) const;
	// the first match of NEEDLE at or after byte FROM (wrapping), or -1
	long Find(const std::string &needle, long from, bool caseSensitive) const;
	int LineOfOffset(long offset) const;

	static const u64 kUndoBurstMs = 700;
	static const size_t kUndoLimit = 200;

private:
	std::string text;
	std::string savedText;
	std::vector<std::string> undoStack;
	std::vector<std::string> redoStack;
	u64 lastEditMs = 0;
	i64 diskTime = 0;
	bool diskExists = false;
	bool conflicting = false;
	bool ReadDiskTime(i64 *time) const;
};

#endif
