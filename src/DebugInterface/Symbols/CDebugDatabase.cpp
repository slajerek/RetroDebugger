#include "CDebugDatabase.h"
#include "CSlrString.h"
#include "DBG_Log.h"
#include <algorithm>
#include <cstring>

CDebugSourceFile::CDebugSourceFile()
{
	id = 0;
	fileNameSlr = NULL;
}

CDebugSourceFile::~CDebugSourceFile()
{
	delete fileNameSlr;
	for (CSlrString *line : lines)
		delete line;
}

CDebugBlock::CDebugBlock(const std::string &name, u32 regionId)
{
	this->name = name;
	this->nameSlr = new CSlrString(name.c_str());
	this->regionId = regionId;
}

CDebugBlock::~CDebugBlock()
{
	delete nameSlr;
}

CDebugDatabase::CDebugDatabase(const char *producer)
{
	this->producer = producer;
	this->finalized = false;
}

CDebugDatabase::~CDebugDatabase()
{
	for (auto &it : files)
		delete it.second;
	for (CDebugBlock *block : blocks)
		delete block;
}

CDebugSourceFile *CDebugDatabase::AddFile(u32 producerFileId, const std::string &originalPath)
{
	CDebugSourceFile *file = new CDebugSourceFile();
	file->id = producerFileId;
	file->originalPath = originalPath;
	size_t slash = originalPath.find_last_of("/\\");
	file->fileName = (slash == std::string::npos) ? originalPath : originalPath.substr(slash + 1);
	file->fileNameSlr = new CSlrString(file->fileName.c_str());

	auto it = files.find(producerFileId);
	if (it != files.end())
	{
		LOGError("CDebugDatabase: file id %u defined twice, keeping the last", producerFileId);
		delete it->second;
	}
	files[producerFileId] = file;
	return file;
}

u32 CDebugDatabase::AddRegion(const std::string &name, const std::string &dest)
{
	CDebugRegion region;
	region.name = name;
	region.dest = dest;
	regions.push_back(region);
	return (u32)regions.size() - 1;
}

u32 CDebugDatabase::AddBlock(const std::string &name, u32 regionId)
{
	blocks.push_back(new CDebugBlock(name, regionId));
	return (u32)blocks.size() - 1;
}

void CDebugDatabase::AddMapping(const CDebugSourceMapping &mapping)
{
	if (finalized || mapping.end <= mapping.start || mapping.regionId >= regions.size())
	{
		LOGError("CDebugDatabase::AddMapping: rejected [%x,%x) region %u", mapping.start, mapping.end, mapping.regionId);
		return;
	}
	mappings.push_back(mapping);
}

u32 CDebugDatabase::AddFunction(const CDebugFunction &function)
{
	if (finalized || function.end <= function.start || function.regionId >= regions.size())
	{
		LOGError("CDebugDatabase::AddFunction: rejected %s [%x,%x)", function.name.c_str(), function.start, function.end);
		return (u32)-1;
	}
	functions.push_back(function);
	return (u32)functions.size() - 1;
}

void CDebugDatabase::Finalize()
{
	if (finalized)
		return;

	functionsByStart.resize(regions.size());
	for (u32 index = 0; index < functions.size(); index++)
		functionsByStart[functions[index].regionId].push_back(index);
	for (std::vector<u32> &byStart : functionsByStart)
	{
		std::sort(byStart.begin(), byStart.end(),
				  [this](u32 a, u32 b) { return functions[a].start < functions[b].start; });
	}

	// Paint each region's ranges in producer order: a later range replaces
	// whatever earlier ranges covered, splitting them where it overlaps only
	// part of one. The result is what a per-address array filled in file
	// order used to hold, without one pointer per address.
	std::vector<std::map<u32, Interval>> painted(regions.size());
	for (u32 index = 0; index < mappings.size(); index++)
	{
		const CDebugSourceMapping &m = mappings[index];
		std::map<u32, Interval> &intervals = painted[m.regionId];

		auto it = intervals.lower_bound(m.start);
		if (it != intervals.begin())
		{
			auto prev = std::prev(it);
			if (prev->second.end > m.start)
				it = prev;
		}
		while (it != intervals.end() && it->second.start < m.end)
		{
			Interval old = it->second;
			it = intervals.erase(it);
			if (old.start < m.start)
				intervals[old.start] = Interval{ old.start, m.start, old.mapping };
			if (old.end > m.end)
				intervals[m.end] = Interval{ m.end, old.end, old.mapping };
		}
		intervals[m.start] = Interval{ m.start, m.end, index };

		bySourceLine[std::make_pair(m.fileId, m.lineStart)].push_back(index);
	}

	effective.resize(regions.size());
	for (size_t r = 0; r < regions.size(); r++)
	{
		effective[r].reserve(painted[r].size());
		for (auto &it : painted[r])
			effective[r].push_back(it.second);
	}
	finalized = true;
}

const CDebugSourceMapping *CDebugDatabase::Resolve(u32 regionId, u32 address) const
{
	if (!finalized || regionId >= effective.size())
		return NULL;

	const std::vector<Interval> &intervals = effective[regionId];
	auto it = std::upper_bound(intervals.begin(), intervals.end(), address,
							   [](u32 a, const Interval &interval) { return a < interval.start; });
	if (it == intervals.begin())
		return NULL;
	--it;
	return (address < it->end) ? &mappings[it->mapping] : NULL;
}

void CDebugDatabase::ResolveCandidates(u32 regionId, u32 address, std::vector<const CDebugSourceMapping *> &out) const
{
	out.clear();
	for (const CDebugSourceMapping &m : mappings)
	{
		if (m.regionId == regionId && address >= m.start && address < m.end)
			out.push_back(&m);
	}
}

void CDebugDatabase::ResolveSource(u32 fileId, i32 line, std::vector<const CDebugSourceMapping *> &out) const
{
	out.clear();
	auto it = bySourceLine.find(std::make_pair(fileId, line));
	if (it == bySourceLine.end())
		return;
	for (u32 index : it->second)
		out.push_back(&mappings[index]);
}

const CDebugFunction *CDebugDatabase::FindFunction(u32 regionId, u32 address) const
{
	if (!finalized || regionId >= functionsByStart.size())
		return NULL;

	const std::vector<u32> &byStart = functionsByStart[regionId];
	auto it = std::upper_bound(byStart.begin(), byStart.end(), address,
							   [this](u32 a, u32 index) { return a < functions[index].start; });
	if (it == byStart.begin())
		return NULL;
	const CDebugFunction &function = functions[*std::prev(it)];
	return address < function.end ? &function : NULL;
}

int CDebugDatabase::FindRegion(const char *name) const
{
	for (size_t r = 0; r < regions.size(); r++)
	{
		if (regions[r].name == name)
			return (int)r;
	}
	return -1;
}

const CDebugSourceFile *CDebugDatabase::GetFile(u32 producerFileId) const
{
	auto it = files.find(producerFileId);
	return it == files.end() ? NULL : it->second;
}

const CDebugRegion *CDebugDatabase::GetRegion(u32 regionId) const
{
	return regionId < regions.size() ? &regions[regionId] : NULL;
}

const CDebugBlock *CDebugDatabase::GetBlock(u32 blockId) const
{
	return blockId < blocks.size() ? blocks[blockId] : NULL;
}
