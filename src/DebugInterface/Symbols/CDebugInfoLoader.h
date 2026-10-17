#ifndef _CDebugInfoLoader_h_
#define _CDebugInfoLoader_h_

// Debug information loaders (RD-006, RD-031). The format of a symbols or
// debug-info file is decided by its CONTENT, never by its extension:
// cc65/ld65 and KickAssembler both name theirs .dbg.

#include "SYS_Defs.h"
#include <string>
#include <utility>
#include <vector>

class CByteBuffer;
class CSlrString;
class CDebugSymbols;

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

// Where loaders look for the source files a debug file names. A source is
// looked up (1) through each remap rule whose FROM prefixes its recorded path
// (FROM replaced by TO: a build directory that moved, another machine's
// checkout), (2) at its recorded path, (3) by file name in debugFileDir.
struct CDebugInfoLoadOptions
{
	std::string debugFileDir;
	std::vector<std::pair<std::string, std::string>> pathRemaps;
};

// Decides the format from the first bytes of a file.
DebugInfoFormat DebugInfoProbe(const u8 *data, int length);

// Loads a symbols / debug-info file into SYMBOLS by content: KickAssembler
// .dbg and Oscar64 .dbj through their loaders, VICE and 64tass labels through
// CDebugSymbols::ParseSymbols. A format recognized but not loaded yet (cc65:
// RD-009) is reported, not fed to the wrong parser.
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

#endif
