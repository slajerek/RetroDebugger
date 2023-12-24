#include "CDataAdaptersVice.h"
#include "CDebugInterfaceVice.h"

CDataAdapterVice::CDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("Vice", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteC64(pointer);
}

void CDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteC64(pointer, value);
}


void CDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByteC64(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceVice->SetByteC64(pointer, value);
	*isAvailable = true;
}

void CDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryC64(buffer, pointerStart, pointerEnd);
}


///

CDataAdapterViceDirectRam::CDataAdapterViceDirectRam(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceDirectRam", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDataAdapterViceDirectRam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceDirectRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteFromRamC64(pointer);
}

void CDataAdapterViceDirectRam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteToRamC64(pointer, value);
}


void CDataAdapterViceDirectRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer >= 0 && pointer < 0x10000)
	{
		*isAvailable = true;
		*value = this->debugInterfaceVice->GetByteFromRamC64(pointer);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterViceDirectRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	this->debugInterfaceVice->SetByteToRamC64(pointer, value);
	if (pointer >= 0 && pointer < 0x10000)
	{
		*isAvailable = true;
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterViceDirectRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryFromRamC64(buffer, pointerStart, pointerEnd);
}

///

CDataAdapterViceDisk::CDataAdapterViceDisk(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceDiskDrive", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDataAdapterViceDisk::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceDisk::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByte1541(pointer);
}

void CDataAdapterViceDisk::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByte1541(pointer, value);
}

void CDataAdapterViceDisk::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterViceDisk::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
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

void CDataAdapterViceDisk::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryDrive1541(buffer, pointerStart, pointerEnd);
}


///

CDataAdapterViceDiskDirectRam::CDataAdapterViceDiskDirectRam(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceDiskDriveDirectRam", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDataAdapterViceDiskDirectRam::AdapterGetDataLength()
{
	return 0x10000;
}


void CDataAdapterViceDiskDirectRam::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteFromRam1541(pointer);
}

void CDataAdapterViceDiskDirectRam::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteToRam1541(pointer, value);
}

void CDataAdapterViceDiskDirectRam::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDataAdapterViceDiskDirectRam::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer < 0x1000)
	{
		*isAvailable = true;
		this->debugInterfaceVice->SetByteToRam1541(pointer, value);
		return;
	}
	*isAvailable = false;
}

void CDataAdapterViceDiskDirectRam::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryFromRamDrive1541(buffer, pointerStart, pointerEnd);
}

// REU

CDataAdapterViceReu::CDataAdapterViceReu(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceReu", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDataAdapterViceReu::AdapterGetDataLength()
{
	return 16*1024*1024; //16MB;
}


void CDataAdapterViceReu::AdapterReadByte(int pointer, uint8 *value)
{
//	*value = this->debugInterface->GetByteFromReu(pointer);
}

void CDataAdapterViceReu::AdapterWriteByte(int pointer, uint8 value)
{
//	this->debugInterface->SetByteToReu(pointer, value);
}

void CDataAdapterViceReu::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
//	if (pointer < 16MB)
//	{
//		*isAvailable = true;
//		*value = this->debugInterface->GetByteReu(pointer);
//		return;
//	}
//	*isAvailable = false;
}

void CDataAdapterViceReu::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
//	if (pointer < 16MB)
//	{
//		*isAvailable = true;
//		this->debugInterface->SetByteToReu(pointer, value);
//		return;
//	}
//	*isAvailable = false;
}

void CDataAdapterViceReu::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
//	this->debugInterface->GetMemoryFromReu(buffer, pointerStart, pointerEnd);
}

// Cartridge
CDataAdapterViceCartridge::CDataAdapterViceCartridge(CDebugInterfaceVice *debugInterfaceVice, CCartridgeDataAdapterViceType memoryType)
: CDebugDataAdapter("ViceCartridge", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
	this->memoryType = memoryType;
}

int CDataAdapterViceCartridge::AdapterGetDataLength()
{
	// TODO: we need to get proper value of cartridge memory size... it is not stored in Vice, so temporarily we assume 512kB cart
	
	return 512 * 1024;
}

extern "C" {
unsigned char c64d_peek_cart_roml(int addr);
void c64d_poke_cart_roml(int addr, unsigned char value);
};

void CDataAdapterViceCartridge::AdapterReadByte(int pointer, uint8 *value)
{
	*value = c64d_peek_cart_roml(pointer);
}

void CDataAdapterViceCartridge::AdapterWriteByte(int pointer, uint8 value)
{
	c64d_poke_cart_roml(pointer, value);
}

void CDataAdapterViceCartridge::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	// TODO: fixme
	*value = c64d_peek_cart_roml(pointer);
	*isAvailable = true;
}

void CDataAdapterViceCartridge::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	// TODO: fixme
	c64d_poke_cart_roml(pointer, value);
	*isAvailable = true;
}

void CDataAdapterViceCartridge::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	int addr;
	u8 *bufPtr = buffer + pointerStart;
	for (addr = pointerStart; addr < pointerEnd; addr++)
	{
		*bufPtr++ = c64d_peek_cart_roml(addr);
	}
}

