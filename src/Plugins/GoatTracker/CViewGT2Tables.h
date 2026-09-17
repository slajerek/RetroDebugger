#ifndef _CViewGT2Tables_H_
#define _CViewGT2Tables_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGT2UndoHistory.h"
#include <vector>

class CGT2FontAtlas;

class CViewGT2Tables : public CGuiView
{
public:
	CViewGT2Tables(const char *name, float posX, float posY, float posZ,
				   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2Tables();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	bool CanUndoTableEdit() const;
	bool CanRedoTableEdit() const;
	bool UndoTableEdit();
	bool RedoTableEdit();
	void ClearTableUndoHistory();
	void BeginTableUndoStep();
	bool CommitTableUndoStep();
	void CancelTableUndoStep();
	u8 ApplyTableCellBackground(u8 colorIndex, int backgroundColor) const;

	CGT2FontAtlas *fontAtlas;

private:
	// One timeline for the whole editor: a table edit, a pattern edit and an
	// instrument load all record the same snapshot type on the same stack, so
	// Ctrl+Z takes back the last change wherever it was made. See
	// CGT2UndoHistory.h.
	typedef CGT2UndoSnapshot TableUndoSnapshot;

	TableUndoSnapshot CaptureTableUndoSnapshot() const;
	bool CommitTableUndoSnapshotIfChanged(const TableUndoSnapshot &before);

	// The cursor is followed only when it actually moved, so a mouse-wheel
	// scroll can look elsewhere in the pool without the next frame yanking
	// the view back to the row being edited.
	int lastCursorTable;
	int lastCursorPos;
	int lastCursorColumn;
	// Fractional wheel accumulator, so a trackpad's sub-notch deltas scroll
	// smoothly instead of rounding to zero or jumping a whole notch.
	float tableWheelAccum;
	int tableWheelLastTable;

	TableUndoSnapshot pendingTableUndoSnapshot;
	bool pendingTableUndoSnapshotActive;
	// Begin/Commit nest: loadinstrument() opens its own step, and it can be
	// reached from docommand() which already opened one (EDIT_INSTRUMENT, or
	// EDIT_TABLES via load()). Only the outermost commit turns the pending
	// snapshot into an undo entry.
	int pendingTableUndoDepth;
};

#endif
