#ifndef _CViewGT2InstrumentTableRow_H_
#define _CViewGT2InstrumentTableRow_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CGT2FontAtlas;

// The instrument-table row the cursor is on, as a view of its own.
//
// It renders exactly what the GT2 Instrument right-click context menu renders
// below its Insert / Delete items -- the table name and the interactive help
// block for that row -- through the same
// CViewGT2Instrument::RenderTableRowHelp(), so there is one implementation and
// the two cannot drift apart. The difference is only where the row comes from:
// the menu uses the row that was right-clicked, this view follows the live
// cursor (etnum / etpos) and updates as you move around the tables.
class CViewGT2InstrumentTableRow : public CGuiView
{
public:
	CViewGT2InstrumentTableRow(const char *name, float posX, float posY, float posZ,
							   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2InstrumentTableRow();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	CGT2FontAtlas *fontAtlas;

	// Whether the cursor is currently on a real row of the edited instrument's
	// slice of table `tableNum`. Static so a test can check the rule without a
	// rendered frame.
	static bool CursorHasRow(int tableNum, int rowPos);
	// Whether that row may be edited: the speedtable's rows are atomic, so it
	// is never editable here.
	static bool CursorCanEdit(int tableNum, int rowPos);
};

#endif
