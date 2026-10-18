#include "CTestSourceDocument.h"
#include "CSourceDocument.h"
#include "CSyntaxLexer.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CViewSourceCode.h"
#include "CDebugInterfaceVice.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebugSourceBreakpoint.h"
#include "SYS_Main.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

bool CTestSourceDocument::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

bool CTestSourceDocument::RunDocument()
{
	CSourceDocument document;
	document.LoadFromText("\xEF\xBB\xBF" "first\r\nsecond\rthird\n\tfourth\nab\tc");
	bool ok = document.NumLines() == 5 && document.GetLine(1) == "first" && document.GetLine(2) == "second"
		&& document.GetLine(3) == "third" && document.GetLine(4) == "    fourth" && document.GetLine(5) == "ab  c"
		&& document.GetLine(0).empty() && document.GetLine(6).empty();
	return Check(ok, "Source document: BOM dropped, LF/CRLF/CR end lines, tabs expand to 4-column stops, out-of-range lines are empty");
}

static bool WriteText(const std::string &path, const std::string &text)
{
	std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
	file << text;
	return (bool)file;
}

bool CTestSourceDocument::RunRegistry()
{
	namespace fs = std::filesystem;
	fs::path dir = "/tmp/rd-source-document";
	std::error_code error;
	fs::remove_all(dir, error);
	fs::create_directories(dir / "moved", error);
	fs::create_directories(dir / "root", error);
	WriteText((dir / "moved" / "a.c").string(), "int a;\n");
	WriteText((dir / "root" / "b.c").string(), "int b;\n");

	CSourceDocumentRegistry registry;
	registry.pathRemaps.push_back(std::make_pair(std::string("/build/old"), (dir / "moved").string()));
	registry.sourceRoots.push_back((dir / "root").string());

	auto a = registry.Open("/build/old/a.c", "");
	auto b = registry.Open("/elsewhere/b.c", "");
	auto missing = registry.Open("/elsewhere/c.c", "");
	std::vector<std::string> candidates = registry.Candidates("/elsewhere/c.c", "");
	bool ok = a && a->GetLine(1) == "int a;" && b && b->GetLine(1) == "int b;" && !missing
		&& candidates.size() == 2 && candidates[1] == (dir / "root" / "c.c").string();
	if (!Check(ok, "Source registry: a recorded path is found through a remap or by file name in a source root; a missing one lists where it was looked for"))
		return false;

	// the same document while unchanged, read again after an edit
	auto again = registry.Open("/build/old/a.c", "");
	WriteText((dir / "moved" / "a.c").string(), "int a2;\nint more;\n");
	fs::last_write_time(dir / "moved" / "a.c", fs::file_time_type::clock::now() + std::chrono::seconds(5), error);
	auto edited = registry.Open("/build/old/a.c", "");
	ok = again.get() == a.get() && edited && edited.get() != a.get() && edited->NumLines() == 2 && edited->GetLine(1) == "int a2;";
	return Check(ok, "Source registry: an unchanged file is served from the cache, an edited one is read again");
}

static std::string DumpTokens(const CSyntaxLexer *lexer, const CSourceDocument &document)
{
	std::ostringstream out;
	int state = 0;
	std::vector<SyntaxSpan> spans;
	for (int line = 1; line <= document.NumLines(); line++)
	{
		const std::string &text = document.GetLine(line);
		lexer->LexLine(text, &state, spans);
		for (const SyntaxSpan &span : spans)
			out << line << ":" << span.start << " " << SyntaxCategoryName(span.category) << " " << text.substr(span.start, span.length) << "\n";
	}
	return out.str();
}

bool CTestSourceDocument::RunLexers()
{
	struct Sample { const char *file; const char *producer; const char *lexer; };
	const Sample samples[] = {
		{ "sample.c", "", "C" },
		{ "sample-kickass.asm", "KickAssembler", "6502 (KickAssembler)" },
		{ "sample-ca65.s", "", "6502 (ca65)" },
		{ "sample-64tass.a65", "", "6502 (64tass)" },
	};
	const char *update = getenv("MT_UPDATE_GOLDEN");
	bool updating = update && update[0] == '1';

	for (const Sample &sample : samples)
	{
		std::string dir = ResolveProjectPath("tests/data/source-debug/lexer");
		std::string path = dir + "/" + sample.file;
		std::string goldenPath = path.substr(0, path.find_last_of('.')) + ".tokens";

		CSourceDocument document;
		const CSyntaxLexer *lexer = SyntaxLexerForPath(path, sample.producer);
		if (!Check(document.Load(path) && lexer && std::string(lexer->GetName()) == sample.lexer,
				   std::string("Lexer: ") + sample.file + " loads and gets the " + sample.lexer + " lexer"))
			return false;

		std::string tokens = DumpTokens(lexer, document);
		if (updating)
			WriteText(goldenPath, tokens);

		std::ifstream golden(goldenPath, std::ios::in | std::ios::binary);
		std::stringstream expected;
		expected << golden.rdbuf();
		bool same = golden && expected.str() == tokens;
		if (!same)
			WriteText("/tmp/source-document-" + std::string(sample.file) + ".tokens", tokens);
		if (!Check(same, std::string("Lexer: ") + sample.file + " tokens match " + goldenPath.substr(goldenPath.find("tests/"))
				   + (same ? "" : " (actual in /tmp/source-document-" + std::string(sample.file) + ".tokens)")))
			return false;
	}
	return true;
}

bool CTestSourceDocument::RunView()
{
#ifdef RUN_COMMODORE64
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	CViewSourceCode *view = viewC64->viewC64SourceCode;
	if (!di || !view)
	{
		ReportRequiredGap("C64 source view not created");
		return true;
	}
	if (view->visible)
	{
		ReportRequiredGap("C64 Source code view is open: the test swaps its symbols");
		return true;
	}

	// scratch symbols with the Oscar64 basic-flow debug info, as in SourceStepping
	CDebugSymbols *scratch = new CDebugSymbolsC64(di);
	scratch->SetDataAdapter(di->dataAdapterC64);
	scratch->CreateDefaultSegment();
	std::string dir = ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow");
	DebugInfoFormat format = DebugInfoLoadFile(scratch, (dir + "/main.dbj").c_str());

	CDebugSymbols *liveSymbols = view->debugSymbols;
	view->debugSymbols = scratch;
	SourceViewFollow follow = view->follow;
	std::vector<std::string> roots = view->documents.sourceRoots;

	bool ok = Check(format == DebugInfoFormat::Oscar64Dbj, "Source view: the Oscar64 basic-flow main.dbj loads into scratch symbols");

	// main.c is not next to the .dbj: a source root finds it
	const CDebugSourceFile *file = ok ? scratch->debugDatabase->Files().begin()->second : NULL;
	if (ok)
	{
		bool missingFirst = !view->documents.Open(file->originalPath, file->loadedPath);
		view->documents.sourceRoots.push_back(ResolveProjectPath("tests/source-debug/fixtures/basic-flow"));
		auto document = view->documents.Open(file->originalPath, file->loadedPath);
		ok = Check(missingFirst && document && document->GetLine(16) == "    result = 1;",
				   "Source view: main.c is missing at its recorded path and found through a source root");
	}

	// the disassembly's "Show source": $0885 is line 17
	ok = ok && Check(view->ShowAddress(0x0885) && view->GetSelectedLine() == 17 && view->follow == SourceViewFollow::Off
					 && !view->ShowAddress(0xC000),
					 "Source view: Show source of $0885 selects main.c:17 and stops following; an unmapped address has no line");

	// double-click navigation targets
	if (ok)
	{
		std::vector<u32> line12 = view->AddressesOfLine(12);
		std::vector<u32> line19 = view->AddressesOfLine(19);
		ok = Check(line12.size() == 1 && line12[0] == 0x089A && line19.empty(),
				   "Source view: line 12 has code at $089A only, line 19's loop has none");
	}

	// gutter clicks: a source breakpoint on a header line binds to the body,
	// a second click removes it; a user's address breakpoint is removed, not
	// shadowed by a new source breakpoint
	if (ok)
	{
		CDebugSymbolsSegment *segment = scratch->currentSegment;
		view->ToggleBreakpointAtLine(11);
		bool added = scratch->sourceBreakpoints.size() == 1 && scratch->sourceBreakpoints[0]->resolvedLine == 12
			&& segment->breakpointsPC->GetBreakpoint(0x089A) != NULL;
		view->ToggleBreakpointAtLine(12);
		bool removed = scratch->sourceBreakpoints.empty() && segment->breakpointsPC->GetBreakpoint(0x089A) == NULL;
		segment->AddBreakpointPC(0x0880);
		view->ToggleBreakpointAtLine(16);
		bool userRemoved = scratch->sourceBreakpoints.empty() && segment->breakpointsPC->GetBreakpoint(0x0880) == NULL;
		ok = Check(added && removed && userRemoved,
				   "Source view: a gutter click on line 11 adds a source breakpoint bound at line 12 ($089A), a click on 12 removes it, a click on a user's breakpoint line removes that");
	}

	view->debugSymbols = liveSymbols;
	view->follow = follow;
	view->documents.sourceRoots = roots;
	view->documents.Clear();
	// scratch leaked on purpose, see CTestSymbolLoaders
	return ok;
#else
	ReportRequiredGap("C64 emulation not compiled in");
	return true;
#endif
}

void CTestSourceDocument::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = RunDocument() && RunRegistry() && RunLexers() && RunView();
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Source documents, the registry, the C and 6502 lexers and the source view's navigation and gutter behave as specified");
}
