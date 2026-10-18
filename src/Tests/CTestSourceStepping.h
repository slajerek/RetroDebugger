#pragma once

#include "CTest.h"
#include "SYS_Defs.h"
#include <string>

enum class SourceStepKind : u8;
enum class SourceStepStopReason : u8;
class CDebugInterfaceVice;

// RD-012 / RD-013: the physical frame tracker and source-level Step
// Into/Over/Out. The tracker is checked on synthetic boundaries (calls,
// returns, an IRQ right after a JSR, a dropped return address). Then the
// Oscar64 basic-flow fixture runs on the C64 with its .dbj loaded and a
// 150-cycle IRQ whose handler calls the fixture's own add() -- mapped code the
// step must not stop in -- and a scripted sequence of source steps has to
// land on the expected lines with the expected stop reasons. Then RD-014:
// source breakpoints resolve (and slide) to addresses, stop the program,
// survive a reload and an unload, and never delete a user's own breakpoint.
class CTestSourceStepping : public CTest
{
public:
	virtual const char *GetName() override { return "SourceStepping"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const char *message);

	bool RunTracker();
	bool RunOscar64(CDebugInterfaceVice *di);
	bool RunSourceBreakpoints(CDebugInterfaceVice *di);
	bool CheckStaleDebugInfo();
	bool RunResetCancelsStep(CDebugInterfaceVice *di);
	bool Step(CDebugInterfaceVice *di, SourceStepKind kind, u16 expectedPc, int expectedLine,
			  SourceStepStopReason expectedReason, u64 budget = 0);
};
