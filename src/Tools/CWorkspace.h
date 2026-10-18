#ifndef _CWorkspace_h_
#define _CWorkspace_h_

// RD-018/019/020: a workspace is a folder (spec #10.1). Opening one needs no
// project file; an optional retrodebugger-workspace.hjson in the root adds
// source roots, build tasks and the target machine. The folder's files are
// listed lazily, directory by directory, skipping build and VCS clutter.
//
// User-specific things never live in the workspace (it may be under version
// control): tool paths (${tool:NAME}) and which folders the user trusted to
// run their build commands (spec #10.4: nothing runs without consent) are
// kept in the app's settings folder.

#include "SYS_Defs.h"
#include "CBuildTask.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

struct CWorkspaceEntry
{
	std::string name;
	std::string path;			// absolute
	bool isDirectory = false;
};

class CWorkspace
{
public:
	static const char *kConfigFileName;	// "retrodebugger-workspace.hjson"

	// ROOT must be a directory; false (and ERROR) otherwise
	bool Open(const std::string &root, std::string *error = NULL);
	bool IsOpen() const { return !root.empty(); }
	void Close();

	std::string root;
	std::string configError;					// the config file did not parse
	std::vector<std::string> sourceRoots;		// absolute
	std::vector<CBuildTaskConfig> buildTasks;
	std::string machine;						// the default target, "" = c64

	// the entries of DIRECTORY (a path inside the root), directories first,
	// both by name; hidden files and build/VCS directories left out; FILTER
	// (case-insensitive substring, may be empty) keeps matching files and
	// every directory
	std::vector<CWorkspaceEntry> List(const std::string &directory, const std::string &filter = "") const;
	// every file below the root matching FILTER, at most MAXFILES (the
	// explorer's search)
	std::vector<CWorkspaceEntry> FindFiles(const std::string &filter, size_t maxFiles = 500) const;
	// PATH inside the root (no "..", no symlink out of it)
	bool Contains(const std::string &path) const;
	std::string RelativePath(const std::string &path) const;

	// writes buildTasks, sourceRoots and machine to the config file
	bool SaveConfig(std::string *error = NULL);
	bool LoadConfig();

	// the variables of a task run: workspace, and tool:NAME for configured tools
	std::map<std::string, std::string> Variables() const;
};

// the user's tool paths (${tool:NAME}), in the settings folder
class CWorkspaceTools
{
public:
	static std::map<std::string, std::string> Load();
	static bool Save(const std::map<std::string, std::string> &tools);
};

// folders whose build commands the user allowed to run, in the settings
// folder; a workspace's tasks change its fingerprint, so edited tasks ask again
class CWorkspaceTrust
{
public:
	static std::string Fingerprint(const CWorkspace &workspace);
	static bool IsTrusted(const CWorkspace &workspace);
	static void SetTrusted(const CWorkspace &workspace, bool trusted);
	static const char *kSettingsFileName;
	static std::string settingsFileOverride;	// tests: a file of their own
};

#endif
