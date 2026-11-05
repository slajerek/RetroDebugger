#ifndef _GuiControlRequest_h_
#define _GuiControlRequest_h_

#include "CDebuggerServerProtocol.h"

namespace GuiControl
{
	enum class Action { Capabilities, Status, Show, Open, SetAddress, Submit, Cancel, Capture };
	enum class Target { MemoryBreakpoints, MemoryBreakpointsEditor };

	struct Request
	{
		Action action = Action::Capabilities;
		Target target = Target::MemoryBreakpoints;
		std::string text;
		std::string requestId;
	};

	// Keep the wire validator authoritative; normalize once before queueing.
	inline bool Parse(const nlohmann::json &args, Request &request)
	{
		if (!Valid(args)) return false;
		struct Entry { const char *name; Action action; };
		static const Entry actions[] = {
			{"capabilities", Action::Capabilities}, {"status", Action::Status},
			{"show", Action::Show}, {"open", Action::Open},
			{"set_address", Action::SetAddress}, {"submit", Action::Submit},
			{"cancel", Action::Cancel}, {"capture", Action::Capture}
		};
		Request parsed;
		for (const Entry &entry : actions)
		{
			if (args["action"] != entry.name) continue;
			parsed.action = entry.action;
			if (args.contains("target"))
				parsed.target = args["target"] == "c64.memory_breakpoints.editor"
					? Target::MemoryBreakpointsEditor : Target::MemoryBreakpoints;
			if (parsed.action == Action::SetAddress) parsed.text = args["text"].get<std::string>();
			if (parsed.action == Action::Status) parsed.requestId = args["requestId"].get<std::string>();
			request = parsed;
			return true;
		}
		return false;
	}
}

#endif
