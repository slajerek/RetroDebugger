#pragma once

#include "CTest.h"

// RD-024: the logical vs physical identity contract (CDebugMemoryMapping.h),
// proven on a fake resolver so it holds before any machine provider exists:
// fetch/read/write resolve separately, the same PC can be two identities,
// aliases come back from ResolveCpuWindows, Unknown and Unmapped are honest
// answers that never compare equal, and the mapping epoch moves only when the
// observed state changes. Pure logic, no emulator.
class CTestMemoryMappingContract : public CTest
{
public:
	virtual const char *GetName() override { return "MemoryMappingContract"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }
};
