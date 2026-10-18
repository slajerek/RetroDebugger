#include "CTestSourceStepTrace.h"
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
#include "CSourceStepController.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include "json.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

using json = nlohmann::json;

bool CTestSourceStepTrace::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
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

static bool ReadJson(const std::string &path, json &out)
{
	std::ifstream in(path);
	if (!in)
		return false;
	try
	{
		in >> out;
	}
	catch (const std::exception &)
	{
		return false;
	}
	return true;
}

// 0x080D, "$080D" or a plain number
static int ParseAddress(const json &value)
{
	if (value.is_number_integer())
		return value.get<int>();
	if (!value.is_string())
		return -1;
	std::string s = value.get<std::string>();
	if (!s.empty() && s[0] == '$')
		return (int)strtol(s.c_str() + 1, NULL, 16);
	return (int)strtol(s.c_str(), NULL, 0);
}

static SourceStepKind ParseAction(const std::string &action)
{
	std::string a = action;
	if (a.compare(0, 6, "source") == 0)
		a = a.substr(6);
	for (char &c : a)
		c = (char)tolower((unsigned char)c);
	if (a == "into")	return SourceStepKind::Into;
	if (a == "over")	return SourceStepKind::Over;
	if (a == "out")		return SourceStepKind::Out;
	return SourceStepKind::None;
}

// "completed", "StepCompleted", "frame-exited", "FrameExited" ... all match
static std::string NormalizeReason(const std::string &reason)
{
	std::string r;
	for (char c : reason)
	{
		if (c != '-' && c != '_')
			r += (char)tolower((unsigned char)c);
	}
	if (r.compare(0, 4, "step") == 0)
		r = r.substr(4);
	return r;
}

static const CDebugFunction *FindFunction(const CDebugDatabase *database, u32 regionId, u16 pc)
{
	for (const CDebugFunction &function : database->Functions())
	{
		if (function.regionId == regionId && pc >= function.start && pc < function.end)
			return &function;
	}
	return NULL;
}

static bool WaitForPause(CDebugInterfaceVice *di, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs; waited += 10)
	{
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && !di->sourceStepper->IsArmed())
			return true;
		SYS_Sleep(10);
	}
	return false;
}

bool CTestSourceStepTrace::RunTrace(CDebugInterfaceVice *di, const std::string &path)
{
	json trace;
	if (!Check(ReadJson(path, trace) && trace.contains("steps") && trace["steps"].is_array(),
			   "Trace " + path + " reads as JSON with a steps array"))
		return false;

	std::string compiler = trace.value("compiler", "?");
	std::string fixture = trace.value("fixture", "?");
	std::string name = compiler + "/" + fixture;
	if (!Check(trace.value("machine", "") == "c64", name + ": machine is c64 (the only one the runner drives yet)"))
		return false;

	std::string dir = ResolveProjectPath(trace.value("artifacts", "").c_str());
	std::vector<u8> prg;
	int entry = trace.contains("entry") ? ParseAddress(trace["entry"]) : -1;
	if (!Check(ReadFile(dir + "/" + trace.value("binary", ""), prg) && prg.size() > 2 && entry >= 0,
			   name + ": the binary reads and the trace names an entry point"))
		return false;

	// fresh scratch symbols per trace; the live ones stay untouched
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	std::string debugInfo = dir + "/" + trace.value("debugInfo", "");
	DebugInfoLoadFile(scratch, debugInfo.c_str());
	std::shared_ptr<const CDebugDatabase> database = scratch->debugDatabase;
	if (!Check(database && scratch->currentSegment && scratch->currentSegment->debugRegionId >= 0,
			   name + ": " + trace.value("debugInfo", "") + " loads with a region for the current segment"))
		return false;
	u32 regionId = (u32)scratch->currentSegment->debugRegionId;

	u16 loadAddress = (u16)(prg[0] | (prg[1] << 8));
	for (size_t i = 2; i < prg.size(); i++)
		di->SetByteToRamC64((u16)(loadAddress + i - 2), prg[i]);

	// $C060 preamble: SEI, CIA1 interrupts off; with "irq" a timer A IRQ every
	// timerCycles into the $C040 handler (INC $02, optional JSR to a fixture
	// function, ack, RTI) and CLI; then JMP entry.
	std::vector<u8> setup = { 0x78, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0xAD, 0x0D, 0xDC };
	if (trace.contains("irq"))
	{
		const json &irq = trace["irq"];
		int cycles = irq.value("timerCycles", 150);
		std::vector<u8> handler = { 0xE6, 0x02 };
		if (irq.contains("call"))
		{
			std::string callee = irq["call"].get<std::string>();
			const CDebugFunction *function = NULL;
			for (const CDebugFunction &f : database->Functions())
			{
				if (f.name == callee)
					function = &f;
			}
			if (!Check(function != NULL, name + ": the IRQ handler's callee " + callee + " is a function of the debug info"))
				return false;
			handler.insert(handler.end(), { 0x20, (u8)(function->start & 0xFF), (u8)(function->start >> 8) });
		}
		handler.insert(handler.end(), { 0xAD, 0x0D, 0xDC, 0xA9, 0xFF, 0x8D, 0x19, 0xD0,
										0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40 });
		for (size_t i = 0; i < handler.size(); i++)
			di->SetByteToRamC64((u16)(0xC040 + i), handler[i]);
		di->SetByteToRamC64(0x0314, 0x40);
		di->SetByteToRamC64(0x0315, 0xC0);
		setup.insert(setup.end(), { 0xA9, (u8)(cycles & 0xFF), 0x8D, 0x04, 0xDC, 0xA9, (u8)(cycles >> 8), 0x8D, 0x05, 0xDC,
									0xA9, 0x81, 0x8D, 0x0D, 0xDC, 0xA9, 0x11, 0x8D, 0x0E, 0xDC, 0x58 });
	}
	setup.insert(setup.end(), { 0x4C, (u8)(entry & 0xFF), (u8)(entry >> 8) });
	for (size_t i = 0; i < setup.size(); i++)
		di->SetByteToRamC64((u16)(0xC060 + i), setup[i]);
	di->SetByteToRamC64(0x0002, 0);

	// run to the start line through a source breakpoint
	std::string startFile = trace["start"].value("file", "");
	int startLine = trace["start"].value("line", 0);
	CDebugSourceBreakpoint *startBp = scratch->AddSourceBreakpoint(startFile.c_str(), startLine);
	if (!Check(startBp->IsResolved() && startBp->resolvedLine == startLine,
			   name + ": the start " + startFile + ":" + std::to_string(startLine) + " resolves to code"))
		return false;
	int startAddress = startBp->bindings[0].address;
	di->MakeJmpC64(0xC060);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	bool paused = WaitForPause(di, 3000);
	scratch->RemoveSourceBreakpoint(startBp);
	u16 pc;
	u8 a, x, y, p, sp;
	di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
	char hex[8];
	snprintf(hex, sizeof(hex), "$%04X", startAddress);
	if (!Check(paused && pc == startAddress, name + ": the program stops at the start line (" + hex + ")"))
		return false;

	// replay
	std::ostringstream log;
	log << "trace " << path << "\nstart " << startFile << ":" << startLine << " at " << hex << "\n";
	int depth = 0;
	bool ok = true;
	std::string mismatch;
	int index = 0;
	for (const json &step : trace["steps"])
	{
		index++;
		SourceStepKind kind = ParseAction(step.value("action", ""));
		u64 budget = step.value("budget", (u64)0);
		bool armed = di->SourceStep(kind, budget);
		bool stopped = !armed || WaitForPause(di, 5000);
		CSourceStepResult r = di->sourceStepper->GetLastResult();
		if (armed)
			depth += r.depth;

		const CDebugSourceFile *file = r.mapped ? database->GetFile(r.fileId) : NULL;
		const CDebugFunction *function = FindFunction(database.get(), regionId, r.pc);
		std::string actualFile = file ? file->fileName : "";
		std::string actualFunction = function ? function->name : "";
		std::string actualReason = SourceStepStopReasonName(r.reason);

		const json &expected = step["expected"];
		bool match = stopped && kind != SourceStepKind::None
			&& NormalizeReason(step.value("expectedReason", "")) == NormalizeReason(actualReason);
		if (expected.contains("line"))
		{
			if (expected["line"].is_null())
				match = match && !r.mapped;
			else
				match = match && r.mapped && r.line == expected["line"].get<int>();
		}
		if (expected.contains("file"))
		{
			std::string expectedFile = expected["file"].get<std::string>();
			size_t slash = expectedFile.find_last_of("/\\");
			match = match && actualFile == (slash == std::string::npos ? expectedFile : expectedFile.substr(slash + 1));
		}
		if (expected.contains("function"))
			match = match && actualFunction == expected["function"].get<std::string>();
		if (expected.contains("depth"))
			match = match && depth == expected["depth"].get<int>();

		char line[320];
		snprintf(line, sizeof(line), "%2d %-10s -> $%04X %s:%d %s depth %d %s (%llu instructions)%s\n   expected %s %s\n",
				 index, step.value("action", "").c_str(), r.pc, actualFile.c_str(), r.mapped ? r.line : 0,
				 actualFunction.c_str(), depth, actualReason.c_str(), (unsigned long long)r.instructions,
				 match ? "" : "  MISMATCH", expected.dump().c_str(), step.value("expectedReason", "").c_str());
		log << line;
		if (!match && ok)
		{
			ok = false;
			mismatch = line;
		}
		if (!stopped)
			break;
	}

	std::string message = name + ": " + std::to_string(trace["steps"].size()) + " source steps match the trace";
	if (!ok)
	{
		std::string dump = "/tmp/source-step-trace-" + compiler + "-" + fixture + ".txt";
		std::ofstream(dump) << log.str();
		message = name + ": " + mismatch + " (full trace in " + dump + ")";
	}
	return Check(ok, message);
}

void CTestSourceStepTrace::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	std::string expectations = ResolveProjectPath("tests/source-debug/expectations");
	json index;
	bool ok = Check(ReadJson(expectations + "/index.json", index) && index.contains("traces") && !index["traces"].empty(),
					"tests/source-debug/expectations/index.json lists the traces");
	int traces = 0;

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
		CDebugSymbols *liveSymbols = di->symbols;
		u8 savedVector[2] = { di->GetByteFromRamC64(0x0314), di->GetByteFromRamC64(0x0315) };

		for (const json &entry : index["traces"])
		{
			ok = RunTrace(di, expectations + "/" + entry.get<std::string>());
			di->SetDebugMode(DEBUGGER_MODE_PAUSED);
			di->PauseEmulationBlockedWait();
			if (!ok)
				break;
			traces++;
		}

		// KERNAL vector, PAL timer A latch ($4025) and the timer A IRQ back
		di->SetByteToRamC64(0x0314, savedVector[0]);
		di->SetByteToRamC64(0x0315, savedVector[1]);
		di->SetByteC64(0xDC04, 0x25);
		di->SetByteC64(0xDC05, 0x40);
		di->SetByteC64(0xDC0D, 0x81);

		guiMain->LockMutex();
		di->LockMutex();
		di->symbols = liveSymbols;
		di->UnlockMutex();
		guiMain->UnlockMutex();
		// scratch symbols leaked on purpose, see CTestSymbolLoaders

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
	char summary[128];
	snprintf(summary, sizeof(summary), "%d source step traces replay as recorded", traces);
	TestCompleted(true, summary);
}
