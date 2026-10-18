#include "CDebugSourceTrace.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugDatabase.h"

CDebugSourceTraceRecorder::CDebugSourceTraceRecorder(size_t capacity)
: capacity(capacity)
{
}

void CDebugSourceTraceRecorder::OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary)
{
	CDebugSymbols *symbols = debugInterface->symbols;
	std::shared_ptr<const CDebugDatabase> db = symbols ? symbols->debugDatabase : nullptr;
	const CDebugSourceMapping *mapping = NULL;
	if (db && symbols->currentSegment && symbols->currentSegment->debugRegionId >= 0)
		mapping = debugInterface->ResolveSourceAt(db.get(), (u32)symbols->currentSegment->debugRegionId, boundary.pc);

	std::lock_guard<std::mutex> lock(mutex);
	if (db.get() != database)
	{
		entries.clear();
		database = db.get();
	}
	if (!entries.empty())
	{
		CDebugSourceTraceEntry &last = entries.back();
		bool sameRun = mapping ? (last.mapped && last.fileId == mapping->fileId && last.line == mapping->lineStart) : !last.mapped;
		if (sameRun)
		{
			last.instructions++;
			return;
		}
	}
	CDebugSourceTraceEntry entry;
	entry.pc = boundary.pc;
	entry.instructions = 1;
	entry.replayed = boundary.replayed;
	if (mapping)
	{
		const CDebugSourceFile *file = db->GetFile(mapping->fileId);
		entry.mapped = true;
		entry.fileId = mapping->fileId;
		entry.fileName = file ? file->fileName : "";
		entry.line = mapping->lineStart;
	}
	entries.push_back(entry);
	while (entries.size() > capacity)
		entries.pop_front();
}

void CDebugSourceTraceRecorder::OnCpuReset(CDebugInterface *debugInterface)
{
	Clear();
}

std::vector<CDebugSourceTraceEntry> CDebugSourceTraceRecorder::GetEntries(size_t maxEntries)
{
	std::lock_guard<std::mutex> lock(mutex);
	size_t first = entries.size() > maxEntries ? entries.size() - maxEntries : 0;
	return std::vector<CDebugSourceTraceEntry>(entries.begin() + first, entries.end());
}

void CDebugSourceTraceRecorder::Clear()
{
	std::lock_guard<std::mutex> lock(mutex);
	entries.clear();
	database = NULL;
}
