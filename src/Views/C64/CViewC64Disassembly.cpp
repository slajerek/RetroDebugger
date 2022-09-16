#include "CViewC64Disassembly.h"
#include "CViewC64.h"
#include "CViewC64StateVIC.h"
#include "C64SettingsStorage.h"

CViewC64Disassembly::CViewC64Disassembly(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
				 CDebugSymbols *symbols, CViewDataDump *viewDataDump, CViewMemoryMap *viewMemoryMap)
: CViewDisassembly(name, posX, posY, posZ, sizeX, sizeY, symbols, viewDataDump, viewMemoryMap)
{
}

void CViewC64Disassembly::GetDisassemblyBackgroundColor(float *colorR, float *colorG, float *colorB)
{
	if (viewC64->viewC64StateVIC->isLockedState
		&& viewC64->viewC64Disassembly->changedByUser == false)
	{
		*colorR = 0.20f; *colorG = 0.0f; *colorB = 0.0f;
	}
	else
	{
		GetColorsFromScheme(c64SettingsDisassemblyBackgroundColor, colorR, colorG, colorB);
	}

}

