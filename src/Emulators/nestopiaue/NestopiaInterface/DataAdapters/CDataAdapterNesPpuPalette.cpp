#include "CDataAdapterNesPpuPalette.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpuPalette::CDataAdapterNesPpuPalette(CDebugInterfaceNes *debugInterface)
: CDataAdapter("NesPpuPalette")
{
	this->debugInterface = debugInterface;
}

int CDataAdapterNesPpuPalette::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpuPalette::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte(pointer);
}

void CDataAdapterNesPpuPalette::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte(pointer, value);
}


void CDataAdapterNesPpuPalette::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpuPalette::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpuPalette::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemory(buffer, pointerStart, pointerEnd);
}


