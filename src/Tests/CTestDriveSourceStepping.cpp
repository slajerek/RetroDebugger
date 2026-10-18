#include "CTestDriveSourceStepping.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CViewSourceCode.h"
#include "CDebugInterfaceVice.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebugSourceBreakpoint.h"
#include "CDebugEventsHistory.h"
#include "CSourceStepController.h"
#include "CDebugInstructionStepper.h"
#include "DebuggerDefs.h"
#include "C64SettingsStorage.h"
#include "CSnapshotsManager.h"
#include "SYS_Threading.h"
#include "SYS_Main.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

bool CTestDriveSourceStepping::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

bool CTestDriveSourceStepping::RunDrive()
{
#ifdef RUN_COMMODORE64
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	CViewSourceCode *view = viewC64->viewDrive1541SourceCode;
	if (!di || !view)
	{
		ReportRequiredGap("C64 or the 1541 Source code view not created");
		return true;
	}
	if (view->visible)
	{
		ReportRequiredGap("1541 Source code view is open: the test swaps its symbols");
		return true;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	// no timeline snapshots while the drive stops: a snapshot catches the
	// drive up to the C64's clock, and a drive stop inside that catch-up
	// cannot park the drive (c64d_debug_pause_check with allowRestore 0)
	bool savedRecording = c64SettingsSnapshotsRecordIsActive;
	di->snapshotsManager->SetRecordingIsActive(false);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(200);
	di->PauseEmulationBlockedWait();

	std::string dir = ResolveProjectPath("tests/data/source-debug/cc65/drive1541/drive1541");
	std::ifstream in(dir + "/main.bin", std::ios::binary);
	std::vector<u8> code((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	if (!Check(code.size() == 0x23, "1541: the drive1541 fixture's code reads (35 bytes for $0500)"))
	{
		di->snapshotsManager->SetRecordingIsActive(savedRecording);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		return false;
	}

	u8 savedRam[0x200];
	for (int i = 0; i < 0x200; i++)
		savedRam[i] = di->GetByteFromRam1541((u16)(0x0500 + i));

	CDebugSymbolsDrive1541 *liveSymbols = di->symbolsDrive1541;
	CDebugSymbolsDrive1541 *scratch = new CDebugSymbolsDrive1541(di);
	scratch->SetDataAdapter(di->dataAdapterDrive1541);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbolsDrive1541 = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	CDebugSymbols *viewSymbols = view->debugSymbols;
	view->debugSymbols = scratch;

	CDebugInfoLoadOptions options;
	options.sourceRoots.push_back(ResolveProjectPath("tests/source-debug/fixtures/drive1541"));
	DebugInfoFormat format = DebugInfoLoadFile(scratch, (dir + "/main.dbg").c_str(), options);
	bool ok = Check(format == DebugInfoFormat::Cc65Dbg && scratch->debugDatabase && scratch->currentSegment
					&& scratch->currentSegment->debugRegionId >= 0,
					"1541: the cc65 debug info of the drive code loads into the drive's symbols");

	// into drive RAM, the drive sent there; a source breakpoint on the loop's
	// line (a PC set while the drive is parked is checked at the new PC too:
	// the step at the end)
	CDebugSourceBreakpoint *atLoop = ok ? scratch->AddSourceBreakpoint("drive.s", 11) : NULL;
	ok = ok && Check(atLoop && atLoop->IsResolved() && atLoop->bindings[0].address == 0x0502,
					 "1541: a source breakpoint on drive.s:11 binds to $0502 in the drive's segment");

	auto waitForPause = [&](int timeoutMs) -> bool {
		for (int waited = 0; waited < timeoutMs; waited += 10)
		{
			if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && !di->driveSourceStepper->IsArmed())
				return true;
			SYS_Sleep(10);
		}
		return false;
	};
	bool driveSent = false;
	if (ok)
	{
		for (size_t i = 0; i < code.size(); i++)
			di->SetByteToRam1541((u16)(0x0500 + i), code[i]);
		di->SetByteToRam1541(0x0600, 0);
		di->MakeJmpNoReset1541(0x0500);
		driveSent = true;
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		bool paused = waitForPause(3000);
		char msg[300];
		snprintf(msg, sizeof(msg), "1541: the drive runs into its code and the source breakpoint stops the machine at $0502 (drive PC $%04X, mode %d, $0500: %02X %02X %02X %02X %02X, count %d, segment bps %d)",
				 di->GetDrive1541PC(), (int)di->GetDebugMode(), di->GetByteFromRam1541(0x0500), di->GetByteFromRam1541(0x0501),
				 di->GetByteFromRam1541(0x0502), di->GetByteFromRam1541(0x0503), di->GetByteFromRam1541(0x0504),
				 di->GetByteFromRam1541(0x0600), scratch->currentSegment ? (int)scratch->currentSegment->breakpointsPC->breakpoints.size() : -1);
		ok = Check(paused && di->GetDrive1541PC() == 0x0502, msg);
		scratch->RemoveSourceBreakpoint(atLoop);
	}
	u8 countAtStart = di->GetByteFromRam1541(0x0600);

	// source steps on the drive CPU; the C64 runs between the stops
	struct Step { SourceStepKind kind; int line; int depth; const char *name; };
	const Step steps[] = {
		{ SourceStepKind::Into, 16, 1, "Into" },	// into bump
		{ SourceStepKind::Over, 17, 1, "Over" },
		{ SourceStepKind::Out, 12, 0, "Out" },		// back after the jsr: inx
		{ SourceStepKind::Over, 13, 0, "Over" },	// jsr delay
		{ SourceStepKind::Over, 14, 0, "Over" },	// over delay: about 82000 drive cycles
		{ SourceStepKind::Over, 11, 0, "Over" },	// jmp loop: jsr bump again
		{ SourceStepKind::Over, 12, 0, "Over" },	// over bump: count 2
	};
	int depth = 0;
	u64 c64CyclesBeforeDelay = 0, c64CyclesAfterDelay = 0;
	for (const Step &step : steps)
	{
		if (!ok)
			break;
		if (step.line == 14)
			c64CyclesBeforeDelay = di->GetMainCpuCycleCounter();
		bool armed = di->SourceStepDrive(step.kind);
		bool paused = armed && waitForPause(3000);
		CSourceStepResult r = di->driveSourceStepper->GetLastResult();
		depth += r.depth;
		char msg[220];
		snprintf(msg, sizeof(msg), "1541: source %s stops at drive.s:%d, depth %d (got %s:%d at $%04X, depth %d, %s)", step.name, step.line,
				 step.depth, r.fileName.c_str(), r.mapped ? r.line : 0, r.pc, depth, SourceStepStopReasonName(r.reason));
		ok = Check(paused && r.mapped && r.line == step.line && r.fileName == "drive.s" && depth == step.depth
				   && r.reason == SourceStepStopReason::Completed && di->GetDrive1541PC() == r.pc, msg);
		if (step.line == 14)
			c64CyclesAfterDelay = di->GetMainCpuCycleCounter();
	}
	if (ok)
	{
		// the drive runs in step with the C64: delay outlasts its catch-up slices, so the C64 clock moves
		u8 count = di->GetByteFromRam1541(0x0600);
		char msg[180];
		snprintf(msg, sizeof(msg), "1541: bump ran twice during the steps (count %d to %d); the C64 ran %llu cycles while the drive stepped over delay",
				 countAtStart, count, (unsigned long long)(c64CyclesAfterDelay - c64CyclesBeforeDelay));
		ok = Check((u8)(count - countAtStart) == 2 && c64CyclesAfterDelay > c64CyclesBeforeDelay + 1000, msg);
	}

	// remote: the same CPU by name
	if (ok)
	{
		CDebuggerApi *api = CDebuggerApi::GetDebuggerApi(EMULATOR_TYPE_C64_VICE);
		nlohmann::json location = api->GetSourceLocationJson(-1, "drive1541");
		bool stepped = api->SourceStep("over", 0, "drive1541") && waitForPause(3000);
		nlohmann::json status = api->GetSourceStepStatusJson("drive1541");
		CDebuggerApi::SourceCpu unknown;
		ok = Check(location.value("fileName", "") == "drive.s" && location.value("line", 0) == 12 && stepped
				   && status.value("reason", "") == "completed" && status.value("line", 0) == 13
				   && !api->GetSourceCpu("drive1571", &unknown),
				   "1541 remote: cpu drive1541 locates the drive PC at drive.s:12 and steps over it to 13; an unknown cpu is refused");
	}

	// RD-037 on the drive: Step Over the JSR at $0506 runs delay() to its
	// return ($0509); then into bump() and Step Out to its caller's $0505
	if (ok)
	{
		auto waitForStep = [&]() {
			for (int waited = 0; waited < 3000; waited += 10)
			{
				if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && !di->driveInstructionStepper->IsArmed()
					&& !di->driveSourceStepper->IsArmed())
					return true;
				SYS_Sleep(10);
			}
			return false;
		};
		bool over = di->GetDrive1541PC() == 0x0506 && di->StepOverDrive() && waitForStep() && di->GetDrive1541PC() == 0x0509;
		bool intoBump = di->SourceStepDrive(SourceStepKind::Over) && waitForStep()		// jmp loop -> line 11
			&& di->SourceStepDrive(SourceStepKind::Into) && waitForStep() && di->GetDrive1541PC() == 0x050C;
		bool out = intoBump && di->StepOutDrive() && waitForStep() && di->GetDrive1541PC() == 0x0505;
		bool notJsr = !di->StepOverDrive();		// at $0505 (inx): not a JSR, not armed
		char msg[200];
		snprintf(msg, sizeof(msg), "1541: instruction Step Over a JSR runs to its return ($0509), Step Out of bump() to $0505; no Step Over arms at a non-JSR (PC $%04X)",
				 di->GetDrive1541PC());
		ok = Check(over && intoBump && out && notJsr, msg);
	}

	// the drive's events history: source and instruction steps leave entries
	// (one per C64 cycle: the drive's steps within one catch-up slice share it)
	if (ok)
	{
		int source = 0, instruction = 0;
		bool lastOut = false;
		CDebugEventsHistory *history = scratch->debugEventsHistory;
		history->mutex->Lock();
		for (auto &it : history->events)
		{
			if (it.second->eventType != DEBUG_EVENT_TYPE_STEP)
				continue;
			std::string description = ((CDebugEventStep *)it.second)->description;
			source += description.compare(0, 12, "1541 Source ") == 0;
			instruction += description.compare(0, 10, "1541 Step ") == 0;
			lastOut = description == "1541 Step Out: returned at $0505";
		}
		history->mutex->Unlock();
		ok = Check(source > 0 && instruction > 0 && lastOut,
				   "1541: the drive's events history has source and instruction step entries, the last 1541 Step Out: returned at $0505");
	}
	if (ok)
	{
		ok = Check(view->ShowAddress(0x050C) && view->GetSelectedLine() == 16,
				   "1541: the 1541 Source code view shows the drive's code ($050C is drive.s:16)");
	}
	if (ok)
	{
		// the drive parked after a stop is sent to $0500, which has a PC
		// breakpoint: it stops there, before the instruction runs (a PC set
		// while parked used to run its first instruction unchecked)
		u8 countBefore = di->GetByteFromRam1541(0x0600);
		di->LockMutex();
		scratch->currentSegment->AddBreakpointPC(0x0500);
		di->UnlockMutex();
		di->MakeJmpNoReset1541(0x0500);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		bool paused = waitForPause(3000);
		u16 pc = di->GetDrive1541PC();
		di->LockMutex();
		scratch->currentSegment->breakpointsPC->DeleteBreakpoint(0x0500);
		di->UnlockMutex();
		char msg[200];
		snprintf(msg, sizeof(msg), "1541: a drive PC set while the drive is parked stops at the breakpoint on the new PC $0500 (drive PC $%04X, count %d to %d)",
				 pc, countBefore, di->GetByteFromRam1541(0x0600));
		ok = Check(paused && pc == 0x0500 && di->GetByteFromRam1541(0x0600) == countBefore, msg);
	}

	// the drive back to its ROM, its RAM and the symbols back
	view->debugSymbols = viewSymbols;
	view->documents.Clear();
	di->CancelDriveSourceStep();
	di->PauseEmulationBlockedWait();
	for (int i = 0; i < 0x200; i++)
		di->SetByteToRam1541((u16)(0x0500 + i), savedRam[i]);
	guiMain->LockMutex();
	di->LockMutex();
	di->symbolsDrive1541 = liveSymbols;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	// scratch leaked on purpose, see CTestSymbolLoaders
	if (driveSent)
		di->ResetDrive1541();
	di->snapshotsManager->SetRecordingIsActive(savedRecording);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(200);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);
	return ok;
#else
	ReportRequiredGap("C64 emulation not compiled in");
	return true;
#endif
}

void CTestDriveSourceStepping::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = RunDrive();
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Source breakpoints and source steps on the 1541 drive CPU, with the C64 running");
}
