#include "CTestSourceVerify.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
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

bool CTestSourceVerify::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

void CTestSourceVerify::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulation is not compiled in, so the code check was never exercised");
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	CDebuggerApi *api = di ? di->GetDebuggerApi() : NULL;
	if (api == NULL)
	{
		TestCompleted(false, "the C64 has no debugger API");
		return;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();

	std::string dir = ResolveProjectPath("tests/data/source-debug/cc65/c64/basic-flow");
	std::string programPath = dir + "/main.prg";
	std::ifstream in(programPath, std::ios::binary);
	std::vector<u8> program((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	// the PRG into RAM at its load address; the RAM it covers restored after
	bool ok = Check(program.size() > 2, "the cc65 basic-flow PRG is read");
	u16 load = ok ? (u16)(program[0] | (program[1] << 8)) : 0;
	std::vector<u8> savedRam;
	for (size_t i = 2; ok && i < program.size(); i++)
	{
		savedRam.push_back(di->GetByteFromRamC64((u16)(load + i - 2)));
		di->SetByteToRamC64((u16)(load + i - 2), program[i]);
	}

	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	if (ok)
	{
		DebugInfoFormat format = DebugInfoLoadFile(scratch, (dir + "/main.dbg").c_str());
		scratch->programPath = programPath;
		ok = Check(format == DebugInfoFormat::Cc65Dbg && scratch->debugDatabase && scratch->debugDatabase->HasImageOffsets(),
				   "the cc65 debug info loads with file offsets");
	}
	if (ok)
	{
		nlohmann::json j = api->VerifySourceCodeJson("");
		ok = Check(j.value("matches", false) && j.value("checked", 0) > 50 && j.value("differing", -1) == 0 && !j.contains("warning"),
				   "the PRG in memory matches the program the debug info was loaded for (" + j.dump() + ")");
	}
	u16 changed = 0x0850;
	if (ok)
	{
		u8 saved = di->GetByteFromRamC64(changed);
		di->SetByteToRamC64(changed, (u8)(saved ^ 0xFF));
		nlohmann::json j = api->VerifySourceCodeJson("");
		di->SetByteToRamC64(changed, saved);
		ok = Check(!j.value("matches", true) && j.value("differing", 0) == 1 && j["firstDiffering"].size() == 1
				   && j["firstDiffering"][0] == changed && j.contains("warning"),
				   "one changed code byte is reported at $0850, with a warning (" + j.value("warning", std::string()) + ")");
	}
	if (ok)
	{
		nlohmann::json j = api->VerifySourceCodeJson(ResolveProjectPath("tests/data/source-debug/cc65/c64/nested-calls/main.prg"));
		ok = Check(!j.value("matches", true) && j.value("differing", 0) > 0,
				   "another build's PRG differs from the code in memory");
	}
	if (ok)
	{
		nlohmann::json j = api->VerifySourceCodeJson("/nonexistent/main.prg");
		ok = Check(!j.value("matches", true) && j.contains("warning"), "an unreadable program file is a warning, not a match");
	}

	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = liveSymbols;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	// scratch leaked on purpose, see CTestSymbolLoaders
	for (size_t i = 0; i < savedRam.size(); i++)
		di->SetByteToRamC64((u16)(load + i), savedRam[i]);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (!ok)
		TestCompleted(false, failure.c_str());
	else
		TestCompleted(true, "the code in memory is checked against the program file the debug info describes");
#endif
}
