#ifndef _CDataAdaptersVice_H_
#define _CDataAdapterVice_H_

#include "CDataAdapter.h"

class CDebugInterfaceVice;

class CDataAdapterVice : public CDataAdapter
{
public:
	CDataAdapterVice(CDebugInterfaceVice *debugInterface);
	CDebugInterfaceVice *debugInterface;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDirectRamDataAdapterVice : public CDataAdapter
{
public:
	CDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterface);
	CDebugInterfaceVice *debugInterface;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDiskDataAdapterVice : public CDataAdapter
{
public:
	CDiskDataAdapterVice(CDebugInterfaceVice *debugInterface);
	CDebugInterfaceVice *debugInterface;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

class CDiskDirectRamDataAdapterVice : public CDataAdapter
{
public:
	CDiskDirectRamDataAdapterVice(CDebugInterfaceVice *debugInterface);
	CDebugInterfaceVice *debugInterface;
	
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

class CCartridgeDataAdapterVice : public CDataAdapter
{
public:
	CCartridgeDataAdapterVice(CDebugInterfaceVice *debugInterface, CCartridgeDataAdapterViceType memoryType);
	CDebugInterfaceVice *debugInterface;
	CCartridgeDataAdapterViceType memoryType;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

//
class CReuDataAdapterVice : public CDataAdapter
{
public:
	CReuDataAdapterVice(CDebugInterfaceVice *debugInterface);
	CDebugInterfaceVice *debugInterface;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};


#endif

