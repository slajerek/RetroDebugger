#include "CWorkspace.h"
#include "CSourceDocument.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "hjson.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

const char *CWorkspace::kConfigFileName = "retrodebugger-workspace.hjson";
const char *CWorkspaceTrust::kSettingsFileName = "WorkspaceTrust.hjson";
std::string CWorkspaceTrust::settingsFileOverride;

static std::string Lower(std::string s)
{
	for (char &c : s)
		c = (char)tolower((unsigned char)c);
	return s;
}

static bool IsSkippedDirectory(const std::string &name)
{
	static const char *skipped[] = { ".git", ".svn", ".hg", "node_modules", "build", "build-macos", "build-linux",
									 "Build", "DerivedData", ".vs", ".idea", ".vscode", "__pycache__" };
	for (const char *s : skipped)
		if (name == s)
			return true;
	return false;
}

bool CWorkspace::Open(const std::string &path, std::string *error)
{
	std::error_code ec;
	fs::path p = SourcePath(path);
	if (!fs::is_directory(p, ec))
	{
		if (error)
			*error = path + " is not a folder";
		return false;
	}
	Close();
	root = SourcePathString(fs::weakly_canonical(p, ec));
	if (ec)
		root = SourcePathString(p);
	LoadConfig();
	return true;
}

void CWorkspace::Close()
{
	root.clear();
	configError.clear();
	sourceRoots.clear();
	buildTasks.clear();
	machine.clear();
}

bool CWorkspace::Contains(const std::string &path) const
{
	if (root.empty())
		return false;
	std::error_code ec;
	fs::path p = fs::weakly_canonical(SourcePath(path), ec);
	if (ec)
		return false;
	std::string s = SourcePathString(p), r = root;
	return s == r || (s.size() > r.size() && s.compare(0, r.size(), r) == 0 && (s[r.size()] == '/' || s[r.size()] == '\\'));
}

std::string CWorkspace::RelativePath(const std::string &path) const
{
	std::error_code ec;
	fs::path rel = fs::relative(SourcePath(path), SourcePath(root), ec);
	return ec ? path : rel.generic_string();
}

std::vector<CWorkspaceEntry> CWorkspace::List(const std::string &directory, const std::string &filter) const
{
	std::vector<CWorkspaceEntry> entries;
	if (!Contains(directory))
		return entries;
	std::error_code ec;
	std::string needle = Lower(filter);
	for (fs::directory_iterator it(SourcePath(directory), ec), end; !ec && it != end; it.increment(ec))
	{
		std::string name = SourcePathString(it->path().filename());
		if (name.empty() || name[0] == '.')
			continue;
		CWorkspaceEntry entry;
		entry.name = name;
		entry.path = SourcePathString(it->path());
		entry.isDirectory = it->is_directory(ec);
		if (entry.isDirectory && IsSkippedDirectory(name))
			continue;
		if (!entry.isDirectory && !needle.empty() && Lower(name).find(needle) == std::string::npos)
			continue;
		entries.push_back(entry);
	}
	std::sort(entries.begin(), entries.end(), [](const CWorkspaceEntry &a, const CWorkspaceEntry &b) {
		if (a.isDirectory != b.isDirectory)
			return a.isDirectory;
		return Lower(a.name) < Lower(b.name);
	});
	return entries;
}

std::vector<CWorkspaceEntry> CWorkspace::FindFiles(const std::string &filter, size_t maxFiles) const
{
	std::vector<CWorkspaceEntry> found;
	if (root.empty())
		return found;
	std::string needle = Lower(filter);
	std::function<void(const std::string &)> walk = [&](const std::string &directory) {
		for (const CWorkspaceEntry &entry : List(directory))
		{
			if (found.size() >= maxFiles)
				return;
			if (entry.isDirectory)
				walk(entry.path);
			else if (needle.empty() || Lower(entry.name).find(needle) != std::string::npos)
				found.push_back(entry);
		}
	};
	walk(root);
	return found;
}

static std::vector<std::string> StringList(const Hjson::Value &value)
{
	std::vector<std::string> out;
	if (value.type() != Hjson::Type::Vector)
		return out;
	for (int i = 0; i < (int)value.size(); i++)
		if (value[i].type() == Hjson::Type::String)
			out.push_back(value[i].to_string());
	return out;
}

bool CWorkspace::LoadConfig()
{
	configError.clear();
	sourceRoots.clear();
	buildTasks.clear();
	machine.clear();
	fs::path file = SourcePath(root) / kConfigFileName;
	std::ifstream in(file, std::ios::in | std::ios::binary);
	if (!in)
		return true;		// a folder without a config is a workspace too
	std::stringstream text;
	text << in.rdbuf();
	Hjson::Value config;
	try
	{
		config = Hjson::Unmarshal(text.str());
	}
	catch (const std::exception &e)
	{
		configError = std::string(kConfigFileName) + ": " + e.what();
		return false;
	}
	if (config.type() != Hjson::Type::Map)
	{
		configError = std::string(kConfigFileName) + ": not an object";
		return false;
	}
	if (config["machine"].type() == Hjson::Type::String)
		machine = config["machine"].to_string();
	for (const std::string &sourceRoot : StringList(config["sourceRoots"]))
	{
		fs::path p = SourcePath(sourceRoot);
		sourceRoots.push_back(SourcePathString((p.is_relative() ? SourcePath(root) / p : p).lexically_normal()));
	}
	Hjson::Value tasks = config["buildTasks"];
	if (tasks.type() == Hjson::Type::Vector)
	{
		for (int i = 0; i < (int)tasks.size(); i++)
		{
			const Hjson::Value task = tasks[i];
			if (task.type() != Hjson::Type::Map)
				continue;
			CBuildTaskConfig t;
			t.name = task["name"].type() == Hjson::Type::String ? task["name"].to_string() : "Build";
			Hjson::Value steps = task["steps"];
			if (steps.type() == Hjson::Type::Vector)
			{
				for (int s = 0; s < (int)steps.size(); s++)
				{
					std::vector<std::string> argv = StringList(steps[s]);
					if (!argv.empty())
						t.steps.push_back(argv);
				}
			}
			t.cwd = task["cwd"].type() == Hjson::Type::String ? task["cwd"].to_string() : "";
			t.env = StringList(task["env"]);
			t.machine = task["machine"].type() == Hjson::Type::String ? task["machine"].to_string() : machine;
			t.program = task["program"].type() == Hjson::Type::String ? task["program"].to_string() : "";
			t.debugInfo = task["debugInfo"].type() == Hjson::Type::String ? task["debugInfo"].to_string() : "";
			std::string after = task["afterBuild"].type() == Hjson::Type::String ? task["afterBuild"].to_string() : "load";
			t.afterBuild = after == "none" ? CBuildTaskConfig::AfterBuild::None
				: after == "run" ? CBuildTaskConfig::AfterBuild::LoadAndRun : CBuildTaskConfig::AfterBuild::Load;
			if (task["timeout"].type() == Hjson::Type::Int64 || task["timeout"].type() == Hjson::Type::Double)
				t.timeoutMs = (int)(task["timeout"].to_double() * 1000.0);
			if (!t.steps.empty())
				buildTasks.push_back(t);
		}
	}
	return true;
}

bool CWorkspace::SaveConfig(std::string *error)
{
	Hjson::Value config(Hjson::Type::Map);
	if (!machine.empty())
		config["machine"] = machine;
	Hjson::Value roots(Hjson::Type::Vector);
	for (const std::string &sourceRoot : sourceRoots)
		roots.push_back(RelativePath(sourceRoot));
	config["sourceRoots"] = roots;
	Hjson::Value tasks(Hjson::Type::Vector);
	for (const CBuildTaskConfig &t : buildTasks)
	{
		Hjson::Value task(Hjson::Type::Map);
		task["name"] = t.name;
		Hjson::Value steps(Hjson::Type::Vector);
		for (const std::vector<std::string> &argv : t.steps)
		{
			Hjson::Value step(Hjson::Type::Vector);
			for (const std::string &arg : argv)
				step.push_back(arg);
			steps.push_back(step);
		}
		task["steps"] = steps;
		if (!t.cwd.empty())
			task["cwd"] = t.cwd;
		if (!t.machine.empty())
			task["machine"] = t.machine;
		if (!t.program.empty())
			task["program"] = t.program;
		if (!t.debugInfo.empty())
			task["debugInfo"] = t.debugInfo;
		task["afterBuild"] = t.afterBuild == CBuildTaskConfig::AfterBuild::None ? "none"
			: t.afterBuild == CBuildTaskConfig::AfterBuild::LoadAndRun ? "run" : "load";
		tasks.push_back(task);
	}
	config["buildTasks"] = tasks;
	std::ofstream out(SourcePath(root) / kConfigFileName, std::ios::out | std::ios::binary | std::ios::trunc);
	out << Hjson::Marshal(config);
	if (!out && error)
		*error = std::string("cannot write ") + kConfigFileName;
	return (bool)out;
}

std::map<std::string, std::string> CWorkspace::Variables() const
{
	std::map<std::string, std::string> variables;
	variables["workspace"] = root;
	for (auto &it : CWorkspaceTools::Load())
		variables["tool:" + it.first] = it.second;
	return variables;
}

// --- the settings folder ---

static bool LoadSettingsMap(const char *fileName, std::map<std::string, std::string> &out)
{
	CByteBuffer *buffer = new CByteBuffer();
	CSlrString *name = new CSlrString(fileName);
	bool loaded = buffer->loadFromSettings(name);
	delete name;
	if (loaded && buffer->length > 0)
	{
		try
		{
			Hjson::Value map = Hjson::Unmarshal(std::string((const char *)buffer->data, (size_t)buffer->length));
			if (map.type() == Hjson::Type::Map)
				for (auto it = map.begin(); it != map.end(); ++it)
					if (it->second.type() == Hjson::Type::String)
						out[it->first] = it->second.to_string();
		}
		catch (const std::exception &)
		{
			loaded = false;
		}
	}
	delete buffer;
	return loaded;
}

static bool SaveSettingsMap(const char *fileName, const std::map<std::string, std::string> &values)
{
	Hjson::Value map(Hjson::Type::Map);
	for (auto &it : values)
		map[it.first] = it.second;
	std::string text = Hjson::Marshal(map);
	CByteBuffer *buffer = new CByteBuffer();
	buffer->PutBytes((u8 *)text.data(), (int)text.size());
	CSlrString *name = new CSlrString(fileName);
	bool saved = buffer->storeToSettings(name);
	delete name;
	delete buffer;
	return saved;
}

std::map<std::string, std::string> CWorkspaceTools::Load()
{
	std::map<std::string, std::string> tools;
	LoadSettingsMap("WorkspaceTools.hjson", tools);
	return tools;
}

bool CWorkspaceTools::Save(const std::map<std::string, std::string> &tools)
{
	return SaveSettingsMap("WorkspaceTools.hjson", tools);
}

std::string CWorkspaceTrust::Fingerprint(const CWorkspace &workspace)
{
	// the commands themselves: an edited task is a new request
	std::string all;
	for (const CBuildTaskConfig &t : workspace.buildTasks)
	{
		all += t.name + "\x1f" + t.cwd + "\x1f";
		for (const std::vector<std::string> &argv : t.steps)
			for (const std::string &arg : argv)
				all += arg + "\x1e";
		for (const std::string &e : t.env)
			all += e + "\x1d";
	}
	u64 hash = 1469598103934665603ULL;
	for (unsigned char c : all)
		hash = (hash ^ c) * 1099511628211ULL;
	char hex[20];
	snprintf(hex, sizeof(hex), "%016llx", (unsigned long long)hash);
	return hex;
}

static std::map<std::string, std::string> LoadTrust()
{
	std::map<std::string, std::string> trust;
	if (!CWorkspaceTrust::settingsFileOverride.empty())
	{
		std::ifstream in(CWorkspaceTrust::settingsFileOverride, std::ios::binary);
		std::stringstream text;
		text << in.rdbuf();
		try
		{
			Hjson::Value map = Hjson::Unmarshal(text.str());
			if (map.type() == Hjson::Type::Map)
				for (auto it = map.begin(); it != map.end(); ++it)
					trust[it->first] = it->second.to_string();
		}
		catch (const std::exception &)
		{
		}
		return trust;
	}
	LoadSettingsMap(CWorkspaceTrust::kSettingsFileName, trust);
	return trust;
}

static void SaveTrust(const std::map<std::string, std::string> &trust)
{
	if (!CWorkspaceTrust::settingsFileOverride.empty())
	{
		Hjson::Value map(Hjson::Type::Map);
		for (auto &it : trust)
			map[it.first] = it.second;
		std::ofstream out(CWorkspaceTrust::settingsFileOverride, std::ios::binary | std::ios::trunc);
		out << Hjson::Marshal(map);
		return;
	}
	SaveSettingsMap(CWorkspaceTrust::kSettingsFileName, trust);
}

bool CWorkspaceTrust::IsTrusted(const CWorkspace &workspace)
{
	std::map<std::string, std::string> trust = LoadTrust();
	auto it = trust.find(workspace.root);
	return it != trust.end() && it->second == Fingerprint(workspace);
}

void CWorkspaceTrust::SetTrusted(const CWorkspace &workspace, bool trusted)
{
	std::map<std::string, std::string> trust = LoadTrust();
	if (trusted)
		trust[workspace.root] = Fingerprint(workspace);
	else
		trust.erase(workspace.root);
	SaveTrust(trust);
}
