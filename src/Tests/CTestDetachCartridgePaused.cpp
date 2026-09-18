#include "CTestDetachCartridgePaused.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
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

// Any CRT that maps ROM at $8000 works; this one ships with the repo.
static std::string TestCrtPath() { return CTest::ResolveProjectPath("docs/tests/eob v1.00 20221121.crt"); }
#define TEST_CRT_PATH TestCrtPath().c_str()

// EXROM/GAME as reported by c64d_get_exrom_game(): 1 means the line is
// inactive (pulled high), i.e. nothing is driving it, i.e. no cartridge.
static bool IsCartridgeMapped(CDebugInterfaceVice *di)
{
	C64StateCartridge state;
	state.exrom = 1;
	state.game = 1;
	di->GetC64CartridgeState(&state);
	return (state.exrom == 0 || state.game == 0);
}

#endif

void CTestDetachCartridgePaused::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulator is not compiled in (RUN_COMMODORE64 undefined), so the paused cartridge detach was never exercised");
	return;
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!di)
	{
		TestCompleted(false, "C64 debug interface is NULL");
		return;
	}

	FILE *f = fopen(TEST_CRT_PATH, "rb");
	if (!f)
	{
		sprintf(failureMsg, "Test cartridge not found: %s", TEST_CRT_PATH);
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

	// Attaching stores the cartridge path in the settings; keep the user's
	// value out of the way and put it back at the end.
	guiMain->LockMutex();
	CSlrString *savedPathToCartridge = c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = NULL;
	guiMain->UnlockMutex();

	int savedDebugMode = di->GetDebugMode();

	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(500);

	bool allPassed = true;

	// --- Step 1: attach the cartridge ---
	if (allPassed)
	{
		CSlrString *path = new CSlrString(TEST_CRT_PATH);
		di->AttachCartridge(path);
		delete path;

		// Poll rather than trust one fixed sleep: the attach runs on the
		// emulation thread and a loaded runner (library load + power cycle)
		// can easily exceed 500ms, which is how the CI arm64 leg failed here.
		// Deadline is generous; a passing attach maps EXROM/GAME almost
		// immediately, so the poll exits early.
		bool attached = IsCartridgeMapped(di);
		for (int i = 0; i < 50 && !attached; i++)
		{
			SYS_Sleep(100);
			attached = IsCartridgeMapped(di);
		}
		

		if (!attached)
		{
			allPassed = false;
			sprintf(failureMsg, "Step 1 FAIL: EXROM/GAME still inactive after AttachCartridge(%s)", TEST_CRT_PATH);
			StepCompleted(1, false, failureMsg);
		}
		else
		{
			StepCompleted(1, true, "Cartridge attached, EXROM/GAME driven");
		}
	}

	// --- Step 2: pause the machine, the way a user debugging it would ---
	if (allPassed)
	{
		di->SetDebugMode(DEBUGGER_MODE_PAUSED);
		SYS_Sleep(500);

		u64 cyclesBefore = di->GetMainCpuCycleCounter();
		SYS_Sleep(300);
		u64 cyclesAfter = di->GetMainCpuCycleCounter();

		if (di->GetDebugMode() != DEBUGGER_MODE_PAUSED || cyclesAfter != cyclesBefore)
		{
			allPassed = false;
			sprintf(failureMsg, "Step 2 FAIL: machine did not stop (debugMode=%d, cycles %llu -> %llu)",
					di->GetDebugMode(), (unsigned long long)cyclesBefore, (unsigned long long)cyclesAfter);
			StepCompleted(2, false, failureMsg);
		}
		else
		{
			StepCompleted(2, true, "Machine paused, CPU is not executing");
		}
	}

	// --- Step 3: detach while paused -- this is the reported bug ---
	//
	// The old implementation queued a maincpu trap here. With the CPU parked in
	// the debugger pause loop the trap never ran, so the cartridge stayed
	// mapped: exactly the "CTRL+SHIFT+0 does nothing" report.
	if (allPassed)
	{
		di->DetachCartridge();

		// Poll for the detach like step 1 polls for the attach: the trap
		// runs on the emulation thread and one fixed sleep raced it on the
		// CI runners.
		bool detached = !IsCartridgeMapped(di);
		for (int i = 0; i < 50 && !detached; i++)
		{
			SYS_Sleep(100);
			detached = !IsCartridgeMapped(di);
		}

		if (!detached)
		{
			C64StateCartridge state;
			state.exrom = 1;
			state.game = 1;
			di->GetC64CartridgeState(&state);
			allPassed = false;
			sprintf(failureMsg, "Step 3 FAIL: cartridge still mapped after DetachCartridge() while paused (exrom=%d game=%d)",
					(int)state.exrom, (int)state.game);
			StepCompleted(3, false, failureMsg);
		}
		else
		{
			StepCompleted(3, true, "Cartridge detached while the machine was paused");
		}
	}

	// --- Step 4: the detach did not resume the machine behind the user's back ---
	//
	// ResetHardSynced() documents the rule this follows: a paused user is
	// debugging, so a synchronous action must not silently start the CPU.
	if (allPassed)
	{
		if (di->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			allPassed = false;
			sprintf(failureMsg, "Step 4 FAIL: detach resumed the machine (debugMode=%d, expected %d)",
					di->GetDebugMode(), DEBUGGER_MODE_PAUSED);
			StepCompleted(4, false, failureMsg);
		}
		else
		{
			StepCompleted(4, true, "Machine is still paused after the detach");
		}
	}

	// --- Step 5: detaching while RUNNING still works (the trap route) ---
	if (allPassed)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(500);

		// The resume must actually resume: verify the cycle counter moves
		// and keep the observed state for the failure message, because the
		// CI leg runs with logging off and a bare "FAIL" cannot tell a
		// paused machine from a broken attach.
		u64 cyclesBefore = di->GetMainCpuCycleCounter();
		SYS_Sleep(300);
		u64 cyclesAfter = di->GetMainCpuCycleCounter();
		int debugModeAtProbe = di->GetDebugMode();

		CSlrString *path = new CSlrString(TEST_CRT_PATH);
		di->AttachCartridge(path);
		delete path;

		bool attached = IsCartridgeMapped(di);
		for (int i = 0; i < 50 && !attached; i++)
		{
			SYS_Sleep(100);
			attached = IsCartridgeMapped(di);
		}

		if (!attached)
		{
			u64 cyclesNow = di->GetMainCpuCycleCounter();
			C64StateCartridge state;
			state.exrom = 1;
			state.game = 1;
			di->GetC64CartridgeState(&state);
			allPassed = false;
			sprintf(failureMsg, "Step 5 FAIL: cartridge could not be re-attached for the running-machine case (debugMode=%d probe=%d, cycles %llu->%llu now %llu, exrom=%d game=%d)",
					debugModeAtProbe, di->GetDebugMode(),
					(unsigned long long)cyclesBefore, (unsigned long long)cyclesAfter, (unsigned long long)cyclesNow,
					(int)state.exrom, (int)state.game);
			StepCompleted(5, false, failureMsg);
		}
		else
		{
			di->DetachCartridge();

			bool detachedWhileRunning = !IsCartridgeMapped(di);
			for (int i = 0; i < 50 && !detachedWhileRunning; i++)
			{
				SYS_Sleep(100);
				detachedWhileRunning = !IsCartridgeMapped(di);
			}

			if (!detachedWhileRunning)
			{
				allPassed = false;
				sprintf(failureMsg, "Step 5 FAIL: cartridge still mapped after DetachCartridge() while running");
				StepCompleted(5, false, failureMsg);
			}
			else
			{
				StepCompleted(5, true, "Cartridge detached while the machine was running");
			}
		}
	}

	// Restore emulator and settings state
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(200);
	if (IsCartridgeMapped(di))
		di->DetachCartridge();
	SYS_Sleep(200);

	if (savedDebugMode != DEBUGGER_MODE_RUNNING)
		di->SetDebugMode(savedDebugMode);

	guiMain->LockMutex();
	if (c64SettingsPathToCartridge)
	{
		delete c64SettingsPathToCartridge;
		c64SettingsPathToCartridge = NULL;
	}
	c64SettingsPathToCartridge = savedPathToCartridge;
	guiMain->UnlockMutex();

	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "Detach Cartridge works whether the machine is paused or running");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestDetachCartridgePaused::Cancel()
{
	isRunning = false;
}
