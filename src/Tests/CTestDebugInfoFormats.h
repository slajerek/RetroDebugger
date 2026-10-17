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
// Also RD-032: SetSegment activates fully (next/previous continue from it),
// both <Watchpoints> layouts, and VICE "al D:" labels reaching the 1541.
// RD-008 / RD-009: the Oscar64 .dbj and cc65 .dbg loaders on the Tier 1
// artifacts of all machines.
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
	bool RunSegmentsAndWatches(CDebugSymbols *symbols);
	bool RunDriveLabels(CDebugSymbols *symbols);
	bool RunOscar64(CDebugSymbols *symbols);
	bool RunCc65(CDebugSymbols *symbols);
};
