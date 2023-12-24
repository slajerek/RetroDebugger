#ifndef _CDataAdaptersVice_H_
#define _CDataAdapterVice_H_

#include "CDebugDataAdapter.h"

class CDebugInterfaceVice;

class CDataAdapterVice : public CDebugDataAdapter
{
public:
	CDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDataAdapterViceDirectRam : public CDebugDataAdapter
{
public:
	CDataAdapterViceDirectRam(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDataAdapterViceDisk : public CDebugDataAdapter
{
public:
	CDataAdapterViceDisk(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDataAdapterViceDiskDirectRam : public CDebugDataAdapter
{
public:
	CDataAdapterViceDiskDirectRam(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

//
enum CCartridgeDataAdapterViceType : u8
{
	C64CartridgeDataAdapterViceTypeRomL = 0,
	C64CartridgeDataAdapterViceTypeRomH,
	C64CartridgeDataAdapterViceTypeRamL
};

class CDataAdapterViceCartridge : public CDebugDataAdapter
{
public:
	CDataAdapterViceCartridge(CDebugInterfaceVice *debugInterfaceVice, CCartridgeDataAdapterViceType memoryType);
	CDebugInterfaceVice *debugInterfaceVice;
	CCartridgeDataAdapterViceType memoryType;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

//
class CDataAdapterViceReu : public CDebugDataAdapter
{
public:
	CDataAdapterViceReu(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};


#endif

