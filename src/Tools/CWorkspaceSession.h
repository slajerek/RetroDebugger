#ifndef _CWorkspaceSession_h_
#define _CWorkspaceSession_h_

// RD-020: what a finished build does to the debug session (spec #10.3). A
// successful build loads its program and its debug info together, onto the
// task's machine; a failed one leaves the loaded program and debug info as
// they were and says, in the symbols' warnings (the Source view's header and
// remote symbols/load), that they are from an earlier build. UI thread.

#include <string>

class CDebugInterface;
class CBuildTaskRun;

class CWorkspaceSession
{
public:
	// "c64" (also ""), "atari", "nes"; NULL when that machine is not compiled in
	static CDebugInterface *Machine(const std::string &machine);
	static bool LoadBuild(const CBuildTaskRun &run, std::string *error);
	static void MarkStale(const CBuildTaskRun &run);
	static const char *kStalePrefix;	// the warning MarkStale adds starts with it
};

#endif
