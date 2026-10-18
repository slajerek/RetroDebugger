#include "CBuildTask.h"
#include "CSourceDocument.h"
#include "CProcessSpawner.h"
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <regex>

std::string CBuildTaskRun::Expand(const std::string &text, const std::map<std::string, std::string> &variables)
{
	std::string out;
	size_t pos = 0;
	while (pos < text.size())
	{
		size_t open = text.find("${", pos);
		if (open == std::string::npos)
		{
			out += text.substr(pos);
			break;
		}
		size_t close = text.find('}', open);
		if (close == std::string::npos)
		{
			out += text.substr(pos);
			break;
		}
		out += text.substr(pos, open - pos);
		std::string name = text.substr(open + 2, close - open - 2);
		auto it = variables.find(name);
		if (it != variables.end())
			out += it->second;
		else if (name.compare(0, 4, "env:") == 0)
		{
			const char *value = getenv(name.substr(4).c_str());
			out += value ? value : "";
		}
		else if (name.compare(0, 5, "tool:") == 0)
			out += name.substr(5);		// no configured path: the tool by name, on PATH
		else
			out += "${" + name + "}";	// unknown: left as written
		pos = close + 1;
	}
	return out;
}

static std::string ResolvePath(const std::string &file, const std::string &cwd)
{
	namespace fs = std::filesystem;
	std::error_code error;
	fs::path p = SourcePath(file);
	if (p.is_relative() && !cwd.empty())
		p = SourcePath(cwd) / p;
	if (!fs::exists(p, error))
		return "";
	fs::path canonical = fs::weakly_canonical(p, error);
	return error ? SourcePathString(p) : SourcePathString(canonical);
}

std::vector<CBuildDiagnostic> BuildParseDiagnostics(const std::vector<std::string> &output, const std::string &cwd)
{
	// Oscar64:     main.c(12, 5) : error 3001: Could not open source file.
	// cc65/ca65:   main.c(12): Error: ';' expected
	// ld65:        ld65: Error: main.cfg(4): Missing memory area
	// gcc-style:   main.c:12:5: error: expected ';'
	// KickAss:     Error: Unknown symbol 'x'  /  at line 12, column 5 in main.asm
	static const std::regex oscar64("^(.+?)\\((\\d+), *(\\d+)\\) *: *(error|warning|info)[^:]*: *(.*)$", std::regex::icase);
	static const std::regex cc65("^(.+?)\\((\\d+)\\): *(Error|Warning|Note): *(.*)$", std::regex::icase);
	static const std::regex ld65("^[a-z0-9]+: *(Error|Warning): *(.+?)\\((\\d+)\\): *(.*)$", std::regex::icase);
	static const std::regex gcc("^(.+?):(\\d+):(?:(\\d+):)? *(error|warning|note|fatal error): *(.*)$", std::regex::icase);
	static const std::regex kickassAt("^ *at line (\\d+), column (\\d+) in (.+?) *$", std::regex::icase);
	static const std::regex kickassError("^ *(Error|Warning): *(.*)$", std::regex::icase);

	auto severityOf = [](std::string s) {
		for (char &c : s)
			c = (char)tolower((unsigned char)c);
		if (s.find("warning") != std::string::npos)
			return CBuildDiagnostic::Severity::Warning;
		if (s.find("note") != std::string::npos || s.find("info") != std::string::npos)
			return CBuildDiagnostic::Severity::Note;
		return CBuildDiagnostic::Severity::Error;
	};

	std::vector<CBuildDiagnostic> diagnostics;
	std::string pendingKickAssMessage;
	CBuildDiagnostic::Severity pendingKickAssSeverity = CBuildDiagnostic::Severity::Error;
	for (size_t i = 0; i < output.size(); i++)
	{
		const std::string &line = output[i];
		std::smatch m;
		CBuildDiagnostic d;
		d.outputLine = (int)i;
		if (std::regex_match(line, m, oscar64))
		{
			d.file = m[1];
			d.line = atoi(m[2].str().c_str());
			d.column = atoi(m[3].str().c_str());
			d.severity = severityOf(m[4]);
			d.message = m[5];
		}
		else if (std::regex_match(line, m, cc65))
		{
			d.file = m[1];
			d.line = atoi(m[2].str().c_str());
			d.severity = severityOf(m[3]);
			d.message = m[4];
		}
		else if (std::regex_match(line, m, ld65))
		{
			d.severity = severityOf(m[1]);
			d.file = m[2];
			d.line = atoi(m[3].str().c_str());
			d.message = m[4];
		}
		else if (std::regex_match(line, m, gcc))
		{
			d.file = m[1];
			d.line = atoi(m[2].str().c_str());
			d.column = m[3].matched ? atoi(m[3].str().c_str()) : 0;
			d.severity = severityOf(m[4]);
			d.message = m[5];
		}
		else if (std::regex_match(line, m, kickassAt))
		{
			d.line = atoi(m[1].str().c_str());
			d.column = atoi(m[2].str().c_str());
			d.file = m[3];
			d.severity = pendingKickAssSeverity;
			d.message = pendingKickAssMessage.empty() ? "error" : pendingKickAssMessage;
			pendingKickAssMessage.clear();
		}
		else
		{
			if (std::regex_match(line, m, kickassError))
			{
				pendingKickAssSeverity = severityOf(m[1]);
				pendingKickAssMessage = m[2];
			}
			continue;
		}
		if (d.line <= 0)
			continue;
		d.path = ResolvePath(d.file, cwd);
		diagnostics.push_back(d);
	}
	return diagnostics;
}

CBuildTaskRun::~CBuildTaskRun()
{
	Cancel();
	Wait();
}

void CBuildTaskRun::Start(const CBuildTaskConfig &config, const std::map<std::string, std::string> &variables)
{
	Cancel();
	Wait();
	expanded = config;
	for (std::vector<std::string> &step : expanded.steps)
		for (std::string &arg : step)
			arg = Expand(arg, variables);
	for (std::string &e : expanded.env)
		e = Expand(e, variables);
	auto it = variables.find("workspace");
	std::string root = it != variables.end() ? it->second : "";
	expanded.cwd = config.cwd.empty() ? root : Expand(config.cwd, variables);
	auto absolute = [&](const std::string &file) -> std::string {
		if (file.empty())
			return file;
		std::filesystem::path p = SourcePath(Expand(file, variables));
		if (p.is_relative())
			p = SourcePath(expanded.cwd) / p;
		return SourcePathString(p.lexically_normal());
	};
	expanded.program = absolute(config.program);
	expanded.debugInfo = absolute(config.debugInfo);
	{
		std::lock_guard<std::mutex> lock(mutex);
		output.clear();
		diagnostics.clear();
		failure.clear();
		exitCode = 0;
	}
	generation++;
	cancel.store(false);
	state.store(State::Running);
	worker = std::thread([this]() { Run(); });
}

void CBuildTaskRun::Cancel()
{
	cancel.store(true);
}

void CBuildTaskRun::Wait()
{
	if (worker.joinable())
		worker.join();
}

// one output chunk, split into lines; diagnostics of the lines since DIAGNOSTICSFROM
void CBuildTaskRun::AddOutput(const std::string &text, size_t *diagnosticsFrom)
{
	std::lock_guard<std::mutex> lock(mutex);
	size_t pos = 0;
	while (pos < text.size())
	{
		size_t end = text.find('\n', pos);
		std::string line = text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		output.push_back(line);
		if (end == std::string::npos)
			break;
		pos = end + 1;
	}
	std::vector<std::string> tail(output.begin() + *diagnosticsFrom, output.end());
	for (CBuildDiagnostic d : BuildParseDiagnostics(tail, expanded.cwd))
	{
		d.outputLine += (int)*diagnosticsFrom;
		diagnostics.push_back(d);
	}
	*diagnosticsFrom = output.size();
}

void CBuildTaskRun::Run()
{
	size_t diagnosticsFrom = 0;
	for (const std::vector<std::string> &step : expanded.steps)
	{
		if (step.empty())
			continue;
		std::string command;
		for (const std::string &arg : step)
			command += (command.empty() ? "" : " ") + arg;
		{
			// the command as run: shown, never parsed (its arguments may look like diagnostics)
			std::lock_guard<std::mutex> lock(mutex);
			output.push_back("> " + command);
			diagnosticsFrom = output.size();
		}

		CProcessSpawnOptions options;
		options.workingDirectory = expanded.cwd;
		options.envVars = expanded.env;
		options.searchPath = true;
		CProcessRunResult result;
		bool exited = CProcessSpawner::Run(step, options, result, expanded.timeoutMs, &cancel);
		AddOutput(result.stdOut, &diagnosticsFrom);
		AddOutput(result.stdErr, &diagnosticsFrom);

		std::lock_guard<std::mutex> lock(mutex);
		if (!exited || result.exitCode != 0)
		{
			exitCode = result.exitCode;
			if (result.cancelled)
			{
				failure = "cancelled";
				state.store(State::Cancelled);
				return;
			}
			if (!result.started)
				failure = step[0] + " did not start: " + result.error;
			else if (result.timedOut)
				failure = step[0] + " timed out after " + std::to_string(expanded.timeoutMs / 1000) + " s";
			else
				failure = step[0] + " exited with " + std::to_string(result.exitCode);
			state.store(State::Failed);
			return;
		}
	}
	// the outputs must exist: a tool that exits 0 without writing them failed too
	std::error_code error;
	for (const std::string &file : { expanded.program, expanded.debugInfo })
	{
		if (!file.empty() && !std::filesystem::exists(SourcePath(file), error))
		{
			std::lock_guard<std::mutex> lock(mutex);
			failure = "the build wrote no " + file;
			state.store(State::Failed);
			return;
		}
	}
	state.store(State::Succeeded);
}

std::string CBuildTaskRun::GetFailure() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return failure;
}

std::vector<std::string> CBuildTaskRun::GetOutput() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return output;
}

std::vector<CBuildDiagnostic> CBuildTaskRun::GetDiagnostics() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return diagnostics;
}
