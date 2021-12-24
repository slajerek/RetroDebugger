#include "CDataAdapterNesPpu.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpu::CDataAdapterNesPpu(CDebugInterfaceNes *debugInterface)
: CDataAdapter("NesPpu")
{
	this->debugInterface = debugInterface;
}

int CDataAdapterNesPpu::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpu::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte(pointer);
}

void CDataAdapterNesPpu::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte(pointer, value);
}


void CDataAdapterNesPpu::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpu::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpu::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemory(buffer, pointerStart, pointerEnd);
}


