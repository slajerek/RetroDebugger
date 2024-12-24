#ifndef CDataAdapterDrive1541Minimal_h
#define CDataAdapterDrive1541Minimal_h

#include "CDebugDataAdapter.h"

class CDebugInterfaceC64;

// Note, old algorithm was painting 64 bytes per row, finished at address $1c10
// then image was painted cropped with texture end at last row (33/1024)

// imageHeight=1024	imageWidth=64
// last cell is vx=16 vy=33 addr=1c10
// 64 per row * 33 = 2112   + 16 = 2128
#define DRIVE1541_MINIMAL_MEMORY_ADAPTER_LENGTH	2128

class CDataAdapterDrive1541Minimal : public CDebugDataAdapter
{
public:
	CDataAdapterDrive1541Minimal(CDebugSymbols *debugSymbols, CDebugDataAdapter *drive1541DataAdapter);	
	CDebugDataAdapter *drive1541DataAdapter;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	
private:
	int len;
	int mapPointerToDriveAddress[65535];
	int mapAddressToPointer[65535];
};


#endif
