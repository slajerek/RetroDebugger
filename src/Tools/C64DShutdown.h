#ifndef _C64DShutdown_h_
#define _C64DShutdown_h_

// Graceful application shutdown, requestable from a remote client.
//
// SYS_Shutdown() only sets mtQuitApplication; the render loop then unwinds on
// the main thread and does the work that actually matters -- ImGui ini save,
// layout serialization, MT_Shutdown() (plugin config), SYS_ApplicationShutdown()
// -- before _exit(0). That is the File -> Quit path, and it is what a remote
// shutdown request must go through too.
//
// Two things a bare SYS_Shutdown() call does not give a remote caller:
//
//   1. A reply. The websocket/MCP request is synchronous, so quitting inside
//      the handler drops the connection before the response is written and the
//      client sees a transport error instead of a result. Hence graceMs.
//
//   2. A guarantee. If the graceful path wedges -- a blocked save, a stuck
//      emulator thread, or a headless render loop parked in nextDrawable --
//      the process never exits and the caller is left with an orphan to kill
//      out-of-band. Hence the watchdog deadline, timeoutMs.

#include "json.hpp"
#include <string>

// Watchdog deadline for a normal shutdown request.
#define C64D_SHUTDOWN_TIMEOUT_DEFAULT_MS	8000
// Watchdog deadline when the caller asks for a forced shutdown. Note this only
// shortens the leash -- a forced shutdown still runs the full graceful path and
// still saves layouts, settings and plugin state.
#define C64D_SHUTDOWN_TIMEOUT_FORCED_MS		1500
// How long to wait before quitting, so the caller's reply gets flushed first.
#define C64D_SHUTDOWN_GRACE_DEFAULT_MS		300

#define C64D_SHUTDOWN_TIMEOUT_MIN_MS		100
#define C64D_SHUTDOWN_TIMEOUT_MAX_MS		120000
#define C64D_SHUTDOWN_GRACE_MIN_MS			0
#define C64D_SHUTDOWN_GRACE_MAX_MS			5000

class C64DShutdownRequest
{
public:
	C64DShutdownRequest();

	// Resolve timeoutMs from force when it was left unset, and clamp every
	// value into range. Idempotent.
	void Normalize();

	bool force;
	int timeoutMs;			// watchdog deadline; < 0 means "derive from force"
	int graceMs;			// delay before the shutdown is triggered
	std::string reason;		// free-form, for the log ("mcp:retro_shutdown", ...)
};

// Parse the { force, timeoutMs, graceMs, reason } vocabulary shared by the
// server/shutdown endpoint and the retro_shutdown MCP tool. Fields that are
// missing or of the wrong type fall back to defaults. The result is normalized.
C64DShutdownRequest C64DParseShutdownParams(const nlohmann::json &params, const char *defaultReason);

// The JSON payload endpoints and tools reply with.
nlohmann::json C64DShutdownRequestToJson(const C64DShutdownRequest &request);

// Schedule application shutdown and return immediately. Returns false when a
// shutdown is already pending, in which case the request is ignored.
bool C64DRequestShutdown(const C64DShutdownRequest &request);

bool C64DIsShutdownPending();

// Test seam: replaces the terminal shutdown actions (MCP stop, SYS_Shutdown,
// watchdog). Passing NULL restores the default executor. Installing or clearing
// an executor also clears the pending flag -- the process lifecycle is being
// re-armed, so a previous request must not block the next one.
typedef void (*C64DShutdownExecutorFn)(const C64DShutdownRequest &request);
void C64DSetShutdownExecutor(C64DShutdownExecutorFn executor);

#endif
//_C64DShutdown_h_
