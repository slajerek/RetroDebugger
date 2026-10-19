#include "CTestSourceVariables.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugSourceWatch.h"
#include "CDebugVariableResolver.h"
#include "CDebugStackDepth.h"
#include "C64SettingsStorage.h"
#include "CViewDataDump.h"
#include "CViewDisassembly.h"
#include "CViewSourceCode.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugInfoLoader.h"
#include "CDebugDatabase.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#ifdef RUN_COMMODORE64
#include "CDebugInterfaceVice.h"
#include "CDebugSymbolsC64.h"
#endif
#include <fstream>
#include <iterator>
#include <vector>

#ifdef RUN_COMMODORE64
static CDebugInterfaceVice *Vice() { return (CDebugInterfaceVice *)viewC64->debugInterfaceC64; }
#endif

bool CTestSourceVariables::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

#ifdef RUN_COMMODORE64
static std::string currentDir;

// the fixture's PRG into RAM and its debug info into the interface's
// (scratch) symbols
bool CTestSourceVariables::Load(const std::string &compiler, const std::string &fixture, const char *debugInfo)
{
	CDebugInterfaceVice *di = Vice();
	currentDir = ResolveProjectPath(("tests/data/source-debug/" + compiler + "/c64/" + fixture).c_str());
	std::ifstream in(currentDir + "/main.prg", std::ios::binary);
	std::vector<u8> program((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	if (program.size() < 3)
		return Check(false, compiler + " " + fixture + ": main.prg is read");
	u16 load = (u16)(program[0] | (program[1] << 8));
	di->PauseEmulationBlockedWait();
	for (size_t i = 2; i < program.size(); i++)
		di->SetByteToRamC64((u16)(load + i - 2), program[i]);
	di->symbols->currentSegment->breakpointsPC->ClearBreakpoints();
	DebugInfoFormat format = DebugInfoLoadFile(di->symbols, (currentDir + "/" + debugInfo).c_str());
	return Check(format != DebugInfoFormat::Unknown && di->symbols->debugDatabase != NULL,
				 compiler + " " + fixture + ": the program and its debug info load");
}

// LAUNCH: from a stub at $C060 (SEI, CIA1 interrupts off, JMP ENTRY); else
// on from the last stop. Runs to the PC breakpoint at BREAKPOINT.
bool CTestSourceVariables::RunTo(u16 entry, u16 breakpoint, bool launch)
{
	CDebugInterfaceVice *di = Vice();
	di->LockMutex();
	di->symbols->currentSegment->AddBreakpointPC(breakpoint);		// the scratch symbols' segment
	di->UnlockMutex();
	if (launch)
	{
		di->SetByteC64(0x0000, 0x2F);		// the 6510 port as the KERNAL leaves it: the last program may have changed it
		di->SetByteC64(0x0001, 0x37);
		const u8 setup[] = { 0x78, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0xAD, 0x0D, 0xDC, 0x4C, (u8)(entry & 0xFF), (u8)(entry >> 8) };
		for (size_t i = 0; i < sizeof(setup); i++)
			di->SetByteToRamC64((u16)(0xC060 + i), setup[i]);
		di->MakeJmpC64(0xC060);
	}
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	for (int waited = 0; waited < 3000; waited += 5)
	{
		SYS_Sleep(5);
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && (u16)di->GetCpuPC() == breakpoint)
			break;
	}
	di->LockMutex();
	di->symbols->currentSegment->breakpointsPC->DeleteBreakpoint(breakpoint);
	di->UnlockMutex();
	char at[64];
	snprintf(at, sizeof(at), "$%04X (PC $%04X)", breakpoint, (unsigned)di->GetCpuPC());
	return Check(di->GetDebugMode() == DEBUGGER_MODE_PAUSED && (u16)di->GetCpuPC() == breakpoint,
				 std::string("the program stops at ") + at);
}

// a hard reset to the KERNAL's READY loop: Oscar64 programs use zero page up
// to $F7, the KERNAL's screen pointers included, which cc65's startup needs
static bool BootToReady(CDebugInterfaceVice *di)
{
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	di->ResetHard();
	SYS_Sleep(200);
	for (int waited = 0; waited < 10000; waited += 20)
	{
		u16 pc = (u16)di->GetCpuPC();
		if (di->GetByteFromRamC64(0x0326) == 0xCA && di->GetByteFromRamC64(0x0327) == 0xF1 && pc >= 0xE5CD && pc <= 0xE5D5)
		{
			di->PauseEmulationBlockedWait();
			return true;
		}
		SYS_Sleep(20);
	}
	di->PauseEmulationBlockedWait();
	return false;
}

nlohmann::json CTestSourceVariables::Variable(const std::string &name)
{
	nlohmann::json j = Vice()->GetDebuggerApi()->GetSourceVariablesJson(name, -1);
	return j.contains("variable") ? j["variable"] : j;
}

static bool Is(const nlohmann::json &v, const char *quality, int value)
{
	return v.value("quality", "") == std::string(quality) && v.contains("value") && v["value"] == value;
}

// cc65 records no types: the first byte instead of a typed value
static bool IsByte(const nlohmann::json &v, const char *quality, int value)
{
	return v.value("quality", "") == std::string(quality) && v.contains("bytes") && !v["bytes"].empty() && v["bytes"][0] == value;
}
#endif

void CTestSourceVariables::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulation is not compiled in, so C variables were never resolved");
#else
	CDebugInterfaceVice *di = Vice();
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();
	u8 savedPort[2] = { di->GetByteC64(0x0000), di->GetByteC64(0x0001) };
	u8 savedVector[2] = { di->GetByteFromRamC64(0x0314), di->GetByteFromRamC64(0x0315) };

	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	bool ok = true;
	// Oscar64 basic-flow: add(left, right) takes zero-page parameter registers;
	// at its entry ($08AA) from doSomething, left = result (1), right = 3
	ok = ok && Load("oscar64", "basic-flow", "main.dbj") && RunTo(0x080D, 0x08AA, true);
	if (ok)
	{
		nlohmann::json left = Variable("left"), right = Variable("right"), result = Variable("result");
		ok = Check(Is(left, "exact", 1) && Is(right, "exact", 3) && left.value("location", "") == "zero-page parameter"
				   && left.value("type", "") == "u8" && left["address"] == 0x0D,
				   "Oscar64: at add's entry its zero-page parameters are exact, left 1 and right 3 (" + left.dump() + ")")
			&& Check(Is(result, "exact", 1) && result.value("location", "") == "fixed" && result.value("size", 0) == 1,
					 "Oscar64: the global result is a fixed u8, exact (" + result.dump() + ")");
	}
	if (ok)
	{
		// the source view's context menu actions: result shown in the memory
		// dump, watched as an unsigned byte; add's left is not watchable
		// (a zero-page parameter register), nothing named "nothing" either
		CViewSourceCode *view = viewC64->viewC64SourceCode;
		guiMain->LockMutex();
		CDebugSymbols *viewSymbols = view->debugSymbols;
		view->debugSymbols = di->symbols;
		bool shown = view->ShowVariableInMemory("result");
		CViewDataDump *dump = view->viewDisassembly ? view->viewDisassembly->viewDataDump : NULL;
		int dumpStart = dump ? dump->dataShowStart : -1;
		bool watched = view->AddVariableWatch("result");
		CDebugSymbolsDataWatch *watch = di->symbols->currentSegment->FindWatch(2225);
		// RD-053: add's left (a zero-page parameter) is watched by name, not by address
		bool byName = view->AddVariableWatch("left");
		CDebugSourceWatch *leftWatch = di->symbols->FindSourceWatch("left", "add");
		bool refused = !view->AddVariableWatch("nothing");
		view->debugSymbols = viewSymbols;
		guiMain->UnlockMutex();
		char msg[200];
		snprintf(msg, sizeof(msg), "Oscar64: the source view shows result in the memory dump (start $%04X) and watches it as an unsigned byte at its address; add's parameter left is watched by name; no watch for an unknown name",
				 (unsigned)dumpStart);
		ok = Check(shown && dump && dumpStart <= 2225 && dumpStart + 0x400 > 2225 && watched && watch
				   && watch->representation == WATCH_REPRESENTATION_UNSIGNED_DEC_8 && byName && leftWatch
				   && !di->symbols->currentSegment->FindWatch(0) && refused, msg);
		if (ok)
		{
			di->symbols->UpdateSourceWatches(true);
			ok = Check(leftWatch->hasValue && leftWatch->live && leftWatch->state.empty() && leftWatch->FormatValue() == "1",
					   "RD-053: stopped in add, the source watch on left shows 1 (" + leftWatch->FormatValue() + ", " + leftWatch->state + ")");
		}
		di->symbols->DeleteAllSourceWatches();
	}
	if (ok)
	{
		nlohmann::json j = di->GetDebuggerApi()->GetSourceVariablesJson("", -1);
		bool all = j.contains("variables") && j["variables"].size() >= 3 && j["variables"][0].value("name", "") == "left"
			&& j["capabilities"].value("localVariables", false) && j["capabilities"].value("types", false);
		bool noMarkers = true;
		for (auto &v : j["variables"])
			noMarkers = noMarkers && v.value("name", "") != "BSSStart" && v.value("name", "") != "StackEnd";
		ok = Check(all && noMarkers, "Oscar64: in scope at add are its parameters, then the globals, without the linker markers");
	}

	// Oscar64 branches-loops: main's i is on the static stack, valid on its
	// lines 20-32; each pass of the loop ($0889, where BCC returns; Oscar64 stores i++ at $088E before the add ends) sees it 0, 1, 2
	ok = ok && Load("oscar64", "branches-loops", "main.dbj");
	for (int i = 0; ok && i < 3; i++)
	{
		ok = RunTo(0x080D, 0x0889, i == 0);
		nlohmann::json v = ok ? Variable("i") : nlohmann::json();
		ok = ok && Check(Is(v, "exact", i) && v.value("location", "") == "static stack" && v.value("function", "") == "main",
						 "Oscar64: main's static-stack i is exact on each pass of the loop: " + std::to_string(i) + " (" + v.dump() + ")");
	}
	if (ok)
	{
		nlohmann::json v = Variable("v");
		ok = Check(v.value("quality", "") == "missing" && v.value("reason", "").find("another function") != std::string::npos,
				   "Oscar64: classify's v is not in scope in main (" + v.dump() + ")");
	}

	// Oscar64 recursion: sum's n is frame-relative (word(SP) + 7). At sum's
	// first line ($08AD) it is 3, 2, 1, 0 down the recursion; at the entry
	// ($089C) the prologue has not set the frame up
	ok = ok && Load("oscar64", "recursion", "main.dbj");
	if (ok)
	{
		ok = RunTo(0x080D, 0x089C, true);
		nlohmann::json v = ok ? Variable("n") : nlohmann::json();
		ok = ok && Check(v.value("quality", "") == "missing" && v.value("reason", "").find("prologue") != std::string::npos,
						 "Oscar64: at sum's entry n is missing: the prologue has not set the frame up (" + v.dump() + ")");
	}
	for (int n = 3; ok && n >= 0; n--)
	{
		ok = RunTo(0x080D, 0x08AD, false);
		nlohmann::json v = ok ? Variable("n") : nlohmann::json();
		ok = ok && Check(Is(v, "exact", n) && v.value("location", "") == "stack frame",
						 "Oscar64: at sum's first line n is exact through the recursion: " + std::to_string(n) + " (" + v.dump() + ")");
	}

	// RD-053: a watch on sum's n, by name: exact at each stop down the
	// recursion, its last value kept (not live) while running, not in scope
	// in main, kept across a reload of the debug info and through the
	// watches' HJSON
	CDebugSourceWatch *nWatch = ok ? di->symbols->AddSourceWatch("n", "sum", WATCH_REPRESENTATION_UNSIGNED_DEC_8, 1) : NULL;
	if (ok)
	{
		ok = RunTo(0x080D, 0x089C, true);
		di->symbols->UpdateSourceWatches(true);
		ok = ok && Check(nWatch->state == "frame not set up" && nWatch->FormatValue().empty(),
						 "RD-053: at sum's entry the watch on n says the frame is not set up (" + nWatch->state + ")");
	}
	for (int n = 3; ok && n >= 2; n--)
	{
		ok = RunTo(0x080D, 0x08AD, false);
		di->symbols->UpdateSourceWatches(true);
		ok = ok && Check(nWatch->live && nWatch->state.empty() && nWatch->FormatValue() == std::to_string(n)
						 && nWatch->value.quality == DebugMappingQuality::Exact,
						 "RD-053: stopped at sum's first line the watch on n shows " + std::to_string(n) + " (" + nWatch->FormatValue() + ")");
	}
	if (ok)
	{
		di->symbols->UpdateSourceWatches(false);		// as the view does while the machine runs
		ok = Check(!nWatch->live && nWatch->hasValue && nWatch->FormatValue() == "2",
				   "RD-053: while running the watch on n keeps the last stop's value 2, marked as not live");
	}
	if (ok)
	{
		const CDebugFunction *mainFunction = NULL;
		for (const CDebugFunction &f : di->symbols->debugDatabase->Functions())
			if (f.name == "main")
				mainFunction = &f;
		CDebugVariableResolution r = mainFunction
			? DebugResolveVariableIn(di->symbols->debugDatabase.get(), di, (u32)di->symbols->currentSegment->debugRegionId, "n", "sum", (u16)mainFunction->start)
			: CDebugVariableResolution();
		ok = Check(mainFunction && r.variable == NULL && r.reason.find("not in scope") != std::string::npos,
				   "RD-053: in main, sum's n is not in scope (" + r.reason + ")");
	}
	if (ok)
	{
		Hjson::Value hjsonWatches;
		di->symbols->SerializeWatchesToHjson(hjsonWatches);
		DebugInfoLoadFile(di->symbols, (currentDir + "/main.dbj").c_str());
		bool keptOverReload = di->symbols->FindSourceWatch("n", "sum") == nWatch;
		di->symbols->DeleteAllSourceWatches();
		di->symbols->DeserializeWatchesFromHjson(hjsonWatches);
		CDebugSourceWatch *restored = di->symbols->FindSourceWatch("n", "sum");
		ok = Check(keptOverReload && restored && restored->representation == WATCH_REPRESENTATION_UNSIGNED_DEC_8
				   && hjsonWatches["SourceWatches"].size() == 1,
				   "RD-053: the watch on n survives a reload of main.dbj, and is saved and loaded with the watches (\"SourceWatches\")");
		di->symbols->DeleteAllSourceWatches();
	}

	// cc65 recursion: the global from its "_result" label, size not recorded;
	// n at offs 0 from sp ($02, from main.lbl), the stack depth not recorded
	int savedStackDecoding = c64SettingsCc65StackDecoding;
	c64SettingsCc65StackDecoding = (int)DebugStackDecoding::Off;
	ok = ok && Check(BootToReady(di), "the C64 is reset to READY for the cc65 build") && Load("cc65", "recursion", "main.dbg");
	if (ok)
	{
		nlohmann::json result = Variable("result");
		ok = Check(result.value("quality", "") == "approximate" && result["address"] == 0x09A7
				   && result.value("reason", "").find("size") != std::string::npos,
				   "cc65: the global result is at its label's $09A7, approximate: the size is not recorded (" + result.dump() + ")");
	}
	if (ok)
	{
		nlohmann::json j = di->GetDebuggerApi()->ResolveSourceLineJson("main.c", 8);
		u16 line8 = (j.contains("addresses") && !j["addresses"].empty()) ? (u16)j["addresses"][0].get<int>() : 0;
		ok = Check(line8 != 0, "cc65: sum's line 8 has code") && RunTo(0x080D, line8, true);
		nlohmann::json v = ok ? Variable("n") : nlohmann::json();
		u16 sp = (u16)(di->GetByteFromRamC64(0x02) | (di->GetByteFromRamC64(0x03) << 8));
		ok = ok && Check(v.value("quality", "") == "approximate" && v.value("location", "") == "stack frame"
						 && v.contains("address") && v["address"] == sp && v.value("reason", "").find("stack depth") != std::string::npos,
						 "cc65, stack depth decoding off: sum's n is word(sp) + 0, approximate: the stack depth is not recorded (" + v.dump() + ")");
		// RD-055: decoded, the depth at line 8 is the prologue's: n is exact
		c64SettingsCc65StackDecoding = (int)DebugStackDecoding::Full;
		v = ok ? Variable("n") : nlohmann::json();
		ok = ok && Check(IsByte(v, "exact", 3) && v["address"] == sp,
						 "cc65, decoding full: at sum's line 8 n is exact: 3 at word(sp) (" + v.dump() + ")");
		c64SettingsCc65StackDecoding = (int)DebugStackDecoding::Limited;
		v = ok ? Variable("n") : nlohmann::json();
		ok = ok && Check(IsByte(v, "exact", 3), "cc65, decoding limited: at line 8 no branch lies on the way from the entry: n is exact 3 (" + v.dump() + ")");
	}
	if (ok)
	{
		// line 10 (return n + sum(n - 1)) is reached through the if's branch
		nlohmann::json j = di->GetDebuggerApi()->ResolveSourceLineJson("main.c", 10);
		u16 line10 = (j.contains("addresses") && !j["addresses"].empty()) ? (u16)j["addresses"][0].get<int>() : 0;
		ok = Check(line10 != 0, "cc65: sum's line 10 has code") && RunTo(0x080D, line10, false);
		c64SettingsCc65StackDecoding = (int)DebugStackDecoding::Limited;
		nlohmann::json limited = ok ? Variable("n") : nlohmann::json();
		c64SettingsCc65StackDecoding = (int)DebugStackDecoding::Full;
		nlohmann::json full = ok ? Variable("n") : nlohmann::json();
		ok = ok && Check(limited.value("quality", "") == "approximate" && limited.value("reason", "").find("branch") != std::string::npos
						 && IsByte(full, "exact", 3),
						 "cc65: at sum's line 10, after the if's branch, limited decoding is approximate and full decoding exact: n = 3 ("
						 + limited.value("reason", "") + "; " + full.dump() + ")");
	}

	// RD-055 nested-calls: middle(v) { unsigned char w = leaf(v); return leaf(w); }
	// cc65 pushes w after the first call: at line 14, v is at sp + 1, w at sp + 0
	// (offs 0 and -1 count from before that push). From top(1): v = 1, w = 2.
	ok = ok && Check(BootToReady(di), "the C64 is reset to READY for the cc65 nested-calls build") && Load("cc65", "nested-calls", "main.dbg");
	if (ok)
	{
		nlohmann::json j = di->GetDebuggerApi()->ResolveSourceLineJson("main.c", 14);
		u16 line14 = (j.contains("addresses") && !j["addresses"].empty()) ? (u16)j["addresses"][0].get<int>() : 0;
		ok = Check(line14 != 0, "cc65: middle's line 14 has code") && RunTo(0x080D, line14, true);
		struct Mode { DebugStackDecoding mode; int v, w; const char *quality; };
		const Mode modes[] = {
			{ DebugStackDecoding::Off, 2, -1, "approximate" },		// v read at sp + 0: that is w's byte
			{ DebugStackDecoding::Limited, 1, 2, "exact" },
			{ DebugStackDecoding::Full, 1, 2, "exact" },
		};
		for (const Mode &m : modes)
		{
			if (!ok)
				break;
			c64SettingsCc65StackDecoding = (int)m.mode;
			nlohmann::json v = Variable("v"), w = Variable("w");
			bool good = IsByte(v, m.quality, m.v) && (m.w < 0 || IsByte(w, m.quality, m.w));
			ok = Check(good, std::string("cc65, decoding ") + DebugStackDecodingName(m.mode) + ": at middle's line 14 v = "
					   + (v.contains("bytes") ? v["bytes"].dump() : "?") + ", w = " + (w.contains("bytes") ? w["bytes"].dump() : "?")
					   + (m.mode == DebugStackDecoding::Off ? " (off: v is read at w's byte -- the wrong value decoding fixes)" : " (exact)"));
		}
	}
	c64SettingsCc65StackDecoding = (int)DebugStackDecoding::Full;

	c64SettingsCc65StackDecoding = savedStackDecoding;

	// KickC nested-calls: "result" a zero-page global (.label result = 2);
	// leaf(__register(A) char v): at its entry ($082C) from middle, v = A =
	// top.v = 1; top's ".label v = 1" is a folded constant, not a label at $0001
	ok = ok && Load("kickc", "nested-calls", "main.dbg");
	if (ok)
	{
		CDebugSymbolsCodeLabel *atPort = di->symbols->currentSegment->FindLabel(0x0001);
		ok = Check(atPort == NULL, "KickC: the folded constant v is not a label at $0001 (the CPU port)");
	}
	ok = ok && RunTo(0x080D, 0x082C, true);
	if (ok)
	{
		nlohmann::json v = Variable("v"), result = Variable("result");
		ok = Check(Is(v, "exact", 1) && v.value("location", "") == "register" && v.value("register", "") == "A",
				   "KickC: at leaf's entry its register parameter v is A, exact: 1 (" + v.dump() + ")")
			&& Check(result.value("location", "") == "fixed" && result["address"] == 2 && result.value("quality", "") == "approximate",
					 "KickC: the global result is fixed at $02, approximate: KickC records no size (" + result.dump() + ")");
	}
	ok = ok && RunTo(0x080D, 0x0825, true);
	if (ok)
	{
		nlohmann::json v = Variable("v");
		ok = Check(v.value("location", "") == "constant" && v.value("constant", -1) == 1 && v.value("quality", "") == "missing",
				   "KickC: in top, v is the folded constant 1, no memory location (" + v.dump() + ")");
	}

	di->PauseEmulationBlockedWait();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = liveSymbols;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	// scratch leaked on purpose, see CTestSymbolLoaders
	di->SetByteC64(0x0000, savedPort[0]);
	di->SetByteC64(0x0001, savedPort[1]);
	di->SetByteToRamC64(0x0314, savedVector[0]);
	di->SetByteToRamC64(0x0315, savedVector[1]);
	di->SetByteC64(0xDC0D, 0x81);
	di->MakeJmpC64(0xE5CD);		// back to the KERNAL's READY loop
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (!ok)
		TestCompleted(false, failure.c_str());
	else
		TestCompleted(true, "C variables resolve with their locations, values and qualities (Oscar64, cc65, KickC)");
#endif
}
