#include "CDataAdapterNesPpuOam.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpuOam::CDataAdapterNesPpuOam(CDebugInterfaceNes *debugInterface)
: CDataAdapter("NesPpuOam")
{
	this->debugInterface = debugInterface;
}

int CDataAdapterNesPpuOam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpuOam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte(pointer);
}

void CDataAdapterNesPpuOam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte(pointer, value);
}


void CDataAdapterNesPpuOam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpuOam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpuOam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemory(buffer, pointerStart, pointerEnd);
}


