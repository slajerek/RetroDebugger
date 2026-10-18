#ifndef _C64BankingModes_h_
#define _C64BankingModes_h_

#include "SYS_Defs.h"

// RD-028: the C64 PLA's 32 memory configurations, mode = (EXROM << 4) |
// (GAME << 3) | ($01 & 7), each giving what the CPU sees in the five
// switchable regions. One table for the views that show a configuration
// from a recorded VIC-II cycle state (the memory bank view, the VIC editor's
// memory access layer); the live machine's answer is the mapping resolver's,
// and CTestC64MemoryMapping checks this table against it for every mode.
enum C64BankEntry : u8
{
	C64BANK_RAM,
	C64BANK_BASIC_ROM,
	C64BANK_KERNAL_ROM,
	C64BANK_CHAR_ROM,
	C64BANK_IO,
	C64BANK_CART_LO,
	C64BANK_CART_HI,
	C64BANK_OPEN		// Ultimax: nothing answers, the CPU reads the bus
};

enum
{
	C64BANK_REGION_8000,	// $8000-$9FFF
	C64BANK_REGION_A000,	// $A000-$BFFF
	C64BANK_REGION_C000,	// $C000-$CFFF
	C64BANK_REGION_D000,	// $D000-$DFFF
	C64BANK_REGION_E000,	// $E000-$FFFF
	C64BANK_NUM_REGIONS
};

int C64BankingMode(u8 exrom, u8 game, u8 port);
C64BankEntry C64BankingEntry(int mode, int region);
int C64BankingRegion(u16 addr);		// -1 below $8000
bool C64BankingIsUltimax(int mode);		// $1000-$7FFF open too
const char *C64BankEntryName(C64BankEntry entry);
const char *C64BankRegionName(int region);

#endif
