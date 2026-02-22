#pragma once

#include "ITestCallback.h"

class CTest
{
public:
	CTest();
	virtual ~CTest();

	virtual const char *GetName() = 0;
	virtual void Run(ITestCallback *callback) = 0;
	virtual void Cancel() = 0;

	bool IsRunning() { return isRunning; }

protected:
	void StepCompleted(int stepId, bool success, const char *message);
	void TestCompleted(bool success, const char *summary);

	int currentStep;
	bool isRunning;
	ITestCallback *callback;
};
