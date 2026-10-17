#pragma once

#include "CTest.h"
#include <string>

class CDebugSymbols;

// RD-031 and RD-032: the format of a symbols / debug-info file is decided by
// its content (cc65 and KickAssembler both write .dbg), using the committed
// Tier 1 artifacts of every producer; DebugInfoLoadFile routes each format to
// its loader and never feeds a cc65 or Oscar64 file to the KickAssembler
// parser; and loading a KickAssembler .dbg twice without clearing first (the
// menu import) replaces the segments instead of leaving stale duplicates.
class CTestDebugInfoFormats : public CTest
{
public:
	virtual const char *GetName() override { return "DebugInfoFormats"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const char *message);
	bool RunProbe();
	bool RunRouting(CDebugSymbols *symbols);
	bool RunReimport(CDebugSymbols *symbols);
};
