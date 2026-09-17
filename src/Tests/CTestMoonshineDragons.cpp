#include "CTestMoonshineDragons.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Main.h"
#include "EmulatorsConfig.h"
#include "MoonshineDragons/C64DebuggerPluginMoonshineDragons.h"
#include "C64SettingsStorage.h"

#include <stdio.h>
#include <string.h>
#include <vector>

#ifdef RUN_COMMODORE64

static char gMSDFailMsg[256];

void CTestMoonshineDragons::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	gMSDFailMsg[0] = '\0';

	CDebugInterfaceC64 *di = viewC64->debugInterfaceC64;
	if (di == NULL)
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

	// --- Step 1: instantiate the plugin (this is what would happen the
	// first time the user activates "Moonshine Dragons" from the Plugins
	// menu).
	if (pluginMoonshineDragons == NULL)
	{
		PLUGIN_MoonshineDragonsInit();
	}
	if (pluginMoonshineDragons == NULL)
	{
		StepCompleted(1, false, "PLUGIN_MoonshineDragonsInit failed (still NULL)");
		TestCompleted(false, "Plugin init failed");
		goto Done;
	}
	StepCompleted(1, true, "Plugin instantiated");

	// --- Step 2: verify BuildPRG produces a well-formed PRG.
	{
		std::vector<u8> prg = pluginMoonshineDragons->BuildPRG();

		if (prg.size() < 50 || prg.size() > 256)
		{
			snprintf(gMSDFailMsg, sizeof(gMSDFailMsg),
				"PRG size out of range: %d (expected 50-256)", (int)prg.size());
			StepCompleted(2, false, gMSDFailMsg);
			TestCompleted(false, gMSDFailMsg);
			goto Done;
		}

		// Load address $0801 (little-endian)
		if (prg[0] != 0x01 || prg[1] != 0x08)
		{
			snprintf(gMSDFailMsg, sizeof(gMSDFailMsg),
				"PRG load addr wrong: $%02X%02X (expected $0801)", prg[1], prg[0]);
			StepCompleted(2, false, gMSDFailMsg);
			TestCompleted(false, gMSDFailMsg);
			goto Done;
		}

		// BASIC stub "10 SYS 2061" — byte 2 starts the stub at $0801.
		// First byte of stub at offset 2 should be $0B (link pointer low).
		// SYS token $9E should appear at offset 2+4 = 6.
		if (prg[2] != 0x0B || prg[6] != 0x9E)
		{
			snprintf(gMSDFailMsg, sizeof(gMSDFailMsg),
				"BASIC stub malformed: prg[2]=$%02X prg[6]=$%02X (expected $0B/$9E)",
				prg[2], prg[6]);
			StepCompleted(2, false, gMSDFailMsg);
			TestCompleted(false, gMSDFailMsg);
			goto Done;
		}

		// TEXT data sits at C64 address $0850 — offset in PRG payload =
		// 2 (header) + ($0850 - $0801) = 2 + 0x4F = 0x51.
		const int TEXT_OFFSET = 2 + (0x0850 - 0x0801);
		if ((int)prg.size() < TEXT_OFFSET + 17)
		{
			snprintf(gMSDFailMsg, sizeof(gMSDFailMsg),
				"PRG too short for TEXT block (size=%d, need >=%d)",
				(int)prg.size(), TEXT_OFFSET + 17);
			StepCompleted(2, false, gMSDFailMsg);
			TestCompleted(false, gMSDFailMsg);
			goto Done;
		}
		const u8 expectedText[17] = {
			0x0D, 0x0F, 0x0F, 0x0E, 0x13, 0x08, 0x09, 0x0E, 0x05,
			0x20,
			0x04, 0x12, 0x01, 0x07, 0x0F, 0x0E, 0x13
		};
		for (int i = 0; i < 17; i++)
		{
			if (prg[TEXT_OFFSET + i] != expectedText[i])
			{
				snprintf(gMSDFailMsg, sizeof(gMSDFailMsg),
					"TEXT byte %d wrong: $%02X (expected $%02X)",
					i, prg[TEXT_OFFSET + i], expectedText[i]);
				StepCompleted(2, false, gMSDFailMsg);
				TestCompleted(false, gMSDFailMsg);
				goto Done;
			}
		}
		StepCompleted(2, true, "BuildPRG: header, BASIC stub, and TEXT bytes match");
	}

	// --- Step 3: live run via GenerateAndRun (LoadPRG path). This is the
	// path that crashes in the user-facing UI when the button is clicked.
	// If the plugin survives this call AND the screen RAM ends up with the
	// title text, the path is working end-to-end.
	pluginMoonshineDragons->GenerateAndRun();

	// LoadPRG runs on its own thread: hard-reset + KERNAL fast-boot patch
	// + 350 ms sleep + autostart. Give it ~3 s to settle.
	SYS_Sleep(3000);

	{
		// Screen RAM at $05EB should hold the 'M' (screen code $0D).
		u8 m = di->GetByteFromRamC64(0x05EB);
		u8 o = di->GetByteFromRamC64(0x05EC);
		if (m != 0x0D || o != 0x0F)
		{
			snprintf(gMSDFailMsg, sizeof(gMSDFailMsg),
				"Screen RAM at $05EB-$05EC = $%02X $%02X (expected $0D $0F for 'MO')",
				m, o);
			StepCompleted(3, false, gMSDFailMsg);
			TestCompleted(false, gMSDFailMsg);
			goto Done;
		}
		StepCompleted(3, true, "GenerateAndRun: program ran, screen RAM contains 'MO' at $05EB");
	}

	TestCompleted(true, "All steps passed");

Done:
	if (!wasRunning)
	{
		viewC64->StopEmulationThread(di);
	}
	this->isRunning = false;
}

void CTestMoonshineDragons::Cancel()
{
	isRunning = false;
}

#else  // !RUN_COMMODORE64

void CTestMoonshineDragons::Run(ITestCallback *cb)
{
	this->callback = cb;
	TestCompleted(true, "Skipped (RUN_COMMODORE64 not defined)");
}

void CTestMoonshineDragons::Cancel()
{
	isRunning = false;
}

#endif
