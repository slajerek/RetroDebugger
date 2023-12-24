#ifndef _C64SIDDump_h_
#define _C64SIDDump_h_

#include "SYS_Defs.h"

class CSidData;
class CByteBuffer;

enum {
	SID_HISTORY_FORMAT_SIDDUMP = 0,
	SID_HISTORY_FORMAT_CSV
};

void C64SIDHistoryToByteBuffer(std::list<CSidData *> *sidDataHistory, CByteBuffer *byteBuffer, u8 format,
							int basefreq=0, int basenote=0xb0, int spacing=0, int oldnotefactor=1, int pattspacing=0, int timeseconds=0, int lowres=0);

#endif
