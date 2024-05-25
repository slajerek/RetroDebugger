#ifndef _CDataAddressEditBox_h_
#define _CDataAddressEditBox_h_

// this is a default class that allows edit and set address used in CViewDataDump
// this is extended by CViewDataAddressEditBoxHex that uses default CGuiEditHex and CViewDataAddressDiskContentEditBox that adds different format

#include "CGuiEditHex.h"

class CDataAddressEditBox;

class CDataAddressEditBoxCallback
{
public:
	virtual void DataAddressEditBoxEnteredValue(CDataAddressEditBox *editBox, u32 lastKeyCode, bool isCancelled) {};
};

class CDataAddressEditBox
{
public:
	CDataAddressEditBox(int numDigits);
	virtual ~CDataAddressEditBox();
	virtual void SetCallback(CDataAddressEditBoxCallback *callback);
	
	virtual void Render(float posX, float posY, CSlrFont *font, float fontSize);
	
	int numDigits;
	
	virtual void SetValue(int value);
	virtual int GetValue();
	
	virtual void SetNumDigits(int numDigits);
	virtual int GetNumDigits();
	
	virtual bool KeyDown(u32 keyCode);
	virtual void FinalizeEntering(u32 keyCode, bool isCancelled);
	virtual void CancelEntering();
	virtual void SetCursorPos(int newPos);
	
	CDataAddressEditBoxCallback *callback;
};

#endif
