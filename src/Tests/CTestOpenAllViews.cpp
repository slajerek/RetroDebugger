#include "CTestOpenAllViews.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "SYS_Funct.h"

void CTestOpenAllViews::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	int step = 0;
	bool allPassed = true;
	int viewsTested = 0;

	// 1. Save initial emulator running states and start all emulators
	std::vector<bool> wasRunning;
	for (auto *di : viewC64->debugInterfaces)
	{
		wasRunning.push_back(di->isRunning);
		if (!di->isRunning)
		{
			viewC64->StartEmulationThread(di);
		}
	}

	// 2. Wait for emulators to initialize
	SYS_Sleep(2000);

	// 3. For each emulator, show each view, render it, restore visibility
	for (auto *di : viewC64->debugInterfaces)
	{
		if (!di->isRunning)
			continue;

		step++;
		int viewCount = 0;

		for (auto *view : di->views)
		{
			bool wasVisible = view->visible;
			view->SetVisible(true);
			view->RenderImGui();
			view->SetVisible(wasVisible);
			viewCount++;
			viewsTested++;
		}

		char msg[256];
		snprintf(msg, sizeof(msg), "%s: rendered %d views", di->GetPlatformNameString(), viewCount);
		StepCompleted(step, true, msg);
	}

	// 4. Restore emulator running states
	for (size_t i = 0; i < viewC64->debugInterfaces.size(); i++)
	{
		CDebugInterface *di = viewC64->debugInterfaces[i];
		if (di->isRunning && !wasRunning[i])
			viewC64->StopEmulationThread(di);
	}

	char summary[256];
	snprintf(summary, sizeof(summary), "Rendered %d views across all emulators without crashes", viewsTested);
	TestCompleted(allPassed, summary);
}

void CTestOpenAllViews::Cancel()
{
	isRunning = false;
}
