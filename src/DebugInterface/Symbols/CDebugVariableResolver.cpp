#include "CDebugVariableResolver.h"
#include "CDebugInterface.h"
#include "CDebugDataAdapter.h"
#include <algorithm>

const char *DebugVariableLocationName(DebugVariableLocation location)
{
	switch (location)
	{
		case DebugVariableLocation::Fixed:			return "fixed";
		case DebugVariableLocation::FixedScoped:	return "static stack";
		case DebugVariableLocation::ZpParam:		return "zero-page parameter";
		case DebugVariableLocation::FrameRelative:	return "stack frame";
		case DebugVariableLocation::Register:		return "register";
		case DebugVariableLocation::Constant:		return "constant";
		case DebugVariableLocation::Absent:			return "optimized out";
	}
	return "?";
}

std::string DebugTypeName(const CDebugDatabase *database, u32 typeId)
{
	const CDebugType *type = database ? database->GetType(typeId) : NULL;
	if (type == NULL)
		return "?";
	if (!type->name.empty())
		return type->name;
	switch (type->kind)
	{
		case DebugTypeKind::UInt:		return "u" + std::to_string(type->size * 8);
		case DebugTypeKind::SInt:		return "i" + std::to_string(type->size * 8);
		case DebugTypeKind::Pointer:	return DebugTypeName(database, type->elementType) + "*";
		case DebugTypeKind::Array:		return DebugTypeName(database, type->elementType) + "[" + std::to_string(type->count) + "]";
		case DebugTypeKind::Struct:		return "struct";
		case DebugTypeKind::Function:	return "function";
		case DebugTypeKind::Unknown:	break;
	}
	return "?";
}

static CDebugVariableResolution Resolve(const CDebugDatabase *database, CDebugInterface *debugInterface,
										const CDebugVariable &variable, const CDebugFunction *function,
										const CDebugSourceMapping *mapping, u16 pc)
{
	CDebugVariableResolution r;
	r.variable = &variable;
	r.name = variable.name;
	r.function = variable.functionIndex >= 0 ? database->Functions()[variable.functionIndex].name : std::string();
	r.type = DebugTypeName(database, variable.typeId);
	r.size = variable.size;
	if (r.size == 0)
	{
		const CDebugType *type = database->GetType(variable.typeId);
		if (type)
			r.size = type->size;
	}
	CDebugDataAdapter *memory = debugInterface->GetDataAdapter();
	auto readByte = [&](u32 address) -> u8 { u8 v = 0; bool available = false; memory->AdapterReadByte((int)address, &v, &available); return v; };

	switch (variable.location)
	{
		case DebugVariableLocation::Fixed:
			r.hasAddress = true;
			r.address = variable.address;
			r.quality = variable.sizeApproximate ? DebugMappingQuality::Approximate : DebugMappingQuality::Exact;
			if (variable.sizeApproximate)
				r.reason = "the size is not recorded: up to the next label";
			break;

		case DebugVariableLocation::FixedScoped:
			r.hasAddress = true;
			r.address = variable.address;
			if (mapping && mapping->lineStart >= variable.enterLine && mapping->lineStart <= variable.leaveLine)
				r.quality = DebugMappingQuality::Exact;
			else
			{
				r.quality = DebugMappingQuality::Approximate;
				r.reason = "outside its lines " + std::to_string(variable.enterLine) + "-" + std::to_string(variable.leaveLine)
					+ ": the static stack may hold another value";
			}
			break;

		case DebugVariableLocation::ZpParam:
			r.hasAddress = true;
			r.address = variable.address;
			if (function && pc == function->start)
				r.quality = DebugMappingQuality::Exact;
			else
			{
				r.quality = DebugMappingQuality::Approximate;
				r.reason = "a zero-page parameter register: certain at the function's entry, the compiler may reuse it after";
			}
			break;

		case DebugVariableLocation::FrameRelative:
		{
			if (variable.baseZp > 0xFF)
			{
				r.reason = "the stack pointer's address is not in the debug info";
				break;
			}
			if (mapping == NULL)
			{
				r.reason = "the PC is in the prologue: the frame is not set up";
				break;
			}
			u32 sp = readByte(variable.baseZp) | (readByte(variable.baseZp + 1) << 8);
			r.hasAddress = true;
			r.address = (sp + variable.address) & 0xFFFF;
			if (variable.frameExactAtLineStarts && pc == mapping->start)
				r.quality = DebugMappingQuality::Exact;
			else
			{
				r.quality = DebugMappingQuality::Approximate;
				r.reason = variable.frameExactAtLineStarts
					? "between line starts the stack pointer may move"
					: "the stack depth at this PC is not recorded: valid between the function's own pushes and a call's argument pushes";
			}
			break;
		}

		case DebugVariableLocation::Register:
			r.reason = "held in a CPU register";
			break;
		case DebugVariableLocation::Constant:
			r.reason = "a folded constant: no memory location";
			break;
		case DebugVariableLocation::Absent:
			r.reason = "optimized out";
			break;
	}

	if (r.hasAddress)
	{
		u32 count = r.size == 0 ? 1 : std::min<u32>(r.size, 64);
		for (u32 i = 0; i < count; i++)
			r.bytes.push_back(readByte((r.address + i) & 0xFFFF));
	}
	return r;
}

// the PC's function's index (-1 when none) and the mapping in effect at the PC
static i32 FunctionAt(const CDebugDatabase *database, CDebugInterface *debugInterface, u32 regionId, u16 pc,
					  const CDebugFunction **function, const CDebugSourceMapping **mapping)
{
	*function = debugInterface->FindFunctionAt(database, regionId, pc);
	*mapping = debugInterface->ResolveSourceAt(database, regionId, pc);
	if (*mapping && *function && ((*mapping)->start < (*function)->start || (*mapping)->start >= (*function)->end))
		*mapping = NULL;		// another function's line: not this frame's body
	return *function ? (i32)(*function - &database->Functions()[0]) : -1;
}

CDebugVariableResolution DebugResolveVariable(const CDebugDatabase *database, CDebugInterface *debugInterface,
											  u32 regionId, const std::string &name, u16 pc)
{
	CDebugVariableResolution missing;
	missing.name = name;
	if (database == NULL)
	{
		missing.reason = "no debug info is loaded";
		return missing;
	}
	const CDebugFunction *function = NULL;
	const CDebugSourceMapping *mapping = NULL;
	i32 functionIndex = FunctionAt(database, debugInterface, regionId, pc, &function, &mapping);
	const CDebugVariable *global = NULL;
	for (const CDebugVariable &variable : database->Variables())
	{
		if (variable.name != name)
			continue;
		if (variable.functionIndex >= 0 && variable.functionIndex == functionIndex)
			return Resolve(database, debugInterface, variable, function, mapping, pc);
		if (variable.functionIndex < 0 && global == NULL)
			global = &variable;
	}
	if (global)
		return Resolve(database, debugInterface, *global, function, mapping, pc);
	bool elsewhere = false;
	for (const CDebugVariable &variable : database->Variables())
		elsewhere = elsewhere || variable.name == name;
	missing.reason = elsewhere ? "a local of another function: not in scope here"
		: (database->capabilities.variables || database->capabilities.localVariables
		   ? "no such variable" : "the debug info records no variables");
	return missing;
}

std::vector<CDebugVariableResolution> DebugVariablesInScope(const CDebugDatabase *database, CDebugInterface *debugInterface,
															u32 regionId, u16 pc)
{
	std::vector<CDebugVariableResolution> out;
	if (database == NULL)
		return out;
	const CDebugFunction *function = NULL;
	const CDebugSourceMapping *mapping = NULL;
	i32 functionIndex = FunctionAt(database, debugInterface, regionId, pc, &function, &mapping);
	for (const CDebugVariable &variable : database->Variables())
		if (functionIndex >= 0 && variable.functionIndex == functionIndex)
			out.push_back(Resolve(database, debugInterface, variable, function, mapping, pc));
	for (const CDebugVariable &variable : database->Variables())
		if (variable.functionIndex < 0)
			out.push_back(Resolve(database, debugInterface, variable, function, mapping, pc));
	return out;
}
