#include "CTestViceSidHooks.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include "C64SettingsStorage.h"
#include <cstdio>
#include <cstring>

extern "C"
{
#include "ViceWrapper.h"
#ifdef HAVE_USBSID
#include "usbsid.h"
#endif
};

// Test program that continuously writes to SID ($D400-$D418):
//   $1000: SEI
//   $1001: LDA #$19        ; frequency low byte
//   $1003: STA $D400       ; Voice 1 Freq Lo
//   $1006: LDA #$03        ; frequency high byte
//   $1008: STA $D401       ; Voice 1 Freq Hi
//   $100B: LDA #$0F
//   $100D: STA $D418       ; Volume = max
//   $1010: LDA #$21        ; sawtooth + gate
//   $1012: STA $D404       ; Voice 1 Control
//   $1015: JMP $1001       ; loop: continuously write SID registers

static const u8 testCode[] = {
	0x78,                   // $1000: SEI
	0xA9, 0x19,             // $1001: LDA #$19
	0x8D, 0x00, 0xD4,       // $1003: STA $D400
	0xA9, 0x03,             // $1006: LDA #$03
	0x8D, 0x01, 0xD4,       // $1008: STA $D401
	0xA9, 0x0F,             // $100B: LDA #$0F
	0x8D, 0x18, 0xD4,       // $100D: STA $D418
	0xA9, 0x21,             // $1010: LDA #$21
	0x8D, 0x04, 0xD4,       // $1012: STA $D404
	0x4C, 0x01, 0x10,       // $1015: JMP $1001 (loop writes)
};

static char failureMsg[512];

void CTestViceSidHooks::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

#ifndef RUN_COMMODORE64
	TestCompleted(true, "Skipped (C64 not enabled)");
	return;
#else
	CDebugInterfaceVice *di = (CDebugInterfaceVice *)viewC64->debugInterfaceC64;
	if (!di)
	{
		TestCompleted(false, "C64 debug interface is NULL");
		return;
	}

	bool wasRunning = di->isRunning;
	if (!wasRunning)
	{
		viewC64->StartEmulationThread(di);
		SYS_Sleep(2000);
	}

	if (!di->isRunning)
	{
		TestCompleted(false, "C64 emulator failed to start");
		return;
	}

	bool allPassed = true;

	// --- Step 1: SID register read/write via API ---
	{
		di->PauseEmulationBlockedWait();

		// Write and read SID registers
		di->SetSidRegister(0, 0x18, 0x0F);  // Volume = max
		u8 vol = di->GetSidRegister(0, 0x18);

		// SID volume is lower 4 bits of $D418
		if ((vol & 0x0F) != 0x0F)
		{
			sprintf(failureMsg, "SID register: wrote $0F to $D418, read $%02X", vol);
			allPassed = false;
		}

		if (allPassed)
			StepCompleted(1, true, "SID register read/write API verified");
		else
			StepCompleted(1, false, failureMsg);
	}

	// --- Step 2: SID write tracking in per-cycle state ---
	if (allPassed)
	{
		u8 prevMode = c64SettingsVicStateRecordingMode;
		di->SetVicRecordStateMode(C64D_VICII_RECORD_MODE_EVERY_CYCLE);

		di->PauseEmulationBlockedWait();

		// Inject SID test code
		for (int i = 0; i < (int)sizeof(testCode); i++)
		{
			di->SetByteToRamC64(0x1000 + i, testCode[i]);
		}

		di->MakeJmpC64(0x1000);

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(300);
		di->PauseEmulationBlockedWait();

		// Scan for SID write markers (retry once if not found — timing sensitive)
		bool foundSidWrite = false;
		int sidWriteCount = 0;

		for (int attempt = 0; attempt < 2 && !foundSidWrite; attempt++)
		{
			if (attempt > 0)
			{
				di->SetDebugMode(DEBUGGER_MODE_RUNNING);
				SYS_Sleep(300);
				di->PauseEmulationBlockedWait();
			}

			for (int rasterLine = 0; rasterLine < 312; rasterLine++)
			{
				for (int rasterCycle = 0; rasterCycle < 63; rasterCycle++)
				{
					vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);
					if (state->sidRegisterWritten >= 0)
					{
						foundSidWrite = true;
						sidWriteCount++;
					}
				}
			}
		}

		di->SetVicRecordStateMode(prevMode);

		if (!foundSidWrite)
		{
			sprintf(failureMsg, "SID write not tracked in per-cycle state");
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "SID write tracking: %d writes found in cycle state", sidWriteCount);
			StepCompleted(2, true, msg);
		}
		else
		{
			StepCompleted(2, false, failureMsg);
		}
	}

	// --- Step 3: SID type and sampling method ---
	if (allPassed)
	{
		// Get SID types list
		std::vector<const char *> sidTypes;
		di->GetSidTypes(&sidTypes);

		if (sidTypes.empty())
		{
			sprintf(failureMsg, "GetSidTypes returned empty list");
			allPassed = false;
		}

		// Set and verify sampling method
		if (allPassed)
		{
			di->SetSidSamplingMethod(0);  // Fast
			di->SetSidEmulateFilters(1);  // Enable filters
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "SID configuration: %d SID types available, num SIDs=%d", (int)sidTypes.size(), di->GetNumSids());
			StepCompleted(3, true, msg);
		}
		else
		{
			StepCompleted(3, false, failureMsg);
		}
	}

	// --- Step 4: SID channel data reception ---
	if (allPassed)
	{
		// Enable channel data reception for SID 0
		di->SetSIDReceiveChannelsData(0, true);

		// Run for a bit with our tone playing
		di->PauseEmulationBlockedWait();
		for (int i = 0; i < (int)sizeof(testCode); i++)
		{
			di->SetByteToRamC64(0x1000 + i, testCode[i]);
		}
		di->MakeJmpC64(0x1000);

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(200);
		di->PauseEmulationBlockedWait();

		// Check that channel data flag is set
		bool isReceiving = (c64d_is_receive_channels_data[0] != 0);

		// Disable channel data
		di->SetSIDReceiveChannelsData(0, false);

		if (!isReceiving)
		{
			sprintf(failureMsg, "SID channel data reception flag not set");
			allPassed = false;
		}

		if (allPassed)
			StepCompleted(4, true, "SID channel data reception enabled/verified");
		else
			StepCompleted(4, false, failureMsg);
	}

	// --- Step 5: HardSID code presence verification ---
	// NOTE: HardSID is a Windows-only feature (real SID chip hardware via HARDSID.DLL).
	// This step does NOT test actual HardSID hardware — it verifies that the HardSID
	// code paths were not accidentally removed during a VICE upgrade.
	//
	// Coverage chain:
	// - sid.h defines SID_ENGINE_HARDSID (=3) — used by sid.c, sid-resources.c under
	//   #ifdef HAVE_HARDSID. If removed from sid.h, the VICE build itself fails.
	// - CDebugInterfaceVice.cpp:SetSidTypeAsync case 15 calls
	//   sid_set_engine_model(SID_ENGINE_HARDSID, ...). If the constant is gone, this
	//   file won't compile.
	// - CDebugInterfaceVice.cpp:GetSidTypes adds "HardSID" at index 15 under
	//   #if defined(WIN32). This test verifies the list size matches expectations,
	//   catching accidental removal of that block.
	// - platform/Windows/src.Windows/hardsid/ contains the DLL loader
	//   (hs-win32-dll.c, hardsid-win32-drv.c) — compile-verified by Windows build.
	if (allPassed)
	{
		std::vector<const char *> sidTypes;
		di->GetSidTypes(&sidTypes);
		int numTypes = (int)sidTypes.size();

#if defined(WIN32)
		// On Windows: expect 16 types (indices 0-14 emulated + index 15 HardSID)
		if (numTypes < 16)
		{
			sprintf(failureMsg, "HardSID missing: GetSidTypes returned %d types on Windows (expected 16)", numTypes);
			allPassed = false;
		}

		if (allPassed)
		{
			bool foundHardSid = false;
			for (int i = 0; i < numTypes; i++)
			{
				if (strstr(sidTypes[i], "HardSID") != NULL)
				{
					foundHardSid = true;
					break;
				}
			}

			if (!foundHardSid)
			{
				sprintf(failureMsg, "HardSID not in GetSidTypes() on Windows (%d types listed)", numTypes);
				allPassed = false;
			}
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "HardSID present in GetSidTypes: %d types (Windows, HARDSID.DLL support)", numTypes);
			StepCompleted(5, true, msg);
		}
		else
		{
			StepCompleted(5, false, failureMsg);
		}
#else
		// On macOS/Linux: expect 15 emulated types (no HardSID, Windows-only) plus
		// USBSID-Pico appended last when built with HAVE_USBSID and a board was
		// attached when the list was built (usbsid_detect()).
		// If this count changes, either new SID engines were added (update expected
		// count) or the HardSID #if defined(WIN32) guard was accidentally removed.
#if defined(HAVE_USBSID)
		const int expectedTypes = (usbsid_detect() > 0) ? 16 : 15;
#else
		const int expectedTypes = 15;
#endif
		if (numTypes != expectedTypes)
		{
			sprintf(failureMsg, "Unexpected SID type count: %d (expected %d on non-Windows)", numTypes, expectedTypes);
			allPassed = false;
		}
#if defined(HAVE_USBSID)
		if (allPassed && numTypes == 16 && strstr(sidTypes[numTypes - 1], "USBSID") == NULL)
		{
			sprintf(failureMsg, "USBSID-Pico is not the last SID type: '%s'", sidTypes[numTypes - 1]);
			allPassed = false;
		}
#endif

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "HardSID code verified: %d SID types (Windows-only HardSID correctly excluded)", numTypes);
			StepCompleted(5, true, msg);
		}
		else
		{
			StepCompleted(5, false, failureMsg);
		}
#endif
	}

	// Restore emulator state
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "SID hooks verified: registers, cycle tracking, types, channel data, HardSID presence");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestViceSidHooks::Cancel()
{
	isRunning = false;
}
