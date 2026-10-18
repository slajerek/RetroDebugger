#pragma once

#include "CTest.h"
#include <string>

// RD-007 + RD-025: banked code. A synthetic database with two banks at one
// address resolves by image offset only where they overlap; on the NES, the
// cc65 UxROM "banked" fixture (bankA in bank 0, bankB in bank 1, both at
// $8000) stops on a source breakpoint only in its own bank: bankB's line
// once, bankA's line on each of its two calls, never on the other bank's
// code at the same address.
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

	bool RunDatabase();
	bool RunNes();
};
