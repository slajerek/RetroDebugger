#include "CTestViceJumpWhilePaused.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebuggerApi.h"
#include "CDebuggerServerApi.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointAddr.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>
#ifdef RUN_COMMODORE64
#include "ViceWrapper.h"
#endif

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

// A third-clock JMP self-loop reproduces the counter-restore park that used to
// consume a PC request and then overwrite it with the already-fetched operand.
#define MIDJUMP_LOOP_ADDR		0x1100
#define MIDJUMP_TARGET_ADDR		0x2100
#define MIDJUMP_BREAK_ADDR		0x2105
#define MIDJUMP_MARKER_ADDR		0x4000
static const u8 midLoopCode[] = { 0x4C, 0x00, 0x11 }; // JMP $1100
static const u8 midTargetCode[] = {
	0xA9, 0x5A,				// $2100: LDA #$5A
	0x8D, 0x00, 0x40,		// $2102: STA $4000
	0x4C, 0x05, 0x21,		// $2105: JMP $2105 (break before dispatch)
};

#ifdef RUN_COMMODORE64
static bool WaitForJumpPark(CDebugInterfaceC64 *di, u16 pc, int instructionCycle)
{
	for (int tries = 0; tries < 200; tries++)
	{
		SYS_Sleep(10);
		C64StateCPU state;
		di->GetC64CpuState(&state);
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED
			&& c64d_is_cpu_parked_in_pause_loop() && state.pc == pc
			&& state.instructionCycle == instructionCycle)
			return true;
	}
	return false;
}
#endif

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
	bool midBreakpointAdded = false;

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

	// --- Steps 4-5: a mid-instruction request must remain queued, then survive
	// both Continue and completing the suspended instruction with Single Step.
	// The original three breakpoint/single-step checks above remain unchanged.
	if (allPassed)
	{
		for (int i = 0; i < (int)sizeof(midLoopCode); i++)
			di->SetByteToRamC64(MIDJUMP_LOOP_ADDR + i, midLoopCode[i]);
		for (int i = 0; i < (int)sizeof(midTargetCode); i++)
			di->SetByteToRamC64(MIDJUMP_TARGET_ADDR + i, midTargetCode[i]);
		segment->AddBreakpointPC(MIDJUMP_BREAK_ADDR);
		midBreakpointAdded = true;
	}
	for (int finishWithStep = 0; finishWithStep < 2 && allPassed; finishWithStep++)
	{
		di->SetByteToRamC64(MIDJUMP_MARKER_ADDR, 0);
		if (!di->MakeJmpC64(MIDJUMP_LOOP_ADDR) || !WaitForJumpPark(di, MIDJUMP_LOOP_ADDR, 0))
		{
			sprintf(failureMsg, "Could not commit setup PC at the safe boundary");
			allPassed = false;
		}
		for (int cycle = 1; cycle <= 3 && allPassed; cycle++)
		{
			di->SetDebugMode(DEBUGGER_MODE_RUN_ONE_CYCLE);
			if (!WaitForJumpPark(di, MIDJUMP_LOOP_ADDR, cycle))
			{
				sprintf(failureMsg, "Could not park JMP $1100 at instruction cycle %d", cycle);
				allPassed = false;
			}
		}
		if (allPassed)
		{
			C64StateCPU beforeState;
			di->GetC64CpuState(&beforeState);
			u64 beforeCounter = di->GetMainCpuCycleCounter();
			unsigned int beforeFrame = di->GetEmulationFrameNumber();
			bool applied = di->MakeJmpC64(MIDJUMP_TARGET_ADDR);
			// Cover both a notified wake and repeated 16ms timeout/backstop wakes.
			SYS_Sleep(80);
			di->GetC64CpuState(&cpuState);
			if (applied || !c64d_is_pc_change_pending()
				|| di->GetDebugMode() != DEBUGGER_MODE_PAUSED
				|| cpuState.pc != MIDJUMP_LOOP_ADDR || cpuState.instructionCycle != 3
				|| cpuState.a != beforeState.a || cpuState.x != beforeState.x
				|| cpuState.y != beforeState.y || cpuState.sp != beforeState.sp
				|| cpuState.processorFlags != beforeState.processorFlags
				|| di->GetDebuggerServerApi()->debuggerApi->GetCpuStatusJson().value("pendingPc", -1) != MIDJUMP_TARGET_ADDR
				|| di->GetMainCpuCycleCounter() != beforeCounter
				|| di->GetEmulationFrameNumber() != beforeFrame
				|| di->GetByteFromRamC64(MIDJUMP_MARKER_ADDR) != 0)
			{
				sprintf(failureMsg, "Cycle-3 jump must remain queued with actual PC/clocks/RAM unchanged");
				allPassed = false;
			}
		}
		if (allPassed && finishWithStep)
		{
			// This step finishes the old JMP; it must not execute target code yet.
			di->SetDebugMode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
			if (!WaitForJumpPark(di, MIDJUMP_TARGET_ADDR, 0)
				|| c64d_is_pc_change_pending()
				|| di->GetByteFromRamC64(MIDJUMP_MARKER_ADDR) != 0)
			{
				sprintf(failureMsg, "Completing the suspended JMP did not commit the queued target");
				allPassed = false;
			}
		}
		if (allPassed)
		{
			di->SetDebugMode(DEBUGGER_MODE_RUNNING);
			if (!WaitForJumpPark(di, MIDJUMP_BREAK_ADDR, 0)
				|| c64d_is_pc_change_pending()
				|| di->GetByteFromRamC64(MIDJUMP_MARKER_ADDR) != 0x5A)
			{
				sprintf(failureMsg, "Queued jump lost on resume: target marker/breakpoint not reached");
				allPassed = false;
			}
		}
		StepCompleted(4 + finishWithStep, allPassed, allPassed
			? "Cycle-3 queued jump survives deliberate execution and reaches target code" : failureMsg);
	}

	// Restore emulator state
	if (midBreakpointAdded)
	{
		if (!allPassed)
			di->PauseEmulationBlockedWait();
		auto it = segment->breakpointsPC->breakpoints.find(MIDJUMP_BREAK_ADDR);
		if (it != segment->breakpointsPC->breakpoints.end())
			segment->breakpointsPC->RemoveBreakpoint(it->second);
	}
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
		TestCompleted(true, "Boundary jump commits before stepping; cycle-3 jump queues and survives resume/step");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestViceJumpWhilePaused::Cancel()
{
	isRunning = false;
}
