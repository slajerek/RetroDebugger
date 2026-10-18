#include "CWorkspaceSession.h"
#include "CBuildTask.h"
#include "CViewC64.h"
#include "CMainMenuHelper.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "CSourceDocument.h"
#include "CDebugSymbols.h"
#include "CDebugInfoLoader.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "EmulatorsConfig.h"
#include <algorithm>
#include <cstring>
#include <filesystem>

const char *CWorkspaceSession::kStalePrefix = "Build failed";

CDebugInterface *CWorkspaceSession::Machine(const std::string &machine)
{
#ifdef RUN_ATARI
	if (machine == "atari")
		return viewC64->debugInterfaceAtari;
#endif
#ifdef RUN_NES
	if (machine == "nes")
		return viewC64->debugInterfaceNes;
#endif
#ifdef RUN_COMMODORE64
	if (machine.empty() || machine == "c64")
		return viewC64->debugInterfaceC64;
#endif
	return NULL;
}

static std::string Extension(const std::string &path)
{
	std::string ext = SourcePathString(SourcePath(path).extension());
	for (char &c : ext)
		c = (char)tolower((unsigned char)c);
	return ext;
}

bool CWorkspaceSession::LoadBuild(const CBuildTaskRun &run, std::string *error)
{
	const CBuildTaskConfig &task = run.expanded;
	CDebugInterface *di = Machine(task.machine);
	if (di == NULL)
	{
		if (error)
			*error = "no " + (task.machine.empty() ? std::string("c64") : task.machine) + " emulator";
		return false;
	}
	bool runIt = task.afterBuild == CBuildTaskConfig::AfterBuild::LoadAndRun;
	if (!task.program.empty())
	{
		if (!di->isRunning)
			viewC64->StartEmulationThread(di);
		CSlrString *path = new CSlrString(task.program.c_str());
		std::string ext = Extension(task.program);
		bool loaded = true;
		if (ext == ".prg")
			loaded = viewC64->mainMenuHelper->LoadPRG(path, runIt, false, false, false);
		else if (ext == ".xex")
			loaded = viewC64->mainMenuHelper->LoadXEX(path, runIt, false, false);
		else if (ext == ".nes")
			loaded = viewC64->mainMenuHelper->LoadNES(path, false);
		else if (ext == ".crt")
			loaded = viewC64->mainMenuHelper->InsertCartridge(path, false);
		else if (ext == ".car" || ext == ".rom")
			loaded = viewC64->mainMenuHelper->InsertAtariCartridge(path, runIt, false, false);
		else
			viewC64->mainMenuHelper->LoadFile(path);
		delete path;
		if (!loaded)
		{
			if (error)
				*error = "cannot load " + task.program;
			return false;
		}
	}
	if (!task.debugInfo.empty())
	{
		DebugInfoFormat format = DebugInfoLoadFile(di->symbols, task.debugInfo.c_str());
		if (format == DebugInfoFormat::Unknown)
		{
			if (error)
				*error = "cannot load the debug info " + task.debugInfo;
			return false;
		}
		// the program and its debug info come from this build: check both
		di->symbols->debugInfoWarnings = DebugInfoCheckStale(di->symbols->debugDatabase.get(), task.debugInfo.c_str(),
															 task.program.empty() ? NULL : task.program.c_str());
	}
	return true;
}

void CWorkspaceSession::MarkStale(const CBuildTaskRun &run)
{
	CDebugInterface *di = Machine(run.expanded.machine);
	if (di == NULL || di->symbols == NULL)
		return;
	std::string warning = std::string(kStalePrefix) + " (" + run.expanded.name + ": " + run.GetFailure()
		+ "): the loaded program and debug info are from an earlier build";
	guiMain->LockMutex();
	std::vector<std::string> &warnings = di->symbols->debugInfoWarnings;
	warnings.erase(std::remove_if(warnings.begin(), warnings.end(), [](const std::string &w) {
		return w.compare(0, strlen(kStalePrefix), kStalePrefix) == 0;
	}), warnings.end());
	warnings.push_back(warning);
	guiMain->UnlockMutex();
}
