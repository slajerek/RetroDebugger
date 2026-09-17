#include "CTestViceJumpWhilePaused.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointAddr.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

// The bug this covers: c64d_set_c64_pc() defers the actual PC write to a VICE CPU
// trap, and that trap is only dispatched at the top of the main CPU loop — ABOVE
// the pause point a CPU breakpoint parks on. Resuming for a single step therefore
// used to fetch the next opcode from the stale reg_pc.
//
// The park point matters: pausing via PauseEmulationBlockedWait() parks at the
// END of the loop body, where the next iteration dispatches the trap before the
// fetch, and the bug does NOT show. This test must park on a CPU breakpoint.

// Loop we break in, at $1000:
#define JMPPAUSE_LOOP_ADDR		0x1000
#define JMPPAUSE_BREAK_ADDR		0x1002		// $1002: JMP $1001
static const u8 loopCode[] = {
	0x78,					// $1000: SEI          (keep IRQs out of the way)
	0xEA,					// $1001: NOP
	0x4C, 0x01, 0x10,		// $1002: JMP $1001
};

// Jump target, at $2000. PHP is a single-byte instruction, so one step from
// $2000 must land on $2001 — an address the pre-jump loop can never produce.
#define JMPPAUSE_TARGET_ADDR	0x2000
#define JMPPAUSE_AFTER_STEP		0x2001
static const u8 targetCode[] = {
	0x08,					// $2000: PHP
	0xEA,					// $2001: NOP
};

static char failureMsg[512];

void CTestViceJumpWhilePaused::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

#ifndef RUN_COMMODORE64
	TestCompleted(true, "Skipped (C64 not enabled)");
	return;
#else
	CDebugInterfaceC64 *di = (CDebugInterfaceC64 *)viewC64->debugInterfaceC64;
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

	bool allPassed = true;
	bool breakpointAdded = false;

	CDebugSymbolsSegment *segment = di->symbolsC64->currentSegment;
	if (!segment)
	{
		TestCompleted(false, "C64 symbols segment is NULL");
		return;
	}

	C64StateCPU cpuState;

	// --- Step 1: park the CPU on a real CPU breakpoint ---
	{
		di->PauseEmulationBlockedWait();

		for (int i = 0; i < (int)sizeof(loopCode); i++)
			di->SetByteToRamC64(JMPPAUSE_LOOP_ADDR + i, loopCode[i]);

		for (int i = 0; i < (int)sizeof(targetCode); i++)
			di->SetByteToRamC64(JMPPAUSE_TARGET_ADDR + i, targetCode[i]);

		segment->AddBreakpointPC(JMPPAUSE_BREAK_ADDR);
		breakpointAdded = true;

		di->MakeJmpC64(JMPPAUSE_LOOP_ADDR);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);

		int tries = 0;
		while (tries < 300)
		{
			SYS_Sleep(10);
			if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
				break;
			tries++;
		}

		// Let the emulation thread actually park in the pause loop. Deliberately NOT
		// PauseEmulationBlockedWait(), which would step an instruction and move the
		// park point to the one that never had the bug.
		SYS_Sleep(200);

		if (di->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			sprintf(failureMsg, "Breakpoint at $%04X did not pause the emulator", JMPPAUSE_BREAK_ADDR);
			allPassed = false;
		}
		else
		{
			di->GetC64CpuState(&cpuState);
			if (cpuState.pc != JMPPAUSE_BREAK_ADDR)
			{
				sprintf(failureMsg, "Parked at PC=$%04X, expected $%04X", cpuState.pc, JMPPAUSE_BREAK_ADDR);
				allPassed = false;
			}
		}

		if (allPassed)
			StepCompleted(1, true, "Paused on CPU breakpoint at $1002");
		else
			StepCompleted(1, false, failureMsg);
	}

	// --- Step 2: jump while paused, and require a committed jump ---
	if (allPassed)
	{
		bool applied = di->MakeJmpC64(JMPPAUSE_TARGET_ADDR);

		if (!applied)
		{
			sprintf(failureMsg, "MakeJmpC64($%04X) reported the jump as still queued while paused",
					JMPPAUSE_TARGET_ADDR);
			allPassed = false;
		}
		else
		{
			di->GetC64CpuState(&cpuState);
			if (cpuState.pc != JMPPAUSE_TARGET_ADDR)
			{
				sprintf(failureMsg, "After jump, CPU status reports PC=$%04X, expected $%04X",
						cpuState.pc, JMPPAUSE_TARGET_ADDR);
				allPassed = false;
			}
		}

		if (di->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			sprintf(failureMsg, "MakeJmpC64 did not preserve the paused state (mode=$%02X)", di->GetDebugMode());
			allPassed = false;
		}

		if (allPassed)
			StepCompleted(2, true, "Jump to $2000 committed while paused, still paused");
		else
			StepCompleted(2, false, failureMsg);
	}

	// --- Step 3: the single step must execute at the requested target ---
	if (allPassed)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);

		int tries = 0;
		while (tries < 200)
		{
			SYS_Sleep(10);
			if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
				break;
			tries++;
		}

		if (di->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			sprintf(failureMsg, "Emulator did not pause after the single step");
			allPassed = false;
		}
		else
		{
			di->GetC64CpuState(&cpuState);
			if (cpuState.pc != JMPPAUSE_AFTER_STEP)
			{
				// $1001 is what the unfixed code produces: the JMP $1001 at the
				// pre-jump PC was executed instead of the PHP at $2000.
				sprintf(failureMsg, "Step executed at the wrong PC: now $%04X, expected $%04X%s",
						cpuState.pc, JMPPAUSE_AFTER_STEP,
						(cpuState.pc == 0x1001) ? " (stepped at the pre-jump PC $1002)" : "");
				allPassed = false;
			}
		}

		if (allPassed)
			StepCompleted(3, true, "Single step executed at $2000, PC now $2001");
		else
			StepCompleted(3, false, failureMsg);
	}

	// Restore emulator state
	if (breakpointAdded)
	{
		auto it = segment->breakpointsPC->breakpoints.find(JMPPAUSE_BREAK_ADDR);
		if (it != segment->breakpointsPC->breakpoints.end())
		{
			segment->breakpointsPC->RemoveBreakpoint(it->second);
		}
	}

	if (!wasRunning)
	{
		di->PauseEmulationBlockedWait();
		viewC64->StopEmulationThread(di);
	}

	if (allPassed)
		TestCompleted(true, "Jump while paused commits before the next single step");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestViceJumpWhilePaused::Cancel()
{
	isRunning = false;
}
