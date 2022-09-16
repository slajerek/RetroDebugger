#ifndef _CViewC64Disassembly_h_
#define _CViewC64Disassembly_h_

#include "CViewDisassembly.h"

class CViewC64Disassembly : public CViewDisassembly
{
public:
	CViewC64Disassembly(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					 CDebugSymbols *symbols, CViewDataDump *viewDataDump, CViewMemoryMap *viewMemoryMap);
	virtual void GetDisassemblyBackgroundColor(float *colorR, float *colorG, float *colorB);
};

#endif
