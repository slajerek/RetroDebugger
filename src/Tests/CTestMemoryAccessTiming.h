#pragma once

#include "CTest.h"

class CTestMemoryAccessTiming : public CTest
{
public:
	virtual const char *GetName() override { return "MemoryAccessTiming"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
