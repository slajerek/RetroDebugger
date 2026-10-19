#ifndef _CDebugSourceWatch_h_
#define _CDebugSourceWatch_h_

#include "SYS_Defs.h"
#include "CDebugVariableResolver.h"
#include "hjson.h"
#include <string>
#include <vector>

class CDebugSymbols;

// RD-053 (RESEARCH-c-symbols.md #7): a watch kept by a C variable's name
// rather than an address -- for locals, parameters and register variables,
// whose address moves with the frame or does not exist. It lives in
// CDebugSymbols, not in a segment, so a debug info reload (which replaces
// the segments) keeps it, and it is resolved again against the new build.
//
// What it shows: a global at a fixed address is read live, like an address
// watch; every other variable is resolved only while the machine is stopped
// (a pause, a breakpoint, a step), and while it runs the row keeps the
// value of the last stop, marked as such -- a live read of a frame slot or
// of a register would look real and be another variable's bytes.
class CDebugSourceWatch
{
public:
	CDebugSourceWatch();

	std::string name;			// the C variable
	std::string function;		// whose local it is; "" for a global
	int representation;			// WATCH_REPRESENTATION_*
	int numberOfValues;

	// the last evaluation (Update)
	CDebugVariableResolution value;
	bool hasValue;				// value was resolved at least once
	bool live;					// value is from now, not from the last stop
	std::string state;			// "" when there is a value to show; else a short why: "not in scope"

	// Resolves the watch: from the PC when PAUSED; while running only a fixed
	// global (the rest keeps the last stop's value, live = false)
	void Update(CDebugSymbols *symbols, bool paused);

	// The value in REPRESENTATION, from value.bytes; "" when there are none
	std::string FormatValue() const;

	void Serialize(Hjson::Value hjsonWatch) const;
	bool Deserialize(Hjson::Value hjsonWatch);
};

#endif
