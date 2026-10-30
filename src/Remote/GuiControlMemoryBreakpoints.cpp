#include "SYS_Main.h"
#include "EmulatorsConfig.h"

#if MT_ENABLE_IMGUI_TEST_ENGINE && defined(RUN_COMMODORE64)
// windows.h exports a legacy Yield() macro, not an ImGui operation.
#ifdef Yield
#undef Yield
#endif
#include "GuiControlOperation.h"
#include "CViewC64.h"
#include "CViewBreakpoints.h"
#include "imgui_te_context.h"
#include "imgui_internal.h"
#include "MT_CaptureHelpers.h"
#include "png.h"

using namespace std;

namespace GuiControl
{
namespace
{
const char *guiPanel = "C64 Memory Breakpoints";

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

}

void RunMemoryBreakpoints(ImGuiTestContext *ctx, Operation &op)
{
	if (!op.Continue(ctx)) return;
	const Action action = op.request.action;
	const bool editorTarget = op.request.target == Target::MemoryBreakpointsEditor;
	if (!viewC64 || !viewC64->viewC64BreakpointsMemory)
	{
		op.Finish("failed", "c64_panel_unavailable"); return;
	}
	ImGuiWindow *panel = nullptr;
	ImGuiWindow *editor = nullptr;
	if (action != Action::Show)
	{
		panel = ImGui::FindWindowByName(guiPanel);
		if (!viewC64->viewC64BreakpointsMemory->visible || !panel || !panel->WasActive)
		{ op.Finish("failed", "panel_not_visible_call_show_first"); return; }
		editor = GuiEditor();
		if (editorTarget && !editor)
		{ op.Finish("failed", "editor_not_open"); return; }
	}
	switch (action)
	{
	case Action::Show:
		if (ImGui::GetCurrentContext()->OpenPopupStack.Size != 0)
		{ op.Finish("failed", "popup_already_open"); return; }
		viewC64->viewC64BreakpointsMemory->SetVisible(true);
		ctx->Yield(3);
		if (!op.Continue(ctx)) return;
		ctx->WindowFocus(guiPanel);
		break;
	case Action::Open:
		if (ImGui::GetCurrentContext()->OpenPopupStack.Size != 0)
		{ op.Finish("failed", "popup_already_open"); return; }
		ctx->SetRef(guiPanel);
		ctx->ItemClick("+");
		ctx->Yield(3);
		if (!op.Continue(ctx)) return;
		if (!GuiEditor()) { op.Finish("failed", "editor_did_not_open"); return; }
		break;
	case Action::Capture:
	{
		vector<unsigned int> pixels;
		int width = 0, height = 0;
		if (!MT_CaptureWindowRGBA(ctx, editorTarget ? editor->Name : guiPanel, &pixels, &width, &height)
			|| ctx->IsError())
		{ op.Finish("failed", "rendered_capture_unavailable"); return; }
		png_image image = {};
		image.version = PNG_IMAGE_VERSION;
		image.width = width; image.height = height; image.format = PNG_FORMAT_RGBA;
		png_alloc_size_t size = 0;
		if (!png_image_write_to_memory(&image, nullptr, &size, 0, pixels.data(), 0, nullptr))
		{ png_image_free(&image); op.Finish("failed", "png_encode_failed"); return; }
		vector<unsigned char> png(size);
		bool ok = png_image_write_to_memory(&image, png.data(), &size, 0, pixels.data(), 0, nullptr) != 0;
		png_image_free(&image);
		if (!ok) { op.Finish("failed", "png_encode_failed"); return; }
		png.resize(size);
		op.SetImage({{"mimeType", "image/png"}, {"encoding", "base64"},
			{"width", width}, {"height", height}, {"data", GuiBase64(png)}});
		break;
	}
	case Action::Cancel:
	case Action::SetAddress:
	case Action::Submit:
	{
		// Bind to the identified popup, never whichever window happens to have focus.
		if (ImGui::GetCurrentContext()->OpenPopupStack.back().Window != editor)
		{ op.Finish("failed", "another_popup_blocks_editor"); return; }
		ctx->SetRef(editor->Name);
		if (action == Action::Cancel)
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
				if (!op.Continue(ctx)) return;
				if (GuiEditor() != editor || ImGui::GetCurrentContext()->OpenPopupStack.back().Window != editor)
				{ op.Finish("failed", "editor_changed_before_dismissal"); return; }
				ctx->MouseClick();
				clicked = true;
				break;
			}
			if (!clicked) { op.Finish("failed", "no_safe_popup_dismissal_point"); return; }
		}
		else
		{
			ctx->ItemInput("##addMemBreakpointPopupAddress_Vice");
			if (!op.Continue(ctx)) return;
			if (action == Action::SetAddress)
				ctx->KeyCharsReplace(op.request.text.c_str());
			else ctx->KeyPress(ImGuiKey_Enter);
		}
		ctx->Yield(3);
		if (!op.Continue(ctx)) return;
		if ((action == Action::Submit || action == Action::Cancel) && GuiEditor())
		{ op.Finish("failed", "editor_still_open"); return; }
		break;
	}
	case Action::Capabilities:
	case Action::Status:
		op.Finish("failed", "invalid_gui_arguments"); return; // Handled before queueing.
	}
	if (op.Continue(ctx)) op.Finish("completed");
}
}
#endif
