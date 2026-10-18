#ifndef _CViewWorkspace_h_
#define _CViewWorkspace_h_

// RD-018..021: the "Project" window -- a folder opened as a workspace (spec
// #10): the explorer, the open files as tabs, an editor per tab and the
// build console. A tab shows its file highlighted, with the PC line and the
// source breakpoint gutter of the workspace's machine when its debug info
// maps the file; Edit switches to plain text editing (undo/redo, save).
// Open files are watched: an external change reloads a clean tab and flags
// a dirty one. A build runs the workspace's task (asking first, once per
// set of commands) and loads its outputs, or marks the session stale.

#include "CGuiView.h"
#include "CSystemFileDialogCallback.h"
#include "CWorkspace.h"
#include "CBuildTask.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class CSourceBuffer;
class CDebugInterface;

class CViewWorkspace : public CGuiView, public CSystemFileDialogCallback
{
public:
	CViewWorkspace(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewWorkspace();

	virtual void RenderImGui();

	CWorkspace workspace;

	bool OpenFolder(const std::string &path);
	void CloseFolder();
	// a tab for PATH (the existing one when open), selected; LINE > 0 scrolls there
	CSourceBuffer *OpenFile(const std::string &path, int line = 0);
	struct Tab
	{
		std::unique_ptr<CSourceBuffer> buffer;
		bool editing = false;
		int scrollToLine = 0;
		int selectedLine = 0;
		std::string notice;			// "Reloaded: changed on disk", ...
	};
	std::vector<Tab> tabs;
	int activeTab = -1;
	int NumTabs() const { return (int)tabs.size(); }
	CSourceBuffer *GetActiveBuffer();
	void SelectTab(int index);
	// a dirty tab closes only with DISCARD
	bool CloseTab(int index, bool discard);
	bool SaveTab(int index, std::string *error = NULL);

	// the file watcher's poll, every kWatchIntervalMs from RenderImGui; tests call it
	void PollFiles();
	static const u64 kWatchIntervalMs = 1000;

	// RD-019/020: TASK of the workspace; without CONSENT an untrusted
	// workspace asks first (false: not started, the question is open)
	CBuildTaskRun build;
	bool StartBuild(int task, bool consent);
	// the finished build's effect, once: load its outputs or mark stale
	void PollBuild();
	std::string lastBuildResult;
	bool OpenDiagnostic(const CBuildDiagnostic &diagnostic);

	// the machine the workspace targets (its debug info decorates the tabs)
	CDebugInterface *GetMachine();

	virtual void SystemDialogPickFolderSelected(CSlrString *path);
	virtual void SystemDialogPickFolderCancelled();

private:
	std::map<std::string, std::vector<CWorkspaceEntry>> listings;	// the explorer's cache
	char filter[128];
	char findText[128];
	bool findCaseSensitive;
	long findOffset;
	bool showFind;
	int selectedTask;
	u32 handledBuildGeneration;
	u64 lastWatchMs;
	int consentTask;			// the task waiting for the user's yes, -1
	int pendingCloseTab;		// a dirty tab waiting for discard / cancel, -1
	std::string editText;		// the editor widget's buffer
	int editTab;
	int lastRenderedTab = -1;	// the tab bar's selection a frame ago: a change of activeTab selects
	bool scrollConsole;

	void RenderToolbar();
	void RenderExplorer();
	void RenderDirectory(const std::string &directory);
	void RenderTabs();
	void RenderEditor(Tab &tab);
	void RenderView(Tab &tab);
	void RenderConsole();
	void RenderPopups();
	const std::vector<CWorkspaceEntry> &Listing(const std::string &directory);
	void FindNext(Tab &tab);
	int PcLineFor(const CSourceBuffer *buffer);
};

#endif
