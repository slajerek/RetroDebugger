#ifndef _CDataAddressEditBoxDiskContents_h_
#define _CDataAddressEditBoxDiskContents_h_

#include "CDataAddressEditBox.h"

class CDataAdapterViceDrive1541DiskContents;
class CDataAddressEditBoxHex;

#define DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES	3
class CDataAddressEditBoxDiskContents : public CDataAddressEditBox, CGuiEditHexCallback
{
public:
	CDataAddressEditBoxDiskContents(CDataAdapterViceDrive1541DiskContents *dataAdapter);
	virtual ~CDataAddressEditBoxDiskContents();
	
	CDataAdapterViceDrive1541DiskContents *dataAdapter;
	
	// address format: xx.yy.zz   (track.sector.byte in sector)
	CGuiEditHex *editHexBoxes[DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES];

	virtual void SetValue(int value);
	virtual int GetValue();

	virtual void Render(float posX, float posY, CSlrFont *font, float fontSize);

	int cursorPos;
	virtual void SetCursorPos(int newPos);
	virtual CGuiEditHex *GetHexBoxFromCursorPos(int cursorPos);
	virtual int GetHexBoxIndexFromCursorPos(int cursorPos);

	virtual bool KeyDown(u32 keyCode);
	virtual void FinalizeEditing(u32 keyCode);
	virtual void CancelEntering();

	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
};

#endif
//public:
//	CDataAddressEditBox(int numDigits);
//	virtual void SetCallback(CDataAddressEditBoxCallback *callback);
//	
//	virtual void Render(float posX, float posY, CSlrFont *font, float fontSize);
//	
//	int numDigits;
//	
//	virtual void SetValue(int value);
//	virtual int GetValue();
//	
//	virtual void SetNumDigits(int numDigits);
//	virtual int GetNumDigits();
//	
//	virtual bool KeyDown(u32 keyCode);
//	virtual void FinalizeEntering(u32 keyCode, bool isCancelled);
//	virtual void CancelEntering();
//	virtual void SetCursorPos(int newPos);
//	
//	CDataAddressEditBoxCallback *callback;
