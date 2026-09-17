#pragma once

#include "CTest.h"

class CTestIde64Settings : public CTest
{
public:
	virtual const char *GetName() override { return "Ide64Settings"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
