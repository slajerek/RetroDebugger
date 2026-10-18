#ifndef _CSourceStepController_h_
#define _CSourceStepController_h_

// Source-level Step Into / Over / Out (RD-013, spec #8.3-#8.7), evaluated at
// every instruction boundary on the emulation thread -- never as a UI loop of
// single steps, which the VICE single-step watchdog would cut short.
//
// Armed with the CPU paused: the database snapshot and region in effect, the
// start PC/SP and the statement there (file and line; no producer gives
// statement ids yet). Then, per boundary:
//  - a pause made by anything else (user breakpoint, pause, manual step)
//    ends the step as Interrupted;
//  - more instructions than the budget: BudgetExceeded;
//  - inside an interrupt entered after arming: skipped (default policy);
//  - Out: stop once the starting frame has returned (RTS/RTI above it);
//  - Over: deeper frames are skipped; in the starting frame stop at the start
//    of a mapped range of another line, or at the start of the starting
//    line's own range again (a loop iteration);
//  - Into: as Over, but also stop at the start of a mapped range in any
//    deeper frame (a callee with source; unmapped helpers run through),
//    except on the callee's definition line when it has a body line after
//    it: the prologue runs to the first statement (cc65 maps it there);
//  - Over/Into whose starting frame returns first: FrameExited, at the
//    caller.
// Unmapped code in the starting frame keeps running until the budget, which
// stops outside an interrupt handler unless one runs past twice the budget.

#include "CDebugExecutionEvents.h"
#include "CDebugExecutionContext.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

class CDebugDatabase;
class CDebugInterface;
struct CDebugSourceMapping;

enum class SourceStepKind : u8
{
	None,
	Into,
	Over,
	Out
};

enum class SourceStepStopReason : u8
{
	None,
	Completed,		// reached the next statement (Into/Over) or the caller (Out)
	FrameExited,	// Into/Over: the starting frame returned before another statement
	Interrupted,	// a breakpoint, a pause or a manual step stopped first
	BudgetExceeded,	// ran the instruction budget out without a decision
	Cancelled,		// another step replaced it
	NoSource		// not armed: no debug info, or no mapping at the PC
};

const char *SourceStepKindName(SourceStepKind kind);
const char *SourceStepStopReasonName(SourceStepStopReason reason);

struct CSourceStepResult
{
	SourceStepKind kind = SourceStepKind::None;
	SourceStepStopReason reason = SourceStepStopReason::None;
	u16 pc = 0;
	bool mapped = false;	// file/line are valid
	u32 fileId = 0;
	i32 line = 0;
	std::string fileName;	// of fileId, when mapped
	int depth = 0;			// call depth of the stop from the start; -1 when the starting frame returned
	u64 instructions = 0;
};

class CSourceStepController
{
public:
	static const u64 kDefaultBudget = 2000000;

	explicit CSourceStepController(CDebugInterface *owner);

	// UI thread, CPU paused. Returns false, with reason NoSource, when
	// Into/Over start where there is no source mapping (Out needs none).
	bool Arm(SourceStepKind kind, std::shared_ptr<const CDebugDatabase> database, u32 regionId,
			 u16 pc, u8 sp, u64 budget);
	void Cancel(SourceStepStopReason reason);
	bool IsArmed() const { return armed.load(std::memory_order_acquire); }

	CSourceStepResult GetLastResult();

	// Emulation thread. Returns true when the step decided to stop here; the
	// caller then pauses the machine.
	bool OnInstructionBoundary(CDebugInterface *debugInterface, const CDebugInstructionBoundary &boundary);

private:
	void Finish(SourceStepStopReason reason, u16 pc, const CDebugSourceMapping *location);
	bool IsStartStatement(const CDebugSourceMapping *location) const;
	bool IsPrologue(const CDebugSourceMapping *location, u16 pc) const;

	CDebugInterface *owner;
	std::atomic<bool> armed;
	SourceStepKind kind;
	std::shared_ptr<const CDebugDatabase> database;
	u32 regionId;
	u8 startSp;
	bool startMapped;
	u32 startFileId;
	i32 startLine;
	u32 startRangeStart;
	u64 budget;
	u64 instructions;
	CDebugExecutionContextTracker tracker;

	std::mutex resultMutex;
	CSourceStepResult lastResult;
};

#endif
