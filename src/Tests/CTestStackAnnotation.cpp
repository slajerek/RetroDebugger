#include "CTestStackAnnotation.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "NesWrapper.h"
#include "CStackAnnotation.h"

#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

// Test program layout (starts at $0FFF):
//   $0FFF: RTS                         ; subroutine target — just returns
//   $1000: SEI                         ; disable interrupts first!
//   $1001: LDX #$FF                   ; prepare to reset SP
//   $1003: TXS                         ; SP = $FF (deterministic)
//   $1004: LDA #$30
//   $1006: PHA                         ; push $30 → $01FF, SP=$FE
//   $1007: PHP                         ; push status → $01FE, SP=$FD
//   $1008: JSR $0FFF                   ; push $100A → $01FD/$01FC, SP=$FB; RTS returns
//   $100B: JSR $1011                   ; push $100D → $01FD/$01FC (overwrites), SP=$FB
//   $100E: JMP $100E                   ; never reached
//   $1011: JMP $1011                   ; subroutine loops forever
//
// Expected final state: SP = $FB, PC = $1011
//   $FF: VALUE  (PHA),     origin $1006
//   $FE: STATUS (PHP),     origin $1007
//   $FD: JSR_PCH,          origin $100B
//   $FC: JSR_PCL,          origin $100B

static const u8 testCode[] = {
	// $0FFF: RTS
	0x60,
	// $1000: SEI
	0x78,
	// $1001: LDX #$FF
	0xA2, 0xFF,
	// $1003: TXS
	0x9A,
	// $1004: LDA #$30
	0xA9, 0x30,
	// $1006: PHA
	0x48,
	// $1007: PHP
	0x08,
	// $1008: JSR $0FFF
	0x20, 0xFF, 0x0F,
	// $100B: JSR $1011
	0x20, 0x11, 0x10,
	// $100E: JMP $100E
	0x4C, 0x0E, 0x10,
	// $1011: JMP $1011
	0x4C, 0x11, 0x10,
};

static char failureMsg[512];

void CTestStackAnnotation::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	failureMsg[0] = '\0';
	bool allPassed = true;

#ifdef RUN_COMMODORE64
	if (!TestC64Stack())
		allPassed = false;
#endif

#ifdef RUN_ATARI
	if (allPassed && !TestAtariStack())
		allPassed = false;
#endif

#ifdef RUN_NES
	if (allPassed && !TestNesStack())
		allPassed = false;
#endif

	if (allPassed)
	{
		TestCompleted(true, "Stack annotations correct for all platforms");
	}
	else
	{
		TestCompleted(false, failureMsg);
	}
}

static bool VerifyStackAnnotations(const char *platform, CStackAnnotationData *sa, u8 sp, char *msg)
{
	// Verify SP = $FB
	if (sp != 0xFB)
	{
		sprintf(msg, "%s: Expected SP=$FB, got SP=$%02X", platform, sp);
		return false;
	}

	// $FF: VALUE (PHA), origin $1006
	if (sa->entryTypes[0xFF] != STACK_ENTRY_VALUE)
	{
		sprintf(msg, "%s: $FF expected VALUE(%d), got %d", platform, STACK_ENTRY_VALUE, sa->entryTypes[0xFF]);
		return false;
	}
	if (sa->originPC[0xFF] != 0x1006)
	{
		sprintf(msg, "%s: $FF origin expected $1006, got $%04X", platform, sa->originPC[0xFF]);
		return false;
	}

	// $FE: STATUS (PHP), origin $1007
	if (sa->entryTypes[0xFE] != STACK_ENTRY_STATUS)
	{
		sprintf(msg, "%s: $FE expected STATUS(%d), got %d", platform, STACK_ENTRY_STATUS, sa->entryTypes[0xFE]);
		return false;
	}
	if (sa->originPC[0xFE] != 0x1007)
	{
		sprintf(msg, "%s: $FE origin expected $1007, got $%04X", platform, sa->originPC[0xFE]);
		return false;
	}

	// $FD: JSR_PCH, origin $100B
	if (sa->entryTypes[0xFD] != STACK_ENTRY_JSR_PCH)
	{
		sprintf(msg, "%s: $FD expected JSR_PCH(%d), got %d", platform, STACK_ENTRY_JSR_PCH, sa->entryTypes[0xFD]);
		return false;
	}
	if (sa->originPC[0xFD] != 0x100B)
	{
		sprintf(msg, "%s: $FD origin expected $100B, got $%04X", platform, sa->originPC[0xFD]);
		return false;
	}

	// $FC: JSR_PCL, origin $100B
	if (sa->entryTypes[0xFC] != STACK_ENTRY_JSR_PCL)
	{
		sprintf(msg, "%s: $FC expected JSR_PCL(%d), got %d", platform, STACK_ENTRY_JSR_PCL, sa->entryTypes[0xFC]);
		return false;
	}
	if (sa->originPC[0xFC] != 0x100B)
	{
		sprintf(msg, "%s: $FC origin expected $100B, got $%04X", platform, sa->originPC[0xFC]);
		return false;
	}

	return true;
}

bool CTestStackAnnotation::TestC64Stack()
{
	CDebugInterfaceC64 *di = (CDebugInterfaceC64 *)viewC64->debugInterfaceC64;
	if (!di)
	{
		sprintf(failureMsg, "C64: debug interface is NULL");
		StepCompleted(1, false, failureMsg);
		return false;
	}

	// Start emulator if not running
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}

	if (!di->isRunning)
	{
		sprintf(failureMsg, "C64: emulator failed to start");
		StepCompleted(1, false, failureMsg);
		return false;
	}

	// Pause emulator and wait for it to stop
	di->PauseEmulationBlockedWait();

	// Clear stack annotations
	di->mainCpuStack.Clear();

	// Write test code: $0FFF-$1013
	for (int i = 0; i < (int)sizeof(testCode); i++)
	{
		di->SetByteToRamC64(0x0FFF + i, testCode[i]);
	}

	// Set PC to $1000 (start after the RTS at $0FFF)
	di->MakeJmpC64(0x1000);

	// Run emulator
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);

	// Poll until PC reaches $1011 (JMP * in subroutine) or timeout
	int tries = 0;
	while (tries < 200)
	{
		SYS_Sleep(10);
		C64StateCPU cpuState;
		di->GetC64CpuState(&cpuState);
		if (cpuState.pc == 0x1011)
			break;
		tries++;
	}

	// Pause
	di->PauseEmulationBlockedWait();

	// Read SP directly from CPU registers (not from viciiStateToShow render snapshot)
	C64StateCPU cpuState;
	di->GetC64CpuState(&cpuState);
	u8 sp = cpuState.sp;

	if (!VerifyStackAnnotations("C64", &di->mainCpuStack, sp, failureMsg))
	{
		StepCompleted(1, false, failureMsg);
		return false;
	}

	// Verify stack memory value for PHA
	u8 val = 0;
	di->dataAdapterC64DirectRam->AdapterReadByte(0x01FF, &val);
	if (val != 0x30)
	{
		sprintf(failureMsg, "C64: Stack [$01FF] expected $30, got $%02X", val);
		StepCompleted(1, false, failureMsg);
		return false;
	}

	// Stop emulator if we started it
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	StepCompleted(1, true, "C64: Stack annotations verified (PHA, PHP, JSR)");
	return true;
}

bool CTestStackAnnotation::TestAtariStack()
{
	CDebugInterfaceAtari *di = (CDebugInterfaceAtari *)viewC64->debugInterfaceAtari;
	if (!di)
	{
		sprintf(failureMsg, "Atari: debug interface is NULL");
		StepCompleted(2, false, failureMsg);
		return false;
	}

	// Start emulator if not running
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}

	if (!di->isRunning)
	{
		sprintf(failureMsg, "Atari: emulator failed to start");
		StepCompleted(2, false, failureMsg);
		return false;
	}

	// Pause emulator
	di->PauseEmulationBlockedWait();

	// Clear stack annotations
	di->mainCpuStack.Clear();

	// Write test code (same 6502 opcodes)
	for (int i = 0; i < (int)sizeof(testCode); i++)
	{
		di->dataAdapter->AdapterWriteByte(0x0FFF + i, testCode[i]);
	}

	// Set PC to $1000
	di->MakeJmpNoReset(di->dataAdapter, 0x1000);

	// Run emulator
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);

	// Poll until PC reaches $1011 or timeout (2 seconds)
	int tries = 0;
	while (tries < 200)
	{
		SYS_Sleep(10);
		u16 pc; u8 a, x, y, flags, sp, irq;
		di->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);
		if (pc == 0x1011)
			break;
		tries++;
	}

	// Pause
	di->PauseEmulationBlockedWait();

	// Read SP
	u16 pc; u8 a, x, y, flags, sp, irq;
	di->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);

	// Debug info if SP is wrong
	if (sp != 0xFB)
	{
		sprintf(failureMsg, "Atari: Expected SP=$FB, got SP=$%02X PC=$%04X (tries=%d)", sp, pc, tries);
		StepCompleted(2, false, failureMsg);
		return false;
	}

	if (!VerifyStackAnnotations("Atari", &di->mainCpuStack, sp, failureMsg))
	{
		StepCompleted(2, false, failureMsg);
		return false;
	}

	// Verify stack memory value for PHA
	u8 val = 0;
	di->dataAdapter->AdapterReadByte(0x01FF, &val);
	if (val != 0x30)
	{
		sprintf(failureMsg, "Atari: Stack [$01FF] expected $30, got $%02X", val);
		StepCompleted(2, false, failureMsg);
		return false;
	}

	// Stop emulator if we started it
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	StepCompleted(2, true, "Atari: Stack annotations verified (PHA, PHP, JSR)");
	return true;
}

// NES requires a cartridge to function (unlike C64/Atari that boot to BASIC).
// We create a minimal NROM (mapper 0) cartridge with the test program in PRG-ROM.
//
// PRG-ROM layout (16KB at $C000-$FFFF):
//   $C000: RTI                       ; NMI/IRQ handler (just return)
//   $C001: RTS                       ; subroutine target
//   $C002: LDA #$00                  ; reset entry point
//   $C004: STA $2000                 ; disable NMI
//   $C007: SEI                       ; disable IRQ
//   $C008: LDX #$FF
//   $C00A: TXS                       ; SP = $FF
//   $C00B: LDA #$30
//   $C00D: PHA                       ; push $30 -> $01FF, SP=$FE
//   $C00E: PHP                       ; push status -> $01FE, SP=$FD
//   $C00F: JSR $C001                 ; push, RTS returns -> SP=$FB then back to $FD
//   $C012: JSR $C018                 ; push $C014 -> $01FD/$01FC, SP=$FB
//   $C015: JMP $C015                 ; never reached
//   $C018: JMP $C018                 ; subroutine loops forever
//
// Vectors: NMI=$C000, RESET=$C002, IRQ=$C000
//
// Expected final state: SP=$FB, PC=$C018
//   $FF: VALUE  (PHA),    origin $C00D
//   $FE: STATUS (PHP),    origin $C00E
//   $FD: JSR_PCH,         origin $C012
//   $FC: JSR_PCL,         origin $C012

static const char *NES_TEST_ROM_PATH = "/tmp/retrodebugger_test_stack.nes";

static bool CreateTestNesRom(const char *path)
{
	static const int PRG_SIZE = 16384;
	static const int HEADER_SIZE = 16;
	u8 rom[HEADER_SIZE + PRG_SIZE];
	memset(rom, 0xFF, sizeof(rom));

	// iNES header
	rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
	rom[4] = 1;    // 1 x 16KB PRG-ROM
	rom[5] = 0;    // 0 CHR-ROM (uses CHR-RAM)
	rom[6] = 0;    // Mapper 0, horizontal mirroring
	rom[7] = 0;
	memset(&rom[8], 0, 8);

	// PRG-ROM at offset 16, maps to $C000-$FFFF (NROM-128)
	u8 *prg = &rom[HEADER_SIZE];
	int off = 0;

	prg[off++] = 0x40;                                     // $C000: RTI
	prg[off++] = 0x60;                                     // $C001: RTS
	prg[off++] = 0xA9; prg[off++] = 0x00;                  // $C002: LDA #$00
	prg[off++] = 0x8D; prg[off++] = 0x00; prg[off++] = 0x20; // $C004: STA $2000
	prg[off++] = 0x78;                                     // $C007: SEI
	prg[off++] = 0xA2; prg[off++] = 0xFF;                  // $C008: LDX #$FF
	prg[off++] = 0x9A;                                     // $C00A: TXS
	prg[off++] = 0xA9; prg[off++] = 0x30;                  // $C00B: LDA #$30
	prg[off++] = 0x48;                                     // $C00D: PHA
	prg[off++] = 0x08;                                     // $C00E: PHP
	prg[off++] = 0x20; prg[off++] = 0x01; prg[off++] = 0xC0; // $C00F: JSR $C001
	prg[off++] = 0x20; prg[off++] = 0x18; prg[off++] = 0xC0; // $C012: JSR $C018
	prg[off++] = 0x4C; prg[off++] = 0x15; prg[off++] = 0xC0; // $C015: JMP $C015
	prg[off++] = 0x4C; prg[off++] = 0x18; prg[off++] = 0xC0; // $C018: JMP $C018

	// Interrupt vectors at end of PRG-ROM
	prg[0x3FFA] = 0x00; prg[0x3FFB] = 0xC0;  // NMI   -> $C000
	prg[0x3FFC] = 0x02; prg[0x3FFD] = 0xC0;  // RESET -> $C002
	prg[0x3FFE] = 0x00; prg[0x3FFF] = 0xC0;  // IRQ   -> $C000

	FILE *f = fopen(path, "wb");
	if (!f) return false;
	fwrite(rom, 1, sizeof(rom), f);
	fclose(f);
	return true;
}

bool CTestStackAnnotation::TestNesStack()
{
	CDebugInterfaceNes *di = (CDebugInterfaceNes *)viewC64->debugInterfaceNes;
	if (!di)
	{
		sprintf(failureMsg, "NES: debug interface is NULL");
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// Start emulator if not running
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}

	if (!di->isRunning)
	{
		sprintf(failureMsg, "NES: emulator failed to start");
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// Pause the emulator so we can safely load a cartridge
	di->SetDebugMode(DEBUGGER_MODE_PAUSED);
	SYS_Sleep(500);

	// Create and load a minimal test cartridge (this properly powers on the NES machine)
	if (!CreateTestNesRom(NES_TEST_ROM_PATH))
	{
		sprintf(failureMsg, "NES: failed to create test ROM at %s", NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	if (!nesd_insert_cartridge((char *)NES_TEST_ROM_PATH))
	{
		sprintf(failureMsg, "NES: failed to load test ROM");
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// Clear stack annotations (cartridge load resets machine state)
	di->mainCpuStack.Clear();

	// Run - the CPU boots from the reset vector ($C002) and executes test code
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);

	// Poll until PC reaches $C018 (JMP loop in subroutine) or timeout
	int tries = 0;
	while (tries < 200)
	{
		SYS_Sleep(10);
		u16 pc; u8 a, x, y, flags, sp, irq;
		di->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);
		if (pc == 0xC018)
			break;
		tries++;
	}

	// Pause
	di->PauseEmulationBlockedWait();

	// Read SP
	u16 pc; u8 a, x, y, flags, sp, irq;
	di->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);

	if (sp != 0xFB)
	{
		sprintf(failureMsg, "NES: SP=$%02X PC=$%04X (tries=%d)", sp, pc, tries);
		nesd_unload_cartridge();
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// Verify annotations
	CStackAnnotationData *sa = &di->mainCpuStack;

	// $FF: VALUE (PHA), origin $C00D
	if (sa->entryTypes[0xFF] != STACK_ENTRY_VALUE || sa->originPC[0xFF] != 0xC00D)
	{
		sprintf(failureMsg, "NES: $FF expected VALUE origin=$C00D, got type=%d origin=$%04X",
				sa->entryTypes[0xFF], sa->originPC[0xFF]);
		nesd_unload_cartridge();
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// $FE: STATUS (PHP), origin $C00E
	if (sa->entryTypes[0xFE] != STACK_ENTRY_STATUS || sa->originPC[0xFE] != 0xC00E)
	{
		sprintf(failureMsg, "NES: $FE expected STATUS origin=$C00E, got type=%d origin=$%04X",
				sa->entryTypes[0xFE], sa->originPC[0xFE]);
		nesd_unload_cartridge();
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// $FD: JSR_PCH, origin $C012
	if (sa->entryTypes[0xFD] != STACK_ENTRY_JSR_PCH || sa->originPC[0xFD] != 0xC012)
	{
		sprintf(failureMsg, "NES: $FD expected JSR_PCH origin=$C012, got type=%d origin=$%04X",
				sa->entryTypes[0xFD], sa->originPC[0xFD]);
		nesd_unload_cartridge();
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// $FC: JSR_PCL, origin $C012
	if (sa->entryTypes[0xFC] != STACK_ENTRY_JSR_PCL || sa->originPC[0xFC] != 0xC012)
	{
		sprintf(failureMsg, "NES: $FC expected JSR_PCL origin=$C012, got type=%d origin=$%04X",
				sa->entryTypes[0xFC], sa->originPC[0xFC]);
		nesd_unload_cartridge();
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// Verify stack memory value for PHA
	u8 val = 0;
	di->dataAdapter->AdapterReadByte(0x01FF, &val);
	if (val != 0x30)
	{
		sprintf(failureMsg, "NES: Stack [$01FF] expected $30, got $%02X", val);
		nesd_unload_cartridge();
		remove(NES_TEST_ROM_PATH);
		StepCompleted(3, false, failureMsg);
		return false;
	}

	// Cleanup: unload cartridge and remove temp file
	nesd_unload_cartridge();
	remove(NES_TEST_ROM_PATH);

	// Stop emulator if we started it
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	StepCompleted(3, true, "NES: Stack annotations verified (PHA, PHP, JSR)");
	return true;
}

void CTestStackAnnotation::Cancel()
{
	isRunning = false;
}
