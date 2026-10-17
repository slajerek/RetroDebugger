#include "CDebugExecutionEvents.h"
#include "CDebugInstructionStepper.h"
#include "CDebugInterface.h"
#include "CSnapshotsManager.h"
#include "DBG_Log.h"

const char *DebugExecutionEventName(DebugExecutionEvent event)
{
	switch (event)
	{
		case DebugExecutionEvent::First:			return "first";
		case DebugExecutionEvent::Sequential:		return "sequential";
		case DebugExecutionEvent::Call:				return "call";
		case DebugExecutionEvent::Return:			return "return";
		case DebugExecutionEvent::InterruptEntry:	return "interrupt-entry";
		case DebugExecutionEvent::InterruptReturn:	return "interrupt-return";
		case DebugExecutionEvent::Break:			return "break";
		case DebugExecutionEvent::StackReloaded:	return "stack-reloaded";
	}
	return "?";
}

bool DebugOpcodeStackDelta(u8 opcode, int *delta)
{
	switch (opcode)
	{
		case 0x20: *delta = -2; return true;	// JSR
		case 0x60: *delta = 2; return true;		// RTS
		case 0x40: *delta = 3; return true;		// RTI
		case 0x00: *delta = -3; return true;	// BRK
		case 0x48:								// PHA
		case 0x08: *delta = -1; return true;	// PHP
		case 0x68:								// PLA
		case 0x28: *delta = 1; return true;		// PLP
		case 0x9A: *delta = 0; return false;	// TXS
		default: *delta = 0; return true;
	}
}

DebugExecutionEvent DebugClassifyBoundary(u8 previousOpcode, u8 previousSp, u8 sp)
{
	int delta;
	if (!DebugOpcodeStackDelta(previousOpcode, &delta))
		return DebugExecutionEvent::StackReloaded;

	// An interrupt taken after the instruction pushes PC and P on top of
	// whatever the instruction itself did.
	int moved = DebugStackDistance(previousSp, sp);
	if (moved == delta - 3)
		return DebugExecutionEvent::InterruptEntry;

	switch (previousOpcode)
	{
		case 0x20: return DebugExecutionEvent::Call;
		case 0x60: return DebugExecutionEvent::Return;
		case 0x40: return DebugExecutionEvent::InterruptReturn;
		case 0x00: return DebugExecutionEvent::Break;
		default:   return DebugExecutionEvent::Sequential;
	}
}

// CDebugInterface side (RD-011): the boundary history and the observer list.

void CDebugInterface::NotifyInstructionBoundary(u16 pc, u8 sp, u8 opcode)
{
	int restart = boundaryHistoryRestart.exchange(BoundaryHistoryKeep, std::memory_order_acq_rel);
	if (restart != BoundaryHistoryKeep)
	{
		// A step seeds the history with the boundary it starts from, so the
		// first instruction it runs is classified too (an IRQ taken right
		// after it is an interrupt entry, not an unexplained first boundary).
		hasLastBoundary = (restart == BoundaryHistorySeed);
		lastBoundary = boundaryHistorySeed;
		boundarySequence = 0;
	}

	CDebugInstructionBoundary boundary;
	boundary.pc = pc;
	boundary.sp = sp;
	boundary.opcode = opcode;
	boundary.sequence = boundarySequence++;
	if (hasLastBoundary)
	{
		boundary.previousPc = lastBoundary.pc;
		boundary.previousOpcode = lastBoundary.opcode;
		boundary.previousSp = lastBoundary.sp;
		boundary.event = DebugClassifyBoundary(lastBoundary.opcode, lastBoundary.sp, sp);
	}
	lastBoundary = boundary;
	hasLastBoundary = true;

	bool stepperArmed = instructionStepper->IsArmed();
	if (stepperArmed)
	{
		instructionStepper->OnInstructionBoundary(this, boundary);
	}

	std::vector<CDebugExecutionObserver *> observers;
	{
		std::lock_guard<std::mutex> lock(executionObserversMutex);
		observers = executionObservers;
	}
	for (CDebugExecutionObserver *observer : observers)
	{
		observer->OnInstructionBoundary(this, boundary);
	}

	if (stepperArmed && !instructionStepper->IsArmed())
	{
		UpdateExecutionObserved();
	}
}

void CDebugInterface::AddExecutionObserver(CDebugExecutionObserver *observer)
{
	{
		std::lock_guard<std::mutex> lock(executionObserversMutex);
		executionObservers.push_back(observer);
	}
	if (!instructionStepper->IsArmed())
		boundaryHistoryRestart.store(BoundaryHistoryReset, std::memory_order_release);
	UpdateExecutionObserved();
}

void CDebugInterface::RemoveExecutionObserver(CDebugExecutionObserver *observer)
{
	{
		std::lock_guard<std::mutex> lock(executionObserversMutex);
		for (auto it = executionObservers.begin(); it != executionObservers.end(); )
		{
			if (*it == observer)
				it = executionObservers.erase(it);
			else
				++it;
		}
	}
	UpdateExecutionObserved();
}

void CDebugInterface::UpdateExecutionObserved()
{
	bool observed = instructionStepper->IsArmed();
	if (!observed)
	{
		std::lock_guard<std::mutex> lock(executionObserversMutex);
		observed = !executionObservers.empty();
	}
	executionObserved.store(observed, std::memory_order_release);
}

// RD-037: step-over and step-out armed on the stepper. Both expect the CPU
// paused at an instruction boundary; while running they only pause.

bool CDebugInterface::BeginInstructionStep(u16 *pc, u8 *sp, u8 *opcode)
{
	if (GetDebugMode() == DEBUGGER_MODE_RUNNING)
	{
		SetDebugMode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
		return false;
	}

	ClearTemporaryBreakpoint();
	if (snapshotsManager)
		snapshotsManager->CancelRestore();

	u8 a, x, y, p;
	GetCpuRegs(pc, &a, &x, &y, &p, sp);

	*opcode = 0;
	bool available = false;
	CDebugDataAdapter *adapter = GetDataAdapter();
	if (adapter)
		adapter->AdapterReadByte(*pc, opcode, &available);
	if (!available)
		*opcode = 0xEA;		// unreadable: classify the first instruction as plain

	// The emulation thread is parked, so the seed is in place before RUNNING.
	boundaryHistorySeed = CDebugInstructionBoundary();
	boundaryHistorySeed.pc = *pc;
	boundaryHistorySeed.sp = *sp;
	boundaryHistorySeed.opcode = *opcode;
	boundaryHistoryRestart.store(BoundaryHistorySeed, std::memory_order_release);
	return true;
}

void CDebugInterface::StepOverSubroutineObserved()
{
	LockMutex();
	u16 pc;
	u8 sp, opcode;
	if (BeginInstructionStep(&pc, &sp, &opcode))
	{
		if (opcode == 0x20)
		{
			instructionStepper->ArmStepOver(pc, (u16)(pc + 3), sp);
			UpdateExecutionObserved();
			LOGD("CDebugInterface::StepOverSubroutine: JSR at %04x, return %04x, sp=%02x", pc, (u16)(pc + 3), sp);
			SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
		else
		{
			SetDebugMode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
		}
	}
	UnlockMutex();
}

bool CDebugInterface::StepOutSubroutine()
{
	if (!SupportsExecutionEvents())
	{
		LOGError("CDebugInterface::StepOutSubroutine: %s does not report execution events", GetPlatformNameString());
		return false;
	}

	LockMutex();
	u16 pc;
	u8 sp, opcode;
	bool armed = BeginInstructionStep(&pc, &sp, &opcode);
	if (armed)
	{
		instructionStepper->ArmStepOut(pc, sp);
		UpdateExecutionObserved();
		LOGD("CDebugInterface::StepOutSubroutine: from %04x, sp=%02x", pc, sp);
		SetDebugMode(DEBUGGER_MODE_RUNNING);
	}
	UnlockMutex();
	return armed;
}
