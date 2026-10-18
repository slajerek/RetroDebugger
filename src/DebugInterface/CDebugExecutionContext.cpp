#include "CDebugExecutionContext.h"
#include "CStackAnnotation.h"
#include <algorithm>
#include <cstring>

const char *DebugFrameKindName(DebugFrameKind kind)
{
	switch (kind)
	{
		case DebugFrameKind::Call:		return "call";
		case DebugFrameKind::Interrupt:	return "interrupt";
		case DebugFrameKind::Break:		return "break";
	}
	return "?";
}

CDebugExecutionContextTracker::CDebugExecutionContextTracker()
{
	Reset();
}

void CDebugExecutionContextTracker::Reset()
{
	frames.clear();
	hasBase = false;
	baseSp = 0;
	leftBase = false;
}

int CDebugExecutionContextTracker::GetCallDepth() const
{
	int depth = 0;
	for (const CDebugPhysicalFrame &frame : frames)
	{
		if (frame.kind == DebugFrameKind::Call)
			depth++;
	}
	return depth;
}

int CDebugExecutionContextTracker::GetInterruptDepth() const
{
	return GetDepth() - GetCallDepth();
}

void CDebugExecutionContextTracker::Apply(const CDebugInstructionBoundary &boundary)
{
	if (!hasBase)
	{
		// The first boundary tracked, or the seed's: everything above this
		// stack level belongs to frames entered before tracking.
		hasBase = true;
		baseSp = (boundary.event == DebugExecutionEvent::First) ? boundary.sp : boundary.previousSp;
	}

	// Frames whose pushes have all been popped are over.
	while (!frames.empty() && DebugStackDistance(frames.back().spBefore, boundary.sp) >= 0)
	{
		frames.pop_back();
	}
	if (frames.empty() && DebugStackDistance(baseSp, boundary.sp) > 0
		&& (boundary.event == DebugExecutionEvent::Return || boundary.event == DebugExecutionEvent::InterruptReturn))
	{
		leftBase = true;
		baseSp = boundary.sp;
	}

	CDebugPhysicalFrame frame;
	frame.fromPc = boundary.previousPc;
	frame.entryPc = boundary.pc;

	switch (boundary.event)
	{
		case DebugExecutionEvent::Call:
			frame.kind = DebugFrameKind::Call;
			frame.spBefore = boundary.previousSp;
			frames.push_back(frame);
			break;

		case DebugExecutionEvent::Break:
			frame.kind = DebugFrameKind::Break;
			frame.spBefore = boundary.previousSp;
			frames.push_back(frame);
			break;

		case DebugExecutionEvent::InterruptEntry:
		{
			// The instruction before the interrupt may have entered a frame of
			// its own (a JSR or BRK whose first instruction never ran yet).
			if (boundary.previousOpcode == 0x20 || boundary.previousOpcode == 0x00)
			{
				CDebugPhysicalFrame inner;
				inner.kind = (boundary.previousOpcode == 0x20) ? DebugFrameKind::Call : DebugFrameKind::Break;
				inner.fromPc = boundary.previousPc;
				inner.entryPc = boundary.pc;	// unknown: the interrupt came first
				inner.spBefore = boundary.previousSp;
				frames.push_back(inner);
			}
			frame.kind = DebugFrameKind::Interrupt;
			frame.spBefore = (u8)(boundary.sp + 3);
			frames.push_back(frame);
			break;
		}

		default:
			break;
	}
}

bool DebugSeedFramesFromStack(const CStackAnnotationData &stack, const u8 *stackPage, u8 sp,
							  std::vector<CDebugPhysicalFrame> &out)
{
	out.clear();
	bool complete = true;
	int p = sp + 1;
	while (p <= 0xFF)
	{
		u8 type = stack.entryTypes[p];
		if (type == STACK_ENTRY_JSR_PCL && p + 1 <= 0xFF && stack.entryTypes[p + 1] == STACK_ENTRY_JSR_PCH)
		{
			CDebugPhysicalFrame frame;
			frame.kind = DebugFrameKind::Call;
			frame.fromPc = stack.originPC[p];
			frame.spBefore = (u8)(p + 1);
			frame.seeded = true;
			if (stackPage)
				frame.returnPc = (u16)(((stackPage[p + 1] << 8) | stackPage[p]) + 1);
			out.push_back(frame);
			p += 2;
			continue;
		}
		bool irq = (type == STACK_ENTRY_IRQ_STATUS || type == STACK_ENTRY_NMI_STATUS || type == STACK_ENTRY_BRK_STATUS);
		if (irq && p + 2 <= 0xFF)
		{
			CDebugPhysicalFrame frame;
			frame.kind = (type == STACK_ENTRY_BRK_STATUS) ? DebugFrameKind::Break : DebugFrameKind::Interrupt;
			frame.fromPc = stack.originPC[p];
			frame.spBefore = (u8)(p + 2);
			frame.seeded = true;
			if (stackPage)
				frame.returnPc = (u16)((stackPage[p + 2] << 8) | stackPage[p + 1]);
			out.push_back(frame);
			p += 3;
			continue;
		}
		if (type == STACK_ENTRY_VALUE || type == STACK_ENTRY_STATUS)
		{
			p++;
			continue;
		}
		// unannotated, or half of a pair whose other half was overwritten
		complete = false;
		break;
	}
	// innermost was found first
	std::reverse(out.begin(), out.end());
	return complete;
}
