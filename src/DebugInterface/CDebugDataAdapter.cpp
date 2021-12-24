//
//  CDebugDataAdapter.cpp
//  RetroDebugger
//
//  Created by Marcin Skoczylas on 04/02/2021.
//

#include "SYS_Defs.h"
#include "CDebugInterface.h"
#include "CDebugDataAdapter.h"
#include "CViewMemoryMap.h"

CDebugDataAdapter::CDebugDataAdapter(char *adapterID, CDebugInterface *debugInterface)
: CDataAdapter(adapterID)
{
	this->debugInterface = debugInterface;
	this->viewMemoryMap = NULL;
}

void CDebugDataAdapter::SetViewMemoryMap(CViewMemoryMap *viewMemoryMap)
{
	this->viewMemoryMap = viewMemoryMap;
}

int CDebugDataAdapter::AdapterGetDataLength()
{
	return CDataAdapter::AdapterGetDataLength();
}

int CDebugDataAdapter::GetDataOffset()
{
	return CDataAdapter::GetDataOffset();
}

void CDebugDataAdapter::AdapterReadByte(int addr, uint8 *value)
{
	CDataAdapter::AdapterReadByte(addr, value);
}

void CDebugDataAdapter::AdapterWriteByte(int addr, uint8 value)
{
	CDataAdapter::AdapterWriteByte(addr, value);
}

void CDebugDataAdapter::AdapterReadByte(int addr, uint8 *value, bool *isAvailable)
{
	CDataAdapter::AdapterReadByte(addr, value, isAvailable);
}

void CDebugDataAdapter::AdapterWriteByte(int addr, uint8 value, bool *isAvailable)
{
	CDataAdapter::AdapterWriteByte(addr, value, isAvailable);
}

void CDebugDataAdapter::AdapterReadBlockDirect(uint8 *buffer, int addrStart, int addrEnd)
{
	CDataAdapter::AdapterReadBlockDirect(buffer, addrStart, addrEnd);
}

void CDebugDataAdapter::MarkCellRead(int addr)
{
	int pc = debugInterface->GetCpuPC();
	viewMemoryMap->CellRead(addr, pc, -1, -1);
}

void CDebugDataAdapter::MarkCellRead(int addr, int pc, int rasterX, int rasterY)
{
	viewMemoryMap->CellRead(addr, pc, rasterX, rasterY);
}

void CDebugDataAdapter::MarkCellWrite(int addr, uint8 value)
{
	int pc = debugInterface->GetCpuPC();
	viewMemoryMap->CellWrite(addr, value, pc, -1, -1);
}

void CDebugDataAdapter::MarkCellWrite(int addr, uint8 value, int pc, int rasterX, int rasterY)
{
	viewMemoryMap->CellWrite(addr, value, pc, rasterX, rasterY);
}

void CDebugDataAdapter::MarkCellExecute(int addr, uint8 opcode)
{
	viewMemoryMap->CellExecute(addr, opcode);
}
