#include "SYS_Main.h"
#include "EmulatorsConfig.h"
#include "CDebuggerGuiControl.h"
#include "CDebuggerServer.h"
#include "GuiControlRequest.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include <memory>
#include <mutex>

#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
// windows.h exports a legacy Yield() macro, not an ImGui operation.
#ifdef Yield
#undef Yield
#endif
#include "CImGuiTestEngine.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "imgui_te_internal.h"
#include "imgui_internal.h"
#include "GuiControlFrameGate.h"
#include "GuiControlOperation.h"
#endif

using namespace std;
using namespace nlohmann;

namespace GuiControl
{
#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
namespace
{
std::mutex guiOperationMutex;
std::shared_ptr<Operation> guiOperation;
// UI-only references: pending startup waits for the test engine's frame hook;
// an active operation remains held through the test engine's teardown.
std::shared_ptr<Operation> guiPendingStartup;
std::shared_ptr<Operation> guiUiOperation;
uint64_t guiNextId = 0;

// Restored by PostFrame only AFTER queue cleanup.  Restoring this from
// teardown would be too early: engine cleanup yields for three more frames.
bool guiSavedRestoreFocus = true;
}

void Operation::Finish(const char *outcome, const char *error)
{
	std::lock_guard<std::mutex> lock(guiOperationMutex);
	finished = true;
	terminal = !engineQueued;
	result["outcome"] = outcome;
	if (error) result["error"] = error;
}

bool Operation::Continue(ImGuiTestContext *ctx)
{
	if (ctx->IsError()) { Finish("failed", "widget_action_failed"); return false; }
	if (GuiClock::now() >= deadline)
	{
		Finish("failed", "action_deadline_exceeded_partial_effects_possible");
		return false;
	}
	return true;
}

void Operation::SetImage(const json &image)
{
	std::lock_guard<std::mutex> lock(guiOperationMutex);
	result["image"] = image;
}

namespace
{
class GuiStartOperation : public CUiThreadTaskCallback
{
public:
	std::shared_ptr<Operation> op;
	GuiStartOperation(std::shared_ptr<Operation> op) : op(op) {}
	void RunUIThreadTask() override
	{
		// CGuiMain removes callbacks but does not delete them.
		std::unique_ptr<GuiStartOperation> deleteAfterRun(this);
		Start(op);
	}
	static void Start(const std::shared_ptr<Operation> &op)
	{
		{
			std::lock_guard<std::mutex> lock(guiOperationMutex);
			if (op->terminal) return;
			if (GuiClock::now() >= op->deadline)
			{ op->terminal = true; op->result["outcome"] = "expired"; return; }
		}
		if (!gHeadlessMode && ImGui::GetCurrentContext()) CImGuiTestEngine::Init();
		auto *engine = CImGuiTestEngine::GetEngine();
		if (gHeadlessMode || !engine || !ImGui::GetCurrentContext())
		{ op->Finish("failed", "interactive_gui_unavailable"); return; }
		const int engineFrame = ImGuiTestEngine_GetFrameCount(engine);
		const int guiFrame = ImGui::GetFrameCount();
		if (!GuiControl::TestEngineFrameReady(engineFrame, guiFrame))
		{
			// Do not queue in the same frame as lazy initialization: the engine's
			// missing-signal assertion remains enabled.  PostFrame retries under
			// the original deadline, after the next NewFrame hook can run.
			guiPendingStartup = op;
			return;
		}
		if (!CImGuiTestEngine::IsTestQueueEmpty())
		{ op->Finish("failed", "imgui_test_engine_busy"); return; }
		{
			std::lock_guard<std::mutex> lock(guiOperationMutex);
			// Status may have expired this request while startup was in progress.
			if (op->terminal) return;
			if (GuiClock::now() >= op->deadline)
			{ op->terminal = true; op->result["outcome"] = "expired"; return; }
			op->started = true;
			op->engineQueued = true;
			op->result["outcome"] = "running";
		}
		LOGM("MCP GUI request %s: queue on synchronized frame %d", op->id.c_str(), guiFrame);
		static ImGuiTest *test = nullptr;
		if (!test) test = IM_REGISTER_TEST(engine, "mcp", "memory_breakpoints_action");
		guiUiOperation = op;
		guiSavedRestoreFocus = ImGuiTestEngine_GetIO(engine).ConfigRestoreFocusAfterTests;
		ImGuiTestEngine_GetIO(engine).ConfigRestoreFocusAfterTests = false;
		test->TestFunc = [](ImGuiTestContext *ctx) { RunMemoryBreakpoints(ctx, *guiUiOperation); };
		test->TeardownFunc = [](ImGuiTestContext *ctx)
		{
			auto op = guiUiOperation;
			std::lock_guard<std::mutex> lock(guiOperationMutex);
			if (!op->finished)
			{
				op->finished = true;
				op->result["outcome"] = "failed";
				op->result["error"] = ctx->IsError() ? "widget_action_failed" : "action_interrupted";
			}
		};
		ImGuiTestEngine_QueueTest(engine, test);
	}
};
}
#endif
}

void GuiControl::PostFrame()
{
#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
	if (guiUiOperation && CImGuiTestEngine::IsTestQueueEmpty())
	{
		ImGuiTestEngine_GetIO(CImGuiTestEngine::GetEngine()).ConfigRestoreFocusAfterTests = guiSavedRestoreFocus;
		std::lock_guard<std::mutex> lock(guiOperationMutex);
		guiUiOperation->terminal = true;
		guiUiOperation.reset();
	}
	if (guiPendingStartup)
	{
		auto pending = guiPendingStartup;
		guiPendingStartup.reset();
		GuiStartOperation::Start(pending);
	}
#endif
}

void GuiControl::RegisterEndpoint(CDebuggerServer *server)
{
	EndpointDescriptor desc;
	desc.fn = "gui/control";
	desc.category = "gui";
	desc.description = "Allowlisted interactive C64 Memory Breakpoints panel/editor; queued actions require status";
	desc.paramsSchema = GuiControl::Schema();
	server->AddEndpointFunction(desc, [server](const string token, json args, unsigned char *, int) -> vector<char>*
	{
		GuiControl::Request request;
		if (!GuiControl::Parse(args, request))
			return server->PrepareResult(HTTP_BAD_REQUEST, token, {{"error", "invalid_gui_arguments"}}, nullptr, 0);
		bool available = false;
		const char *reason = "imgui_test_support_disabled";
#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
		// The driver is initialized lazily on the UI thread by the first action.
		available = !gHeadlessMode && guiMain;
		reason = gHeadlessMode ? "headless" : "interactive_gui_unavailable";
#elif MT_ENABLE_IMGUI_TEST_ENGINE
		reason = "native_c64_support_disabled";
#endif
#ifdef MODE_HEADLESS
		available = false;
		reason = "gui_support_disabled";
#endif
		if (request.action == GuiControl::Action::Capabilities)
			return server->PrepareResult(HTTP_OK, token, {{"available", available},
				{"reason", available ? "" : reason}, {"targets", json::array({"c64.memory_breakpoints", "c64.memory_breakpoints.editor"})}}, nullptr, 0);
		if (!available)
			return server->PrepareResult(HTTP_SERVICE_UNAVAILABLE, token, {{"error", reason}}, nullptr, 0);
#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
		std::shared_ptr<Operation> op;
		{
			std::lock_guard<std::mutex> lock(guiOperationMutex);
			// Expiration is cancellation BEFORE UI execution.  The queued callback
			// retains its own record and cannot execute a replacement operation.
			if (guiOperation && !guiOperation->terminal && !guiOperation->started && GuiClock::now() >= guiOperation->deadline)
			{ guiOperation->terminal = true; guiOperation->result["outcome"] = "expired"; }
			if (request.action == GuiControl::Action::Status)
			{
				if (!guiOperation || guiOperation->id != request.requestId)
					return server->PrepareResult(HTTP_NOT_FOUND, token, {{"error", "unknown_gui_request"}}, nullptr, 0);
				json result = guiOperation->result;
				if (!guiOperation->terminal) result = {{"requestId", guiOperation->id}, {"outcome", guiOperation->started ? "running" : "queued"}};
				result["deadlineExceeded"] = GuiClock::now() >= guiOperation->deadline;
				return server->PrepareResult(HTTP_OK, token, result, nullptr, 0);
			}
			if (guiOperation && !guiOperation->terminal)
				return server->PrepareResult(HTTP_SERVICE_UNAVAILABLE, token, {{"error", "gui_busy"}, {"requestId", guiOperation->id}}, nullptr, 0);
			op = std::make_shared<Operation>();
			op->id = std::to_string(++guiNextId);
			op->request = request;
			op->deadline = GuiClock::now() + std::chrono::seconds(5);
			op->result = {{"requestId", op->id}, {"outcome", "queued"}};
			guiOperation = op;
		}
		guiMain->AddUiThreadTask(new GuiStartOperation(op));
		// Do not read op->result here: the UI may already be writing it.
		return server->PrepareResult(HTTP_OK, token, {{"requestId", op->id}, {"outcome", "queued"}}, nullptr, 0);
#else
		return nullptr; // Unreachable after the availability refusal.
#endif
	});
}
