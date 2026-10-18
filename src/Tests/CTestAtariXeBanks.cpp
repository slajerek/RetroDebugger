#include "CTestAtariXeBanks.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebuggerServer.h"
#include "CDebugMemoryMapping.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include "C64SettingsStorage.h"
#ifdef RUN_ATARI
#include "CDebugInterfaceAtari.h"
#include "AtariWrapper.h"
#endif
#include <cstdio>

using namespace nlohmann;

bool CTestAtariXeBanks::Check(bool condition, const std::string &message)
{
	if (!condition)
	{
		failure = message;
		return false;
	}
	StepCompleted(stepNum++, true, message.c_str());
	return true;
}

#ifdef RUN_ATARI
static json CallEndpoint(CDebuggerServer *server, const std::string &fn, const json &params)
{
	std::vector<char> *result = server->RunEndpointFunction(fn, "", params, nullptr, 0);
	if (!result || result->empty())
	{
		delete result;
		return json();
	}
	std::string raw(result->data(), result->size());
	delete result;
	size_t nullPos = raw.find('\0');
	if (nullPos != std::string::npos)
		raw = raw.substr(0, nullPos);
	try { return json::parse(raw); }
	catch (const json::exception &) { return json(); }
}

static bool WaitForPause(CDebugInterface *di, int timeoutMs)
{
	for (int waited = 0; waited < timeoutMs; waited += 10)
	{
		if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
			return true;
		SYS_Sleep(10);
	}
	return false;
}
#endif

void CTestAtariXeBanks::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifndef RUN_ATARI
	TestSkipped("Atari emulation is not compiled in, so the 130XE banks were never exercised");
#else
	CDebugInterfaceAtari *atari = viewC64->debugInterfaceAtari;
	CDebugMemoryMappingResolver *resolver = atari ? atari->GetMemoryMappingResolver() : NULL;
	if (resolver == NULL)
	{
		TestCompleted(false, "the Atari has no memory mapping resolver");
		return;
	}
	bool serverStartedByTest = false;
	if (viewC64->debuggerServer == NULL || !viewC64->debuggerServer->isRunning)
	{
		viewC64->DebuggerServerWebSocketsStart();
		serverStartedByTest = true;
	}
	for (int waited = 0; waited < 2000 && !(viewC64->debuggerServer && viewC64->debuggerServer->isRunning); waited += 50)
		SYS_Sleep(50);
	CDebuggerServer *server = viewC64->debuggerServer;
	if (server == NULL || !server->isRunning)
	{
		TestCompleted(false, "the debugger server did not start");
		return;
	}

	bool wasRunning = atari->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(atari);
		SYS_Sleep(2000);
	}
	atari->PauseEmulationBlockedWait();

	int machineType = -1, ramSizeKb = 0;
	atrd_get_banking_state(&machineType, &ramSizeKb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	if (machineType != 1)
	{
		atari->SetDebugMode(DEBUGGER_MODE_RUNNING);
		if (!wasRunning)
			viewC64->StopEmulationThread(atari);
		if (serverStartedByTest)
			server->Stop();
		TestSkipped("the Atari machine is not an XL/XE, so it has no PORTB extended RAM");
		return;
	}
	bool ramSizeChanged = false;
	if (ramSizeKb != 128)
	{
		atari->SetRamSizeOption(4);		// XL/XE option 4 = 128 KB
		ramSizeChanged = true;
	}
	atari->PauseEmulationBlockedWait();

	int portb = 0, portbMask = 0;
	atrd_get_banking_state(NULL, NULL, &portb, &portbMask, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	u16 savedPc;
	u8 a, x, y, p, sp;
	atari->GetCpuRegs(&savedPc, &a, &x, &y, &p, &sp);

	// DDRB all outputs, as CTestMachineMemoryMapping does
	u8 savedPbctl = atari->GetByte(0xD303);
	atari->SetByte(0xD303, (u8)(savedPbctl & ~0x04));
	atari->SetByte(0xD301, 0xFF);
	atari->SetByte(0xD303, (u8)(savedPbctl | 0x04));
	u8 savedPortb = (u8)(portb | portbMask);

	// bit 4 clear: the CPU sees the bank in bits 2-3; bit 5 set: ANTIC base RAM
	u8 portbBank1 = (u8)((savedPortb & ~0x3C) | (0 << 2) | 0x20);
	u8 portbBank3 = (u8)((savedPortb & ~0x3C) | (2 << 2) | 0x20);
	u8 portbBase = (u8)(savedPortb | 0x30);

	// $4000 in bank 1: INC $0600 / RTS; in bank 3: INC $0601 / RTS
	const u8 bank1Code[] = { 0xEE, 0x00, 0x06, 0x60 };
	const u8 bank3Code[] = { 0xEE, 0x01, 0x06, 0x60 };
	atari->SetByte(0xD301, portbBank1);
	for (int i = 0; i < 4; i++) atari->SetByte((u16)(0x4000 + i), bank1Code[i]);
	atari->SetByte(0xD301, portbBank3);
	for (int i = 0; i < 4; i++) atari->SetByte((u16)(0x4000 + i), bank3Code[i]);
	atari->SetByte(0xD301, portbBase);
	u8 baseByte = atari->GetByte(0x4000);
	// $3000: LDA #bank1 / STA PORTB / JSR $4000 / LDA #bank3 / STA PORTB / JSR $4000 / JMP $3000
	const u8 program[] = { 0xA9, portbBank1, 0x8D, 0x01, 0xD3, 0x20, 0x00, 0x40,
						   0xA9, portbBank3, 0x8D, 0x01, 0xD3, 0x20, 0x00, 0x40, 0x4C, 0x00, 0x30 };
	for (int i = 0; i < (int)sizeof(program); i++) atari->SetByte((u16)(0x3000 + i), program[i]);
	atari->SetByte(0x0600, 0);
	atari->SetByte(0x0601, 0);

	const std::string fnAdd = std::string(atari->GetPlatformNameEndpointString()) + "/cpu/breakpoint/add";
	const std::string fnRemove = std::string(atari->GetPlatformNameEndpointString()) + "/cpu/breakpoint/remove";
	bool ok = true;
	{
		json noKind = CallEndpoint(server, fnAdd, { {"addr", 0x4000}, {"physical", { {"device", "atari-xe-ram"}, {"region", 3}, {"offset", 0} }} });
		ok = Check(noKind.value("status", 0) == HTTP_BAD_REQUEST, "remote: a physical identity without its kind is refused");
	}
	if (ok)
	{
		json added = CallEndpoint(server, fnAdd, { {"addr", "$4000"},
			{"physical", { {"kind", "Extended RAM"}, {"device", "atari-xe-ram"}, {"region", 3}, {"offset", 0} }} });
		ok = Check(added.value("status", 0) == HTTP_OK, "remote: cpu/breakpoint/add at $4000 bound to extended bank 3's first byte");
	}
	if (ok)
	{
		atari->MakeJmpNoReset(atari->GetDataAdapter(), 0x3000);
		atari->SetDebugMode(DEBUGGER_MODE_RUNNING);
		bool paused = WaitForPause(atari, 3000);
		CDebugMappedAddress at = resolver->ResolveFetch(0x4000);
		u8 calls1 = atari->GetByte(0x0600), calls3 = atari->GetByte(0x0601);
		char msg[256];
		snprintf(msg, sizeof(msg), "the breakpoint stops at $4000 only in bank 3, after bank 1's routine ran (PC $%04X, region %u, $0600=%u, $0601=%u)",
				 (unsigned)atari->GetCpuPC(), at.physical.region, calls1, calls3);
		ok = Check(paused && (u16)atari->GetCpuPC() == 0x4000 && at.physical.kind == DebugMemoryKind::ExtendedRam
				   && at.physical.region == 3 && calls1 == 1 && calls3 == 0, msg);
		if (ok)
		{
			CDebugMappedAddress antic = resolver->Resolve(0x4000, DebugMemoryAccess::Read, DebugMemoryBusMaster::Antic);
			u8 under = 0;
			atari->GetDataAdapterDirectRam()->AdapterReadByte(0x4000, &under);
			ok = Check(antic.physical.kind == DebugMemoryKind::Ram && antic.physical.device == "atari-ram" && under == baseByte,
					   "while the CPU has bank 3, ANTIC fetches base RAM, and the direct-RAM adapter reads base RAM there");
		}
		if (ok)
		{
			// bit 5 clear too: ANTIC fetches the CPU's bank (Atari800 leaves ANTIC_xe_ptr NULL)
			atari->SetByte(0xD301, (u8)(portbBank3 & ~0x20));
			CDebugMappedAddress antic = resolver->Resolve(0x4000, DebugMemoryAccess::Read, DebugMemoryBusMaster::Antic);
			atari->SetByte(0xD301, portbBank3);
			ok = Check(antic.physical.kind == DebugMemoryKind::ExtendedRam && antic.physical.region == 3,
					   "with PORTB bits 4 and 5 clear, ANTIC fetches the CPU's bank 3");
		}
		if (ok)
		{
			atari->SetDebugMode(DEBUGGER_MODE_RUNNING);
			paused = WaitForPause(atari, 3000);
			calls1 = atari->GetByte(0x0600);
			calls3 = atari->GetByte(0x0601);
			snprintf(msg, sizeof(msg), "continued, it passes bank 1's $4000 and stops at bank 3's again ($0600=%u, $0601=%u)", calls1, calls3);
			ok = Check(paused && (u16)atari->GetCpuPC() == 0x4000 && resolver->ResolveFetch(0x4000).physical.region == 3
					   && calls1 == 2 && calls3 == 1, msg);
		}
	}
	atari->PauseEmulationBlockedWait();
	CallEndpoint(server, fnRemove, { {"addr", 0x4000} });

	atari->SetByte(0xD301, savedPortb);
	atari->SetByte(0xD303, (u8)(savedPbctl & ~0x04));
	atari->SetByte(0xD301, (u8)~portbMask);			// DDRB as it was
	atari->SetByte(0xD303, savedPbctl);
	atari->MakeJmpNoReset(atari->GetDataAdapter(), savedPc);
	if (ramSizeChanged)
		atari->SetRamSizeOption(c64SettingsAtariRamSizeOption);
	atari->SetDebugMode(DEBUGGER_MODE_RUNNING);
	if (!wasRunning)
		viewC64->StopEmulationThread(atari);
	if (serverStartedByTest)
		server->Stop();

	if (!ok)
		TestCompleted(false, failure.c_str());
	else
		TestCompleted(true, "a breakpoint bound to one 130XE bank stops only in that bank");
#endif
}
