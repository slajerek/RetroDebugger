#ifndef _CDataAdapterAtari_H_
#define _CDataAdapterAtari_H_

#include "CDataAdapter.h"

class CDebugInterfaceAtari;

class CDataAdapterAtari : public CDataAdapter
{
public:
	CDataAdapterAtari(CDebugInterfaceAtari *debugInterface);
	CDebugInterfaceAtari *debugInterface;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};



#endif

