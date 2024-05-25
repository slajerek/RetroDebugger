#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceDrive1541.h"

CDataAdapterViceDrive1541::CDataAdapterViceDrive1541(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("ViceDrive1541", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)(debugSymbols->debugInterface);
}

int CDataAdapterViceDrive1541::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceDrive1541::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByte1541(pointer);
}

void CDataAdapterViceDrive1541::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByte1541(pointer, value);
}

void CDataAdapterViceDrive1541::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0x8000 && pointer <= 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByte1541(pointer);
		return;
	}
	else if (pointer < 0x2000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByte1541(pointer);
		return;
	}
	*isAvailable = false;
	*value = 0x00;
}

void CDataAdapterViceDrive1541::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer >= 0xc000)
	{
		*isAvailable = true;
		this->debugInterfaceVice->SetByte1541(pointer, value);
		return;
	}
	else if (pointer < 0x2000)
	{
		*isAvailable = true;
		this->debugInterfaceVice->SetByte1541(pointer, value);
		return;
	}
	*isAvailable = false;
}

void CDataAdapterViceDrive1541::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryDrive1541(buffer, pointerStart, pointerEnd);
}
