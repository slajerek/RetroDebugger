#ifndef _CDebugDatabase_h_
#define _CDebugDatabase_h_

// The canonical debug database (RD-006, spec #6 of
// docs/specs/source-level-debugging): source files, code regions, blocks and
// address -> source mappings from ONE loaded debug artifact. Every consumer
// (source view, stepping, source breakpoints) reads it; loaders
// (CDebugInfoLoader.h) are its only writers.
//
// A database is built once, Finalize()d, and then never changes: a reload
// builds a new one and CDebugSymbols swaps the shared pointer, so a reader
// holding the old snapshot keeps a consistent view until it lets go.
//
// Regions are code-identity CANDIDATES (a KickAssembler <Segment> today);
// tying a region to a physical identity (CDebugMemoryMapping.h) is RD-007.

#include "SYS_Defs.h"
#include <map>
#include <string>
#include <vector>

class CSlrString;

enum class DebugMappingQuality : u8
{
	Missing,		// no mapping for this address
	Approximate,	// mapped, but the producer cannot vouch for it (e.g. asm-level only)
	Ambiguous,		// several candidates, none provably right for the current mapping
	Exact			// the producer's own address -> source mapping for this identity
};

// What a producer's debug information can say; consumers never synthesize
// what is missing here as exact information (spec #6.4).
struct CDebugDatabaseCapabilities
{
	bool lineMappings = false;
	bool columns = false;
	bool functions = false;
	bool statements = false;
	bool physicalIdentity = false;
};

class CDebugSourceFile
{
public:
	CDebugSourceFile();
	~CDebugSourceFile();

	u32 id;						// the producer's file id
	std::string originalPath;	// exactly as the producer wrote it
	std::string loadedPath;		// where the text was read from; empty when not found
	std::string fileName;		// last component of originalPath, '/' or '\' separated
	CSlrString *fileNameSlr;

	// The text, [0] empty and [n] = line n, tabs expanded to four spaces;
	// empty when the file was not found. Owned.
	std::vector<CSlrString *> lines;

	bool HasText() const { return !lines.empty(); }
};

struct CDebugRegion
{
	std::string name;
	std::string dest;	// KickAssembler <Segment dest="...">, kept for RD-007
};

class CDebugBlock
{
public:
	CDebugBlock(const std::string &name, u32 regionId);
	~CDebugBlock();
	std::string name;
	CSlrString *nameSlr;
	u32 regionId;
};

struct CDebugSourceMapping
{
	u32 start = 0;		// half-open address range [start, end)
	u32 end = 0;
	u32 regionId = 0;
	u32 blockId = 0;
	u32 fileId = 0;
	i32 lineStart = 0;
	i32 columnStart = 0;
	i32 lineEnd = 0;
	i32 columnEnd = 0;
	DebugMappingQuality quality = DebugMappingQuality::Exact;
};

class CDebugDatabase
{
public:
	explicit CDebugDatabase(const char *producer);
	~CDebugDatabase();

	std::string producer;
	CDebugDatabaseCapabilities capabilities;

	// --- building: loaders only, before Finalize() ---
	CDebugSourceFile *AddFile(u32 producerFileId, const std::string &originalPath);
	u32 AddRegion(const std::string &name, const std::string &dest);
	u32 AddBlock(const std::string &name, u32 regionId);
	// In producer order. Where ranges of one region overlap, the later mapping
	// is the one in effect (what the KickAssembler loader always did); the
	// earlier ones stay available as candidates.
	void AddMapping(const CDebugSourceMapping &mapping);
	void Finalize();

	// --- queries: after Finalize() ---
	// The mapping in effect at ADDRESS in REGION, or NULL. O(log n).
	const CDebugSourceMapping *Resolve(u32 regionId, u32 address) const;
	// Every mapping of REGION covering ADDRESS, in producer order.
	void ResolveCandidates(u32 regionId, u32 address, std::vector<const CDebugSourceMapping *> &out) const;
	// Every mapping whose first line is LINE of FILEID, in producer order.
	void ResolveSource(u32 fileId, i32 line, std::vector<const CDebugSourceMapping *> &out) const;

	int FindRegion(const char *name) const;
	const CDebugSourceFile *GetFile(u32 producerFileId) const;
	const CDebugRegion *GetRegion(u32 regionId) const;
	const CDebugBlock *GetBlock(u32 blockId) const;
	size_t NumFiles() const { return files.size(); }
	size_t NumRegions() const { return regions.size(); }
	size_t NumMappings() const { return mappings.size(); }
	const std::map<u32, CDebugSourceFile *> &Files() const { return files; }

private:
	struct Interval
	{
		u32 start;
		u32 end;
		u32 mapping;
	};

	std::map<u32, CDebugSourceFile *> files;
	std::vector<CDebugRegion> regions;
	std::vector<CDebugBlock *> blocks;
	std::vector<CDebugSourceMapping> mappings;

	// Per region: non-overlapping, sorted intervals naming the mapping in effect.
	std::vector<std::vector<Interval>> effective;
	// (fileId, lineStart) -> mapping indices
	std::map<std::pair<u32, i32>, std::vector<u32>> bySourceLine;
	bool finalized;
};

#endif
