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
#include <memory>
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
	// RD-022: C variables (RESEARCH-c-symbols.md #6)
	bool variables = false;			// globals with addresses
	bool localVariables = false;	// parameters and locals
	bool types = false;				// sizes and kinds of the variables' types
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

// A function's entry and code range, as the producer reports it.
struct CDebugFunction
{
	std::string name;
	std::string signature;	// producer's decorated form, e.g. Oscar64 "add(u8,u8)->u8"
	u32 start = 0;			// half-open [start, end)
	u32 end = 0;
	u32 regionId = 0;
	u32 fileId = 0;
	i32 line = 0;			// line of the definition (or of the entry when unknown)
	bool lineIsDefinition = false;	// the producer or the source text said so
	bool hasImageOffset = false;	// where start's byte is in the program file (RD-007)
	u64 imageOffset = 0;
};

// RD-022: a variable's type, as the producer records it (Oscar64 "uint",
// "ptr", "array"; cc65 nothing). Never guessed from the name.
enum class DebugTypeKind : u8
{
	Unknown,
	UInt,
	SInt,
	Pointer,
	Array,
	Struct,
	Function
};
static const u32 kNoDebugType = 0xFFFFFFFF;

struct CDebugType
{
	u32 id = 0;
	DebugTypeKind kind = DebugTypeKind::Unknown;
	u32 size = 0;
	u32 elementType = kNoDebugType;		// Pointer: the target, Array: the element
	u32 count = 0;						// Array: size / element size
	std::string name;
};

// RD-022: where a variable lives (RESEARCH-c-symbols.md #5), which decides
// what may be claimed about its value at a PC.
enum class DebugVariableLocation : u8
{
	Fixed,			// an absolute address, always
	FixedScoped,	// an absolute address, valid while its function runs its enter..leave lines
	ZpParam,		// a zero-page parameter register: valid at the function's entry
	FrameRelative,	// word(baseZp) + address: the software stack frame
	Register,		// a CPU register: not readable from memory
	Constant,		// folded: never a memory location
	Absent			// optimized out
};

struct CDebugVariable
{
	std::string name;
	i32 functionIndex = -1;			// -1: global; else the AddFunction index
	u32 typeId = kNoDebugType;
	DebugVariableLocation location = DebugVariableLocation::Fixed;
	u32 address = 0;				// absolute; FrameRelative: the offset from the stack pointer
	u32 baseZp = 0;					// FrameRelative: the zero-page address of the 16-bit stack pointer
	u32 size = 0;					// bytes; 0 when unknown
	bool sizeApproximate = false;	// cc65: from the next label or the segment end
	bool frameExactAtLineStarts = false;	// Oscar64: the frame is set up at every line start of the body
	i32 enterLine = 0;				// FixedScoped: the source lines of its scope
	i32 leaveLine = 0;
	u32 regionId = 0;
	char cpuRegister = 0;			// Register: 'A', 'X' or 'Y' -- at the function's entry
	i64 constantValue = 0;			// Constant: the folded value
	std::string note;				// why a Fixed address is only approximate ("" when not)
};

// RD-017: one call an inlined function's code came from -- the caller's
// FILEID:LINE and the inlined function's name ("" when unknown)
struct CDebugInlineSite
{
	u32 fileId = 0;
	i32 line = 0;
	std::string function;
};
static const u32 kNoInlineChain = 0xFFFFFFFF;

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
	// RD-007: the offset of start's byte in the program file (a cartridge
	// image) when the producer records it (cc65 segments' ooffs). Banks that
	// share CPU addresses differ here: it is the bank-independent identity.
	bool hasImageOffset = false;
	u64 imageOffset = 0;
	// RD-017: code inlined into its caller(s): the calls it came from,
	// innermost first (AddInlineChain); kNoInlineChain when not inlined
	u32 inlineChain = kNoInlineChain;
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
	u32 AddFunction(const CDebugFunction &function);
	u32 AddInlineChain(const std::vector<CDebugInlineSite> &chain);
	void AddType(const CDebugType &type);
	void AddVariable(const CDebugVariable &variable);
	void Finalize();

	// --- queries: after Finalize() ---
	// The mapping in effect at ADDRESS in REGION, or NULL. O(log n).
	const CDebugSourceMapping *Resolve(u32 regionId, u32 address) const;
	// Every mapping of REGION covering ADDRESS, in producer order.
	void ResolveCandidates(u32 regionId, u32 address, std::vector<const CDebugSourceMapping *> &out) const;
	// Every mapping whose first line is LINE of FILEID, in producer order.
	void ResolveSource(u32 fileId, i32 line, std::vector<const CDebugSourceMapping *> &out) const;

	// The function whose range holds ADDRESS in REGION, or NULL. O(log n).
	const CDebugFunction *FindFunction(u32 regionId, u32 address) const;

	// RD-007, banked code: some mappings carry image offsets. With the fetch's
	// image offset known (the machine's resolver), the mapping or function
	// whose bytes are the ones in the CPU window at ADDRESS; without it, or
	// when nothing matches, the same as Resolve/FindFunction.
	bool HasImageOffsets() const { return !imageEffective.empty(); }
	// ADDRESS in REGION is covered by code of more than one bank (mappings
	// whose image offsets differ for the same CPU address): only there does
	// the bank in the window decide. O(log n).
	bool IsBankedAddress(u32 regionId, u32 address) const;
	const CDebugSourceMapping *ResolveImage(u32 regionId, u32 address, bool hasImageOffset, u64 imageOffset) const;
	const CDebugFunction *FindFunctionImage(u32 regionId, u32 address, bool hasImageOffset, u64 imageOffset) const;
	const std::vector<CDebugFunction> &Functions() const { return functions; }

	// RD-022: variables, in producer order, and their types
	const std::vector<CDebugVariable> &Variables() const { return variables; }
	const CDebugType *GetType(u32 typeId) const;
	const std::map<u32, CDebugType> &Types() const { return types; }

	// RD-017: the inline calls of a mapping, innermost first; empty when the
	// code is its function's own. The logical depth adds their number to the
	// physical call depth; the innermost names the logical function.
	const std::vector<CDebugInlineSite> &InlineChain(const CDebugSourceMapping *mapping) const;
	int InlineDepth(const CDebugSourceMapping *mapping) const { return (int)InlineChain(mapping).size(); }
	size_t NumInlineChains() const { return inlineChains.size(); }
	const std::vector<CDebugInlineSite> &GetInlineChain(u32 index) const { return inlineChains[index]; }

	int FindRegion(const char *name) const;
	const CDebugSourceFile *GetFile(u32 producerFileId) const;
	const CDebugRegion *GetRegion(u32 regionId) const;
	const CDebugBlock *GetBlock(u32 blockId) const;
	size_t NumFiles() const { return files.size(); }
	size_t NumRegions() const { return regions.size(); }
	size_t NumBlocks() const { return blocks.size(); }
	size_t NumMappings() const { return mappings.size(); }
	const std::vector<CDebugSourceMapping> &Mappings() const { return mappings; }
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
	std::vector<CDebugFunction> functions;
	std::vector<CDebugVariable> variables;
	std::map<u32, CDebugType> types;
	std::vector<std::vector<CDebugInlineSite>> inlineChains;
	// Per region: function indices sorted by start.
	std::vector<std::vector<u32>> functionsByStart;

	// Per region: non-overlapping, sorted intervals naming the mapping in effect.
	std::vector<std::vector<Interval>> effective;
	struct ImageInterval
	{
		u64 start;
		u64 end;
		u32 mapping;
	};
	std::vector<ImageInterval> imageEffective;	// over image offsets, later mappings win
	std::vector<std::vector<std::pair<u32, u32>>> bankedRanges;	// per region, sorted, merged [start, end)
	// (fileId, lineStart) -> mapping indices
	std::map<std::pair<u32, i32>, std::vector<u32>> bySourceLine;
	bool finalized;
};

// RD-043: BASE (may be NULL) with FRAGMENTS added on top -- each a database
// of one region, e.g. DebugInfoFromAssembly's -- in order, so where they
// overlap the later fragment's mappings are in effect. Fragment mappings go
// into region *REGIONID of the result; with *REGIONID < 0 a "Default" region
// is added for them and *REGIONID set to it. File ids are renumbered past
// BASE's. The result is finalized.
std::shared_ptr<CDebugDatabase> DebugDatabaseMerge(const CDebugDatabase *base,
												   const std::vector<std::shared_ptr<const CDebugDatabase>> &fragments,
												   int *regionId);

#endif
