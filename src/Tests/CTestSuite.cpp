#include "CTestSuite.h"
#include "CTestRunner.h"
#include "CTestEmulatorStartup.h"
#include "CTestVicEditorZoom.h"
#include "CTestVicEditorCursor.h"
#include "CTestOpenAllViews.h"
#include "CTestStackAnnotation.h"
#include "CTestMemoryAccessTiming.h"
#include "CViewC64.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>
using namespace std;

bool CTestSuite::isCLIModeActive = false;

CTestSuite::CTestSuite()
{
	currentTestIndex = -1;
	isRunning = false;
	exitOnCompletion = false;
}

CTestSuite::~CTestSuite()
{
	for (auto test : tests)
	{
		delete test;
	}
	tests.clear();
}

void CTestSuite::RegisterAllTests(CTestSuite *suite)
{
	suite->tests.push_back(new CTestEmulatorStartup());
	suite->tests.push_back(new CTestVicEditorZoom());
	suite->tests.push_back(new CTestVicEditorCursor());
	suite->tests.push_back(new CTestOpenAllViews());
	suite->tests.push_back(new CTestStackAnnotation());
	suite->tests.push_back(new CTestMemoryAccessTiming());
}

void CTestSuite::RunFromCLI(const char *testName)
{
	isCLIModeActive = true;
	CTestRunner::isTestPending = true;

	CTestSuite *suite = new CTestSuite();
	suite->exitOnCompletion = true;
	suite->resultsFilePath = "tests/results/last_run.txt";

	if (testName != NULL)
	{
		// Register all tests then filter to matching name
		RegisterAllTests(suite);

		vector<CTest *> filtered;
		for (auto test : suite->tests)
		{
			if (strcmp(test->GetName(), testName) == 0)
			{
				filtered.push_back(test);
			}
			else
			{
				delete test;
			}
		}
		suite->tests = filtered;

		if (suite->tests.empty())
		{
			LOGError("CTestSuite::RunFromCLI: No test found with name '%s'", testName);

			// Write failure results
			FILE *f = fopen(suite->resultsFilePath.c_str(), "w");
			if (f)
			{
				fprintf(f, "[%s] FAIL: Test not found\n", testName);
				fprintf(f, "---\n");
				fprintf(f, "RESULT: 0/1 passed\n");
				fclose(f);
			}

			CTestRunner::isTestPending = false;
			SYS_Shutdown();
			delete suite;
			return;
		}
	}
	else
	{
		RegisterAllTests(suite);
	}

	if (viewC64->testRunner == NULL)
	{
		viewC64->testRunner = new CTestRunner();
	}

	suite->Run();
}

void CTestSuite::QuickStart()
{
	// Set flag to prevent interference
	CTestRunner::isTestPending = true;

	CTestSuite *suite = new CTestSuite();

	// Add tests in order
	suite->tests.push_back(new CTestEmulatorStartup());

	// Ensure test runner exists
	if (viewC64->testRunner == NULL)
	{
		viewC64->testRunner = new CTestRunner();
	}

	suite->Run();
}

void CTestSuite::Run()
{
	LOGM("CTestSuite::Run: Starting test suite with %d tests", (int)tests.size());
	isRunning = true;
	currentTestIndex = -1;
	results.clear();
	RunNextTest();
}

void CTestSuite::Cancel()
{
	LOGM("CTestSuite::Cancel");
	isRunning = false;

	if (currentTestIndex >= 0 && currentTestIndex < (int)tests.size())
	{
		tests[currentTestIndex]->Cancel();
	}
}

void CTestSuite::RunNextTest()
{
	if (!isRunning)
		return;

	currentTestIndex++;

	if (currentTestIndex >= (int)tests.size())
	{
		LOGM("CTestSuite: All tests completed");
		CTestRunner::isTestPending = false;
		isRunning = false;

		if (exitOnCompletion)
		{
			WriteResults();

			int passed = 0;
			for (auto &r : results)
			{
				if (r.success) passed++;
			}
			LOGM("SUITE RESULTS: %d/%d passed", passed, (int)results.size());
			SYS_Shutdown();
		}
		return;
	}

	CTest *test = tests[currentTestIndex];
	LOGM("CTestSuite: Running test %d/%d: %s", currentTestIndex + 1, (int)tests.size(), test->GetName());
	test->Run(this);
}

void CTestSuite::OnTestStepCompleted(CTest *test, int stepId, bool success, const char *message)
{
	LOGM("CTestSuite: [%s] Step %d %s: %s", test->GetName(), stepId, success ? "OK" : "FAILED", message);
}

void CTestSuite::OnTestCompleted(CTest *test, bool success, const char *summary)
{
	LOGM("CTestSuite: [%s] Completed %s: %s", test->GetName(), success ? "OK" : "FAILED", summary);

	results.push_back({test->GetName(), success, summary});

	if (!success)
	{
		LOGM("CTestSuite: Test failed, stopping suite");
		CTestRunner::isTestPending = false;
		isRunning = false;

		if (exitOnCompletion)
		{
			WriteResults();

			int passed = 0;
			for (auto &r : results)
			{
				if (r.success) passed++;
			}
			LOGM("SUITE RESULTS: %d/%d passed", passed, (int)results.size());
			SYS_Shutdown();
		}
		return;
	}

	// Run next test
	RunNextTest();
}

void CTestSuite::WriteResults()
{
	FILE *f = fopen(resultsFilePath.c_str(), "w");
	if (!f)
	{
		LOGError("CTestSuite::WriteResults: Failed to open %s", resultsFilePath.c_str());
		return;
	}

	int passed = 0;
	for (auto &r : results)
	{
		fprintf(f, "[%s] %s: %s\n", r.testName.c_str(), r.success ? "PASS" : "FAIL", r.summary.c_str());
		if (r.success) passed++;
	}
	fprintf(f, "---\n");
	fprintf(f, "RESULT: %d/%d passed\n", passed, (int)results.size());
	fclose(f);

	LOGM("CTestSuite: Results written to %s", resultsFilePath.c_str());
}
