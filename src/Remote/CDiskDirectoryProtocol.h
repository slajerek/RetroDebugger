#ifndef _CDISKDIRECTORYPROTOCOL_H_
#define _CDISKDIRECTORYPROTOCOL_H_

#include "json.hpp"

// Display is deliberately ASCII-only and lossy; raw PETSCII is authoritative.
inline nlohmann::json DirectoryName(const unsigned char *bytes, size_t size)
{
	std::string display;
	size_t end = size;
	while (end && (bytes[end-1] == 0xa0 || bytes[end-1] == 0x20)) --end;
	for (size_t i = 0; i < end; ++i)
		display += bytes[i] >= 0x20 && bytes[i] <= 0x7e ? (char)bytes[i] : '?';
	return {{"display", display}, {"petscii", std::vector<unsigned char>(bytes, bytes+size)}};
}

// Shared by the native endpoint and MCP validation, not a transport coercion.
inline bool ParseDiskDirectoryDevice(const nlohmann::json &params, int &device)
{
	device = 8;
	if (!params.is_object()) return false;
	if (params.contains("platform") && params.at("platform") != "c64") return false;
	if (!params.contains("device")) return true;
	const auto &value = params.at("device");
	if (!value.is_number_integer() || value < 8 || value > 11) return false;
	device = value.get<int>();
	return true;
}

#endif
