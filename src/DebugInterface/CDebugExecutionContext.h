#ifndef _CDebugExecutionContext_h_
#define _CDebugExecutionContext_h_

// The physical execution context tracker (RD-012, spec #8.2): call and
// interrupt frames observed from instruction-boundary events since tracking
// started. A frame ends when the stack pointer is back at (or above) where it
// was before the frame's pushes, so a routine that drops its return address
// and jumps away ends its frame too. Frames entered before tracking started
// are not known: the base is Unknown, and nothing here claims otherwise.
// Logical (source) frames come from the debug database on top of this.

#include "CDebugExecutionEvents.h"
#include <vector>

enum class DebugFrameKind : u8
{
	Call,			// JSR
	Interrupt,		// IRQ or NMI taken by the CPU
	Break			// BRK
};

const char *DebugFrameKindName(DebugFrameKind kind);

struct CDebugPhysicalFrame
{
	DebugFrameKind kind = DebugFrameKind::Call;
	u16 fromPc = 0;		// the JSR/BRK, or the instruction after which the interrupt hit
	u16 entryPc = 0;	// the first instruction of the frame
	u8 spBefore = 0;	// the stack pointer before the frame's pushes
};

class CDebugExecutionContextTracker
{
public:
	CDebugExecutionContextTracker();

	void Reset();
	void Apply(const CDebugInstructionBoundary &boundary);

	const std::vector<CDebugPhysicalFrame> &GetFrames() const { return frames; }
	int GetDepth() const { return (int)frames.size(); }
	int GetCallDepth() const;			// Call frames only
	int GetInterruptDepth() const;		// Interrupt and Break frames
	bool IsInInterrupt() const { return GetInterruptDepth() > 0; }

	// The stack moved below the tracked frames' base and came back above it
	// (a return from a frame entered before tracking started).
	bool HasLeftBase() const { return leftBase; }

private:
	std::vector<CDebugPhysicalFrame> frames;
	bool hasBase;
	u8 baseSp;
	bool leftBase;
};

#endif
