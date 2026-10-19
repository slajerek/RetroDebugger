#pragma once

#include "CTest.h"
#include <string>

// RD-055: cc65 stack depth decoding against the running program, and its
// cost. Each committed cc65 C64 fixture runs with a breakpoint at every
// function's prologue end and every C line start; at a prologue end the C
// stack pointer of that activation (keyed by the 6502 S) is the ground
// truth an `auto` offset counts from, so at each later line start of the
// activation every local's true address is that sp + offs. Every address
// the Limited and Full decoders call exact must be it; the counts of exact,
// approximate and wrong answers per mode are logged. The benchmark times
// each mode on the fixtures' functions and on a large synthetic function
// (branches, helper calls) and fails above a budget.
class CTestCc65StackDepth : public CTest
{
public:
	virtual const char *GetName() override { return "Cc65StackDepth"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
};
