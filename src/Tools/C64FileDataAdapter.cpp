#include "C64FileDataAdapter.h"
#include "CSlrFile.h"
#include "CByteBuffer.h"
#include "SYS_Main.h"


C64FileDataAdapter::C64FileDataAdapter(CSlrFile *file)
: CDataAdapter("C64File")
{
	this->byteBuffer = new CByteBuffer(file, false);
}

int C64FileDataAdapter::AdapterGetDataLength()
{
	return this->byteBuffer->length;
}


void C64FileDataAdapter::AdapterReadByte(int pointer, u8 *value, bool *isAvailable)
{
	if (pointer < 0)
	{
		*isAvailable = false;
	}
	else if (pointer >= this->byteBuffer->length)
	{
		*isAvailable = false;
	}
	else
	{
		*isAvailable = true;
		*value = this->byteBuffer->data[pointer];
	}
}

void C64FileDataAdapter::AdapterWriteByte(int pointer, u8 value, bool *isAvailable)
{
	if (pointer < 0)
	{
		*isAvailable = false;
	}
	else if (pointer >= this->byteBuffer->length)
	{
		*isAvailable = false;
	}
	else
	{
		*isAvailable = true;
		this->byteBuffer->data[pointer] = value;
	}
}

