#include "CDebugSourceBreakpoint.h"
#include "CDebugDatabase.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointAddr.h"
#include "CDebugInterface.h"
#include "DBG_Log.h"
#include <algorithm>

static std::string FileNameOf(const std::string &path)
{
	size_t slash = path.find_last_of("/\\");
	return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

static const CDebugSourceFile *FindSourceFile(const CDebugDatabase *database, const std::string &path)
{
	const CDebugSourceFile *byName = NULL;
	std::string name = FileNameOf(path);
	for (auto &it : database->Files())
	{
		const CDebugSourceFile *file = it.second;
		if (file->originalPath == path || (!file->loadedPath.empty() && file->loadedPath == path))
			return file;
		if (byName == NULL && file->fileName == name)
			byName = file;
	}
	return byName;
}

void DebugResolveSourceLine(const CDebugDatabase *database, const std::string &path, i32 line,
							i32 *resolvedLine, std::vector<const CDebugSourceMapping *> &out)
{
	out.clear();
	*resolvedLine = 0;
	if (database == NULL || line <= 0)
		return;

	const CDebugSourceFile *file = FindSourceFile(database, path);
	if (file == NULL)
		return;

	// A line without code slides forward, but not into the next function.
	i32 limit = line + 1000;
	for (const CDebugFunction &function : database->Functions())
	{
		if (function.fileId == file->id && function.line > line && function.line < limit)
			limit = function.line;
	}

	std::vector<const CDebugSourceMapping *> mappings;
	for (i32 candidate = line; candidate < limit; candidate++)
	{
		database->ResolveSource(file->id, candidate, mappings);
		for (const CDebugSourceMapping *m : mappings)
		{
			// in effect at its first address, and not the continuation of
			// another range of the same line
			if (database->Resolve(m->regionId, m->start) != m)
				continue;
			bool continuation = false;
			for (const CDebugSourceMapping *other : mappings)
			{
				if (other != m && other->regionId == m->regionId && other->end == m->start)
					continuation = true;
			}
			if (!continuation)
				out.push_back(m);
		}
		if (!out.empty())
		{
			*resolvedLine = candidate;
			return;
		}
	}
}

// --- CDebugSymbols: the source breakpoint list ---

CDebugSourceBreakpoint *CDebugSymbols::FindSourceBreakpoint(const char *path, i32 line)
{
	for (CDebugSourceBreakpoint *breakpoint : sourceBreakpoints)
	{
		if (breakpoint->path == path && breakpoint->line == line)
			return breakpoint;
	}
	return NULL;
}

CDebugSourceBreakpoint *CDebugSymbols::AddSourceBreakpoint(const char *path, i32 line)
{
	debugInterface->LockMutex();
	CDebugSourceBreakpoint *breakpoint = FindSourceBreakpoint(path, line);
	if (breakpoint == NULL)
	{
		breakpoint = new CDebugSourceBreakpoint();
		breakpoint->path = path;
		breakpoint->line = line;
		sourceBreakpoints.push_back(breakpoint);
		BindSourceBreakpoint(breakpoint);
	}
	debugInterface->UnlockMutex();
	return breakpoint;
}

void CDebugSymbols::RemoveSourceBreakpoint(CDebugSourceBreakpoint *breakpoint)
{
	debugInterface->LockMutex();
	auto it = std::find(sourceBreakpoints.begin(), sourceBreakpoints.end(), breakpoint);
	if (it != sourceBreakpoints.end())
	{
		UnbindSourceBreakpoint(breakpoint);
		sourceBreakpoints.erase(it);
		delete breakpoint;
	}
	debugInterface->UnlockMutex();
}

void CDebugSymbols::SetDebugDatabase(std::shared_ptr<const CDebugDatabase> database)
{
	debugInterface->LockMutex();
	for (CDebugSourceBreakpoint *breakpoint : sourceBreakpoints)
		UnbindSourceBreakpoint(breakpoint);
	this->debugDatabase = database;
	for (CDebugSourceBreakpoint *breakpoint : sourceBreakpoints)
		BindSourceBreakpoint(breakpoint);
	debugInterface->UnlockMutex();
}

void CDebugSymbols::UnbindSourceBreakpoint(CDebugSourceBreakpoint *breakpoint)
{
	for (const CDebugSourceBreakpointBinding &binding : breakpoint->bindings)
	{
		// a segment a reload replaced is gone from the list, with its breakpoints
		if (std::find(segments.begin(), segments.end(), binding.segment) == segments.end())
			continue;
		if (!binding.created)
		{
			// RD-025: this bank's condition comes off the breakpoint it joined
			CDebugBreakpointAddr *joined = binding.banked ? binding.segment->breakpointsPC->GetBreakpoint(binding.address) : NULL;
			if (joined)
			{
				auto it = std::find(joined->imageOffsets.begin(), joined->imageOffsets.end(), binding.imageOffset);
				if (it != joined->imageOffsets.end())
					joined->imageOffsets.erase(it);
			}
			continue;
		}

		// another source breakpoint on the same address takes the ownership over
		bool shared = false;
		for (CDebugSourceBreakpoint *other : sourceBreakpoints)
		{
			if (other == breakpoint)
				continue;
			for (CDebugSourceBreakpointBinding &otherBinding : other->bindings)
			{
				if (!shared && otherBinding.segment == binding.segment && otherBinding.address == binding.address)
				{
					otherBinding.created = true;
					shared = true;
				}
			}
		}
		if (!shared)
		{
			binding.segment->breakpointsPC->DeleteBreakpoint(binding.address);
		}
		else if (binding.banked)
		{
			// the other bank's source breakpoint keeps the address breakpoint
			CDebugBreakpointAddr *addressBreakpoint = binding.segment->breakpointsPC->GetBreakpoint(binding.address);
			if (addressBreakpoint)
			{
				auto &offsets = addressBreakpoint->imageOffsets;
				auto it = std::find(offsets.begin(), offsets.end(), binding.imageOffset);
				if (it != offsets.end())
					offsets.erase(it);
			}
		}
	}
	breakpoint->bindings.clear();
	breakpoint->resolvedLine = 0;
}

void CDebugSymbols::BindSourceBreakpoint(CDebugSourceBreakpoint *breakpoint)
{
	std::vector<const CDebugSourceMapping *> mappings;
	i32 resolvedLine = 0;
	DebugResolveSourceLine(debugDatabase.get(), breakpoint->path, breakpoint->line, &resolvedLine, mappings);

	for (const CDebugSourceMapping *m : mappings)
	{
		CDebugSymbolsSegment *segment = NULL;
		for (CDebugSymbolsSegment *candidate : segments)
		{
			if (candidate->debugRegionId == (int)m->regionId)
			{
				segment = candidate;
				break;
			}
		}
		if (segment == NULL || segment->breakpointsPC == NULL)
			continue;

		CDebugSourceBreakpointBinding binding;
		binding.segment = segment;
		binding.address = (int)m->start;
		// RD-025: where banks share the address, only this bank's bytes stop
		binding.banked = m->hasImageOffset && debugDatabase->IsBankedAddress(m->regionId, m->start);
		binding.imageOffset = m->imageOffset;
		CDebugBreakpointAddr *existing = segment->breakpointsPC->GetBreakpoint(binding.address);
		binding.created = (existing == NULL);
		if (binding.created)
		{
			segment->AddBreakpointPC(binding.address);
			if (binding.banked)
				segment->breakpointsPC->GetBreakpoint(binding.address)->imageOffsets.push_back(binding.imageOffset);
		}
		else if (binding.banked && !existing->imageOffsets.empty())
		{
			// another bank's source breakpoint made it: this bank stops there too
			existing->imageOffsets.push_back(binding.imageOffset);
		}
		breakpoint->bindings.push_back(binding);
	}
	if (!breakpoint->bindings.empty())
		breakpoint->resolvedLine = resolvedLine;

	LOGD("CDebugSymbols: source breakpoint %s:%d -> line %d, %d address(es)", breakpoint->path.c_str(),
		 breakpoint->line, breakpoint->resolvedLine, (int)breakpoint->bindings.size());
}
