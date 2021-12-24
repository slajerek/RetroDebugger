#include "CDataAdapterNesRam.h"
#include "CDebugInterfaceNes.h"

CDataAdapterNesRam::CDataAdapterNesRam(CDebugInterfaceNes *debugInterface)
: CDebugDataAdapter("NesRam", debugInterface)
{
	this->debugInterface = debugInterface;
}

int CDataAdapterNesRam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterNesRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte(pointer);
}

void CDataAdapterNesRam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte(pointer, value);
}


void CDataAdapterNesRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterNesRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByte(pointer, value);
	*isAvailable = true;
}

void CDataAdapterNesRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemory(buffer, pointerStart, pointerEnd);
}
