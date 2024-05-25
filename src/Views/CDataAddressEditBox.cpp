#include "CDataAddressEditBox.h"


CDataAddressEditBox::CDataAddressEditBox(int numDigits)
{
	this->numDigits = numDigits;
}

void CDataAddressEditBox::SetCallback(CDataAddressEditBoxCallback *callback)
{
	this->callback = callback;
}

void CDataAddressEditBox::Render(float posX, float posY, CSlrFont *font, float fontSize)
{
}

void CDataAddressEditBox::SetValue(int value)
{
}

int CDataAddressEditBox::GetValue()
{
	return 0;
}


void CDataAddressEditBox::SetNumDigits(int numDigits)
{
	this->numDigits = numDigits;
}

int CDataAddressEditBox::GetNumDigits()
{
	return this->numDigits;
}

bool CDataAddressEditBox::KeyDown(u32 keyCode)
{
	return false;
}

void CDataAddressEditBox::FinalizeEntering(u32 keyCode, bool isCancelled)
{
}

void CDataAddressEditBox::CancelEntering()
{
}

void CDataAddressEditBox::SetCursorPos(int newPos)
{
}

CDataAddressEditBox::~CDataAddressEditBox()
{
}
