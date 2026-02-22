#pragma once

#include "ITestCallback.h"
#include <vector>
#include <string>
using namespace std;

class CTest;

struct CTestSuiteResult
{
	string testName;
	bool success;
	string summary;
};

// Runs all automated tests in sequence
class CTestSuite : public ITestCallback
{
public:
	CTestSuite();
	virtual ~CTestSuite();

	// Quick start static method for one-liner usage
	static void QuickStart();

	// CLI mode: register all known tests, optionally filtered by name
	static void RegisterAllTests(CTestSuite *suite);
	static void RunFromCLI(const char *testName);

	// True when CLI test mode is active
	static bool isCLIModeActive;

	void Run();
	void Cancel();

	// ITestCallback
	virtual void OnTestStepCompleted(CTest *test, int stepId, bool success, const char *message) override;
	virtual void OnTestCompleted(CTest *test, bool success, const char *summary) override;

private:
	void RunNextTest();
	void WriteResults();

	vector<CTest *> tests;
	vector<CTestSuiteResult> results;
	int currentTestIndex;
	bool isRunning;
	bool exitOnCompletion;
	string resultsFilePath;
};
