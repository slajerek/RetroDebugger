#ifndef _CDataAdapterAtari_H_
#define _CDataAdapterAtari_H_

#include "CDebugDataAdapter.h"

class CDebugInterfaceAtari;

class CDataAdapterAtari : public CDebugDataAdapter
{
public:
	CDataAdapterAtari(CDebugSymbols *debugSymbols);
	CDebugInterfaceAtari *debugInterfaceAtari;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

// RD-029: the 64 KB of RAM, not the CPU view: under a switched-in ROM (OS,
// Self Test, BASIC, cartridge) the RAM it hides, under a 130XE bank at
// $4000-$7FFF the base RAM (Save/Load Binary, the monitor's RAM view)
class CDataAdapterAtariDirectRam : public CDataAdapterAtari
{
public:
	CDataAdapterAtariDirectRam(CDebugSymbols *debugSymbols);
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};



#endif

