#pragma once

#include "CTest.h"
#include <string>

// RD-059: a data watch keeps the memory its bytes are in. On the C64 a watch
// added at $E000 binds to the RAM under the KERNAL, shows that RAM while the
// KERNAL is mapped in (marked as not mapped in) and when it is out; an
// unbound watch (an older watch file) still shows what the CPU sees; an I/O
// address binds to nothing; the binding is saved and loaded with the watch.
// The Atari extended RAM banks are in AtariXeBanks, NES PRG banks in
// BankedSource.
class CTestBankedWatches : public CTest
{
public:
	virtual const char *GetName() override { return "BankedWatches"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
};
