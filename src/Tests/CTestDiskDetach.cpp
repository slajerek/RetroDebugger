#include "CTestDiskDetach.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "CDebuggerApi.h"
#include "CDataAdapterViceDrive1541DiskContents.h"
#include "C64SettingsStorage.h"
#include "CSlrString.h"
#include "CGuiMain.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

static char failureMsg[512];

#ifdef RUN_COMMODORE64

// Through the engine's resolver: right from the git root and from a
// --package run alike.
static std::string TestD64Path() { return CTest::ResolveProjectPath("tests/data/bitbreaker.d64"); }
#define TEST_D64_PATH TestD64Path().c_str()

// Free RAM under BASIC ROM — nothing the KERNAL touches while it sits at the
// READY prompt, and wiped by a power cycle, which is exactly the difference
// this test is looking for.
static const int MARKER_ADDR = 0xC000;
static const int MARKER_SIZE = 8;
static const u8 MARKER[MARKER_SIZE] = { 0xA5, 0x5A, 0xC6, 0x4D, 0x11, 0x22, 0x33, 0x44 };

static bool MarkerIsIntact(CDebugInterfaceVice *di)
{
	for (int i = 0; i < MARKER_SIZE; i++)
	{
		if (di->GetByteFromRamC64(MARKER_ADDR + i) != MARKER[i])
			return false;
	}
	return true;
}

#endif

void CTestDiskDetach::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulator is not compiled in (RUN_COMMODORE64 undefined)");
	return;
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!di)
	{
		TestCompleted(false, "C64 debug interface is NULL");
		return;
	}

	// The D64 has to be there — without it there is nothing to detach and the
	// test would be checking nothing at all.
	FILE *f = fopen(TEST_D64_PATH, "rb");
	if (!f)
	{
		sprintf(failureMsg, "Test disk image not found: %s", TEST_D64_PATH);
		TestCompleted(false, failureMsg);
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

	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(500);

	// Detaching clears the stored D64 path and persists settings, exactly like
	// the GUI action does. Take the user's value out of the way so the test
	// does not throw it away, and put it back at the end.
	guiMain->LockMutex();
	CSlrString *savedPathToD64 = c64SettingsPathToD64;
	c64SettingsPathToD64 = NULL;
	guiMain->UnlockMutex();

	bool allPassed = true;

	// --- Step 1: attach the disk image ---
	if (allPassed)
	{
		CSlrString *path = new CSlrString(TEST_D64_PATH);
		di->InsertD64(path);
		delete path;
		SYS_Sleep(200);

		if (!di->dataAdapterViceDrive1541DiskContents->IsDiskAttached())
		{
			allPassed = false;
			sprintf(failureMsg, "Step 1 FAIL: disk not attached after InsertD64(%s)", TEST_D64_PATH);
			StepCompleted(1, false, failureMsg);
		}
		else
		{
			StepCompleted(1, true, "Disk image attached to unit 8");
		}
	}

	// --- Step 2: mark RAM so a reset becomes visible ---
	if (allPassed)
	{
		for (int i = 0; i < MARKER_SIZE; i++)
			di->SetByteToRamC64(MARKER_ADDR + i, MARKER[i]);
		SYS_Sleep(100);

		if (!MarkerIsIntact(di))
		{
			allPassed = false;
			sprintf(failureMsg, "Step 2 FAIL: RAM marker at $%04X could not be written", MARKER_ADDR);
			StepCompleted(2, false, failureMsg);
		}
		else
		{
			StepCompleted(2, true, "RAM marker written");
		}
	}

	CDebuggerApi *api = di->GetDebuggerApi();
	if (allPassed && !api)
	{
		allPassed = false;
		sprintf(failureMsg, "C64 debugger API is NULL");
		StepCompleted(3, false, failureMsg);
	}

	// --- Step 3: out-of-range device numbers are rejected, disk stays put ---
	if (allPassed)
	{
		// VICE units are 8..11; anything else must be refused rather than
		// forwarded to file_system_detach_disk()
		bool rejectedLow = !api->DetachDriveDisk(7);
		bool rejectedHigh = !api->DetachDriveDisk(12);
		SYS_Sleep(100);
		bool stillAttached = di->dataAdapterViceDrive1541DiskContents->IsDiskAttached();

		if (!rejectedLow || !rejectedHigh || !stillAttached)
		{
			allPassed = false;
			sprintf(failureMsg, "Step 3 FAIL: invalid device numbers not rejected (7 rejected=%d, 12 rejected=%d, disk still attached=%d)",
					rejectedLow ? 1 : 0, rejectedHigh ? 1 : 0, stillAttached ? 1 : 0);
			StepCompleted(3, false, failureMsg);
		}
		else
		{
			StepCompleted(3, true, "Device numbers outside 8..11 rejected, disk untouched");
		}
	}

	// --- Step 4: detach unit 8 and check the machine survived ---
	unsigned int frameBefore = 0;
	if (allPassed)
	{
		frameBefore = di->GetEmulationFrameNumber();

		if (!api->DetachDriveDisk(8))
		{
			allPassed = false;
			sprintf(failureMsg, "Step 4 FAIL: DetachDriveDisk(8) returned false");
			StepCompleted(4, false, failureMsg);
		}
		else
		{
			SYS_Sleep(300);

			bool detached = !di->dataAdapterViceDrive1541DiskContents->IsDiskAttached();
			bool markerIntact = MarkerIsIntact(di);
			unsigned int frameAfter = di->GetEmulationFrameNumber();
			// DetachEverything() power-cycles and zeroes the frame counter;
			// this action must not.
			bool frameCounterKept = (frameAfter >= frameBefore);

			if (!detached || !markerIntact || !frameCounterKept)
			{
				allPassed = false;
				sprintf(failureMsg, "Step 4 FAIL: detached=%d ramPreserved=%d frames %u -> %u (a reset happened)",
						detached ? 1 : 0, markerIntact ? 1 : 0, frameBefore, frameAfter);
				StepCompleted(4, false, failureMsg);
			}
			else
			{
				StepCompleted(4, true, "Disk detached, RAM and frame counter preserved (no reset)");
			}
		}
	}

	// --- Step 5: the emulation is still running afterwards ---
	if (allPassed)
	{
		u64 cyclesBefore = di->GetMainCpuCycleCounter();
		SYS_Sleep(300);
		u64 cyclesAfter = di->GetMainCpuCycleCounter();

		if (cyclesAfter == cyclesBefore)
		{
			allPassed = false;
			sprintf(failureMsg, "Step 5 FAIL: CPU cycle counter stuck at %llu after detach",
					(unsigned long long)cyclesBefore);
			StepCompleted(5, false, failureMsg);
		}
		else
		{
			StepCompleted(5, true, "CPU keeps executing after the detach");
		}
	}

	// Restore emulator and settings state
	if (di->dataAdapterViceDrive1541DiskContents->IsDiskAttached())
		di->DetachDriveDisk();

	guiMain->LockMutex();
	if (c64SettingsPathToD64)
	{
		delete c64SettingsPathToD64;
		c64SettingsPathToD64 = NULL;
	}
	c64SettingsPathToD64 = savedPathToD64;
	guiMain->UnlockMutex();
	C64DebuggerStoreSettings();

	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "Detach Disk Image removes the disk without resetting the machine");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestDiskDetach::Cancel()
{
	isRunning = false;
}
