#include "CTestCc65StackDepth.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugInfoLoader.h"
#include "CDebugDatabase.h"
#include "CDebugStackDepth.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#ifdef RUN_COMMODORE64
#include "CDebugInterfaceVice.h"
#endif
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <vector>

bool CTestCc65StackDepth::Check(bool condition, const std::string &message)
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
static double MicrosPerCall(const CDebugDatabase *db, CDebugInterface *di, const CDebugFunction &f, u32 region, u16 pc,
							DebugStackDecoding mode, int calls, int *instructions)
{
	auto t0 = std::chrono::steady_clock::now();
	CDebugStackDepth d;
	for (int i = 0; i < calls; i++)
		d = DebugCc65StackDepth(db, di, f, region, pc, mode);
	auto t1 = std::chrono::steady_clock::now();
	if (instructions)
		*instructions = d.instructions;
	return std::chrono::duration<double, std::micro>(t1 - t0).count() / calls;
}

// a hard reset to READY: the previous program may have changed zero page
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
#endif

void CTestCc65StackDepth::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulation is not compiled in, so cc65 stack decoding was never measured");
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (di == NULL)
	{
		TestCompleted(false, "the C64 is not created");
		return;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}

	const char *fixtures[] = { "basic-flow", "nested-calls", "recursion", "branches-loops", "indirect-calls", "same-line" };
	const DebugStackDecoding modes[] = { DebugStackDecoding::Limited, DebugStackDecoding::Full };
	int exact[3] = { 0 }, approximate[3] = { 0 }, wrong[3] = { 0 };
	double worstMicros[3] = { 0 }, totalMicros[3] = { 0 };
	int timed[3] = { 0 };
	std::string firstWrong;
	bool ok = true;

	for (const char *fixture : fixtures)
	{
		if (!ok)
			break;
		std::string dir = ResolveProjectPath((std::string("tests/data/source-debug/cc65/c64/") + fixture).c_str());
		ok = Check(BootToReady(di), std::string(fixture) + ": the C64 is at READY");
		std::ifstream in(dir + "/main.prg", std::ios::binary);
		std::vector<u8> program((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		if (!ok || program.size() < 3)
		{
			ok = Check(false, std::string(fixture) + ": main.prg is read");
			break;
		}
		u16 load = (u16)(program[0] | (program[1] << 8));
		for (size_t i = 2; i < program.size(); i++)
			di->SetByteToRamC64((u16)(load + i - 2), program[i]);
		di->symbols->currentSegment->breakpointsPC->ClearBreakpoints();
		DebugInfoFormat format = DebugInfoLoadFile(di->symbols, (dir + "/main.dbg").c_str());
		std::shared_ptr<const CDebugDatabase> db = di->symbols->debugDatabase;
		u32 region = di->symbols->currentSegment ? (u32)di->symbols->currentSegment->debugRegionId : 0;
		ok = Check(format == DebugInfoFormat::Cc65Dbg && db && !db->runtimeSymbols.empty(),
				   std::string(fixture) + ": the cc65 debug info loads, with the runtime's symbols from main.lbl");
		if (!ok)
			break;

		// the functions with autos, their prologue ends and C line starts
		struct Stop { const CDebugFunction *function; bool prologueEnd; };
		std::map<u16, Stop> stops;
		std::map<const CDebugFunction *, std::vector<const CDebugVariable *>> autos;
		for (const CDebugVariable &v : db->Variables())
			if (v.cc65Stack && v.functionIndex >= 0)
				autos[&db->Functions()[v.functionIndex]].push_back(&v);
		for (auto &it : autos)
		{
			const CDebugFunction *f = it.first;
			u32 body = f->end;
			for (const CDebugSourceMapping &m : db->Mappings())
				if (m.regionId == region && m.fileId == f->fileId && m.start >= f->start && m.start < f->end && m.lineStart != f->line && m.start < body)
					body = m.start;
			if (body < f->end)
				stops[(u16)body] = Stop{ f, true };
			for (const CDebugSourceMapping &m : db->Mappings())
				if (m.regionId == region && m.fileId == f->fileId && m.start > body && m.start < f->end && !stops.count((u16)m.start))
					stops[(u16)m.start] = Stop{ f, false };
		}
		di->LockMutex();
		for (auto &s : stops)
			di->symbols->currentSegment->AddBreakpointPC(s.first);
		di->UnlockMutex();

		// from a stub at $C060: SEI, CIA1 interrupts off, JMP $080D
		di->SetByteC64(0x0000, 0x2F);
		di->SetByteC64(0x0001, 0x37);
		const u8 setup[] = { 0x78, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0xAD, 0x0D, 0xDC, 0x4C, 0x0D, 0x08 };
		for (size_t i = 0; i < sizeof(setup); i++)
			di->SetByteToRamC64((u16)(0xC060 + i), setup[i]);
		di->MakeJmpC64(0xC060);

		u16 spZp = 0;
		for (auto &it : autos)
			spZp = (u16)it.second[0]->baseZp;
		std::map<u8, u16> frameSp;		// 6502 S at the activation -> its C sp at the prologue end
		int numStops = 0;
		for (; numStops < 400; numStops++)
		{
			di->SetDebugMode(DEBUGGER_MODE_RUNNING);
			bool paused = false;
			for (int waited = 0; waited < 1000 && !paused; waited += 2)
			{
				SYS_Sleep(2);
				paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED && stops.count((u16)di->GetCpuPC());
			}
			if (!paused)
				break;		// the program reached its final loop
			u16 pc;
			u8 a, x, y, p, s;
			di->GetCpuRegs(&pc, &a, &x, &y, &p, &s);
			u16 sp = (u16)(di->GetByteFromRamC64(spZp) | (di->GetByteFromRamC64((u16)(spZp + 1)) << 8));
			const Stop &stop = stops[pc];
			if (stop.prologueEnd)
				frameSp[s] = sp;
			auto frame = frameSp.find(s);
			if (frame == frameSp.end())
				continue;
			for (const CDebugVariable *v : autos[stop.function])
			{
				u16 truth = (u16)(frame->second + v->address);
				for (DebugStackDecoding mode : modes)
				{
					int m = (int)mode;
					auto t0 = std::chrono::steady_clock::now();
					CDebugStackDepth d = DebugCc65StackDepth(db.get(), di, *stop.function, region, pc, mode);
					double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
					totalMicros[m] += us;
					timed[m]++;
					if (us > worstMicros[m])
						worstMicros[m] = us;
					if (!d.known || !d.exact)
					{
						approximate[m]++;
						continue;
					}
					u16 decoded = (u16)(sp + v->address + (u32)(d.depth - d.refDepth));
					if (decoded == truth)
						exact[m]++;
					else
					{
						wrong[m]++;
						if (firstWrong.empty())
						{
							char buf[200];
							snprintf(buf, sizeof(buf), "%s %s() %s at $%04X: decoded $%04X, truth $%04X (%s)", fixture, stop.function->name.c_str(),
									 v->name.c_str(), pc, decoded, truth, DebugStackDecodingName(mode));
							firstWrong = buf;
						}
					}
				}
			}
		}
		di->LockMutex();
		di->symbols->currentSegment->breakpointsPC->ClearBreakpoints();
		di->UnlockMutex();
		char msg[200];
		if (stops.empty())
			snprintf(msg, sizeof(msg), "%s: no function has a stack local", fixture);
		else
			snprintf(msg, sizeof(msg), "%s: %d stops at prologue ends and line starts of the functions with locals", fixture, numStops);
		ok = Check(numStops > 0 || stops.empty(), msg);
	}

	if (ok)
	{
		char msg[400];
		snprintf(msg, sizeof(msg), "accuracy against the running programs: limited %d exact, %d approximate, %d wrong; full %d exact, %d approximate, %d wrong%s%s",
				 exact[1], approximate[1], wrong[1], exact[2], approximate[2], wrong[2], firstWrong.empty() ? "" : "; first wrong: ", firstWrong.c_str());
		ok = Check(wrong[1] == 0 && wrong[2] == 0 && exact[2] > 0 && exact[2] >= exact[1], msg);
	}
	if (ok)
	{
		char msg[300];
		snprintf(msg, sizeof(msg), "benchmark, the fixtures' functions at their stops: limited %.1f us average (worst %.1f), full %.1f us average (worst %.1f), over %d resolutions each",
				 timed[1] ? totalMicros[1] / timed[1] : 0.0, worstMicros[1], timed[2] ? totalMicros[2] / timed[2] : 0.0, worstMicros[2], timed[2]);
		LOGM("Cc65StackDepth: %s", msg);
		ok = Check(timed[2] > 0, msg);
	}

	// the synthetic function: N blocks of "LDY #0 / LDA (sp),Y / BEQ +6 /
	// JSR pusha / JSR incsp1 / JSR pushax / JSR tosaddax" then RTS, at $4000,
	// helpers named at $6000/$6003/$6006/$6009 (code: RTS), past its 7 KB
	if (ok)
	{
		di->PauseEmulationBlockedWait();
		CDebugDatabase db("bench");
		u32 region = db.AddRegion("Default", "");
		u32 block = db.AddBlock("", region);
		db.AddFile(0, "bench.c");
		const u16 base = 0x4000;
		const int blocks = 400;
		u16 at = base;
		std::vector<u8> code;
		for (int i = 0; i < blocks; i++)
		{
			const u8 b[] = { 0xA0, 0x00, 0xB1, 0x02, 0xF0, 0x06, 0x20, 0x00, 0x60, 0x20, 0x03, 0x60,
							 0x20, 0x06, 0x60, 0x20, 0x09, 0x60 };
			code.insert(code.end(), b, b + sizeof(b));
		}
		code.push_back(0x60);
		for (size_t i = 0; i < code.size(); i++)
			di->SetByteToRamC64((u16)(at + i), code[i]);
		for (u16 h = 0x6000; h < 0x600C; h += 3)
			di->SetByteToRamC64(h, 0x60);
		db.runtimeSymbols[0x6000] = "pusha";
		db.runtimeSymbols[0x6003] = "incsp1";
		db.runtimeSymbols[0x6006] = "pushax";
		db.runtimeSymbols[0x6009] = "tosaddax";
		CDebugFunction f;
		f.name = "bench";
		f.start = base;
		f.end = (u32)(base + code.size());
		f.regionId = region;
		f.fileId = 0;
		f.line = 1;
		db.AddFunction(f);
		CDebugSourceMapping m;
		m.start = base;
		m.end = (u32)(base + code.size());
		m.regionId = region;
		m.blockId = block;
		m.lineStart = m.lineEnd = 2;
		db.AddMapping(m);
		db.Finalize();
		const CDebugFunction &fn = db.Functions()[0];
		u16 lastBlock = (u16)(base + (blocks - 1) * 18);
		int instrLimited = 0, instrFull = 0;
		double limited = MicrosPerCall(&db, di, fn, region, lastBlock, DebugStackDecoding::Limited, 50, &instrLimited);
		double full = MicrosPerCall(&db, di, fn, region, lastBlock, DebugStackDecoding::Full, 50, &instrFull);
		CDebugStackDepth d = DebugCc65StackDepth(&db, di, fn, region, lastBlock, DebugStackDecoding::Full);
		CDebugStackDepth dl = DebugCc65StackDepth(&db, di, fn, region, lastBlock, DebugStackDecoding::Limited);
		char msg[400];
		snprintf(msg, sizeof(msg), "benchmark, a synthetic %d-byte function (%d branches, %d helper calls), PC in its last block: limited %.0f us (%d instructions, %s), full %.0f us (%d instructions, %s%s%s); budget 20000 us",
				 (int)code.size(), blocks, blocks * 4, limited, instrLimited, dl.exact ? "exact" : "approximate", full, instrFull, d.exact ? "exact" : "approximate",
				 d.reason.empty() ? "" : ": ", d.reason.c_str());
		LOGM("Cc65StackDepth: %s", msg);
		// each block nets zero: pusha +1, incsp1 -1, pushax +2, tosaddax -2, on both paths of its branch
		ok = Check(d.known && d.exact && d.depth == 0 && full < 20000.0 && limited < 20000.0, msg);
	}

	// the cc65 programs switch BASIC out ($01): back to a reset machine at READY
	BootToReady(di);
	di->symbols->DeleteAllSymbols();
	di->symbols->CreateDefaultSegment();
	di->symbols->ActivateSelectedSegment();
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);
	if (ok)
		TestCompleted(true, "cc65 stack depth decoding: every exact answer matches the running program; timed per mode");
	else
		TestCompleted(false, failure.c_str());
#endif
}
