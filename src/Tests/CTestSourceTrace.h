#pragma once

#include "CTest.h"
#include <string>

// RD-045: the executed source lines (CDebugSourceTraceRecorder), recorded
// from live boundaries on the C64 and from the C64 Ultimate's decoded bus
// trace, replayed: synthetic bus cycles of a three-line loop give the loop's
// lines, marked replayed, on a machine that cannot step. The C64U reports no
// stepping in its capabilities and refuses a source step; it has its own
// source view.
class CTestSourceTrace : public CTest
{
public:
	virtual const char *GetName() override { return "SourceTrace"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
	bool RunLiveC64();
	bool RunReplayedC64U();
};
