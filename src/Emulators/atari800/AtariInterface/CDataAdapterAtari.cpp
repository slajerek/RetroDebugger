#include "CDataAdapterAtari.h"
#include "CDebugInterfaceAtari.h"
#include "AtariWrapper.h"

CDataAdapterAtari::CDataAdapterAtari(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("Atari", debugSymbols)
{
	this->debugInterfaceAtari = (CDebugInterfaceAtari*)(debugSymbols->debugInterface);
}

int CDataAdapterAtari::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterAtari::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceAtari->GetByte(pointer);
}

void CDataAdapterAtari::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceAtari->SetByte(pointer, value);
}


void CDataAdapterAtari::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceAtari->GetByte(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterAtari::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceAtari->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterAtari::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceAtari->GetMemory(buffer, pointerStart, pointerEnd);
}



CDataAdapterAtariDirectRam::CDataAdapterAtariDirectRam(CDebugSymbols *debugSymbols)
: CDataAdapterAtari(debugSymbols)
{
	this->adapterID = "AtariDirectRam";
}

void CDataAdapterAtariDirectRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = (uint8)atrd_peek_ram(pointer);
}

void CDataAdapterAtariDirectRam::AdapterWriteByte(int pointer, uint8 value)
{
	atrd_poke_ram(pointer, value);
}

void CDataAdapterAtariDirectRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	*isAvailable = pointer >= 0 && pointer < 0x10000;
	*value = *isAvailable ? (uint8)atrd_peek_ram(pointer) : 0;
}

void CDataAdapterAtariDirectRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	*isAvailable = pointer >= 0 && pointer < 0x10000;
	if (*isAvailable)
		atrd_poke_ram(pointer, value);
}

void CDataAdapterAtariDirectRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	for (int i = pointerStart; i < pointerEnd && i < 0x10000; i++)
		buffer[i] = (uint8)atrd_peek_ram(i);
}
