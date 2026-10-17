#ifndef _CDebugSourceBreakpoint_h_
#define _CDebugSourceBreakpoint_h_

// Source breakpoints (RD-014, spec #8.8): the user's intent -- a source file
// and a line -- kept on CDebugSymbols, separate from the address breakpoints
// it currently resolves to. Every database swap re-resolves it, so it
// survives a recompile and a reload; a line that maps to nothing stays as an
// unresolved breakpoint instead of disappearing.
//
// Resolution: the file is matched by recorded path, loaded path or file name;
// a line without code slides forward to the first line with code, but not
// past the definition line of the next function in that file. Each mapping of
// that line in effect at its first address gives one address breakpoint, in
// the segment whose region the mapping belongs to. A breakpoint the user
// already had at that address is shared, never deleted by the source one.

#include "SYS_Defs.h"
#include <string>
#include <vector>

class CDebugDatabase;
struct CDebugSourceMapping;
class CDebugSymbolsSegment;

struct CDebugSourceBreakpointBinding
{
	CDebugSymbolsSegment *segment = NULL;
	int address = 0;
	bool created = false;	// the address breakpoint was added for this source breakpoint
};

class CDebugSourceBreakpoint
{
public:
	std::string path;		// as the user gave it
	i32 line = 0;

	// the current resolution
	i32 resolvedLine = 0;	// 0 when unresolved
	std::vector<CDebugSourceBreakpointBinding> bindings;

	bool IsResolved() const { return !bindings.empty(); }
};

// The first addresses LINE of PATH resolves to in DATABASE (one per mapping
// in effect), and the line actually used. Empty when nothing matches.
void DebugResolveSourceLine(const CDebugDatabase *database, const std::string &path, i32 line,
							i32 *resolvedLine, std::vector<const CDebugSourceMapping *> &out);

#endif
