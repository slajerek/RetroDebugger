#pragma once

#include "CTest.h"

class CTestViceNetworkSocket : public CTest
{
public:
	virtual const char *GetName() override { return "ViceNetworkSocket"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
