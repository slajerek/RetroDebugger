#include "CDataAdaptersVice.h"
#include "CDebugInterfaceVice.h"

CDataAdapterVice::CDataAdapterVice(CDebugInterfaceVice *debugInterface)
: CDataAdapter("Vice")
{
	this->debugInterface = debugInterface;
}

int CDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByteC64(pointer);
}

void CDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByteC64(pointer, value);
}


void CDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByteC64(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByteC64(pointer, value);
	*isAvailable = true;
}

void CDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemoryC64(buffer, pointerStart, pointerEnd);
}


///

CDirectRamDataAdapterVice::CDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterface)
: CDataAdapter("ViceDirectRam")
{
	this->debugInterface = debugInterface;
}

int CDirectRamDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByteFromRamC64(pointer);
}

void CDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByteToRamC64(pointer, value);
}


void CDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0 && pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByteFromRamC64(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterface->SetByteToRamC64(pointer, value);
	if (pointer >= 0 && pointer < 0x10000)
	{
		*isAvailable = true;
	}
	else
	{
		*isAvailable = false;
	}
}

void CDirectRamDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemoryFromRamC64(buffer, pointerStart, pointerEnd);
}

///

CDiskDataAdapterVice::CDiskDataAdapterVice(CDebugInterfaceVice *debugInterface)
: CDataAdapter("ViceDiskDrive")
{
	this->debugInterface = debugInterface;
}

int CDiskDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDiskDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByte1541(pointer);
}

void CDiskDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByte1541(pointer, value);
}

void CDiskDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0x8000 && pointer <= 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByte1541(pointer);
		return;
	}
	else if (pointer < 0x2000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByte1541(pointer);
		return;
	}
	*isAvailable = false;
	*value = 0x00;
}

void CDiskDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer >= 0xc000)
	{
		*isAvailable = true;
		this->debugInterface->SetByte1541(pointer, value);
		return;
	}
	else if (pointer < 0x2000)
	{
		*isAvailable = true;
		this->debugInterface->SetByte1541(pointer, value);
		return;
	}
	*isAvailable = false;
}

void CDiskDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemoryDrive1541(buffer, pointerStart, pointerEnd);
}


///

CDiskDirectRamDataAdapterVice::CDiskDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterface)
: CDataAdapter("ViceDiskDriveDirectRam")
{
	this->debugInterface = debugInterface;
}

int CDiskDirectRamDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDiskDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterface->GetByteFromRam1541(pointer);
}

void CDiskDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterface->SetByteToRam1541(pointer, value);
}

void CDiskDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0xc000 && pointer <= 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByteFromRam1541(pointer);
		return;
	}
	else if (pointer < 0x2000)
	{
		*isAvailable = true;
		*value = this->debugInterface->GetByteFromRam1541(pointer);
		return;
	}
	*isAvailable = false;
	*value = 0x00;
}

void CDiskDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer < 0x1000)
	{
		*isAvailable = true;
		this->debugInterface->SetByteToRam1541(pointer, value);
		return;
	}
	*isAvailable = false;
}

void CDiskDirectRamDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterface->GetMemoryFromRamDrive1541(buffer, pointerStart, pointerEnd);
}

// REU

CReuDataAdapterVice::CReuDataAdapterVice(CDebugInterfaceVice *debugInterface)
: CDataAdapter("ViceReu")
{
	this->debugInterface = debugInterface;
}

int CReuDataAdapterVice::AdapterGetDataLength()
{
	return 16*1024*1024; //16MB;
}


void CReuDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
//	*value = this->debugInterface->GetByteFromReu(pointer);
}

void CReuDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
//	this->debugInterface->SetByteToReu(pointer, value);
}

void CReuDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
//	if (pointer < 16MB)
//	{
//		*isAvailable = true;
//		*value = this->debugInterface->GetByteReu(pointer);
//		return;
//	}
//	*isAvailable = false;
}

void CReuDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
//	if (pointer < 16MB)
//	{
//		*isAvailable = true;
//		this->debugInterface->SetByteToReu(pointer, value);
//		return;
//	}
//	*isAvailable = false;
}

void CReuDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
//	this->debugInterface->GetMemoryFromReu(buffer, pointerStart, pointerEnd);
}

// Cartridge
CCartridgeDataAdapterVice::CCartridgeDataAdapterVice(CDebugInterfaceVice *debugInterface, CCartridgeDataAdapterViceType memoryType)
: CDataAdapter("ViceCartridge")
{
	this->debugInterface = debugInterface;
	this->memoryType = memoryType;
}

int CCartridgeDataAdapterVice::AdapterGetDataLength()
{
	// TODO: we need to get proper value of cartridge memory size... it is not stored in Vice, so temporarily we assume 512kB cart
	
	return 512 * 1024;
}

extern "C" {
unsigned char c64d_peek_cart_roml(int addr);
void c64d_poke_cart_roml(int addr, unsigned char value);
};

void CCartridgeDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = c64d_peek_cart_roml(pointer);
}

void CCartridgeDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	c64d_poke_cart_roml(pointer, value);
}

void CCartridgeDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	// TODO: fixme
	*value = c64d_peek_cart_roml(pointer);
	*isAvailable = true;
}

void CCartridgeDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	// TODO: fixme
	c64d_poke_cart_roml(pointer, value);
	*isAvailable = true;
}

void CCartridgeDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	int addr;
	u8 *bufPtr = buffer + pointerStart;
	for (addr = pointerStart; addr < pointerEnd; addr++)
	{
		*bufPtr++ = c64d_peek_cart_roml(addr);
	}
}

