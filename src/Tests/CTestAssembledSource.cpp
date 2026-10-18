#include "CTestAssembledSource.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CViewSourceCode.h"
#include "CDebugInterfaceVice.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CSlrString.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstring>

bool CTestAssembledSource::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

static std::string LineText(const CDebugSourceFile *file, i32 line)
{
	if (file == NULL || line <= 0 || line >= (i32)file->lines.size())
		return "";
	char *text = file->lines[line]->GetStdASCII();
	std::string s(text);
	delete [] text;
	return s;
}

bool CTestAssembledSource::RunDatabase()
{
	// lines 1, 2 and 3 of a three-line text at $C000-$C005
	const char *text = "\tlda #1\n\tinc $d020\n\trts\n";
	std::map<u32, i32> lines = { { 0xC000, 1 }, { 0xC001, 1 }, { 0xC002, 2 }, { 0xC003, 2 }, { 0xC004, 2 }, { 0xC005, 3 } };
	std::shared_ptr<CDebugDatabase> fragment = DebugInfoFromAssembly("snippet.asm", text, strlen(text), lines);
	const CDebugSourceMapping *atInc = fragment->Resolve(0, 0xC003);
	const CDebugSourceFile *file = fragment->GetFile(0);
	bool ok = fragment->NumMappings() == 3 && atInc && atInc->start == 0xC002 && atInc->end == 0xC005 && atInc->lineStart == 2
		&& file && file->fileName == "snippet.asm" && LineText(file, 2) == "    inc $d020";
	if (!Check(ok, "Assembled database: the line map becomes one mapping per run of bytes from a line, the text is the file's (tabs expanded)"))
		return false;

	// on top of loaded debug info: both resolve, the fragment in the base's region
	std::shared_ptr<CDebugDatabase> base = std::make_shared<CDebugDatabase>("test");
	u32 region = base->AddRegion("Default", "");
	u32 block = base->AddBlock("", region);
	base->AddFile(3, "main.c");
	CDebugSourceMapping m;
	m.start = 0x0880;
	m.end = 0x0890;
	m.regionId = region;
	m.blockId = block;
	m.fileId = 3;
	m.lineStart = m.lineEnd = 17;
	base->AddMapping(m);
	base->Finalize();
	int regionId = (int)region;
	std::shared_ptr<CDebugDatabase> merged = DebugDatabaseMerge(base.get(), { fragment }, &regionId);
	const CDebugSourceMapping *baseLine = merged->Resolve(region, 0x0885);
	const CDebugSourceMapping *asmLine = merged->Resolve(region, 0xC005);
	ok = regionId == (int)region && merged->NumFiles() == 2 && baseLine && baseLine->fileId == 3 && baseLine->lineStart == 17
		&& asmLine && asmLine->lineStart == 3 && merged->GetFile(asmLine->fileId) && merged->GetFile(asmLine->fileId)->fileName == "snippet.asm"
		&& LineText(merged->GetFile(asmLine->fileId), 3) == "    rts";
	return Check(ok, "Assembled database: merged on top of loaded debug info, both resolve in the base's region; the fragment's file is renumbered past the base's");
}

bool CTestAssembledSource::RunC64()
{
#ifdef RUN_COMMODORE64
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	CDebuggerApi *api = di ? CDebuggerApi::GetDebuggerApi(EMULATOR_TYPE_C64_VICE) : NULL;
	CViewSourceCode *view = viewC64->viewC64SourceCode;
	if (!di || !api || !view)
	{
		ReportRequiredGap("C64 debugger API or source view not created");
		return true;
	}
	if (view->visible)
	{
		ReportRequiredGap("C64 Source code view is open: the test swaps its symbols");
		return true;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();

	u8 savedRam[0x200];
	for (int i = 0; i < 0x200; i++)
		savedRam[i] = di->GetByteFromRamC64((u16)(0xC000 + i));

	CDebugSymbols *liveSymbols = di->symbols;
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();
	CDebugSymbols *viewSymbols = view->debugSymbols;
	view->debugSymbols = scratch;

	// the plugin path: A(...) lines, then into RAM
	int start = 0, size = 0;
	api->Assemble64TassClearBuffer();
	api->Assemble64TassAddLine("*=$c000");
	api->Assemble64TassAddLine("start   lda #$01");
	api->Assemble64TassAddLine("loop    inc border");
	api->Assemble64TassAddLine("        jmp loop");
	api->Assemble64TassAddLine("border = $d020");
	bool assembled = api->Assemble64TassToRam(&start, &size, NULL, true);
	const u8 expected[] = { 0xA9, 0x01, 0xEE, 0x20, 0xD0, 0x4C, 0x02, 0xC0 };
	bool bytes = assembled && start == 0xC000 && size == 8;
	for (int i = 0; bytes && i < 8; i++)
		bytes = di->GetByteFromRamC64((u16)(0xC000 + i)) == expected[i];
	CDebugSymbolsSegment *segment = scratch->currentSegment;
	CDebugSymbolsCodeLabel *startLabel = segment ? segment->FindLabel(0xC000) : NULL;
	CDebugSymbolsCodeLabel *loopLabel = segment ? segment->FindLabel(0xC002) : NULL;
	bool ok = Check(bytes && startLabel && !strcmp(startLabel->GetLabelText(), "start") && loopLabel
					&& !strcmp(loopLabel->GetLabelText(), "loop") && segment->FindLabel(0xD020) == NULL,
					"C64: 64tass code assembled into RAM ($C000, 8 bytes) gives the segment its code labels start and loop, not the constant border");

	// its text and line map: $C005 (jmp) is line 4 of 64tass-C000.asm
	std::shared_ptr<const CDebugDatabase> database = scratch->debugDatabase;
	const CDebugSourceMapping *atJmp = (ok && database && segment->debugRegionId >= 0)
		? database->Resolve((u32)segment->debugRegionId, 0xC005) : NULL;
	const CDebugSourceFile *file = atJmp ? database->GetFile(atJmp->fileId) : NULL;
	ok = ok && Check(atJmp && atJmp->lineStart == 4 && file && file->fileName == "64tass-C000.asm"
					 && LineText(file, 3) == "loop    inc border",
					 "C64: the assembled text is a source of the symbols, $C005 maps to its line 4");

	// the source view shows it: no file on disk, the text from the database
	if (ok)
	{
		std::shared_ptr<const CSourceDocument> document = view->OpenFileText(file);
		ok = Check(view->ShowAddress(0xC002) && view->GetSelectedLine() == 3 && document && document->NumLines() == 5
				   && document->GetLine(4) == "        jmp loop",
				   "C64: the source view shows $C002 at line 3 of the assembled text, read from memory");
	}

	// assembled over: the old text is gone; elsewhere: both stay
	if (ok)
	{
		api->Assemble64TassClearBuffer();
		api->Assemble64TassAddLine("*=$c000");
		api->Assemble64TassAddLine("again   rts");
		bool over = api->Assemble64TassToRam(&start, &size, NULL, true);
		api->Assemble64TassClearBuffer();
		api->Assemble64TassAddLine("*=$c100");
		api->Assemble64TassAddLine("other   nop");
		api->Assemble64TassAddLine("        rts");
		bool elsewhere = api->Assemble64TassToRam(&start, &size, NULL, true);
		database = scratch->debugDatabase;
		const CDebugSourceMapping *atC000 = database->Resolve((u32)segment->debugRegionId, 0xC000);
		const CDebugSourceMapping *atC101 = database->Resolve((u32)segment->debugRegionId, 0xC101);
		ok = Check(over && elsewhere && scratch->assembledSources.size() == 2 && database->NumFiles() == 2
				   && atC000 && LineText(database->GetFile(atC000->fileId), atC000->lineStart) == "again   rts"
				   && atC101 && atC101->lineStart == 3,
				   "C64: code assembled over $C000 replaces that source, code at $C100 is a second one");
	}

	// a debug info load starts over
	if (ok)
	{
		DebugInfoFormat format = DebugInfoLoadFile(scratch, ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow/main.dbj").c_str());
		database = scratch->debugDatabase;
		ok = Check(format == DebugInfoFormat::Oscar64Dbj && scratch->assembledSources.empty() && database
				   && database->NumFiles() == 1 && database->Files().begin()->second->fileName == "main.c",
				   "C64: loading debug info drops the assembled sources");
	}

	// remote: a whole source, its labels and line ranges in the result
	if (ok)
	{
		nlohmann::json result = api->Assemble64TassJson("entry   lda #0\n        sta $d021\n        rts\n", 0xC000);
		bool labels = false;
		for (const nlohmann::json &label : result.value("labels", nlohmann::json::array()))
			labels |= label.value("name", "") == "entry" && label.value("address", "") == "C000" && label.value("code", false);
		nlohmann::json lines = result.value("lines", nlohmann::json::array());
		ok = Check(!result.contains("error") && result.value("address", "") == "C000" && result.value("size", 0) == 6 && labels
				   && lines.size() == 3 && lines[1].value("line", 0) == 2 && lines[1].value("start", "") == "C002"
				   && lines[1].value("end", "") == "C004" && di->GetByteFromRamC64(0xC005) == 0x60,
				   "C64 remote: a 64tass source at $C000 is written and reports its label entry and its three line ranges");
		// and still joins the symbols, on top of the loaded debug info
		database = scratch->debugDatabase;
		const CDebugSourceMapping *atSta = database->Resolve((u32)segment->debugRegionId, 0xC003);
		ok = ok && Check(atSta && LineText(database->GetFile(atSta->fileId), atSta->lineStart) == "        sta $d021"
						 && database->Resolve((u32)segment->debugRegionId, 0x0885) != NULL,
						 "C64 remote: the source joins the loaded Oscar64 debug info");
	}

	view->debugSymbols = viewSymbols;
	view->documents.Clear();
	for (int i = 0; i < 0x200; i++)
		di->SetByteToRamC64((u16)(0xC000 + i), savedRam[i]);
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

void CTestAssembledSource::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = RunDatabase() && RunC64();
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Code the embedded 64tass assembles gets labels, a line map and its text as a source");
}
