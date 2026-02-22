#include "CTestEmulatorStartup.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "SYS_Main.h"

void CTestEmulatorStartup::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	int step = 0;
	bool allPassed = true;

#ifdef RUN_COMMODORE64
	step++;
	if (viewC64->debugInterfaceC64 != NULL)
	{
		StepCompleted(step, true, "C64 debug interface initialized");
	}
	else
	{
		StepCompleted(step, false, "C64 debug interface is NULL");
		allPassed = false;
	}
#endif

#ifdef RUN_ATARI
	step++;
	if (viewC64->debugInterfaceAtari != NULL)
	{
		StepCompleted(step, true, "Atari debug interface initialized");
	}
	else
	{
		StepCompleted(step, false, "Atari debug interface is NULL");
		allPassed = false;
	}
#endif

#ifdef RUN_NES
	step++;
	if (viewC64->debugInterfaceNes != NULL)
	{
		StepCompleted(step, true, "NES debug interface initialized");
	}
	else
	{
		StepCompleted(step, false, "NES debug interface is NULL");
		allPassed = false;
	}
#endif

	if (allPassed)
	{
		TestCompleted(true, "All enabled emulators initialized successfully");
	}
	else
	{
		TestCompleted(false, "One or more emulator interfaces failed to initialize");
	}
}

void CTestEmulatorStartup::Cancel()
{
	isRunning = false;
}
