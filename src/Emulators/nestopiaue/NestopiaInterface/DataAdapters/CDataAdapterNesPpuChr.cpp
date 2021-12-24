#include "CDataAdapterNesPpuChr.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpuChr::CDataAdapterNesPpuChr(CDebugInterfaceNes *debugInterface)
: CDataAdapter("NesPpuChr")
{
	this->debugInterface = debugInterface;
}

int CDataAdapterNesPpuChr::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpuChr::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte(pointer);
}

void CDataAdapterNesPpuChr::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte(pointer, value);
}


void CDataAdapterNesPpuChr::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpuChr::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpuChr::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemory(buffer, pointerStart, pointerEnd);
}


