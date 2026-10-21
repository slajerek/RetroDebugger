#pragma once

#include "CTest.h"
#include "EmulatorsConfig.h"
#include "CTestTimelineCancellationChecks.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <vector>
#ifdef RUN_COMMODORE64
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CSnapshotsManager.h"
#include "C64SettingsStorage.h"
#include "DebuggerDefs.h"
#include "ViceWrapper.h"
#include "SYS_Main.h"
#endif

// Native Pending-timeout/late-service and idle history-clear coverage only.
// Run separately with observed PAL (model 0) and NTSC (model 3) profiles.
// No post-admission cancellation, snapshot restore, or timebase claim follows.
class CTestTimelineCancellation : public CTest
{
	std::atomic<bool> cancelled{false};
#ifdef RUN_COMMODORE64
	using Observation = TimelineCancellationChecks::Observation;
	CDebugInterfaceC64 *di = nullptr;
	CSnapshotsManager *manager = nullptr;
	CDebugSymbolsSegment *segment = nullptr;
	std::vector<int> ownedBreakpoints;

	void Require(bool condition, const char *message)
	{
		if (cancelled.load()) throw std::runtime_error("TimelineCancellation cancelled");
		if (!condition) throw std::runtime_error(message);
	}

	template<class Predicate> void Wait(Predicate predicate, const char *message)
	{
		const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (!predicate())
		{
			Require(std::chrono::steady_clock::now() < end, message);
			SYS_Sleep(5);
		}
		Require(true, message);
	}

	bool Parked() const
	{
		return di->GetDebugMode() == DEBUGGER_MODE_PAUSED && c64d_is_cpu_parked_in_pause_loop();
	}

	Observation Observe(const char *label)
	{
		Require(Parked() && !c64d_is_pc_change_pending(), "Observation needs settled pause without queued PC");
		Observation o;
		o.counter = di->GetMainCpuCycleCounter();
		o.frame = di->GetEmulationFrameNumber();
		C64StateCPU cpu{};
		C64StateVIC vic{};
		di->GetC64CpuState(&cpu); // Live CPU registers, never the selected GUI-frame cache.
		di->GetVICState(&vic);
		o.cpu = {{cpu.pc, cpu.a, cpu.x, cpu.y, cpu.processorFlags, cpu.sp,
			cpu.instructionCycle, di->GetByteC64(1)}};
		o.rasterLine = vic.raster_line;
		o.rasterCycle = vic.raster_cycle;
		o.model = di->GetC64ModelType();
		o.pendingPC = c64d_get_pending_pc();
		o.border = di->GetByteC64(0xd020) & 15;
		o.background = di->GetByteC64(0xd021) & 15;
		o.display = di->GetByteC64(0xd011) & 16;
		o.sprites = di->GetByteC64(0xd015);
		for (unsigned i = 0; i < o.ram.size(); ++i) o.ram[i] = di->GetByteFromRamC64(0x4000 + i);
		CDebugMemoryCell *cell = di->symbols->memory->GetMemoryCell(0x4000);
		Require(cell != nullptr, "Marker access cell unavailable");
		const DebugMemoryCellLastAccess access = cell->GetLastAccess();
		o.access = {{static_cast<u64>(access.read.pc), access.read.cycle, access.read.frame,
			static_cast<u64>(access.read.rasterLine), static_cast<u64>(access.read.rasterCycle),
			static_cast<u64>(access.write.pc), access.write.cycle, access.write.frame,
			static_cast<u64>(access.write.rasterLine), static_cast<u64>(access.write.rasterCycle)}};
		char regs[192];
		std::snprintf(regs, sizeof(regs), "{\"pc\":%u,\"a\":%u,\"x\":%u,\"y\":%u,\"p\":%u,\"sp\":%u}",
			unsigned(cpu.pc), unsigned(cpu.a), unsigned(cpu.x), unsigned(cpu.y), unsigned(cpu.processorFlags), unsigned(cpu.sp));
		o.registersJson = regs;
		o.paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED;
		o.parked = c64d_is_cpu_parked_in_pause_loop();
		Require(o.paused && o.parked && o.counter == di->GetMainCpuCycleCounter()
			&& o.frame == di->GetEmulationFrameNumber() && !c64d_is_pc_change_pending(), "Observation advanced or acquired a queued PC");
		LOGM("TimelineCancellation observation=%s model=%d counter=%llu frame=%llu cpu=%s phase=%d port=%d raster=%d/%d colors=%d/%d dma=%d/%d pendingPC=%d",
			label, o.model, (unsigned long long)o.counter, (unsigned long long)o.frame, regs,
			o.cpu[6], o.cpu[7], o.rasterLine, o.rasterCycle, o.border, o.background, o.display, o.sprites, o.pendingPC);
		for (unsigned i = 0; i < o.ram.size(); ++i) LOGM("TimelineCancellation observation=%s ram[%u]=%02x", label, i, unsigned(o.ram[i]));
		for (unsigned i = 0; i < o.access.size(); ++i) LOGM("TimelineCancellation observation=%s access[%u]=%llu", label, i, (unsigned long long)o.access[i]);
		return o;
	}

	void BreakAt(int entry, int point)
	{
		Require(segment->breakpointsPC->breakpoints.find(point) == segment->breakpointsPC->breakpoints.end(), "Fixture breakpoint already owned by another test");
		segment->AddBreakpointPC(point);
		ownedBreakpoints.push_back(point);
		// Ordinary fixture execution explicitly follows even if the current PC
		// request is queued at an in-flight instruction.  No implicit execution.
		di->MakeJmpC64(entry);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		Wait([&] {
			if (!Parked() || c64d_is_pc_change_pending()) return false;
			C64StateCPU cpu{};
			di->GetC64CpuState(&cpu);
			return cpu.pc == point && cpu.instructionCycle == 0;
		}, "Authored stage did not reach its pre-dispatch breakpoint");
	}

	void RemoveBreakpoints()
	{
		for (int point : ownedBreakpoints)
		{
			auto found = segment->breakpointsPC->breakpoints.find(point);
			if (found != segment->breakpointsPC->breakpoints.end()) segment->breakpointsPC->RemoveBreakpoint(found->second);
		}
		ownedBreakpoints.clear();
	}

	void LogResult(const char *label, const TimelineSeek::Result &r)
	{
		LOGM("TimelineCancellation result=%s seq=%llu outcome=%s completed=%d mutated=%d resolved=%d context=%d actual=%d source=%d origin=%llu/%llu/%d/%d/%d actualState=%llu/%llu/%d/%d/%d originRegs=%s actualRegs=%s",
			label, (unsigned long long)r.sequence, r.outcome.c_str(), int(r.completed), int(r.mutationStarted),
			int(r.destinationResolved), int(r.contextAvailable), int(r.actualAvailable), int(r.sourceAccessAvailable),
			(unsigned long long)r.origin.counter, (unsigned long long)r.origin.frame, r.origin.pc, int(r.origin.paused), int(r.origin.instructionBoundary),
			(unsigned long long)r.actual.counter, (unsigned long long)r.actual.frame, r.actual.pc, int(r.actual.paused), int(r.actual.instructionBoundary),
			r.origin.registersJson.c_str(), r.actual.registersJson.c_str());
	}

	void CheckIdle(const TimelineSeek::Result &r)
	{
		const auto status = manager->GetTimelineStatus();
		const bool service = manager->TimelineNeedsCpuService(), active = manager->IsStrictTimelineSeekActive();
		LOGM("TimelineCancellation idle seq=%llu lastSeq=%llu lastOutcome=%s busy=%d service=%d active=%d checkpoints=%llu instructions=%llu coverage=%d boundary=%d bounds=%d",
			(unsigned long long)r.sequence, (unsigned long long)status.lastRequest.sequence, status.lastRequest.outcome.c_str(),
			int(status.busy), int(service), int(active), (unsigned long long)status.verifiedCheckpoints,
			(unsigned long long)status.instructionCount, int(status.instructionCoverage), int(status.atInstructionBoundary), int(status.bounds.available));
		Require(TimelineCancellationChecks::IdleResult(status, r, service, active), "Transaction did not retain the completed idle sequence");
	}
#endif
public:
	const char *GetName() override { return "TimelineCancellation"; }
	void Cancel() override { cancelled.store(true); }
	void Run(ITestCallback *cb) override
	{
		callback = cb;
		isRunning = true;
		currentStep = 0;
		cancelled.store(false);
#ifndef RUN_COMMODORE64
		TestSkipped("C64 not enabled; native timeline cancellation not exercised");
#else
		di = viewC64->debugInterfaceC64;
		if (!di || !di->snapshotsManager || !di->symbolsC64 || !di->symbolsC64->currentSegment || !di->symbols->memory)
		{
			TestCompleted(false, "C64 manager/symbols/access state unavailable");
			return;
		}
		manager = di->snapshotsManager;
		segment = di->symbolsC64->currentSegment;
		const bool wasRunning = di->isRunning;
		const bool recording = c64SettingsSnapshotsRecordIsActive;
		const int interval = c64SettingsSnapshotsIntervalNumFrames, limit = c64SettingsSnapshotsLimit;
		bool warp = false, changedSettings = false;
		std::string error;
		try
		{
			if (!wasRunning) viewC64->StartEmulationThread(di);
			Wait([&] { return di->isRunning; }, "C64 emulation thread did not start");
			di->SetDebugMode(DEBUGGER_MODE_PAUSED);
			Wait([&] { return Parked(); }, "Initial native pause did not settle");
			const int model = di->GetC64ModelType();
			Require(di->GetEmulatorType() == EMULATOR_TYPE_C64_VICE, "Native VICE backend required");
			Require(model == 0 || model == 3, "Select standard PAL (0) or NTSC (3); run each separately");
			Require(!manager->GetTimelineStatus().busy && !manager->IsStrictTimelineSeekActive(), "Preexisting timeline transaction");
			Require((di->GetByteC64(1) & 7) == 7, "Fixture needs ordinary I/O-visible banking");
			warp = di->GetSettingIsWarpSpeed();
			changedSettings = true;
			di->SetSettingIsWarpSpeed(false);
			manager->SetRecordingStoreInterval(1);
			manager->SetRecordingLimit(64);
			manager->SetRecordingIsActive(true);
			manager->ClearSnapshotsHistory();
			// Byte-identical authored fixture used by the independently reviewed
			// A/B manual stability run.  No ROM calls or PAL rate arithmetic.
			static const u8 code[] = {
				0x78,0xd8,0x18,0xb8,0xa9,0x7f,0x8d,0x0d,0xdc,0x8d,0x0d,0xdd,0xad,0x0d,0xdc,0xad,0x0d,0xdd,
				0xa9,0x00,0x8d,0x15,0xd0,0x8d,0x11,0xd0,0x8d,0x1a,0xd0,0xa9,0x06,0x8d,0x20,0xd0,0x8d,0x21,0xd0,
				0xa9,0x11,0x8d,0x00,0x40,0xa2,0xf0,0x9a,0xa9,0x7e,0xa0,0x00,0xa2,0x00,0xca,0xd0,0xfd,0x88,0xd0,0xf8,
				0x4c,0x39,0x10,0xa9,0x02,0x8d,0x20,0xd0,0x8d,0x21,0xd0,0xa9,0x22,0x8d,0x00,0x40,
				0xa2,0xf0,0x9a,0xa9,0x7e,0xa0,0x00,0xa2,0x00,0xca,0xd0,0xfd,0x88,0xd0,0xf8,0x4c,0x58,0x10};
			static const u8 sentinel[] = {0x5a,0xa5,0x3c,0x69,0x96,0xc3,0x0f,0xf0,0x12,0x34,0x56,0x78,0x87,0x65,0x43,0x21};
			for (unsigned i = 0; i < sizeof(code); ++i) di->SetByteToRamC64(0x1000+i, code[i]);
			for (unsigned i = 0; i < sizeof(sentinel); ++i) di->SetByteToRamC64(0x4000+i, sentinel[i]);
			BreakAt(0x1000, 0x1039);
			const Observation a = Observe("A");
			RemoveBreakpoints();
			BreakAt(0x103c, 0x1058);
			const Observation b = Observe("B");
			RemoveBreakpoints();
			for (const Observation *o : {&a, &b})
			{
				Require(o->cpu[1] == 0x7e && o->cpu[2] == 0 && o->cpu[3] == 0 && o->cpu[5] == 0xf0
					&& (o->cpu[4] & 0xcf) == 6 && o->cpu[6] == 0 && (o->cpu[7] & 7) == 7
					&& !o->display && !o->sprites && o->model == model, "Authored A/B CPU or DMA state mismatch");
				for (unsigned i = 1; i < sizeof(sentinel); ++i) Require(o->ram[i] == sentinel[i], "Sentinel tail changed");
			}
			Require(a.cpu[0] == 0x1039 && b.cpu[0] == 0x1058 && a.ram[0] == 0x11 && b.ram[0] == 0x22
				&& a.border == 6 && a.background == 6 && b.border == 2 && b.background == 2
				&& a.counter < b.counter && a.frame < b.frame, "Authored A and B must be independently distinct");
			const auto covered = manager->GetTimelineStatus();
			Require(covered.seekSupported && covered.recording && !covered.busy && covered.verifiedCheckpoints > 0
				&& covered.bounds.available && covered.bounds.firstCounter <= a.counter && covered.bounds.lastCounter >= a.counter
				&& covered.instructionCoverage && covered.atInstructionBoundary, "A needs real retained checkpoint and boundary coverage");
			LOGM("TimelineCancellation fixture model=%d A=%llu B=%llu first=%llu last=%llu checkpoints=%llu", model,
				(unsigned long long)a.counter, (unsigned long long)b.counter, (unsigned long long)covered.bounds.firstCounter,
				(unsigned long long)covered.bounds.lastCounter, (unsigned long long)covered.verifiedCheckpoints);
			StepCompleted(1, true, "Distinct authored A/B states and verified earlier checkpoint coverage");

			TimelineSeek::Request request;
			request.value = a.counter;
			TimelineSeek::Result timed;
			{
				struct Guard {
					CSnapshotsManager &manager;
					explicit Guard(CSnapshotsManager &m) : manager(m) { manager.LockMutex(); }
					~Guard() { manager.UnlockMutex(); }
				} guard(*manager);
				// Real CPU Take/BeginMutation cannot acquire this same lock.  The
				// caller synchronously owns Submit/Wait; no fabricated CPU service.
				timed = manager->PerformTimelineSeek(request, 50);
				LogResult("pending-timeout", timed);
				Require(TimelineCancellationChecks::PendingTimeout(timed, covered.lastRequest.sequence,
					manager->TimelineNeedsCpuService(), manager->IsStrictTimelineSeekActive()), "Pending timeout admitted work or invented context");
			}
			CheckIdle(timed);
			Require(TimelineCancellationChecks::SameMachine(b, Observe("after-timeout")), "Pending timeout changed B");
			StepCompleted(2, true, "Manager-lock-excluded Pending request timed out before mutation");

			Require(b.counter < std::numeric_limits<TimelineSeek::Counter>::max(), "Future-target counter overflow");
			request.value = b.counter + 1;
			const auto future = manager->PerformTimelineSeek(request, 2000);
			LogResult("future-service-barrier", future);
			Require(TimelineCancellationChecks::Refusal(future, "future_target", timed.sequence, b, true), "New CPU-serviced request did not refuse the future target");
			CheckIdle(future);
			Require(TimelineCancellationChecks::SameMachine(b, Observe("after-service")), "Late CPU service changed B or restored A");
			StepCompleted(3, true, "Newer CPU-serviced refusal retains B without delayed restoration");

			manager->ClearSnapshotsHistory();
			Require(TimelineCancellationChecks::Cleared(manager->GetTimelineStatus()), "History clear left checkpoint or instruction coverage");
			Require(TimelineCancellationChecks::SameMachine(b, Observe("after-clear")), "Idle history clear changed live machine state");
			request.value = a.counter;
			const auto cleared = manager->PerformTimelineSeek(request, 2000);
			LogResult("cleared-history", cleared);
			// Clear invalidates coverage/its boundary hint, NOT the unchanged live
			// instruction cycle.  Do not confuse index metadata with CPU phase.
			Require(TimelineCancellationChecks::Refusal(cleared, "no_history", future.sequence, b, false), "Cleared history request did not refuse with its own sequence");
			CheckIdle(cleared);
			Require(TimelineCancellationChecks::Cleared(manager->GetTimelineStatus())
				&& TimelineCancellationChecks::SameMachine(b, Observe("after-no-history")), "History or live B changed during no-history refusal");
			StepCompleted(4, true, "Explicit paused history clear refuses the former target without machine advancement");
		}
		catch (const std::exception &e) { error = e.what(); }
		// This fixture owns only its breakpoints/settings.  Authored RAM/history
		// are disposable; it never claims to restore a previous interactive game.
		if (di->isRunning)
		{
			di->SetDebugMode(DEBUGGER_MODE_PAUSED);
			const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			while (!Parked() && std::chrono::steady_clock::now() < until) SYS_Sleep(5);
			if (!Parked()) error += "; cleanup pause did not settle";
		}
		RemoveBreakpoints();
		if (changedSettings)
		{
			manager->SetRecordingIsActive(recording);
			manager->SetRecordingStoreInterval(interval);
			manager->SetRecordingLimit(limit);
			di->SetSettingIsWarpSpeed(warp);
			if (c64SettingsSnapshotsRecordIsActive != recording || c64SettingsSnapshotsIntervalNumFrames != interval
				|| c64SettingsSnapshotsLimit != limit || di->GetSettingIsWarpSpeed() != warp)
				error += "; recording/warp settings did not restore";
		}
		if (!wasRunning && di->isRunning) viewC64->StopEmulationThread(di);
		if (error.empty()) StepCompleted(5, true, "Owned breakpoint/recording/warp settings restored");
		TestCompleted(error.empty(), error.empty() ? "Pending cancellation, newer service, and idle history clear; selected model only" : error.c_str());
#endif
	}
};
