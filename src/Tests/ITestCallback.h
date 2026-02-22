#pragma once

class CTest;

class ITestCallback
{
public:
	virtual ~ITestCallback() {}
	virtual void OnTestStepCompleted(CTest *test, int stepId, bool success, const char *message) = 0;
	virtual void OnTestCompleted(CTest *test, bool success, const char *summary) = 0;
};
