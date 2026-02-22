#include "SYS_Defs.h"
#include "CTest.h"

CTest::CTest()
{
	currentStep = 0;
	isRunning = false;
	callback = NULL;
}

CTest::~CTest()
{
}

void CTest::StepCompleted(int stepId, bool success, const char *message)
{
	if (callback)
	{
		callback->OnTestStepCompleted(this, stepId, success, message);
	}
}

void CTest::TestCompleted(bool success, const char *summary)
{
	isRunning = false;
	if (callback)
	{
		callback->OnTestCompleted(this, success, summary);
	}
}
