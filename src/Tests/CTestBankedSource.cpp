#include "CTestBankedSource.h"
#include "CTestNesCartridge.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceNes.h"
#include "CDebugInterfaceVice.h"
#include "CDebugSymbolsC64.h"
#include "C64SettingsStorage.h"
#include "CSlrString.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointAddr.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebugSourceBreakpoint.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstdio>

bool CTestBankedSource::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

bool CTestBankedSource::RunDatabase()
{
	// bank 0 code at $8000-$8010 from image offset $0010, bank 1 at
	// $8000-$8008 from $4010, fixed code at $C000 from $C010
	CDebugDatabase database("test");
	u32 region = database.AddRegion("Default", "");
	u32 block = database.AddBlock("", region);
	database.AddFile(0, "banked.c");
	auto add = [&](u32 start, u32 end, i32 line, u64 image) {
		CDebugSourceMapping m;
		m.start = start;
		m.end = end;
		m.regionId = region;
		m.blockId = block;
		m.lineStart = m.lineEnd = line;
		m.hasImageOffset = true;
		m.imageOffset = image;
		database.AddMapping(m);
	};
	add(0x8000, 0x8010, 10, 0x0010);
	add(0x8000, 0x8008, 20, 0x4010);
	add(0xC000, 0xC004, 30, 0xC010);
	database.Finalize();

	const CDebugSourceMapping *bank0 = database.ResolveImage(region, 0x8004, true, 0x0014);
	const CDebugSourceMapping *bank1 = database.ResolveImage(region, 0x8004, true, 0x4014);
	const CDebugSourceMapping *bank0Only = database.ResolveImage(region, 0x800C, true, 0x001C);
	const CDebugSourceMapping *unknown = database.ResolveImage(region, 0x8004, false, 0);
	bool ok = database.HasImageOffsets()
		&& database.IsBankedAddress(region, 0x8000) && database.IsBankedAddress(region, 0x8007)
		&& !database.IsBankedAddress(region, 0x8008) && !database.IsBankedAddress(region, 0xC000)
		&& bank0 && bank0->lineStart == 10 && bank1 && bank1->lineStart == 20
		&& bank0Only && bank0Only->lineStart == 10
		&& unknown && unknown->lineStart == 20;	// no bank known: the address's mapping in effect
	return Check(ok, "Banked database: $8000-$8007 is banked, $8008 and $C000 are not; the image offset picks bank 0 or 1, an unknown bank falls back to the mapping in effect");
}

bool CTestBankedSource::RunNes(const NesCartridge &cart)
{
#ifdef RUN_NES
	CDebugInterface *di = viewC64->debugInterfaceNes;
	if (!di)
	{
		ReportRequiredGap("NES not created");
		return true;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(1000);
	}

	std::string dir = ResolveProjectPath((std::string("tests/data/source-debug/cc65/") + cart.machine + "/banked").c_str());
	std::string label = std::string("Banked NES ") + cart.mapper + ": ";
	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbols(di, true);
	scratch->SetDataAdapter(liveSymbols->dataAdapter);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	CDebugInfoLoadOptions options;
	options.sourceRoots.push_back(ResolveProjectPath("tests/source-debug/fixtures/banked"));
	DebugInfoFormat format = DebugInfoLoadFile(scratch, (dir + "/main.dbg").c_str(), options);
	bool ok = Check(format == DebugInfoFormat::Cc65Dbg && scratch->debugDatabase && scratch->debugDatabase->HasImageOffsets()
					&& scratch->currentSegment && scratch->currentSegment->debugRegionId >= 0,
					label + "the cc65 debug info loads with image offsets");

	// bankB's body and bankA's body, both at $8000 + n
	CDebugSourceBreakpoint *onB = ok ? scratch->AddSourceBreakpoint(cart.source, cart.lineB) : NULL;
	CDebugBreakpointAddr *addressB = (onB && onB->IsResolved())
		? scratch->currentSegment->breakpointsPC->GetBreakpoint(onB->bindings[0].address) : NULL;
	ok = ok && Check(onB && onB->IsResolved() && onB->bindings.size() == 1 && onB->bindings[0].banked
					 && addressB && addressB->imageOffsets.size() == 1 && addressB->imageOffsets[0] >= 0x4010,
					 label + "the source breakpoint on bankB's line binds to one address with bank 1's image offset");

	auto runToPause = [&](int timeoutMs) -> bool {
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		for (int waited = 0; waited < timeoutMs; waited += 10)
		{
			SYS_Sleep(10);
			if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
				return true;
		}
		return false;
	};
	// _result is the one BSS byte (main.map)
	auto readResult = [&]() { u8 v = 0; bool available = false; di->GetDataAdapter()->AdapterReadByte(cart.resultAddress, &v, &available); return v; };

	if (ok)
	{
		// the cartridge loads paused at reset; run to the first stop
		ok = Check(TestNesInsertCartridge(di, (dir + "/main.nes").c_str(), true), label + "main.nes (64 KB PRG) inserts");
	}
	if (ok)
	{
		bool paused = runToPause(3000);
		int pc = di->GetCpuPC();
		u8 value = readResult();
		char msg[200];
		snprintf(msg, sizeof(msg), "%sthe bankB breakpoint stops once bank 1 is in, after bankA's first call (PC $%04X, result %d), not at bankA's code at the same address", label.c_str(), pc, value);
		ok = Check(paused && pc == onB->bindings[0].address && value == 1, msg);
	}

	// bankA's line: the two calls of bankA stop, bankB's call in between does not
	CDebugSourceBreakpoint *onA = NULL;
	if (ok)
	{
		scratch->RemoveSourceBreakpoint(onB);
		onA = scratch->AddSourceBreakpoint(cart.source, cart.lineA);
		ok = Check(onA && onA->IsResolved() && onA->bindings.size() == 1 && onA->bindings[0].banked,
				   label + "the source breakpoint on bankA's line binds with bank 0's image offset");
	}
	if (ok)
	{
		// from the bankB stop: bankB returns (11), bankA's second call stops
		bool paused = runToPause(3000);
		int pc = di->GetCpuPC();
		u8 value = readResult();
		char msg[200];
		snprintf(msg, sizeof(msg), "%sthe bankA breakpoint next stops at bankA's second call (PC $%04X, result %d), skipping the rest of bankB", label.c_str(), pc, value);
		ok = Check(paused && pc == onA->bindings[0].address && value == 11, msg);
	}
	if (ok)
	{
		// no third bankA call: the program loops in main
		bool paused = runToPause(500);
		ok = Check(!paused, label + "no further stop -- the loop in main never reaches bank 0's line again");
		di->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}

	if (onA)
		scratch->RemoveSourceBreakpoint(onA);
	TestNesUnloadCartridge(di, false);
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
	ReportRequiredGap("NES emulation not compiled in");
	return true;
#endif
}

// The C64: the Magic Desk build (main.crt is ld65's output, so the debug
// info's image offsets are .crt file offsets) attached from the file; the
// machine resets into it. bankA (bank 0) and bankB (bank 1) are at $9000.
bool CTestBankedSource::RunC64()
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
	const std::string label = "Banked C64 Magic Desk: ";
	std::string dir = ResolveProjectPath("tests/data/source-debug/cc65/c64-magicdesk/banked");

	// attaching stores the cartridge path in the settings: keep the user's
	guiMain->LockMutex();
	CSlrString *savedPathToCartridge = c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = NULL;
	guiMain->UnlockMutex();

	di->PauseEmulationBlockedWait();
	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	CDebugInfoLoadOptions options;
	options.sourceRoots.push_back(ResolveProjectPath("tests/source-debug/fixtures/banked"));
	DebugInfoFormat format = DebugInfoLoadFile(scratch, (dir + "/main.dbg").c_str(), options);
	bool ok = Check(format == DebugInfoFormat::Cc65Dbg && scratch->debugDatabase && scratch->debugDatabase->HasImageOffsets()
					&& scratch->currentSegment && scratch->currentSegment->debugRegionId >= 0,
					label + "the cc65 debug info loads with image offsets");

	// bankB's body (line 22) and bankA's body (line 16), both at $9000 + n
	CDebugSourceBreakpoint *onB = ok ? scratch->AddSourceBreakpoint("magicdesk.c", 22) : NULL;
	CDebugBreakpointAddr *addressB = (onB && onB->IsResolved())
		? scratch->currentSegment->breakpointsPC->GetBreakpoint(onB->bindings[0].address) : NULL;
	ok = ok && Check(onB && onB->IsResolved() && onB->bindings.size() == 1 && onB->bindings[0].banked
					 && addressB && addressB->imageOffsets.size() == 1 && addressB->imageOffsets[0] >= 0x2060,
					 label + "the source breakpoint on bankB's line binds to one address with bank 1's .crt offset");

	auto waitForPause = [&](int timeoutMs) -> bool {
		for (int waited = 0; waited < timeoutMs; waited += 10)
		{
			SYS_Sleep(10);
			if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
				return true;
		}
		return false;
	};
	auto runToPause = [&](int timeoutMs) -> bool {
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		return waitForPause(timeoutMs);
	};
	// _result is the one BSS byte, at $08D1 (main.map)
	auto readResult = [&]() { return di->GetByteFromRamC64(0x08D1); };

	bool attached = false;
	if (ok)
	{
		// the attach resets into the cartridge and runs to the first stop
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		CSlrString *path = new CSlrString((dir + "/main.crt").c_str());
		di->AttachCartridge(path);
		delete path;
		attached = true;
		// the attach pauses briefly by itself while it resets into the
		// cartridge: wait for the pause at the breakpoint
		bool paused = false;
		for (int waited = 0; waited < 5000 && !paused; waited += 10)
		{
			SYS_Sleep(10);
			paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED && (u16)di->GetCpuPC() == onB->bindings[0].address;
		}
		u16 pc = (u16)di->GetCpuPC();
		u8 value = readResult();
		char msg[220];
		snprintf(msg, sizeof(msg), "%sthe cartridge attaches; the bankB breakpoint stops once bank 1 is in, after bankA's first call (PC $%04X, result %d), not at bankA's code at the same address", label.c_str(), pc, value);
		ok = Check(paused && pc == onB->bindings[0].address && value == 1, msg);
	}

	CDebugSourceBreakpoint *onA = NULL;
	if (ok)
	{
		scratch->RemoveSourceBreakpoint(onB);
		onB = NULL;
		onA = scratch->AddSourceBreakpoint("magicdesk.c", 16);
		ok = Check(onA && onA->IsResolved() && onA->bindings.size() == 1 && onA->bindings[0].banked,
				   label + "the source breakpoint on bankA's line binds with bank 0's .crt offset");
	}
	if (ok)
	{
		bool paused = runToPause(3000);
		u16 pc = (u16)di->GetCpuPC();
		u8 value = readResult();
		char msg[200];
		snprintf(msg, sizeof(msg), "%sthe bankA breakpoint next stops at bankA's second call (PC $%04X, result %d), skipping the rest of bankB", label.c_str(), pc, value);
		ok = Check(paused && pc == onA->bindings[0].address && value == 11, msg);
	}
	if (ok)
	{
		bool paused = runToPause(500);
		ok = Check(!paused, label + "no further stop -- the loop in main never reaches bank 0's line again");
	}

	if (onA)
		scratch->RemoveSourceBreakpoint(onA);
	if (onB)
		scratch->RemoveSourceBreakpoint(onB);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (attached)
	{
		di->DetachCartridge();		// resets the machine
		SYS_Sleep(200);
	}
	di->PauseEmulationBlockedWait();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = liveSymbols;
	di->UnlockMutex();
	if (c64SettingsPathToCartridge)
		delete c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = savedPathToCartridge;
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

void CTestBankedSource::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	// bankA's and bankB's body lines, and _result, from each build's main.map
	const NesCartridge uxrom = { "UxROM", "nes-uxrom", "uxrom.c", 18, 24, 0x6000 };
	const NesCartridge mmc1 = { "MMC1", "nes-mmc1", "mmc1.c", 19, 25, 0x0300 };
	bool ok = RunDatabase() && RunNes(uxrom) && RunNes(mmc1) && RunC64();
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Banked code resolves and breaks by the bank in the window (NES UxROM and MMC1, C64 Magic Desk; cc65)");
}
