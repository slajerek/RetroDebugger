#pragma once

#include "CTest.h"

// Regression test for a jump requested while the debugger is paused on a CPU
// breakpoint. MakeJmpC64 must commit the new PC before the next single step, so
// that the step executes at the requested target and not at the pre-jump PC.
class CTestViceJumpWhilePaused : public CTest
{
public:
	virtual const char *GetName() override { return "ViceJumpWhilePaused"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
