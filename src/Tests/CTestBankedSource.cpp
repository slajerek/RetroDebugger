#include "CTestBankedSource.h"
#include "CTestNesCartridge.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceNes.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceTask.h"
#include "CDebugSymbolsC64.h"
#include "C64SettingsStorage.h"
#include "CSlrString.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointAddr.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebuggerApi.h"
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

// RD-025: the label shown at ADDRESS is the one of the bank in (both banks'
// functions start there)
static bool LabelIs(CDebugSymbols *symbols, int address, const char *name, std::string &got)
{
	CDebugSymbolsCodeLabel *codeLabel = symbols->currentSegment ? symbols->currentSegment->FindLabel(address) : NULL;
	got = codeLabel ? codeLabel->GetLabelText() : "(none)";
	return codeLabel && got == name;
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
	if (!Check(ok, "Banked database: $8000-$8007 is banked, $8008 and $C000 are not; the image offset picks bank 0 or 1, an unknown bank falls back to the mapping in effect"))
		return false;

	// both banks' lines start at $8000: each line resolves to its own bank's
	// range (the binder once kept only the range a by-address lookup picked)
	std::vector<const CDebugSourceMapping *> line10, line20;
	i32 resolved10 = 0, resolved20 = 0;
	DebugResolveSourceLine(&database, "banked.c", 10, &resolved10, line10);
	DebugResolveSourceLine(&database, "banked.c", 20, &resolved20, line20);
	ok = resolved10 == 10 && line10.size() == 1 && line10[0]->imageOffset == 0x0010
		&& resolved20 == 20 && line20.size() == 1 && line20[0]->imageOffset == 0x4010;
	return Check(ok, "Banked database: lines 10 and 20, both starting at $8000 in banks 0 and 1, each resolve to their own bank's range");
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
					 && addressB && addressB->imageOffsets.size() == 1 && addressB->imageOffsets[0] >= cart.bank1ImageOffset,
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

	CDebugSymbolsDataWatch *prgWatch = NULL;		// RD-059
	u8 prgBytes[16] = { 0 };		// bankA and bankB begin alike: 16 bytes tell them apart
	if (ok)
	{
		// the cartridge loads paused at reset; run to the first stop
		ok = Check(TestNesInsertCartridge(di, (dir + "/main.nes").c_str(), true), label + "main.nes inserts");
	}
	if (ok)
	{
		bool paused = runToPause(3000);
		int pc = di->GetCpuPC();
		u8 value = readResult();
		char msg[200];
		snprintf(msg, sizeof(msg), "%sthe bankB breakpoint stops once bank 1 is in, after bankA's first call (PC $%04X, result %d), not at bankA's code at the same address", label.c_str(), pc, value);
		ok = Check(paused && pc == onB->bindings[0].address && value == 1, msg);
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x8000, "_bankB", got), label + "with bank 1 in, the label at $8000 is _bankB (got " + got + ")");
		// RD-059: a watch on bankB's code, added with bank 1 in, keeps bank 1's bytes
		if (ok)
		{
			u16 at = onB->bindings[0].address;
			guiMain->LockMutex();
			scratch->currentSegment->AddWatch(at, (char *)"rd059-prg", WATCH_REPRESENTATION_HEX_8, 16);
			prgWatch = scratch->currentSegment->FindWatch(at);
			bool bound = prgWatch && prgWatch->BindToMemory();
			guiMain->UnlockMutex();
			for (int i = 0; bound && i < 16; i++)
				prgBytes[i] = prgWatch->ReadByte(i, NULL);
			ok = Check(bound && prgWatch->physical.device == "nes-prg" && prgWatch->physical.kind == DebugMemoryKind::CartridgeRom,
					   label + "RD-059: a watch on bankB's code binds to " + (bound ? prgWatch->MemoryName() : std::string("nothing")));
		}
		// RD-007: the code check counts the bank in, not the other one at $8000
		nlohmann::json verify = di->GetDebuggerApi()->VerifySourceCodeJson(dir + "/main.nes");
		ok = ok && Check(verify.value("matches", false) && verify.value("checked", 0) > 0,
						 label + "the code in memory matches main.nes, bank 1's code compared while bank 1 is in (" + verify.dump() + ")");
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
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x8000, "_bankA", got), label + "with bank 0 in, the label at $8000 is _bankA (got " + got + ")");
		if (ok && prgWatch)
		{
			CDebugMemoryMappingResolver *resolver = di->GetMemoryMappingResolver();
			bool same = true, elsewhere = true, differs = false;
			for (int i = 0; i < 16; i++)
			{
				bool e = false;
				u8 v = prgWatch->ReadByte(i, &e);
				same = same && v == prgBytes[i];
				elsewhere = elsewhere && e;
				differs = differs || v != di->GetDataAdapter()->AdapterReadByteModulus(prgWatch->address + i);
			}
			char detail[160];
			snprintf(detail, sizeof(detail), " (same %d, not mapped in %d, differs %d; at $%04X %s $%05X now)", same, elsewhere, differs, prgWatch->address,
					 resolver ? resolver->ResolveRead(prgWatch->address).physical.device.c_str() : "?",
					 resolver ? resolver->ResolveRead(prgWatch->address).physical.offset : 0);
			ok = Check(same && elsewhere && differs,
					   label + "RD-059: with bank 0 in, the watch still shows bank 1's bytes (read from that bank, not mapped in), which differ from what the CPU sees" + detail);
		}
	}
	if (ok)
	{
		// no third bankA call: the program loops in main
		bool paused = runToPause(500);
		ok = Check(!paused, label + "no further stop -- the loop in main never reaches bank 0's line again");
		di->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}

	if (prgWatch)
		scratch->currentSegment->DeleteWatch(prgWatch->address);
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
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x9000, "_bankB", got), label + "with bank 1 in, the label at $9000 is _bankB (got " + got + ")");
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
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x9000, "_bankA", got), label + "with bank 0 in, the label at $9000 is _bankA (got " + got + ")");
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

bool CTestBankedSource::RunC64KickAss()
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
	const std::string label = "Banked KickAssembler Magic Desk: ";
	std::string dir = ResolveProjectPath("tests/data/source-debug/kickass/c64-magicdesk/banked-kickass");
	const char *source = "kickass-magicdesk.asm";
	const int lineA = 80, lineB = 86;		// bankA's and bankB's first instruction, both at $9000
	const u16 resultAddress = 0x0002;

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

	auto segmentNamed = [&](const char *name) -> CDebugSymbolsSegment * {
		CSlrString *str = new CSlrString(name);
		CDebugSymbolsSegment *segment = scratch->FindSegment(str);
		delete str;
		return segment;
	};
	auto lineAt = [&](CDebugSymbolsSegment *segment, u16 address, DebugMappingQuality *quality) -> int {
		const CDebugSourceMapping *m = (segment && scratch->debugDatabase)
			? scratch->debugDatabase->Resolve((u32)segment->debugRegionId, address) : NULL;
		if (quality)
			*quality = m ? m->quality : DebugMappingQuality::Missing;
		return m ? m->lineStart : 0;
	};

	// without the table: the .dbg loaded from a copy that has none next to it
	CDebugInfoLoadOptions options;
	options.sourceRoots.push_back(ResolveProjectPath("tests/source-debug/fixtures/banked-kickass"));
	std::string bare = "/tmp/rd-kickass-banked-no-table.dbg";
	bool copied = false;
	{
		FILE *in = fopen((dir + "/main.dbg").c_str(), "rb");
		FILE *out = in ? fopen(bare.c_str(), "wb") : NULL;
		if (in && out)
		{
			char buf[4096];
			size_t n;
			while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
				fwrite(buf, 1, n, out);
			copied = true;
		}
		if (in) fclose(in);
		if (out) fclose(out);
	}
	DebugInfoFormat format = copied ? DebugInfoLoadFile(scratch, bare.c_str(), options) : DebugInfoFormat::Unknown;
	CDebugSymbolsSegment *bank0 = segmentNamed("Bank0");
	CDebugSymbolsSegment *bank1 = segmentNamed("Bank1");
	DebugMappingQuality q0 = DebugMappingQuality::Missing, q1 = DebugMappingQuality::Missing, qReset = DebugMappingQuality::Missing;
	int l0 = lineAt(bank0, 0x9000, &q0), l1 = lineAt(bank1, 0x9000, &q1);
	lineAt(bank0, 0x8009, &qReset);
	char msg[300];
	snprintf(msg, sizeof(msg), "%swithout a segment table, Bank0 and Bank1 load as two segments and the code both place at $9000 is ambiguous (lines %d/%d, quality %d/%d); bank 0's own code at $8009 stays exact, the [fill] bytes are no code",
			 label.c_str(), l0, l1, (int)q0, (int)q1);
	bool ok = Check(format == DebugInfoFormat::KickAssDbg && bank0 && bank1 && bank0 != bank1
					&& l0 == lineA && l1 == lineB && q0 == DebugMappingQuality::Ambiguous && q1 == DebugMappingQuality::Ambiguous
					&& qReset == DebugMappingQuality::Exact && lineAt(bank1, 0x8009, NULL) == 0, msg);
	remove(bare.c_str());

	// with the table next to the .dbg (main.segments.json)
	format = ok ? DebugInfoLoadFile(scratch, (dir + "/main.dbg").c_str(), options) : DebugInfoFormat::Unknown;
	CDebugSymbolsSegment *banked = segmentNamed("Bank0+Bank1");
	DebugMappingQuality qBanked = DebugMappingQuality::Missing;
	if (banked)
		lineAt(banked, 0x9000, &qBanked);
	ok = ok && Check(format == DebugInfoFormat::KickAssDbg && banked && !segmentNamed("Bank0") && !segmentNamed("Bank1")
					 && scratch->currentSegment == banked && scratch->debugDatabase->HasImageOffsets()
					 && scratch->debugDatabase->IsBankedAddress((u32)banked->debugRegionId, 0x9000)
					 && !scratch->debugDatabase->IsBankedAddress((u32)banked->debugRegionId, 0x8009)
					 && qBanked == DebugMappingQuality::Exact,
					 label + "with main.segments.json, Bank0 and Bank1 load as one segment \"Bank0+Bank1\" whose code at $9000 is banked and exact");

	CDebugSourceBreakpoint *onB = ok ? scratch->AddSourceBreakpoint(source, lineB) : NULL;
	CDebugBreakpointAddr *addressB = (onB && onB->IsResolved())
		? scratch->currentSegment->breakpointsPC->GetBreakpoint(onB->bindings[0].address) : NULL;
	ok = ok && Check(onB && onB->IsResolved() && onB->bindings.size() == 1 && onB->bindings[0].banked
					 && addressB && addressB->imageOffsets.size() == 1 && addressB->imageOffsets[0] == 0x2060 + 0x1000,
					 label + "the source breakpoint on bankB's line binds to $9000 in bank 1 (.crt offset $3060, from the table)");

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
	auto readResult = [&]() { return di->GetByteFromRamC64(resultAddress); };

	bool attached = false;
	if (ok)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		CSlrString *path = new CSlrString((dir + "/main.crt").c_str());
		di->AttachCartridge(path);
		delete path;
		attached = true;
		bool paused = false;
		for (int waited = 0; waited < 5000 && !paused; waited += 10)
		{
			SYS_Sleep(10);
			paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED && (u16)di->GetCpuPC() == 0x9000;
		}
		u16 pc = (u16)di->GetCpuPC();
		u8 value = readResult();
		const CDebugSourceMapping *m = di->ResolveSourceAt(scratch->debugDatabase.get(), (u32)banked->debugRegionId, 0x9000);
		snprintf(msg, sizeof(msg), "%sthe cartridge attaches; the bankB breakpoint stops once bank 1 is in, after bankA's first call (PC $%04X, result %d), and the PC resolves to bankB's line (%d)",
				 label.c_str(), pc, value, m ? m->lineStart : 0);
		ok = Check(paused && pc == 0x9000 && value == 1 && m && m->lineStart == lineB, msg);
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x9000, "bankB", got), label + "with bank 1 in, the label at $9000 is bankB (got " + got + ")");
	}

	CDebugSourceBreakpoint *onA = NULL;
	if (ok)
	{
		scratch->RemoveSourceBreakpoint(onB);
		onB = NULL;
		onA = scratch->AddSourceBreakpoint(source, lineA);
		ok = Check(onA && onA->IsResolved() && onA->bindings.size() == 1 && onA->bindings[0].banked,
				   label + "the source breakpoint on bankA's line binds with bank 0's .crt offset");
	}
	if (ok)
	{
		bool paused = runToPause(3000);
		u16 pc = (u16)di->GetCpuPC();
		u8 value = readResult();
		const CDebugSourceMapping *m = di->ResolveSourceAt(scratch->debugDatabase.get(), (u32)banked->debugRegionId, 0x9000);
		snprintf(msg, sizeof(msg), "%sthe bankA breakpoint next stops at bankA's second call (PC $%04X, result %d, line %d), skipping the rest of bankB",
				 label.c_str(), pc, value, m ? m->lineStart : 0);
		ok = Check(paused && pc == 0x9000 && value == 11 && m && m->lineStart == lineA, msg);
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x9000, "bankA", got), label + "with bank 0 in, the label at $9000 is bankA (got " + got + ")");
	}
	if (ok)
	{
		bool paused = runToPause(500);
		ok = Check(!paused, label + "no further stop -- main's loop never reaches bank 0's line again");
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

#ifdef RUN_ATARI
// Atari cartridge changes on the emulation thread, between two frames
class CTaskInsertAtariCartridge : public CDebugInterfaceTask
{
public:
	CTaskInsertAtariCartridge(CDebugInterfaceAtari *di, const std::string &path) : di(di), path(path) {}
	virtual void ExecuteTask() override { di->InsertCartridge((char *)path.c_str(), true); }
private:
	CDebugInterfaceAtari *di;
	std::string path;
};

class CTaskDetachAtariCartridge : public CDebugInterfaceTask
{
public:
	CTaskDetachAtariCartridge(CDebugInterfaceAtari *di) : di(di) {}
	virtual void ExecuteTask() override { di->DetachCartridge(); }
private:
	CDebugInterfaceAtari *di;
};
#endif

// the machine running: until every queued VSync task has run
static bool WaitForVSyncTasks(CDebugInterface *di, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs; waited += 10)
	{
		di->LockTasksMutex();
		bool empty = di->vsyncTasks.empty();
		di->UnlockTasksMutex();
		if (empty)
			return true;
		SYS_Sleep(10);
	}
	return false;
}

// The Atari: the XEGS build (main.car is ld65's output, so the debug info's
// image offsets are .car file offsets) inserted on the emulation thread;
// the machine cold starts and the OS runs the cartridge. bankA (bank 0) and
// bankB (bank 1) are at $8000, main() in the fixed bank at $A000.
bool CTestBankedSource::RunAtari()
{
#ifdef RUN_ATARI
	CDebugInterfaceAtari *di = (CDebugInterfaceAtari *)viewC64->debugInterfaceAtari;
	if (!di)
	{
		ReportRequiredGap("Atari not created");
		return true;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	const std::string label = "Banked Atari XEGS: ";
	std::string dir = ResolveProjectPath("tests/data/source-debug/cc65/atari-xegs/banked");

	di->PauseEmulationBlockedWait();
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

	// bankB's body (line 21) and bankA's body (line 15), both at $8000 + n
	CDebugSourceBreakpoint *onB = ok ? scratch->AddSourceBreakpoint("xegs.c", 21) : NULL;
	CDebugBreakpointAddr *addressB = (onB && onB->IsResolved())
		? scratch->currentSegment->breakpointsPC->GetBreakpoint(onB->bindings[0].address) : NULL;
	char bound[160] = "";
	if (onB && onB->IsResolved())
		snprintf(bound, sizeof(bound), " (%d bindings, $%04X, banked %d, %d image offsets, first $%llX)", (int)onB->bindings.size(),
				 onB->bindings[0].address, (int)onB->bindings[0].banked, addressB ? (int)addressB->imageOffsets.size() : -1,
				 (unsigned long long)(addressB && !addressB->imageOffsets.empty() ? addressB->imageOffsets[0] : 0));
	ok = ok && Check(onB && onB->IsResolved() && onB->bindings.size() == 1 && onB->bindings[0].banked
					 && addressB && addressB->imageOffsets.size() == 1 && addressB->imageOffsets[0] >= 0x2010,
					 label + "the source breakpoint on bankB's line binds to one address with bank 1's .car offset" + bound);

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
	// _result is the one BSS byte, at $3000 (main.map)
	auto readResult = [&]() { u8 v = 0; bool available = false; di->GetDataAdapter()->AdapterReadByte(0x3000, &v, &available); return v; };

	bool inserted = false;
	if (ok)
	{
		// the insert cold starts the machine; the OS runs the cartridge to the first stop
		di->AddVSyncTask(new CTaskInsertAtariCartridge(di, dir + "/main.car"));
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		inserted = WaitForVSyncTasks(di, 5000);
		bool paused = false;
		for (int waited = 0; inserted && waited < 5000 && !paused; waited += 10)
		{
			SYS_Sleep(10);
			paused = di->GetDebugMode() == DEBUGGER_MODE_PAUSED && (u16)di->GetCpuPC() == onB->bindings[0].address;
		}
		u16 pc = (u16)di->GetCpuPC();
		u8 value = readResult();
		char msg[220];
		snprintf(msg, sizeof(msg), "%sthe cartridge inserts; the bankB breakpoint stops once bank 1 is in, after bankA's first call (PC $%04X, result %d), not at bankA's code at the same address", label.c_str(), pc, value);
		ok = Check(inserted && paused && value == 1, msg);
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x8000, "_bankB", got), label + "with bank 1 in, the label at $8000 is _bankB (got " + got + ")");
	}

	CDebugSourceBreakpoint *onA = NULL;
	if (ok)
	{
		scratch->RemoveSourceBreakpoint(onB);
		onB = NULL;
		onA = scratch->AddSourceBreakpoint("xegs.c", 15);
		ok = Check(onA && onA->IsResolved() && onA->bindings.size() == 1 && onA->bindings[0].banked,
				   label + "the source breakpoint on bankA's line binds with bank 0's .car offset");
	}
	if (ok)
	{
		bool paused = runToPause(3000);
		u16 pc = (u16)di->GetCpuPC();
		u8 value = readResult();
		char msg[200];
		snprintf(msg, sizeof(msg), "%sthe bankA breakpoint next stops at bankA's second call (PC $%04X, result %d), skipping the rest of bankB", label.c_str(), pc, value);
		ok = Check(paused && pc == onA->bindings[0].address && value == 11, msg);
		std::string got;
		ok = ok && Check(LabelIs(scratch, 0x8000, "_bankA", got), label + "with bank 0 in, the label at $8000 is _bankA (got " + got + ")");
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
	if (inserted)
	{
		di->AddVSyncTask(new CTaskDetachAtariCartridge(di));		// resets the machine
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		if (!WaitForVSyncTasks(di, 5000))
			LOGError("CTestBankedSource: the Atari cartridge detach task never ran");
	}
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
	return ok;
#else
	ReportRequiredGap("Atari emulation not compiled in");
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
	const NesCartridge uxrom = { "UxROM", "nes-uxrom", "uxrom.c", 18, 24, 0x6000, 0x4010 };
	const NesCartridge mmc1 = { "MMC1", "nes-mmc1", "mmc1.c", 19, 25, 0x0300, 0x4010 };
	const NesCartridge mmc3 = { "MMC3", "nes-mmc3", "mmc3.c", 17, 23, 0x0300, 0x2010 };	// 8 KB banks
	bool ok = RunDatabase() && RunNes(uxrom) && RunNes(mmc1) && RunNes(mmc3) && RunC64() && RunC64KickAss() && RunAtari();
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Banked code resolves and breaks by the bank in the window (NES UxROM, MMC1 and MMC3, C64 Magic Desk, Atari XEGS; cc65, and KickAssembler with a segment table)");
}
