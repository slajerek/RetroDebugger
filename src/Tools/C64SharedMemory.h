#ifndef _C64SHAREDMEMORY_H_
#define _C64SHAREDMEMORY_H_

#include "SYS_Main.h"
#include "CByteBuffer.h"

void C64DebuggerInitSharedMemory();

int C64DebuggerSendConfiguration(CByteBuffer *byteBuffer);
void C64DebuggerReceivedConfiguration(CByteBuffer *byteBuffer);

#endif