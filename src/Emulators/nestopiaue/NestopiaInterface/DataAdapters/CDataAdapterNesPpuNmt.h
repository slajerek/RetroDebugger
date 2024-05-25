#ifndef _CDataAdapterNesPpuNmt_H_
#define _CDataAdapterNesPpuNmt_H_

#include "CDebugDataAdapter.h"

class CDebugInterfaceNes;

class CDataAdapterNesPpuNmt : public CDebugDataAdapter
{
public:
	CDataAdapterNesPpuNmt(CDebugSymbols *debugSymbols);
	CDebugInterfaceNes *debugInterfaceNes;
	
	virtual int AdapterGetDataLength();
	
	// renderers should add this offset to the presented address
	virtual int GetDataOffset();
	
	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd);
};



#endif

