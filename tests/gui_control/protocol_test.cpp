// Portable checks of the production parser and published schema.
#include <cstdint>
using u8 = uint8_t;
#include "CDebuggerServerProtocol.h"
#include <cassert>
#include <iostream>
using nlohmann::json;

int main()
{
	json cases = json::array();
	auto check = [&](json args, bool valid)
	{
		assert(GuiControl::Valid(args) == valid);
		cases.push_back({{"args", args}, {"valid", valid}});
	};
	check({{"action", "capabilities"}}, true);
	check({{"action", "status"}, {"requestId", "1"}}, true);
	for (const char *action : {"show", "open", "set_address", "submit", "cancel", "capture"})
		for (const char *target : {"c64.memory_breakpoints", "c64.memory_breakpoints.editor", "C64 Screen", "//$FOCUSED"})
		{
			json args = {{"action", action}, {"target", target}};
			bool editor = std::string(target) == "c64.memory_breakpoints.editor";
			bool panel = std::string(target) == "c64.memory_breakpoints";
			if (std::string(action) == "set_address") args["text"] = "ce00";
			bool valid = (std::string(action) == "show" || std::string(action) == "open") ? panel
				: std::string(action) == "capture" ? (panel || editor) : editor;
			check(args, valid);
			args["extra"] = true;
			check(args, false);
		}
	for (const auto &text : json::array({nullptr, true, 1, "", "a\n", "a\t", std::string(65, 'a'), std::string(1, '\0'), "é"}))
		check({{"action", "set_address"}, {"target", "c64.memory_breakpoints.editor"}, {"text", text}}, false);
	for (const auto &text : json::array({"a", "ce00", "label with spaces", std::string(64, '~')}))
		check({{"action", "set_address"}, {"target", "c64.memory_breakpoints.editor"}, {"text", text}}, true);
	for (const auto &id : json::array({nullptr, true, 1, 1.5, "", "a", "-1", "1\n", std::string(21, '1')}))
		check({{"action", "status"}, {"requestId", id}}, false);
	for (const auto &bad : json::array({nullptr, true, 1, json::array(), json::object(),
		{{"action", "key"}, {"key", "Enter"}}, {{"action", 1}},
		{{"action", "show"}, {"target", "c64.memory_breakpoints"}, {"text", "ce00"}},
		{{"action", "capabilities"}, {"target", "c64.memory_breakpoints"}},
		{{"action", "status"}, {"requestId", "1"}, {"target", "c64.memory_breakpoints"}},
		{{"action", "capture"}, {"target", "c64.memory_breakpoints"}, {"path", "out.png"}}}))
		check(bad, false);
	std::cout << json({{"schema", GuiControl::Schema()}, {"cases", cases}}).dump() << std::endl;
}
