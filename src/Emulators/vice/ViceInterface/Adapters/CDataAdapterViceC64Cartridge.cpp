#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceC64Cartridge.h"
#include "CDataAddressEditBoxHex.h"

CDataAdapterViceC64Cartridge::CDataAdapterViceC64Cartridge(CDebugSymbols *debugSymbols, CCartridgeDataAdapterViceType memoryType)
: CDebugDataAdapter("ViceCartridge", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)(debugSymbols->debugInterface);
	this->memoryType = memoryType;
}

int CDataAdapterViceC64Cartridge::AdapterGetDataLength()
{
	// TODO: we need to get proper value of cartridge memory size... it is not stored in Vice, so temporarily we assume 512kB cart
	
	return 512 * 1024;
}

extern "C" {
unsigned char c64d_peek_cart_roml(int addr);
void c64d_poke_cart_roml(int addr, unsigned char value);
};

void CDataAdapterViceC64Cartridge::AdapterReadByte(int pointer, uint8 *value)
{
	*value = c64d_peek_cart_roml(pointer);
}

void CDataAdapterViceC64Cartridge::AdapterWriteByte(int pointer, uint8 value)
{
	c64d_poke_cart_roml(pointer, value);
}

void CDataAdapterViceC64Cartridge::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	// TODO: fixme
	*value = c64d_peek_cart_roml(pointer);
	*isAvailable = true;
}

void CDataAdapterViceC64Cartridge::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	// TODO: fixme
	c64d_poke_cart_roml(pointer, value);
	*isAvailable = true;
}

void CDataAdapterViceC64Cartridge::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	int addr;
	u8 *bufPtr = buffer + pointerStart;
	for (addr = pointerStart; addr < pointerEnd; addr++)
	{
		*bufPtr++ = c64d_peek_cart_roml(addr);
	}
}

CDataAddressEditBox *CDataAdapterViceC64Cartridge::CreateDataAddressEditBox(CDataAddressEditBoxCallback *callback)
{
	CDataAddressEditBox *editBox = new CDataAddressEditBoxHex(5);
	editBox->SetCallback(callback);
	return editBox;
}

void CDataAdapterViceC64Cartridge::GetAddressStringForCell(int cell, char *str, int maxLen)
{
//	sprintfHexCode16(str, GetAddressForCell(cell));
	snprintf(str, maxLen, "%05x", GetAddressForCell(cell));
}
