#include "CDataAdapterNesPpuChr.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpuChr::CDataAdapterNesPpuChr(CDebugInterfaceNes *debugInterfaceNes)
: CDataAdapter("NesPpuChr")
{
	this->debugInterfaceNes = debugInterfaceNes;
}

int CDataAdapterNesPpuChr::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpuChr::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceNes->GetByte(pointer);
}

void CDataAdapterNesPpuChr::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceNes->SetByte(pointer, value);
}


void CDataAdapterNesPpuChr::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpuChr::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceNes->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpuChr::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceNes->GetMemory(buffer, pointerStart, pointerEnd);
}


