#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceC64Reu.h"

// TODO: not implemented REU data adapter
CDataAdapterViceC64Reu::CDataAdapterViceC64Reu(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("ViceReu", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)(debugSymbols->debugInterface);
}

int CDataAdapterViceC64Reu::AdapterGetDataLength()
{
	return 16*1024*1024; //16MB;
}


void CDataAdapterViceC64Reu::AdapterReadByte(int pointer, uint8 *value)
{
//	*value = this->debugInterface->GetByteFromReu(pointer);
}

void CDataAdapterViceC64Reu::AdapterWriteByte(int pointer, uint8 value)
{
//	this->debugInterface->SetByteToReu(pointer, value);
}

void CDataAdapterViceC64Reu::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
//	if (pointer < 16MB)
//	{
//		*isAvailable = true;
//		*value = this->debugInterface->GetByteReu(pointer);
//		return;
//	}
//	*isAvailable = false;
}

void CDataAdapterViceC64Reu::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
//	if (pointer < 16MB)
//	{
//		*isAvailable = true;
//		this->debugInterface->SetByteToReu(pointer, value);
//		return;
//	}
//	*isAvailable = false;
}

void CDataAdapterViceC64Reu::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
//	this->debugInterface->GetMemoryFromReu(buffer, pointerStart, pointerEnd);
}

