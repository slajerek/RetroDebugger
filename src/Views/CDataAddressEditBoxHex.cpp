#include "CDataAddressEditBoxHex.h"


CDataAddressEditBoxHex::CDataAddressEditBoxHex(int numDigits)
: CDataAddressEditBox(numDigits)
{
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editHex->SetValue(0, numDigits);
}


void CDataAddressEditBoxHex::Render(float posX, float posY, CSlrFont *font, float fontSize)
{
	font->BlitTextColor(editHex->textWithCursor, posX, posY, -1, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
}

void CDataAddressEditBoxHex::SetValue(int value)
{
	editHex->SetValue(value, numDigits);
}

int CDataAddressEditBoxHex::GetValue()
{
	return editHex->value;
}

void CDataAddressEditBoxHex::SetNumDigits(int numDigits)
{
	this->numDigits = numDigits;
	editHex->SetValue(editHex->value, numDigits);
}


bool CDataAddressEditBoxHex::KeyDown(u32 keyCode)
{
	editHex->KeyDown(keyCode);
	return true;
}

void CDataAddressEditBoxHex::FinalizeEntering(u32 keyCode, bool isCancelled)
{
	editHex->FinalizeEntering(keyCode, isCancelled);
}

void CDataAddressEditBoxHex::CancelEntering()
{
	editHex->CancelEntering();
}

void CDataAddressEditBoxHex::SetCursorPos(int newPos)
{
	editHex->SetCursorPos(newPos);
}

void CDataAddressEditBoxHex::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (this->callback)
	{
		this->callback->DataAddressEditBoxEnteredValue(this, lastKeyCode, isCancelled);
	}
}

CDataAddressEditBoxHex::~CDataAddressEditBoxHex()
{
	delete editHex;
}
