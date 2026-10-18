#pragma once

#include "CTest.h"
#include <string>

// RD-029/RD-030 (spec #3B.3, 130XE): a synthesized bank-switch program in
// base RAM calls $4000 in extended bank 1, then in bank 3 -- the same PC,
// two physical RAM bytes. A PC breakpoint bound to bank 3's byte (remote
// cpu/breakpoint/add with "physical") stops only when bank 3 is in; ANTIC
// keeps base RAM while the CPU has a bank; the direct-RAM adapter reads the
// base RAM under a bank. An XL/XE without 128 KB is cold-started as a 130XE
// and put back at the end.
class CTestAtariXeBanks : public CTest
{
public:
	virtual const char *GetName() override { return "AtariXeBanks"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
};
