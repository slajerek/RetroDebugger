#pragma once

#include "CTest.h"

class CTestMoonshineDragons : public CTest
{
public:
	virtual const char *GetName() override { return "MoonshineDragons"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
