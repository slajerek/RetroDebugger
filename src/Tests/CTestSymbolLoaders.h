#pragma once

#include "CTest.h"
#include <string>

class CDebugSymbols;

// RD-038 (source-level debugging, docs/specs/source-level-debugging): pins
// what the EXISTING symbol loaders do today, so the canonical debug database
// migration (RD-006/RD-031/RD-032) can prove it changed nothing by keeping
// this test green:
//   - VICE .vs labels (assets/tests/labelstest1.vs) and 64tass "NAME = $addr"
//     labels, both through CDebugSymbols::ParseSymbols;
//   - KickAssembler .dbg XML (assets/tests/c64/dankwars.dbg) through
//     CDebugSymbols::ParseSourceDebugInfo: segments, the per-segment
//     address -> file:line:column map, labels and PC breakpoints;
//   - source text loading for a .dbg whose source exists
//     (assets/tests/symbols/simple.asm), including tab expansion.
// Parses into a scratch CDebugSymbolsC64 swapped in for the run, so the live
// C64 symbols (and every other test) are untouched. Quirks of today's loaders
// are asserted AS THEY ARE and commented; a later change to one of them must
// update the assertion on purpose.
class CTestSymbolLoaders : public CTest
{
public:
	virtual const char *GetName() override { return "SymbolLoaders"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	bool Check(bool condition, const char *message);
	bool RunViceLabels(CDebugSymbols *symbols);
	bool Run64TassLabels(CDebugSymbols *symbols);
	bool RunKickAssDbg(CDebugSymbols *symbols);
	bool RunKickAssSourceText(CDebugSymbols *symbols);
	std::string failure;
};
