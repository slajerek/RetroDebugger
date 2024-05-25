#include "CDataAdapterAtari.h"
#include "CDebugInterfaceAtari.h"

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


