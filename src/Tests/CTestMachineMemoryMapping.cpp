#include "CTestMachineMemoryMapping.h"
#include "CTestNesCartridge.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceNes.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugMemoryMapping.h"
#include "NesWrapper.h"
#include "AtariWrapper.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include "C64SettingsStorage.h"
#include <cstdio>
#include <cstring>
#include <vector>

bool CTestMachineMemoryMapping::Check(bool condition, const char *message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message);
	return true;
}

static bool Is(const CDebugMappedAddress &m, DebugMemoryKind kind, const char *device)
{
	return m.physical.kind == kind && m.physical.device == device;
}

// UxROM (mapper 2): four 16 KB PRG banks, the last fixed at $C000, the bank at
// $8000 selected by any write to $8000-$FFFF. Every byte $FF (bus conflicts
// then change nothing); reset jumps to a JMP-to-self in the fixed bank.
static bool WriteUxRomImage(const char *path)
{
	const int banks = 4;
	std::vector<u8> rom(16 + banks * 0x4000, 0xFF);
	memset(&rom[0], 0, 16);
	rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
	rom[4] = banks;			// PRG-ROM in 16 KB units
	rom[5] = 0;				// CHR-RAM
	rom[6] = 0x20;			// mapper 2 (low nibble in bits 4-7)
	u8 *last = &rom[16 + (banks - 1) * 0x4000];
	last[0x0000] = 0x4C; last[0x0001] = 0x00; last[0x0002] = 0xC0;	// $C000: JMP $C000
	last[0x3FFC] = 0x00; last[0x3FFD] = 0xC0;							// RESET -> $C000
	last[0x3FFA] = 0x00; last[0x3FFB] = 0xC0;
	last[0x3FFE] = 0x00; last[0x3FFF] = 0xC0;
	FILE *f = fopen(path, "wb");
	if (f == NULL)
		return false;
	bool ok = fwrite(&rom[0], 1, rom.size(), f) == rom.size();
	fclose(f);
	return ok;
}

bool CTestMachineMemoryMapping::RunNes()
{
	CDebugInterfaceNes *nes = (CDebugInterfaceNes *)viewC64->debugInterfaceNes;
	CDebugMemoryMappingResolver *resolver = nes ? nes->GetMemoryMappingResolver() : NULL;
	if (!Check(resolver != NULL, "NES: the interface has a mapping resolver"))
		return false;

	const char *romPath = "/tmp/retrodebugger_test_uxrom.nes";
	if (!Check(WriteUxRomImage(romPath), "NES: a four-bank UxROM image is written"))
		return false;

	bool wasRunning = nes->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(nes);
		SYS_Sleep(2000);
	}
	bool ok = true;
	int mapper = -1;
	unsigned int prgSize = 0;
	ok = TestNesInsertCartridge(nes, romPath, true) && nesd_get_cartridge_info(&mapper, NULL, &prgSize)
		&& Check(mapper == 2 && prgSize == 4 * 0x4000, "NES: the UxROM image loads as mapper 2 with 64 KB PRG");

	if (ok)
	{
		nes->SetByte(0x8000, 0x01);
		CDebugMappedAddress bank1 = resolver->ResolveFetch(0x8000);
		CDebugMappedAddress fixed = resolver->ResolveFetch(0xC000);
		u64 epochBank1 = resolver->GetMappingEpoch();
		nes->SetByte(0x8000, 0x02);
		CDebugMappedAddress bank2 = resolver->ResolveFetch(0x8000);
		CDebugMappedAddress fixedAgain = resolver->ResolveFetch(0xC000);

		ok = Check(Is(bank1, DebugMemoryKind::CartridgeRom, "nes-prg") && bank1.physical.offset == 0x4000
				   && Is(bank2, DebugMemoryKind::CartridgeRom, "nes-prg") && bank2.physical.offset == 0x8000
				   && bank1.physical != bank2.physical && resolver->GetMappingEpoch() > epochBank1,
				   "NES UxROM: $8000 is PRG offset $4000 in bank 1 and $8000 in bank 2 -- one PC, two identities")
			&& Check(fixed.physical == fixedAgain.physical && fixed.physical.offset == 0xC000,
					 "NES UxROM: $C000 stays on the fixed last bank (PRG offset $C000) across the switch")
			&& Check(Is(resolver->ResolveWrite(0x8000), DebugMemoryKind::Io, "nes-mapper"),
					 "NES: a write to PRG-ROM space reaches the mapper's register, not storage");

		if (ok)
		{
			std::vector<u16> windows = resolver->ResolveCpuWindows(bank2.physical);
			ok = Check(windows.size() == 1 && windows[0] == 0x8000,
					   "NES UxROM: bank 2's first byte is visible only at $8000 while bank 2 is selected");
		}
		if (ok)
		{
			CDebugMappedAddress ram = resolver->ResolveRead(0x0012);
			CDebugMappedAddress mirror = resolver->ResolveRead(0x1812);
			ok = Check(Is(ram, DebugMemoryKind::Ram, "nes-ram") && ram.physical == mirror.physical
					   && Is(resolver->ResolveRead(0x2008), DebugMemoryKind::Io, "nes-ppu")
					   && resolver->ResolveRead(0x2008).physical.offset == 0,
					   "NES: $1812 is an alias of RAM $0012; $2008 is PPU register 0")
				&& Check(resolver->ResolveRead(0x5000).physical.kind == DebugMemoryKind::Unknown,
						 "NES: the expansion area $4020-$5FFF is Unknown, not guessed");
		}
		TestNesUnloadCartridge(nes, true);
		if (ok)
		{
			ok = Check(resolver->ResolveFetch(0x8000).physical.kind == DebugMemoryKind::Unknown,
					   "NES: with no cartridge, $8000 is Unknown");
		}
	}
	nes->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(nes);
	remove(romPath);
	return ok;
}

bool CTestMachineMemoryMapping::RunAtari()
{
	CDebugInterfaceAtari *atari = viewC64->debugInterfaceAtari;
	CDebugMemoryMappingResolver *resolver = atari ? atari->GetMemoryMappingResolver() : NULL;
	if (!Check(resolver != NULL, "Atari: the interface has a mapping resolver"))
		return false;

	bool wasRunning = atari->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(atari);
		SYS_Sleep(2000);
	}
	atari->PauseEmulationBlockedWait();

	int machineType = -1, ramSizeKb = 0, portb = 0, portbMask = 0, cartA0bf = 0;
	atrd_get_banking_state(&machineType, &ramSizeKb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	// The CPU/ANTIC extended-bank case needs a 130XE: an XL/XE with another
	// RAM size is cold-started as 128 KB here and put back at the end.
	bool ramSizeChanged = false;
	if (machineType == 1 && ramSizeKb != 128)
	{
		atari->SetRamSizeOption(4);		// XL/XE option 4 = 128 KB
		ramSizeChanged = true;
	}
	atrd_get_banking_state(&machineType, &ramSizeKb, &portb, &portbMask, NULL, NULL, NULL, NULL, &cartA0bf, NULL, NULL, NULL);
	u8 savedPortb = (u8)portb;

	bool ok = Check(Is(resolver->ResolveRead(0x0080), DebugMemoryKind::Ram, "atari-ram")
					&& Is(resolver->ResolveRead(0xD000), DebugMemoryKind::Io, "atari-io")
					&& Is(resolver->ResolveWrite(0xD40A), DebugMemoryKind::Io, "atari-io"),
					"Atari: page zero is RAM, GTIA and ANTIC are I/O");

	if (ok && machineType == 1)		// XL/XE
	{
		// PORTB drives the banking only through pins DDRB makes outputs
		// (input pins read as 1), and a $D301 write reaches DDRB with PBCTL
		// bit 2 clear, the output register with it set. Bit 0 set = OS ROM in,
		// bit 1 clear = BASIC in.
		u8 savedPbctl = atari->GetByte(0xD303);
		atari->SetByte(0xD303, (u8)(savedPbctl & ~0x04));
		atari->SetByte(0xD301, 0xFF);						// DDRB: all outputs
		atari->SetByte(0xD303, (u8)(savedPbctl | 0x04));

		atari->SetByte(0xD301, (u8)(savedPortb | 0x01));	// OS ROM in
		CDebugMappedAddress os = resolver->ResolveFetch(0xE000);
		u64 epochOs = resolver->GetMappingEpoch();
		ok = Check(Is(os, DebugMemoryKind::Rom, "atari-os") && os.physical.offset == 0x2000
				   && resolver->ResolveWrite(0xE000).physical.kind == DebugMemoryKind::Unmapped,
				   "Atari XL/XE: with the OS in, $E000 is OS ROM offset $2000 and a write there is dropped");
		if (ok)
		{
			atari->SetByte(0xD301, (u8)(savedPortb & ~0x01));	// OS ROM out
			CDebugMappedAddress ram = resolver->ResolveFetch(0xE000);
			ok = Check(Is(ram, DebugMemoryKind::Ram, "atari-ram") && ram.physical.offset == 0xE000
					   && resolver->GetMappingEpoch() > epochOs,
					   "Atari XL/XE: with the OS out, the same $E000 is RAM: a different identity");
		}
		if (ok)
		{
			// RD-029: the direct-RAM adapter reaches the RAM under the OS ROM
			CDebugDataAdapter *cpuView = atari->GetDataAdapter();
			CDebugDataAdapter *directRam = atari->GetDataAdapterDirectRam();
			atari->SetByte(0xD301, (u8)(savedPortb | 0x01));	// OS ROM in
			u8 rom = 0, savedRam = 0, seen = 0, underRom = 0;
			cpuView->AdapterReadByte(0xE000, &rom);
			directRam->AdapterReadByte(0xE000, &savedRam);
			u8 pattern = (u8)(rom ^ 0xA5);
			directRam->AdapterWriteByte(0xE000, pattern);
			cpuView->AdapterReadByte(0xE000, &seen);
			directRam->AdapterReadByte(0xE000, &underRom);
			bool romKept = seen == rom && underRom == pattern;
			atari->SetByte(0xD301, (u8)(savedPortb & ~0x01));	// OS ROM out
			cpuView->AdapterReadByte(0xE000, &seen);
			directRam->AdapterWriteByte(0xE000, savedRam);
			ok = Check(directRam != cpuView && romKept && seen == pattern,
					   "Atari XL/XE: the direct-RAM adapter writes the RAM under the OS ROM, which the CPU sees once the OS is out");
		}
		if (ok && !cartA0bf)
		{
			atari->SetByte(0xD301, (u8)((savedPortb & ~0x02) | 0x01));	// BASIC in
			CDebugMappedAddress basic = resolver->ResolveFetch(0xA000);
			atari->SetByte(0xD301, (u8)((savedPortb | 0x02) | 0x01));		// BASIC out
			CDebugMappedAddress noBasic = resolver->ResolveFetch(0xA000);
			ok = Check(Is(basic, DebugMemoryKind::Rom, "atari-basic") && Is(noBasic, DebugMemoryKind::Ram, "atari-ram"),
					   "Atari XL/XE: PORTB bit 1 switches BASIC and RAM at $A000");
		}
		if (ok && ramSizeKb == 128)
		{
			// 130XE: bit 4 clear = CPU sees extended bank (bits 2-3), bit 5 set = ANTIC sees base RAM
			atari->SetByte(0xD301, (u8)((savedPortb & ~0x1C) | 0x08 | 0x20));
			CDebugMappedAddress cpu = resolver->ResolveFetch(0x4000);
			CDebugMappedAddress antic = resolver->Resolve(0x4000, DebugMemoryAccess::Read, DebugMemoryBusMaster::Antic);
			ok = Check(Is(cpu, DebugMemoryKind::ExtendedRam, "atari-xe-ram") && cpu.physical.region == 3
					   && Is(antic, DebugMemoryKind::Ram, "atari-ram") && cpu.physical != antic.physical,
					   "Atari 130XE: at $4000 the CPU sees extended bank 3 while ANTIC sees base RAM");
			if (ok)
			{
				// RD-029: with bank 3 in, the direct-RAM adapter still reads the base RAM ANTIC sees
				u8 base = 0;
				atari->GetDataAdapterDirectRam()->AdapterReadByte(0x4000, &base);
				atari->SetByte(0xD301, (u8)((savedPortb & ~0x1C) | 0x08 | 0x10 | 0x20));	// bit 4 set: base RAM
				u8 baseSeen = 0;
				atari->GetDataAdapter()->AdapterReadByte(0x4000, &baseSeen);
				CDebugMappedAddress cpuBase = resolver->ResolveFetch(0x4000);
				ok = Check(base == baseSeen && Is(cpuBase, DebugMemoryKind::Ram, "atari-ram"),
						   "Atari 130XE: the direct-RAM adapter reads base RAM under a bank, and PORTB bit 4 set gives the CPU base RAM");
			}
		}
		else if (ok)
		{
			ReportRequiredGap("Atari machine is not a 128 KB 130XE, so the CPU/ANTIC extended-bank case was never exercised");
		}
		atari->SetByte(0xD301, savedPortb);
		atari->SetByte(0xD303, (u8)(savedPbctl & ~0x04));
		atari->SetByte(0xD301, (u8)~portbMask);			// DDRB as it was
		atari->SetByte(0xD303, savedPbctl);
	}
	else if (ok)
	{
		ReportRequiredGap("Atari machine is not XL/XE, so PORTB banking was never exercised");
	}

	if (ramSizeChanged)
		atari->SetRamSizeOption(c64SettingsAtariRamSizeOption);
	atari->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(atari);
	return ok;
}

void CTestMachineMemoryMapping::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	stepNum = 1;
	failure.clear();

	bool ok = true;
#ifdef RUN_NES
	ok = RunNes();
#else
	ReportRequiredGap("NES emulation not compiled in");
#endif
#ifdef RUN_ATARI
	if (ok)
		ok = RunAtari();
#else
	ReportRequiredGap("Atari emulation not compiled in");
#endif
	if (!ok)
	{
		TestCompleted(false, failure.c_str());
		return;
	}
	TestCompleted(true, "NES and Atari mapping providers follow their emulators' banking");
}
