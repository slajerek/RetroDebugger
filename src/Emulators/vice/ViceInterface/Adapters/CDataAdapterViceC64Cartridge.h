#ifndef CDataAdapterViceC64Cartridge_h
#define CDataAdapterViceC64Cartridge_h

#include "CDebugDataAdapter.h"

class CDebugInterfaceVice;

enum CCartridgeDataAdapterViceType : u8
{
	C64CartridgeDataAdapterViceTypeRomL = 0,
	C64CartridgeDataAdapterViceTypeRomH,
	C64CartridgeDataAdapterViceTypeRamL
};

class CDataAdapterViceC64Cartridge : public CDebugDataAdapter
{
public:
	CDataAdapterViceC64Cartridge(CDebugSymbols *debugSymbols, CCartridgeDataAdapterViceType memoryType);
	CDebugInterfaceVice *debugInterfaceVice;
	CCartridgeDataAdapterViceType memoryType;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
	
	virtual void GetAddressStringForCell(int cell, char *str, int maxLen);
	virtual CDataAddressEditBox *CreateDataAddressEditBox(CDataAddressEditBoxCallback *callback);
};

#endif //CDataAdapterViceC64Cartridge_h
