#ifndef _CDebugInfoLoader_h_
#define _CDebugInfoLoader_h_

// Debug information loaders (RD-006, RD-031). The format of a symbols or
// debug-info file is decided by its CONTENT, never by its extension:
// cc65/ld65 and KickAssembler both name theirs .dbg.

#include "SYS_Defs.h"

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

// Decides the format from the first bytes of a file.
DebugInfoFormat DebugInfoProbe(const u8 *data, int length);

// Loads a symbols / debug-info file into SYMBOLS by content: KickAssembler
// .dbg through the KickAssembler loader, VICE and 64tass labels through
// CDebugSymbols::ParseSymbols. Formats recognized but not loaded yet (cc65,
// Oscar64: RD-009, RD-008) are reported, not fed to the wrong parser.
// Returns the format found; Unknown when the file is missing or unrecognized.
DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, CSlrString *path);
DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, const char *path);

// The KickAssembler .dbg loader: replaces SYMBOLS' segments with the file's,
// fills them with labels, watches and breakpoints, and swaps in a new
// CDebugDatabase with the source mappings. Source files not found at their
// recorded path are looked for by name in DEBUGFILEDIR (may be NULL).
bool DebugInfoLoadKickAss(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const char *debugFileDir);

#endif
