#include "CDataAddressEditBoxDiskContents.h"
#include "CGuiEditHex.h"
#include "CDataAdapterViceDrive1541DiskContents.h"
#include "SYS_KeyCodes.h"

CDataAddressEditBoxDiskContents::CDataAddressEditBoxDiskContents(CDataAdapterViceDrive1541DiskContents *dataAdapter)
						// xx.yy.zz = 8 digits
						//  3 hex boxes                       *2 digits + 2 dots
: CDataAddressEditBox(DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES*2        + (DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES-1))
{
	this->dataAdapter = dataAdapter;
	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES; i++)
	{
		editHexBoxes[i] = new CGuiEditHex(this);
		editHexBoxes[i]->isCapitalLetters = false;
	}
	
	cursorPos = -1;
}

void CDataAddressEditBoxDiskContents::Render(float posX, float posY, CSlrFont *font, float fontSize)
{
	float px = posX;

	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES-1; i++)
	{
		// a hex box and a dot
		font->BlitTextColor(editHexBoxes[i]->textWithCursor, px, posY, -1, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
		px += 2.0f * fontSize;
		font->BlitTextColor(".", px, posY, -1, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
		px += fontSize;
	}

	// last hex box
	font->BlitTextColor(editHexBoxes[DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES-1]->textWithCursor, px, posY, -1, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
}

bool CDataAddressEditBoxDiskContents::KeyDown(u32 keyCode)
{
	if (keyCode == MTKEY_ENTER)
	{
		FinalizeEditing(keyCode);
		return true;
	}
	
	CGuiEditHex *hexBox = GetHexBoxFromCursorPos(cursorPos);
	int hexBoxIndex = GetHexBoxIndexFromCursorPos(cursorPos);
	
	if (hexBox)
	{
		hexBox->KeyDown(keyCode);
		
//		if (keyCode == MTKEY_HOME)
//		{
//			
//		}
		
		// xx.xx.xx
		// 01234567
		
		if (keyCode != MTKEY_BACKSPACE)
		{
			cursorPos++;
		}
		else
		{
			cursorPos--;
			if (cursorPos == -1)
				cursorPos = 0;
	
			if (cursorPos % 3 == 2)
				cursorPos--;
			
			SetCursorPos(cursorPos);
			return true;
		}
	}
	
	int newHexBoxIndex = GetHexBoxIndexFromCursorPos(cursorPos);
	
	if (newHexBoxIndex == -1)
	{
		FinalizeEditing(keyCode);
		return true;
	}
	
	if (newHexBoxIndex != hexBoxIndex)
	{
		// switch to next
		editHexBoxes[hexBoxIndex]->SetCursorPos(-1);
		editHexBoxes[newHexBoxIndex]->SetCursorPos(0);
		cursorPos = newHexBoxIndex*3;
	}
	
	return true;
}

void CDataAddressEditBoxDiskContents::FinalizeEditing(u32 keyCode)
{
	// finished entering
	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES; i++)
	{
		editHexBoxes[i]->SetCursorPos(-1);
	}
	
	if (callback)
	{
		callback->DataAddressEditBoxEnteredValue(this, keyCode, false);
	}

}

void CDataAddressEditBoxDiskContents::SetValue(int value)
{
	Drive1541ContentsIndex *index = dataAdapter->GetTrackAndSectorForDataAddress(value);
	
	editHexBoxes[0]->SetValue(index->track+1, 2);
	editHexBoxes[1]->SetValue(index->sector, 2);
	editHexBoxes[2]->SetValue(index->sectorDataOffset, 2);
	
	// reset typing
	editHexBoxes[0]->SetCursorPos(0);
	editHexBoxes[1]->SetCursorPos(-1);
	editHexBoxes[2]->SetCursorPos(-1);

	cursorPos = 0;
}

int CDataAddressEditBoxDiskContents::GetValue()
{
	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES; i++)
	{
		editHexBoxes[i]->UpdateValue();
	}

	int track = editHexBoxes[0]->value-1;
	int sector = editHexBoxes[1]->value;
	int offset = editHexBoxes[2]->value;
	
	int value = dataAdapter->GetDataAddressForTrackAndSector(track, sector) + offset;

	return value;
}

void CDataAddressEditBoxDiskContents::SetCursorPos(int newPos)
{
	this->cursorPos = newPos;
	
	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES; i++)
	{
		editHexBoxes[i]->SetCursorPos(-1);
	}
	
	int index = GetHexBoxIndexFromCursorPos(cursorPos);
	editHexBoxes[index]->SetCursorPos(cursorPos % 3);
}

int CDataAddressEditBoxDiskContents::GetHexBoxIndexFromCursorPos(int cursorPos)
{
	if (cursorPos < 0 || cursorPos >= this->numDigits)
		return -1;
	
	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES; i++)
	{
		// 00.11.22
		// . || left digit || right digit
		if (cursorPos == (i*3-1) || cursorPos == (i*3) || cursorPos == (i*3+1))
		{
			return i;
		}
	}
	
	return -1;
}

CGuiEditHex *CDataAddressEditBoxDiskContents::GetHexBoxFromCursorPos(int cursorPos)
{
	if (cursorPos < 0)
		return NULL;
	
	for (int i = 0; i < DATA_ADDRESS_DISK_CONTENTS_NUM_HEX_BOXES; i++)
	{
		// 00.11.22
		// . || left digit || right digit
		if (cursorPos == (i*3-1) || cursorPos == (i*3) || cursorPos == (i*3+1))
		{
			return editHexBoxes[i];
		}
	}
	return NULL;
}

void CDataAddressEditBoxDiskContents::CancelEntering()
{
	CGuiEditHex *editHex = GetHexBoxFromCursorPos(this->cursorPos);
	if (editHex)
		editHex->CancelEntering();
}

void CDataAddressEditBoxDiskContents::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
//	LOGD("CDataAddressEditBoxDiskContents::GuiEditHexEnteredValue");
	// we do not need to do anything
}

CDataAddressEditBoxDiskContents::~CDataAddressEditBoxDiskContents()
{
	
}

