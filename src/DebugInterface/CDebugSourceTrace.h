#ifndef _CDebugSourceTrace_h_
#define _CDebugSourceTrace_h_

// RD-045: the source lines a CPU executed last. An execution observer: it
// takes live boundaries (VICE, Atari800, Nestopia) and replayed ones (the
// C64U's bus trace, decoded) alike, so a machine that cannot be stopped
// still shows where its program has been. Consecutive instructions of one
// line are one entry; an unmapped stretch (ROM, interrupt code without debug
// info) is one entry without a line.

#include "CDebugExecutionEvents.h"
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CDebugDatabase;

struct CDebugSourceTraceEntry
{
	bool mapped = false;
	u32 fileId = 0;
	std::string fileName;
	i32 line = 0;
	u16 pc = 0;				// the first instruction of the run
	u64 instructions = 0;	// in the run
	bool replayed = false;
};

class CDebugSourceTraceRecorder : public CDebugExecutionObserver
{
public:
	static const size_t kDefaultCapacity = 256;
	explicit CDebugSourceTraceRecorder(size_t capacity = kDefaultCapacity);

	virtual void OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary) override;
	virtual void OnCpuReset(CDebugInterface *debugInterface) override;

	// newest last; at most MAXENTRIES
	std::vector<CDebugSourceTraceEntry> GetEntries(size_t maxEntries = kDefaultCapacity);
	void Clear();

private:
	size_t capacity;
	std::mutex mutex;
	std::deque<CDebugSourceTraceEntry> entries;
	// the database the entries' file ids belong to: a reload starts over
	const CDebugDatabase *database = NULL;
};

#endif
