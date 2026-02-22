#include "CTestMemoryAccessTiming.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

extern "C"
{
#include "ViceWrapper.h"
};

// Test programs injected at $1000:
//
// Program A — STA absolute (opcode $8D, no indexing):
//   $1000: SEI
//   $1001: LDA #$42
//   $1003: STA $0400     ; 4 cycles, only cycle 4 is the actual write
//   $1006: JMP $1003     ; loop: writes $0400 every 7 cycles
//
// Program B — STA absolute,X with X=0 (opcode $9D, has dummy read):
//   $1000: SEI
//   $1001: LDX #$00
//   $1003: LDA #$42
//   $1005: STA $0400,X   ; 5 cycles, cycle 4 = dummy read, cycle 5 = write
//   $1008: JMP $1005     ; loop
//
// The test verifies that:
//   - Program A: ALL marked cycles are writes (no bogus reads)
//   - Program B: marked cycles include only writes (after the fix),
//                or both reads and writes (before the fix)

static const u8 testCodeA[] = {
	0x78,                   // $1000: SEI
	0xA9, 0x42,             // $1001: LDA #$42
	0x8D, 0x00, 0x04,       // $1003: STA $0400
	0x4C, 0x03, 0x10,       // $1006: JMP $1003
};

static const u8 testCodeB[] = {
	0x78,                   // $1000: SEI
	0xA2, 0x00,             // $1001: LDX #$00
	0xA9, 0x42,             // $1003: LDA #$42
	0x9D, 0x00, 0x04,       // $1005: STA $0400,X
	0x4C, 0x05, 0x10,       // $1008: JMP $1005
};

static char failureMsg[512];

static bool RunTestProgram(CDebugInterfaceC64 *di, const char *label,
						   const u8 *code, int codeLen,
						   int *outTotalMarked, int *outWriteMarked, int *outReadMarked)
{
	// Clear watch table
	memset(c64d_mem_access_watch, 0, sizeof(c64d_mem_access_watch));

	// Watch address $0400
	c64d_mem_access_watch[0x0400] = 1;

	// Pause emulator
	di->PauseEmulationBlockedWait();

	// Write test code at $1000
	for (int i = 0; i < codeLen; i++)
	{
		di->SetByteToRamC64(0x1000 + i, code[i]);
	}

	// Set PC to $1000
	di->MakeJmpC64(0x1000);

	// Run for 2 frames (~40ms at 50Hz PAL)
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(80);
	di->PauseEmulationBlockedWait();

	// Scan per-cycle state array
	int totalMarked = 0;
	int writeMarked = 0;
	int readMarked = 0;

	for (int rasterLine = 0; rasterLine < 312; rasterLine++)
	{
		for (int rasterCycle = 0; rasterCycle < 63; rasterCycle++)
		{
			vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);
			if (state->memAccessAddr == 0x0400)
			{
				totalMarked++;
				if (state->memAccessIsWrite)
					writeMarked++;
				else
					readMarked++;
			}
		}
	}

	// Clear watch table
	memset(c64d_mem_access_watch, 0, sizeof(c64d_mem_access_watch));

	*outTotalMarked = totalMarked;
	*outWriteMarked = writeMarked;
	*outReadMarked = readMarked;

	return true;
}

void CTestMemoryAccessTiming::Run(ITestCallback *cb)
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

	// --- Test A: STA absolute ($8D) ---
	int totalA, writeA, readA;
	RunTestProgram(di, "STA_abs", testCodeA, sizeof(testCodeA), &totalA, &writeA, &readA);

	if (totalA == 0)
	{
		sprintf(failureMsg, "STA_abs: no marked cycles found (expected writes to $0400)");
		allPassed = false;
	}
	else if (readA > 0)
	{
		sprintf(failureMsg, "STA_abs: found %d read markings (expected 0, got %d writes + %d reads)", readA, writeA, readA);
		allPassed = false;
	}

	if (allPassed)
	{
		char msg[128];
		sprintf(msg, "STA_abs: %d writes, %d reads (OK)", writeA, readA);
		StepCompleted(1, true, msg);
	}
	else
	{
		StepCompleted(1, false, failureMsg);
	}

	// --- Test B: STA absolute,X ($9D) with X=0 ---
	if (allPassed)
	{
		int totalB, writeB, readB;
		RunTestProgram(di, "STA_absX", testCodeB, sizeof(testCodeB), &totalB, &writeB, &readB);

		if (totalB == 0)
		{
			sprintf(failureMsg, "STA_absX: no marked cycles found (expected writes to $0400)");
			allPassed = false;
		}
		else if (readB > 0)
		{
			// This tests the fix: after fixing the bogus read issue,
			// dummy reads should NOT be recorded in mem_access tracking
			sprintf(failureMsg, "STA_absX: found %d bogus reads (expected 0, got %d writes + %d reads)", readB, writeB, readB);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "STA_absX: %d writes, %d reads (OK)", writeB, readB);
			StepCompleted(2, true, msg);
		}
		else
		{
			StepCompleted(2, false, failureMsg);
		}
	}

	// Restore emulator state
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
	{
		TestCompleted(true, "Memory access timing correct (no bogus reads)");
	}
	else
	{
		TestCompleted(false, failureMsg);
	}
#endif
}

void CTestMemoryAccessTiming::Cancel()
{
	isRunning = false;
}
