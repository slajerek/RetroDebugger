#include "CTestWorkspace.h"
#include "CSourceDocument.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CViewWorkspace.h"
#include "CWorkspace.h"
#include "CWorkspaceSession.h"
#include "CBuildTask.h"
#include "CSourceBuffer.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

bool CTestWorkspace::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

static void WriteFile(const fs::path &path, const std::string &text)
{
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	out << text;
}

static std::string ReadFile(const fs::path &path)
{
	std::ifstream in(path, std::ios::binary);
	std::stringstream text;
	text << in.rdbuf();
	return text.str();
}

// a later modification time than the buffer saw: the watcher's cue
static void Touch(const fs::path &path)
{
	std::error_code ec;
	fs::last_write_time(path, fs::file_time_type::clock::now() + std::chrono::seconds(10), ec);
}

static bool WaitForBuild(CBuildTaskRun &run, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs && run.IsRunning(); waited += 10)
		SYS_Sleep(10);
	return !run.IsRunning();
}

// one shell command as a build step, per platform
static std::vector<std::string> Shell(const std::string &posix, const std::string &windows)
{
#ifdef _WIN32
	return { "cmd.exe", "/c", windows };
#else
	return { "/bin/sh", "-c", posix };
#endif
}

void CTestWorkspace::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();
	bool ok = true;

	std::error_code ec;
	fs::path root = fs::temp_directory_path(ec) / "rd-workspace-test";
	fs::remove_all(root, ec);
	fs::create_directories(root / "sub", ec);
	fs::create_directories(root / "build", ec);
	fs::create_directories(root / ".git", ec);
	WriteFile(root / "main.c", "int main(void) {\n    return 0;\n}\n");
	WriteFile(root / "sub" / "inc.h", "#define X 1\n");
	WriteFile(root / "build" / "junk.o", "");
	WriteFile(root / ".hidden", "");
	std::string program = ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow/main.prg");
	std::string debugInfo = ResolveProjectPath("tests/data/source-debug/oscar64/c64/basic-flow/main.dbj");
	WriteFile(root / CWorkspace::kConfigFileName,
		"{\n  machine: c64\n  buildTasks: [\n    { \"name\": \"Fake\", \"steps\": [[\"" + std::string(
#ifdef _WIN32
		"cmd.exe\", \"/c\", \"echo built"
#else
		"/bin/sh\", \"-c\", \"echo built"
#endif
		) + "\"]], \"afterBuild\": \"none\" }\n  ]\n}\n");

	// --- the folder ---
	CViewWorkspace view("ProjectTest", 0, 0, -1, 400, 300);
	CWorkspace &ws = view.workspace;
	bool opened = view.OpenFolder(SourcePathString(root));
	ok = Check(opened && ws.configError.empty() && ws.machine == "c64"
			   && ws.buildTasks.size() == 1 && ws.buildTasks[0].name == "Fake",
			   "Workspace: a folder opens; its config gives the machine and a build task (" + std::string(opened ? "opened" : "not opened")
			   + ", machine '" + ws.machine + "', " + std::to_string(ws.buildTasks.size()) + " tasks" + (ws.configError.empty() ? "" : ", " + ws.configError) + ")");
	if (ok)
	{
		std::vector<CWorkspaceEntry> top = ws.List(ws.root);
		std::string names;
		for (const CWorkspaceEntry &e : top)
			names += (names.empty() ? "" : " ") + e.name + (e.isDirectory ? "/" : "");
		std::vector<CWorkspaceEntry> found = ws.FindFiles("INC");
		ok = Check(names == std::string("sub/ main.c ") + CWorkspace::kConfigFileName && found.size() == 1 && found[0].name == "inc.h"
				   && !ws.Contains(SourcePathString((root / ".."))),
				   "Workspace: the listing has directories first, no build/VCS/hidden entries (" + names + "); file search is case-blind");
	}

	// --- tabs ---
	CSourceBuffer *mainBuffer = NULL, *incBuffer = NULL;
	if (ok)
	{
		mainBuffer = view.OpenFile(SourcePathString((root / "main.c")));
		incBuffer = view.OpenFile(SourcePathString((root / "sub" / "inc.h")));
		bool second = view.NumTabs() == 2 && view.GetActiveBuffer() == incBuffer;
		view.SelectTab(0);
		bool switched = view.GetActiveBuffer() == mainBuffer;
		CSourceBuffer *again = view.OpenFile(SourcePathString((root / "sub" / "inc.h")), 1);
		ok = Check(mainBuffer && incBuffer && second && switched && again == incBuffer && view.NumTabs() == 2
				   && view.GetActiveBuffer() == incBuffer,
				   "Workspace: two files are two tabs; tabs switch; opening an open file selects its tab");
	}

	// --- editing ---
	if (ok)
	{
		std::string original = mainBuffer->GetText();
		mainBuffer->SetText("int main(void) {\n    return 1;\n}\n", 1000);
		mainBuffer->SetText("int main(void) {\n    return 12;\n}\n", 1100);		// the same burst
		bool dirty = mainBuffer->IsDirty();
		bool undone = mainBuffer->Undo() && mainBuffer->GetText() == original && !mainBuffer->IsDirty();
		bool redone = mainBuffer->Redo() && mainBuffer->GetText().find("return 12;") != std::string::npos;
		std::string error;
		bool saved = view.SaveTab(0, &error) && !mainBuffer->IsDirty() && ReadFile(root / "main.c") == mainBuffer->GetText();
		ok = Check(dirty && undone && redone && saved,
				   "Workspace: an edit burst is one undo step; undo, redo and save (the file has the text, the tab is clean)");
	}

	// --- the watcher ---
	if (ok)
	{
		WriteFile(root / "sub" / "inc.h", "#define X 2\n");
		Touch(root / "sub" / "inc.h");
		mainBuffer->SetText("int main(void) { return 3; }\n", 5000);
		WriteFile(root / "main.c", "/* changed outside */\n");
		Touch(root / "main.c");
		view.PollFiles();
		bool reloaded = incBuffer->GetText() == "#define X 2\n" && !incBuffer->IsDirty() && !view.tabs[1].notice.empty();
		bool conflict = mainBuffer->IsConflicting() && mainBuffer->GetText() == "int main(void) { return 3; }\n";
		bool reverted = mainBuffer->Revert() && mainBuffer->GetText() == "/* changed outside */\n" && !mainBuffer->IsConflicting();
		mainBuffer->SetText("int main(void) { return 4; }\n", 9000);
		bool refused = !view.CloseTab(0, false) && view.NumTabs() == 2;
		bool discarded = view.CloseTab(0, true) && view.NumTabs() == 1;
		ok = Check(reloaded && conflict && reverted && refused && discarded,
				   "Workspace: a clean tab reloads a file changed on disk, a dirty one keeps its edits and flags the conflict; "
				   "a dirty tab closes only when discarded");
		mainBuffer = NULL;
	}

	// --- diagnostics ---
	if (ok)
	{
		std::vector<std::string> output = {
			"main.c(2, 5) : error 3001: Could not open source file.",
			"main.c(2): Warning: Unused variable",
			"main.c:3:1: error: expected ';'",
			"Error: Unknown symbol 'foo'",
			"at line 2, column 3 in main.c",
			"ld65: Error: missing.cfg(4): Missing memory area",
			"nothing to see here",
		};
		std::vector<CBuildDiagnostic> d = BuildParseDiagnostics(output, SourcePathString(root));
		bool parsed = d.size() == 5
			&& d[0].line == 2 && d[0].column == 5 && d[0].severity == CBuildDiagnostic::Severity::Error && d[0].message == "Could not open source file."
			&& d[1].line == 2 && d[1].severity == CBuildDiagnostic::Severity::Warning
			&& d[2].line == 3 && d[2].column == 1
			&& d[3].line == 2 && d[3].column == 3 && d[3].message == "Unknown symbol 'foo'" && d[3].outputLine == 4
			&& d[4].file == "missing.cfg" && d[4].path.empty()
			&& !d[0].path.empty() && fs::equivalent(SourcePath(d[0].path), root / "main.c", ec);
		bool opened = parsed && view.OpenDiagnostic(d[2]) && view.GetActiveBuffer()
			&& SourceDocumentFileName(view.GetActiveBuffer()->path) == "main.c" && view.tabs[view.activeTab].selectedLine == 3
			&& !view.OpenDiagnostic(d[4]);
		std::string got;
		for (const CBuildDiagnostic &x : d)
			got += " [" + x.file + ":" + std::to_string(x.line) + ":" + std::to_string(x.column) + " " + std::to_string((int)x.severity)
				+ " '" + x.message + "' @" + std::to_string(x.outputLine) + (x.path.empty() ? "" : " path") + "]";
		ok = Check(parsed && opened,
				   "Workspace: Oscar64, cc65, gcc-style and KickAssembler diagnostics parse; one opens its file at its line, one without a file does not ("
				   + std::string(parsed ? "parsed" : "not parsed") + got + ")");
	}

	// --- consent ---
	CWorkspaceTrust::settingsFileOverride = SourcePathString((root / "trust.hjson"));
	if (ok)
	{
		bool asked = !view.StartBuild(0, false) && !view.build.IsRunning();
		bool started = view.StartBuild(0, true) && WaitForBuild(view.build, 10000)
			&& view.build.GetState() == CBuildTaskRun::State::Succeeded;
		bool trusted = CWorkspaceTrust::IsTrusted(ws);
		ws.buildTasks[0].steps[0].push_back("--more");
		bool editedAsksAgain = !CWorkspaceTrust::IsTrusted(ws);
		ws.buildTasks[0].steps[0].pop_back();
		std::vector<std::string> out = view.build.GetOutput();
		bool echoed = false;
		for (const std::string &line : out)
			echoed = echoed || line == "built";
		ok = Check(asked && started && trusted && editedAsksAgain && echoed,
				   "Workspace: a build of an untrusted folder waits for consent, then runs (its output kept); edited commands ask again");
	}

	// --- the session: failed build stale, successful build loads ---
#ifdef RUN_COMMODORE64
	CDebugInterface *di = viewC64->debugInterfaceC64;
	if (ok && di)
	{
		bool wasRunning = di->isRunning;
		if (!wasRunning)
		{
			viewC64->StartEmulationThread(di);
			SYS_Sleep(2000);
		}
		CDebugSymbols *liveSymbols = di->symbols;
		CDebugSymbols *scratch = new CDebugSymbolsC64(di);
		scratch->SetDataAdapter(liveSymbols->dataAdapter);
		scratch->CreateDefaultSegment();
		guiMain->LockMutex();
		di->LockMutex();
		di->symbols = scratch;
		di->UnlockMutex();
		guiMain->UnlockMutex();
		DebugInfoLoadFile(scratch, debugInfo.c_str());
		std::shared_ptr<const CDebugDatabase> before = scratch->debugDatabase;

		CBuildTaskConfig failing;
		failing.name = "Broken";
		failing.machine = "c64";
		failing.steps = { Shell("echo 'main.c:2:5: error: boom'; exit 2", "echo main.c:2:5: error: boom & exit /b 2") };
		failing.program = "out/main.prg";
		failing.debugInfo = "out/main.dbj";
		ws.buildTasks = { failing };
		CWorkspaceTrust::SetTrusted(ws, true);
		bool startedFailing = view.StartBuild(0, false) && WaitForBuild(view.build, 10000);
		view.PollBuild();
		std::vector<CBuildDiagnostic> diagnostics = view.build.GetDiagnostics();
		bool stale = false;
		for (const std::string &w : scratch->debugInfoWarnings)
			stale = stale || w.compare(0, strlen(CWorkspaceSession::kStalePrefix), CWorkspaceSession::kStalePrefix) == 0;
		ok = Check(startedFailing && view.build.GetState() == CBuildTaskRun::State::Failed && view.build.GetExitCode() == 2
				   && diagnostics.size() == 1 && diagnostics[0].line == 2 && stale && scratch->debugDatabase == before,
				   "Workspace: a failed build (exit 2, one diagnostic) keeps the loaded debug info and marks it stale");

		if (ok)
		{
			CBuildTaskConfig working = failing;
			working.name = "Copy";
			working.afterBuild = CBuildTaskConfig::AfterBuild::Load;
#ifdef _WIN32
			// separate steps: in cmd, "if ... && copy" would skip the copies once out exists
			working.steps = { { "cmd.exe", "/c", "if not exist out mkdir out" },
							  { "cmd.exe", "/c", "copy /y \"" + SourcePathString(SourcePath(program).make_preferred()) + "\" out\\main.prg" },
							  { "cmd.exe", "/c", "copy /y \"" + SourcePathString(SourcePath(debugInfo).make_preferred()) + "\" out\\main.dbj" } };
#else
			working.steps = { Shell("mkdir -p out && cp '" + program + "' out/main.prg && cp '" + debugInfo + "' out/main.dbj", "") };
#endif
			ws.buildTasks = { working };
			CWorkspaceTrust::SetTrusted(ws, true);
			bool built = view.StartBuild(0, false) && WaitForBuild(view.build, 20000)
				&& view.build.GetState() == CBuildTaskRun::State::Succeeded;
			view.PollBuild();
			bool cleared = true;
			for (const std::string &w : scratch->debugInfoWarnings)
				cleared = cleared && w.compare(0, strlen(CWorkspaceSession::kStalePrefix), CWorkspaceSession::kStalePrefix) != 0;
			ok = Check(built && scratch->debugDatabase && scratch->debugDatabase != before && cleared
					   && view.lastBuildResult.find("loaded") != std::string::npos,
					   "Workspace: a successful build loads its program and debug info together; the stale mark is gone (" + view.lastBuildResult + ")");
		}

		guiMain->LockMutex();
		di->LockMutex();
		di->symbols = liveSymbols;
		di->UnlockMutex();
		guiMain->UnlockMutex();
		// scratch leaked on purpose, see CTestSymbolLoaders
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		if (!wasRunning)
			viewC64->StopEmulationThread(di);
	}
#endif
	CWorkspaceTrust::settingsFileOverride.clear();
	view.CloseFolder();
	fs::remove_all(root, ec);

	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "A folder as a project: listing, tabs, editing, watcher, diagnostics, consent, builds and the session");
}
