#pragma once

#include "CTest.h"

class CTestViceCmdlinePassthrough : public CTest
{
public:
	virtual const char *GetName() override { return "ViceCmdlinePassthrough"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
