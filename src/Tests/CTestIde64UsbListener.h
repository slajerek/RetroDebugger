#pragma once

#include "CTest.h"

class CTestIde64UsbListener : public CTest
{
public:
	virtual const char *GetName() override { return "Ide64UsbListener"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
