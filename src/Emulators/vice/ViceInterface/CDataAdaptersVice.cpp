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

CDirectRamDataAdapterVice::CDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceDirectRam", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDirectRamDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteFromRamC64(pointer);
}

void CDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteToRamC64(pointer, value);
}


void CDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
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

void CDirectRamDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryFromRamC64(buffer, pointerStart, pointerEnd);
}

///

CDiskDataAdapterVice::CDiskDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceDiskDrive", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDiskDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDiskDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByte1541(pointer);
}

void CDiskDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByte1541(pointer, value);
}

void CDiskDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDiskDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
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

void CDiskDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryDrive1541(buffer, pointerStart, pointerEnd);
}


///

CDiskDirectRamDataAdapterVice::CDiskDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceDiskDriveDirectRam", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
}

int CDiskDirectRamDataAdapterVice::AdapterGetDataLength()
{
	return 0x10000;
}


void CDiskDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value)
{
	*value = this->debugInterfaceVice->GetByteFromRam1541(pointer);
}

void CDiskDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value)
{
	this->debugInterfaceVice->SetByteToRam1541(pointer, value);
}

void CDiskDirectRamDataAdapterVice::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
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

void CDiskDirectRamDataAdapterVice::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer < 0x1000)
	{
		*isAvailable = true;
		this->debugInterfaceVice->SetByteToRam1541(pointer, value);
		return;
	}
	*isAvailable = false;
}

void CDiskDirectRamDataAdapterVice::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	this->debugInterfaceVice->GetMemoryFromRamDrive1541(buffer, pointerStart, pointerEnd);
}

// REU

CReuDataAdapterVice::CReuDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice)
: CDebugDataAdapter("ViceReu", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
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
CCartridgeDataAdapterVice::CCartridgeDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice, CCartridgeDataAdapterViceType memoryType)
: CDebugDataAdapter("ViceCartridge", debugInterfaceVice)
{
	this->debugInterfaceVice = debugInterfaceVice;
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

