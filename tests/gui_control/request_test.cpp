#include <cstdint>
using u8 = uint8_t;
#include "GuiControlRequest.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>

using GuiControl::Action;
using GuiControl::Target;
using nlohmann::json;

static void Check(bool condition, const char *message)
{
	if (!condition) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
	try
	{
		Check(argc == 2, "Pass the unchanged protocol_test schema-cases.json output");
		std::ifstream input(argv[1]);
		json corpus;
		input >> corpus;
		const std::map<std::string, Action> actions = {
			{"capabilities", Action::Capabilities}, {"status", Action::Status},
			{"show", Action::Show}, {"open", Action::Open},
			{"set_address", Action::SetAddress}, {"submit", Action::Submit},
			{"cancel", Action::Cancel}, {"capture", Action::Capture}
		};
		std::map<Action, int> covered;
		int count = 0;
		for (const auto &item : corpus.at("cases"))
		{
			const json &args = item.at("args");
			GuiControl::Request request;
			request.action = Action::Submit;
			request.target = Target::MemoryBreakpointsEditor;
			request.text = "unchanged";
			request.requestId = "987";
			bool valid = GuiControl::Parse(args, request);
			Check(valid == item.at("valid").get<bool>(), "normalization changed acceptance");
			if (!valid)
			{
				Check(request.action == Action::Submit && request.target == Target::MemoryBreakpointsEditor
					&& request.text == "unchanged" && request.requestId == "987", "rejection mutated request");
			}
			else
			{
				Check(request.action == actions.at(args.at("action").get<std::string>()), "wrong typed action");
				Target target = args.value("target", "") == "c64.memory_breakpoints.editor"
					? Target::MemoryBreakpointsEditor : Target::MemoryBreakpoints;
				Check(request.target == target, "wrong typed target");
				Check(request.text == args.value("text", ""), "text changed or leaked from previous request");
				Check(request.requestId == args.value("requestId", ""), "requestId changed or leaked");
				++covered[request.action];
			}
			++count;
		}
		Check(count == 83 && covered.size() == actions.size(), "missing original cases or action coverage");
		std::puts("PASS: typed normalization of 83 unchanged cases, all 8 actions, and rejection immutability");
		return 0;
	}
	catch (const std::exception &error)
	{
		std::fprintf(stderr, "%s\n", error.what());
		return 1;
	}
}
