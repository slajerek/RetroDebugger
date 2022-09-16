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

class CDirectRamDataAdapterVice : public CDebugDataAdapter
{
public:
	CDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDiskDataAdapterVice : public CDebugDataAdapter
{
public:
	CDiskDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDiskDirectRamDataAdapterVice : public CDebugDataAdapter
{
public:
	CDiskDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice);
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

class CCartridgeDataAdapterVice : public CDebugDataAdapter
{
public:
	CCartridgeDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice, CCartridgeDataAdapterViceType memoryType);
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
class CReuDataAdapterVice : public CDebugDataAdapter
{
public:
	CReuDataAdapterVice(CDebugInterfaceVice *debugInterfaceVice);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};


#endif

