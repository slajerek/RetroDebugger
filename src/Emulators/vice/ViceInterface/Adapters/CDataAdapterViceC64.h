#ifndef CDataAdapterViceC64_h
#define CDataAdapterViceC64_h

#include "CDebugDataAdapter.h"

class CDebugInterfaceVice;

class CDataAdapterViceC64 : public CDebugDataAdapter
{
public:
	CDataAdapterViceC64(CDebugSymbols *debugSymbols);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

#endif //CDataAdapterViceC64_h

