#ifndef _CDebugInfoLoader_h_
#define _CDebugInfoLoader_h_

// Debug information loaders (RD-006, RD-031). The format of a symbols or
// debug-info file is decided by its CONTENT, never by its extension:
// cc65/ld65 and KickAssembler both name theirs .dbg.

#include "SYS_Defs.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CByteBuffer;
class CSlrString;
class CDebugSymbols;
class CDebugDatabase;
class CDebugSourceFile;

enum class DebugInfoFormat : u8
{
	Unknown,
	KickAssDbg,		// KickAssembler -debugdump XML, <C64debugger version="1.0">
	Cc65Dbg,		// ld65 --dbgfile text, "version\tmajor=2,minor=..."
	Oscar64Dbj,		// Oscar64 -g JSON, {"memory": [...], ...}
	ViceLabels,		// VICE monitor labels, "al C:080d .start"
	Tass64Labels	// 64tass --labels, "start = $080d"
};

const char *DebugInfoFormatName(DebugInfoFormat format);

// RD-007: where a KickAssembler segment's bytes are in the program file.
// KickAssembler names its segments but records no file offset, so code of
// several banks assembled at one CPU address cannot be told apart from the
// .dbg alone: without an entry such segments' mappings are Ambiguous. With
// one, the byte at CPU ADDRESS of SEGMENT is at IMAGEOFFSET in the program
// file (a .crt, .car or .nes, header included), and every listed segment is
// loaded into one symbols segment whose code resolves by the bank in.
struct CDebugSegmentImage
{
	std::string segment;
	u32 address = 0;
	u64 imageOffset = 0;
};

// Reads a segment table, (H)JSON: {"segments": [{"segment": "Bank0",
// "address": "$8000", "image": "$0050"}, ...]}; numbers as JSON numbers or
// "$hex"/"0xhex" strings. DebugInfoLoadFile reads <debug file without its
// extension>.segments.json when the caller gives no table. False with
// ERROR set when the file is unreadable or malformed.
bool DebugInfoReadSegmentTable(const char *path, std::vector<CDebugSegmentImage> &segments, std::string &error);
// The table's path for a debug file: main.dbg -> main.segments.json
std::string DebugInfoSegmentTablePath(const char *debugFilePath);

// Where loaders look for the source files a debug file names. A source is
// looked up (1) through each remap rule whose FROM prefixes its recorded path
// (FROM replaced by TO: a build directory that moved, another machine's
// checkout), (2) a relative recorded path against debugFileDir, (3) at its
// recorded path, (4) by file name in debugFileDir, (5) by file name in each
// of sourceRoots.
struct CDebugInfoLoadOptions
{
	std::string debugFileDir;
	std::string debugFilePath;		// set by DebugInfoLoadFile
	std::vector<std::pair<std::string, std::string>> pathRemaps;
	std::vector<std::string> sourceRoots;
	std::vector<CDebugSegmentImage> segmentImages;		// KickAssembler only
};

// Decides the format from the first bytes of a file.
DebugInfoFormat DebugInfoProbe(const u8 *data, int length);

// Loads a symbols / debug-info file into SYMBOLS by content: KickAssembler
// .dbg, cc65 .dbg and Oscar64 .dbj through their loaders, VICE and 64tass
// labels through CDebugSymbols::ParseSymbols.
// Returns the format found; Unknown when the file is missing or unrecognized.
DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, CSlrString *path);
DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, const char *path);
DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, const char *path, const CDebugInfoLoadOptions &options);

// The KickAssembler .dbg loader: replaces SYMBOLS' segments with the file's,
// fills them with labels, watches and breakpoints, and swaps in a new
// CDebugDatabase with the source mappings. Source files not found at their
// recorded path are looked for by name in DEBUGFILEDIR (may be NULL).
bool DebugInfoLoadKickAss(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const char *debugFileDir);
bool DebugInfoLoadKickAss(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const CDebugInfoLoadOptions &options);

// The Oscar64 .dbj loader (RD-008): one "Default" segment and region, a
// function (and a block, for the source view) per "functions" entry, line
// mappings from each function's "lines", labels for functions and variables.
// Oscar64 has no columns; "origins" (inline provenance) is ignored until RD-017.
bool DebugInfoLoadOscar64(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const CDebugInfoLoadOptions &options);

// The cc65 / ld65 --dbgfile loader (RD-009): one "Default" segment and
// region; line mappings from every "line" record's spans (address = segment
// start + span start), asm (ca65, including cc65's generated .s) first and C
// lines last, so C wins where both cover an address and asm stays a
// candidate; functions from C symbols with external linkage via their asm
// scope; labels from asm labels except cc65's generated Lxxxx ones. No columns.
bool DebugInfoLoadCc65(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const CDebugInfoLoadOptions &options);

// The text of FILE from memory, as LoadSourceText reads a file: [0] empty,
// line n at [n], tabs expanded to four spaces (RD-043).
void DebugSourceFileSetText(CDebugSourceFile *file, const char *text, size_t length);

// RD-043: a database for code the embedded 64tass assembled: one file NAME
// holding TEXT, one region and block, and a line mapping per run of
// consecutive bytes from one line (LINES: logical address -> source line,
// what the assembler reported for its last pass). Producer "64tass".
std::shared_ptr<CDebugDatabase> DebugInfoFromAssembly(const char *name, const char *text, size_t length,
													  const std::map<u32, i32> &lines);

// Stale debug info (RD-014, spec #6.1): none of the supported producers
// records a build id or a hash of the binary, so the check is by file time.
// Stale when a source DATABASE read is newer than DEBUGFILEPATH, or when
// PROGRAMPATH (may be NULL) and the debug file were not written by one build
// (more than kDebugInfoBuildSkewSeconds apart). Each reason is one sentence
// naming the files; empty when nothing is stale or a file is missing.
static const int kDebugInfoBuildSkewSeconds = 120;
std::vector<std::string> DebugInfoCheckStale(const CDebugDatabase *database, const char *debugFilePath,
											 const char *programPath);

// RD-007 identity check: the code the debug info maps, in the emulator's
// memory, against PROGRAMPATH's bytes at the mappings' image offsets (the
// producer's file offsets, cc65's ooffs). For a cartridge build only bytes
// fetched from the expected image offset count (the bank in). Differing bytes mean another build is loaded, or the
// code was changed at run time. Not having image offsets checks nothing.
struct CDebugCodeCheck
{
	bool programRead = false;
	int checked = 0;					// bytes compared
	int differing = 0;
	std::vector<u32> firstDiffering;	// up to 8 CPU addresses
	std::string warning;				// one sentence, empty when it matches
};
class CDebugInterface;
CDebugCodeCheck DebugInfoCheckCode(const CDebugDatabase *database, CDebugInterface *debugInterface, const char *programPath);

#endif
