#include "CTestNesCartridge.h"
#include "EmulatorsConfig.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceTask.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <atomic>
#include <memory>
#include <string>

#ifdef RUN_NES
#include "NesWrapper.h"
#include "CDebugInterfaceNes.h"

namespace
{
	struct TaskState
	{
		std::atomic<bool> done { false };
		std::atomic<bool> ok { false };
	};

	class CTaskNesCartridge : public CDebugInterfaceTask
	{
	public:
		CTaskNesCartridge(CDebugInterface *di, const char *path, bool pauseAfter, std::shared_ptr<TaskState> state)
		: di(di), path(path ? path : ""), insert(path != NULL), pauseAfter(pauseAfter), state(state) {}

		virtual void ExecuteTask() override
		{
			bool ok = insert ? nesd_insert_cartridge((char *)path.c_str()) : nesd_unload_cartridge();
			if (ok)
			{
				// what the app's insert task does after a load
				di->ResetEmulationFrameCounter();
				((CDebugInterfaceNes *)di)->ResetClockCounters();
				di->ClearHistory();
			}
			if (pauseAfter)
				di->SetDebugMode(DEBUGGER_MODE_PAUSED);
			state->ok.store(ok);
			state->done.store(true);
		}

	private:
		CDebugInterface *di;
		std::string path;
		bool insert;
		bool pauseAfter;
		std::shared_ptr<TaskState> state;
	};

	bool RunTask(CDebugInterface *di, const char *path, bool pauseAfter, int timeoutMs)
	{
		std::shared_ptr<TaskState> state = std::make_shared<TaskState>();
		di->AddVSyncTask(new CTaskNesCartridge(di, path, pauseAfter, state));
		if (di->GetDebugMode() != DEBUGGER_MODE_RUNNING)
			di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		for (int waited = 0; waited < timeoutMs && !state->done.load(); waited += 10)
			SYS_Sleep(10);
		if (!state->done.load())
			return false;
		// the pause lands at the end of the frame the task ran in
		for (int waited = 0; pauseAfter && waited < 2000 && di->GetDebugMode() != DEBUGGER_MODE_PAUSED; waited += 10)
			SYS_Sleep(10);
		return state->ok.load();
	}
}

bool TestNesInsertCartridge(CDebugInterface *di, const char *path, bool pauseAfter, int timeoutMs)
{
	return RunTask(di, path, pauseAfter, timeoutMs);
}

bool TestNesUnloadCartridge(CDebugInterface *di, bool pauseAfter, int timeoutMs)
{
	return RunTask(di, NULL, pauseAfter, timeoutMs);
}

#else

bool TestNesInsertCartridge(CDebugInterface *di, const char *path, bool pauseAfter, int timeoutMs) { return false; }
bool TestNesUnloadCartridge(CDebugInterface *di, bool pauseAfter, int timeoutMs) { return false; }

#endif
