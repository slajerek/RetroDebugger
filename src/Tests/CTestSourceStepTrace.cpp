#include "CTestSourceStepTrace.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "NesWrapper.h"
#include "AtariWrapper.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugDatabase.h"
#include "CDebugInfoLoader.h"
#include "CDebugSourceBreakpoint.h"
#include "CSourceStepController.h"
#include "CDebugInterfaceTask.h"
#include "C64SettingsStorage.h"
#include "CSlrString.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include "json.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

using json = nlohmann::json;

bool CTestSourceStepTrace::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

static bool ReadFile(const std::string &path, std::vector<u8> &out)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return false;
	u8 buffer[4096];
	size_t n;
	while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
		out.insert(out.end(), buffer, buffer + n);
	fclose(f);
	return true;
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

static bool ReadJson(const std::string &path, json &out)
{
	std::ifstream in(path);
	if (!in)
		return false;
	try
	{
		in >> out;
	}
	catch (const std::exception &)
	{
		return false;
	}
	return true;
}

// 0x080D, "$080D" or a plain number
static int ParseAddress(const json &value)
{
	if (value.is_number_integer())
		return value.get<int>();
	if (!value.is_string())
		return -1;
	std::string s = value.get<std::string>();
	if (!s.empty() && s[0] == '$')
		return (int)strtol(s.c_str() + 1, NULL, 16);
	return (int)strtol(s.c_str(), NULL, 0);
}

static SourceStepKind ParseAction(const std::string &action)
{
	std::string a = action;
	if (a.compare(0, 6, "source") == 0)
		a = a.substr(6);
	for (char &c : a)
		c = (char)tolower((unsigned char)c);
	if (a == "into")	return SourceStepKind::Into;
	if (a == "over")	return SourceStepKind::Over;
	if (a == "out")		return SourceStepKind::Out;
	return SourceStepKind::None;
}

// "completed", "StepCompleted", "frame-exited", "FrameExited" ... all match
static std::string NormalizeReason(const std::string &reason)
{
	std::string r;
	for (char c : reason)
	{
		if (c != '-' && c != '_')
			r += (char)tolower((unsigned char)c);
	}
	if (r.compare(0, 4, "step") == 0)
		r = r.substr(4);
	return r;
}

static const CDebugFunction *FindFunctionByName(const CDebugDatabase *database, const std::string &name)
{
	for (const CDebugFunction &function : database->Functions())
	{
		if (function.name == name)
			return &function;
	}
	return NULL;
}

// where each machine's interrupt handler counts itself (16 bit)
static u16 InterruptCounterAddress(const std::string &machine)
{
	if (machine == "atari")	return 0x06F0;	// page 6, free for the user
	if (machine == "nes")	return 0x04F0;	// RAM neither Oscar64 nor cc65 places anything in
	// not zero page: cc65 keeps its C stack pointer at $02/$03, and an IRQ
	// counting there corrupted every local and argument (RD-015)
	return 0x02A7;	// C64: $02A7-$02FF are unused by the KERNAL and both compilers
}

static bool WaitForPause(CDebugInterface *di, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs; waited += 10)
	{
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && !di->sourceStepper->IsArmed())
			return true;
		SYS_Sleep(10);
	}
	return false;
}

// A C64 paused before its reset routine ends keeps zeroed RAM vectors, and a
// trace then takes the CPU over for good: code that calls the KERNAL (cc65's
// crt0 does) jumps through $0326 to $0000. Wait, running, for the READY
// prompt's keyboard loop at $E5CD. A machine left in a previous test's
// program -- paused again by a BRK it runs into, or running code in RAM with
// its page 3 overwritten (macOS CI after SourceStepping) -- is hard reset
// first.
static bool WaitForKernalBoot(CDebugInterfaceVice *di)
{
	auto atReady = [di]() {
		u16 pc;
		u8 a, x, y, p, sp;
		di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
		return di->GetByteFromRamC64(0x0326) == 0xCA && di->GetByteFromRamC64(0x0327) == 0xF1
			&& di->GetByteFromRamC64(0x0314) != 0 && di->GetByteFromRamC64(0x0315) != 0
			&& pc >= 0xE5CD && pc <= 0xE5D5;
	};
	auto inRam = [di]() {
		u16 pc;
		u8 a, x, y, p, sp;
		di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
		return pc < 0xA000;
	};
	if (di->GetDebugMode() != DEBUGGER_MODE_RUNNING)
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	bool reset = false;
	for (int waited = 0; waited < 10000; waited += 20)
	{
		if (atReady())
			return true;
		if (di->GetDebugMode() != DEBUGGER_MODE_RUNNING || (waited >= 1000 && inRam()))
		{
			reset = true;
			break;
		}
		SYS_Sleep(20);
	}
	if (!reset)
		return false;
	LOGM("WaitForKernalBoot: the C64 is not booting to READY, hard reset");
	di->ResetHard();
	if (di->GetDebugMode() != DEBUGGER_MODE_RUNNING)
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	for (int waited = 0; waited < 10000; waited += 20)
	{
		if (atReady())
			return true;
		SYS_Sleep(20);
	}
	return false;
}

#ifdef RUN_NES
// NES cartridge changes run between two frames on the emulation thread, the
// app's InsertCartridge path: a paused NES thread sits inside Execute, and a
// board swapped from another thread under it crashed (Linux CI)
class CTaskPauseNes : public CDebugInterfaceTask
{
public:
	CTaskPauseNes(CDebugInterface *di) : di(di) {}
	virtual void ExecuteTask() override { di->SetDebugMode(DEBUGGER_MODE_PAUSED); }
private:
	CDebugInterface *di;
};

class CTaskUnloadNes : public CDebugInterfaceTask
{
public:
	virtual void ExecuteTask() override { nesd_unload_cartridge(); }
};
#endif

// the machine running: until every queued VSync task has run (each runs and
// is deleted under the tasks mutex)
static bool WaitForVSyncTasks(CDebugInterface *di, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs; waited += 10)
	{
		di->LockTasksMutex();
		bool empty = di->vsyncTasks.empty();
		di->UnlockTasksMutex();
		if (empty)
			return true;
		SYS_Sleep(10);
	}
	return false;
}

CTestSourceStepTrace::Machine *CTestSourceStepTrace::GetMachine(const std::string &name)
{
	for (Machine &machine : machines)
	{
		if (machine.name == name)
			return &machine;
	}
	CDebugInterface *di = NULL;
#ifdef RUN_COMMODORE64
	if (name == "c64")
		di = viewC64->debugInterfaceC64;
#endif
#ifdef RUN_ATARI
	if (name == "atari")
		di = viewC64->debugInterfaceAtari;
#endif
#ifdef RUN_NES
	if (name == "nes")
		di = viewC64->debugInterfaceNes;
#endif
	if (di == NULL)
		return NULL;

	Machine machine;
	machine.name = name;
	machine.di = di;
	machine.started = !di->isRunning;
	if (machine.started)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	if (name == "c64" && !WaitForKernalBoot((CDebugInterfaceVice *)di))
	{
		Check(false, "the C64 boots to the READY prompt (vectors at $0314/$0326 set), hard reset if needed");
		if (machine.started)
			viewC64->StopEmulationThread(di);
		return NULL;
	}
	if (name == "nes")
	{
		// may have no cartridge, so no CPU running to pause at
		di->SetDebugMode(DEBUGGER_MODE_PAUSED);
		SYS_Sleep(200);
	}
	else
	{
		di->PauseEmulationBlockedWait();
	}
	machine.liveSymbols = di->symbols;
	if (name == "c64")
	{
		machine.savedVector[0] = Peek(di, 0x0314);
		machine.savedVector[1] = Peek(di, 0x0315);
		machine.savedPort[0] = ((CDebugInterfaceVice *)di)->GetByteC64(0x0000);
		machine.savedPort[1] = ((CDebugInterfaceVice *)di)->GetByteC64(0x0001);
	}
	machines.push_back(machine);
	return &machines.back();
}

void CTestSourceStepTrace::RestoreMachines()
{
	for (Machine &machine : machines)
	{
		CDebugInterface *di = machine.di;
		di->SetDebugMode(DEBUGGER_MODE_PAUSED);
		if (machine.name == "c64")
		{
			if (machine.c64Cartridge && !DetachC64Cartridge(machine))
				LOGError("CTestSourceStepTrace: the C64 did not boot to READY after the cartridge detach");
			// KERNAL vector, 6510 port, PAL timer A latch ($4025) and the timer A IRQ back
			CDebugInterfaceVice *vice = (CDebugInterfaceVice *)di;
			vice->PauseEmulationBlockedWait();
			vice->SetByteC64(0x0000, machine.savedPort[0]);
			vice->SetByteC64(0x0001, machine.savedPort[1]);
			vice->SetByteToRamC64(0x0314, machine.savedVector[0]);
			vice->SetByteToRamC64(0x0315, machine.savedVector[1]);
			vice->SetByteC64(0xDC04, 0x25);
			vice->SetByteC64(0xDC05, 0x40);
			vice->SetByteC64(0xDC0D, 0x81);
		}
		else if (machine.name == "atari")
			RemoveAtariInterrupt(machine);
#ifdef RUN_NES
		if (machine.name == "nes")
			di->AddVSyncTask(new CTaskUnloadNes());		// runs once the machine runs again, below
#endif
		guiMain->LockMutex();
		di->LockMutex();
		di->symbols = machine.liveSymbols;
		di->UnlockMutex();
		guiMain->UnlockMutex();
		// scratch symbols leaked on purpose, see CTestSymbolLoaders

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		if (machine.name == "nes" && !WaitForVSyncTasks(di, 5000))
			LOGError("CTestSourceStepTrace: the NES cartridge unload task never ran");
		if (machine.started)
			viewC64->StopEmulationThread(di);
	}
	machines.clear();
}

// The C64 preamble at $C060: SEI, CIA1 interrupts off, JMP entry. With "irq"
// the KERNAL IRQ vector goes to the $C040 handler (16-bit counter, optional
// JSR to a fixture function, ack, RTI); its timer starts at the start line
// (InstallInterrupt): the callee may write C variables, and cc65's BSS
// overlays its ONCE startup code -- tick()'s INC _ticks patched initlib.
bool CTestSourceStepTrace::LaunchC64(CDebugInterface *debugInterface, const json &trace, const std::vector<u8> &binary,
									 const CDebugDatabase *database, const std::string &name)
{
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)debugInterface;
	int entry = trace.contains("entry") ? ParseAddress(trace["entry"]) : -1;
	if (!Check(binary.size() > 2 && entry >= 0, name + ": the PRG has a load address and the trace names an entry point"))
		return false;
	u16 loadAddress = (u16)(binary[0] | (binary[1] << 8));
	for (size_t i = 2; i < binary.size(); i++)
		di->SetByteToRamC64((u16)(loadAddress + i - 2), binary[i]);

	std::vector<u8> setup = { 0x78, 0xA9, 0x7F, 0x8D, 0x0D, 0xDC, 0xAD, 0x0D, 0xDC };
	if (trace.contains("irq"))
	{
		const json &irq = trace["irq"];
		u16 counter = InterruptCounterAddress("c64");
		std::vector<u8> handler = { 0xEE, (u8)(counter & 0xFF), (u8)(counter >> 8), 0xD0, 0x03,	// 16-bit counter
									0xEE, (u8)((counter + 1) & 0xFF), (u8)((counter + 1) >> 8) };
		if (irq.contains("call"))
		{
			std::string callee = irq["call"].get<std::string>();
			const CDebugFunction *function = FindFunctionByName(database, callee);
			if (!Check(function != NULL, name + ": the IRQ handler's callee " + callee + " is a function of the debug info"))
				return false;
			handler.insert(handler.end(), { 0x20, (u8)(function->start & 0xFF), (u8)(function->start >> 8) });
		}
		handler.insert(handler.end(), { 0xAD, 0x0D, 0xDC, 0xA9, 0xFF, 0x8D, 0x19, 0xD0,
										0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40 });
		for (size_t i = 0; i < handler.size(); i++)
			di->SetByteToRamC64((u16)(0xC040 + i), handler[i]);
		di->SetByteToRamC64(0x0314, 0x40);
		di->SetByteToRamC64(0x0315, 0xC0);
	}
	setup.insert(setup.end(), { 0x4C, (u8)(entry & 0xFF), (u8)(entry >> 8) });
	for (size_t i = 0; i < setup.size(); i++)
		di->SetByteToRamC64((u16)(0xC060 + i), setup[i]);
	Poke(di, InterruptCounterAddress("c64"), { 0, 0 });
	di->MakeJmpC64(0xC060);
	return true;
}

// A C64 .crt: attached from the file (VICE resets into it), so the fetched
// ROM bytes have their .crt file offsets. The attach pauses briefly by itself
// while it resets: the launch waits for the pause at the start breakpoint.
// The user's cartridge setting is kept aside until RestoreMachines.
bool CTestSourceStepTrace::LaunchC64Cartridge(Machine &machine, const std::string &binaryPath, u16 startAddress,
											  const std::string &name)
{
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)machine.di;
	if (!machine.c64Cartridge)
	{
		guiMain->LockMutex();
		machine.savedCartridgePath = c64SettingsPathToCartridge;
		c64SettingsPathToCartridge = NULL;
		guiMain->UnlockMutex();
		machine.c64Cartridge = true;
	}
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	CSlrString *path = new CSlrString(binaryPath.c_str());
	di->AttachCartridge(path);
	delete path;
	for (int waited = 0; waited < 5000; waited += 10)
	{
		SYS_Sleep(10);
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED && (u16)di->GetCpuPC() == startAddress)
			return true;
	}
	return Check(false, name + ": the cartridge attaches and runs to the start line");
}

// the cartridge off (VICE resets), the C64 back at the READY prompt, paused
bool CTestSourceStepTrace::DetachC64Cartridge(Machine &machine)
{
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)machine.di;
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	di->DetachCartridge();
	machine.c64Cartridge = false;
	guiMain->LockMutex();
	if (c64SettingsPathToCartridge)
		delete c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = machine.savedCartridgePath;
	machine.savedCartridgePath = NULL;
	guiMain->UnlockMutex();
	SYS_Sleep(200);
	bool ready = WaitForKernalBoot(di);
	di->PauseEmulationBlockedWait();
	return ready;
}

// An Atari XEX: its segments go straight into memory, then a jump to the
// trace's entry or the RUNAD ($02E0) segment.
bool CTestSourceStepTrace::LaunchAtari(CDebugInterface *di, const json &trace, const std::vector<u8> &binary,
									   const std::string &name)
{
	int entry = trace.contains("entry") ? ParseAddress(trace["entry"]) : -1;
	size_t i = 0;
	bool valid = binary.size() > 6 && binary[0] == 0xFF && binary[1] == 0xFF;
	while (valid && i + 4 <= binary.size())
	{
		if (binary[i] == 0xFF && binary[i + 1] == 0xFF)
			i += 2;
		int start = binary[i] | (binary[i + 1] << 8);
		int end = binary[i + 2] | (binary[i + 3] << 8);
		i += 4;
		if (end < start || i + (end - start + 1) > binary.size())
		{
			valid = false;
			break;
		}
		for (int addr = start; addr <= end; addr++)
		{
			u8 value = binary[i + addr - start];
			if (addr == 0x02E0 && entry < 0 && end >= 0x02E1)
				entry = value | (binary[i + addr - start + 1] << 8);
			if (addr < 0x02E0 || addr > 0x02E3)		// RUNAD/INITAD stay the OS's
				Poke(di, (u16)addr, { value });
		}
		i += end - start + 1;
	}
	if (!Check(valid && entry >= 0, name + ": the XEX segments load and give an entry point"))
		return false;
	di->MakeJmpNoReset(di->GetDataAdapter(), (u16)entry);
	return true;
}

// A NES cartridge: inserted (a reset), then with "entry" the CPU is sent there
// -- for builds that write no reset vector.
bool CTestSourceStepTrace::LaunchNes(CDebugInterface *di, const json &trace, const std::string &binaryPath,
									 const std::string &name)
{
#ifdef RUN_NES
	std::string cartridge = binaryPath;
	if (trace.contains("irq"))
	{
		// the IRQ vector (the last PRG bank's $FFFE) to the handler at $0400, in a copy
		std::vector<u8> image;
		ReadFile(binaryPath, image);
		size_t prgEnd = image.size() >= 16 ? 16 + ((image[6] & 0x04) ? 512 : 0) + (size_t)image[4] * 16384 : 0;
		if (!Check(image.size() >= 16 && memcmp(image.data(), "NES\x1A", 4) == 0 && image[4] > 0 && prgEnd <= image.size(),
				   name + ": the cartridge has an iNES header and its PRG ROM"))
			return false;
		image[prgEnd - 2] = 0x00;
		image[prgEnd - 1] = 0x04;
		cartridge = "/tmp/source-step-trace-irq.nes";
		std::ofstream(cartridge, std::ios::binary).write((const char *)image.data(), (std::streamsize)image.size());
	}
	// on the emulation thread; the second task pauses the new board at its reset
	((CDebugInterfaceNes *)di)->InsertCartridge((char *)cartridge.c_str());
	di->AddVSyncTask(new CTaskPauseNes(di));
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!Check(WaitForVSyncTasks(di, 5000) && WaitForPause(di, 2000), name + ": the cartridge inserts, paused at its reset"))
		return false;
	if (trace.contains("entry"))
		di->MakeJmpNoReset(di->GetDataAdapter(), (u16)ParseAddress(trace["entry"]));
	return true;
#else
	return false;
#endif
}

// The interrupt is installed at the start line, after the program's own
// startup ran (Oscar64's NES startup does SEI; cc65's BSS overlays its startup
// code, which a handler writing C variables would patch). A stub at
// handler + $40 (C64: $C0A0) -- the registers saved, the timer set up, CLI and a wait
// until the handler counted once, the registers back, CLI, JMP start -- runs
// back to the start breakpoint with only the I flag changed and the next
// interrupt due within the trace's first cycles (Atari800 runs POKEY timers
// once per 114-cycle scanline; a trace runs about as long). The
// handler saves A/X/Y, counts itself, calls the "call" function and
// acknowledges.
//  - C64: CIA1 timer A every timerCycles into LaunchC64's handler at $C040,
//    entered through the KERNAL's $0314 vector.
//  - Atari: POKEY timer 1 at 1.79 MHz (period AUDF1 + 4 cycles), handler at
//    $0600. The OS (AltirraOS without user ROMs) is banked out through PORTB
//    (XL/XE), so the CPU vectors at $FFFA-$FFFF are RAM the test owns: IRQ to
//    the handler, NMI to an RTI at $063F, no OS dispatch in between. The stub
//    sets SKCTL 3 itself, so it does not depend on the OS having taken POKEY
//    out of its init state (SKCTL 0 stops the timers).
//  - NES: the DMC, one byte sample at the fastest rate, refilled from the
//    handler at $0400 (the cartridge copy's IRQ vector); frame IRQ off. The
//    period is 8 x 54 = 432 cycles whatever timerCycles says.
bool CTestSourceStepTrace::InstallInterrupt(Machine &machine, const json &trace, const CDebugDatabase *database,
											u16 startAddress, const std::string &name)
{
	CDebugInterface *di = machine.di;
	bool atari = machine.name == "atari";
	bool c64 = machine.name == "c64";
	const json &irq = trace["irq"];
	u16 handlerAddress = c64 ? 0xC040 : atari ? 0x0600 : 0x0400;
	u16 counter = InterruptCounterAddress(machine.name);
	u8 counterLo = (u8)(counter & 0xFF), counterHi = (u8)(counter >> 8);
	std::vector<u8> stub = { 0x08, 0x48, 0x8A, 0x48 };						// PHP PHA TXA PHA

	if (c64)
	{
		// the handler is LaunchC64's; CIA1 timer A every timerCycles
		int cycles = irq.value("timerCycles", 150);
		stub.insert(stub.end(), { 0xA9, (u8)(cycles & 0xFF), 0x8D, 0x04, 0xDC,	// timer A latch
								  0xA9, (u8)(cycles >> 8), 0x8D, 0x05, 0xDC,
								  0xA9, 0x81, 0x8D, 0x0D, 0xDC,				// timer A IRQ on
								  0xA9, 0x11, 0x8D, 0x0E, 0xDC });			// load and start
	}
	else
	{
		std::vector<u8> handler = { 0x48, 0x8A, 0x48, 0x98, 0x48,				// PHA TXA PHA TYA PHA
									0xEE, counterLo, counterHi, 0xD0, 0x03,	// INC counter, BNE
									0xEE, (u8)(counterLo + 1), counterHi };
		if (irq.contains("call"))
		{
			std::string callee = irq["call"].get<std::string>();
			const CDebugFunction *function = FindFunctionByName(database, callee);
			if (!Check(function != NULL, name + ": the interrupt handler's callee " + callee + " is a function of the debug info"))
				return false;
			handler.insert(handler.end(), { 0x20, (u8)(function->start & 0xFF), (u8)(function->start >> 8) });
		}
		if (atari)
			handler.insert(handler.end(), { 0xA9, 0x00, 0x8D, 0x0E, 0xD2,			// IRQEN off and on: acknowledged
											0xA9, 0x01, 0x8D, 0x0E, 0xD2 });
		else
			handler.insert(handler.end(), { 0xA9, 0x8F, 0x8D, 0x10, 0x40,			// DMC IRQ on, rate 15
											0xA9, 0x10, 0x8D, 0x15, 0x40 });		// restart, acknowledged
		handler.insert(handler.end(), { 0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40 });		// PLA TAY PLA TAX PLA RTI
		Poke(di, handlerAddress, handler);

		if (atari)
		{
			int machineType, ramSizeKb, portb, portbMask;
			atrd_get_banking_state(&machineType, &ramSizeKb, &portb, &portbMask, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!Check(machineType == 1, name + ": the Atari is an XL/XE, whose PORTB banks the OS out for the interrupt vectors"))
				return false;
			// DDRB all outputs (PBCTL bit 2 clear selects it), then PORTB bit 0 clear: RAM under the OS
			machine.atariPortb = (u8)portb;
			machine.atariPbctl = ((CDebugInterfaceAtari *)di)->GetPiaRegister(0x03);
			Poke(di, 0xD303, { (u8)(machine.atariPbctl & ~0x04) });
			Poke(di, 0xD301, { 0xFF });
			Poke(di, 0xD303, { (u8)(machine.atariPbctl | 0x04) });
			Poke(di, 0xD301, { (u8)(portb & ~0x01) });
			for (int i = 0; i < 6; i++)
				machine.atariVectors[i] = Peek(di, (u16)(0xFFFA + i));
			machine.atariSkctl = ((CDebugInterfaceAtari *)di)->GetPokeyRegister(0x0F);
			machine.atariInterrupt = true;
			u16 rti = (u16)(handlerAddress + 0x3F);
			Poke(di, rti, { 0x40 });
			Poke(di, 0xFFFA, { (u8)(rti & 0xFF), (u8)(rti >> 8) });
			Poke(di, 0xFFFE, { (u8)(handlerAddress & 0xFF), (u8)(handlerAddress >> 8) });

			int cycles = irq.value("timerCycles", 150);
			u8 audf = (u8)(cycles < 5 ? 1 : cycles > 259 ? 255 : cycles - 4);
			stub.insert(stub.end(), { 0xA9, 0x03, 0x8D, 0x0F, 0xD2,				// SKCTL out of init: the timers count
									  0xA9, 0x40, 0x8D, 0x08, 0xD2,				// AUDCTL: channel 1 at 1.79 MHz
									  0xA9, audf, 0x8D, 0x00, 0xD2,				// AUDF1
									  0xA9, 0xA0, 0x8D, 0x01, 0xD2,				// AUDC1 silent
									  0xA9, 0x01, 0x8D, 0x0E, 0xD2,				// IRQEN: timer 1
									  0x8D, 0x09, 0xD2 });						// STIMER
		}
		else
		{
			stub.insert(stub.end(), { 0xA9, 0x40, 0x8D, 0x17, 0x40,				// frame IRQ off
									  0xA9, 0x8F, 0x8D, 0x10, 0x40,				// DMC IRQ on, rate 15
									  0xA9, 0x00, 0x8D, 0x12, 0x40,				// sample at $C000
									  0x8D, 0x13, 0x40,							// one byte
									  0xA9, 0x10, 0x8D, 0x15, 0x40 });			// start
		}
	}
	// interrupts on until the handler counted (C64, Atari: once; NES: twice, the DMC
	// is periodic from the first refill on, then 64 x 5 cycles of its 432), so
	// the next one is due early in the trace; the counter then starts at zero
	u8 syncCount = machine.name == "nes" ? 2 : 1;
	stub.insert(stub.end(), { 0x58,											// CLI
							  0xAD, counterLo, counterHi, 0xC9, syncCount, 0x90, 0xF9 });	// LDA counter, CMP, BCC wait
	if (machine.name == "nes")
		stub.insert(stub.end(), { 0xA2, 0x40, 0xCA, 0xD0, 0xFD });			// LDX #64, DEX, BNE
	stub.insert(stub.end(), { 0x78,											// SEI
							  0xA9, 0x00, 0x8D, counterLo, counterHi, 0x8D, (u8)(counterLo + 1), counterHi,
							  0x68, 0xAA, 0x68, 0x28, 0x58,					// PLA TAX PLA PLP CLI
							  0x4C, (u8)(startAddress & 0xFF), (u8)(startAddress >> 8) });	// JMP start

	u16 stubAddress = c64 ? 0xC0A0 : (u16)(handlerAddress + 0x40);	// C64: past the $C060 preamble
	Poke(di, stubAddress, stub);
	Poke(di, counter, { 0, 0 });
	if (c64)
		((CDebugInterfaceVice *)di)->MakeJmpC64(stubAddress);
	else
		di->MakeJmpNoReset(di->GetDataAdapter(), stubAddress);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	bool paused = WaitForPause(di, 5000);
	u16 pc;
	u8 a, x, y, p, sp;
	di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
	return Check(paused && pc == startAddress, name + ": the interrupt is installed and the program is back at the start line");
}

// timer IRQ off, the vectors and PORTB back; the machine is paused
void CTestSourceStepTrace::RemoveAtariInterrupt(Machine &machine)
{
	if (!machine.atariInterrupt)
		return;
	CDebugInterface *di = machine.di;
	Poke(di, 0xD20E, { 0x00 });
	Poke(di, 0xD20F, { machine.atariSkctl });
	for (int i = 0; i < 6; i++)
		Poke(di, (u16)(0xFFFA + i), { machine.atariVectors[i] });
	Poke(di, 0xD301, { machine.atariPortb });
	Poke(di, 0xD303, { machine.atariPbctl });
	machine.atariInterrupt = false;
}

bool CTestSourceStepTrace::RunTrace(const std::string &path)
{
	json trace;
	if (!Check(ReadJson(path, trace) && trace.contains("steps") && trace["steps"].is_array(),
			   "Trace " + path + " reads as JSON with a steps array"))
		return false;

	std::string compiler = trace.value("compiler", "?");
	std::string fixture = trace.value("fixture", "?");
	std::string machineName = trace.value("machine", "");
	// the build's directory names the cell: nes-uxrom, nes-mmc1 run on "nes"
	std::string artifacts = trace.value("artifacts", "");
	size_t cellEnd = artifacts.find_last_of('/');
	size_t cellStart = cellEnd == std::string::npos ? std::string::npos : artifacts.find_last_of('/', cellEnd - 1);
	std::string cell = cellStart == std::string::npos ? machineName : artifacts.substr(cellStart + 1, cellEnd - cellStart - 1);
	std::string name = compiler + "/" + cell + "/" + fixture;
	Machine *machine = GetMachine(machineName);
	if (machine == NULL)
	{
		if (!failure.empty())
			return false;
		if (machineName == "c64" || machineName == "atari" || machineName == "nes")
		{
			ReportRequiredGap((machineName + " emulation not compiled in").c_str());
			return true;
		}
		return Check(false, name + ": unknown machine " + machineName);
	}
	CDebugInterface *di = machine->di;

	std::string dir = ResolveProjectPath(artifacts.c_str());
	std::string binaryPath = dir + "/" + trace.value("binary", "");
	std::vector<u8> binary;
	if (!Check(ReadFile(binaryPath, binary) && !binary.empty(), name + ": the binary reads"))
		return false;

	// fresh scratch symbols per trace; the live ones come back in RestoreMachines
	CDebugSymbols *scratch;
	if (machineName == "c64")
	{
		scratch = new CDebugSymbolsC64(di);
		scratch->SetDataAdapter(((CDebugInterfaceVice *)di)->dataAdapterC64);
	}
	else
	{
		scratch = new CDebugSymbols(di, true);
		scratch->SetDataAdapter(machine->liveSymbols->dataAdapter);
	}
	scratch->CreateDefaultSegment();
	guiMain->LockMutex();
	di->LockMutex();
	di->symbols = scratch;
	di->UnlockMutex();
	guiMain->UnlockMutex();

	// the fixture's sources: the artifacts hold only what the build wrote
	std::string debugInfo = dir + "/" + trace.value("debugInfo", "");
	CDebugInfoLoadOptions loadOptions;
	loadOptions.sourceRoots.push_back(ResolveProjectPath(("tests/source-debug/fixtures/" + trace.value("fixture", "")).c_str()));
	DebugInfoLoadFile(scratch, debugInfo.c_str(), loadOptions);
	std::shared_ptr<const CDebugDatabase> database = scratch->debugDatabase;
	if (!Check(database && scratch->currentSegment && scratch->currentSegment->debugRegionId >= 0,
			   name + ": " + trace.value("debugInfo", "") + " loads with a region for the current segment"))
		return false;
	u32 regionId = (u32)scratch->currentSegment->debugRegionId;

	// run to the start line through a source breakpoint
	std::string startFile = trace["start"].value("file", "");
	int startLine = trace["start"].value("line", 0);
	CDebugSourceBreakpoint *startBp = scratch->AddSourceBreakpoint(startFile.c_str(), startLine);
	if (!Check(startBp->IsResolved() && startBp->resolvedLine == startLine,
			   name + ": the start " + startFile + ":" + std::to_string(startLine) + " resolves to code"))
		return false;
	int startAddress = startBp->bindings[0].address;

	bool launched = false;
	bool cartridge = binaryPath.size() > 4 && binaryPath.compare(binaryPath.size() - 4, 4, ".crt") == 0;
	if (machineName == "c64" && cartridge)
		launched = LaunchC64Cartridge(*machine, binaryPath, (u16)startAddress, name);
	else if (machineName == "c64")
	{
		// a cartridge a previous trace attached: off, back to the READY prompt
		if (machine->c64Cartridge && !DetachC64Cartridge(*machine))
			return Check(false, name + ": the previous trace's cartridge detaches and the C64 boots to READY");
		launched = LaunchC64(di, trace, binary, database.get(), name);
	}
	else if (machineName == "atari")
	{
		RemoveAtariInterrupt(*machine);		// its handler would call into code about to be overwritten
		launched = LaunchAtari(di, trace, binary, name);
	}
#ifdef RUN_NES
	else if (machineName == "nes")
		launched = LaunchNes(di, trace, binaryPath, name);
#endif
	if (!launched)
		return false;
	bool paused = true;		// a cartridge launch waits for the start line itself
	if (!cartridge)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		paused = WaitForPause(di, 5000);
	}
	u16 pc;
	u8 a, x, y, p, sp;
	di->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
	char hex[8], got[48];
	snprintf(hex, sizeof(hex), "$%04X", startAddress);
	snprintf(got, sizeof(got), "PC $%04X, mode %d", pc, (int)di->GetDebugMode());
	if (!Check(paused && pc == startAddress, name + ": the program stops at the start line (" + hex + "; " + got + ")"))
		return false;
	if (trace.contains("irq") && !InstallInterrupt(*machine, trace, database.get(), (u16)startAddress, name))
		return false;
	scratch->RemoveSourceBreakpoint(startBp);

	// replay
	std::ostringstream log;
	log << "trace " << path << "\nstart " << startFile << ":" << startLine << " at " << hex << "\n";
	int depth = 0;
	bool ok = true;
	std::string mismatch;
	int index = 0;
	for (const json &step : trace["steps"])
	{
		index++;
		SourceStepKind kind = ParseAction(step.value("action", ""));
		u64 budget = step.value("budget", (u64)0);
		bool armed = di->SourceStep(kind, budget);
		bool stopped = !armed || WaitForPause(di, 5000);
		CSourceStepResult r = di->sourceStepper->GetLastResult();
		if (armed)
			depth += r.depth;

		const CDebugSourceFile *file = r.mapped ? database->GetFile(r.fileId) : NULL;
		const CDebugFunction *function = di->FindFunctionAt(database.get(), regionId, r.pc);
		std::string actualFile = file ? file->fileName : "";
		std::string actualFunction = function ? function->name : "";
		std::string actualReason = SourceStepStopReasonName(r.reason);

		const json &expected = step["expected"];
		bool match = stopped && kind != SourceStepKind::None
			&& NormalizeReason(step.value("expectedReason", "")) == NormalizeReason(actualReason);
		if (expected.contains("line"))
		{
			if (expected["line"].is_null())
				match = match && !r.mapped;
			else
				match = match && r.mapped && r.line == expected["line"].get<int>();
		}
		if (expected.contains("file"))
		{
			std::string expectedFile = expected["file"].get<std::string>();
			size_t slash = expectedFile.find_last_of("/\\");
			match = match && actualFile == (slash == std::string::npos ? expectedFile : expectedFile.substr(slash + 1));
		}
		if (expected.contains("function"))
			match = match && actualFunction == expected["function"].get<std::string>();
		if (expected.contains("depth"))
			match = match && depth == expected["depth"].get<int>();
		if (expected.contains("pc"))
			match = match && r.pc == ParseAddress(expected["pc"]);

		char line[320];
		snprintf(line, sizeof(line), "%2d %-10s -> $%04X %s:%d %s depth %d %s (%llu instructions)%s\n   expected %s %s\n",
				 index, step.value("action", "").c_str(), r.pc, actualFile.c_str(), r.mapped ? r.line : 0,
				 actualFunction.c_str(), depth, actualReason.c_str(), (unsigned long long)r.instructions,
				 match ? "" : "  MISMATCH", expected.dump().c_str(), step.value("expectedReason", "").c_str());
		log << line;
		if (!match && ok)
		{
			ok = false;
			mismatch = line;
		}
		if (!stopped)
			break;
	}

	std::string message = name + ": " + std::to_string(trace["steps"].size()) + " source steps match the trace";
	if (ok && trace.contains("irq"))
	{
		// the handler's counter: the interrupt really ran during the trace
		u16 counter = InterruptCounterAddress(machineName);
		int irqs = Peek(di, counter) | (Peek(di, (u16)(counter + 1)) << 8);
		message += ", with the IRQ taken " + std::to_string(irqs) + " times";
		if (irqs == 0)
		{
			ok = false;
			mismatch = "the IRQ handler never ran";
			log << mismatch << "\n";
		}
	}
	if (!ok)
	{
		std::string dump = "/tmp/source-step-trace-" + compiler + "-" + machineName + "-" + fixture + ".txt";
		std::ofstream(dump) << log.str();
		message = name + ": " + mismatch + " (full trace in " + dump + ")";
	}
	return Check(ok, message);
}

void CTestSourceStepTrace::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	std::string expectations = ResolveProjectPath("tests/source-debug/expectations");
	json index;
	bool ok = Check(ReadJson(expectations + "/index.json", index) && index.contains("traces") && !index["traces"].empty(),
					"tests/source-debug/expectations/index.json lists the traces");
	int traces = 0;
	if (ok)
	{
		for (const json &entry : index["traces"])
		{
			ok = RunTrace(expectations + "/" + entry.get<std::string>());
			for (Machine &machine : machines)
				machine.di->SetDebugMode(DEBUGGER_MODE_PAUSED);
			if (!ok)
				break;
			traces++;
		}
	}
	RestoreMachines();

	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	char summary[128];
	snprintf(summary, sizeof(summary), "%d source step traces replay as recorded", traces);
	TestCompleted(true, summary);
}
