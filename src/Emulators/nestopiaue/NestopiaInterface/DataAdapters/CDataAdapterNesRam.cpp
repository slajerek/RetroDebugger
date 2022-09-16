#include "CDataAdapterNesRam.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesRam::CDataAdapterNesRam(CDebugInterfaceNes *debugInterfaceNes)
: CDebugDataAdapter("NesRam", debugInterfaceNes)
{
	this->debugInterfaceNes = debugInterfaceNes;
}

int CDataAdapterNesRam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceNes->GetByte(pointer);
}

void CDataAdapterNesRam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceNes->SetByte(pointer, value);
}


void CDataAdapterNesRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceNes->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceNes->GetMemory(buffer, pointerStart, pointerEnd);
}
