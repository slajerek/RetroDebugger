#pragma once

#include "CTest.h"
#include "SYS_Defs.h"
#include "json.hpp"
#include <string>

// RD-022: C variables from the debug info, resolved at real stops and
// compared with the values the programs compute. Oscar64 (C64): a global,
// zero-page parameters at a function's entry, a static-stack local inside
// and outside its lines, a frame-relative parameter through a recursion
// (missing in the prologue), a local of another function out of scope.
// cc65: a global and a stack local, approximate with their reasons.
class CTestSourceVariables : public CTest
{
public:
	virtual const char *GetName() override { return "SourceVariables"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
	bool Load(const std::string &compiler, const std::string &fixture, const char *debugInfo);
	bool RunTo(u16 entry, u16 breakpoint, bool launch);
	nlohmann::json Variable(const std::string &name);
};
