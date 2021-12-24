#include "CDataAdapterAtari.h"
#include "CDebugInterfaceAtari.h"

CDataAdapterAtari::CDataAdapterAtari(CDebugInterfaceAtari *debugInterface)
: CDataAdapter("Atari")
{
	this->debugInterface = debugInterface;
}

int CDataAdapterAtari::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterAtari::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte(pointer);
}

void CDataAdapterAtari::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte(pointer, value);
}


void CDataAdapterAtari::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByte(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterAtari::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterAtari::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemory(buffer, pointerStart, pointerEnd);
}


