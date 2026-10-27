#include "CTestViceDrive1541.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include "CDrive1541Target.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsSegmentDrive1541.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebuggerServerApiVice.h"
#include "CDebuggerServer.h"
#include "CDebuggerServerProtocol.h"
#include <cstdio>
#include <cstring>

extern "C"
{
#include "ViceWrapper.h"
#include "drivetypes.h"
uint8 c64d_get_drivecpu_status_internal(drive_context_t *drv);
void c64d_set_drivecpu_pc_no_trap(int driveNr, uint16 pc);
void c64d_set_drivecpu_regs_no_trap(int driveNr, uint8 a, uint8 x, uint8 y, uint8 p, uint8 sp);
};

static char failureMsg[512];

static bool CheckOwnedTargetDecisions()
{
	CDrive1541Target target;
	int pauses = 0;
	auto pause = [&]() -> bool { pauses++; return true; };
	if (target.AtBoundary(0x600, pause) || pauses) return false;
	uint64_t first = target.Arm(0x600);
	if (!first || target.Observe().hit || target.AtBoundary(0x601, pause)) return false;
	// A sampled target PC alone, or a refused mode change, is not hit evidence.
	if (target.AtBoundary(0x600, []() -> bool { return false; }) || target.Observe().hit) return false;
	target.Cancel(first);
	if (target.AtBoundary(0x600, pause) || target.Observe().active || pauses) return false;
	uint64_t second = target.Arm(0x602);
	if (second == first || target.Cancel(first).active == false) return false;
	if (!target.AtBoundary(0x602, pause) || pauses != 1) return false;
	auto observed = target.Cancel(second);
	return observed.requestId == second && observed.pc == 0x602 && observed.hit && !observed.active
		&& !target.AtBoundary(0x602, pause) && pauses == 1;
}

#ifdef RUN_COMMODORE64
// Capture the real state endpoint without opening a socket or replacing the live server.
class CDriveStateTestServer : public CDebuggerServer
{
public:
	EndpointHandlerV1 stateHandler;
	nlohmann::json result;
	int status = 0;
	virtual void AddEndpointFunction(const EndpointDescriptor &desc, EndpointHandlerV1 handler) override
	{
		if (desc.fn == "c64/drive1541/state") stateHandler = handler;
	}
	virtual std::vector<char> *PrepareResult(int code, const std::string token, nlohmann::json value,
		u8 *binaryData, int binaryDataSize) override
	{
		status = code; result = value; return nullptr;
	}
};

static bool CheckFullDriveFlags()
{
	// Independent expected bytes cover every cached N/Z combination on a local CPU.
	drivecpu_context_t cpu = {};
	drive_context_t drive = {};
	drive.cpu = &cpu;
	cpu.cpu_regs.p = 0x05;
	const uint8 negative[4] = {0x80, 0, 0x80, 0};
	const uint8 zeroCache[4] = {0, 1, 1, 0};
	const uint8 expected[4] = {0xA7, 0x25, 0xA5, 0x27};
	for (int i = 0; i < 4; i++)
	{
		cpu.cpu_regs.n = negative[i]; cpu.cpu_regs.z = zeroCache[i];
		if (c64d_get_drivecpu_status_internal(&drive) != expected[i]) return false;
	}
	return true;
}

static bool CheckReadOnlyDriveState(CDebugInterfaceVice *di)
{
	CDriveStateTestServer server;
	di->GetDebuggerServerApi()->RegisterEndpoints(&server);
	if (!server.stateHandler) return false;
	c64d_drive1541_state_t before = {}, after = {};
	CDebugSymbolsSegment *segment = di->symbolsDrive1541 ? di->symbolsDrive1541->currentSegment : nullptr;
	if (!segment || !segment->breakpointsPC) return false;
	CDebugBreakpointsAddr *bps = segment->breakpointsPC;
	const int mode = di->GetDebugMode();
	const int parked = c64d_is_cpu_parked_in_pause_loop();
	const int temporary = bps->temporaryBreakpointPC;
	const bool breakOnPC = segment->breakOnPC;
	CDebugBreakpointAddr *breakpoint = bps->GetBreakpoint(0x601);
	di->LockMutex();
	bool sampled = c64d_drive1541_sample_state(&before) != 0;
	di->UnlockMutex();
	if (!sampled || mode != DEBUGGER_MODE_PAUSED || !parked || !c64d_drive1541_try_begin_control()) return false;
	// Prove querying works without acquiring or releasing another operation's admission.
	struct AdmissionGuard { ~AdmissionGuard() { c64d_drive1541_end_control(); } } admission;
	bool passed = true;
	try
	{
		for (const auto &params : nlohmann::json::array({nullptr, nlohmann::json::object(), {{"device", 8}}}))
		{
			server.status = 0; server.result = nullptr;
			server.stateHandler("", params, nullptr, 0);
			const auto &value = server.result;
			passed = passed && server.status == HTTP_OK && value.at("available") == true
				&& value.at("outcome") == "sampled" && value.at("consistency") == "best_effort"
				&& value.at("pc") == before.pc && value.at("a") == before.a && value.at("x") == before.x
				&& value.at("y") == before.y && value.at("sp") == before.sp && value.at("flags") == before.flags
				&& value.at("cycle") == before.cycle && value.at("model") == before.model
				&& value.at("headTrackPosition") == before.headTrackPosition
				&& value.at("ramSize") == di->dataAdapterDrive1541DirectRam->AdapterGetDataLength()
				&& value.at("debugMode") == mode && value.at("parked") == true;
		}
		server.stateHandler("", {{"device", 9}}, nullptr, 0);
		passed = passed && server.status == HTTP_BAD_REQUEST;
	}
	catch (...) { passed = false; }
	if (c64d_drive1541_try_begin_control()) passed = false;
	di->LockMutex();
	sampled = c64d_drive1541_sample_state(&after) != 0;
	di->UnlockMutex();
	return passed && sampled && before.pc == after.pc && before.a == after.a && before.x == after.x
		&& before.y == after.y && before.sp == after.sp && before.flags == after.flags && before.cycle == after.cycle
		&& mode == di->GetDebugMode() && parked == c64d_is_cpu_parked_in_pause_loop()
		&& temporary == bps->temporaryBreakpointPC && breakOnPC == segment->breakOnPC
		&& breakpoint == bps->GetBreakpoint(0x601);
}

// Controlled RAM-only instruction loops; no disk image, drive-debug selection,
// snapshot, or lifecycle transition is part of the fixture.
static bool CheckNativeOwnedRun(CDebugInterfaceVice *di)
{
	bool passed = true;
	auto require = [&](bool condition, const char *message) -> bool {
		if (!condition && passed) { snprintf(failureMsg, sizeof(failureMsg), "%s", message); passed = false; }
		return condition;
	};
	c64d_drive1541_pause_result_t paused;
	if (!require(c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED,
		"Run-until fixture requires an observed pause")) return false;
	CDebugSymbolsSegment *segment = di->symbolsDrive1541 ? di->symbolsDrive1541->currentSegment : nullptr;
	if (!require(segment && segment->breakpointsPC, "Drive breakpoint context unavailable for fixture")) return false;
	CDebugBreakpointsAddr *bps = segment->breakpointsPC;
	if (!require(bps->GetBreakpoint(0x601) == nullptr, "Fixture needs unused breakpoint address $0601")) return false;

	C64StateCPU saved;
	di->GetDrive1541CpuState(&saved);
	const int savedCachedPC = di->GetDrive1541PC();
	const int savedTemporary = bps->temporaryBreakpointPC;
	const bool savedBreakOnPC = segment->breakOnPC;
	uint8 savedRam[8];
	const uint8 program[8] = {0xEA, 0xEA, 0x4C, 0x00, 0x06, 0x4C, 0x05, 0x06};
	CDebugBreakpointAddr *fixtureBreakpoint = nullptr;
	di->LockMutex();
	for (int i = 0; i < 8; i++) { savedRam[i] = di->GetByteFromRam1541(0x600 + i); di->SetByteToRam1541(0x600 + i, program[i]); }
	bps->temporaryBreakpointPC = 0x700; // Unreachable, independently owned legacy fixture.
	c64d_set_drivecpu_regs_no_trap(0, saved.a, saved.x, saved.y, saved.processorFlags | 4, saved.sp);
	c64d_set_drivecpu_pc_no_trap(0, 0x600);
	di->UnlockMutex();

	c64d_drive1541_run_result_t result;
	int status = c64d_drive1541_run_until(0x600, 1000, &result);
	require(status == C64D_DRIVE_ALREADY_AT_TARGET && !result.ownedTargetHit && !result.resumed && result.requestId == 0,
		"Initial PC sample was incorrectly reported as an owned hit");
	if (passed && c64d_drive1541_try_begin_control())
	{
		status = c64d_drive1541_run_until(0x601, 1000, &result);
		c64d_drive1541_end_control();
		require(status == C64D_DRIVE_CONTROL_BUSY && !result.resumed && !result.requestId, "Busy run-until changed execution");
	}
	else if (passed) require(false, "Run-until admission unexpectedly busy");
	if (passed)
	{
		status = c64d_drive1541_run_until(0x601, 1000, &result);
		require(status == C64D_DRIVE_TARGET_OBSERVED && result.ownedTargetHit && result.requestId && result.resumed,
			"Owned drive boundary hit was not reported");
	}
	require(c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED, "Drive did not park after owned hit");
	if (passed)
	{
		di->LockMutex();
		segment->AddBreakpointPC(0x601);
		segment->breakOnPC = true; // Fixture must exercise checks even if the prior UI disabled them.
		fixtureBreakpoint = bps->GetBreakpoint(0x601);
		c64d_set_drivecpu_pc_no_trap(0, 0x600);
		di->UnlockMutex();
		require(fixtureBreakpoint != nullptr, "User breakpoint fixture was not installed");
	}
	if (passed)
	{
		status = c64d_drive1541_run_until(0x601, 1000, &result);
		// With drive-debug selection, the legacy post-instruction check can stop first.
		require((status == C64D_DRIVE_TARGET_OBSERVED && result.ownedTargetHit)
			|| (status == C64D_DRIVE_STOPPED_UNKNOWN && !result.ownedTargetHit), "Same-address breakpoint lost truthful attribution");
		require(c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED, "Drive did not park at user breakpoint");
		require(bps->GetBreakpoint(0x601) == fixtureBreakpoint && bps->temporaryBreakpointPC == 0x700,
			"Run-until overwrote user or temporary breakpoint state");
	}
	if (passed)
	{
		c64d_set_drivecpu_pc_no_trap(0, 0x600);
		status = c64d_drive1541_run_until(0x602, 1000, &result);
		require(status == C64D_DRIVE_STOPPED_UNKNOWN && !result.ownedTargetHit, "Earlier user stop was misreported as owned target");
		require(c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED, "Drive did not park after unknown stop");
	}
	if (passed)
	{
		di->LockMutex();
		bps->DeleteBreakpoint(fixtureBreakpoint); fixtureBreakpoint = nullptr;
		c64d_set_drivecpu_pc_no_trap(0, 0x600);
		di->UnlockMutex();
		status = c64d_drive1541_run_until(0x605, 25, &result);
		require(status == C64D_DRIVE_RUN_TIMEOUT && !result.ownedTargetHit, "Unreachable target did not expire truthfully");
		require(c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED, "Timeout pause was not observed");
	}
	if (passed)
	{
		c64d_set_drivecpu_pc_no_trap(0, 0x605); // Execute the cancelled target afterward.
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(10);
		require(di->GetDebugMode() == DEBUGGER_MODE_RUNNING, "Cancelled target caused a later pause");
		require(c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED, "Running-to-parked pause failed");
	}
	// Never write fixture bytes back into a core that has not acknowledged pause.
	if (c64d_drive1541_pause(1000, &paused) == C64D_DRIVE_PAUSE_OBSERVED)
	{
		di->LockMutex();
		if (fixtureBreakpoint && bps->GetBreakpoint(0x601) == fixtureBreakpoint) bps->DeleteBreakpoint(fixtureBreakpoint);
		require(bps->temporaryBreakpointPC == 0x700, "Legacy temporary target was changed");
		bps->temporaryBreakpointPC = savedTemporary;
		segment->breakOnPC = savedBreakOnPC;
		for (int i = 0; i < 8; i++) di->SetByteToRam1541(0x600 + i, savedRam[i]);
		c64d_set_drivecpu_regs_no_trap(0, saved.a, saved.x, saved.y, saved.processorFlags, saved.sp);
		c64d_set_drivecpu_pc_no_trap(0, saved.pc);
		viceCurrentDiskPC[0] = (uint16)savedCachedPC;
		di->UnlockMutex();
	}
	else require(false, "Fixture restoration refused because pause was not observed");
	return passed;
}
#endif

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

	// --- Step 10: exact request ownership, no sampled-PC inference, and cancellation ---
	if (allPassed)
	{
		allPassed = CheckOwnedTargetDecisions();
		if (!allPassed) sprintf(failureMsg, "Owned target decisions accepted stale, cancelled, or unobserved evidence");
		StepCompleted(10, allPassed, allPassed ? "Owned target identity and cancellation controls passed" : failureMsg);
	}

	// --- Step 11: normal drive execution, existing breakpoints, timeout, and post-cancel run ---
	if (allPassed)
	{
		allPassed = CheckNativeOwnedRun(di);
		StepCompleted(11, allPassed, allPassed ? "Owned drive run-until preserves legacy breakpoints and disarms on return" : failureMsg);
	}

	// --- Step 12: complete status includes separately cached negative and zero flags ---
	if (allPassed)
	{
		allPassed = CheckFullDriveFlags();
		if (!allPassed) sprintf(failureMsg, "Drive status omitted or misread cached N/Z flags");
		StepCompleted(12, allPassed, allPassed ? "Complete drive status matches four N/Z controls" : failureMsg);
	}

	// --- Step 13: real state endpoint preserves paused CPU and existing control ownership ---
	if (allPassed)
	{
		allPassed = CheckReadOnlyDriveState(di);
		if (!allPassed) sprintf(failureMsg, "Drive state query changed execution/control or returned incorrect fields");
		StepCompleted(13, allPassed, allPassed ? "Read-only state fields and legacy pc/ramSize verified" : failureMsg);
	}

	// Restore emulator state
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "1541 drive verified: CPU state, memory, registers, adapter, dirty flags, observed pause, control admission, owned run-until, full flags, read-only state");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestViceDrive1541::Cancel()
{
	isRunning = false;
}
