#include "SYS_Main.h"
#include "SYS_FileSystem.h"
#include "CDebuggerServerApi.h"
#include "CDebuggerServer.h"
#include "CDebugInterface.h"
#include "CSnapshotsManager.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointsData.h"
#include "CDebugBreakpointAddr.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDataAdapter.h"
#include "CByteBuffer.h"
#include "DebuggerDefs.h"
#include "CViewC64.h"
#include "CImageData.h"
#include "png.h"

#include "VID_Main.h"
#include "CGuiMain.h"
#include <chrono>
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
#include "MT_CaptureHelpers.h"
#include "CViewBreakpoints.h"
#endif

using namespace std;
using namespace nlohmann;

// How long a remote snapshot save/load waits for the emulation thread to reach
// a CPU instruction boundary before failing (covers paused-mode 16ms polling).
static const u32 SNAPSHOT_BOUNDARY_TIMEOUT_MS = 5000;

// PNG in-memory write helpers for screen/snapshot endpoint
static void PngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
	vector<uint8_t> *buf = (vector<uint8_t> *)png_get_io_ptr(png_ptr);
	buf->insert(buf->end(), data, data + length);
}

static void PngFlushCallback(png_structp png_ptr)
{
	(void)png_ptr;
}

namespace
{
#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
using GuiClock = std::chrono::steady_clock;
struct GuiOperation
{
	string id;
	json args;
	json result;
	GuiClock::time_point deadline;
	bool terminal = false;
	bool started = false;
	bool engineQueued = false;
	bool finished = false;
};
std::mutex guiOperationMutex;
std::shared_ptr<GuiOperation> guiOperation;
// UI-only references: pending startup waits for the test engine's frame hook;
// an active operation remains held through the test engine's teardown.
std::shared_ptr<GuiOperation> guiPendingStartup;
std::shared_ptr<GuiOperation> guiUiOperation;
uint64_t guiNextId = 0;
const char *guiPanel = "C64 Memory Breakpoints";

void GuiFinish(const std::shared_ptr<GuiOperation> &op, const char *outcome, const char *error = nullptr)
{
	std::lock_guard<std::mutex> lock(guiOperationMutex);
	op->finished = true;
	op->terminal = !op->engineQueued;
	op->result["outcome"] = outcome;
	if (error) op->result["error"] = error;
}

bool GuiContinue(const std::shared_ptr<GuiOperation> &op, ImGuiTestContext *ctx)
{
	if (ctx->IsError()) { GuiFinish(op, "failed", "widget_action_failed"); return false; }
	if (GuiClock::now() >= op->deadline)
	{
		GuiFinish(op, "failed", "action_deadline_exceeded_partial_effects_possible");
		return false;
	}
	return true;
}

ImGuiWindow *GuiEditor()
{
	ImGuiWindow *panel = ImGui::FindWindowByName(guiPanel);
	if (!panel) return nullptr;
	ImGuiID id = panel->GetID("addMemBreakpointPopup");
	for (const ImGuiPopupData &popup : ImGui::GetCurrentContext()->OpenPopupStack)
		if (popup.PopupId == id && popup.Window && popup.Window->WasActive) return popup.Window;
	return nullptr;
}

string GuiBase64(const vector<unsigned char> &data)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	string out;
	for (size_t i = 0; i < data.size(); i += 3)
	{
		unsigned int bits = (unsigned int)data[i] << 16;
		if (i + 1 < data.size()) bits |= (unsigned int)data[i + 1] << 8;
		if (i + 2 < data.size()) bits |= data[i + 2];
		out += alphabet[(bits >> 18) & 63]; out += alphabet[(bits >> 12) & 63];
		out += i + 1 < data.size() ? alphabet[(bits >> 6) & 63] : '=';
		out += i + 2 < data.size() ? alphabet[bits & 63] : '=';
	}
	return out;
}

// Restored by GuiPostFrame only AFTER queue cleanup.  Restoring this from
// teardown would be too early: engine cleanup yields for three more frames.
bool guiSavedRestoreFocus = true;

void GuiRun(ImGuiTestContext *ctx)
{
	auto op = guiUiOperation;
	if (!GuiContinue(op, ctx)) return;
	const string action = op->args["action"];
	const bool editorTarget = op->args["target"] == "c64.memory_breakpoints.editor";
	if (!viewC64 || !viewC64->viewC64BreakpointsMemory)
	{
		GuiFinish(op, "failed", "c64_panel_unavailable"); return;
	}
	if (action == "show")
	{
		if (ImGui::GetCurrentContext()->OpenPopupStack.Size != 0)
		{ GuiFinish(op, "failed", "popup_already_open"); return; }
		viewC64->viewC64BreakpointsMemory->SetVisible(true);
		ctx->Yield(3);
		if (!GuiContinue(op, ctx)) return;
		ctx->WindowFocus(guiPanel);
	}
	else
	{
		ImGuiWindow *panel = ImGui::FindWindowByName(guiPanel);
		if (!viewC64->viewC64BreakpointsMemory->visible || !panel || !panel->WasActive)
		{ GuiFinish(op, "failed", "panel_not_visible_call_show_first"); return; }
		ImGuiWindow *editor = GuiEditor();
		if (editorTarget && !editor)
		{ GuiFinish(op, "failed", "editor_not_open"); return; }
		if (action == "open")
		{
			if (ImGui::GetCurrentContext()->OpenPopupStack.Size != 0)
			{ GuiFinish(op, "failed", "popup_already_open"); return; }
			ctx->SetRef(guiPanel);
			ctx->ItemClick("+");
			ctx->Yield(3);
			if (!GuiContinue(op, ctx)) return;
			if (!GuiEditor()) { GuiFinish(op, "failed", "editor_did_not_open"); return; }
		}
		else if (action == "capture")
		{
			vector<unsigned int> pixels;
			int width = 0, height = 0;
			if (!MT_CaptureWindowRGBA(ctx, editorTarget ? editor->Name : guiPanel, &pixels, &width, &height)
				|| ctx->IsError())
			{ GuiFinish(op, "failed", "rendered_capture_unavailable"); return; }
			png_image image = {};
			image.version = PNG_IMAGE_VERSION;
			image.width = width; image.height = height; image.format = PNG_FORMAT_RGBA;
			png_alloc_size_t size = 0;
			if (!png_image_write_to_memory(&image, nullptr, &size, 0, pixels.data(), 0, nullptr))
			{ png_image_free(&image); GuiFinish(op, "failed", "png_encode_failed"); return; }
			vector<unsigned char> png(size);
			bool ok = png_image_write_to_memory(&image, png.data(), &size, 0, pixels.data(), 0, nullptr) != 0;
			png_image_free(&image);
			if (!ok) { GuiFinish(op, "failed", "png_encode_failed"); return; }
			png.resize(size);
			std::lock_guard<std::mutex> lock(guiOperationMutex);
			op->result["image"] = {{"mimeType", "image/png"}, {"encoding", "base64"},
				{"width", width}, {"height", height}, {"data", GuiBase64(png)}};
		}
		else
		{
			// Bind to the identified popup, never whichever window happens to have focus.
			if (ImGui::GetCurrentContext()->OpenPopupStack.back().Window != editor)
			{ GuiFinish(op, "failed", "another_popup_blocks_editor"); return; }
			ctx->SetRef(editor->Name);
			if (action == "cancel")
			{
				// Use the popup's existing outside-click dismissal, without adding
				// a widget, calling ClosePopupToLevel, or moving unrelated windows.
				const ImRect bounds = panel->OuterRectClipped;
				const ImVec2 points[] = {
					ImVec2(bounds.Min.x + 2, bounds.Min.y + 2),
					ImVec2(bounds.Min.x + 2, bounds.Max.y - 2),
					ImVec2(bounds.Max.x - 2, bounds.Min.y + 2),
					ImVec2(bounds.Max.x - 2, bounds.Max.y - 2)
				};
				bool clicked = false;
				for (const ImVec2 &point : points)
				{
					if (!bounds.Contains(point) || editor->Rect().Contains(point)) continue;
					ctx->MouseMoveToPos(point);
					if (!GuiContinue(op, ctx)) return;
					if (GuiEditor() != editor || ImGui::GetCurrentContext()->OpenPopupStack.back().Window != editor)
					{ GuiFinish(op, "failed", "editor_changed_before_dismissal"); return; }
					ctx->MouseClick();
					clicked = true;
					break;
				}
				if (!clicked) { GuiFinish(op, "failed", "no_safe_popup_dismissal_point"); return; }
			}
			else
			{
				ctx->ItemInput("##addMemBreakpointPopupAddress_Vice");
				if (!GuiContinue(op, ctx)) return;
				if (action == "set_address")
				{
					string text = op->args["text"];
					ctx->KeyCharsReplace(text.c_str());
				}
				else ctx->KeyPress(ImGuiKey_Enter);
			}
			ctx->Yield(3);
			if (!GuiContinue(op, ctx)) return;
			if ((action == "submit" || action == "cancel") && GuiEditor())
			{ GuiFinish(op, "failed", "editor_still_open"); return; }
		}
	}
	if (GuiContinue(op, ctx)) GuiFinish(op, "completed");
}

class GuiStartOperation : public CUiThreadTaskCallback
{
public:
	std::shared_ptr<GuiOperation> op;
	GuiStartOperation(std::shared_ptr<GuiOperation> op) : op(op) {}
	void RunUIThreadTask() override
	{
		// CGuiMain removes callbacks but does not delete them.
		std::unique_ptr<GuiStartOperation> deleteAfterRun(this);
		Start(op);
	}
	static void Start(const std::shared_ptr<GuiOperation> &op)
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
		{ GuiFinish(op, "failed", "interactive_gui_unavailable"); return; }
		const int engineFrame = ImGuiTestEngine_GetFrameCount(engine);
		const int guiFrame = ImGui::GetFrameCount();
		if (!GuiControl::TestEngineFrameReady(engineFrame, guiFrame))
		{
			// Do not queue in the same frame as lazy initialization: the engine's
			// missing-signal assertion remains enabled.  GuiPostFrame retries under
			// the original deadline, after the next NewFrame hook can run.
			guiPendingStartup = op;
			return;
		}
		if (!CImGuiTestEngine::IsTestQueueEmpty())
		{ GuiFinish(op, "failed", "imgui_test_engine_busy"); return; }
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
		test->TestFunc = GuiRun;
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
#endif
}

void CDebuggerServerApi::GuiPostFrame()
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

void CDebuggerServerApi::RegisterGuiEndpoint(CDebuggerServer *server)
{
	EndpointDescriptor desc;
	desc.fn = "gui/control";
	desc.category = "gui";
	desc.description = "Allowlisted interactive C64 Memory Breakpoints panel/editor; queued actions require status";
	desc.paramsSchema = GuiControl::Schema();
	server->AddEndpointFunction(desc, [server](const string token, json args, unsigned char *, int) -> vector<char>*
	{
		if (!GuiControl::Valid(args))
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
		if (args["action"] == "capabilities")
			return server->PrepareResult(HTTP_OK, token, {{"available", available},
				{"reason", available ? "" : reason}, {"targets", json::array({"c64.memory_breakpoints", "c64.memory_breakpoints.editor"})}}, nullptr, 0);
		if (!available)
			return server->PrepareResult(HTTP_SERVICE_UNAVAILABLE, token, {{"error", reason}}, nullptr, 0);
#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
		std::shared_ptr<GuiOperation> op;
		{
			std::lock_guard<std::mutex> lock(guiOperationMutex);
			// Expiration is cancellation BEFORE UI execution.  The queued callback
			// retains its own record and cannot execute a replacement operation.
			if (guiOperation && !guiOperation->terminal && !guiOperation->started && GuiClock::now() >= guiOperation->deadline)
			{ guiOperation->terminal = true; guiOperation->result["outcome"] = "expired"; }
			if (args["action"] == "status")
			{
				if (!guiOperation || guiOperation->id != args["requestId"].get<std::string>())
					return server->PrepareResult(HTTP_NOT_FOUND, token, {{"error", "unknown_gui_request"}}, nullptr, 0);
				json result = guiOperation->result;
				if (!guiOperation->terminal) result = {{"requestId", guiOperation->id}, {"outcome", guiOperation->started ? "running" : "queued"}};
				result["deadlineExceeded"] = GuiClock::now() >= guiOperation->deadline;
				return server->PrepareResult(HTTP_OK, token, result, nullptr, 0);
			}
			if (guiOperation && !guiOperation->terminal)
				return server->PrepareResult(HTTP_SERVICE_UNAVAILABLE, token, {{"error", "gui_busy"}, {"requestId", guiOperation->id}}, nullptr, 0);
			op = std::make_shared<GuiOperation>();
			op->id = std::to_string(++guiNextId);
			op->args = args;
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

CDebuggerServerApi::CDebuggerServerApi(CDebugInterface *debugInterface)
: debugInterface(debugInterface)
{
	debuggerApi = debugInterface->GetDebuggerApi();
	// debuggerApi may be NULL for backends that don't implement it (e.g. C64U)
}

// Helper: register endpoint with both handler AND descriptor metadata
static void RegisterEndpoint(CDebuggerServer *server, const char *fn, const char *platform,
							 const char *category, const char *description,
							 EndpointHandlerV1 handler,
							 bool binaryIn = false, bool binaryOut = false)
{
	EndpointDescriptor desc;
	desc.fn = fn;
	desc.platform = platform;
	desc.category = category;
	desc.description = description;
	desc.supportsBinaryInput = binaryIn;
	desc.supportsBinaryOutput = binaryOut;
	server->AddEndpointFunction(desc, handler);
}

static json MemoryAccessToJson(const DebugMemoryCellAccess &access)
{
	json result = {{"available", access.IsAvailable()}, {"pc", nullptr},
		{"cycle", nullptr}, {"frame", nullptr}, {"rasterLine", nullptr}, {"rasterCycle", nullptr}};
	if (access.IsAvailable())
	{
		result["pc"] = access.pc;
		result["cycle"] = access.cycle;
		if (access.frame != (u32)-1) result["frame"] = access.frame;
		if (access.rasterLine >= 0) result["rasterLine"] = access.rasterLine;
		if (access.rasterCycle >= 0) result["rasterCycle"] = access.rasterCycle;
	}
	return result;
}

void CDebuggerServerApi::RegisterEndpoints(CDebuggerServer *server)
{
	LOGD("CDebuggerServerApi::RegisterEndpoints: /%s", debugInterface->GetPlatformNameEndpointString());

	// Skip endpoint registration for backends without a debugger API (e.g. C64U stub)
	if (!debuggerApi)
	{
		LOGD("CDebuggerServerApi::RegisterEndpoints: debuggerApi is NULL for /%s, skipping", debugInterface->GetPlatformNameEndpointString());
		return;
	}

	char *buf = SYS_GetCharBuf();
	const char *plat = debugInterface->GetPlatformNameEndpointString();

	// PR1 exposes only the existing frame-navigation queue, not precise seek.
	for (bool statusOnly : {true, false})
	{
		EndpointDescriptor desc;
		desc.fn = std::string(plat) + (statusOnly ? "/timeline/status" : "/timeline/frame");
		desc.platform = plat;
		desc.category = "timeline";
		desc.description = statusOnly ? "C64-only read-only frame history/recording status"
			: "C64-only native frame navigation; queued acceptance, not completed restoration";
		desc.paramsSchema = {{"type", "object"}, {"additionalProperties", false}};
		if (!statusOnly)
		{
			desc.paramsSchema["properties"] = {{"frame", {{"type", "integer"}, {"minimum", 0}, {"maximum", INT_MAX}}}};
			desc.paramsSchema["required"] = json::array({"frame"});
		}
		server->AddEndpointFunction(desc,
			[this, server, statusOnly](const string token, json params, unsigned char *, int) -> vector<char>*
		{
			if (debugInterface->GetEmulatorType() != EMULATOR_TYPE_C64_VICE)
				return server->PrepareResult(HTTP_BAD_REQUEST, token, {{"error", "unsupported_platform"}}, NULL, 0);
			int frame = 0;
			if (statusOnly ? !(params.is_null() || (params.is_object() && params.empty()))
				: !TimelineNavigation::ParseFrame(params, frame))
				return server->PrepareResult(HTTP_BAD_REQUEST, token, {{"error", "invalid_timeline_arguments"}}, NULL, 0);
			CSnapshotsManager *manager = debugInterface->snapshotsManager;
			if (!manager)
				return server->PrepareResult(HTTP_SERVICE_UNAVAILABLE, token, {{"error", "timeline_unavailable"}}, NULL, 0);
			json result = statusOnly ? TimelineNavigation::StatusJson(manager->GetFrameNavigationStatus())
				: TimelineNavigation::ResultJson(manager->RequestFrameNavigation(frame), frame);
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}
	
	sprintf(buf, "%s/reset/hard", plat);
	RegisterEndpoint(server, buf, plat, "control", "Hard reset the machine",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->ResetMachine(true);
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		ev["hard"] = true;
		server->BroadcastEvent("emulation.reset", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/reset/soft", plat);
	RegisterEndpoint(server, buf, plat, "control", "Soft reset the machine",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->ResetMachine(false);
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		ev["hard"] = false;
		server->BroadcastEvent("emulation.reset", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/detachEverything", plat);
	RegisterEndpoint(server, buf, plat, "control", "Detach all cartridges, disks, and tapes",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->DetachEverything();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/detachDiskImage", plat);
	RegisterEndpoint(server, buf, plat, "control", "Detach the disk image from one drive without resetting the machine",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int device = debuggerApi->GetDefaultDiskDriveNumber();
		if (params.contains("device"))
		{
			device = params.at("device").get<int>();
		}
		
		json result;
		result["platform"] = debugInterface->GetPlatformNameEndpointString();
		result["device"] = device;
		
		if (!debuggerApi->DetachDriveDisk(device))
		{
			result["error"] = "disk detach is not supported by this platform, or the device number is out of range";
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		}
		
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		ev["device"] = device;
		server->BroadcastEvent("media.detached", ev);
		
		result["status"] = "detached";
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/attachDiskImage", plat);
	RegisterEndpoint(server, buf, plat, "control", "Attach one disk image without resetting, loading, or running a program",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		result["platform"] = debugInterface->GetPlatformNameEndpointString();
		if (!params.contains("path") || !params.at("path").is_string()
			|| params.at("path").get<string>().empty())
		{
			result["error"] = "non-empty disk image path required";
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		}

		string filePath = params.at("path").get<string>();
		int device = debuggerApi->GetDefaultDiskDriveNumber();
		if (params.contains("device"))
		{
			if (!params.at("device").is_number_integer())
			{
				result["error"] = "integer disk device required";
				return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
			}
			device = params.at("device").get<int>();
		}
		result["device"] = device;
		result["path"] = filePath;

		if (!SYS_FileExists(filePath.c_str()))
		{
			result["error"] = "disk image file not found";
			return server->PrepareResult(HTTP_NOT_FOUND, token, result, NULL, 0);
		}

		if (!debuggerApi->AttachDriveDisk(filePath.c_str(), device))
		{
			result["error"] = "disk attach is not supported by this platform/device, or the image could not be attached";
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		}

		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		ev["device"] = device;
		ev["path"] = filePath;
		server->BroadcastEvent("media.attached", ev);

		result["status"] = "attached";
		result["autoRun"] = false;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/warp/set", plat);
	RegisterEndpoint(server, buf, plat, "control", "Enable or disable warp speed",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		bool warp = params.at("warp").get<bool>();
		debuggerApi->SetWarpSpeed(warp);
		// read the state back so a client can verify its own action in one round trip
		json result;
		result["warp"] = debuggerApi->GetWarpSpeed();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/warp/get", plat);
	RegisterEndpoint(server, buf, plat, "control", "Get current warp speed state",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		result["warp"] = debuggerApi->GetWarpSpeed();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/pause", plat);
	RegisterEndpoint(server, buf, plat, "control", "Pause emulation",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->PauseEmulation();
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		server->BroadcastEvent("emulation.paused", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/continue", plat);
	RegisterEndpoint(server, buf, plat, "control", "Continue emulation after pause",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->UnPauseEmulation();
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		server->BroadcastEvent("emulation.continued", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/cycle", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Step one CPU cycle",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOneCycle();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/instruction", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Step one CPU instruction",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOverInstruction();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/subroutine", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Step over subroutine call",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOverSubroutine();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/status", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Get CPU registers and flags",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json cpuStatus = debuggerApi->GetCpuStatusJson();
		return server->PrepareResult(HTTP_OK, token, cpuStatus, NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/makejmp", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Set program counter to address",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		bool applied = debuggerApi->MakeJmp(address);
		json result;
		result["address"] = address;
		result["applied"] = applied;
		if (!applied)
		{
			// The PC change is queued but has not reached the CPU yet, so a step
			// issued now would still execute at the old PC.
			result["pendingPc"] = address;
		}
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/counters/read", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Read cycle, instruction, and frame counters",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json counters;
		counters["cycle"] = debuggerApi->GetMainCpuCycleCounter();
		counters["instruction"] = debuggerApi->GetMainCpuInstructionCycleCounter();
		counters["frame"] = debuggerApi->GetEmulationFrameNumber();
		return server->PrepareResult(HTTP_OK, token, counters, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/lastAccess", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Read last recorded CPU-address reader and writer without changing emulation",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!params.contains("address") || !params.at("address").is_number_integer()
			|| params.at("address") < 0 || params.at("address") > 0xFFFF)
		{
			return server->PrepareResult(HTTP_BAD_REQUEST, token,
				{{"error", "address must be an integer from 0 to 65535"}}, NULL, 0);
		}
		int address = params.at("address").get<int>();
		CDebugInterfaceMutexGuard guard(debugInterface);
		CDebugMemory *memory = debugInterface->symbols ? debugInterface->symbols->memory : NULL;
		if (!memory)
		{
			return server->PrepareResult(HTTP_SERVICE_UNAVAILABLE, token,
				{{"error", "CPU memory access tracking is unavailable"}}, NULL, 0);
		}
		CDebugMemoryCell *cell = memory->GetMemoryCell(address);
		if (!cell)
		{
			return server->PrepareResult(HTTP_BAD_REQUEST, token,
				{{"error", "address is outside the CPU memory marker map"}}, NULL, 0);
		}

		// No memory adapter reads: inspecting a record must not become a CPU read.
		DebugMemoryCellLastAccess access = cell->GetLastAccess();
		json result = {{"platform", debugInterface->GetPlatformNameEndpointString()},
			{"address", address}, {"addressSpace", "cpu"},
			{"read", MemoryAccessToJson(access.read)}, {"write", MemoryAccessToJson(access.write)}};
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/writeBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Write binary block to CPU-visible memory",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
//		LOGD("cpu/memory/writeBlock: address=%d size=%d", address, binaryDataSize);
//		LOG_PrintHexArray(binaryData, binaryDataSize);

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryWithIO();
		if (address + binaryDataSize > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		for (int i = 0; i < binaryDataSize; i++)
		{
			dataAdapter->AdapterWriteByte(address++, binaryData[i]);
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	}, true);
	
	sprintf(buf, "%s/cpu/memory/readBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Read binary block from CPU-visible memory",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryWithIO();
		int address, size;
		if (!params.contains("address") || !params.contains("size")
			|| !DebuggerProtocol::ValidateMemoryReadRange(params.at("address"), params.at("size"),
				dataAdapter->AdapterGetDataLength(), address, size))
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		u8 *resultBinaryData = new u8[size];

		for (int i = 0; i < size; i++)
		{
			dataAdapter->AdapterReadByte(address++, &resultBinaryData[i]);
		}
		auto *resp = server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
		delete[] resultBinaryData;
		return resp;
	}, false, true);

	sprintf(buf, "%s/ram/clear", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Clear RAM region to a value",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
		int value = 0;
		if (params.contains("value"))
		{
			value = params.at("value").get<int>();
		}

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryDirectRAM();
		if (address + size > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		for (int i = 0; i < size; i++)
		{
			dataAdapter->AdapterWriteByte(address++, value);
		}
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/ram/writeBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Write binary block to direct RAM",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
//		LOGD("%s/ram/writeBlock: address=%d size=%d", debugInterface->GetPlatformNameEndpointString(), address, binaryDataSize);
//		LOG_PrintHexArray(binaryData, binaryDataSize);

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryDirectRAM();
		if (address + binaryDataSize > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		for (int i = 0; i < binaryDataSize; i++)
		{
			dataAdapter->AdapterWriteByte(address++, binaryData[i]);
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	}, true);
	
	sprintf(buf, "%s/ram/readBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Read binary block from direct RAM",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryDirectRAM();
		int address, size;
		if (!params.contains("address") || !params.contains("size")
			|| !DebuggerProtocol::ValidateMemoryReadRange(params.at("address"), params.at("size"),
				dataAdapter->AdapterGetDataLength(), address, size))
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		u8 *resultBinaryData = new u8[size];

		for (int i = 0; i < size; i++)
		{
			dataAdapter->AdapterReadByte(address++, &resultBinaryData[i]);
		}
		auto *resp = server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
		delete[] resultBinaryData;
		return resp;
	}, false, true);
	
	sprintf(buf, "%s/input/key/down", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send key-down event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int mtKeyCode = params.at("keyCode").get<int>();
		bool res = debuggerApi->KeyboardDown(mtKeyCode);
		return server->PrepareResult(res ? HTTP_OK : HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/key/up", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send key-up event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int mtKeyCode = params.at("keyCode").get<int>();
		bool res = debuggerApi->KeyboardUp(mtKeyCode);
		return server->PrepareResult(res ? HTTP_OK : HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/joystick/down", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send joystick axis down event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u32 axis = CDebuggerApi::JoypadAxisNameToAxisCode(params.at("axis"));
		int port = params.at("port");
		debuggerApi->JoystickDown(port, axis);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/joystick/up", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send joystick axis up event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u32 axis = CDebuggerApi::JoypadAxisNameToAxisCode(params.at("axis"));
		int port = params.at("port");
		debuggerApi->JoystickUp(port, axis);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// breakpoints
	sprintf(buf, "%s/cpu/breakpoint/add", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Add CPU address breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		json j;

		u64 breakpointId = debuggerApi->AddBreakpointPC((int)addr);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}

		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/cpu/breakpoint/remove", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Remove CPU address breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		json j;

		u64 breakpointId = debuggerApi->RemoveBreakpointPC((int)addr);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}

		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/breakpoint/add", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Add memory data breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		u64 value = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("value"));

		json j;

		u32 memoryAccess = MEMORY_BREAKPOINT_ACCESS_WRITE;
		if (params.contains("access"))
		{
			if (params.at("access") == "read")
			{
				memoryAccess = MEMORY_BREAKPOINT_ACCESS_READ;
			}
			else if (params.at("access") == "write")
			{
				memoryAccess = MEMORY_BREAKPOINT_ACCESS_WRITE;
			}
			else return server->PrepareResult(HTTP_BAD_REQUEST, token, j, NULL, 0);
		}

		DataBreakpointComparison comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
		if (params.contains("comparison"))
		{
			string comparisonStr = params.at("comparison");
			comparison = CDebugBreakpointsData::StrToDataBreakpointComparison(comparisonStr.c_str());
		}

		u64 breakpointId = debuggerApi->AddBreakpointMemory((int)addr, memoryAccess, comparison, (int)value);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}

		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/breakpoint/remove", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Remove memory data breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		json j;

		u64 breakpointId = debuggerApi->RemoveBreakpointMemory((int)addr);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}

		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});
	
	// set/get segment
	sprintf(buf, "%s/segment/read", plat);
	RegisterEndpoint(server, buf, plat, "segment", "Get current debug segment name",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json j;
		CSlrString *segmentNameStr = debuggerApi->GetCurrentSegmentName();
		if (segmentNameStr == NULL)
		{
			j["segment"] = "";
		}
		else
		{
			const char *name = segmentNameStr->GetStdASCII();
			j["segment"] = name;
			delete [] name;
		}

		return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
	});

	sprintf(buf, "%s/segment/write", plat);
	RegisterEndpoint(server, buf, plat, "segment", "Set current debug segment by name",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		string segmentName = params["segment"];
		CSlrString *segmentNameStr = new CSlrString(segmentName.c_str());
		bool ret = debuggerApi->SetCurrentSegment(segmentNameStr);
		delete segmentNameStr;

		if (ret)
		{
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		}
		else
		{
			return server->PrepareResult(HTTP_NOT_FOUND, token, json(), NULL, 0);
		}

	});

	
	// Breakpoint listing
	sprintf(buf, "%s/cpu/breakpoint/list", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "List all CPU address breakpoints",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json bpList = json::array();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				CDebugBreakpointsAddr *bps = symbols->currentSegment->breakpointsPC;
				if (bps)
				{
					for (auto &pair : bps->breakpoints)
					{
						json bp;
						bp["address"] = pair.first;
						bp["isActive"] = pair.second->isActive;
						bpList.push_back(bp);
					}
				}
			}
		}

		result["breakpoints"] = bpList;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/breakpoint/list", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "List all memory data breakpoints",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json bpList = json::array();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				CDebugBreakpointsData *bps = symbols->currentSegment->breakpointsData;
				if (bps)
				{
					for (auto &pair : bps->breakpoints)
					{
						json bp;
						bp["address"] = pair.first;
						bp["isActive"] = pair.second->isActive;
						bpList.push_back(bp);
					}
				}
			}
		}

		result["breakpoints"] = bpList;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Machine state
	sprintf(buf, "%s/state", plat);
	RegisterEndpoint(server, buf, plat, "state", "Get machine state including run/pause status",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result["platform"] = debugInterface->GetPlatformNameEndpointString();
			result["isRunning"] = debugInterface->isRunning;
			result["isPaused"] = debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING;

			// CPU status
			CDataAdapter *da = debugInterface->GetDataAdapter();
			if (da)
			{
				result["cpuPC"] = debugInterface->GetCpuPC();
				result["dataAdapterSize"] = da->AdapterGetDataLength();
			}
		}

		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Watch list
	sprintf(buf, "%s/watch/list", plat);
	RegisterEndpoint(server, buf, plat, "watch", "List all data watches",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json watchList = json::array();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				for (auto &pair : symbols->currentSegment->watches)
				{
					CDebugSymbolsDataWatch *w = pair.second;
					json watch;
					watch["address"] = w->address;
					if (w->watchName)
						watch["name"] = w->watchName;
					watch["representation"] = w->representation;
					watch["numberOfValues"] = w->numberOfValues;
					watchList.push_back(watch);
				}
			}
		}

		result["watches"] = watchList;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Watch add
	sprintf(buf, "%s/watch/add", plat);
	RegisterEndpoint(server, buf, plat, "watch", "Add a data watch at address",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		string name = params.value("name", "");
		int numValues = params.value("numberOfValues", 1);

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				char *nameBuf = NULL;
				if (!name.empty())
				{
					nameBuf = new char[name.size() + 1];
					strcpy(nameBuf, name.c_str());
				}
				symbols->currentSegment->AddWatch(address, nameBuf, 0, numValues);
				if (nameBuf) delete[] nameBuf;
			}
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// Watch remove
	sprintf(buf, "%s/watch/remove", plat);
	RegisterEndpoint(server, buf, plat, "watch", "Remove a data watch at address",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				symbols->currentSegment->RemoveWatch(address);
			}
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// Snapshot save — returns snapshot as binary blob
	sprintf(buf, "%s/snapshot/save", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Save emulator snapshot as binary blob",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		CByteBuffer *byteBuffer = new CByteBuffer();
		bool success = debugInterface->SaveChipsSnapshotAtCpuBoundary(byteBuffer, SNAPSHOT_BOUNDARY_TIMEOUT_MS);

		if (success && byteBuffer->length > 0)
		{
			json result;
			result["size"] = byteBuffer->length;
			vector<char> *resp = server->PrepareResult(HTTP_OK, token, result, byteBuffer->data, byteBuffer->length);
			delete byteBuffer;
			return resp;
		}

		delete byteBuffer;
		json err;
		err["error"] = "snapshot save did not complete at a CPU instruction boundary (emulator not running, or paused on Atari/NES?)";
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, err, NULL, 0);
	}, false, true);

	// Snapshot load — accepts snapshot binary blob
	sprintf(buf, "%s/snapshot/load", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Load emulator snapshot from binary blob",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!binaryData || binaryDataSize <= 0)
		{
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);
		}

		CByteBuffer *byteBuffer = new CByteBuffer(binaryData, binaryDataSize);
		bool success = debugInterface->LoadChipsSnapshotAtCpuBoundary(byteBuffer, SNAPSHOT_BOUNDARY_TIMEOUT_MS);
		delete byteBuffer;

		if (success)
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		json err;
		err["error"] = "snapshot load did not complete at a CPU instruction boundary (emulator not running, or paused on Atari/NES?)";
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, err, NULL, 0);
	}, true);

	// Snapshot save to file — uses SaveChipsSnapshotAtCpuBoundary (CPU-boundary-synced) then writes to file
	sprintf(buf, "%s/snapshot/saveFile", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Save emulator snapshot to a file path",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string path = params.at("path").get<string>();

		CByteBuffer *byteBuffer = new CByteBuffer();
		bool success = debugInterface->SaveChipsSnapshotAtCpuBoundary(byteBuffer, SNAPSHOT_BOUNDARY_TIMEOUT_MS);

		if (!success || byteBuffer->length == 0)
		{
			delete byteBuffer;
			json err;
			err["error"] = "snapshot save did not complete at a CPU instruction boundary (emulator not running, or paused on Atari/NES?)";
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, err, NULL, 0);
		}

		FILE *f = fopen(path.c_str(), "wb");
		if (!f)
		{
			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}
		fwrite(byteBuffer->data, 1, byteBuffer->length, f);
		fclose(f);

		json result;
		result["path"] = path;
		result["size"] = byteBuffer->length;
		delete byteBuffer;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Snapshot load from file — reads file then uses LoadChipsSnapshotAtCpuBoundary (CPU-boundary-synced)
	sprintf(buf, "%s/snapshot/loadFile", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Load emulator snapshot from a file path",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string path = params.at("path").get<string>();

		FILE *f = fopen(path.c_str(), "rb");
		if (!f)
			return server->PrepareResult(HTTP_NOT_FOUND, token, json(), NULL, 0);

		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fileSize <= 0)
		{
			fclose(f);
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);
		}

		u8 *data = new u8[fileSize];
		fread(data, 1, fileSize, f);
		fclose(f);

		// CByteBuffer takes ownership of 'data' and frees it in its destructor
		CByteBuffer *byteBuffer = new CByteBuffer(data, (int)fileSize);
		bool success = debugInterface->LoadChipsSnapshotAtCpuBoundary(byteBuffer, SNAPSHOT_BOUNDARY_TIMEOUT_MS);
		delete byteBuffer; // also frees 'data'

		if (success)
		{
			json result;
			result["path"] = path;
			result["size"] = (long)fileSize;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		json err;
		err["error"] = "snapshot load did not complete at a CPU instruction boundary (emulator not running, or paused on Atari/NES?)";
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, err, NULL, 0);
	});

	// Quick snapshot save — saves to the standard quick-slot file (slot is 1-indexed, matching RetroDebugger UI)
	sprintf(buf, "%s/snapshot/quickSave", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Save quick snapshot to a numbered slot (slot 1-N, matches RetroDebugger UI)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int slot = params.at("slot").get<int>();
		if (slot < 1)
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);

		int fileId = slot - 1; // UI is 1-indexed, files are 0-indexed
		const char *ext = nullptr;
		switch (debugInterface->GetEmulatorType())
		{
			case EMULATOR_TYPE_C64_VICE:  ext = "snap"; break;
			case EMULATOR_TYPE_ATARI800:  ext = "a8s";  break;
			case EMULATOR_TYPE_NESTOPIA:  ext = "sav";  break;
			default:
				return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}

		string path = gUTFPathToSettings->GetStdStringUTF8();
		char fname[64];
		sprintf(fname, "snapshot-%d.%s", fileId, ext);
		path += fname;

		CByteBuffer *byteBuffer = new CByteBuffer();
		bool success = debugInterface->SaveChipsSnapshotAtCpuBoundary(byteBuffer, SNAPSHOT_BOUNDARY_TIMEOUT_MS);

		if (!success || byteBuffer->length == 0)
		{
			delete byteBuffer;
			json err;
			err["error"] = "snapshot save did not complete at a CPU instruction boundary (emulator not running, or paused on Atari/NES?)";
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, err, NULL, 0);
		}

		FILE *f = fopen(path.c_str(), "wb");
		if (!f)
		{
			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}
		fwrite(byteBuffer->data, 1, byteBuffer->length, f);
		fclose(f);

		json result;
		result["slot"] = slot;
		result["path"] = path;
		result["size"] = byteBuffer->length;
		delete byteBuffer;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Quick snapshot load — loads from the standard quick-slot file (slot is 1-indexed, matching RetroDebugger UI)
	sprintf(buf, "%s/snapshot/quickLoad", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Load quick snapshot from a numbered slot (slot 1-N, matches RetroDebugger UI)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int slot = params.at("slot").get<int>();
		if (slot < 1)
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);

		int fileId = slot - 1;
		const char *ext = nullptr;
		switch (debugInterface->GetEmulatorType())
		{
			case EMULATOR_TYPE_C64_VICE:  ext = "snap"; break;
			case EMULATOR_TYPE_ATARI800:  ext = "a8s";  break;
			case EMULATOR_TYPE_NESTOPIA:  ext = "sav";  break;
			default:
				return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}

		string path = gUTFPathToSettings->GetStdStringUTF8();
		char fname[64];
		sprintf(fname, "snapshot-%d.%s", fileId, ext);
		path += fname;

		FILE *f = fopen(path.c_str(), "rb");
		if (!f)
			return server->PrepareResult(HTTP_NOT_FOUND, token, json(), NULL, 0);

		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fileSize <= 0)
		{
			fclose(f);
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);
		}

		u8 *data = new u8[fileSize];
		fread(data, 1, fileSize, f);
		fclose(f);

		CByteBuffer *byteBuffer = new CByteBuffer(data, (int)fileSize);
		bool success = debugInterface->LoadChipsSnapshotAtCpuBoundary(byteBuffer, SNAPSHOT_BOUNDARY_TIMEOUT_MS);
		delete byteBuffer;

		if (success)
		{
			json result;
			result["slot"] = slot;
			result["path"] = path;
			result["size"] = (long)fileSize;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		json err;
		err["error"] = "snapshot load did not complete at a CPU instruction boundary (emulator not running, or paused on Atari/NES?)";
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, err, NULL, 0);
	});

	// --- Input injection ---

	// Joystick button/direction press
	sprintf(buf, "%s/input/joystickDown", plat);
	RegisterEndpoint(server, buf, plat, "input", "Press joystick directions/buttons (axes bitmask: N=1 S=2 W=4 E=8 FIRE=16 FIRE2=32 START=64 SELECT=128)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int port = params.value("port", 0);
		uint32_t axes = params.at("axes").get<uint32_t>();
		debugInterface->JoystickDown(port, axes);
		json result;
		result["port"] = port;
		result["axes"] = axes;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Joystick button/direction release
	sprintf(buf, "%s/input/joystickUp", plat);
	RegisterEndpoint(server, buf, plat, "input", "Release joystick directions/buttons (axes bitmask: N=1 S=2 W=4 E=8 FIRE=16 FIRE2=32 START=64 SELECT=128)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int port = params.value("port", 0);
		uint32_t axes = params.at("axes").get<uint32_t>();
		debugInterface->JoystickUp(port, axes);
		json result;
		result["port"] = port;
		result["axes"] = axes;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Read current joystick state
	sprintf(buf, "%s/input/joystickState", plat);
	RegisterEndpoint(server, buf, plat, "input", "Read current joystick state bitmask for a port",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int port = params.value("port", 0);
		uint32_t state = debugInterface->GetJoystickState(port);
		json result;
		result["port"] = port;
		result["state"] = state;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Keyboard key press
	sprintf(buf, "%s/input/keyDown", plat);
	RegisterEndpoint(server, buf, plat, "input", "Press a keyboard key (keyCode: MTKEY/SDL keycode; ASCII for printable chars)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		uint32_t keyCode = params.at("keyCode").get<uint32_t>();
		debugInterface->KeyboardDown(keyCode);
		json result;
		result["keyCode"] = keyCode;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Keyboard key release
	sprintf(buf, "%s/input/keyUp", plat);
	RegisterEndpoint(server, buf, plat, "input", "Release a keyboard key (keyCode: MTKEY/SDL keycode; ASCII for printable chars)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		uint32_t keyCode = params.at("keyCode").get<uint32_t>();
		debugInterface->KeyboardUp(keyCode);
		json result;
		result["keyCode"] = keyCode;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Emulator start — starts the emulation thread for this platform
	sprintf(buf, "%s/emulator/start", plat);
	RegisterEndpoint(server, buf, plat, "emulator", "Start the emulation thread for this platform",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (debugInterface->isRunning)
		{
			json result;
			result["status"] = "already_running";
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		viewC64->StartEmulationThreadSafe(debugInterface);
		json result;
		result["status"] = "started";
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Emulator stop — stops the emulation thread for this platform
	sprintf(buf, "%s/emulator/stop", plat);
	RegisterEndpoint(server, buf, plat, "emulator", "Stop the emulation thread for this platform",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!debugInterface->isRunning)
		{
			json result;
			result["status"] = "already_stopped";
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		viewC64->StopEmulationThreadSafe(debugInterface);
		json result;
		result["status"] = "stopped";
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// --- MCP analysis tools ---

	sprintf(buf, "%s/cpu/disassemble", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Disassemble memory as 6502 instructions",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.value("address", 0);
		int count = params.value("count", 64);
		bool includeBytes = params.value("includeBytes", true);
		bool includeLabels = params.value("includeLabels", true);

		if (count > 512) count = 512;
		if (count < 1) count = 1;

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->DisassembleMemory(address, count, includeBytes, includeLabels);
		}
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/assemble", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Assemble 6502 instructions and write to memory",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		string code = params.at("code").get<string>();

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->AssembleCode(address, code);
		}

		if (result.contains("error"))
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/codemap", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Get memory regions identified as executable code",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int startAddress = params.value("startAddress", 0);
		int endAddress = params.value("endAddress", 65535);

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->GetCodeMap(startAddress, endAddress);
		}
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/memory/search", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Search RAM for a specific byte value",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		const auto &vj = params.at("value");
		int value = vj.is_string() ? std::stoi(vj.get<string>()) : vj.get<int>();
		int startAddress = params.value("startAddress", 0);
		int endAddress = params.value("endAddress", 65535);

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryWithIO();
		int adapterLen = dataAdapter->AdapterGetDataLength();
		if (endAddress >= adapterLen) endAddress = adapterLen - 1;

		json matches = json::array();
		for (int addr = startAddress; addr <= endAddress; addr++)
		{
			u8 b = 0;
			dataAdapter->AdapterReadByte(addr, &b);
			if (b == (u8)value)
				matches.push_back(addr);
		}

		json result;
		result["value"] = value;
		result["startAddress"] = startAddress;
		result["endAddress"] = endAddress;
		result["matches"] = matches;
		result["count"] = (int)matches.size();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/search", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Search memory for opcode patterns",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string pattern = params.at("pattern").get<string>();
		int startAddress = params.value("startAddress", 0);
		int endAddress = params.value("endAddress", 65535);
		bool executedOnly = params.value("executedOnly", true);

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->SearchOpcodePattern(pattern, startAddress, endAddress, executedOnly);
		}

		if (result.contains("error"))
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Helper lambda: get screen pixels cropped to actual content size, encode as PNG.
	// Uses debuggerApi->GetScreenImage() which handles cropping per platform.
	auto encodeScreenPng = [this](int &outW, int &outH, string &errorText) -> vector<uint8_t>
	{
		errorText = "";
		CImageData *img = debuggerApi->GetScreenImage(&outW, &outH);
		if (!img || !img->resultData || outW <= 0 || outH <= 0)
		{
			// not an encoder problem: the emulator has no frame to hand out
			errorText = "Screen image not available";
			LOGError("encodeScreenPng: screen image not available (img=%s data=%s w=%d h=%d)",
					 img ? "ok" : "NULL", (img && img->resultData) ? "ok" : "NULL", outW, outH);
			return {};
		}

		uint8_t *pixels = img->resultData;

		vector<uint8_t> pngData;
		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			// most likely cause: png.h version does not match the linked libpng
			errorText = "PNG encoder init failed (libpng version mismatch?): header "
						PNG_LIBPNG_VER_STRING ", library ";
			errorText += png_get_libpng_ver(NULL);
			LOGError("encodeScreenPng: png_create_write_struct failed, header=%s library=%s",
					 PNG_LIBPNG_VER_STRING, png_get_libpng_ver(NULL));
			return {};
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			errorText = "PNG info struct allocation failed";
			LOGError("encodeScreenPng: png_create_info_struct failed");
			png_destroy_write_struct(&png_ptr, NULL);
			return {};
		}

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			errorText = "PNG encoding failed";
			LOGError("encodeScreenPng: libpng longjmp during encoding");
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return {};
		}

		png_set_write_fn(png_ptr, &pngData, PngWriteCallback, PngFlushCallback);
		png_set_IHDR(png_ptr, info_ptr, outW, outH, 8, PNG_COLOR_TYPE_RGB_ALPHA,
					 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);

		vector<png_bytep> rowPtrs(outH);
		for (int y = 0; y < outH; y++)
			rowPtrs[y] = pixels + y * outW * 4;
		png_write_image(png_ptr, rowPtrs.data());
		png_write_end(png_ptr, NULL);
		png_destroy_write_struct(&png_ptr, &info_ptr);

		return pngData;
	};

	sprintf(buf, "%s/screen/snapshot", plat);
	RegisterEndpoint(server, buf, plat, "screen", "Capture current emulator screen as PNG (binary payload)",
	[this, server, encodeScreenPng](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int w = 0, h = 0;
		string errorText;
		vector<uint8_t> pngData = encodeScreenPng(w, h, errorText);
		if (pngData.empty())
		{
			json errorJson;
			errorJson["error"] = errorText;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}
		json result;
		result["width"] = w;
		result["height"] = h;
		result["format"] = "png";
		return server->PrepareResult(HTTP_OK, token, result, pngData.data(), (int)pngData.size());
	});

	sprintf(buf, "%s/screen/save", plat);
	RegisterEndpoint(server, buf, plat, "screen", "Save current emulator screen as PNG file to a given path",
	[this, server, encodeScreenPng](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string path = params.at("path").get<string>();

		int w = 0, h = 0;
		string errorText;
		vector<uint8_t> pngData = encodeScreenPng(w, h, errorText);
		if (pngData.empty())
		{
			json errorJson;
			errorJson["error"] = errorText;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}

		FILE *fp = fopen(path.c_str(), "wb");
		if (!fp)
		{
			json errorJson;
			errorJson["error"] = "Cannot open file for writing: " + path;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}
		fwrite(pngData.data(), 1, pngData.size(), fp);
		fclose(fp);

		json result;
		result["path"] = path;
		result["width"] = w;
		result["height"] = h;
		result["format"] = "png";
		result["byteCount"] = (int)pngData.size();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	SYS_ReleaseCharBuf(buf);
}
