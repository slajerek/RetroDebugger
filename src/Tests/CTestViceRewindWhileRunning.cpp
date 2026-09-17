#include "CTestViceRewindWhileRunning.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CMainMenuHelper.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "CSnapshotsManager.h"
#include "C64SettingsStorage.h"
#include "CSlrString.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

#ifdef RUN_COMMODORE64

static char failureMsg[1024];

// How many rewind-while-running attempts to make. The bug is intermittent and
// phase-dependent (where in the frame, relative to a pending IRQ/alarm, the
// restore lands), so we sweep the rewind depth and try many times.
static const int ITERATIONS = 400;

void CTestViceRewindWhileRunning::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
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

	bool savedForceUnpause = c64SettingsForceUnpause;
	bool savedRecord = c64SettingsSnapshotsRecordIsActive;
	c64SettingsForceUnpause = true;

	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(1000);

	// Load the frozen raster-IRQ demo (a stable copy of xparty.prg) and
	// autostart it, so the rewind path exercises VIC raster IRQ restore.
	CSlrString *path = new CSlrString(CTest::ResolveProjectPath("tests/data/rewind-jam-test.prg").c_str());
	bool loaded = viewC64->mainMenuHelper->LoadPRG(path, true, false, false, false);
	delete path;
	if (!loaded)
	{
		c64SettingsForceUnpause = savedForceUnpause;
		if (!wasRunning)
			viewC64->StopEmulationThread(di);
		TestCompleted(false, "LoadPRG(rewind-jam-test.prg) returned false");
		return;
	}

	// Let the demo start and reach steady state (raster IRQs running).
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(3000);

	// Enable per-frame timeline recording so rewind has snapshots to land on.
	c64SettingsSnapshotsRecordIsActive = true;
	di->snapshotsManager->SetRecordingIsActive(true);
	di->snapshotsManager->SetRecordingStoreInterval(1);
	di->snapshotsManager->SetRecordingLimit(2000);

	// Build up a few seconds of timeline history.
	SYS_Sleep(3000);

	// Confirm the demo is actually executing (sample PC) and record its range,
	// so we know the rewinds land in live demo code (not boot/idle). In headless
	// GetEmulationFrameNumber() is unreliable, so PC range is our sanity signal.
	u16 pcMin = 0xFFFF, pcMax = 0x0000;
	for (int s = 0; s < 16; s++)
	{
		C64StateCPU st;
		di->GetC64CpuState(&st);
		if (st.pc < pcMin) pcMin = st.pc;
		if (st.pc > pcMax) pcMax = st.pc;
		SYS_Sleep(15);
	}

	int minFrame = 0, maxFrame = 0;
	di->snapshotsManager->GetFramesLimits(&minFrame, &maxFrame);
	int firstMin = minFrame, firstMax = maxFrame;

	bool failed = false;
	const char *failKind = "";
	int doneIters = 0;
	for (int i = 0; i < ITERATIONS && this->isRunning; i++)
	{
		doneIters = i;

		// Stay RUNNING (not paused) — the "scrub while code runs, do not stop"
		// scenario.
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);

		// CRITICAL: mimic the GUI cmd+left — route the Left key into the
		// emulator. This is what makes the keyboard path differ from mouse
		// scrubbing: KeyboardDown/Up set c64d_vice_input_tasks_flag, which lets
		// the snapshot restore run from the per-cycle hook (mid-instruction)
		// instead of only at the instruction boundary. Without this the bug does
		// not reproduce headless (mouse scrubbing is fine).
		di->KeyboardDown(MTKEY_ARROW_LEFT);

		// Rewind to an ABSOLUTE recorded frame. The headless live frame counter
		// is unreliable (returns ~boot), so instead of NumFramesOffset we sweep
		// the actually-recorded window [minFrame..maxFrame] directly.
		di->snapshotsManager->GetFramesLimits(&minFrame, &maxFrame);
		int span = maxFrame - minFrame;
		if (span > 4)
		{
			int target = minFrame + 1 + ((i * 13) % (span - 1));
			di->snapshotsManager->RestoreSnapshotByFrame(target, -1, DEBUGGER_MODE_RUNNING);
		}

		// Vary settle time to hit different points relative to the restore /
		// raster; every 4th iteration fires the next rewind almost immediately
		// to stress restore-while-a-previous-restore-just-finished timing.
		SYS_Sleep((i % 4 == 0) ? 3 : 25);
		di->KeyboardUp(MTKEY_ARROW_LEFT);

		if (di->IsCpuJam())            { failed = true; failKind = "CPU JAM"; break; }
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED) { failed = true; failKind = "unexpected PAUSED"; break; }

		// 6502 stuck ("deadlock"): while RUNNING the cycle counter must advance.
		u64 cyA = di->GetMainCpuCycleCounter();
		SYS_Sleep(40);
		u64 cyB = di->GetMainCpuCycleCounter();

		if (di->IsCpuJam())            { failed = true; failKind = "CPU JAM"; break; }
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED) { failed = true; failKind = "unexpected PAUSED"; break; }
		if (cyB == cyA)                { failed = true; failKind = "cycle stall (6502 stuck)"; break; }
	}

	// Capture state at the failure (or final) point.
	C64StateCPU cpu;
	di->GetC64CpuState(&cpu);
	u8 opAtPC = di->GetByteC64(cpu.pc);
	int jam = di->IsCpuJam() ? 1 : 0;
	uint8 dm = di->GetDebugMode();
	u64 cyc = di->GetMainCpuCycleCounter();
	unsigned int frame = di->GetEmulationFrameNumber();

	// Restore settings and pause state; unjam so we leave a clean machine.
	if (jam)
		di->ForceRunAndUnJamCpu();
	c64SettingsForceUnpause = savedForceUnpause;
	c64SettingsSnapshotsRecordIsActive = savedRecord;
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (failed)
	{
		sprintf(failureMsg,
			"Rewind-while-running %s at iter %d/%d: PC=$%04X op=$%02X sp=$%02X p=$%02X jam=%d mode=$%02X cyc=%llu frame=%u (recorded %d..%d, demo PC range $%04X-$%04X). See 'CPU JAM'/'CPU BRK' log line (--log-dir) for interrupt/alarm/CLK state.",
			failKind, doneIters, ITERATIONS, cpu.pc, opAtPC, cpu.sp, cpu.processorFlags, jam, dm,
			(unsigned long long)cyc, frame, minFrame, maxFrame, pcMin, pcMax);
		TestCompleted(false, failureMsg);
		return;
	}

	char okMsg[320];
	sprintf(okMsg,
		"Rewind-while-running OK across %d iterations: PC=$%04X cyc=%llu frame=%u (recorded %d..%d, initial %d..%d, demo PC range $%04X-$%04X)",
		ITERATIONS, cpu.pc, (unsigned long long)cyc, frame, minFrame, maxFrame, firstMin, firstMax, pcMin, pcMax);
	TestCompleted(true, okMsg);
}

void CTestViceRewindWhileRunning::Cancel()
{
	this->isRunning = false;
}

#else

void CTestViceRewindWhileRunning::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	TestCompleted(true, "Skipped (C64 not compiled in)");
}

void CTestViceRewindWhileRunning::Cancel()
{
	this->isRunning = false;
}

#endif
