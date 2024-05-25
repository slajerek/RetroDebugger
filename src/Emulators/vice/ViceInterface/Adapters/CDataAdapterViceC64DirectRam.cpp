#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceC64DirectRam.h"

CDataAdapterViceC64DirectRam::CDataAdapterViceC64DirectRam(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("ViceDirectRam", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)(debugSymbols->debugInterface);
}

int CDataAdapterViceC64DirectRam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceC64DirectRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteFromRamC64(pointer);
}

void CDataAdapterViceC64DirectRam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteToRamC64(pointer, value);
}


void CDataAdapterViceC64DirectRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0 && pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByteFromRamC64(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterViceC64DirectRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceVice->SetByteToRamC64(pointer, value);
	if (pointer >= 0 && pointer < 0x10000)
	{
		*isAvailable = true;
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterViceC64DirectRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryFromRamC64(buffer, pointerStart, pointerEnd);
}
