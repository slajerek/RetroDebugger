#include "CTestC64MemoryMapping.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugMemoryMapping.h"
#include "C64BankingModes.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include "CSlrString.h"
#include "GUI_Main.h"
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

extern CSlrString *c64SettingsPathToCartridge;

extern "C"
{
	uint8_t c64d_peek_c64(uint16_t addr);
}

#define MM_CHECK(cond, msg) \
	do { \
		if (!(cond)) { failed = msg; goto done; } \
		StepCompleted(stepNum++, true, msg); \
	} while (0)

static bool Is(const CDebugMappedAddress &m, DebugMemoryKind kind, const char *device)
{
	return m.physical.kind == kind && (device == NULL || m.physical.device == device);
}

// A .crt: the 64-byte header, then one CHIP packet per 8 KB bank (each 16
// bytes of packet header and the data). Bank N's data carries 0xB0+N at $100.
struct CrtChip { u16 type, bank, start; std::vector<u8> data; };

static u32 CrtChipDataOffset(int chipIndex)
{
	return 0x40 + chipIndex * (0x10 + 0x2000) + 0x10;
}

static bool WriteCrt(const std::string &path, u16 hardwareType, u8 exrom, u8 game, const std::vector<CrtChip> &chips)
{
	std::vector<u8> out(0x40, 0);
	memcpy(&out[0], "C64 CARTRIDGE   ", 16);
	out[0x13] = 0x40;
	out[0x14] = 0x01;
	out[0x16] = (u8)(hardwareType >> 8);	out[0x17] = (u8)hardwareType;
	out[0x18] = exrom;						out[0x19] = game;
	memcpy(&out[0x20], "RD MAPPING TEST", 15);
	for (const CrtChip &chip : chips)
	{
		const u8 header[16] = { 'C', 'H', 'I', 'P', 0x00, 0x00, 0x20, 0x10, (u8)(chip.type >> 8), (u8)chip.type,
								(u8)(chip.bank >> 8), (u8)chip.bank, (u8)(chip.start >> 8), (u8)chip.start, 0x20, 0x00 };
		out.insert(out.end(), header, header + 16);
		out.insert(out.end(), chip.data.begin(), chip.data.end());
	}
	FILE *f = fopen(path.c_str(), "wb");
	if (f == NULL)
		return false;
	bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
	fclose(f);
	return ok;
}

static std::vector<u8> CrtBank(int bank)
{
	std::vector<u8> data(0x2000, 0xFF);
	data[0x100] = (u8)(0xB0 + bank);
	return data;
}

// at the KERNAL's READY loop with its vectors set (after a cartridge detach reset)
static bool WaitForReady(CDebugInterfaceC64 *c64)
{
	for (int waited = 0; waited < 10000; waited += 20)
	{
		u16 pc = (u16)c64->GetCpuPC();
		if (c64->GetByteFromRamC64(0x0326) == 0xCA && c64->GetByteFromRamC64(0x0327) == 0xF1 && pc >= 0xE5CD && pc <= 0xE5D5)
			return true;
		SYS_Sleep(20);
	}
	return false;
}

static bool AttachAndPauseAt(CDebugInterfaceC64 *c64, const std::string &path, u16 loopPc)
{
	c64->SetDebugMode(DEBUGGER_MODE_RUNNING);
	CSlrString *p = new CSlrString(path.c_str());
	c64->AttachCartridge(p);
	delete p;
	for (int waited = 0; waited < 5000; waited += 20)
	{
		SYS_Sleep(20);
		if ((u16)c64->GetCpuPC() == loopPc)
		{
			c64->PauseEmulationBlockedWait();
			if ((u16)c64->GetCpuPC() == loopPc)
				return true;
			c64->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
	}
	return false;
}

static bool DetachAndBoot(CDebugInterfaceC64 *c64)
{
	c64->SetDebugMode(DEBUGGER_MODE_RUNNING);
	c64->DetachCartridge();
	bool detached = false;
	for (int waited = 0; waited < 5000 && !detached; waited += 10)
	{
		C64StateCartridge state;
		state.exrom = state.game = 0;
		c64->GetC64CartridgeState(&state);
		detached = state.exrom == 1 && state.game == 1;
		if (!detached)
			SYS_Sleep(10);
	}
	bool ready = detached && WaitForReady(c64);
	c64->PauseEmulationBlockedWait();
	return ready;
}

static bool IsCart(const CDebugMappedAddress &m, int region, u32 offset, u32 imageOffset)
{
	return m.physical.kind == DebugMemoryKind::CartridgeRom && m.physical.device == "c64-cart" && m.physical.region == region
		&& m.physical.offset == offset && m.hasImageOffset && m.imageOffset == imageOffset;
}

bool CTestC64MemoryMapping::RunBankedCartridges(int &stepNum, std::string &failed)
{
	CDebugInterfaceC64 *c64 = viewC64->debugInterfaceC64;
	CDebugMemoryMappingResolver *resolver = c64->GetMemoryMappingResolver();
	guiMain->LockMutex();
	CSlrString *savedCartridgePath = c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = NULL;
	guiMain->UnlockMutex();
	bool attached = false;
	auto check = [&](bool condition, const char *msg) {
		if (!condition && failed.empty())
			failed = msg;
		else if (condition)
			StepCompleted(stepNum++, true, msg);
		return condition;
	};

	// Ocean (type 5), 4 x 8 KB, 16 KB mode: $8000 and $A000 both read the ROML
	// bank. Bank 0 is a CBM80 cartridge whose cold start loops at $8009.
	std::string oceanPath = "/tmp/rd-mapping-ocean.crt";
	std::vector<CrtChip> ocean;
	for (int bank = 0; bank < 4; bank++)
		ocean.push_back({ 0, (u16)bank, 0x8000, CrtBank(bank) });
	const u8 boot[] = { 0x09, 0x80, 0x09, 0x80, 0xC3, 0xC2, 0xCD, 0x38, 0x30, 0x4C, 0x09, 0x80 };
	memcpy(ocean[0].data.data(), boot, sizeof(boot));
	bool ok = check(WriteCrt(oceanPath, 5, 0, 0, ocean), "Ocean: a four-bank .crt is written")
		&& check(attached = AttachAndPauseAt(c64, oceanPath, 0x8009), "Ocean: the cartridge attaches and runs to its loop at $8009");
	if (ok)
	{
		CDebugMappedAddress bank0 = resolver->ResolveFetch(0x8009);
		c64->SetByteC64(0xDE00, 2);
		CDebugMappedAddress low2 = resolver->ResolveFetch(0x8100);
		CDebugMappedAddress high2 = resolver->ResolveRead(0xA100);
		ok = check(IsCart(bank0, 0, 0x0009, CrtChipDataOffset(0) + 0x0009), "Ocean: $8009 is bank 0, at its .crt file offset")
			&& check(IsCart(low2, 0, 2 * 0x2000 + 0x100, CrtChipDataOffset(2) + 0x100) && c64->GetByteC64(0x8100) == 0xB2,
					 "Ocean: after $DE00=2, $8100 is bank 2 and reads its marker byte")
			&& check(high2.physical == low2.physical && high2.imageOffset == low2.imageOffset,
					 "Ocean: $A100 reads the same ROML bank byte as $8100");
		c64->SetByteC64(0xDE00, 0);
	}
	if (attached)
		ok = check(DetachAndBoot(c64), "Ocean: the cartridge detaches and the C64 boots to READY") && ok;
	attached = false;

	// EasyFlash (type 32): boots in Ultimax with ROMH bank 0 at $E000 (reset
	// vector -> a loop at $E000); $DE02=7 is the 16 KB mode, $DE00 the bank.
	std::string easyFlashPath = "/tmp/rd-mapping-easyflash.crt";
	std::vector<CrtChip> easyFlash = {
		{ 2, 0, 0x8000, CrtBank(0) }, { 2, 0, 0xE000, CrtBank(0) },
		{ 2, 1, 0x8000, CrtBank(1) }, { 2, 1, 0xA000, CrtBank(1) },
	};
	easyFlash[1].data[0x0000] = 0x4C;	easyFlash[1].data[0x0001] = 0x00;	easyFlash[1].data[0x0002] = 0xE0;
	easyFlash[1].data[0x1FFC] = 0x00;	easyFlash[1].data[0x1FFD] = 0xE0;
	easyFlash[3].data[0x100] = 0xC1;	// bank 1's ROMH marker
	if (ok)
		ok = check(WriteCrt(easyFlashPath, 32, 1, 0, easyFlash), "EasyFlash: a two-bank .crt is written")
			&& check(attached = AttachAndPauseAt(c64, easyFlashPath, 0xE000), "EasyFlash: the cartridge boots in Ultimax to its loop at $E000");
	if (ok)
	{
		CDebugMappedAddress romh0 = resolver->ResolveFetch(0xE000);
		c64->SetByteC64(0xDE00, 1);
		c64->SetByteC64(0xDE02, 7);
		CDebugMappedAddress roml1 = resolver->ResolveRead(0x8100);
		CDebugMappedAddress romh1 = resolver->ResolveRead(0xA100);
		ok = check(IsCart(romh0, 1, 0x0000, CrtChipDataOffset(1)), "EasyFlash: in Ultimax $E000 is ROMH bank 0, at its .crt file offset")
			&& check(IsCart(roml1, 0, 0x2000 + 0x100, CrtChipDataOffset(2) + 0x100) && c64->GetByteC64(0x8100) == 0xB1,
					 "EasyFlash: in 16 KB mode with bank 1, $8100 is ROML bank 1 and reads its marker byte")
			&& check(IsCart(romh1, 1, 0x2000 + 0x100, CrtChipDataOffset(3) + 0x100) && c64->GetByteC64(0xA100) == 0xC1,
					 "EasyFlash: $A100 is ROMH bank 1 at the interleaved .crt offset");
	}
	if (ok)
	{
		// RD-028: the shared 32-mode table (memory bank view, VIC editor) against
		// VICE's own read tables. EasyFlash's $DE02 with the mode bit gives every
		// EXROM/GAME pair: 4 = none, 5 = Ultimax, 6 = 8 KB, 7 = 16 KB.
		struct { u8 de02, exrom, game; } pairs[] = { { 6, 0, 1 }, { 7, 0, 0 }, { 5, 1, 0 }, { 4, 1, 1 } };
		const u16 regionAddress[C64BANK_NUM_REGIONS] = { 0x8100, 0xA100, 0xC100, 0xD100, 0xE100 };
		std::string mismatches;
		c64->SetByteC64(0x0000, 0x2F);
		for (auto &pair : pairs)
		{
			for (u8 port = 0; port < 8; port++)
			{
				c64->SetByteC64(0x0001, 0x37);		// I/O in to reach $DE02 (every pair at $x7)
				c64->SetByteC64(0xDE02, pair.de02);
				c64->SetByteC64(0x0001, (u8)(0x30 | port));
				int mode = C64BankingMode(pair.exrom, pair.game, port);
				for (int region = 0; region < C64BANK_NUM_REGIONS; region++)
				{
					CDebugMappedAddress m = resolver->ResolveRead(regionAddress[region]);
					C64BankEntry expected = C64BankingEntry(mode, region);
					const char *device = m.physical.device.c_str();
					C64BankEntry got = m.physical.kind == DebugMemoryKind::Unmapped ? C64BANK_OPEN
						: !strcmp(device, "c64-ram") ? C64BANK_RAM : !strcmp(device, "c64-basic") ? C64BANK_BASIC_ROM
						: !strcmp(device, "c64-kernal") ? C64BANK_KERNAL_ROM : !strcmp(device, "c64-chargen") ? C64BANK_CHAR_ROM
						: !strcmp(device, "c64-io") ? C64BANK_IO
						: (!strcmp(device, "c64-cart") && m.physical.region == 0) ? C64BANK_CART_LO
						: (!strcmp(device, "c64-cart") && m.physical.region == 1) ? C64BANK_CART_HI : (C64BankEntry)0xFF;
					if (got != expected)
					{
						char buf[160];
						snprintf(buf, sizeof(buf), " mode %d %s: table %s, VICE %s;", mode, C64BankRegionName(region),
								 C64BankEntryName(expected), got == (C64BankEntry)0xFF ? m.physical.device.c_str() : C64BankEntryName(got));
						mismatches += buf;
					}
				}
			}
		}
		c64->SetByteC64(0x0001, 0x37);
		c64->SetByteC64(0xDE02, 0);
		ok = check(mismatches.empty(), mismatches.empty() ? "the 32-mode banking table matches VICE in every EXROM/GAME/$01 mode"
				   : ("the 32-mode banking table differs from VICE:" + mismatches).c_str());
	}
	if (attached)
		ok = check(DetachAndBoot(c64), "EasyFlash: the cartridge detaches and the C64 boots to READY") && ok;

	guiMain->LockMutex();
	if (c64SettingsPathToCartridge)
		delete c64SettingsPathToCartridge;
	c64SettingsPathToCartridge = savedCartridgePath;
	guiMain->UnlockMutex();
	return ok;
}

void CTestC64MemoryMapping::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	int stepNum = 1;

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulation is not compiled in, so the C64 mapping provider was never exercised");
	return;
#else
	CDebugInterfaceC64 *c64 = viewC64->debugInterfaceC64;
	CDebugMemoryMappingResolver *resolver = c64 ? c64->GetMemoryMappingResolver() : NULL;
	if (resolver == NULL)
	{
		TestCompleted(false, "the VICE C64 has no memory mapping resolver");
		return;
	}

	bool wasRunning = c64->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(c64);
		SYS_Sleep(2000);
	}
	c64->PauseEmulationBlockedWait();
	u8 savedDdr = c64d_peek_c64(0x0000);
	u8 savedPort = c64d_peek_c64(0x0001);

	std::string failed;
	{
		// Spec #3B.2, no cartridge: $A000 / $D000 / $E000 for each low-bit pattern
		struct Row { u8 port; const char *a000; const char *d000; const char *e000; };
		const Row rows[] = {
			{ 0x37, "c64-basic", "c64-io", "c64-kernal" },
			{ 0x36, "c64-ram", "c64-io", "c64-kernal" },
			{ 0x35, "c64-ram", "c64-io", "c64-ram" },
			{ 0x34, "c64-ram", "c64-ram", "c64-ram" },		// LORAM=HIRAM=0: RAM everywhere
			{ 0x33, "c64-basic", "c64-chargen", "c64-kernal" },
			{ 0x32, "c64-ram", "c64-chargen", "c64-kernal" },
			{ 0x31, "c64-ram", "c64-chargen", "c64-ram" },
			{ 0x30, "c64-ram", "c64-ram", "c64-ram" },
		};
		c64->SetByteC64(0x0000, 0x2F);
		for (const Row &row : rows)
		{
			c64->SetByteC64(0x0001, row.port);
			CDebugMappedAddress a = resolver->ResolveFetch(0xA000);
			CDebugMappedAddress d = resolver->ResolveFetch(0xD000);
			CDebugMappedAddress e = resolver->ResolveFetch(0xE000);
			char msg[256];
			snprintf(msg, sizeof(msg), "$01=$%02X: $A000 %s, $D000 %s, $E000 %s (got %s, %s, %s)", row.port,
					 row.a000, row.d000, row.e000, a.physical.device.c_str(), d.physical.device.c_str(), e.physical.device.c_str());
			bool ok = a.physical.device == row.a000 && d.physical.device == row.d000 && e.physical.device == row.e000;
			if (!ok)
			{
				failed = msg;
				goto done;
			}
			StepCompleted(stepNum++, true, msg);
		}

		c64->SetByteC64(0x0001, 0x37);
		CDebugMappedAddress basic = resolver->ResolveFetch(0xA123);
		CDebugMappedAddress hiddenWrite = resolver->ResolveWrite(0xA123);
		u64 epochAt37 = resolver->GetMappingEpoch();
		MM_CHECK(Is(basic, DebugMemoryKind::Rom, "c64-basic") && basic.physical.offset == 0x0123
				 && Is(hiddenWrite, DebugMemoryKind::Ram, "c64-ram") && hiddenWrite.physical.offset == 0xA123,
				 "fetch at $A123 reads BASIC ROM while a write there lands in the RAM underneath");

		MM_CHECK(Is(resolver->ResolveWrite(0xD020), DebugMemoryKind::Io, "c64-io")
				 && resolver->ResolveWrite(0xD020).physical.offset == 0x0020
				 && Is(resolver->ResolveRead(0xD800), DebugMemoryKind::Ram, "c64-color-ram")
				 && Is(resolver->ResolveWrite(0xE000), DebugMemoryKind::Ram, "c64-ram"),
				 "with I/O visible, I/O writes stay I/O, color RAM is its own storage, KERNAL writes reach RAM");

		MM_CHECK(Is(resolver->ResolveRead(0x0001), DebugMemoryKind::CpuPort, "c64-cpu-port")
				 && Is(resolver->ResolveRead(0x0002), DebugMemoryKind::Ram, "c64-ram"),
				 "$00/$01 are the 6510 port, not RAM");

		CDebugPhysicalAddress kernalByte = resolver->ResolveFetch(0xE000).physical;
		std::vector<u16> windows = resolver->ResolveCpuWindows(kernalByte);
		MM_CHECK(windows.size() == 1 && windows[0] == 0xE000, "a KERNAL byte is visible at exactly one CPU address");

		c64->SetByteC64(0x0001, 0x36);
		CDebugMappedAddress ramAtA123 = resolver->ResolveFetch(0xA123);
		MM_CHECK(ramAtA123.physical != basic.physical && ramAtA123.physical == hiddenWrite.physical
				 && resolver->GetMappingEpoch() > epochAt37,
				 "the same PC after switching BASIC out is a different identity: the RAM the write reached");

		c64->SetByteC64(0x0001, 0x35);
		MM_CHECK(resolver->ResolveCpuWindows(kernalByte).empty()
				 && Is(resolver->ResolveWrite(0xD020), DebugMemoryKind::Io, "c64-io"),
				 "with the KERNAL banked out its bytes have no CPU window; I/O is still I/O");

		c64->SetByteC64(0x0001, 0x34);
		MM_CHECK(Is(resolver->ResolveWrite(0xD020), DebugMemoryKind::Ram, "c64-ram"),
				 "with I/O banked out, a write to $D020 lands in RAM");

		MM_CHECK(resolver->Resolve(0xD000, DebugMemoryAccess::Read, DebugMemoryBusMaster::Vic).physical.kind == DebugMemoryKind::Unknown,
				 "the VIC-II's view is Unknown until it is modelled, not the CPU's answer");
	}
	c64->SetByteC64(0x0000, savedDdr);
	c64->SetByteC64(0x0001, savedPort);
	RunBankedCartridges(stepNum, failed);
	TestCompleted(failed.empty(), failed.empty() ? "C64 mapping provider follows VICE's 6510 port configurations and banked cartridges" : failed.c_str());
	c64->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(c64);
	return;

done:
	c64->SetByteC64(0x0000, savedDdr);
	c64->SetByteC64(0x0001, savedPort);
	c64->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(c64);

	if (!failed.empty())
	{
		TestCompleted(false, failed.c_str());
		return;
	}
	TestCompleted(true, "C64 mapping provider follows VICE's 6510 port configurations");
#endif
}
