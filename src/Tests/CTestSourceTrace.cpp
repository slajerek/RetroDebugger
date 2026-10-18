#include "CTestSourceTrace.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceC64U.h"
#include "C64UTraceBuffer.h"
#include "C64U6502Decoder.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebugSourceTrace.h"
#include "CSourceStepController.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

bool CTestSourceTrace::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

// a three-line loop at $C000: nop, nop, jmp $c000 -- lines 1, 2 and 3
static const char *kLoopText = "        nop\n        nop\n        jmp $c000\n";
static std::shared_ptr<CDebugDatabase> LoopDatabase()
{
	std::map<u32, i32> lines = { { 0xC000, 1 }, { 0xC001, 2 }, { 0xC002, 3 }, { 0xC003, 3 }, { 0xC004, 3 } };
	return DebugInfoFromAssembly("loop.asm", kLoopText, strlen(kLoopText), lines);
}

// the entries' lines, oldest first, as "1 2 3 ..." ("-" for unmapped)
static std::string Lines(const std::vector<CDebugSourceTraceEntry> &entries, bool *allReplayed, bool *noneReplayed)
{
	std::string s;
	*allReplayed = true;
	*noneReplayed = true;
	for (const CDebugSourceTraceEntry &entry : entries)
	{
		if (!s.empty())
			s += " ";
		s += entry.mapped ? std::to_string(entry.line) : std::string("-");
		*allReplayed = *allReplayed && entry.replayed;
		*noneReplayed = *noneReplayed && !entry.replayed;
	}
	return s;
}

bool CTestSourceTrace::RunLiveC64()
{
#ifdef RUN_COMMODORE64
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!di)
	{
		ReportRequiredGap("C64 not created");
		return true;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();
	u8 savedRam[5];
	for (int i = 0; i < 5; i++)
		savedRam[i] = di->GetByteFromRamC64((u16)(0xC000 + i));
	u16 pc;
	u8 a, x, y, p, sp;
	di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);

	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	scratch->AddAssembledSource(LoopDatabase(), 0xC000, 0xC005, {});

	const u8 loop[] = { 0xEA, 0xEA, 0x4C, 0x00, 0xC0 };
	for (int i = 0; i < 5; i++)
		di->SetByteToRamC64((u16)(0xC000 + i), loop[i]);
	di->SetSourceTraceRecording(true);
	di->MakeJmpC64(0xC000);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(100);
	di->PauseEmulationBlockedWait();
	di->SetSourceTraceRecording(false);

	// the loop's lines, in order, the newest last; interrupt code (KERNAL, no source) between them
	std::vector<CDebugSourceTraceEntry> entries = di->sourceTrace->GetEntries(64);
	std::vector<CDebugSourceTraceEntry> mapped;
	for (const CDebugSourceTraceEntry &entry : entries)
		if (entry.mapped)
			mapped.push_back(entry);
	bool allReplayed, noneReplayed;
	std::string lines = Lines(mapped, &allReplayed, &noneReplayed);
	bool cycle = mapped.size() >= 6;
	for (size_t i = 1; cycle && i < mapped.size(); i++)
		cycle = mapped[i].line == (mapped[i - 1].line % 3) + 1 && mapped[i].fileName == "loop.asm";
	char msg[300];
	snprintf(msg, sizeof(msg), "C64 live: recording gives the loop's lines in order, newest last, none replayed (%d entries, mapped: %.120s)",
			 (int)entries.size(), lines.c_str());
	bool ok = Check(cycle && noneReplayed && entries.size() <= 64, msg);

	for (int i = 0; i < 5; i++)
		di->SetByteToRamC64((u16)(0xC000 + i), savedRam[i]);
	di->MakeJmpC64(pc);
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = liveSymbols;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	// scratch leaked on purpose, see CTestSymbolLoaders
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);
	return ok;
#else
	ReportRequiredGap("C64 emulation not compiled in");
	return true;
#endif
}

// one bus cycle as the C64U's debug stream sends it: CPU phase, read
static uint32_t BusRead(u16 address, u8 data)
{
	return (uint32_t)address | ((uint32_t)data << 16) | (1u << 24) | (1u << 31);
}

// RD-017: remote source locations name the logical function and its inline calls
bool CTestSourceTrace::RunInlineLocation()
{
#ifdef RUN_COMMODORE64
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!di)
	{
		ReportRequiredGap("C64 not created");
		return true;
	}
	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	CDebugInfoLoadOptions options;
	options.sourceRoots.push_back(ResolveProjectPath("tests/source-debug/fixtures/inline"));
	DebugInfoLoadFile(scratch, ResolveProjectPath("tests/data/source-debug/oscar64/c64-O2/inline/main.dbj").c_str(), options);
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	CDebuggerApi *api = di->GetDebuggerApi();
	nlohmann::json inlined = api->GetSourceLocationJson(0x0888);		// line 19, work() inlined into main at 24
	nlohmann::json own = api->GetSourceLocationJson(0x0891);			// line 25, main's own
	bool ok = Check(inlined.value("line", 0) == 19 && inlined.value("function", "") == "work"
					&& inlined.contains("inlined") && inlined["inlined"].size() == 1
					&& inlined["inlined"][0].value("function", "") == "work"
					&& inlined["inlined"][0]["calledFrom"].value("line", 0) == 24
					&& own.value("function", "") == "main" && !own.contains("inlined"),
					"Oscar64 -O2: the location of inlined code names the logical function work() and the call at main-O2.c:24; main's own line has no inline chain");

	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = liveSymbols;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	// scratch leaked on purpose, see CTestSymbolLoaders
	return ok;
#else
	ReportRequiredGap("C64 emulation not compiled in");
	return true;
#endif
}

bool CTestSourceTrace::RunReplayedC64U()
{
	CDebugInterfaceC64U *di = (CDebugInterfaceC64U *)viewC64->debugInterfaceC64U;
	if (!di || !viewC64->viewC64USourceCode)
	{
		ReportRequiredGap("C64U backend or its source view not created");
		return true;
	}
	CDebugInterface::CDebugBackendCapabilities capabilities = di->GetExecutionCapabilities();
	bool ok = Check(!capabilities.sourceStep && !capabilities.stepInstruction && !capabilities.instructionHook
					&& capabilities.executionTrace && !di->SourceStep(SourceStepKind::Over),
					"C64U: observed only -- no stepping in its capabilities, a source step is refused; an execution trace instead");

	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(liveSymbols->dataAdapter);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	scratch->AddAssembledSource(LoopDatabase(), 0xC000, 0xC005, {});

	// three times round the loop, cycle by cycle: nop (2), nop (2), jmp (3)
	std::vector<uint32_t> bus;
	for (int round = 0; round < 3; round++)
	{
		bus.push_back(BusRead(0xC000, 0xEA));
		bus.push_back(BusRead(0xC001, 0xEA));
		bus.push_back(BusRead(0xC001, 0xEA));
		bus.push_back(BusRead(0xC002, 0x4C));
		bus.push_back(BusRead(0xC002, 0x4C));
		bus.push_back(BusRead(0xC003, 0x00));
		bus.push_back(BusRead(0xC004, 0xC0));
	}
	bus.push_back(BusRead(0xC000, 0xEA));	// the next opcode fetch ends the last jmp
	di->SetSourceTraceRecording(true);
	if (ok)
	{
		di->GetDecoder6510()->Reset();
		di->DrainTraceBusData();		// what came before this test
		di->GetTraceBuffer()->Append(bus.data(), (int)bus.size());
		di->DrainTraceBusData();
		std::vector<CDebugSourceTraceEntry> entries = di->sourceTrace->GetEntries(64);
		bool allReplayed, noneReplayed;
		std::string lines = Lines(entries, &allReplayed, &noneReplayed);
		ok = Check(lines == "1 2 3 1 2 3 1 2 3 1" && allReplayed && entries[2].pc == 0xC002 && entries[2].instructions == 1,
				   "C64U: the decoded bus trace, replayed, gives the loop's lines 1 2 3 three times, each marked replayed (" + lines + ")");
	}
	if (ok)
	{
		nlohmann::json trace = di->GetDebuggerApi()->GetSourceTraceJson(3);
		ok = Check(trace.value("recording", false) && trace["lines"].size() == 3 && trace["lines"][2].value("line", 0) == 1
				   && trace["lines"][1].value("file", "") == "loop.asm" && trace["capabilities"].value("sourceStep", true) == false,
				   "C64U remote: source/trace returns the last three lines and the capabilities (sourceStep false)");
	}
	di->SetSourceTraceRecording(false);

	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = liveSymbols;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	// scratch leaked on purpose, see CTestSymbolLoaders
	return ok;
}

void CTestSourceTrace::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = RunLiveC64() && RunReplayedC64U() && RunInlineLocation();
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Executed source lines are recorded live and from a replayed C64U bus trace");
}
