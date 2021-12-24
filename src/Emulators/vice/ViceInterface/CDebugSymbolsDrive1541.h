#ifndef _CDebugSymbolsDrive1541_h_
#define _CDebugSymbolsDrive1541_h_

#include "CDebugSymbols.h"

class CDebugSymbolsDrive1541 : public CDebugSymbols
{
public:
	CDebugSymbolsDrive1541(CDebugInterface *debugInterface, CDataAdapter *dataAdapter);
	
	// get currently executed addr (PC) to be checked against breakpoint, note it is virtual to have it different for C64 Drive CPU
	virtual int GetCurrentExecuteAddr();

	virtual CDebugSymbolsSegment *CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum);
};

#endif

