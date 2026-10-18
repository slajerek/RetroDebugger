#pragma once

#include "CTest.h"
#include <string>

// RD-018..021: the project (a folder as a workspace). In a scratch folder:
// the listing skips build and VCS directories, the optional config gives
// build tasks; files open as tabs (one per file) and switch; a buffer edits,
// undoes, redoes and saves; the watcher reloads a clean tab and flags a
// dirty one; compiler diagnostics parse and open their file at the line; a
// build waits for the user's consent; a failed build leaves the loaded
// session marked stale, a successful one loads its program and debug info.
class CTestWorkspace : public CTest
{
public:
	virtual const char *GetName() override { return "Workspace"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
};
