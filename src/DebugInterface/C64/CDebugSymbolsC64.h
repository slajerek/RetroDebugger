#ifndef _CDebugSymbolsC64_h_
#define _CDebugSymbolsC64_h

#include "CDebugSymbols.h"

class CDebugSymbolsC64 : public CDebugSymbols
{
public:
	CDebugSymbolsC64(CDebugInterface *debugInterface);
	
	virtual CDebugSymbolsSegment *CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum);
};

#endif

