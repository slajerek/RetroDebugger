#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceDrive1541DirectRam.h"

CDataAdapterViceDrive1541DirectRam::CDataAdapterViceDrive1541DirectRam(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("ViceDrive1541DirectRam", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)(debugSymbols->debugInterface);
}

int CDataAdapterViceDrive1541DirectRam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceDrive1541DirectRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteFromRam1541(pointer);
}

void CDataAdapterViceDrive1541DirectRam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteToRam1541(pointer, value);
}

void CDataAdapterViceDrive1541DirectRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0xc000 && pointer <= 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByteFromRam1541(pointer);
		return;
	}
	else if (pointer < 0x2000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByteFromRam1541(pointer);
		return;
	}
	*isAvailable = false;
	*value = 0x00;
}

void CDataAdapterViceDrive1541DirectRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer < 0x1000)
	{
		*isAvailable = true;
		this->debugInterfaceVice->SetByteToRam1541(pointer, value);
		return;
	}
	*isAvailable = false;
}

void CDataAdapterViceDrive1541DirectRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryFromRamDrive1541(buffer, pointerStart, pointerEnd);
}
