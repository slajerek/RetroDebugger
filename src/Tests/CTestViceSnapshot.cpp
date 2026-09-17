#include "CTestViceSnapshot.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "CByteBuffer.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

static char failureMsg[512];

void CTestViceSnapshot::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

#ifndef RUN_COMMODORE64
	TestCompleted(true, "Skipped (C64 not enabled)");
	return;
#else
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

	bool allPassed = true;

	// Same value CTestSnapshotBoundary uses for the same API.
	const u32 BOUNDARY_TIMEOUT_MS = 5000;

	// WHICH SNAPSHOT API THIS TEST USES, AND WHY
	//
	// Save/LoadChipsSnapshotSynced() run the snapshot ON THE CALLING THREAD.
	// They are the emulation thread's entry point -- CSnapshotsManager calls
	// them from ConsumeExternalSnapshotRequestUnlocks(), at a CPU instruction
	// boundary. Calling them from the test (main) thread WHILE THE EMULATOR IS
	// RUNNING is a data race: raster_snapshot_read() does lib_realloc() on
	// vicii.raster.canvas->draw_buffer while the emulation thread is copying
	// out of that same buffer in c64d_refresh_screen_no_callback(). The freed
	// pages get reused by unrelated allocations, and the suite then dies tens
	// of tests later in whatever touched them next.
	//
	// Caught by AddressSanitizer 2026-09-09:
	//   heap-use-after-free ... READ of size 1 thread T18
	//     c64d_refresh_screen_no_callback ViceWrapper.cpp:624
	//   freed by thread T0:
	//     lib_realloc -> raster_snapshot_read raster-snapshot.c:98
	//     -> LoadChipsSnapshotSynced -> CTestViceSnapshot::Run
	// It is the root of src/TODO.txt's "FIRST RUN" crash.
	//
	// So the running-emulator steps below use the *AtCpuBoundary() variants --
	// the documented entry points for off-emulation-thread callers (remote,
	// MCP, tests). They queue the operation and the emulation thread performs
	// it at the next instruction boundary, so nothing reallocs under it.
	//
	// Step 5 uses them too, for the same reason. Being PAUSED is not an
	// exemption -- ASan showed the pause loop itself refreshes screen lines
	// (c64d_debug_pause_check ViceWrapper.cpp:1636 ->
	// c64d_refresh_lines_fast_locked), so a snapshot realloc from another
	// thread races there exactly as it does while running. The deadlock guard
	// that step pins is inside *Synced() and is still exercised: the boundary
	// API's work IS a *Synced() call, made by the emulation thread while the
	// debugger is paused, which is the situation the guard exists for.

	// Ensure emulator is running (not paused from a previous test)
	if (di->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(100);
	}

	// --- Step 1: Save snapshot to buffer (synchronous API) ---
	CByteBuffer *snapshotBuffer = new CByteBuffer();

	{
		// Write known pattern to RAM before saving
		di->SetByteToRamC64(0x0800, 0xAA);
		di->SetByteToRamC64(0x0801, 0xBB);
		di->SetByteToRamC64(0x0802, 0xCC);

		// Use SaveChipsSnapshotSynced — the synchronous buffer-based save
		bool saved = di->SaveChipsSnapshotAtCpuBoundary(snapshotBuffer, BOUNDARY_TIMEOUT_MS);

		if (!saved || snapshotBuffer->length == 0)
		{
			sprintf(failureMsg, "SaveChipsSnapshotAtCpuBoundary %s (buffer length=%d)",
					saved ? "produced empty buffer" : "returned false", (int)snapshotBuffer->length);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Snapshot saved: %d bytes", (int)snapshotBuffer->length);
			StepCompleted(1, true, msg);
		}
		else
		{
			StepCompleted(1, false, failureMsg);
		}
	}

	// --- Step 2: Modify state after save ---
	u8 origRAM[3];

	if (allPassed)
	{
		// Record what we saved
		origRAM[0] = 0xAA;
		origRAM[1] = 0xBB;
		origRAM[2] = 0xCC;

		// Modify RAM
		di->SetByteToRamC64(0x0800, 0x11);
		di->SetByteToRamC64(0x0801, 0x22);
		di->SetByteToRamC64(0x0802, 0x33);

		// Brief pause to let the write settle, then verify
		SYS_Sleep(50);

		// Pause to read RAM safely
		di->PauseEmulationBlockedWait();
		u8 modified = di->GetByteFromRamC64(0x0800);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(50);

		if (modified != 0x11)
		{
			sprintf(failureMsg, "State modification failed: $0800=$%02X (expected $11)", modified);
			allPassed = false;
		}

		if (allPassed)
			StepCompleted(2, true, "State modified: RAM changed ($AA->$11, $BB->$22, $CC->$33)");
		else
			StepCompleted(2, false, failureMsg);
	}

	// --- Step 3: Restore snapshot and verify RAM ---
	if (allPassed)
	{
		snapshotBuffer->Rewind();
		bool loaded = di->LoadChipsSnapshotAtCpuBoundary(snapshotBuffer, BOUNDARY_TIMEOUT_MS);

		if (!loaded)
		{
			sprintf(failureMsg, "LoadChipsSnapshotAtCpuBoundary returned false");
			allPassed = false;
		}

		if (allPassed)
		{
			// Pause to read RAM safely after restore
			SYS_Sleep(50);
			di->PauseEmulationBlockedWait();

			u8 ram0 = di->GetByteFromRamC64(0x0800);
			u8 ram1 = di->GetByteFromRamC64(0x0801);
			u8 ram2 = di->GetByteFromRamC64(0x0802);

			di->SetDebugMode(DEBUGGER_MODE_RUNNING);
			SYS_Sleep(50);

			if (ram0 != origRAM[0] || ram1 != origRAM[1] || ram2 != origRAM[2])
			{
				sprintf(failureMsg, "RAM not restored: $0800=$%02X,$%02X,$%02X (expected $%02X,$%02X,$%02X)",
						ram0, ram1, ram2, origRAM[0], origRAM[1], origRAM[2]);
				allPassed = false;
			}
		}

		if (allPassed)
		{
			StepCompleted(3, true, "Snapshot restored: RAM values match pre-save state");
		}
		else
		{
			StepCompleted(3, false, failureMsg);
		}
	}

	// --- Step 4: Second save/restore cycle to verify re-save works ---
	if (allPassed)
	{
		// Write new pattern
		di->SetByteToRamC64(0x0900, 0x42);
		di->SetByteToRamC64(0x0901, 0x43);

		CByteBuffer *snapshot2 = new CByteBuffer();
		bool saved2 = di->SaveChipsSnapshotAtCpuBoundary(snapshot2, BOUNDARY_TIMEOUT_MS);

		if (!saved2 || snapshot2->length == 0)
		{
			sprintf(failureMsg, "Second SaveChipsSnapshotAtCpuBoundary failed");
			allPassed = false;
		}

		if (allPassed)
		{
			// Modify
			di->SetByteToRamC64(0x0900, 0x00);
			di->SetByteToRamC64(0x0901, 0x00);

			// Restore
			snapshot2->Rewind();
			bool loaded2 = di->LoadChipsSnapshotAtCpuBoundary(snapshot2, BOUNDARY_TIMEOUT_MS);

			if (!loaded2)
			{
				sprintf(failureMsg, "Second LoadChipsSnapshotAtCpuBoundary failed");
				allPassed = false;
			}
			else
			{
				SYS_Sleep(50);
				di->PauseEmulationBlockedWait();

				u8 val0 = di->GetByteFromRamC64(0x0900);
				u8 val1 = di->GetByteFromRamC64(0x0901);

				di->SetDebugMode(DEBUGGER_MODE_RUNNING);
				SYS_Sleep(50);

				if (val0 != 0x42 || val1 != 0x43)
				{
					sprintf(failureMsg, "Second restore: $0900=$%02X,$%02X (expected $42,$43)", val0, val1);
					allPassed = false;
				}
			}
		}

		if (allPassed)
			StepCompleted(4, true, "Second save/restore cycle verified");
		else
			StepCompleted(4, false, failureMsg);

		delete snapshot2;
	}

	delete snapshotBuffer;

	// --- Step 5: save while the debugger is PAUSED (deadlock regression) ---
	// Writing a snapshot runs the drive CPUs forward, and they park in
	// c64d_debug_pause_check(0) while the debugger is paused -- waiting for an
	// unpause that only the thread doing the snapshot could deliver. That hung
	// the suite here for 10+ minutes with no output. Step 3 hit it only by
	// chance, depending on whether the machine happened to be paused; this
	// forces it.
	//
	// The guard is CSnapshotsManager::IsSnapshotOperationInProgress(), raised
	// by CSnapshotOperationScope inside Save/LoadChipsSnapshotSynced() -- so it
	// is on the operation, not on one entry point, and the boundary API below
	// still runs straight through it.
	//
	// If the guard regresses, this step does not fail -- it HANGS. That is the
	// bug being pinned, and a hung run is what the runner's timeout is for.
	if (allPassed)
	{
		int savedDebugMode = di->GetDebugMode();
		di->SetDebugMode(DEBUGGER_MODE_PAUSED);

		CByteBuffer *pausedSnapshot = new CByteBuffer();
		bool savedWhilePaused = di->SaveChipsSnapshotAtCpuBoundary(pausedSnapshot, BOUNDARY_TIMEOUT_MS);
		bool loadedWhilePaused = false;
		if (savedWhilePaused && pausedSnapshot->length > 0)
		{
			pausedSnapshot->Rewind();
			loadedWhilePaused = di->LoadChipsSnapshotAtCpuBoundary(pausedSnapshot, BOUNDARY_TIMEOUT_MS);
		}

		di->SetDebugMode(savedDebugMode);

		if (savedWhilePaused && loadedWhilePaused)
		{
			StepCompleted(5, true, "Save/load while PAUSED completed without deadlocking");
		}
		else
		{
			sprintf(failureMsg, "Paused save/load failed: saved=%d len=%d loaded=%d",
					(int)savedWhilePaused, (int)pausedSnapshot->length, (int)loadedWhilePaused);
			StepCompleted(5, false, failureMsg);
			allPassed = false;
		}
		delete pausedSnapshot;
	}

	// Restore emulator state
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "Snapshots verified: buffer save/load, state restore, file save/load");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestViceSnapshot::Cancel()
{
	isRunning = false;
}
