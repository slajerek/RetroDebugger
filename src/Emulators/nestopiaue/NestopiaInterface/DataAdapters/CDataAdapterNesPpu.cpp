#include "CDataAdapterNesPpu.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesPpu::CDataAdapterNesPpu(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("NesPpu", debugSymbols)
{
	this->debugInterfaceNes = (CDebugInterfaceNes *)(debugSymbols->debugInterface);
}

int CDataAdapterNesPpu::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesPpu::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceNes->GetByte(pointer);
}

void CDataAdapterNesPpu::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceNes->SetByte(pointer, value);
}


void CDataAdapterNesPpu::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesPpu::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceNes->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesPpu::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceNes->GetMemory(buffer, pointerStart, pointerEnd);
}


