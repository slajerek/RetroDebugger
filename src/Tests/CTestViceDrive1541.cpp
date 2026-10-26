#include "CTestViceDrive1541.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

extern "C"
{
#include "ViceWrapper.h"
};

static char failureMsg[512];

void CTestViceDrive1541::Run(ITestCallback *cb)
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

	// --- Step 1: Drive CPU state access ---
	{
		di->PauseEmulationBlockedWait();

		int drivePC = di->GetDrive1541PC();

		// Drive PC should be in valid range ($0000-$FFFF)
		if (drivePC < 0 || drivePC > 0xFFFF)
		{
			sprintf(failureMsg, "Drive PC out of range: %d", drivePC);
			allPassed = false;
		}

		C64StateCPU driveState;
		di->GetDrive1541CpuState(&driveState);

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Drive CPU: PC=$%04X A=$%02X X=$%02X Y=$%02X SP=$%02X",
					driveState.pc, driveState.a, driveState.x, driveState.y, driveState.sp);
			StepCompleted(1, true, msg);
		}
		else
		{
			StepCompleted(1, false, failureMsg);
		}
	}

	// --- Step 2: Drive memory read/write ---
	if (allPassed)
	{
		di->PauseEmulationBlockedWait();

		// Write to drive RAM at a safe address ($0400 - work area)
		di->SetByteToRam1541(0x0400, 0x55);
		u8 readBack = di->GetByteFromRam1541(0x0400);

		if (readBack != 0x55)
		{
			sprintf(failureMsg, "Drive RAM: wrote $55 to $0400, read $%02X", readBack);
			allPassed = false;
		}

		// Test another address
		if (allPassed)
		{
			di->SetByteToRam1541(0x0401, 0xAA);
			readBack = di->GetByteFromRam1541(0x0401);
			if (readBack != 0xAA)
			{
				sprintf(failureMsg, "Drive RAM: wrote $AA to $0401, read $%02X", readBack);
				allPassed = false;
			}
		}

		if (allPassed)
			StepCompleted(2, true, "Drive memory read/write round-trip verified");
		else
			StepCompleted(2, false, failureMsg);
	}

	// --- Step 3: Drive register read consistency ---
	if (allPassed)
	{
		di->PauseEmulationBlockedWait();

		// Read drive registers and verify they're consistent between state and PC query
		C64StateCPU driveState;
		di->GetDrive1541CpuState(&driveState);
		int drivePC = di->GetDrive1541PC();

		if ((u16)drivePC != driveState.pc)
		{
			sprintf(failureMsg, "Drive PC mismatch: GetDrive1541PC()=$%04X vs state.pc=$%04X", drivePC, driveState.pc);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Drive registers: PC=$%04X A=$%02X X=$%02X Y=$%02X SP=$%02X",
					driveState.pc, driveState.a, driveState.x, driveState.y, driveState.sp);
			StepCompleted(3, true, msg);
		}
		else
		{
			StepCompleted(3, false, failureMsg);
		}
	}

	// --- Step 4: Drive state struct ---
	if (allPassed)
	{
		C64StateDrive1541 driveState;
		di->GetDrive1541State(&driveState);

		// Head track position should be in valid range (typically 0-83 for 35 tracks * 2 halftracks + extra)
		if (driveState.headTrackPosition < 0 || driveState.headTrackPosition > 200)
		{
			sprintf(failureMsg, "Drive state: headTrackPosition=%d out of expected range", driveState.headTrackPosition);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Drive state: headTrackPosition=%d", driveState.headTrackPosition);
			StepCompleted(4, true, msg);
		}
		else
		{
			StepCompleted(4, false, failureMsg);
		}
	}

	// --- Step 5: Drive data adapter access ---
	if (allPassed)
	{
		di->SetByteToRam1541(0x0402, 0xCC);

		u8 val = 0;
		di->dataAdapterDrive1541DirectRam->AdapterReadByte(0x0402, &val);

		if (val != 0xCC)
		{
			sprintf(failureMsg, "Drive adapter: read $%02X at $0402, expected $CC", val);
			allPassed = false;
		}

		if (allPassed)
			StepCompleted(5, true, "Drive data adapter access verified");
		else
			StepCompleted(5, false, failureMsg);
	}

	// --- Step 6: Disk dirty flags ---
	if (allPassed)
	{
		// Clear dirty flags and verify
		di->ClearDriveDirtyForSnapshotFlag();
		bool dirtySnap = di->IsDriveDirtyForSnapshot();

		di->ClearDriveDirtyForRefreshFlag(0);
		bool dirtyRefresh = di->IsDriveDirtyForRefresh(0);

		// After clearing, flags should be false
		if (dirtySnap)
		{
			sprintf(failureMsg, "Drive dirty for snapshot flag not cleared");
			allPassed = false;
		}
		else if (dirtyRefresh)
		{
			sprintf(failureMsg, "Drive dirty for refresh flag not cleared");
			allPassed = false;
		}

		if (allPassed)
			StepCompleted(6, true, "Disk dirty flag management verified");
		else
			StepCompleted(6, false, failureMsg);
	}

	// --- Step 7: observed pause never steps or resumes an already parked CPU ---
	if (allPassed)
	{
		c64d_drive1541_pause_result_t observed;
		int status = c64d_drive1541_pause(1000, &observed);
		if (status != C64D_DRIVE_PAUSE_OBSERVED || !observed.parked || observed.debugMode != DEBUGGER_MODE_PAUSED)
		{
			sprintf(failureMsg, "Drive pause not observed: status=%d mode=%d parked=%d", status, observed.debugMode, observed.parked);
			allPassed = false;
		}
		if (allPassed)
		{
			int pc = di->GetDrive1541PC();
			u64 cycle = di->GetMainCpuCycleCounter();
			status = c64d_drive1541_pause(0, &observed);
			if (status != C64D_DRIVE_PAUSE_OBSERVED || observed.pauseRequested || !observed.parked
				|| di->GetDrive1541PC() != pc || di->GetMainCpuCycleCounter() != cycle)
			{
				sprintf(failureMsg, "Already parked drive pause stepped, resumed, or requested execution change");
				allPassed = false;
			}
		}
		StepCompleted(7, allPassed, allPassed ? "Parked pause observation preserves execution" : failureMsg);
	}

	// --- Step 8: busy and invalid requests cannot issue another pause ---
	if (allPassed)
	{
		c64d_drive1541_pause_result_t observed;
		if (c64d_drive1541_pause(-1, &observed) != C64D_DRIVE_INVALID_ARGUMENT || observed.pauseRequested
			|| c64d_drive1541_pause(5001, &observed) != C64D_DRIVE_INVALID_ARGUMENT || observed.pauseRequested)
		{
			sprintf(failureMsg, "Invalid timeout changed execution"); allPassed = false;
		}
		if (allPassed && c64d_drive1541_try_begin_control())
		{
			int status = c64d_drive1541_pause(1000, &observed);
			c64d_drive1541_end_control();
			if (status != C64D_DRIVE_CONTROL_BUSY || observed.pauseRequested)
			{
				sprintf(failureMsg, "Nested control admission was not refused"); allPassed = false;
			}
		}
		else if (allPassed)
		{
			sprintf(failureMsg, "Control admission remained busy after pause"); allPassed = false;
		}
		StepCompleted(8, allPassed, allPassed ? "Invalid/busy control requests refused without mutation" : failureMsg);
	}

	// --- Step 9: mode alone, stale parked alone, and elapsed time cannot fake an acknowledgment ---
	if (allPassed)
	{
		for (int expired = 0; expired <= 1; expired++)
		{
			int expected = expired ? C64D_DRIVE_PAUSE_TIMEOUT : C64D_DRIVE_PAUSE_PENDING;
			if (c64d_drive1541_pause_outcome(DEBUGGER_MODE_PAUSED, 0, expired) != expected
				|| c64d_drive1541_pause_outcome(DEBUGGER_MODE_RUNNING, 1, expired) != expected
				|| c64d_drive1541_pause_outcome(DEBUGGER_MODE_RUNNING, 0, expired) != expected
				|| c64d_drive1541_pause_outcome(DEBUGGER_MODE_PAUSED, 1, expired) != C64D_DRIVE_PAUSE_OBSERVED)
			{
				sprintf(failureMsg, "Pause observation decision falsely acknowledged or ignored a deadline");
				allPassed = false; break;
			}
		}
		StepCompleted(9, allPassed, allPassed ? "Timeout requires no acknowledgment; mode alone is insufficient" : failureMsg);
	}

	// Restore emulator state
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "1541 drive verified: CPU state, memory, registers, adapter, dirty flags, observed pause, control admission");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestViceDrive1541::Cancel()
{
	isRunning = false;
}
