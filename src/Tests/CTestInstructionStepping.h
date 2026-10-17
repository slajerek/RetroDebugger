#pragma once

#include "CTest.h"
#include "SYS_Defs.h"
#include <functional>
#include <string>

class CDebugInterface;

// RD-011 / RD-037: execution events and instruction step-over/step-out.
// The boundary classifier is checked on its own; then on each machine a
// recursive subroutine is stepped over from inside its first frame (where
// the old PC+3 breakpoint stopped one frame too deep) and stepped out of.
// On the C64 also: a user breakpoint inside the subroutine wins over an armed
// step-over and ends it, and an IRQ handler that runs through the return
// address does not stop the step there.
class CTestInstructionStepping : public CTest
{
public:
	virtual const char *GetName() override { return "InstructionStepping"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const char *message);

	bool RunClassifier();
	bool RunRecursion(CDebugInterface *di, const char *machine, u16 base, u8 counterZp,
					  std::function<void(u16)> jump);
	bool RunC64();
	bool RunAtari();
	bool RunNes();
};
