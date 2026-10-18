#include "CTestInstructionStepping.h"
#include "CTestNesCartridge.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "CDebugExecutionEvents.h"
#include "CDebugInstructionStepper.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "NesWrapper.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>
#include <vector>

bool CTestInstructionStepping::Check(bool condition, const char *message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message);
	return true;
}

static bool WaitPaused(CDebugInterface *di, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs; waited += 10)
	{
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
			return true;
		SYS_Sleep(10);
	}
	return di->GetDebugMode() == DEBUGGER_MODE_PAUSED;
}

static void Regs(CDebugInterface *di, u16 *pc, u8 *sp, u8 *x = NULL)
{
	u8 a, xr, y, p;
	di->GetCpuRegs(pc, &a, &xr, &y, &p, sp);
	if (x)
		*x = xr;
}

static void Poke(CDebugInterface *di, u16 addr, const std::vector<u8> &bytes)
{
	for (size_t i = 0; i < bytes.size(); i++)
		di->GetDataAdapter()->AdapterWriteByte((int)(addr + i), bytes[i]);
}

static u8 Peek(CDebugInterface *di, u16 addr)
{
	u8 value = 0;
	di->GetDataAdapter()->AdapterReadByte(addr, &value);
	return value;
}

static void SetBreakpoint(CDebugInterface *di, u16 addr, bool set)
{
	di->LockMutex();
	CDebugSymbolsSegment *segment = di->symbols->currentSegment;
	if (set)
		segment->AddBreakpointPC(addr);
	else
		segment->breakpointsPC->DeleteBreakpoint(addr);
	di->UnlockMutex();
}

// Records the derived events while a step runs.
class CEventRecorder : public CDebugExecutionObserver
{
public:
	std::atomic<int> counts[8];
	CEventRecorder() { for (int i = 0; i < 8; i++) counts[i] = 0; }
	virtual void OnInstructionBoundary(CDebugInterface *, const CDebugInstructionBoundary &boundary) override
	{
		counts[(int)boundary.event]++;
	}
	int Count(DebugExecutionEvent event) { return counts[(int)event].load(); }
};

bool CTestInstructionStepping::RunClassifier()
{
	bool ok = DebugClassifyBoundary(0x20, 0xF0, 0xEE) == DebugExecutionEvent::Call
		&& DebugClassifyBoundary(0x60, 0xEE, 0xF0) == DebugExecutionEvent::Return
		&& DebugClassifyBoundary(0x40, 0xED, 0xF0) == DebugExecutionEvent::InterruptReturn
		&& DebugClassifyBoundary(0x00, 0xF0, 0xED) == DebugExecutionEvent::Break
		&& DebugClassifyBoundary(0xEA, 0xF0, 0xF0) == DebugExecutionEvent::Sequential
		&& DebugClassifyBoundary(0x9A, 0xF0, 0x40) == DebugExecutionEvent::StackReloaded;
	if (!Check(ok, "Boundary classifier: JSR, RTS, RTI, BRK, plain and TXS from the previous opcode and SP"))
		return false;

	// An interrupt taken after an instruction adds its three bytes on top of
	// the instruction's own stack change, wrapping included.
	ok = DebugClassifyBoundary(0xEA, 0xF0, 0xED) == DebugExecutionEvent::InterruptEntry
		&& DebugClassifyBoundary(0x20, 0xF0, 0xEB) == DebugExecutionEvent::InterruptEntry
		&& DebugClassifyBoundary(0x60, 0x01, 0x00) == DebugExecutionEvent::InterruptEntry
		&& DebugStackDistance(0xFE, 0x01) == 3 && DebugStackDistance(0x01, 0xFE) == -3;
	return Check(ok, "Boundary classifier: an IRQ/NMI after any instruction (JSR, RTS across the wrap) is an interrupt entry");
}

// The common program, at BASE (counter in zero page COUNTERZP):
//   +$00 NOP                 jump target (runs before the first boundary)
//   +$01 JSR +$10            returns to +$04
//   +$04 JMP +$04
//   +$10 DEC zp
//   +$12 BEQ +$17
//   +$14 JSR +$10            the step-over point, one frame deep
//   +$17 RTS
bool CTestInstructionStepping::RunRecursion(CDebugInterface *di, const char *machine, u16 base, u8 counterZp,
											std::function<void(u16)> jump)
{
	char msg[256];
	u16 sub = base + 0x10;
	Poke(di, base, { 0xEA, 0x20, (u8)sub, (u8)(sub >> 8), 0x4C, (u8)(base + 4), (u8)((base + 4) >> 8) });
	Poke(di, sub, { 0xC6, counterZp, 0xF0, 0x03, 0x20, (u8)sub, (u8)(sub >> 8), 0x60 });
	Poke(di, counterZp, { 3 });

	SetBreakpoint(di, sub + 4, true);
	jump(base);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	bool paused = WaitPaused(di, 3000);
	SetBreakpoint(di, sub + 4, false);

	u16 pc;
	u8 sp;
	Regs(di, &pc, &sp);
	snprintf(msg, sizeof(msg), "%s: a breakpoint stops the recursive routine one frame deep (PC $%04X, counter %d)",
			 machine, pc, Peek(di, counterZp));
	if (!Check(paused && pc == sub + 4 && Peek(di, counterZp) == 2, msg))
		return false;

	u8 startSp = sp;
	di->StepOverSubroutine();
	paused = WaitPaused(di, 3000);
	Regs(di, &pc, &sp);
	snprintf(msg, sizeof(msg),
			 "%s: step-over of the recursive JSR stops at $%04X with SP back at $%02X after all frames returned "
			 "(PC $%04X, SP $%02X, counter %d, %s) -- the PC+3 breakpoint stopped one frame deep",
			 machine, sub + 7, startSp, pc, sp, Peek(di, counterZp),
			 DebugStepStopReasonName(di->instructionStepper->GetLastStopReason()));
	if (!Check(paused && pc == sub + 7 && sp == startSp && Peek(di, counterZp) == 0
			   && di->instructionStepper->GetLastStopReason() == DebugStepStopReason::Returned
			   && !di->IsObservingExecution(), msg))
		return false;

	di->StepOutSubroutine();
	paused = WaitPaused(di, 3000);
	Regs(di, &pc, &sp);
	snprintf(msg, sizeof(msg), "%s: step-out returns to the caller at $%04X with SP $%02X (PC $%04X, SP $%02X, %s)",
			 machine, base + 4, (u8)(startSp + 2), pc, sp,
			 DebugStepStopReasonName(di->instructionStepper->GetLastStopReason()));
	return Check(paused && pc == base + 4 && sp == (u8)(startSp + 2)
				 && di->instructionStepper->GetLastKind() == DebugStepKind::StepOut
				 && di->instructionStepper->GetLastStopReason() == DebugStepStopReason::Returned, msg);
}

bool CTestInstructionStepping::RunC64()
{
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!Check(di != NULL && di->SupportsExecutionEvents(), "C64: the interface reports execution events"))
		return false;

	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();

	auto jump = [di](u16 addr) { di->MakeJmpC64(addr); };
	bool ok = RunRecursion(di, "C64", 0xC000, 0xFB, jump);

	// Self-contained, so it does not depend on how far the KERNAL has booted:
	// $C060 SEI, CIA1 timer A IRQ every 4000 cycles, CLI, JMP $C030
	// $C030 JSR $C020           the step-over point; returns to $C033
	// $C033 INC $FC             also reached from the IRQ handler
	// $C035 RTS
	// $C020 LDX #$40 / LDY #0 / DEY / BNE / DEX / BNE / RTS   (~80k cycles)
	// $C040 JSR $C033, ack CIA1 and VIC, PLA/TAY/PLA/TAX/PLA, RTI  (via $0314)
	char msg[256];
	u8 savedVector[2] = { Peek(di, 0x0314), Peek(di, 0x0315) };
	if (ok)
	{
		Poke(di, 0xC020, { 0xA2, 0x40, 0xA0, 0x00, 0x88, 0xD0, 0xFD, 0xCA, 0xD0, 0xF8, 0x60 });
		Poke(di, 0xC030, { 0x20, 0x20, 0xC0, 0xE6, 0xFC, 0x60 });
		Poke(di, 0xC040, { 0x20, 0x33, 0xC0, 0xAD, 0x0D, 0xDC, 0xA9, 0xFF, 0x8D, 0x19, 0xD0,
						   0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40 });
		Poke(di, 0xC060, { 0x78, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0xAD, 0x0D, 0xDC,
						   0xA9, 0xA0, 0x8D, 0x04, 0xDC, 0xA9, 0x0F, 0x8D, 0x05, 0xDC,
						   0xA9, 0x81, 0x8D, 0x0D, 0xDC, 0xA9, 0x11, 0x8D, 0x0E, 0xDC,
						   0x58, 0x4C, 0x30, 0xC0 });
		Poke(di, 0x0314, { 0x40, 0xC0 });

		// A user breakpoint inside the subroutine stops first and ends the step.
		SetBreakpoint(di, 0xC030, true);
		jump(0xC060);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		bool paused = WaitPaused(di, 3000);
		SetBreakpoint(di, 0xC030, false);
		SetBreakpoint(di, 0xC027, true);
		di->StepOverSubroutine();
		paused = paused && WaitPaused(di, 3000);
		SetBreakpoint(di, 0xC027, false);
		u16 pc;
		u8 sp;
		Regs(di, &pc, &sp);
		snprintf(msg, sizeof(msg), "C64: a user breakpoint inside the stepped-over routine wins and ends the step (PC $%04X, %s)",
				 pc, DebugStepStopReasonName(di->instructionStepper->GetLastStopReason()));
		ok = Check(paused && pc == 0xC027 && !di->instructionStepper->IsArmed() && !di->IsObservingExecution()
				   && di->instructionStepper->GetLastStopReason() == DebugStepStopReason::Interrupted, msg);
	}
	if (ok)
	{
		// IRQs run the handler through $C033 while the step is armed.
		Poke(di, 0xFC, { 0 });
		SetBreakpoint(di, 0xC030, true);
		jump(0xC060);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		bool paused = WaitPaused(di, 3000);
		SetBreakpoint(di, 0xC030, false);

		u16 pc;
		u8 sp, x;
		Regs(di, &pc, &sp);
		u8 startSp = sp;
		u8 irqPassesBefore = Peek(di, 0xFC);
		CEventRecorder recorder;
		di->AddExecutionObserver(&recorder);
		di->StepOverSubroutine();
		paused = paused && WaitPaused(di, 5000);
		di->RemoveExecutionObserver(&recorder);
		Regs(di, &pc, &sp, &x);
		u8 irqPasses = Peek(di, 0xFC) - irqPassesBefore;
		snprintf(msg, sizeof(msg),
				 "C64: step-over ignores $C033 inside IRQ handlers (%d passes) and stops there only on return "
				 "(PC $%04X, SP $%02X/$%02X, X %d, %s)",
				 irqPasses, pc, sp, startSp, x, DebugStepStopReasonName(di->instructionStepper->GetLastStopReason()));
		ok = Check(paused && pc == 0xC033 && sp == startSp && x == 0 && irqPasses >= 1
				   && di->instructionStepper->GetLastStopReason() == DebugStepStopReason::Returned, msg);
		if (ok)
		{
			snprintf(msg, sizeof(msg), "C64: the events seen meanwhile include calls (%d), returns (%d), interrupt entries (%d) and RTIs (%d)",
					 recorder.Count(DebugExecutionEvent::Call), recorder.Count(DebugExecutionEvent::Return),
					 recorder.Count(DebugExecutionEvent::InterruptEntry), recorder.Count(DebugExecutionEvent::InterruptReturn));
			ok = Check(recorder.Count(DebugExecutionEvent::Call) >= 1 && recorder.Count(DebugExecutionEvent::Return) >= 1
					   && recorder.Count(DebugExecutionEvent::InterruptEntry) >= 1
					   && recorder.Count(DebugExecutionEvent::InterruptEntry) == recorder.Count(DebugExecutionEvent::InterruptReturn)
					   && !di->IsObservingExecution(), msg);
		}
	}
	// The KERNAL's own vector back, and its PAL timer A latch ($4025).
	Poke(di, 0x0314, { savedVector[0], savedVector[1] });
	Poke(di, 0xDC04, { 0x25, 0x40 });

	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);
	return ok;
}

bool CTestInstructionStepping::RunAtari()
{
	CDebugInterfaceAtari *di = viewC64->debugInterfaceAtari;
	if (!Check(di != NULL && di->SupportsExecutionEvents(), "Atari: the interface reports execution events"))
		return false;

	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();

	auto jump = [di](u16 addr) { di->MakeJmpNoReset(di->GetDataAdapter(), addr); };
	bool ok = RunRecursion(di, "Atari", 0x0600, 0xCB, jump);

	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);
	return ok;
}

// A minimal NROM whose reset code disables NMI and IRQ and spins; the test
// program itself runs from internal RAM.
static bool WriteSpinRom(const char *path)
{
	std::vector<u8> rom(16 + 0x4000, 0xFF);
	memset(&rom[0], 0, 16);
	rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
	rom[4] = 1;
	u8 *prg = &rom[16];
	const u8 code[] = { 0x78, 0xA9, 0x00, 0x8D, 0x00, 0x20, 0xA2, 0xFF, 0x9A, 0x4C, 0x09, 0xC0, 0x40 };
	memcpy(prg, code, sizeof(code));		// $C000 SEI / LDA #0 / STA $2000 / LDX #$FF / TXS / JMP $C009 ; $C00C RTI
	prg[0x3FFA] = 0x0C; prg[0x3FFB] = 0xC0;
	prg[0x3FFC] = 0x00; prg[0x3FFD] = 0xC0;
	prg[0x3FFE] = 0x0C; prg[0x3FFF] = 0xC0;
	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	bool written = fwrite(rom.data(), 1, rom.size(), f) == rom.size();
	fclose(f);
	return written;
}

bool CTestInstructionStepping::RunNes()
{
	CDebugInterfaceNes *di = viewC64->debugInterfaceNes;
	if (!Check(di != NULL && di->SupportsExecutionEvents(), "NES: the interface reports execution events"))
		return false;

	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	const char *romPath = "/tmp/retrodebugger_test_stepping.nes";
	bool ok = Check(WriteSpinRom(romPath) && TestNesInsertCartridge(di, romPath, true),
					"NES: a spinning NROM is loaded so the CPU runs");
	if (ok)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(200);
		di->PauseEmulationBlockedWait();

		auto jump = [di](u16 addr) { di->MakeJmpNoReset(di->GetDataAdapter(), addr); };
		ok = RunRecursion(di, "NES", 0x0300, 0x10, jump);
		TestNesUnloadCartridge(di, true);
	}
	remove(romPath);

	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);
	return ok;
}

void CTestInstructionStepping::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = RunClassifier();
#ifdef RUN_COMMODORE64
	if (ok)
		ok = RunC64();
#else
	ReportRequiredGap("C64 emulation not compiled in");
#endif
#ifdef RUN_ATARI
	if (ok)
		ok = RunAtari();
#else
	ReportRequiredGap("Atari emulation not compiled in");
#endif
#ifdef RUN_NES
	if (ok)
		ok = RunNes();
#else
	ReportRequiredGap("NES emulation not compiled in");
#endif
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "Step-over and step-out follow the stack, not PC+3, on all three machines");
}
