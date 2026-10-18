#include "CTestCoreMappingAccessors.h"
#include "CTestNesCartridge.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugDataAdapter.h"
#include "NesWrapper.h"
#include "CDebugInterfaceNes.h"
#include "SYS_Main.h"
#include "AtariWrapper.h"
#include <cstdio>

extern "C"
{
#include "ViceWrapper.h"
unsigned char c64d_peek_cart_roml(int addr);
unsigned char c64d_peek_cart_buffer(int kind, int addr);
};

#define CA_CHECK(cond, msg) \
	do { \
		if (!(cond)) { TestCompleted(false, msg); return; } \
		StepCompleted(stepNum++, true, msg); \
	} while (0)

void CTestCoreMappingAccessors::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	int stepNum = 1;

#ifdef RUN_COMMODORE64
	{
		int memConfig = -100, cartType = -100, exrom = -100, game = -100;
		int romlBank = -100, romhBank = -100, exportRam = -100;
		c64d_get_banking_state(&memConfig, &cartType, &exrom, &game, &romlBank, &romhBank, &exportRam);
		CA_CHECK(memConfig >= 0 && memConfig <= 31 && cartType >= -1 && romlBank >= 0 && romhBank >= 0
				 && exportRam >= 0,
				 "C64: c64d_get_banking_state fills every field with a sane value");
		CA_CHECK(((memConfig >> 3) & 1) == (exrom & 1) && ((memConfig >> 4) & 1) == (game & 1),
				 "C64: memConfig carries the EXROM/GAME lines VICE holds");
		c64d_get_banking_state(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		StepCompleted(stepNum++, true, "C64: c64d_get_banking_state accepts NULL outputs");

		CA_CHECK(c64d_peek_cart_buffer(0, -1) == 0 && c64d_peek_cart_buffer(0, 2 * 1024 * 1024) == 0
				 && c64d_peek_cart_buffer(7, 0) == 0,
				 "C64: cartridge buffer peek is bounds-checked");

		CDebugInterfaceC64 *c64 = viewC64->debugInterfaceC64;
		CDebugDataAdapter *cartAdapter = c64 ? c64->dataAdapterCartridgeC64 : NULL;
		if (cartAdapter != NULL)
		{
			bool same = true;
			for (int addr = 0; addr < 0x2000 && same; addr += 0x101)
			{
				u8 value = 0;
				cartAdapter->AdapterReadByte(addr, &value);
				same = (value == c64d_peek_cart_roml(addr));
			}
			bool available = true;
			u8 value = 0;
			cartAdapter->AdapterReadByte(cartAdapter->AdapterGetDataLength(), &value, &available);
			CA_CHECK(same && !available, "C64: the cartridge view reads ROML and reports its end");
		}
	}
#endif

#ifdef RUN_ATARI
	{
		int machineType = -1, ramSizeKb = -1, portb = -1, portbMask = -1, xeBank = -1, anticXeBank = -1;
		int selftest = -1, cart809f = -1, cartA0bf = -1, cartType = -1, cartState = -1, cartSizeKb = -1;
		atrd_get_banking_state(&machineType, &ramSizeKb, &portb, &portbMask, &xeBank, &anticXeBank,
							   &selftest, &cart809f, &cartA0bf, &cartType, &cartState, &cartSizeKb);
		CA_CHECK(machineType >= 0 && machineType <= 2 && ramSizeKb >= 16 && portb >= 0 && portb <= 0xFF
				 && xeBank >= 0 && anticXeBank >= 0 && cartSizeKb >= 0,
				 "Atari: atrd_get_banking_state fills every field with a sane value");
		atrd_get_banking_state(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		StepCompleted(stepNum++, true, "Atari: atrd_get_banking_state accepts NULL outputs");

		// MEMORY_RAM 0, MEMORY_ROM 1, MEMORY_HARDWARE 2; page zero is RAM on
		// every model, and $D000 is GTIA on the 400/800 and XL/XE.
		CA_CHECK(atrd_get_memory_attrib(0x0080) == 0
				 && (machineType == 2 || atrd_get_memory_attrib(0xD000) == 2)
				 && atrd_get_memory_attrib(-1) == -1 && atrd_get_memory_attrib(0x10000) == -1,
				 "Atari: per-address attributes come from the emulator, out of range is -1");
		CA_CHECK(atrd_peek_backing(0, 0) >= 0 && atrd_peek_backing(0, 0x4000) == -1
				 && atrd_peek_backing(9, 0) == -1 && atrd_peek_backing(1, -1) == -1,
				 "Atari: backing RAM peeks are bounds-checked");
	}
#endif

#ifdef RUN_NES
	{
		CA_CHECK(nesd_peek_safe_io(0x0000) == nesd_peek_safe_io(0x0800)
				 && nesd_peek_safe_io(0x07FF) == nesd_peek_safe_io(0x1FFF),
				 "NES: internal RAM mirrors through $1FFF");
		CA_CHECK(nesd_peek_safe_io(0x2002) == 0 && nesd_peek_safe_io(0x3FFF) == 0
				 && nesd_peek_safe_io(0x4016) == 0 && nesd_peek_safe_io(0x401F) == 0,
				 "NES: PPU/APU/I-O registers are not read by the safe peek (they report 0)");

		int mapper = -1, subMapper = -1;
		unsigned int prgRomSize = 0;
		int isWrk = -1, source = -1, writable = -1;
		unsigned int offset = 0;
		if (nesd_get_cartridge_info(&mapper, &subMapper, &prgRomSize))
		{
			int mapped = nesd_get_prg_mapping(0xFFFC, &isWrk, &source, &offset, &writable);
			char msg[256];
			snprintf(msg, sizeof(msg), "NES: mapper %d, PRG %u bytes; reset vector at PRG offset $%X", mapper, prgRomSize, offset);
			CA_CHECK(mapper >= 0 && prgRomSize > 0 && mapped && isWrk == 0 && source == 0
					 && offset < prgRomSize && !writable, msg);
		}
		else
		{
			CA_CHECK(!nesd_get_prg_mapping(0x8000, &isWrk, &source, &offset, &writable),
					 "NES: no cartridge loaded, so there is no PRG mapping");
		}
		CA_CHECK(!nesd_get_prg_mapping(0x0000, NULL, NULL, NULL, NULL)
				 && !nesd_get_prg_mapping(0x4020, NULL, NULL, NULL, NULL),
				 "NES: addresses outside cartridge space have no PRG mapping");

		// With a real cartridge: tests/data/test_input_replay.nes is NROM-128,
		// one 16 KB PRG bank visible at both $8000 and $C000 -- an alias.
		CDebugInterfaceNes *nes = (CDebugInterfaceNes *)viewC64->debugInterfaceNes;
		std::string romPath = ResolveProjectPath("tests/data/test_input_replay.nes");
		if (nes != NULL && !romPath.empty())
		{
			bool wasRunning = nes->isRunning;
			if (!wasRunning)
			{
				viewC64->StartEmulationThread(nes);
				SYS_Sleep(2000);
			}

			// Load on the emulation thread and stay paused (CTestNesCartridge.h):
			// nesd_insert_cartridge loads AND powers the machine, so the board's
			// page tables are mapped when it returns.
			bool loaded = nes->isRunning && TestNesInsertCartridge(nes, romPath.c_str(), true)
				&& nesd_get_cartridge_info(&mapper, &subMapper, &prgRomSize);

			unsigned int at8000 = 1, atC000 = 1, atFFFC = 0;
			int src8000 = -1, srcC000 = -1, srcFFFC = -1, wrk8000 = -1, wr8000 = -1;
			bool ok8000 = false, okC000 = false, okFFFC = false;
			if (loaded)
			{
				ok8000 = nesd_get_prg_mapping(0x8000, &wrk8000, &src8000, &at8000, &wr8000);
				okC000 = nesd_get_prg_mapping(0xC000, NULL, &srcC000, &atC000, NULL);
				okFFFC = nesd_get_prg_mapping(0xFFFC, NULL, &srcFFFC, &atFFFC, NULL);
				TestNesUnloadCartridge(nes, false);
			}
			nes->SetDebugMode(DEBUGGER_MODE_RUNNING);
			if (!wasRunning)
				viewC64->StopEmulationThread(nes);

			if (loaded)
			{
				char msg[512];
				bool ok = mapper == 0 && prgRomSize == 0x4000 && ok8000 && okC000 && okFFFC
					&& wrk8000 == 0 && wr8000 == 0 && src8000 == 0 && srcC000 == 0 && srcFFFC == 0
					&& at8000 == 0 && atC000 == 0 && atFFFC == 0x3FFC;
				snprintf(msg, sizeof(msg), ok
						 ? "NES: NROM-128 maps PRG offset 0 at both $8000 and $C000, $FFFC at offset $3FFC"
						 : "NES: NROM-128 PRG mapping wrong: mapper=%d prg=%u $8000 ok=%d src=%d off=$%X wrk=%d w=%d, $C000 ok=%d src=%d off=$%X, $FFFC ok=%d src=%d off=$%X",
						 mapper, prgRomSize, ok8000, src8000, at8000, wrk8000, wr8000, okC000, srcC000, atC000, okFFFC, srcFFFC, atFFFC);
				CA_CHECK(ok, msg);
				CA_CHECK(!nesd_get_cartridge_info(NULL, NULL, NULL), "NES: the test cartridge is unloaded again");
			}
			else
			{
				ReportRequiredGap("NES test ROM did not load, so PRG mapping with a cartridge was never exercised");
			}
		}
		else
		{
			ReportRequiredGap("tests/data/test_input_replay.nes not found, so PRG mapping with a cartridge was never exercised");
		}
	}
#endif

	TestCompleted(true, "core mapping accessors return sane values");
}
