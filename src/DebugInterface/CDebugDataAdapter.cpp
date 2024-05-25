//
//  CDebugDataAdapter.cpp
//  RetroDebugger
//
//  Created by Marcin Skoczylas on 04/02/2021.
//

#include "SYS_Defs.h"
#include "CDebugSymbols.h"
#include "CDebugDataAdapter.h"
#include "CDebugMemory.h"
#include "CDataAddressEditBoxHex.h"

CDebugDataAdapter::CDebugDataAdapter(const char *adapterID, CDebugSymbols *debugSymbols)
: CDataAdapter(adapterID)
{
	this->debugSymbols = debugSymbols;
	this->debugInterface = debugSymbols->debugInterface;
}

CDebugDataAdapter::~CDebugDataAdapter()
{
}

int CDebugDataAdapter::AdapterGetDataLength()
{
	return CDataAdapter::AdapterGetDataLength();
}

int CDebugDataAdapter::GetDataOffset()
{
	return CDataAdapter::GetDataOffset();
}

int CDebugDataAdapter::GetAddressForCell(int cell)
{
	return cell;
}

void CDebugDataAdapter::GetAddressStringForCell(int cell, char *str, int maxLen)
{
//	sprintfHexCode16(str, GetAddressForCell(cell));
	snprintf(str, maxLen, "%04x", GetAddressForCell(cell));
}

CDataAddressEditBox *CDebugDataAdapter::CreateDataAddressEditBox(CDataAddressEditBoxCallback *callback)
{
	CDataAddressEditBox *editBox = new CDataAddressEditBoxHex(4);
	editBox->SetCallback(callback);
	return editBox;
}

void CDebugDataAdapter::AdapterReadByte(int cell, uint8 *value)
{
	CDataAdapter::AdapterReadByte(cell, value);
}

void CDebugDataAdapter::AdapterWriteByte(int cell, uint8 value)
{
	CDataAdapter::AdapterWriteByte(cell, value);
}

void CDebugDataAdapter::AdapterReadByte(int cell, uint8 *value, bool *isAvailable)
{
	CDataAdapter::AdapterReadByte(cell, value, isAvailable);
}

void CDebugDataAdapter::AdapterWriteByte(int cell, uint8 value, bool *isAvailable)
{
	CDataAdapter::AdapterWriteByte(cell, value, isAvailable);
}

void CDebugDataAdapter::AdapterReadBlockDirect(uint8 *buffer, int cellStart, int cellEnd)
{
	CDataAdapter::AdapterReadBlockDirect(buffer, cellStart, cellEnd);
}

