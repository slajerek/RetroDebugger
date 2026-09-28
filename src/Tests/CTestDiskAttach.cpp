#include "CTestDiskAttach.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceVice.h"
#include "CDebuggerApi.h"
#include "CDataAdapterViceDrive1541DiskContents.h"
#include "C64SettingsStorage.h"
#include "SYS_Main.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <string>

#ifdef RUN_COMMODORE64
static const int MARKER_ADDR = 0xC000;
static std::string TestD64Path() { return CTest::ResolveProjectPath("tests/data/bitbreaker.d64"); }
#endif

void CTestDiskAttach::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulator is not compiled in (RUN_COMMODORE64 undefined)");
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!di)
	{
		TestCompleted(false, "C64 debug interface is NULL");
		return;
	}

	std::string diskPath = TestD64Path();
	FILE *f = fopen(diskPath.c_str(), "rb");
	if (!f)
	{
		TestCompleted(false, "DiskAttach fixture not found");
		return;
	}
	fclose(f);

	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	if (!di->isRunning)
	{
		TestCompleted(false, "C64 emulator failed to start");
		return;
	}
	if (di->dataAdapterViceDrive1541DiskContents->IsDiskAttached())
	{
		if (!wasRunning)
			viewC64->StopEmulationThread(di);
		TestSkipped("A disk is already mounted; refusing to replace user media");
		return;
	}

	CDebuggerApi *api = di->GetDebuggerApi();
	if (!api)
	{
		if (!wasRunning)
			viewC64->StopEmulationThread(di);
		TestCompleted(false, "C64 debugger API is NULL");
		return;
	}

	uint8 previousMode = di->GetDebugMode();
	di->SetDebugMode(DEBUGGER_MODE_PAUSED);
	SYS_Sleep(100);
	u16 pcBefore;
	u8 aBefore, xBefore, yBefore, pBefore, spBefore;
	di->GetCpuRegs(&pcBefore, &aBefore, &xBefore, &yBefore, &pBefore, &spBefore);
	u64 cyclesBefore = di->GetMainCpuCycleCounter();
	unsigned int frameBefore = di->GetEmulationFrameNumber();
	u8 previousMarker = di->GetByteFromRamC64(MARKER_ADDR);
	di->SetByteToRamC64(MARKER_ADDR, 0xA5);
	bool previousAutoRun = c64SettingsAutoJmpFromInsertedDiskFirstPrg;
	c64SettingsAutoJmpFromInsertedDiskFirstPrg = true;

	bool allPassed = true;
	if (api->AttachDriveDisk(diskPath.c_str(), 9) || api->AttachDriveDisk("", 8)
		|| api->AttachDriveDisk((diskPath + ".missing").c_str(), 8)
		|| di->dataAdapterViceDrive1541DiskContents->IsDiskAttached())
	{
		allPassed = false;
		StepCompleted(1, false, "Invalid drive/path was accepted or changed media");
	}
	else
		StepCompleted(1, true, "Invalid drive and missing/empty paths rejected");

	if (allPassed)
	{
		if (!api->AttachDriveDisk(diskPath.c_str(), 8)
			|| !di->dataAdapterViceDrive1541DiskContents->IsDiskAttached())
		{
			allPassed = false;
			StepCompleted(2, false, "D64 did not mount on device 8");
		}
		else
			StepCompleted(2, true, "D64 mounted on device 8");
	}

	// VICE's failed replacement path is not atomic for an existing invalid image.
	// This test covers a valid D64 and early-rejected inputs, not that known bug.
	if (allPassed)
	{
		u16 pcAfter;
		u8 aAfter, xAfter, yAfter, pAfter, spAfter;
		di->GetCpuRegs(&pcAfter, &aAfter, &xAfter, &yAfter, &pAfter, &spAfter);
		bool preserved = di->GetDebugMode() == DEBUGGER_MODE_PAUSED
			&& pcAfter == pcBefore && aAfter == aBefore && xAfter == xBefore
			&& yAfter == yBefore && pAfter == pBefore && spAfter == spBefore
			&& di->GetMainCpuCycleCounter() == cyclesBefore
			&& di->GetEmulationFrameNumber() == frameBefore
			&& di->GetByteFromRamC64(MARKER_ADDR) == 0xA5;
		if (!preserved)
		{
			allPassed = false;
			StepCompleted(3, false, "Paused C64 registers, counters or RAM changed during attach");
		}
		else
			StepCompleted(3, true, "Paused CPU/RAM/counters preserved despite autorun setting");
	}

	if (di->dataAdapterViceDrive1541DiskContents->IsDiskAttached())
		di->DetachDriveDisk(8);
	bool detached = !di->dataAdapterViceDrive1541DiskContents->IsDiskAttached();
	if (!detached)
	{
		allPassed = false;
		StepCompleted(4, false, "Test fixture could not be detached");
	}
	else
		StepCompleted(4, true, "Fixture detached; original no-media state restored");

	c64SettingsAutoJmpFromInsertedDiskFirstPrg = previousAutoRun;
	di->SetByteToRamC64(MARKER_ADDR, previousMarker);
	di->SetDebugMode(previousMode);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	TestCompleted(allPassed, allPassed ? "Disk image attaches without reset or autorun"
		: "Disk attach-only contract failed");
#endif
}

void CTestDiskAttach::Cancel()
{
	isRunning = false;
}
