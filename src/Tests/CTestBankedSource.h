#pragma once

#include "CTest.h"
#include "SYS_Defs.h"
#include <string>

// RD-007 + RD-025: banked code. A synthetic database with two banks at one
// address resolves by image offset only where they overlap; on the NES, the
// cc65 "banked" fixture, built for UxROM and for MMC1 (bankA in bank 0,
// bankB in bank 1, both at $8000), stops on a source breakpoint only in its own bank: bankB's line
// once, bankA's line on each of its two calls, never on the other bank's
// code at the same address. The C64 runs the same checks on a Magic Desk
// cartridge build (banks 0 and 1 at $9000, .crt file offsets), the Atari on
// an XEGS 32 KB build (banks 0 and 1 at $8000, .car file offsets).
class CTestBankedSource : public CTest
{
public:
	virtual const char *GetName() override { return "BankedSource"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);

	struct NesCartridge
	{
		const char *mapper;
		const char *machine;		// the build: tests/data/source-debug/cc65/<machine>/banked
		const char *source;
		int lineA, lineB;
		u16 resultAddress;
	};
	bool RunDatabase();
	bool RunNes(const NesCartridge &cart);
	bool RunC64();
	bool RunAtari();
};
