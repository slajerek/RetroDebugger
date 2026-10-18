#ifndef _CDebugExecutionEvents_h_
#define _CDebugExecutionEvents_h_

// Generic execution events (RD-011, spec #8). Each emulator's per-instruction
// breakpoint hook reports an instruction boundary -- the CPU is about to
// execute the opcode at PC -- and CDebugInterface derives what happened since
// the previous boundary (a call, a return, an interrupt entry) from the
// previous opcode and the stack pointer movement. Nothing is reported, and the
// hook costs one relaxed atomic load, while no observer is armed.
//
// Main CPU, and the 1541 drive CPU while a drive source step is armed (RD-044).

#include "SYS_Defs.h"
#include <atomic>

class CDebugInterface;

enum class DebugExecutionEvent : u8
{
	First,				// the first boundary after arming: nothing is known before it
	Sequential,			// any other instruction, branches and jumps included
	Call,				// JSR
	Return,				// RTS
	InterruptEntry,		// an IRQ or NMI was taken after the previous instruction
	InterruptReturn,	// RTI
	Break,				// BRK
	StackReloaded		// TXS: the stack pointer moved by the program's choice
};

const char *DebugExecutionEventName(DebugExecutionEvent event);

struct CDebugInstructionBoundary
{
	u16 pc = 0;					// the instruction about to execute
	u8 sp = 0;
	u8 opcode = 0;				// at pc, peeked without side effects

	DebugExecutionEvent event = DebugExecutionEvent::First;
	u16 previousPc = 0;			// the instruction that just executed
	u8 previousOpcode = 0;
	u8 previousSp = 0;

	u64 sequence = 0;			// boundaries since the observation was armed
	bool replayed = false;		// from a recorded trace (C64U): the machine cannot be stopped here
};

// The 6502 stack pointer change an instruction makes by itself, without an
// interrupt taken after it. Returns false for TXS (the change is X - SP).
bool DebugOpcodeStackDelta(u8 opcode, int *delta);

// Classifies what happened between two boundaries.
DebugExecutionEvent DebugClassifyBoundary(u8 previousOpcode, u8 previousSp, u8 sp);

// Signed distance from stack pointer FROM to TO on the 256-byte stack page:
// positive when TO is above FROM (the stack has shrunk).
inline int DebugStackDistance(u8 from, u8 to) { return (int)(i8)(u8)(to - from); }

// The boundary stream of one CPU: each boundary classified from the one
// before it. A restart (reset, or a seed: the boundary a step starts from, so
// the first instruction it runs is classified too) is requested from any
// thread and applied by the emulation thread at its next boundary.
class CDebugBoundaryHistory
{
public:
	CDebugBoundaryHistory();
	CDebugInstructionBoundary Next(u16 pc, u8 sp, u8 opcode);	// emulation thread
	void RequestReset();
	// with the emulation thread parked (the seed is in place before it runs)
	void RequestSeed(u16 pc, u8 sp, u8 opcode);

private:
	enum { Keep, Reset, Seed };
	std::atomic<int> restart;
	CDebugInstructionBoundary seed;
	// emulation thread only
	CDebugInstructionBoundary last;
	bool hasLast;
	u64 sequence;
};

// Observers run on the emulation thread inside the CPU loop: they must not
// block, may read machine state and may call SetDebugMode(PAUSED), which
// pauses at this boundary, before the opcode at pc executes. A replayed
// boundary (spec #20.8 guard 3: a recorded trace, e.g. the C64U's bus trace
// decoded) comes the same way, after the fact; nothing stops there.
class CDebugExecutionObserver
{
public:
	virtual ~CDebugExecutionObserver() {}
	virtual void OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary) = 0;
	virtual void OnCpuReset(CDebugInterface *debugInterface) {}
};

#endif
