#include "CDebugInstructionStepper.h"
#include "CDebugInterface.h"
#include "DebuggerDefs.h"
#include "DBG_Log.h"

const char *DebugStepKindName(DebugStepKind kind)
{
	switch (kind)
	{
		case DebugStepKind::None:		return "none";
		case DebugStepKind::StepOver:	return "step-over";
		case DebugStepKind::StepOut:	return "step-out";
	}
	return "?";
}

const char *DebugStepStopReasonName(DebugStepStopReason reason)
{
	switch (reason)
	{
		case DebugStepStopReason::None:			return "none";
		case DebugStepStopReason::Returned:		return "returned";
		case DebugStepStopReason::FrameLeft:	return "frame-left";
		case DebugStepStopReason::Interrupted:	return "interrupted";
		case DebugStepStopReason::Cancelled:	return "cancelled";
	}
	return "?";
}

CDebugInstructionStepper::CDebugInstructionStepper()
{
	kind.store((u8)DebugStepKind::None);
	startPc = 0;
	returnPc = 0;
	startSp = 0;
	boundaries = 0;
	lastKind.store((u8)DebugStepKind::None);
	lastStopReason.store((u8)DebugStepStopReason::None);
	lastStopPc.store(0);
	lastBoundaries.store(0);
}

void CDebugInstructionStepper::ArmStepOver(u16 startPc, u16 returnPc, u8 startSp)
{
	Arm(DebugStepKind::StepOver, startPc, returnPc, startSp);
}

void CDebugInstructionStepper::ArmStepOut(u16 startPc, u8 startSp)
{
	Arm(DebugStepKind::StepOut, startPc, 0, startSp);
}

void CDebugInstructionStepper::Arm(DebugStepKind stepKind, u16 startPc, u16 returnPc, u8 startSp)
{
	Cancel(DebugStepStopReason::Cancelled);

	this->startPc = startPc;
	this->returnPc = returnPc;
	this->startSp = startSp;
	this->boundaries = 0;
	lastKind.store((u8)stepKind, std::memory_order_release);
	lastStopReason.store((u8)DebugStepStopReason::None, std::memory_order_release);
	kind.store((u8)stepKind, std::memory_order_release);
}

void CDebugInstructionStepper::Cancel(DebugStepStopReason reason)
{
	u8 armed = kind.load(std::memory_order_acquire);
	while (armed != (u8)DebugStepKind::None)
	{
		if (kind.compare_exchange_weak(armed, (u8)DebugStepKind::None, std::memory_order_acq_rel))
		{
			lastStopReason.store((u8)reason, std::memory_order_release);
			lastBoundaries.store(boundaries, std::memory_order_release);
			LOGD("CDebugInstructionStepper: %s from %04x stopped: %s",
				 DebugStepKindName((DebugStepKind)armed), startPc, DebugStepStopReasonName(reason));
			return;
		}
	}
}

void CDebugInstructionStepper::Finish(DebugStepStopReason reason, u16 pc)
{
	lastStopPc.store(pc, std::memory_order_release);
	Cancel(reason);
}

void CDebugInstructionStepper::OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary)
{
	DebugStepKind stepKind = (DebugStepKind)kind.load(std::memory_order_acquire);
	if (stepKind == DebugStepKind::None)
		return;

	boundaries++;

	// A breakpoint evaluated at this boundary (or a pause from any thread)
	// already stopped the machine: the user's stop wins and the step ends.
	if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		Finish(DebugStepStopReason::Interrupted, boundary.pc);
		return;
	}

	int distance = DebugStackDistance(startSp, boundary.sp);
	DebugStepStopReason reason = DebugStepStopReason::None;

	if (stepKind == DebugStepKind::StepOver)
	{
		// Inside the subroutine the stack is below startSp (its return
		// address, plus any interrupt frames). Back at startSp or above, the
		// call is over one way or another.
		if (distance >= 0)
		{
			reason = (distance == 0 && boundary.pc == returnPc)
				? DebugStepStopReason::Returned : DebugStepStopReason::FrameLeft;
		}
	}
	else if (stepKind == DebugStepKind::StepOut)
	{
		// The current frame's return address sits just above startSp; its
		// RTS (or RTI, stepping out of a handler) lifts the stack above it.
		// Returns from deeper calls and interrupts land at or below startSp.
		if (distance > 0
			&& (boundary.event == DebugExecutionEvent::Return
				|| boundary.event == DebugExecutionEvent::InterruptReturn))
		{
			reason = DebugStepStopReason::Returned;
		}
	}

	if (reason != DebugStepStopReason::None)
	{
		Finish(reason, boundary.pc);
		debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}
