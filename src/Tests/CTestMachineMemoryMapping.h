#pragma once

#include "CTest.h"
#include <string>

// RD-040 and RD-029: the NES and Atari memory mapping providers.
// NES: a synthesized four-bank UxROM ROM (Tier 0) proves the same PC as two
// PRG identities after a mapper write, the fixed last bank, RAM mirrors as
// aliases, writes to ROM space reaching the mapper, and no cartridge as
// Unknown. Atari: RAM/ROM/hardware from the emulator, writes to ROM dropped,
// OS and BASIC switched by PORTB, and -- on a 130XE-style machine -- the
// CPU and ANTIC seeing different banks at $4000.
class CTestMachineMemoryMapping : public CTest
{
public:
	virtual const char *GetName() override { return "MachineMemoryMapping"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const char *message);
	bool RunNes();
	bool RunAtari();
};
