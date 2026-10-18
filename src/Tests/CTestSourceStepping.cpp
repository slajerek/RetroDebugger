#include "CTestSourceStepping.h"
#include "CDebugEventsHistory.h"
#include "CDebuggerApi.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterfaceVice.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebugSourceBreakpoint.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugExecutionContext.h"
#include "CStackAnnotation.h"
#include "CSourceStepController.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstdio>
#include <filesystem>
#include <vector>

bool CTestSourceStepping::Check(bool condition, const char *message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message);
	return true;
}

static CDebugInstructionBoundary B(u16 previousPc, u8 previousOpcode, u8 previousSp, u16 pc, u8 sp)
{
	CDebugInstructionBoundary b;
	b.previousPc = previousPc;
	b.previousOpcode = previousOpcode;
	b.previousSp = previousSp;
	b.pc = pc;
	b.sp = sp;
	b.event = DebugClassifyBoundary(previousOpcode, previousSp, sp);
	return b;
}

bool CTestSourceStepping::RunTracker()
{
	CDebugExecutionContextTracker t;
	t.Apply(B(0x1000, 0x20, 0xF0, 0x2000, 0xEE));		// JSR $2000
	t.Apply(B(0x2000, 0x20, 0xEE, 0x3000, 0xEC));		// JSR $3000
	bool ok = t.GetCallDepth() == 2 && t.GetFrames()[1].fromPc == 0x2000 && t.GetFrames()[1].entryPc == 0x3000;
	t.Apply(B(0x3000, 0x60, 0xEC, 0x2003, 0xEE));		// RTS
	ok = ok && t.GetCallDepth() == 1 && !t.HasLeftBase();
	if (!Check(ok, "Tracker: JSR pushes a call frame, RTS pops it"))
		return false;

	// An IRQ taken right after a JSR: the call frame and the interrupt frame
	// both exist, and RTI then RTS unwind them in order.
	t.Reset();
	t.Apply(B(0x1000, 0xEA, 0xF0, 0x1001, 0xF0));
	t.Apply(B(0x1001, 0x20, 0xF0, 0xFF48, 0xEB));		// JSR, then IRQ: -2 -3
	ok = t.GetCallDepth() == 1 && t.GetInterruptDepth() == 1 && t.IsInInterrupt();
	t.Apply(B(0xEA81, 0x40, 0xEB, 0x2000, 0xEE));		// RTI back into the callee
	ok = ok && t.GetCallDepth() == 1 && !t.IsInInterrupt();
	t.Apply(B(0x2000, 0x60, 0xEE, 0x1004, 0xF0));		// RTS
	ok = ok && t.GetDepth() == 0 && !t.HasLeftBase();
	if (!Check(ok, "Tracker: an IRQ after a JSR stacks a call and an interrupt frame; RTI and RTS unwind them"))
		return false;

	// A routine that drops its return address (PLA PLA) ends its frame; a
	// return from the frame tracking started in leaves the base.
	t.Reset();
	t.Apply(B(0x1000, 0xEA, 0xF0, 0x1001, 0xF0));
	t.Apply(B(0x1001, 0x20, 0xF0, 0x2000, 0xEE));
	t.Apply(B(0x2000, 0x68, 0xEE, 0x2001, 0xEF));
	t.Apply(B(0x2001, 0x68, 0xEF, 0x2002, 0xF0));
	ok = t.GetDepth() == 0 && !t.HasLeftBase();
	t.Apply(B(0x2002, 0x60, 0xF0, 0x0900, 0xF2));
	ok = ok && t.HasLeftBase();
	if (!Check(ok, "Tracker: PLA/PLA of the return address ends the frame; an RTS above the base leaves it"))
		return false;

	// Seeding from the stack annotation: from the top, a JSR pair (return
	// $0812+1), a PHA, an IRQ triple, a JSR pair, then an unannotated byte
	CStackAnnotationData stack;
	stack.Clear();
	u8 page[0x100] = {};
	auto put = [&](int at, u8 type, u16 origin, u8 value) { stack.entryTypes[at] = type; stack.originPC[at] = origin; page[at] = value; };
	put(0xF1, STACK_ENTRY_JSR_PCL, 0x0810, 0x12);	put(0xF2, STACK_ENTRY_JSR_PCH, 0x0810, 0x08);
	put(0xF3, STACK_ENTRY_VALUE, 0x0900, 0x55);
	put(0xF4, STACK_ENTRY_IRQ_STATUS, 0x0A00, 0x20);	put(0xF5, STACK_ENTRY_IRQ_PCL, 0x0A00, 0x00);	put(0xF6, STACK_ENTRY_IRQ_PCH, 0x0A00, 0x0A);
	put(0xF7, STACK_ENTRY_JSR_PCL, 0x0B00, 0x02);	put(0xF8, STACK_ENTRY_JSR_PCH, 0x0B00, 0x0B);
	std::vector<CDebugPhysicalFrame> seeded;
	bool complete = DebugSeedFramesFromStack(stack, page, 0xF0, seeded);
	ok = !complete && seeded.size() == 3
		&& seeded[2].kind == DebugFrameKind::Call && seeded[2].fromPc == 0x0810 && seeded[2].returnPc == 0x0813 && seeded[2].spBefore == 0xF2
		&& seeded[1].kind == DebugFrameKind::Interrupt && seeded[1].returnPc == 0x0A00 && seeded[1].spBefore == 0xF6
		&& seeded[0].kind == DebugFrameKind::Call && seeded[0].fromPc == 0x0B00 && seeded[0].returnPc == 0x0B03 && seeded[0].seeded;
	// fully annotated to $01FF: complete
	for (int at = 0xF9; at <= 0xFF; at++)
		put(at, STACK_ENTRY_VALUE, 0, 0);
	complete = DebugSeedFramesFromStack(stack, page, 0xF0, seeded);
	ok = ok && complete && seeded.size() == 3;
	return Check(ok, "Tracker: frames seeded from the stack annotation -- JSR pairs and IRQ triples, PHA skipped, an unannotated byte ends the walk as incomplete");

}

bool CTestSourceStepping::Step(CDebugInterfaceVice *di, SourceStepKind kind, u16 expectedPc, int expectedLine,
							   SourceStepStopReason expectedReason, u64 budget)
{
	bool armed = di->SourceStep(kind, budget);
	bool paused = false;
	for (int waited = 0; armed && waited < 5000; waited += 10)
	{
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && !di->sourceStepper->IsArmed())
		{
			paused = true;
			break;
		}
		SYS_Sleep(10);
	}
	CSourceStepResult r = di->sourceStepper->GetLastResult();

	char msg[256];
	snprintf(msg, sizeof(msg), "Oscar64 basic-flow: step %s stops at $%04X line %d (%s): got $%04X line %d (%s, %llu instructions)",
			 SourceStepKindName(kind), expectedPc, expectedLine, SourceStepStopReasonName(expectedReason),
			 r.pc, r.mapped ? r.line : 0, SourceStepStopReasonName(r.reason), (unsigned long long)r.instructions);
	return Check(armed && paused && r.pc == expectedPc && r.reason == expectedReason
				 && (expectedLine == 0 ? !r.mapped : (r.mapped && r.line == expectedLine)), msg);
}

static bool ReadFile(const std::string &path, std::vector<u8> &out)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return false;
	u8 buffer[4096];
	size_t n;
	while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
		out.insert(out.end(), buffer, buffer + n);
	fclose(f);
	return true;
}

bool CTestSourceStepping::RunOscar64(CDebugInterfaceVice *di)
{
	std::vector<u8> prg;
	std::string dir = ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow");
	if (!Check(ReadFile(dir + "/main.prg", prg) && prg.size() > 2 && prg[0] == 0x01 && prg[1] == 0x08,
			   "Oscar64 basic-flow: main.prg loads at $0801"))
		return false;

	CDebugSymbols *scratch = di->symbols;
	DebugInfoFormat format = DebugInfoLoadFile(scratch, (dir + "/main.dbj").c_str());
	if (!Check(format == DebugInfoFormat::Oscar64Dbj && scratch->debugDatabase
			   && scratch->currentSegment && scratch->currentSegment->debugRegionId >= 0,
			   "Oscar64 basic-flow: main.dbj loads into the C64 symbols with a region for the current segment"))
		return false;

	for (size_t i = 2; i < prg.size(); i++)
		di->SetByteToRamC64((u16)(0x0801 + i - 2), prg[i]);

	// $C060 SEI, CIA1 timer A IRQ every 150 cycles, CLI, JMP $080D (startup)
	// $C040 IRQ handler via $0314: INC $02, JSR add ($08AA, mapped line 8), ack, RTI
	const u8 setup[] = { 0x78, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0xAD, 0x0D, 0xDC,
						 0xA9, 0x96, 0x8D, 0x04, 0xDC, 0xA9, 0x00, 0x8D, 0x05, 0xDC,
						 0xA9, 0x81, 0x8D, 0x0D, 0xDC, 0xA9, 0x11, 0x8D, 0x0E, 0xDC,
						 0x58, 0x4C, 0x0D, 0x08 };
	const u8 handler[] = { 0xE6, 0x02, 0x20, 0xAA, 0x08, 0xAD, 0x0D, 0xDC, 0xA9, 0xFF, 0x8D, 0x19, 0xD0,
						   0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40 };
	for (int i = 0; i < (int)sizeof(setup); i++)
		di->SetByteToRamC64(0xC060 + i, setup[i]);
	for (int i = 0; i < (int)sizeof(handler); i++)
		di->SetByteToRamC64(0xC040 + i, handler[i]);
	di->SetByteToRamC64(0x0314, 0x40);
	di->SetByteToRamC64(0x0315, 0xC0);
	di->SetByteToRamC64(0x0002, 0);

	di->LockMutex();
	scratch->currentSegment->AddBreakpointPC(0x0880);
	di->UnlockMutex();
	di->MakeJmpC64(0xC060);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	bool paused = false;
	for (int waited = 0; waited < 3000 && !paused; waited += 10)
	{
		paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED;
		if (!paused)
			SYS_Sleep(10);
	}
	di->LockMutex();
	scratch->currentSegment->breakpointsPC->DeleteBreakpoint(0x0880);
	di->UnlockMutex();
	u16 pc;
	u8 a, x, y, p, sp;
	di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);

	bool ok = Check(paused && pc == 0x0880, "Oscar64 basic-flow: a breakpoint on main() stops at $0880 (line 16)");

	// main: 16 result = 1; 17 doSomething(); 18 result = add(result, 2); 19 for (;;)
	// doSomething: 12 result = add(result, 3); 13 }     add: 8 return left + right;
	ok = ok && Step(di, SourceStepKind::Over, 0x0885, 17, SourceStepStopReason::Completed);
	ok = ok && Step(di, SourceStepKind::Into, 0x089A, 12, SourceStepStopReason::Completed);
	ok = ok && Step(di, SourceStepKind::Into, 0x08AA, 8, SourceStepStopReason::Completed);
	if (ok)
	{
		// RD-012: the call stack seeded from the stack annotation, in add()
		nlohmann::json stack = di->GetDebuggerApi()->GetCallStackJson();
		const nlohmann::json &frames = stack["frames"];
		bool seeded = stack["current"].value("function", "") == "add" && frames.size() >= 2
			&& frames[0].value("kind", "") == "call" && frames[0]["caller"].value("function", "") == "doSomething"
			&& frames[0]["caller"].value("line", 0) == 12
			&& frames[1]["caller"].value("function", "") == "main" && frames[1]["caller"].value("line", 0) == 17;
		std::string message = "Call stack from the stack annotation in add(): called from doSomething() line 12, called from main() line 17: " + stack.dump();
		ok = Check(seeded, message.c_str());
	}
	ok = ok && Step(di, SourceStepKind::Out, 0x08A6, 12, SourceStepStopReason::Completed);
	ok = ok && Step(di, SourceStepKind::Over, 0x08A9, 13, SourceStepStopReason::Completed);
	ok = ok && Step(di, SourceStepKind::Over, 0x0888, 18, SourceStepStopReason::FrameExited);
	ok = ok && Step(di, SourceStepKind::Into, 0x08AA, 8, SourceStepStopReason::Completed);
	ok = ok && Step(di, SourceStepKind::Into, 0x0894, 18, SourceStepStopReason::FrameExited);
	// line 19's loop has no mapping: the step runs it until the budget
	ok = ok && Step(di, SourceStepKind::Over, 0x0897, 0, SourceStepStopReason::BudgetExceeded, 20000);

	if (ok)
	{
		char msg[160];
		u8 result = di->GetByteFromRamC64(0x08B1);
		u8 irqs = di->GetByteFromRamC64(0x0002);
		snprintf(msg, sizeof(msg), "Oscar64 basic-flow: the program computed result = %d (1 + 3 + 2) while the IRQ called add() %d times (mod 256)", result, irqs);
		ok = Check(result == 6 && irqs != 0 && !di->IsObservingExecution(), msg);
	}

	if (ok)
	{
		// every stop left an entry in the events history (RD-013)
		int steps = 0;
		bool first = false, budget = false;
		CDebugEventsHistory *history = scratch->debugEventsHistory;
		history->mutex->Lock();
		for (auto &it : history->events)
		{
			if (it.second->eventType != DEBUG_EVENT_TYPE_STEP)
				continue;
			const char *description = ((CDebugEventStep *)it.second)->description;
			steps++;
			first = first || !strcmp(description, "Source Over: completed at main.c:17 ($0885)");
			budget = budget || !strcmp(description, "Source Over: budget-exceeded at $0897");
		}
		history->mutex->Unlock();
		char msg[160];
		snprintf(msg, sizeof(msg), "Oscar64 basic-flow: each of the 9 source steps left a step event in the history (%d found)", steps);
		ok = Check(steps == 9 && first && budget, msg);
	}

	return ok;
}

static bool HasPcBreakpoint(CDebugSymbols *symbols, int address)
{
	return symbols->currentSegment && symbols->currentSegment->breakpointsPC->GetBreakpoint(address) != NULL;
}

bool CTestSourceStepping::RunSourceBreakpoints(CDebugInterfaceVice *di)
{
	CDebugSymbols *symbols = di->symbols;
	const CDebugDatabase *database = symbols->debugDatabase.get();
	std::vector<const CDebugSourceMapping *> out;
	i32 line12, line11, line9, line15, full, other;
	DebugResolveSourceLine(database, "main.c", 12, &line12, out);
	bool ok = line12 == 12 && out.size() == 1 && out[0]->start == 0x089A;
	DebugResolveSourceLine(database, "main.c", 11, &line11, out);
	ok = ok && line11 == 12 && out.size() == 1 && out[0]->start == 0x089A;
	DebugResolveSourceLine(database, "main.c", 15, &line15, out);
	ok = ok && line15 == 16 && out.size() == 1 && out[0]->start == 0x0880;
	DebugResolveSourceLine(database, "main.c", 9, &line9, out);
	ok = ok && line9 == 0 && out.empty();
	DebugResolveSourceLine(database, "/tmp/rd-source-debug/oscar64/c64/basic-flow/main.c", 18, &full, out);
	ok = ok && full == 18 && out.size() == 1 && out[0]->start == 0x0888;
	DebugResolveSourceLine(database, "other.c", 12, &other, out);
	ok = ok && other == 0 && out.empty();
	if (!Check(ok, "Source breakpoints: lines resolve by file name or recorded path; a line without code slides to the next one, not into the next function"))
		return false;

	// A breakpoint on doSomething's header line stops at its first statement.
	CDebugSourceBreakpoint *bp = symbols->AddSourceBreakpoint("main.c", 11);
	ok = Check(bp->IsResolved() && bp->resolvedLine == 12 && HasPcBreakpoint(symbols, 0x089A),
			   "Source breakpoint main.c:11 resolves to line 12 and an address breakpoint at $089A");
	if (!ok)
		return false;

	di->SetByteToRamC64(0x0314, 0x40);
	di->SetByteToRamC64(0x0315, 0xC0);
	di->MakeJmpC64(0xC060);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	bool paused = false;
	for (int waited = 0; waited < 3000 && !paused; waited += 10)
	{
		paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED;
		if (!paused)
			SYS_Sleep(10);
	}
	u16 pc;
	u8 a, x, y, p, sp;
	di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
	char msg[160];
	snprintf(msg, sizeof(msg), "Source breakpoint stops the rerun program in doSomething() at $089A (PC $%04X, result %d)",
			 pc, di->GetByteFromRamC64(0x08B1));
	if (!Check(paused && pc == 0x089A && di->GetByteFromRamC64(0x08B1) == 1, msg))
		return false;

	// Reload: the old segments go, the source breakpoint binds to the new one.
	std::string dbj = ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow/main.dbj");
	DebugInfoFormat format = DebugInfoLoadFile(symbols, dbj.c_str());
	if (!Check(format == DebugInfoFormat::Oscar64Dbj && bp->IsResolved() && HasPcBreakpoint(symbols, 0x089A),
			   "After a reload of main.dbj the source breakpoint is bound again in the new segment"))
		return false;

	// Unload and load: unresolved in between, kept as intent.
	symbols->DeleteSourceDebugInfo();
	ok = !bp->IsResolved() && !HasPcBreakpoint(symbols, 0x089A) && symbols->sourceBreakpoints.size() == 1;
	DebugInfoLoadFile(symbols, dbj.c_str());
	ok = ok && bp->IsResolved() && HasPcBreakpoint(symbols, 0x089A);
	if (!Check(ok, "Without debug info the source breakpoint stays, unresolved and with no address breakpoint; it binds again on load"))
		return false;

	// A user's own address breakpoint is shared, never deleted.
	symbols->currentSegment->AddBreakpointPC(0x0880);
	CDebugSourceBreakpoint *onMain = symbols->AddSourceBreakpoint("main.c", 16);
	ok = onMain->IsResolved() && !onMain->bindings[0].created;
	symbols->RemoveSourceBreakpoint(onMain);
	ok = ok && HasPcBreakpoint(symbols, 0x0880);
	symbols->RemoveSourceBreakpoint(bp);
	ok = ok && !HasPcBreakpoint(symbols, 0x089A) && symbols->sourceBreakpoints.empty();
	symbols->currentSegment->breakpointsPC->DeleteBreakpoint(0x0880);
	if (!Check(ok, "Removing source breakpoints deletes only the address breakpoints they created"))
		return false;

	// Persistence: the breakpoints file keeps file + line, not the address
	// breakpoint it made; loading it binds the source breakpoint again.
	std::string bpPath = "/tmp/rd-source-breakpoints.hjson";
	symbols->AddSourceBreakpoint("main.c", 11);
	CSlrString *bpPathSlr = new CSlrString(bpPath.c_str());
	symbols->SaveBreakpointsRetroDebuggerFormat(bpPathSlr);
	CDebugSourceBreakpoint *saved = symbols->FindSourceBreakpoint("main.c", 11);
	ok = saved && saved->IsResolved() && HasPcBreakpoint(symbols, 0x089A);
	if (saved)
		symbols->RemoveSourceBreakpoint(saved);
	ok = ok && !HasPcBreakpoint(symbols, 0x089A);
	symbols->LoadBreakpointsRetroDebuggerFormat(bpPathSlr);
	delete bpPathSlr;
	CDebugSourceBreakpoint *loaded = symbols->FindSourceBreakpoint("main.c", 11);
	ok = ok && loaded && loaded->IsResolved() && loaded->resolvedLine == 12
		&& loaded->bindings.size() == 1 && loaded->bindings[0].created && HasPcBreakpoint(symbols, 0x089A);
	if (loaded)
		symbols->RemoveSourceBreakpoint(loaded);
	ok = ok && !HasPcBreakpoint(symbols, 0x089A) && symbols->sourceBreakpoints.empty();
	if (!Check(ok, "Source breakpoints save to the breakpoints file as file + line and bind again when it loads"))
		return false;

	// RD-036: the remote/MCP queries on the same data
	CDebuggerApi *api = di->GetDebuggerApi();
	nlohmann::json location = api->GetSourceLocationJson(0x0885);
	nlohmann::json resolved = api->ResolveSourceLineJson("main.c", 11);
	nlohmann::json added = api->AddSourceBreakpointJson("main.c", 11);
	nlohmann::json listed = api->ListSourceBreakpointsJson();
	bool removedBp = api->RemoveSourceBreakpoint("main.c", 11);
	ok = location.value("mapped", false) && location.value("line", 0) == 17 && location.value("function", "") == "main"
		&& location["candidates"].size() >= 1
		&& resolved.value("resolvedLine", 0) == 12 && resolved["addresses"] == nlohmann::json::array({0x089A})
		&& added.value("resolved", false) && added.value("resolvedLine", 0) == 12
		&& listed.size() == 1 && listed[0]["addresses"] == nlohmann::json::array({0x089A})
		&& removedBp && api->ListSourceBreakpointsJson().empty() && !api->RemoveSourceBreakpoint("main.c", 11);
	if (!Check(ok, "Remote/MCP queries: $0885 is main.c:17 in main(); main.c:11 resolves to line 12 at $089A; a source breakpoint adds, lists and removes"))
		return false;

	return CheckStaleDebugInfo();
}

// RD-014: stale debug info by file time, on scratch files in /tmp.
bool CTestSourceStepping::CheckStaleDebugInfo()
{
	namespace fs = std::filesystem;
	fs::path dir = "/tmp/rd-stale-debug-info";
	std::error_code error;
	fs::create_directories(dir, error);
	const char *names[] = { "main.prg", "main.dbj", "main.c" };
	for (const char *name : names)
	{
		FILE *f = fopen((dir / name).string().c_str(), "wb");
		if (f)
		{
			fputs("x", f);
			fclose(f);
		}
	}
	auto setTime = [&](const char *name, int secondsAgo) {
		fs::last_write_time(dir / name, fs::file_time_type::clock::now() - std::chrono::seconds(secondsAgo), error);
	};
	std::string prg = (dir / "main.prg").string(), dbj = (dir / "main.dbj").string();
	CDebugDatabase database("test");
	CDebugSourceFile *source = database.AddFile(0, "main.c");
	source->loadedPath = (dir / "main.c").string();
	source->fileName = "main.c";

	// one build: all three within a few seconds
	setTime("main.c", 1000);
	setTime("main.dbj", 990);
	setTime("main.prg", 990);
	bool ok = DebugInfoCheckStale(&database, dbj.c_str(), prg.c_str()).empty();
	// the source edited after the build
	setTime("main.c", 10);
	std::vector<std::string> edited = DebugInfoCheckStale(&database, dbj.c_str(), prg.c_str());
	ok = ok && edited.size() == 1 && edited[0].find("main.c changed after main.dbj") == 0;
	setTime("main.c", 1000);
	// the program rebuilt without its debug info
	setTime("main.prg", 100);
	std::vector<std::string> newer = DebugInfoCheckStale(&database, dbj.c_str(), prg.c_str());
	ok = ok && newer.size() == 1 && newer[0].find("main.prg is newer than main.dbj") == 0;
	// the debug info from a later build than the program
	setTime("main.prg", 2000);
	std::vector<std::string> older = DebugInfoCheckStale(&database, dbj.c_str(), prg.c_str());
	ok = ok && older.size() == 1 && older[0].find("main.dbj is newer than main.prg") == 0;
	// a missing debug file decides nothing
	ok = ok && DebugInfoCheckStale(&database, (dir / "none.dbj").string().c_str(), prg.c_str()).empty();
	return Check(ok, "Stale debug info: a source edited after the build, or a program and debug file from different builds, is reported; one build is not");
}

void CTestSourceStepping::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = RunTracker();

#ifdef RUN_COMMODORE64
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (ok && di)
	{
		bool wasRunning = di->isRunning;
		if (!wasRunning)
		{
			viewC64->StartEmulationThread(di);
			SYS_Sleep(2000);
		}
		di->PauseEmulationBlockedWait();

		// Scratch symbols, as in CTestSymbolLoaders: the live ones stay untouched.
		CDebugSymbols *liveSymbols = di->symbols;
		CDebugSymbols *scratch = new CDebugSymbolsC64(di);
		scratch->SetDataAdapter(di->dataAdapterC64);
		scratch->CreateDefaultSegment();
		guiMain->LockMutex();
		di->LockMutex();
		di->symbols = scratch;
		di->UnlockMutex();
		guiMain->UnlockMutex();

		u8 savedVector[2] = { di->GetByteFromRamC64(0x0314), di->GetByteFromRamC64(0x0315) };
		ok = RunOscar64(di);
		if (ok)
			ok = RunSourceBreakpoints(di);

		// stop the fast IRQ: KERNAL vector back, PAL timer A latch ($4025)
		di->PauseEmulationBlockedWait();
		di->SetByteToRamC64(0x0314, savedVector[0]);
		di->SetByteToRamC64(0x0315, savedVector[1]);
		di->SetByteC64(0xDC04, 0x25);
		di->SetByteC64(0xDC05, 0x40);

		di->PauseEmulationBlockedWait();
		guiMain->LockMutex();
		di->LockMutex();
		di->symbols = liveSymbols;
		di->UnlockMutex();
		guiMain->UnlockMutex();
		// scratch leaked on purpose, see CTestSymbolLoaders

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		if (!wasRunning)
			viewC64->StopEmulationThread(di);
	}
#else
	ReportRequiredGap("C64 emulation not compiled in");
#endif

	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Source Into/Over/Out land on the expected Oscar64 lines with interrupts running mapped code; source breakpoints resolve, stop and survive reloads");
}
