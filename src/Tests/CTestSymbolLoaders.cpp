#include "CTestSymbolLoaders.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugDatabase.h"
#include "CDebugBreakpointsAddr.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include <cstring>

bool CTestSymbolLoaders::Check(bool condition, const char *message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message);
	return true;
}

// CByteBuffer takes ownership of a new[] buffer.
static CByteBuffer *BufferFromText(const std::string &text)
{
	uint8 *data = new uint8[text.size()];
	memcpy(data, text.data(), text.size());
	return new CByteBuffer(data, (int)text.size());
}

static bool LabelIs(CDebugSymbolsSegment *segment, int address, const char *text)
{
	CDebugSymbolsCodeLabel *label = segment->FindLabel(address);
	return label != NULL && strcmp(label->GetLabelText(), text) == 0;
}

static bool SegmentNamed(CDebugSymbolsSegment *segment, const char *name)
{
	return segment != NULL && segment->name->CompareWith(name);
}

// The mapping in effect at ADDRESS in SEGMENT's region of the loaded database.
static const CDebugSourceMapping *At(CDebugSymbols *symbols, CDebugSymbolsSegment *segment, int address)
{
	if (symbols->debugDatabase == NULL || segment->debugRegionId < 0)
		return NULL;
	return symbols->debugDatabase->Resolve((u32)segment->debugRegionId, (u32)address);
}

static bool SourceLineIs(const CDebugSourceMapping *line, int sourceId, int lineStart, int colStart, int lineEnd, int colEnd)
{
	return line != NULL && (int)line->fileId == sourceId
		&& line->lineStart == lineStart && line->columnStart == colStart
		&& line->lineEnd == lineEnd && line->columnEnd == colEnd;
}

static bool SourceTextIs(const CDebugSourceFile *file, int lineNum, const char *text)
{
	if (lineNum >= (int)file->lines.size())
		return false;
	char *ascii = file->lines[lineNum]->GetStdASCII();
	bool same = strcmp(ascii, text) == 0;
	delete [] ascii;
	return same;
}

// VICE monitor labels: "al C:8104 .kbobxpos".
bool CTestSymbolLoaders::RunViceLabels(CDebugSymbols *symbols)
{
	std::string path = ResolveProjectPath("assets/tests/labelstest1.vs");
	if (!Check(!path.empty(), "VICE .vs: project root resolved"))
		return false;
	CSlrString *slrPath = new CSlrString(path.c_str());
	symbols->ParseSymbols(slrPath);
	delete slrPath;

	CDebugSymbolsSegment *segment = symbols->currentSegment;
	if (!Check(symbols->segments.size() == 1 && SegmentNamed(segment, "Default"),
			   "VICE .vs: the format has no segments, labels land in one 'Default' segment"))
		return false;

	// 91 lines, two addresses defined twice ($0002, $85C2): 89 labels.
	if (!Check(segment->codeLabelByAddress.size() == 89, "VICE .vs: 89 labels for 89 distinct addresses"))
		return false;
	if (!Check(LabelIs(segment, 0x8104, "kbobxpos") && LabelIs(segment, 0x8000, "main"),
			   "VICE .vs: labels keyed by address, leading '.' removed"))
		return false;
	CDebugSymbolsCodeLabel *music = segment->FindLabelByText("music");
	if (!Check(music != NULL && music->address == 0x1000, "VICE .vs: label found by text"))
		return false;
	// Quirk: a second label at the same address REPLACES the first one, in both indexes.
	if (!Check(LabelIs(segment, 0x0002, "zp_begin") && LabelIs(segment, 0x85C2, "TB")
			   && segment->FindLabelByText("ScanResult") == NULL,
			   "VICE .vs: a duplicate address keeps only the last label (today's behaviour)"))
		return false;
	return true;
}

// 64tass --labels output: "NAME = $addr" or "NAME = decimal".
bool CTestSymbolLoaders::Run64TassLabels(CDebugSymbols *symbols)
{
	CByteBuffer *buffer = BufferFromText("DRIVERDONE      = $0c48\nCOUNT = 42\nlowcase\t= $c000\n");
	symbols->ParseSymbols(buffer);
	delete buffer;

	CDebugSymbolsSegment *segment = symbols->currentSegment;
	if (!Check(symbols->segments.size() == 1 && SegmentNamed(segment, "Default")
			   && segment->codeLabelByAddress.size() == 3,
			   "64tass labels: three labels in the 'Default' segment"))
		return false;
	if (!Check(LabelIs(segment, 0x0C48, "DRIVERDONE") && LabelIs(segment, 42, "COUNT")
			   && LabelIs(segment, 0xC000, "lowcase"),
			   "64tass labels: '$' is hex, a bare number is decimal, spaces and tabs around '='"))
		return false;
	return true;
}

// KickAssembler -debugdump .dbg, loaded the way PRG autoload does it
// (CMainMenuHelper: DeleteAllSymbols, then ParseSourceDebugInfo).
bool CTestSymbolLoaders::RunKickAssDbg(CDebugSymbols *symbols)
{
	std::string path = ResolveProjectPath("assets/tests/c64/dankwars.dbg");
	if (!Check(!path.empty(), "KickAss .dbg: project root resolved"))
		return false;
	symbols->DeleteAllSymbols();
	CSlrString *slrPath = new CSlrString(path.c_str());
	symbols->ParseSourceDebugInfo(slrPath);
	delete slrPath;

	if (!Check(symbols->segments.size() == 3 && SegmentNamed(symbols->segments[0], "Base")
			   && SegmentNamed(symbols->segments[1], "Default") && SegmentNamed(symbols->segments[2], "Patch"),
			   "KickAss .dbg: one segment per <Segment>, in file order"))
		return false;
	if (!Check(symbols->currentSegment == symbols->segments[0], "KickAss .dbg: the first segment is activated"))
		return false;
	CDebugSymbolsSegment *base = symbols->segments[0];
	CDebugSymbolsSegment *patch = symbols->segments[2];

	if (!Check(symbols->debugDatabase != NULL && symbols->debugDatabase->NumFiles() == 3,
			   "KickAss .dbg: three <Sources> entries"))
		return false;

	// $0801-$F834 -> file 1, 19:16-19:23 (one range covering the whole program).
	if (!Check(SourceLineIs(At(symbols, base, 0x0801), 1, 19, 16, 19, 23)
			   && At(symbols, base, 0xF834) == At(symbols, base, 0x0801)
			   && At(symbols, base, 0x0800) == NULL,
			   "KickAss .dbg: an address range maps every address to one source line object"))
		return false;
	if (!Check(SourceLineIs(At(symbols, patch, 0xA000), 1, 8, 3, 8, 5)
			   && SourceLineIs(At(symbols, patch, 0xA002), 2, 6, 3, 6, 5)
			   && SourceLineIs(At(symbols, patch, 0xB010), 2, 17, 3, 17, 7)
			   && At(symbols, patch, 0xA023) == NULL,
			   "KickAss .dbg: file:line:column per address in the 'Patch' segment"))
		return false;
	// The map is per segment: 'Base' covers $0801-$F834, 'Patch' does not.
	if (!Check(At(symbols, patch, 0x0801) == NULL
			   && At(symbols, base, 0xA000) != At(symbols, patch, 0xA000),
			   "KickAss .dbg: addresses of one segment are absent from another"))
		return false;
	const CDebugSourceMapping *a002 = At(symbols, patch, 0xA002);
	const CDebugSourceMapping *b000 = At(symbols, patch, 0xB000);
	if (!Check(a002 != NULL && b000 != NULL
			   && symbols->debugDatabase->GetBlock(a002->blockId)->name == "Joycontrol1"
			   && symbols->debugDatabase->GetBlock(b000->blockId)->name == "Joycontrol2",
			   "KickAss .dbg: each line remembers its <Block>"))
		return false;

	if (!Check(patch->codeLabelByAddress.size() == 7 && LabelIs(patch, 0xA000, "test")
			   && LabelIs(patch, 0xB010, "Joy2Int") && base->codeLabelByAddress.empty(),
			   "KickAss .dbg: <Labels> go to their segment"))
		return false;
	// Quirk: "Start" is defined in both blocks; the by-text index keeps the last one.
	CDebugSymbolsCodeLabel *start = patch->FindLabelByText("Start");
	if (!Check(start != NULL && start->address == 0xB000 && LabelIs(patch, 0xA002, "Start"),
			   "KickAss .dbg: a duplicate label text resolves to the last address (today's behaviour)"))
		return false;

	if (!Check(patch->breakpointsPC->breakpoints.size() == 5
			   && patch->breakpointsPC->breakpoints.count(0xA00B) == 1
			   && base->breakpointsPC->breakpoints.empty(),
			   "KickAss .dbg: <Breakpoints> become PC breakpoints of their segment"))
		return false;

	// The fixture's sources are absolute Windows paths that do not exist here.
	const CDebugSourceFile *missing = symbols->debugDatabase->GetFile(1);
	if (!Check(missing != NULL && !missing->HasText() && missing->fileName == "dankwars.asm",
			   "KickAss .dbg: a missing source file leaves no text and is not an error"))
		return false;
	return true;
}

// A .dbg whose source file exists: the loader reads the text, line-indexed
// from 1, with tabs expanded to four spaces.
bool CTestSymbolLoaders::RunKickAssSourceText(CDebugSymbols *symbols)
{
	std::string asmPath = ResolveProjectPath("assets/tests/symbols/simple.asm");
	if (!Check(!asmPath.empty(), "KickAss source text: project root resolved"))
		return false;

	std::string xml =
		"<C64debugger version=\"1.0\">\n"
		"<Sources values=\"INDEX,FILE\">\n"
		"0," + asmPath + "\n"
		"</Sources>\n"
		"<Segment name=\"Default\" dest=\"\" values=\"START,END,FILE_IDX,LINE1,COL1,LINE2,COL2\">\n"
		"<Block name=\"Default\">\n"
		"$1000,$1001,0,3,8,3,16\n"
		"$1002,$1004,0,4,2,4,10\n"
		"$1005,$1007,0,5,7,5,15\n"
		"</Block>\n"
		"</Segment>\n"
		"<Labels values=\"SEGMENT,ADDRESS,NAME,START,END,FILE_IDX,LINE1,COL1,LINE2,COL2\">\n"
		"Default,$1000,start,1,3,1,3,6\n"
		"Default,$1005,loop,1,5,1,5,5\n"
		"</Labels>\n"
		"</C64debugger>\n";

	symbols->DeleteAllSymbols();
	CByteBuffer *buffer = BufferFromText(xml);
	symbols->ParseSourceDebugInfo(buffer);
	delete buffer;

	if (!Check(symbols->debugDatabase != NULL && symbols->debugDatabase->NumFiles() == 1
			   && symbols->segments.size() == 1,
			   "KickAss source text: one source, one segment"))
		return false;
	const CDebugSourceFile *file = symbols->debugDatabase->GetFile(0);
	// Index 0 is an empty placeholder so that index == 1-based line number.
	if (!Check(file != NULL && file->lines.size() == 6 && SourceTextIs(file, 0, ""),
			   "KickAss source text: 5 lines loaded, indexed from 1"))
		return false;
	if (!Check(SourceTextIs(file, 3, "start:    lda #$01") && SourceTextIs(file, 4, "    sta $d020")
			   && SourceTextIs(file, 2, "        * = $1000"),
			   "KickAss source text: each tab becomes four spaces (no tab stops)"))
		return false;
	CDebugSymbolsSegment *segment = symbols->segments[0];
	const CDebugSourceMapping *line = At(symbols, segment, 0x1003);
	if (!Check(SourceLineIs(line, 0, 4, 2, 4, 10) && symbols->debugDatabase->GetFile(line->fileId) == file
			   && LabelIs(segment, 0x1005, "loop"),
			   "KickAss source text: address -> line -> text resolves end to end"))
		return false;
	return true;
}

void CTestSymbolLoaders::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulation is not compiled in");
	return;
#else
	CDebugInterfaceC64 *c64 = viewC64->debugInterfaceC64;
	if (c64 == NULL || c64->symbols == NULL || c64->dataAdapterC64 == NULL)
	{
		TestCompleted(false, "C64 debug interface, symbols or data adapter is NULL");
		return;
	}

	typedef bool (CTestSymbolLoaders::*SubTest)(CDebugSymbols *);
	SubTest subTests[] = {
		&CTestSymbolLoaders::RunViceLabels,
		&CTestSymbolLoaders::Run64TassLabels,
		&CTestSymbolLoaders::RunKickAssDbg,
		&CTestSymbolLoaders::RunKickAssSourceText,
	};

	CDebugSymbols *liveSymbols = c64->symbols;
	bool ok = true;
	for (SubTest subTest : subTests)
	{
		// The loaders write into debugInterface->symbols (the loader used to read
		// it directly), so a scratch instance is swapped in for each part.
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

		// The scratch instance is deliberately leaked: segments are never
		// deleted today (see the "THE BUG!" notes in CDebugSymbols); RD-032
		// decides their lifetime.
		if (!ok)
			break;
	}

	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "VICE .vs, 64tass and KickAss .dbg loaders behave as recorded");
#endif
}
