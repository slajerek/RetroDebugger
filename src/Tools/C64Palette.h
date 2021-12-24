#ifndef _C64PALETTE_H_
#define _C64PALETTE_H_

#include "SYS_Defs.h"
#include <vector>

class CSlrString;

class C64PaletteData
{
public:
	C64PaletteData(char *paletteName, uint8 *palette);
	char *paletteName;
	uint8 *palette;
};

void C64InitPalette();
void C64SetPaletteNum(uint16 paletteNum);
void C64SetPalette(uint8 *palette);
void C64SetVicePalette(uint8 *palette);
void C64SetPalette(char *paletteName);
void C64GetAvailablePalettes(std::vector<CSlrString *> *vicPalettes);
void C64SetPaletteOriginalColorCodes();

std::vector<C64PaletteData *> *C64GetAvailablePalettes();

#endif


