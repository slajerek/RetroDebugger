#pragma once

#include "ITestCallback.h"
#include <vector>
#include <string>
using namespace std;

class CTest;

struct CTestResult
{
	string testName;
	bool success;
	string summary;
};

class CTestRunner : public ITestCallback
{
public:
	CTestRunner();
	virtual ~CTestRunner();

	void RunTest(CTest *test);
	void CancelCurrentTest();

	bool IsRunning();
	CTest *GetCurrentTest() { return currentTest; }

	// Static flag to prevent interference when test is scheduled
	static bool isTestPending;
	static bool IsTestPending() { return isTestPending; }

	// ITestCallback
	virtual void OnTestStepCompleted(CTest *test, int stepId, bool success, const char *message) override;
	virtual void OnTestCompleted(CTest *test, bool success, const char *summary) override;

	// Results
	vector<CTestResult> results;

private:
	CTest *currentTest;
};
