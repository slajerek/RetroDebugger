#pragma once

#include "CTest.h"
#include <string>

// RD-028: the C64 memory mapping provider against the 6510 port table of
// spec #3B.2 (no cartridge): every $01 configuration's view of $A000, $D000
// and $E000, fetch vs write routing (writes under ROM reach RAM, I/O writes
// stay I/O), the same PC as two identities, ROM aliases, the CPU port, and
// the mapping epoch. Drives VICE: the emulator is paused while $00/$01 are
// switched and restored afterwards. RD-028/RD-030: banked cartridges --
// Ocean and EasyFlash .crt files written by the test, attached, banks
// switched through $DE00: storage, bank offset and .crt file offset.
class CTestC64MemoryMapping : public CTest
{
	bool RunBankedCartridges(int &stepNum, std::string &failed);
public:
	virtual const char *GetName() override { return "C64MemoryMapping"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }
};
