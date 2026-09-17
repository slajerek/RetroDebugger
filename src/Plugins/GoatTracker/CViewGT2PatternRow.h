#ifndef _CViewGT2PatternRow_H_
#define _CViewGT2PatternRow_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CGT2FontAtlas;

// The effect on the pattern row the cursor is on, as a view of its own.
//
// It renders exactly the **Edit Effect** block the pattern's right-click
// context menu shows when the click lands on a command column -- the command
// picker and the argument editor for whatever command is selected -- through
// the same CViewGT2Patterns::RenderPatternCommandEditor(), so there is one
// implementation and the two cannot drift apart.
//
// Only that block. Selection / Track / Pattern / Copy Effects and the rest of
// the context menu stay in the context menu: they are actions on a click
// target, not a reading of the row under the cursor, and a permanently visible
// view of them would just be a second main menu.
class CViewGT2PatternRow : public CGuiView
{
public:
	CViewGT2PatternRow(const char *name, float posX, float posY, float posZ,
					   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2PatternRow();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	CGT2FontAtlas *fontAtlas;

	// Whether the cursor is on a pattern row this view can edit: inside the
	// channel's pattern and not the ENDPATT marker. Static so a test can check
	// the rule without a rendered frame.
	static bool CursorRowIsEditable();
};

#endif
