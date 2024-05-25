#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceC64.h"

CDataAdapterViceC64::CDataAdapterViceC64(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("Vice", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)(debugSymbols->debugInterface);
}

int CDataAdapterViceC64::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceC64::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteC64(pointer);
}

void CDataAdapterViceC64::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteC64(pointer, value);
}


void CDataAdapterViceC64::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByteC64(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterViceC64::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceVice->SetByteC64(pointer, value);
	*isAvailable = true;
}

void CDataAdapterViceC64::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryC64(buffer, pointerStart, pointerEnd);
}

