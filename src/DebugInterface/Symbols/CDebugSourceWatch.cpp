#include "CDebugSourceWatch.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugInterface.h"
#include "DebuggerDefs.h"
#include <cstdio>

CDebugSourceWatch::CDebugSourceWatch()
{
	representation = WATCH_REPRESENTATION_HEX_8;
	numberOfValues = 1;
	hasValue = false;
	live = false;
}

// a short why for a row without a value
static std::string StateOf(const CDebugVariableResolution &r)
{
	if (r.variable == NULL)
		return r.reason.compare(0, 12, "not in scope") == 0 ? "not in scope" : "not found";
	if (!r.bytes.empty())
		return "";
	switch (r.variable->location)
	{
		case DebugVariableLocation::FrameRelative:	return "frame not set up";
		case DebugVariableLocation::Register:		return "register, not readable";
		case DebugVariableLocation::Constant:		return "constant " + std::to_string(r.variable->constantValue);
		case DebugVariableLocation::Absent:			return "optimized out";
		default:									return "not available";
	}
}

void CDebugSourceWatch::Update(CDebugSymbols *symbols, bool paused)
{
	std::shared_ptr<const CDebugDatabase> database = symbols->debugDatabase;
	CDebugInterface *debugInterface = symbols->debugInterface;
	if (!database || !symbols->currentSegment || symbols->currentSegment->debugRegionId < 0)
	{
		value = CDebugVariableResolution();
		value.name = name;
		value.reason = "no debug info is loaded";
		hasValue = false;
		live = false;
		state = "no debug info";
		return;
	}
	u32 regionId = (u32)symbols->currentSegment->debugRegionId;
	u16 pc = (u16)debugInterface->GetCpuPC();

	if (!paused)
	{
		// only a global at a fixed address is the same bytes whatever runs
		if (!function.empty())
		{
			live = false;
			return;
		}
		CDebugVariableResolution r = DebugResolveVariableIn(database.get(), debugInterface, regionId, name, function, pc);
		if (r.variable == NULL || r.variable->location != DebugVariableLocation::Fixed)
		{
			live = false;
			return;
		}
		value = r;
		hasValue = true;
		live = true;
		state = StateOf(r);
		return;
	}

	value = DebugResolveVariableIn(database.get(), debugInterface, regionId, name, function, pc);
	hasValue = true;
	live = true;
	state = StateOf(value);
}

std::string CDebugSourceWatch::FormatValue() const
{
	const std::vector<u8> &b = value.bytes;
	if (b.empty())
		return "";
	auto le = [&](int n) -> u32 { u32 v = 0; for (int i = 0; i < n && i < (int)b.size(); i++) v |= (u32)b[i] << (8 * i); return v; };
	auto be = [&](int n) -> u32 { u32 v = 0; for (int i = 0; i < n; i++) v = (v << 8) | (i < (int)b.size() ? b[i] : 0); return v; };
	char buf[64];
	switch (representation)
	{
		default:
		case WATCH_REPRESENTATION_HEX_8:							snprintf(buf, sizeof(buf), "%02x", le(1)); break;
		case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:				snprintf(buf, sizeof(buf), "%04x", le(2)); break;
		case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:				snprintf(buf, sizeof(buf), "%04x", be(2)); break;
		case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:				snprintf(buf, sizeof(buf), "%08x", le(4)); break;
		case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:				snprintf(buf, sizeof(buf), "%08x", be(4)); break;
		case WATCH_REPRESENTATION_UNSIGNED_DEC_8:					snprintf(buf, sizeof(buf), "%u", le(1)); break;
		case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:	snprintf(buf, sizeof(buf), "%u", le(2)); break;
		case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:		snprintf(buf, sizeof(buf), "%u", be(2)); break;
		case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:	snprintf(buf, sizeof(buf), "%u", le(4)); break;
		case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:		snprintf(buf, sizeof(buf), "%u", be(4)); break;
		case WATCH_REPRESENTATION_SIGNED_DEC_8:						snprintf(buf, sizeof(buf), "%d", (i8)le(1)); break;
		case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:		snprintf(buf, sizeof(buf), "%d", (i16)le(2)); break;
		case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:			snprintf(buf, sizeof(buf), "%d", (i16)be(2)); break;
		case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:		snprintf(buf, sizeof(buf), "%d", (i32)le(4)); break;
		case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:			snprintf(buf, sizeof(buf), "%d", (i32)be(4)); break;
		case WATCH_REPRESENTATION_BIN:
		{
			u32 v = le(1);
			for (int i = 0; i < 8; i++)
				buf[i] = (v & (0x80 >> i)) ? '1' : '0';
			buf[8] = 0;
			break;
		}
	}
	return buf;
}

void CDebugSourceWatch::Serialize(Hjson::Value hjsonWatch) const
{
	hjsonWatch["Name"] = name;
	if (!function.empty())
		hjsonWatch["Function"] = function;
	hjsonWatch["Format"] = CDebugSymbolsDataWatch::RepresentationToStr(representation);
	if (numberOfValues != 1)
		hjsonWatch["Count"] = numberOfValues;
}

bool CDebugSourceWatch::Deserialize(Hjson::Value hjsonWatch)
{
	if (hjsonWatch["Name"].type() != Hjson::Type::String)
		return false;
	name = hjsonWatch["Name"].to_string();
	function = hjsonWatch["Function"].type() == Hjson::Type::String ? hjsonWatch["Function"].to_string() : std::string();
	if (hjsonWatch["Format"].type() == Hjson::Type::String)
		representation = CDebugSymbolsDataWatch::StrToRepresentation(hjsonWatch["Format"].to_string().c_str());
	if (hjsonWatch["Count"].type() == Hjson::Type::Int64)
		numberOfValues = (int)hjsonWatch["Count"].to_int64();
	return true;
}
