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
	// VICE keeps 2 MB buffers (C64CART_ROM_LIMIT) but not the size of the
	// attached image; the view shows the first 512 KB because every byte here
	// also gets a CDebugMemoryCell. Physical cartridge identity for source
	// debugging comes from c64d_get_cart_state(), not from this view.
	return 512 * 1024;
}

extern "C" {
unsigned char c64d_peek_cart_buffer(int kind, int addr);
void c64d_poke_cart_buffer(int kind, int addr, unsigned char value);
};

// CCartridgeDataAdapterViceType order matches c64d_peek_cart_buffer's kinds.
void CDataAdapterViceC64Cartridge::AdapterReadByte(int pointer, uint8 *value)
{
	*value = c64d_peek_cart_buffer(memoryType, pointer);
}

void CDataAdapterViceC64Cartridge::AdapterWriteByte(int pointer, uint8 value)
{
	c64d_poke_cart_buffer(memoryType, pointer, value);
}

void CDataAdapterViceC64Cartridge::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	*isAvailable = (pointer >= 0 && pointer < AdapterGetDataLength());
	*value = *isAvailable ? c64d_peek_cart_buffer(memoryType, pointer) : 0;
}

void CDataAdapterViceC64Cartridge::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	*isAvailable = (pointer >= 0 && pointer < AdapterGetDataLength());
	if (*isAvailable)
		c64d_poke_cart_buffer(memoryType, pointer, value);
}

void CDataAdapterViceC64Cartridge::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	int addr;
	u8 *bufPtr = buffer + pointerStart;
	for (addr = pointerStart; addr < pointerEnd; addr++)
	{
		*bufPtr++ = c64d_peek_cart_buffer(memoryType, addr);
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
