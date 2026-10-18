#include "CSourceStepController.h"
#include "CDebugInterface.h"
#include "CDebugDatabase.h"
#include "DebuggerDefs.h"
#include "DBG_Log.h"

const char *SourceStepKindName(SourceStepKind kind)
{
	switch (kind)
	{
		case SourceStepKind::None:	return "none";
		case SourceStepKind::Into:	return "into";
		case SourceStepKind::Over:	return "over";
		case SourceStepKind::Out:	return "out";
	}
	return "?";
}

const char *SourceStepStopReasonName(SourceStepStopReason reason)
{
	switch (reason)
	{
		case SourceStepStopReason::None:			return "none";
		case SourceStepStopReason::Completed:		return "completed";
		case SourceStepStopReason::FrameExited:		return "frame-exited";
		case SourceStepStopReason::Interrupted:		return "interrupted";
		case SourceStepStopReason::BudgetExceeded:	return "budget-exceeded";
		case SourceStepStopReason::Cancelled:		return "cancelled";
		case SourceStepStopReason::NoSource:		return "no-source";
	}
	return "?";
}

// the owner's bank-aware lookups (RD-007); without an owner -- the 1541
// drive CPU, whose address space is flat -- the plain ones
static const CDebugSourceMapping *ResolveAt(CDebugInterface *owner, const CDebugDatabase *database, u32 regionId, u16 pc)
{
	if (owner)
		return owner->ResolveSourceAt(database, regionId, pc);
	return database ? database->Resolve(regionId, pc) : NULL;
}

static const CDebugFunction *FunctionAt(CDebugInterface *owner, const CDebugDatabase *database, u32 regionId, u16 pc)
{
	if (owner)
		return owner->FindFunctionAt(database, regionId, pc);
	return database ? database->FindFunction(regionId, pc) : NULL;
}

CSourceStepController::CSourceStepController(CDebugInterface *owner)
{
	this->owner = owner;
	armed.store(false);
	kind = SourceStepKind::None;
	regionId = 0;
	startSp = 0;
	startMapped = false;
	startFileId = 0;
	startLine = 0;
	startRangeStart = 0;
	startInlineDepth = 0;
	lastInlineDepth = 0;
	budget = kDefaultBudget;
	instructions = 0;
}

bool CSourceStepController::Arm(SourceStepKind kind, std::shared_ptr<const CDebugDatabase> database, u32 regionId,
								u16 pc, u8 sp, u64 budget)
{
	Cancel(SourceStepStopReason::Cancelled);

	const CDebugSourceMapping *start = ResolveAt(owner, database.get(), regionId, pc);
	if (start == NULL && kind != SourceStepKind::Out)
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		lastResult = CSourceStepResult();
		lastResult.kind = kind;
		lastResult.reason = SourceStepStopReason::NoSource;
		lastResult.pc = pc;
		return false;
	}

	this->kind = kind;
	this->database = database;
	this->regionId = regionId;
	this->startSp = sp;
	this->startMapped = (start != NULL);
	this->startFileId = start ? start->fileId : 0;
	this->startLine = start ? start->lineStart : 0;
	this->startRangeStart = start ? start->start : 0;
	this->startInlineDepth = (start && database) ? database->InlineDepth(start) : 0;
	this->lastInlineDepth = this->startInlineDepth;
	this->budget = budget;
	this->instructions = 0;
	tracker.Reset();
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		lastResult = CSourceStepResult();
		lastResult.kind = kind;
	}
	LOGD("CSourceStepController: %s from %04x (line %d), sp=%02x", SourceStepKindName(kind), pc, startLine, sp);
	armed.store(true, std::memory_order_release);
	return true;
}

void CSourceStepController::Cancel(SourceStepStopReason reason)
{
	bool wasArmed = true;
	if (armed.compare_exchange_strong(wasArmed, false, std::memory_order_acq_rel))
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		lastResult.reason = reason;
		lastResult.instructions = instructions;
		LOGD("CSourceStepController: %s stopped: %s", SourceStepKindName(kind), SourceStepStopReasonName(reason));
	}
}

CSourceStepResult CSourceStepController::GetLastResult()
{
	std::lock_guard<std::mutex> lock(resultMutex);
	return lastResult;
}

void CSourceStepController::Finish(SourceStepStopReason reason, u16 pc, const CDebugSourceMapping *location)
{
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		lastResult.pc = pc;
		lastResult.mapped = (location != NULL);
		lastResult.fileId = location ? location->fileId : 0;
		lastResult.line = location ? location->lineStart : 0;
		const CDebugSourceFile *file = (location && database) ? database->GetFile(location->fileId) : NULL;
		lastResult.fileName = file ? file->fileName : "";
		// logical (RD-017): inline calls count as calls. In a callee its own
		// inline depth adds to the call; in the start frame the change of
		// inline depth is the change; leaving the frame leaves the start's
		// inline calls with it
		int inlineDepth = (location && database) ? database->InlineDepth(location) : lastInlineDepth;
		int calls = tracker.GetCallDepth();
		if (tracker.HasLeftBase())
			lastResult.depth = -(1 + startInlineDepth);
		else
			lastResult.depth = calls > 0 ? calls + inlineDepth : inlineDepth - startInlineDepth;
	}
	Cancel(reason);
}

// LOCATION is on the definition line of the function at PC, and the function
// has a line of its own after it (a one-line function's only line is its body)
bool CSourceStepController::IsPrologue(const CDebugSourceMapping *location, u16 pc) const
{
	const CDebugFunction *function = FunctionAt(owner, database.get(), regionId, pc);
	if (function == NULL || !function->lineIsDefinition || function->line <= 0 || location->fileId != function->fileId
		|| location->lineStart != function->line)
		return false;
	for (u32 address = location->end; address < function->end; )
	{
		const CDebugSourceMapping *next = function->hasImageOffset
			? database->ResolveImage(regionId, address, true, function->imageOffset + (address - function->start))
			: database->Resolve(regionId, address);
		if (next == NULL)
		{
			address++;
			continue;
		}
		if (next->fileId == function->fileId && next->lineStart != function->line)
			return true;
		address = next->end > address ? next->end : address + 1;
	}
	return false;
}

bool CSourceStepController::IsStartStatement(const CDebugSourceMapping *location) const
{
	return startMapped && location->fileId == startFileId && location->lineStart == startLine;
}

bool CSourceStepController::OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary)
{
	if (!armed.load(std::memory_order_acquire))
		return false;

	tracker.Apply(boundary);
	instructions++;

	const CDebugSourceMapping *location = ResolveAt(owner, database.get(), regionId, boundary.pc);

	if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		Finish(SourceStepStopReason::Interrupted, boundary.pc, location);
		return false;
	}

	// Out of budget: stop outside interrupt handlers if one comes within
	// another budget's worth of instructions, anywhere otherwise.
	if (instructions > budget && (!tracker.IsInInterrupt() || instructions > 2 * budget))
	{
		Finish(SourceStepStopReason::BudgetExceeded, boundary.pc, location);
		return true;
	}

	// Interrupt handlers entered after arming are not part of the step.
	if (tracker.IsInInterrupt())
		return false;

	if (tracker.HasLeftBase())
	{
		Finish(kind == SourceStepKind::Out ? SourceStepStopReason::Completed : SourceStepStopReason::FrameExited,
			   boundary.pc, location);
		return true;
	}

	if (location == NULL || boundary.pc != location->start)
		return false;
	int inlineDepth = database->InlineDepth(location);
	lastInlineDepth = inlineDepth;

	// RD-017: out of an inlined function -- its caller's next statement --
	// within the same physical frame
	if (kind == SourceStepKind::Out)
	{
		if (tracker.GetCallDepth() == 0 && inlineDepth < startInlineDepth)
		{
			Finish(SourceStepStopReason::Completed, boundary.pc, location);
			return true;
		}
		return false;
	}

	// deeper: a call made, or code inlined into this statement
	bool deeper = tracker.GetCallDepth() > 0 || (tracker.GetCallDepth() == 0 && inlineDepth > startInlineDepth);
	if (deeper && kind == SourceStepKind::Over)
		return false;

	// Into a function: run its prologue (cc65 maps it to the definition
	// line) to the first statement, as gdb does
	if (deeper && kind == SourceStepKind::Into && IsPrologue(location, boundary.pc))
		return false;

	if (deeper || !IsStartStatement(location) || boundary.pc == startRangeStart)
	{
		Finish(SourceStepStopReason::Completed, boundary.pc, location);
		return true;
	}
	return false;
}
