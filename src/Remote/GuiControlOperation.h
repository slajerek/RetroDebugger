#ifndef _GuiControlOperation_h_
#define _GuiControlOperation_h_

#include "GuiControlRequest.h"
#include <chrono>

struct ImGuiTestContext;

namespace GuiControl
{
	using GuiClock = std::chrono::steady_clock;

	// Internal operation shared by the driver and the one allowlisted handler.
	// Lifecycle and result writes remain owned by the GUI-control module.
	struct Operation
	{
		std::string id;
		Request request;
		nlohmann::json result;
		GuiClock::time_point deadline;
		bool terminal = false;
		bool started = false;
		bool engineQueued = false;
		bool finished = false;

		void Finish(const char *outcome, const char *error = nullptr);
		bool Continue(ImGuiTestContext *ctx);
		void SetImage(const nlohmann::json &image);
	};

	void RunMemoryBreakpoints(ImGuiTestContext *ctx, Operation &op);
}

#endif
