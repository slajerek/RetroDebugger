#ifndef _CDataAddressEditBoxHex_h_
#define _CDataAddressEditBoxHex_h_

// this is a default class that allows edit and set address used in CViewDataDump
// this is extended by CViewDataAddressEditBoxHex that uses default CGuiEditHex and CViewDataAddressDiskContentEditBox that adds different format

#include "CDataAddressEditBox.h"
#include "CGuiEditHex.h"

class CDataAddressEditBoxHex;

class CDataAddressEditBoxHex : public CDataAddressEditBox, CGuiEditHexCallback
{
public:
	CDataAddressEditBoxHex(int numDigits);
	virtual ~CDataAddressEditBoxHex();
	virtual void Render(float posX, float posY, CSlrFont *font, float fontSize);
	
	virtual void SetValue(int value);
	virtual int GetValue();
	virtual void SetNumDigits(int numDigits);
	
	virtual bool KeyDown(u32 keyCode);
	virtual void FinalizeEntering(u32 keyCode, bool isCancelled);
	virtual void CancelEntering();
	virtual void SetCursorPos(int newPos);

	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
};

#endif
