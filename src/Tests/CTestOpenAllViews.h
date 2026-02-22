#pragma once

#include "CTest.h"

class CTestOpenAllViews : public CTest
{
public:
	virtual const char *GetName() override { return "OpenAllViews"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
