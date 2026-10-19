#include "CTestBankedWatches.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CGuiMain.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsDataWatch.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#ifdef RUN_COMMODORE64
#include "CDebugInterfaceVice.h"
#endif
#include <cstdio>

bool CTestBankedWatches::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

void CTestBankedWatches::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifndef RUN_COMMODORE64
	TestSkipped("C64 emulation is not compiled in, so banked C64 watches were never exercised");
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (di == NULL || di->GetMemoryMappingResolver() == NULL)
	{
		TestCompleted(false, "the C64 has no memory mapping resolver");
		return;
	}
	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}
	di->PauseEmulationBlockedWait();

	CDebugSymbolsSegment *segment = di->symbols->currentSegment;
	u8 savedDdr = di->GetByteC64(0x0000);
	u8 savedPort = di->GetByteC64(0x0001);
	di->SetByteC64(0x0000, 0x2F);				// the port's direction as the KERNAL sets it
	u8 savedRam[2] = { di->GetByteFromRamC64(0xE000), di->GetByteFromRamC64(0xE001) };
	di->SetByteC64(0x0001, 0x37);				// BASIC and KERNAL in
	di->SetByteToRamC64(0xE000, 0x5A);			// the RAM under the KERNAL
	di->SetByteToRamC64(0xE001, 0xA5);
	u8 kernal0 = di->GetByteC64(0xE000), kernal1 = di->GetByteC64(0xE001);
	char msg[260];

	guiMain->LockMutex();
	segment->AddWatch(0xE000, (char *)"rd059", WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN, 1);
	CDebugSymbolsDataWatch *bound = segment->FindWatch(0xE000);
	bool didBind = bound && bound->BindToMemory();
	guiMain->UnlockMutex();
	bool e0 = false, e1 = false;
	u8 b0 = didBind ? bound->ReadByte(0, &e0) : 0, b1 = didBind ? bound->ReadByte(1, &e1) : 0;
	snprintf(msg, sizeof(msg), "a watch added at $E000 with the KERNAL in binds to the RAM under it (%s $%04X) and shows that RAM, marked as not mapped in: $%02X $%02X (the CPU sees $%02X $%02X)",
			 didBind ? bound->physical.device.c_str() : "-", didBind ? bound->physical.offset : 0, b0, b1, kernal0, kernal1);
	bool ok = Check(didBind && bound->physical.device == "c64-ram" && bound->physical.offset == 0xE000
					&& b0 == 0x5A && b1 == 0xA5 && e0 && e1 && !(kernal0 == 0x5A && kernal1 == 0xA5)
					&& bound->MemoryName() == "c64-ram", msg);

	if (ok)
	{
		di->SetByteC64(0x0001, 0x35);			// KERNAL out: RAM at $E000
		e0 = true;
		b0 = bound->ReadByte(0, &e0);
		CDebugMappedAddress now = di->GetMemoryMappingResolver()->ResolveRead(0xE000);
		snprintf(msg, sizeof(msg), "with the KERNAL out the watch reads the same RAM, now mapped in at $E000 (got $%02X, %s; $01=$%02X, $E000 resolves to %s $%04X, CPU sees $%02X)",
				 b0, e0 ? "not mapped in" : "mapped in", di->GetByteC64(0x0001), now.physical.device.c_str(), now.physical.offset, di->GetByteC64(0xE000));
		ok = Check(b0 == 0x5A && !e0, msg);
		di->SetByteC64(0x0001, 0x37);
	}
	if (ok)
	{
		guiMain->LockMutex();
		segment->AddWatch(0xE001, (char *)"rd059-unbound", WATCH_REPRESENTATION_HEX_8, 1);
		CDebugSymbolsDataWatch *unbound = segment->FindWatch(0xE001);
		guiMain->UnlockMutex();
		e0 = true;
		b0 = unbound ? unbound->ReadByte(0, &e0) : 0;
		snprintf(msg, sizeof(msg), "a watch without a binding (an older watch file) shows what the CPU sees at $E001: the KERNAL's $%02X (got $%02X)", kernal1, b0);
		ok = Check(unbound && !unbound->hasPhysical && b0 == kernal1 && !e0, msg);
		segment->DeleteWatch(0xE001);
	}
	if (ok)
	{
		guiMain->LockMutex();
		segment->AddWatch(0xD020, (char *)"rd059-io", WATCH_REPRESENTATION_HEX_8, 1);
		CDebugSymbolsDataWatch *io = segment->FindWatch(0xD020);
		bool ioBound = io && io->BindToMemory();
		guiMain->UnlockMutex();
		ok = Check(io && !ioBound && !io->hasPhysical, "a watch on an I/O register ($D020) binds to nothing: it stays a live read through the CPU");
		segment->DeleteWatch(0xD020);
	}
	if (ok)
	{
		Hjson::Value saved;
		bound->Serialize(saved);
		CDebugSymbolsDataWatch loaded(segment);
		loaded.Deserialize(saved);
		Hjson::Value old;
		old["Address"] = "e000";
		old["Format"] = "hex8";
		CDebugSymbolsDataWatch oldFormat(segment);
		oldFormat.Deserialize(old);
		e0 = false;
		b0 = loaded.ReadByte(0, &e0);
		ok = Check(loaded.hasPhysical && loaded.physical == bound->physical && b0 == 0x5A && e0
				   && !oldFormat.hasPhysical && oldFormat.address == 0xE000,
				   "the binding is saved with the watch (\"Memory\") and loads back; a watch without it loads unbound");
	}

	segment->DeleteWatch(0xE000);
	di->SetByteToRamC64(0xE000, savedRam[0]);
	di->SetByteToRamC64(0xE001, savedRam[1]);
	di->SetByteC64(0x0001, savedPort);
	di->SetByteC64(0x0000, savedDdr);
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (ok)
		TestCompleted(true, "C64 watches keep their memory: RAM under the KERNAL, unbound and I/O watches, saved and loaded");
	else
		TestCompleted(false, failure.c_str());
#endif
}
