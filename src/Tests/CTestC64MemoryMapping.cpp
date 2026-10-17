#include "CTestC64MemoryMapping.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugMemoryMapping.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstdio>
#include <string>

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
