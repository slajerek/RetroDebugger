#include "CTestSnapshotBoundary.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CByteBuffer.h"
#include "SYS_Main.h"
#include "DebuggerDefs.h"
#include <cstdio>

static char failureMsg[512];

// Loop A at $8000: LDA #$A1 / STA $4000 / JMP $8000
static const u16 LOOP_A_ADDR = 0x8000;
static const u8  LOOP_A_CODE[] = { 0xA9, 0xA1, 0x8D, 0x00, 0x40, 0x4C, 0x00, 0x80 };
// Loop B at $9000: LDA #$B2 / STA $4000 / JMP $9000
static const u16 LOOP_B_ADDR = 0x9000;
static const u8  LOOP_B_CODE[] = { 0xA9, 0xB2, 0x8D, 0x00, 0x40, 0x4C, 0x00, 0x90 };
static const u16 MARKER_ADDR = 0x4000;
static const u32 BOUNDARY_TIMEOUT_MS = 5000;

static void InstallLoop(CDebugInterfaceC64 *di, u16 addr, const u8 *code, int len)
{
	for (int i = 0; i < len; i++)
		di->SetByteToRamC64(addr + i, code[i]);
}

static bool WaitForMarker(CDebugInterfaceC64 *di, u8 expected, int timeoutMs)
{
	for (int t = 0; t < timeoutMs; t += 50)
	{
		if (di->GetByteFromRamC64(MARKER_ADDR) == expected)
			return true;
		SYS_Sleep(50);
	}
	return di->GetByteFromRamC64(MARKER_ADDR) == expected;
}

void CTestSnapshotBoundary::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

#ifndef RUN_COMMODORE64
	TestSkipped("C64 not enabled - boundary snapshot path not exercised");
	return;
#else
	CDebugInterfaceC64 *di = viewC64->debugInterfaceC64;
	if (!di)
	{
		TestCompleted(false, "C64 debug interface is NULL");
		return;
	}

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

	if (di->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(100);
	}

	bool allPassed = true;
	CByteBuffer *snapshotBuffer = new CByteBuffer();

	// --- Step 1: run loop A, confirm it executes ---
	{
		InstallLoop(di, LOOP_A_ADDR, LOOP_A_CODE, sizeof(LOOP_A_CODE));
		di->SetByteToRamC64(MARKER_ADDR, 0x00);
		di->MakeJmpC64(LOOP_A_ADDR);

		if (!WaitForMarker(di, 0xA1, 2000))
		{
			sprintf(failureMsg, "loop A did not run: $4000=$%02X",
					di->GetByteFromRamC64(MARKER_ADDR));
			allPassed = false;
		}
		StepCompleted(1, allPassed, "loop A running, $4000=$A1");
	}

	// --- Step 2: save snapshot from the test thread (the MCP situation) ---
	if (allPassed)
	{
		bool saved = di->SaveChipsSnapshotAtCpuBoundary(snapshotBuffer, BOUNDARY_TIMEOUT_MS);
		if (!saved || snapshotBuffer->length == 0)
		{
			sprintf(failureMsg, "SaveChipsSnapshotAtCpuBoundary failed (saved=%d len=%d)",
					saved, (int)snapshotBuffer->length);
			allPassed = false;
		}
		StepCompleted(2, allPassed, "snapshot saved at CPU boundary");
	}

	// --- Step 3: run loop B, confirm it executes ---
	if (allPassed)
	{
		InstallLoop(di, LOOP_B_ADDR, LOOP_B_CODE, sizeof(LOOP_B_CODE));
		di->MakeJmpC64(LOOP_B_ADDR);

		if (!WaitForMarker(di, 0xB2, 2000))
		{
			sprintf(failureMsg, "loop B did not run: $4000=$%02X",
					di->GetByteFromRamC64(MARKER_ADDR));
			allPassed = false;
		}
		if (allPassed)
		{
			int pc = di->GetCpuPC();
			if (!(pc >= LOOP_B_ADDR && pc <= LOOP_B_ADDR + 7))
			{
				sprintf(failureMsg, "PC=$%04X not in loop B before load", pc);
				allPassed = false;
			}
		}
		StepCompleted(3, allPassed, "loop B running, $4000=$B2");
	}

	// --- Step 4: load snapshot from the test thread; CPU must resume loop A ---
	if (allPassed)
	{
		bool loaded = di->LoadChipsSnapshotAtCpuBoundary(snapshotBuffer, BOUNDARY_TIMEOUT_MS);
		if (!loaded)
		{
			sprintf(failureMsg, "LoadChipsSnapshotAtCpuBoundary returned false");
			allPassed = false;
		}
		StepCompleted(4, allPassed, "snapshot loaded at CPU boundary");
	}

	// --- Step 5: live PC must be back in loop A ---
	// GetCpuPC() returns viceCurrentC64PC, refreshed from the CPU loop's local
	// reg_pc every instruction — this observes the genuinely executing CPU.
	if (allPassed)
	{
		SYS_Sleep(100); // let the CPU thread run a few restored instructions
		int pc = di->GetCpuPC();
		if (!(pc >= LOOP_A_ADDR && pc <= LOOP_A_ADDR + 7))
		{
			sprintf(failureMsg, "after load PC=$%04X, expected $8000-$8007 (CPU kept pre-load state)", pc);
			allPassed = false;
		}
		StepCompleted(5, allPassed, "PC restored into loop A");
	}

	// --- Step 6: continued execution writes $A1 (loop A), never $B2 (loop B) ---
	if (allPassed)
	{
		di->SetByteToRamC64(MARKER_ADDR, 0x00);
		if (!WaitForMarker(di, 0xA1, 2000))
		{
			sprintf(failureMsg, "after load $4000=$%02X, expected $A1 (loop A executing)",
					di->GetByteFromRamC64(MARKER_ADDR));
			allPassed = false;
		}
		StepCompleted(6, allPassed, "restored program continues executing");
	}

	// --- Step 7: load while PAUSED — must succeed via the 16ms pause-loop
	// poll, restore PC, and remain paused (no instruction executed) ---
	if (allPassed)
	{
		// diverge again: run loop B so the pre-load state differs
		InstallLoop(di, LOOP_B_ADDR, LOOP_B_CODE, sizeof(LOOP_B_CODE));
		di->MakeJmpC64(LOOP_B_ADDR);
		WaitForMarker(di, 0xB2, 2000);

		di->PauseEmulationBlockedWait();

		bool loaded = di->LoadChipsSnapshotAtCpuBoundary(snapshotBuffer, BOUNDARY_TIMEOUT_MS);
		if (!loaded)
		{
			sprintf(failureMsg, "paused LoadChipsSnapshotAtCpuBoundary returned false");
			allPassed = false;
		}

		if (allPassed && di->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			sprintf(failureMsg, "emulator did not stay paused across boundary load (mode=%d)",
					di->GetDebugMode());
			allPassed = false;
		}

		if (allPassed)
		{
			SYS_Sleep(100); // let the pause-loop iteration finish the register import
			int pc = di->GetCpuPC();
			if (!(pc >= LOOP_A_ADDR && pc <= LOOP_A_ADDR + 7))
			{
				sprintf(failureMsg, "paused load: PC=$%04X, expected $8000-$8007", pc);
				allPassed = false;
			}
		}
		StepCompleted(7, allPassed, "paused boundary load restored PC, stayed paused");
	}

	// --- Step 8: resume — restored program must continue with loop A ---
	if (allPassed)
	{
		di->SetByteToRamC64(MARKER_ADDR, 0x00);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);

		if (!WaitForMarker(di, 0xA1, 2000))
		{
			sprintf(failureMsg, "after paused load + resume $4000=$%02X, expected $A1",
					di->GetByteFromRamC64(MARKER_ADDR));
			allPassed = false;
		}
		StepCompleted(8, allPassed, "resumed into restored loop A");
	}

	delete snapshotBuffer;

	// leave a clean machine for the next test
	di->ResetHard();
	SYS_Sleep(500);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);

	if (!wasRunning)
	{
		viewC64->StopEmulationThread(di);
	}

	if (allPassed)
		TestCompleted(true, "MCP-style snapshot save/load at CPU boundary verified (running + paused)");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestSnapshotBoundary::Cancel()
{
}
