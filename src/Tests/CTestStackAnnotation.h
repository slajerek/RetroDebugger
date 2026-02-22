#pragma once

#include "CTest.h"

class CTestStackAnnotation : public CTest
{
public:
	virtual const char *GetName() override { return "StackAnnotation"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;

private:
	bool TestC64Stack();
	bool TestAtariStack();
	bool TestNesStack();
};
