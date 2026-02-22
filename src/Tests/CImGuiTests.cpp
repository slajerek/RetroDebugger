#ifdef ENABLE_IMGUI_TEST_ENGINE

#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "CViewC64.h"
#include "CDebugInterface.h"

void RegisterRetroDebuggerTests(ImGuiTestEngine *engine)
{
	// Test: Verify main menu bar items exist
	ImGuiTest *t = IM_REGISTER_TEST(engine, "ui", "main_menu_bar_items");
	t->TestFunc = [](ImGuiTestContext *ctx)
	{
		ctx->SetRef("##MainMenuBar");
		ctx->MenuCheck("File");
		ctx->MenuCheck("Code");
		ctx->MenuCheck("Workspace");
		ctx->MenuCheck("Settings");
		ctx->MenuCheck("Help");
	};

	// Test: Open all views from all emulators to catch rendering crashes
	ImGuiTest *t2 = IM_REGISTER_TEST(engine, "ui", "open_all_views");
	t2->TestFunc = [](ImGuiTestContext *ctx)
	{
		// 1. Save initial emulator running states
		std::vector<bool> wasRunning;
		for (auto *di : viewC64->debugInterfaces)
			wasRunning.push_back(di->isRunning);

		// 2. Start all emulators that aren't running
		for (auto *di : viewC64->debugInterfaces)
		{
			if (!di->isRunning)
				viewC64->StartEmulationThread(di);
		}

		// Wait for emulators to initialize
		ctx->Yield(30);

		// 3. For each emulator, show each view, render frames, hide view
		for (auto *di : viewC64->debugInterfaces)
		{
			if (!di->isRunning) continue;

			for (auto *view : di->views)
			{
				bool wasVisible = view->visible;
				view->SetVisible(true);
				ctx->Yield(5);  // render 5 frames to catch crashes
				view->SetVisible(wasVisible);  // restore original visibility
			}
		}

		// 4. Restore emulator running states
		for (size_t i = 0; i < viewC64->debugInterfaces.size(); i++)
		{
			CDebugInterface *di = viewC64->debugInterfaces[i];
			if (di->isRunning && !wasRunning[i])
				viewC64->StopEmulationThread(di);
		}

		ctx->Yield(10);  // let cleanup settle
	};
}

#endif // ENABLE_IMGUI_TEST_ENGINE
