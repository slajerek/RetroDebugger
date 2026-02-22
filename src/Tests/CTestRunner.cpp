#include "CTestRunner.h"
#include "CTest.h"
#include "SYS_Main.h"

bool CTestRunner::isTestPending = false;

CTestRunner::CTestRunner()
{
	currentTest = NULL;
}

CTestRunner::~CTestRunner()
{
	if (currentTest)
	{
		delete currentTest;
	}
}

void CTestRunner::RunTest(CTest *test)
{
	if (currentTest)
	{
		currentTest->Cancel();
		delete currentTest;
	}

	currentTest = test;
	LOGI("CTestRunner: Starting test '%s'", test->GetName());
	test->Run(this);
}

void CTestRunner::CancelCurrentTest()
{
	if (currentTest)
	{
		LOGI("CTestRunner: Cancelling test '%s'", currentTest->GetName());
		currentTest->Cancel();
		delete currentTest;
		currentTest = NULL;
	}
}

bool CTestRunner::IsRunning()
{
	return currentTest != NULL && currentTest->IsRunning();
}

void CTestRunner::OnTestStepCompleted(CTest *test, int stepId, bool success, const char *message)
{
	if (success)
	{
		LOGI("CTestRunner: [%s] Step %d completed: %s", test->GetName(), stepId, message);
	}
	else
	{
		LOGError("CTestRunner: [%s] Step %d FAILED: %s", test->GetName(), stepId, message);
	}
}

void CTestRunner::OnTestCompleted(CTest *test, bool success, const char *summary)
{
	CTestResult result;
	result.testName = test->GetName();
	result.success = success;
	result.summary = summary;
	results.push_back(result);

	if (success)
	{
		LOGI("CTestRunner: [%s] TEST PASSED: %s", test->GetName(), summary);
	}
	else
	{
		LOGError("CTestRunner: [%s] TEST FAILED: %s", test->GetName(), summary);
	}

	delete currentTest;
	currentTest = NULL;
	isTestPending = false;
}
