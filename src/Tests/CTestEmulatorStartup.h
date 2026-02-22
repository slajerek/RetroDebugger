#pragma once

#include "CTest.h"

class CTestEmulatorStartup : public CTest
{
public:
	virtual const char *GetName() override { return "EmulatorStartup"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
