#ifndef _C_DRIVE1541_CONTROL_H_
#define _C_DRIVE1541_CONTROL_H_

#include "json.hpp"

// Shared input contract only; no emulator state or platform dependency.
namespace Drive1541Control
{
	inline bool ParsePause(const nlohmann::json &params, bool tool, int &timeoutMs)
	{
		if (!params.is_object()) return false;
		if (tool && (!params.contains("platform") || params.at("platform") != "c64")) return false;
		for (auto it = params.begin(); it != params.end(); ++it)
			if (it.key() != "device" && it.key() != "timeoutMs" && !(tool && it.key() == "platform")) return false;
		if (params.contains("device") && (!params.at("device").is_number_integer() || params.at("device") != 8)) return false;
		timeoutMs = 1000;
		if (params.contains("timeoutMs"))
		{
			const auto &value = params.at("timeoutMs");
			if (!value.is_number_integer() || value < 0 || value > 5000) return false;
			timeoutMs = value.get<int>();
		}
		return true;
	}

	inline bool ParseRunUntil(const nlohmann::json &params, bool tool, int &address, int &timeoutMs)
	{
		if (!params.is_object() || !params.contains("address")) return false;
		const auto &value = params.at("address");
		if (!value.is_number_integer() || value < 0 || value > 65535) return false;
		auto control = params;
		control.erase("address");
		if (!ParsePause(control, tool, timeoutMs) || timeoutMs == 0) return false;
		address = value.get<int>();
		return true;
	}
}

#endif
