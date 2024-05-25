#ifndef CDataAdapterViceDrive1541DirectRam_h
#define CDataAdapterViceDrive1541DirectRam_h

#include "CDebugDataAdapter.h"

class CDebugSymbols;
class CDebugInterfaceVice;

class CDataAdapterViceDrive1541DirectRam : public CDebugDataAdapter
{
public:
	CDataAdapterViceDrive1541DirectRam(CDebugSymbols *debugSymbols);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};

#endif //CDataAdapterViceDrive1541DirectRam_h
