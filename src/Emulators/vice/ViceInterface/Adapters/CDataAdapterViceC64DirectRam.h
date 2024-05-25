#ifndef CDataAdapterViceC64DirectRam_h
#define CDataAdapterViceC64DirectRam_h

#include "CDebugDataAdapter.h"

class CDebugInterfaceVice;

class CDataAdapterViceC64DirectRam : public CDebugDataAdapter
{
public:
	CDataAdapterViceC64DirectRam(CDebugSymbols *debugSymbols);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

#endif //CDataAdapterViceC64DirectRam_h
