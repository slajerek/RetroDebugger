#ifndef CDataAdapterViceC64Reu_h
#define CDataAdapterViceC64Reu_h

#include "CDebugDataAdapter.h"

class CDebugInterfaceVice;

class CDataAdapterViceC64Reu : public CDebugDataAdapter
{
public:
	CDataAdapterViceC64Reu(CDebugSymbols *debugSymbols);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};


#endif //CDataAdapterViceC64Reu_h
