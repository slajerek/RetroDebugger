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



#endif

