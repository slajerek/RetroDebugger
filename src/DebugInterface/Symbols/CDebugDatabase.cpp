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

u32 CDebugDatabase::AddInlineChain(const std::vector<CDebugInlineSite> &chain)
{
	inlineChains.push_back(chain);
	return (u32)inlineChains.size() - 1;
}

const std::vector<CDebugInlineSite> &CDebugDatabase::InlineChain(const CDebugSourceMapping *mapping) const
{
	static const std::vector<CDebugInlineSite> kNone;
	if (mapping == NULL || mapping->inlineChain == kNoInlineChain || mapping->inlineChain >= inlineChains.size())
		return kNone;
	return inlineChains[mapping->inlineChain];
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

	// the same painting over image offsets, for mappings that have them
	std::map<u64, ImageInterval> imagePainted;
	for (u32 index = 0; index < mappings.size(); index++)
	{
		const CDebugSourceMapping &m = mappings[index];
		if (!m.hasImageOffset || m.end <= m.start)
			continue;
		u64 start = m.imageOffset, end = m.imageOffset + (m.end - m.start);
		auto it = imagePainted.lower_bound(start);
		if (it != imagePainted.begin())
		{
			auto prev = std::prev(it);
			if (prev->second.end > start)
				it = prev;
		}
		while (it != imagePainted.end() && it->second.start < end)
		{
			ImageInterval old = it->second;
			it = imagePainted.erase(it);
			if (old.start < start)
				imagePainted[old.start] = ImageInterval{ old.start, start, old.mapping };
			if (old.end > end)
				imagePainted[end] = ImageInterval{ end, old.end, old.mapping };
		}
		imagePainted[start] = ImageInterval{ start, end, index };
	}
	for (auto &it : imagePainted)
		imageEffective.push_back(it.second);

	// CPU ranges where two banks' code overlap (different image offset for
	// the same address)
	bankedRanges.resize(regions.size());
	if (!imageEffective.empty())
	{
		for (size_t r = 0; r < regions.size(); r++)
		{
			std::vector<const CDebugSourceMapping *> withImage;
			for (const CDebugSourceMapping &m : mappings)
			{
				if (m.regionId == r && m.hasImageOffset && m.end > m.start)
					withImage.push_back(&m);
			}
			std::sort(withImage.begin(), withImage.end(),
					  [](const CDebugSourceMapping *a, const CDebugSourceMapping *b) { return a->start < b->start; });
			std::vector<std::pair<u32, u32>> overlaps;
			for (size_t i = 0; i < withImage.size(); i++)
			{
				const CDebugSourceMapping *a = withImage[i];
				for (size_t j = i + 1; j < withImage.size() && withImage[j]->start < a->end; j++)
				{
					const CDebugSourceMapping *b = withImage[j];
					if ((i64)a->imageOffset - (i64)a->start != (i64)b->imageOffset - (i64)b->start)
						overlaps.push_back(std::make_pair(b->start, std::min(a->end, b->end)));
				}
			}
			std::sort(overlaps.begin(), overlaps.end());
			for (const auto &o : overlaps)
			{
				if (!bankedRanges[r].empty() && o.first <= bankedRanges[r].back().second)
					bankedRanges[r].back().second = std::max(bankedRanges[r].back().second, o.second);
				else
					bankedRanges[r].push_back(o);
			}
		}
	}
	finalized = true;
}

bool CDebugDatabase::IsBankedAddress(u32 regionId, u32 address) const
{
	if (regionId >= bankedRanges.size())
		return false;
	const std::vector<std::pair<u32, u32>> &ranges = bankedRanges[regionId];
	auto it = std::upper_bound(ranges.begin(), ranges.end(), address,
							   [](u32 a, const std::pair<u32, u32> &range) { return a < range.first; });
	if (it == ranges.begin())
		return false;
	--it;
	return address < it->second;
}

const CDebugSourceMapping *CDebugDatabase::ResolveImage(u32 regionId, u32 address, bool hasImageOffset, u64 imageOffset) const
{
	if (hasImageOffset && finalized && !imageEffective.empty())
	{
		auto it = std::upper_bound(imageEffective.begin(), imageEffective.end(), imageOffset,
								   [](u64 a, const ImageInterval &interval) { return a < interval.start; });
		if (it != imageEffective.begin())
		{
			--it;
			if (imageOffset < it->end)
			{
				const CDebugSourceMapping &m = mappings[it->mapping];
				// the same byte seen at the same CPU address: banked code, not a copy
				if (m.regionId == regionId && m.start + (u32)(imageOffset - m.imageOffset) == address)
					return &m;
			}
		}
	}
	return Resolve(regionId, address);
}

const CDebugFunction *CDebugDatabase::FindFunctionImage(u32 regionId, u32 address, bool hasImageOffset, u64 imageOffset) const
{
	if (hasImageOffset && !imageEffective.empty())
	{
		for (const CDebugFunction &function : functions)
		{
			if (function.regionId == regionId && function.hasImageOffset
				&& address >= function.start && address < function.end
				&& function.imageOffset + (address - function.start) == imageOffset)
				return &function;
		}
	}
	return FindFunction(regionId, address);
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

static CDebugSourceFile *CopyFile(CDebugDatabase *to, u32 id, const CDebugSourceFile *file)
{
	CDebugSourceFile *copy = to->AddFile(id, file->originalPath);
	copy->loadedPath = file->loadedPath;
	for (CSlrString *line : file->lines)
		copy->lines.push_back(new CSlrString(line));
	return copy;
}

std::shared_ptr<CDebugDatabase> DebugDatabaseMerge(const CDebugDatabase *base,
												   const std::vector<std::shared_ptr<const CDebugDatabase>> &fragments,
												   int *regionId)
{
	std::shared_ptr<CDebugDatabase> merged = std::make_shared<CDebugDatabase>(base ? base->producer.c_str() : "64tass");
	u32 nextFileId = 0;
	if (base)
	{
		merged->capabilities = base->capabilities;
		for (u32 i = 0; i < (u32)base->NumRegions(); i++)
			merged->AddRegion(base->GetRegion(i)->name, base->GetRegion(i)->dest);
		for (u32 i = 0; i < (u32)base->NumBlocks(); i++)
			merged->AddBlock(base->GetBlock(i)->name, base->GetBlock(i)->regionId);
		for (auto &it : base->Files())
		{
			CopyFile(merged.get(), it.first, it.second);
			nextFileId = std::max(nextFileId, it.first + 1);
		}
		for (u32 i = 0; i < (u32)base->NumInlineChains(); i++)
			merged->AddInlineChain(base->GetInlineChain(i));
		for (const CDebugSourceMapping &mapping : base->Mappings())
			merged->AddMapping(mapping);
		for (const CDebugFunction &function : base->Functions())
			merged->AddFunction(function);
	}
	merged->capabilities.lineMappings = true;
	if (*regionId < 0 || *regionId >= (int)merged->NumRegions())
		*regionId = (int)merged->AddRegion("Default", "");
	u32 region = (u32)*regionId;

	for (const std::shared_ptr<const CDebugDatabase> &fragment : fragments)
	{
		u32 block = merged->AddBlock("", region);
		std::map<u32, u32> fileIds;
		for (auto &it : fragment->Files())
		{
			fileIds[it.first] = nextFileId;
			CopyFile(merged.get(), nextFileId++, it.second);
		}
		for (CDebugSourceMapping mapping : fragment->Mappings())
		{
			mapping.regionId = region;
			mapping.blockId = block;
			mapping.fileId = fileIds[mapping.fileId];
			const std::vector<CDebugInlineSite> &chain = fragment->InlineChain(&mapping);
			if (!chain.empty())
			{
				std::vector<CDebugInlineSite> sites = chain;
				for (CDebugInlineSite &site : sites)
					site.fileId = fileIds[site.fileId];
				mapping.inlineChain = merged->AddInlineChain(sites);
			}
			else
				mapping.inlineChain = kNoInlineChain;
			merged->AddMapping(mapping);
		}
		for (CDebugFunction function : fragment->Functions())
		{
			function.regionId = region;
			function.fileId = fileIds[function.fileId];
			merged->AddFunction(function);
		}
	}
	merged->Finalize();
	return merged;
}
