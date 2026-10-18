#pragma once

#include "CTest.h"
#include "SYS_Defs.h"
#include <string>

class CDebugInterfaceVice;

// RD-015: the step-trace runner. Replays every trace listed in
// tests/source-debug/expectations/index.json (spec #9.5 format plus the
// machine, the Tier 1 artifacts, the entry point and an optional IRQ): loads
// the binary and its debug info into scratch symbols, runs to the start line
// through a source breakpoint, then issues each source step and compares
// (file, line, function, depth, reason). A failing trace writes the actual
// trace next to the expected one to /tmp/source-step-trace-<compiler>-<fixture>.txt.
class CTestSourceStepTrace : public CTest
{
public:
	virtual const char *GetName() override { return "SourceStepTrace"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);

	bool RunTrace(CDebugInterfaceVice *di, const std::string &path);
};
