#pragma once

#include "CTest.h"

// RD-006: the canonical debug database on its own, no emulator. Later
// mappings win where ranges overlap (split, not dropped), ranges are half-open,
// regions are independent, every covering mapping stays a candidate, the
// reverse index finds ranges by file:line, and file names come out of both
// '/' and '\' paths.
class CTestDebugDatabase : public CTest
{
public:
	virtual const char *GetName() override { return "DebugDatabase"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }
};
