#ifndef _CDebugInstructionStepper_h_
#define _CDebugInstructionStepper_h_

// Instruction-level step-over and step-out (RD-037), driven by execution
// events instead of a temporary breakpoint at PC+3. Both decide by the stack
// pointer, so they hold where the PC+3 breakpoint did not:
//  - a recursive call reaches PC+3 in a deeper frame first: step-over stops
//    only when PC+3 is reached with the stack back where it started;
//  - an interrupt handler that runs through PC+3 does so with its own frame
//    on the stack, so it does not stop there either;
//  - a user breakpoint (or any other pause) that wins the race cancels the
//    step instead of leaving a stale target behind.
// A subroutine that drops its return address and leaves by another way
// stops step-over at the first boundary where the stack is back at (or above)
// its starting level, reason FrameLeft.

#include "CDebugExecutionEvents.h"
#include <atomic>

enum class DebugStepKind : u8
{
	None,
	StepOver,		// run until the JSR at the start PC has returned
	StepOut			// run until the current frame's RTS/RTI has executed
};

enum class DebugStepStopReason : u8
{
	None,
	Returned,		// the step reached its target
	FrameLeft,		// step-over: the stack came back without returning to PC+3
	Interrupted,	// a breakpoint, a pause or another step stopped first
	Cancelled		// reset, restore or another request replaced it
};

const char *DebugStepKindName(DebugStepKind kind);
const char *DebugStepStopReasonName(DebugStepStopReason reason);

class CDebugInstructionStepper : public CDebugExecutionObserver
{
public:
	CDebugInstructionStepper();

	// Armed while the CPU is paused, before the caller sets RUNNING.
	void ArmStepOver(u16 startPc, u16 returnPc, u8 startSp);
	void ArmStepOut(u16 startPc, u8 startSp);

	// Disarms an armed step; no-op otherwise. Any thread.
	void Cancel(DebugStepStopReason reason);

	bool IsArmed() const { return kind.load(std::memory_order_acquire) != (u8)DebugStepKind::None; }

	// The most recent step's outcome, for the status line, remote and tests.
	DebugStepKind GetLastKind() const { return (DebugStepKind)lastKind.load(std::memory_order_acquire); }
	DebugStepStopReason GetLastStopReason() const { return (DebugStepStopReason)lastStopReason.load(std::memory_order_acquire); }
	u16 GetLastStopPc() const { return lastStopPc.load(std::memory_order_acquire); }
	u64 GetLastStepBoundaries() const { return lastBoundaries.load(std::memory_order_acquire); }

	virtual void OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary) override;

private:
	void Arm(DebugStepKind stepKind, u16 startPc, u16 returnPc, u8 startSp);
	void Finish(DebugStepStopReason reason, u16 pc);

	std::atomic<u8> kind;
	u16 startPc;
	u16 returnPc;
	u8 startSp;
	u64 boundaries;

	std::atomic<u8> lastKind;
	std::atomic<u8> lastStopReason;
	std::atomic<u16> lastStopPc;
	std::atomic<u64> lastBoundaries;
};

#endif
