#ifndef _CViewGT2InstrumentList_H_
#define _CViewGT2InstrumentList_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CGT2FontAtlas;

struct GT2InstrumentListRect
{
	float x1;
	float y1;
	float x2;
	float y2;
};

class CViewGT2InstrumentList : public CGuiView
{
public:
	CViewGT2InstrumentList(const char *name, float posX, float posY, float posZ,
						   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2InstrumentList();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// A *.ins dragged in from the OS lands in the slot under the cursor.
	virtual bool DoDropFile(char *filePath);
	void SelectInstrument(int instrumentNum);
	static int GetInstrumentGridRow(int instrumentNum);
	static GT2InstrumentListRect GetInstrumentRowBackgroundRect(float originX, float originY, int row);
	static GT2InstrumentListRect GetInstrumentListFrameRect(float originX, float originY);

	CGT2FontAtlas *fontAtlas;

	// Origin of the last drawn frame. DoDropFile() runs outside the render
	// pass and still has to map the cursor to a row.
	float lastRenderOriginX;
	float lastRenderOriginY;

	// Last instrument the view scrolled to, so a selection made with the
	// keyboard is brought into view without fighting the mouse wheel.
	int lastScrolledToInstrument;

	// Inline rename state.
	bool renaming;
	bool renameFocusPending;      // true on the frame rename starts, drives SetKeyboardFocusHere
	char renameBuffer[17];        // MAX_INSTRNAMELEN + 1

private:
	void RenderContextMenu();
	void BeginRename();
	void CommitRename();
};

#endif
