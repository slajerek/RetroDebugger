#include "CTestAutoloadD64.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CMainMenuHelper.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "C64SettingsStorage.h"
#include "CSlrString.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include <cstdio>
#include <cstring>

extern void C64DebuggerSetSetting(const char *settingName, void *value);

#ifdef RUN_COMMODORE64

static char failureMsg[1024];

// Addresses where bitbreaker.d64 hangs when autoload is broken:
//   $103C: BIT $DD00
//   $103F: BMI $103C
// These two instructions form a wait-for-drive loop. PC stuck in that range
// for many frames == failure.
static const u16 HANG_LO = 0x103C;
static const u16 HANG_HI = 0x1040;

// Threshold: emulator must reach this frame before we sample. Gives the
// fastloader plenty of cycles to start running its depacker code if the
// autoload worked.
static const unsigned int TARGET_FRAME = 200;

// Maximum wall-clock time to wait for the target frame (autoload triggers a
// reset/snapshot, sleeps for ~1.5s, then runs ~3s of emulation). Generous.
static const int MAX_WAIT_MS = 5000;

// Sample window — how many extra frames to sample PC after reaching the
// target frame, to confirm the code is *staying* in the hang loop (and not
// just briefly passing through $103c on its way somewhere useful).
static const int SAMPLE_FRAMES = 30;

void CTestAutoloadD64::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

	CDebugInterfaceC64 *di = (CDebugInterfaceC64 *)viewC64->debugInterfaceC64;
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

	// Make sure the C64 is running before we trigger the autoload, so the
	// reset/snapshot path inside InsertD64 -> LoadPRG -> ThreadRun has a live
	// CPU to talk to.
	di->SetDebugMode(DEBUGGER_MODE_RUNNING);
	SYS_Sleep(1000);

	// Force a known-good configuration for this test, regardless of what the
	// user has persisted. Restored at the end.
	bool savedAutoJmpFirstPrg = c64SettingsAutoJmpFromInsertedDiskFirstPrg;
	bool savedForceUnpause = c64SettingsForceUnpause;
	int  savedAutoJmpDoReset = c64SettingsAutoJmpDoReset;
	int  savedWaitAfterReset = c64SettingsAutoJmpWaitAfterReset;
	c64SettingsAutoJmpFromInsertedDiskFirstPrg = true;
	c64SettingsForceUnpause = true;
	// Use HARD reset mode (no snapshot, no model change) — simplest path,
	// matches what 8f8f6ec0 (0.64.74) did and gives the fastloader the most
	// predictable chip state to talk to.
	c64SettingsAutoJmpDoReset = MACHINE_LOADPRG_RESET_MODE_HARD;
	if (c64SettingsAutoJmpWaitAfterReset < 1500)
		c64SettingsAutoJmpWaitAfterReset = 1500;

	CSlrString *path = new CSlrString(CTest::ResolveProjectPath("tests/data/bitbreaker.d64").c_str());

	bool inserted = viewC64->mainMenuHelper->InsertD64(path, false, true, 0, false);
	delete path;

	if (!inserted)
	{
		c64SettingsAutoJmpFromInsertedDiskFirstPrg = savedAutoJmpFirstPrg;
		c64SettingsForceUnpause = savedForceUnpause;
		c64SettingsAutoJmpDoReset = savedAutoJmpDoReset;
		c64SettingsAutoJmpWaitAfterReset = savedWaitAfterReset;
		TestCompleted(false, "InsertD64 returned false (file missing or attach failed)");
		return;
	}

	// Wall-clock wait — long enough to cover the autoload worker thread
	// (reset + ~1.5s sleep) and then give the demo plenty of frames to
	// either succeed or get stuck.
	SYS_Sleep(MAX_WAIT_MS);

	// Sample PC across SAMPLE_FRAMES iterations. If every sample falls in
	// [$103C, $1040) the loader is wedged on BIT $DD00 / BMI $103C.
	// Frame counter is unreliable in headless mode (no vsync), so we
	// sample by wall time instead.
	int hangSamples = 0;
	int totalSamples = 0;
	u16 lastPC = 0;
	u16 drivePCMin = 0xFFFF, drivePCMax = 0x0000;
	int driveStuckAt0400 = 0;
	for (int i = 0; i < SAMPLE_FRAMES; i++)
	{
		C64StateCPU cpu;
		di->GetC64CpuState(&cpu);
		lastPC = cpu.pc;
		totalSamples++;
		if (cpu.pc >= HANG_LO && cpu.pc < HANG_HI)
			hangSamples++;

		C64StateCPU dcpu;
		((CDebugInterfaceVice *)di)->GetDrive1541CpuState(&dcpu);
		if (dcpu.pc < drivePCMin) drivePCMin = dcpu.pc;
		if (dcpu.pc > drivePCMax) drivePCMax = dcpu.pc;
		if (dcpu.pc >= 0x0400 && dcpu.pc < 0x0500) driveStuckAt0400++;

		SYS_Sleep(20);
	}
	(void)drivePCMin; (void)drivePCMax; (void)driveStuckAt0400;

	// Also dump chip state at the failure point — CIA2 (IEC bus from C64
	// side) and drive VIA1 (IEC bus from drive side). Whether or not the
	// test fails, these give us forensic evidence to compare against a
	// working autoload.
	u8 dd00 = di->GetByteC64(0xDD00);
	u8 dd02 = di->GetByteC64(0xDD02);
	u8 v1800 = di->GetByte1541(0x1800);
	u8 v1801 = di->GetByte1541(0x1801);
	u8 v1802 = di->GetByte1541(0x1802);
	u8 v1803 = di->GetByte1541(0x1803);
	u8 v180c = di->GetByte1541(0x180C);
	(void)v1801; (void)v1802; (void)v1803;
	// Read drive RAM at $0400-$0407 to see what's actually executing
	u8 r0400[8];
	for (int i = 0; i < 8; i++)
		r0400[i] = ((CDebugInterfaceVice *)di)->GetByteFromRam1541(0x0400 + i);

	// Also dump the C64 hang loop and instructions immediately preceding it.
	// We need to know whether the C64 is supposed to be DRIVING a line to
	// wake up the drive's $0400 JMP($1800) poller, or whether it's just
	// waiting for the drive to initiate the handshake.
	{
		FILE *f = fopen("/tmp/autoload-c64-1000-12ff.bin", "wb");
		if (f) {
			for (u16 a = 0x1000; a < 0x1300; a++) {
				u8 b = di->GetByteC64(a);
				fwrite(&b, 1, 1, f);
			}
			fclose(f);
		}
	}
	{
		FILE *f = fopen("/tmp/autoload-drive-0000-07ff.bin", "wb");
		if (f) {
			for (u16 a = 0x0000; a < 0x0800; a++) {
				u8 b = ((CDebugInterfaceVice *)di)->GetByteFromRam1541(a);
				fwrite(&b, 1, 1, f);
			}
			fclose(f);
		}
	}
	// drive cpu PC — is the drive even running its idle loop?
	u16 drivePC = 0;
	{
		CDebugInterfaceVice *vi = (CDebugInterfaceVice *)di;
		C64StateCPU dcpu;
		vi->GetDrive1541CpuState(&dcpu);
		drivePC = dcpu.pc;
	}

	unsigned int reachedFrame = di->GetEmulationFrameNumber();

	c64SettingsAutoJmpFromInsertedDiskFirstPrg = savedAutoJmpFirstPrg;
	c64SettingsForceUnpause = savedForceUnpause;
	c64SettingsAutoJmpDoReset = savedAutoJmpDoReset;
	c64SettingsAutoJmpWaitAfterReset = savedWaitAfterReset;

	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (totalSamples == 0)
	{
		TestCompleted(false, "No PC samples collected (frame counter not advancing)");
		return;
	}

	const char *chipFmt = " chips: DD00=$%02X DD02=$%02X drv 1800-180C=%02X %02X %02X %02X %02X drvPC=$%04X drvRAM[$0400..]=%02X %02X %02X %02X %02X %02X %02X %02X (PCrange=$%04X-$%04X 04xx=%d/%d frame=%u)";

	if (totalSamples == 0)
	{
		TestCompleted(false, "No PC samples collected");
		return;
	}

	// All samples in the hang loop -> autoload is broken.
	if (hangSamples == totalSamples)
	{
		int n = sprintf(failureMsg,
			"Autoload hung at $103C BIT $DD00 / $103F BMI $103C (PC=$%04X across %d samples) — drive not responding to fastloader's IEC poll",
			lastPC, totalSamples);
		sprintf(failureMsg + n, chipFmt, dd00, dd02, v1800, v1801, v1802, v1803, v180c, drivePC, r0400[0], r0400[1], r0400[2], r0400[3], r0400[4], r0400[5], r0400[6], r0400[7], drivePCMin, drivePCMax, driveStuckAt0400, totalSamples, reachedFrame);
		TestCompleted(false, failureMsg);
		return;
	}

	// More than half in the hang loop is also a failure.
	if (hangSamples * 2 > totalSamples)
	{
		int n = sprintf(failureMsg,
			"Autoload partially wedged: %d/%d PC samples inside hang loop $103C-$103F (last PC=$%04X)",
			hangSamples, totalSamples, lastPC);
		sprintf(failureMsg + n, chipFmt, dd00, dd02, v1800, v1801, v1802, v1803, v180c, drivePC, r0400[0], r0400[1], r0400[2], r0400[3], r0400[4], r0400[5], r0400[6], r0400[7], drivePCMin, drivePCMax, driveStuckAt0400, totalSamples, reachedFrame);
		TestCompleted(false, failureMsg);
		return;
	}

	// Emulator stalled before reaching the demo at all (e.g. CPU paused or
	// jammed somewhere far from $1xxx).
	if (reachedFrame < 10 && lastPC < 0x0200)
	{
		int n = sprintf(failureMsg,
			"Autoload never started the demo: PC=$%04X, frame=%u — CPU did not advance after autoload",
			lastPC, reachedFrame);
		sprintf(failureMsg + n, chipFmt, dd00, dd02, v1800, v1801, v1802, v1803, v180c, drivePC, r0400[0], r0400[1], r0400[2], r0400[3], r0400[4], r0400[5], r0400[6], r0400[7], drivePCMin, drivePCMax, driveStuckAt0400, totalSamples, reachedFrame);
		TestCompleted(false, failureMsg);
		return;
	}

	char okMsg[512];
	int n = sprintf(okMsg, "Autoload OK: %d/%d samples outside hang loop, last PC=$%04X",
			totalSamples - hangSamples, totalSamples, lastPC);
	sprintf(okMsg + n, chipFmt, dd00, dd02, v1800, v1801, v1802, v1803, v180c, drivePC, r0400[0], r0400[1], r0400[2], r0400[3], r0400[4], r0400[5], r0400[6], r0400[7], drivePCMin, drivePCMax, driveStuckAt0400, totalSamples, reachedFrame);
	TestCompleted(true, okMsg);
}

void CTestAutoloadD64::Cancel()
{
	isRunning = false;
}

#else

void CTestAutoloadD64::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	TestCompleted(true, "Skipped (C64 not compiled in)");
}

void CTestAutoloadD64::Cancel()
{
	isRunning = false;
}

#endif
