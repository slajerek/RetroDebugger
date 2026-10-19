#ifndef _CDebugStackDepth_h_
#define _CDebugStackDepth_h_

#include "SYS_Defs.h"
#include <string>

class CDebugDatabase;
class CDebugInterface;
struct CDebugFunction;

// RD-055: cc65's software stack depth at a PC. cc65 records an `auto`
// local's offset from `sp` as it is after the function's prologue has pushed
// its parameters, but not how far `sp` has moved since: the function's own
// pushes (a local's initializer, a call's arguments) and the callee's pops
// change it inside a statement. The depth is decoded from the code: the
// runtime helpers' effect on `sp` (pusha +1, pushax +2, decspN +N, incspN -N,
// subysp/addysp +-Y, popa -1, the tos* operators -2 or -4) and a called C
// function's net effect (what it pops of its caller's arguments).
//
//  Off      nothing is decoded: the offset is taken from the current sp
//  Limited  one linear walk from the function's entry to the PC; exact
//           only when no jump or branch lies on the way and every helper
//           on it is known
//  Full     the depth at every instruction of the function by data flow
//           over its branches; exact where every path into the PC agrees
//           and every helper reached is known
enum class DebugStackDecoding : u8
{
	Off = 0,
	Limited = 1,
	Full = 2
};

const char *DebugStackDecodingName(DebugStackDecoding mode);

struct CDebugStackDepth
{
	bool known = false;			// depth and refDepth are set
	bool exact = false;			// and nothing on the way was guessed
	int depth = 0;				// bytes pushed since the function's entry, at the PC
	int refDepth = 0;			// the same after the prologue: where cc65's offsets are counted from
	std::string reason;			// why not known / not exact; "" when exact
	int instructions = 0;		// decoded (for the benchmark)
};

// The depth at PC inside FUNCTION (of DATABASE's REGIONID), code bytes and
// helper names read through DEBUGINTERFACE (its CPU view and its symbols).
CDebugStackDepth DebugCc65StackDepth(const CDebugDatabase *database, CDebugInterface *debugInterface,
									 const CDebugFunction &function, u32 regionId, u16 pc, DebugStackDecoding mode);

// the setting (c64SettingsCc65StackDecoding)
DebugStackDecoding DebugGetStackDecoding();

#endif
