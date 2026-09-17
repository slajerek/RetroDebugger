#ifndef _CViewGT2Instrument_H_
#define _CViewGT2Instrument_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CGT2FontAtlas;

int GT2WtblSelectedWaveformIndex(unsigned char leftValue);
int GT2WtblSelectedCommandIndex(unsigned char leftValue);
unsigned char GT2WtblApplyWaveformSelection(unsigned char leftValue, int waveformIndex);
unsigned char GT2WtblApplyCommandSelection(int commandIndex);
bool GT2WtblContextHasValidRow(int tableStart, int tableLength, int position);
int GT2WtblContextRowFromClick(int tableStart, int tableLength, int clickedOffset);
bool GT2WtblContextShouldEnterEditMode(int tableStart, int tableLength, int clickedOffset);
bool GT2WtblContextShouldCreateRowOnSelection(int tableStart, int tableLength, int clickedOffset);
int GT2WtblContextSelectableFlags();
int GT2InstrumentTableFromGridCol(int gridCol);
// Moves the table-edit cursor one column left (direction -1) or right (+1)
// WITHIN the current instrument's own table slices. Native tablecommands()
// wraps across the shared table pool -- it lands the cursor at whatever pool
// offset etview[] happens to point at in the next table, which are rows this
// instrument does not own and which only the GT2 Tables view shows. Here the
// step skips tables the instrument owns no rows in, and keeps the slice-
// relative row (clamped) when it does cross.
// sliceLen[] is MAX_TABLES long; sliceLen[t] == 0 means "no slice".
// inOutRow is relative to the slice start, not a pool position.
// Returns false when the instrument owns no table rows at all.
bool GT2InstrumentTableCursorStep(int direction, const int *sliceLen,
								  int *inOutTable, int *inOutRow, int *inOutColumn);
int GT2WtblContextTextColorMode(bool selected, bool hovered);

class CViewGT2Instrument : public CGuiView
{
public:
	CViewGT2Instrument(const char *name, float posX, float posY, float posZ,
					   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2Instrument();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// A *.ins dragged in from the OS loads into the instrument being edited.
	virtual bool DoDropFile(char *filePath);
	u8 ApplyInstrumentCellBackground(u8 colorIndex, int backgroundColor) const;
	// Routes ENTER on an instrument's table-pointer field (eipos 2..5) into
	// the corresponding ImGui table view — gototable() when the pointer is
	// non-zero, InsertTableRow() / AllocateSpeedtableEntry() when it's zero.
	// Returns true if consumed. Callable from any focused view (legacy
	// text-mode included) so the user doesn't drop into the legacy editor.
	bool HandleInstrumentTablePointerEnter(bool isShift, bool isAlt, bool isControl, bool isSuper);

	CGT2FontAtlas *fontAtlas;

	// Table name + interactive help/edit block for one instrument-table row,
	// and applying whatever is changed there. Public because
	// CViewGT2InstrumentTableRow renders the very same block for the row under
	// the cursor -- one implementation, two places it appears.
	// activeColumn is GT2's etcolumn: 0/1 are the left byte's nybbles, 2/3 the
	// right byte's. The lines describing that byte are drawn highlighted.
	// Pass -1 for no highlight.
	void RenderTableRowHelp(int tableNum, int rowPos, bool hasRow, bool canEdit,
							int activeColumn);

private:
	void InsertTableRow();   // wavetable / pulsetable / filtertable only
	void DeleteTableRow();
	void AllocateSpeedtableEntry();
	void RenderTablePalette();
	bool CreateWavetableRowWithLeft(unsigned char value);
	void SetWavetableLeft(unsigned char value);
	void SetWavetableRight(unsigned char value);
	// One-cell undo writes to the focused filter-table row, used by the
	// right-click context menu's passband / resonance / routing editor.
	void SetFiltertableLeft(unsigned char value);
	void SetFiltertableRight(unsigned char value);
	void SetFirstwave(unsigned char value);
	bool tableContextHasClickedRow;
	bool tableContextCanEdit;
	// First visible row within each table slice of the current instrument.
	// Relative to the slice start, NOT the pool position -- deliberately not
	// the native etview[], which is pool-absolute and (under etlock) shared
	// across all four tables; here every slice starts at a different pool
	// position, so a shared absolute view would show unrelated rows.
	int tableViewOffset[4];
	int lastInstrumentNum;   // resets the offsets when the instrument changes
	// The cursor is followed only when it actually moved, so a mouse-wheel
	// scroll can look further down a long table without the next frame
	// yanking the view back to the row being edited.
	int lastCursorTable;
	int lastCursorPos;
	int lastCursorColumn;
	// Fractional wheel accumulator, so a trackpad's sub-notch deltas scroll
	// smoothly instead of rounding to zero or jumping a whole notch.
	float tableWheelAccum;
	int tableWheelLastTable;
};

#endif
