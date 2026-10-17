#include "CTestDebugInfoFormats.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "CSlrFileFromOS.h"
#include <cstring>
#include <vector>

bool CTestDebugInfoFormats::Check(bool condition, const char *message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message);
	return true;
}

static DebugInfoFormat ProbeFile(const char *relative)
{
	std::string path = CTest::ResolveProjectPath(relative);
	CSlrFileFromOS *file = new CSlrFileFromOS((char *)path.c_str());
	if (!file->Exists())
	{
		delete file;
		return DebugInfoFormat::Unknown;
	}
	CByteBuffer *buffer = new CByteBuffer(file, false);
	delete file;
	DebugInfoFormat format = DebugInfoProbe(buffer->data, buffer->length);
	delete buffer;
	return format;
}

static DebugInfoFormat ProbeText(const char *text)
{
	return DebugInfoProbe((const u8 *)text, (int)strlen(text));
}

bool CTestDebugInfoFormats::RunProbe()
{
	if (!Check(ProbeFile("assets/tests/c64/dankwars.dbg") == DebugInfoFormat::KickAssDbg
			   && ProbeFile("tests/data/source-debug/kickass/c64/basic-flow/main.dbg") == DebugInfoFormat::KickAssDbg
			   && ProbeFile("tests/data/source-debug/kickc/nes/basic-flow/main.dbg") == DebugInfoFormat::KickAssDbg,
			   "KickAssembler .dbg (hand-written asm and KickC output) is recognized"))
		return false;
	if (!Check(ProbeFile("tests/data/source-debug/cc65/c64/basic-flow/main.dbg") == DebugInfoFormat::Cc65Dbg
			   && ProbeFile("tests/data/source-debug/cc65/atari/same-line/main.dbg") == DebugInfoFormat::Cc65Dbg,
			   "a cc65 .dbg of the same extension is told apart by content"))
		return false;
	if (!Check(ProbeFile("tests/data/source-debug/oscar64/c64/basic-flow/main.dbj") == DebugInfoFormat::Oscar64Dbj,
			   "Oscar64 .dbj is recognized"))
		return false;
	if (!Check(ProbeFile("assets/tests/labelstest1.vs") == DebugInfoFormat::ViceLabels
			   && ProbeFile("tests/data/source-debug/64tass/c64/basic-flow/main.vs") == DebugInfoFormat::ViceLabels,
			   "VICE labels (from VICE and from 64tass --vice-labels) are recognized"))
		return false;
	if (!Check(ProbeText("DRIVERDONE      = $0c48\n") == DebugInfoFormat::Tass64Labels
			   && ProbeText("; labels\n\nloop\t= 2049\n") == DebugInfoFormat::Tass64Labels,
			   "64tass NAME = value labels are recognized, comments and blank lines skipped"))
		return false;
	if (!Check(ProbeText("\xEF\xBB\xBF  \n<C64debugger version=\"1.0\">") == DebugInfoFormat::KickAssDbg
			   && ProbeText("<?xml version=\"1.0\"?>\n<C64debugger version=\"1.0\">") == DebugInfoFormat::KickAssDbg,
			   "a BOM, leading blanks and an XML declaration do not hide KickAssembler's root"))
		return false;
	if (!Check(ProbeText("") == DebugInfoFormat::Unknown && ProbeText("hello world\n") == DebugInfoFormat::Unknown
			   && ProbeText("{ \"name\": 1 }") == DebugInfoFormat::Unknown && DebugInfoProbe(NULL, 5) == DebugInfoFormat::Unknown,
			   "anything else is Unknown, not guessed"))
		return false;
	return true;
}

bool CTestDebugInfoFormats::RunRouting(CDebugSymbols *symbols)
{
	size_t segmentsBefore = symbols->segments.size();
	DebugInfoFormat format = DebugInfoLoadFile(symbols, ResolveProjectPath("tests/data/source-debug/cc65/c64/basic-flow/main.dbg").c_str());
	if (!Check(format == DebugInfoFormat::Cc65Dbg && symbols->debugDatabase == NULL
			   && symbols->segments.size() == segmentsBefore,
			   "a cc65 .dbg is reported and NOT fed to the KickAssembler parser"))
		return false;

	format = DebugInfoLoadFile(symbols, ResolveProjectPath("tests/data/source-debug/kickass/c64/basic-flow/main.dbg").c_str());
	CDebugSymbolsSegment *segment = symbols->currentSegment;
	if (!Check(format == DebugInfoFormat::KickAssDbg && symbols->debugDatabase != NULL && segment != NULL
			   && segment->debugRegionId >= 0,
			   "a KickAssembler .dbg goes to the KickAssembler loader"))
		return false;

	std::shared_ptr<const CDebugDatabase> db = symbols->debugDatabase;
	const CDebugSourceMapping *m = db->Resolve((u32)segment->debugRegionId, 0x0813);
	CDebugSymbolsCodeLabel *add = segment->FindLabelByText("add");
	if (!Check(m != NULL && m->fileId == 1 && m->lineStart == 8 && m->start == 0x0812 && m->end == 0x0815
			   && add != NULL && add->address == 0x0832,
			   "basic-flow (KickAssembler): $0813 is kickass-c64.asm:8, label add at $0832"))
		return false;

	std::vector<const CDebugSourceMapping *> ranges;
	db->ResolveSource(1, 19, ranges);
	if (!Check(ranges.size() == 1 && ranges[0]->start == 0x082B && ranges[0]->end == 0x082E,
			   "basic-flow (KickAssembler): source line 19 resolves back to $082B-$082D"))
		return false;

	format = DebugInfoLoadFile(symbols, ResolveProjectPath("assets/tests/labelstest1.vs").c_str());
	if (!Check(format == DebugInfoFormat::ViceLabels && symbols->currentSegment != NULL
			   && symbols->currentSegment->FindLabelByText("main") != NULL,
			   "VICE labels go to the labels parser"))
		return false;
	return true;
}

bool CTestDebugInfoFormats::RunReimport(CDebugSymbols *symbols)
{
	// Load the same .dbg twice WITHOUT DeleteAllSymbols in between, as the
	// "Import labels" menu does.
	std::string path = ResolveProjectPath("assets/tests/c64/dankwars.dbg");
	DebugInfoLoadFile(symbols, path.c_str());
	std::shared_ptr<const CDebugDatabase> first = symbols->debugDatabase;
	DebugInfoLoadFile(symbols, path.c_str());
	std::shared_ptr<const CDebugDatabase> second = symbols->debugDatabase;

	if (!Check(symbols->segments.size() == 3 && first != NULL && second != NULL && first != second,
			   "a second import replaces the segments (3, not 6) and the database snapshot"))
		return false;

	CSlrString *patchName = new CSlrString("Patch");
	CDebugSymbolsSegment *patch = symbols->FindSegment(patchName);
	delete patchName;
	const CDebugSourceMapping *m = (patch != NULL && patch->debugRegionId >= 0)
		? second->Resolve((u32)patch->debugRegionId, 0xA002) : NULL;
	if (!Check(patch == symbols->segments[2] && m != NULL && m->lineStart == 6
			   && first->Resolve((u32)patch->debugRegionId, 0xA002) != NULL,
			   "the segment found by name is the new one, and a reader's old snapshot stays valid"))
		return false;
	return true;
}

void CTestDebugInfoFormats::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	if (!RunProbe())
	{
		TestCompleted(false, failure.c_str());
		return;
	}

#ifndef RUN_COMMODORE64
	ReportRequiredGap("C64 emulation not compiled in, so loader routing and re-import were never exercised");
	TestCompleted(true, "probe passed; loader routing needs the C64 and was not exercised");
	return;
#else
	CDebugInterfaceC64 *c64 = viewC64->debugInterfaceC64;
	if (c64 == NULL || c64->symbols == NULL || c64->dataAdapterC64 == NULL)
	{
		TestCompleted(false, "C64 debug interface, symbols or data adapter is NULL");
		return;
	}

	typedef bool (CTestDebugInfoFormats::*SubTest)(CDebugSymbols *);
	SubTest subTests[] = { &CTestDebugInfoFormats::RunRouting, &CTestDebugInfoFormats::RunReimport };

	CDebugSymbols *liveSymbols = c64->symbols;
	bool ok = true;
	for (SubTest subTest : subTests)
	{
		// Scratch symbols, as in CTestSymbolLoaders: the live ones stay untouched.
		CDebugSymbols *scratch = new CDebugSymbolsC64(c64);
		scratch->SetDataAdapter(c64->dataAdapterC64);
		scratch->CreateDefaultSegment();

		guiMain->LockMutex();
		c64->LockMutex();
		c64->symbols = scratch;
		c64->UnlockMutex();
		guiMain->UnlockMutex();

		ok = (this->*subTest)(scratch);

		guiMain->LockMutex();
		c64->LockMutex();
		c64->symbols = liveSymbols;
		c64->UnlockMutex();
		guiMain->UnlockMutex();

		// leaked on purpose, see CTestSymbolLoaders
		if (!ok)
			break;
	}

	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "debug info formats are recognized by content and routed to their loaders");
#endif
}
