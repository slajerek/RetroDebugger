#ifndef _C64DATAADAPTERS_H_
#define _C64DATAADAPTERS_H_

#include "CDataAdapter.h"

class CSlrFile;
class CByteBuffer;


class C64FileDataAdapter : public CDataAdapter
{
public:
	C64FileDataAdapter(CSlrFile *file);

	CByteBuffer *byteBuffer;
	
	virtual int AdapterGetDataLength();
	virtual void AdapterReadByte(int pointer, u8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, u8 value, bool *isAvailable);
};

#endif
