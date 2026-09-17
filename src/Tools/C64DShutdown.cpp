#include "C64DShutdown.h"

#include "CMCPServer.h"
#include "DBG_Log.h"
#include "SYS_Main.h"
#include "SYS_Threading.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>

// _exit(): <unistd.h> on POSIX, declared by <stdlib.h>/<process.h> on MSVC.
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <io.h>
#endif

using namespace nlohmann;

static int ClampInt(int value, int minValue, int maxValue)
{
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;
	return value;
}

//
// C64DShutdownRequest
//

C64DShutdownRequest::C64DShutdownRequest()
{
	force = false;
	timeoutMs = -1;
	graceMs = C64D_SHUTDOWN_GRACE_DEFAULT_MS;
}

void C64DShutdownRequest::Normalize()
{
	// A negative deadline means the caller did not pick one, so it comes from
	// `force`. Anything the caller did pick is kept -- force is about urgency,
	// not about overruling an explicit number.
	if (timeoutMs < 0)
	{
		timeoutMs = force ? C64D_SHUTDOWN_TIMEOUT_FORCED_MS : C64D_SHUTDOWN_TIMEOUT_DEFAULT_MS;
	}

	timeoutMs = ClampInt(timeoutMs, C64D_SHUTDOWN_TIMEOUT_MIN_MS, C64D_SHUTDOWN_TIMEOUT_MAX_MS);
	graceMs = ClampInt(graceMs, C64D_SHUTDOWN_GRACE_MIN_MS, C64D_SHUTDOWN_GRACE_MAX_MS);
}

C64DShutdownRequest C64DParseShutdownParams(const json &params, const char *defaultReason)
{
	C64DShutdownRequest request;
	request.reason = (defaultReason != NULL) ? defaultReason : "";

	// Everything here is defensive on purpose: these params arrive from a
	// remote client, and a malformed field must fall back to the default
	// rather than throw out of an endpoint handler.
	if (params.is_object())
	{
		if (params.contains("force") && params["force"].is_boolean())
		{
			request.force = params["force"].get<bool>();
		}

		if (params.contains("timeoutMs") && params["timeoutMs"].is_number_integer())
		{
			request.timeoutMs = params["timeoutMs"].get<int>();
		}

		if (params.contains("graceMs") && params["graceMs"].is_number_integer())
		{
			request.graceMs = params["graceMs"].get<int>();
		}

		if (params.contains("reason") && params["reason"].is_string())
		{
			std::string reason = params["reason"].get<std::string>();
			if (!reason.empty())
			{
				request.reason = reason;
			}
		}
	}

	request.Normalize();
	return request;
}

json C64DShutdownRequestToJson(const C64DShutdownRequest &request)
{
	json result;
	result["status"] = "shutting_down";
	result["force"] = request.force;
	result["timeoutMs"] = request.timeoutMs;
	result["graceMs"] = request.graceMs;
	if (!request.reason.empty())
	{
		result["reason"] = request.reason;
	}
	return result;
}

//
// Scheduling
//

static void C64DDefaultShutdownExecutor(const C64DShutdownRequest &request);

static std::atomic<bool> gShutdownPending(false);
static C64DShutdownExecutorFn gShutdownExecutor = C64DDefaultShutdownExecutor;

static void C64DDefaultShutdownExecutor(const C64DShutdownRequest &request)
{
	LOGM("C64DShutdown: shutting down (force=%d timeoutMs=%d reason=%s)",
		 (int)request.force, request.timeoutMs, request.reason.c_str());

	// Stop the MCP JSON-RPC loop first, so no further tool call is dispatched
	// into an application that is already on its way out.
	MCP_ServerStop();

	// The File -> Quit path. The render loop unwinds on the main thread and
	// saves the ImGui ini, the layouts and the plugin config before _exit(0).
	SYS_Shutdown();

	// Watchdog. If the graceful path does not reach that _exit in time -- a
	// wedged emulator thread, a blocked save, or a headless render loop parked
	// in nextDrawable -- take the process down here instead of leaving an
	// orphan behind for the caller to kill out of band. That orphan is the
	// whole reason this endpoint exists, so failing open is not an option.
	SYS_Sleep(request.timeoutMs);

	// Deliberately no LOGError/LOG_Shutdown/fflush() here, however tempting.
	// The thread we are rescuing is usually stuck in the engine's own shutdown
	// tail (LOG_Shutdown(); fflush(NULL); _exit(0)), and fflush(NULL) walks
	// every open stream taking its lock -- so anything here that touches stdio
	// would queue up behind the exact deadlock we are here to break. Measured:
	// in headless mode the graceful path reaches its own _exit() in ~0.3s and
	// then never returns from fflush(NULL), leaving the orphan behind.
	//
	// write(2) is a raw syscall and takes no stdio lock; _exit(2) likewise.
	// The MCP reply was already flushed by CMCPServer::WriteMessage, so there
	// is nothing of the caller's that we are dropping on the floor.
	static const char kForcedMsg[] = "C64DShutdown: graceful shutdown timed out, forcing exit\n";
#ifdef WIN32
	_write(2, kForcedMsg, (unsigned int)(sizeof(kForcedMsg) - 1));
#else
	ssize_t ignored = write(2, kForcedMsg, sizeof(kForcedMsg) - 1);
	(void)ignored;
#endif
	_exit(0);
}

// Runs the shutdown off the caller's thread so the endpoint handler can return
// its reply first. The object is deliberately never freed: on the default path
// this thread ends the process, and a request is only granted once.
class CShutdownThread : public CSlrThread
{
public:
	C64DShutdownRequest request;

	virtual void ThreadRun(void *passData)
	{
		ThreadSetName("C64DShutdown");

		// Give the caller's reply -- a websocket frame, or an MCP JSON-RPC
		// response on stdout -- time to be written before we quit under it.
		if (request.graceMs > 0)
		{
			SYS_Sleep(request.graceMs);
		}

		C64DShutdownExecutorFn executor = gShutdownExecutor;
		if (executor != NULL)
		{
			executor(request);
		}
	}
};

bool C64DRequestShutdown(const C64DShutdownRequest &request)
{
	if (gShutdownPending.exchange(true))
	{
		LOGWarning("C64DRequestShutdown: shutdown already pending, ignoring request (reason=%s)",
				   request.reason.c_str());
		return false;
	}

	CShutdownThread *thread = new CShutdownThread();
	thread->request = request;
	thread->request.Normalize();

	LOGD("C64DRequestShutdown: scheduled (force=%d timeoutMs=%d graceMs=%d reason=%s)",
		 (int)thread->request.force, thread->request.timeoutMs, thread->request.graceMs,
		 thread->request.reason.c_str());

	SYS_StartThread(thread);
	return true;
}

bool C64DIsShutdownPending()
{
	return gShutdownPending.load();
}

void C64DSetShutdownExecutor(C64DShutdownExecutorFn executor)
{
	gShutdownExecutor = (executor != NULL) ? executor : C64DDefaultShutdownExecutor;

	// Swapping the executor re-arms the process lifecycle: a request granted
	// against the previous executor must not block the next one.
	gShutdownPending.store(false);
}
