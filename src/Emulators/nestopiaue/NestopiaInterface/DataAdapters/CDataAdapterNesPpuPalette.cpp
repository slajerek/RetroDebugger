#include "CDataAdapterNesPpuPalette.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpuPalette::CDataAdapterNesPpuPalette(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("NesPpuPalette", debugSymbols)
{
	this->debugInterfaceNes = (CDebugInterfaceNes *)(debugSymbols->debugInterface);
}

int CDataAdapterNesPpuPalette::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpuPalette::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceNes->GetByte(pointer);
}

void CDataAdapterNesPpuPalette::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceNes->SetByte(pointer, value);
}


void CDataAdapterNesPpuPalette::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceNes->GetByte(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterNesPpuPalette::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceNes->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpuPalette::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceNes->GetMemory(buffer, pointerStart, pointerEnd);
}


