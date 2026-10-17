#include "CTestDebugInfoFormats.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "CSlrFileFromOS.h"
#include <cstring>
#include <cstdio>
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

static CByteBuffer *BufferFromText(const std::string &text)
{
	uint8 *data = new uint8[text.size()];
	memcpy(data, text.data(), text.size());
	return new CByteBuffer(data, (int)text.size());
}

bool CTestDebugInfoFormats::RunSegmentsAndWatches(CDebugSymbols *symbols)
{
	DebugInfoLoadFile(symbols, ResolveProjectPath("assets/tests/c64/dankwars.dbg").c_str());
	CSlrString *patchName = new CSlrString("Patch");
	bool found = symbols->SetSegment(patchName);
	delete patchName;
	if (!Check(found && symbols->currentSegment == symbols->segments[2] && symbols->currentSegmentNum == 2,
			   "SetSegment activates the segment, including its number"))
		return false;
	symbols->SelectNextSegment();
	if (!Check(symbols->currentSegment == symbols->segments[0],
			   "next segment continues from the one SetSegment chose (wraps to the first)"))
		return false;

	const char *head = "<C64debugger version=\"1.0\">\n<Sources values=\"INDEX,FILE\">\n0,a.asm\n</Sources>\n"
		"<Segment name=\"Default\" dest=\"\" values=\"START,END,FILE_IDX,LINE1,COL1,LINE2,COL2\">\n</Segment>\n";
	std::string rangeLayout = std::string(head)
		+ "<Watchpoints values=\"SEGMENT,ADDRESS1,ADDRESS2,ARGUMENT\">\n"
		+ "Default,$c000,$c00f,store\nDefault,$d020,,load\n</Watchpoints>\n</C64debugger>\n";
	CByteBuffer *buffer = BufferFromText(rangeLayout);
	DebugInfoLoadKickAss(symbols, buffer, NULL);
	delete buffer;
	CDebugSymbolsDataWatch *range = symbols->currentSegment->FindWatch(0xC000);
	CDebugSymbolsDataWatch *single = symbols->currentSegment->FindWatch(0xD020);
	if (!Check(range != NULL && range->numberOfValues == 16 && single != NULL && single->numberOfValues == 1,
			   "<Watchpoints> ADDRESS1,ADDRESS2 layout: the range size is the number of values"))
		return false;

	std::string countLayout = std::string(head)
		+ "<Watchpoints>\nDefault,$c100,4\n</Watchpoints>\n</C64debugger>\n";
	buffer = BufferFromText(countLayout);
	DebugInfoLoadKickAss(symbols, buffer, NULL);
	delete buffer;
	CDebugSymbolsDataWatch *counted = symbols->currentSegment->FindWatch(0xC100);
	if (!Check(counted != NULL && counted->numberOfValues == 4,
			   "<Watchpoints> older ADDRESS,NUMBER_OF_VALUES layout still works"))
		return false;
	return true;
}

bool CTestDebugInfoFormats::RunDriveLabels(CDebugSymbols *symbols)
{
	CDebugInterfaceC64 *c64 = viewC64->debugInterfaceC64;
	if (c64->symbolsDrive1541 == NULL || c64->dataAdapterDrive1541 == NULL)
	{
		ReportRequiredGap("no 1541 symbols on this C64 backend, so D: labels were never exercised");
		return true;
	}

	// a scratch 1541 symbols object too, so the live drive labels stay untouched
	CDebugSymbolsDrive1541 *liveDrive = c64->symbolsDrive1541;
	CDebugSymbolsDrive1541 *scratchDrive = new CDebugSymbolsDrive1541(c64);
	scratchDrive->SetDataAdapter(c64->dataAdapterDrive1541);
	scratchDrive->CreateDefaultSegment();
	guiMain->LockMutex();
	c64->LockMutex();
	c64->symbolsDrive1541 = scratchDrive;
	c64->UnlockMutex();
	guiMain->UnlockMutex();

	CByteBuffer *buffer = BufferFromText("al C:1000 .main\nal D:0500 .drivecode\n");
	symbols->ParseSymbols(buffer);
	delete buffer;
	bool c64Label = symbols->currentSegment->FindLabelByText("main") != NULL
		&& symbols->currentSegment->FindLabelByText("drivecode") == NULL;
	CDebugSymbolsCodeLabel *driveLabel = scratchDrive->currentSegment->FindLabelByText("drivecode");

	guiMain->LockMutex();
	c64->LockMutex();
	c64->symbolsDrive1541 = liveDrive;
	c64->UnlockMutex();
	guiMain->UnlockMutex();

	return Check(c64Label && driveLabel != NULL && driveLabel->address == 0x0500,
				 "VICE \"al D:\" labels go to the 1541 symbols, \"al C:\" to the C64");
}

bool CTestDebugInfoFormats::RunOscar64(CDebugSymbols *symbols)
{
	// The artifacts record their build directory; remap it to the fixture
	// so the source text is found (that is what the manifest's buildDir is for).
	CDebugInfoLoadOptions options;
	options.pathRemaps.push_back(std::make_pair(std::string("/tmp/rd-source-debug/oscar64/c64/basic-flow"),
												ResolveProjectPath("tests/source-debug/fixtures/basic-flow")));
	DebugInfoFormat format = DebugInfoLoadFile(symbols,
		ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow/main.dbj").c_str(), options);
	std::shared_ptr<const CDebugDatabase> db = symbols->debugDatabase;
	CDebugSymbolsSegment *segment = symbols->currentSegment;
	if (!Check(format == DebugInfoFormat::Oscar64Dbj && db != NULL && db->producer == "Oscar64"
			   && symbols->segments.size() == 1 && segment != NULL && segment->debugRegionId == 0
			   && db->capabilities.functions && db->capabilities.lineMappings && !db->capabilities.columns,
			   "Oscar64 .dbj: one Default segment, functions and line mappings, no columns"))
		return false;

	// add() is [2218, 2224) at main.c:7, its body line 8; main's lines 16-18
	const CDebugFunction *add = db->FindFunction(0, 2220);
	const CDebugFunction *main = db->FindFunction(0, 2176);
	const CDebugSourceMapping *body = db->Resolve(0, 2219);
	const CDebugSourceMapping *call = db->Resolve(0, 2185);
	const CDebugSourceFile *mainC = body ? db->GetFile(body->fileId) : NULL;
	if (!Check(add != NULL && add->name == "add" && add->signature == "add(u8,u8)->u8" && add->start == 2218
			   && add->end == 2224 && add->line == 7 && main != NULL && main->name == "main"
			   && db->FindFunction(0, 2224) == NULL,
			   "Oscar64 .dbj: functions resolve by address, with their signature and range"))
		return false;
	if (!Check(body != NULL && body->lineStart == 8 && call != NULL && call->lineStart == 18
			   && mainC != NULL && mainC->fileName == "main.c" && db->Resolve(0, 2199) == NULL,
			   "Oscar64 .dbj: line mappings, and the endless loop's jump has none (Missing)"))
		return false;

	CDebugSymbolsCodeLabel *addLabel = segment->FindLabelByText("add");
	if (!Check(addLabel != NULL && addLabel->address == 2218 && segment->FindLabelByText("result") != NULL,
			   "Oscar64 .dbj: labels for functions and variables"))
		return false;

	char *text = (mainC != NULL && mainC->lines.size() > 8) ? mainC->lines[8]->GetStdASCII() : NULL;
	bool textOk = text != NULL && strcmp(text, "    return left + right;") == 0;
	delete [] text;
	if (!Check(textOk, "Oscar64 .dbj: the source text is found through the build-directory remap"))
		return false;

	const char *machines[] = { "atari", "nes" };
	for (const char *machine : machines)
	{
		std::string path = ResolveProjectPath((std::string("tests/data/source-debug/oscar64/") + machine + "/basic-flow/main.dbj").c_str());
		format = DebugInfoLoadFile(symbols, path.c_str());
		db = symbols->debugDatabase;
		bool found = false;
		for (const CDebugFunction &f : db->Functions())
			found |= (f.name == "add" && db->Resolve(0, f.start) != NULL);
		char msg[128];
		snprintf(msg, sizeof(msg), "Oscar64 .dbj (%s): add() has a line mapping at its entry", machine);
		if (!Check(format == DebugInfoFormat::Oscar64Dbj && found, msg))
			return false;
	}
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
	SubTest subTests[] = { &CTestDebugInfoFormats::RunRouting, &CTestDebugInfoFormats::RunReimport,
						   &CTestDebugInfoFormats::RunSegmentsAndWatches, &CTestDebugInfoFormats::RunDriveLabels,
						   &CTestDebugInfoFormats::RunOscar64 };

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
