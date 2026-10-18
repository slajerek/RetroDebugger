#ifndef _CBuildTask_h_
#define _CBuildTask_h_

// RD-019: build tasks of a workspace. A task is a list of steps -- argv
// vectors, no shell -- run one after another in the workspace (or its own
// cwd) through the engine's CProcessSpawner::Run on a worker thread, with a
// time limit and cancel. Its output is kept line by line and parsed into
// diagnostics (file, line, column, severity, message) by the compilers'
// formats: Oscar64, cc65/ca65/ld65, KickAssembler, gcc-style. On success the
// task's outputs (program, debug info) are what the workspace loads.
//
// Variables in arguments, cwd and outputs: ${workspace} (the root), ${env:NAME},
// ${tool:NAME} (a user tool path, CWorkspaceTools; the bare name -- searched
// on PATH -- when none is configured).

#include "SYS_Defs.h"
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct CBuildDiagnostic
{
	enum class Severity { Error, Warning, Note };
	Severity severity = Severity::Error;
	std::string file;		// as the tool printed it; resolved against the cwd
	std::string path;		// absolute when the file exists
	int line = 0;
	int column = 0;
	std::string message;
	int outputLine = 0;		// index into the task's output lines
};

// the diagnostics in OUTPUT (one tool's lines), resolved against CWD
std::vector<CBuildDiagnostic> BuildParseDiagnostics(const std::vector<std::string> &output, const std::string &cwd);

struct CBuildTaskConfig
{
	std::string name;
	std::vector<std::vector<std::string>> steps;
	std::string cwd;					// "" = the workspace root
	std::vector<std::string> env;		// KEY=VALUE
	std::string machine;				// c64, atari, nes: where the outputs load
	std::string program;				// output: the program or cartridge
	std::string debugInfo;				// output: the debug info
	enum class AfterBuild { None, Load, LoadAndRun };
	AfterBuild afterBuild = AfterBuild::Load;
	int timeoutMs = 120000;
};

class CBuildTaskRun
{
public:
	enum class State { Idle, Running, Succeeded, Failed, Cancelled };

	// starts CONFIG on a worker thread; VARIABLES maps "workspace", "tool:NAME", ...
	void Start(const CBuildTaskConfig &config, const std::map<std::string, std::string> &variables);
	void Cancel();
	void Wait();						// for tests and shutdown
	~CBuildTaskRun();

	State GetState() const { return state.load(); }
	bool IsRunning() const { return state.load() == State::Running; }
	int GetExitCode() const { return exitCode; }
	std::string GetFailure() const;		// why it failed, one sentence
	std::vector<std::string> GetOutput() const;
	std::vector<CBuildDiagnostic> GetDiagnostics() const;
	// the outputs with variables expanded, valid once started
	CBuildTaskConfig expanded;
	u32 generation = 0;					// increases with every Start

	static std::string Expand(const std::string &text, const std::map<std::string, std::string> &variables);

private:
	std::atomic<State> state { State::Idle };
	std::atomic<bool> cancel { false };
	std::thread worker;
	mutable std::mutex mutex;
	std::vector<std::string> output;
	std::vector<CBuildDiagnostic> diagnostics;
	std::string failure;
	int exitCode = 0;
	void Run();
	void AddOutput(const std::string &text, size_t *diagnosticsFrom);
};

#endif
