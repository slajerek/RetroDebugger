#include "CDataAdapterNesPpuOam.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpuOam::CDataAdapterNesPpuOam(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("NesPpuOam", debugSymbols)
{
	this->debugInterfaceNes = (CDebugInterfaceNes *)(debugSymbols->debugInterface);
}

int CDataAdapterNesPpuOam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpuOam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceNes->GetByte(pointer);
}

void CDataAdapterNesPpuOam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceNes->SetByte(pointer, value);
}


void CDataAdapterNesPpuOam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpuOam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceNes->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpuOam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceNes->GetMemory(buffer, pointerStart, pointerEnd);
}


