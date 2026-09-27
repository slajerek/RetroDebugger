#include "CTestVicePlatformAbstraction.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "DebuggerDefs.h"
#include "C64SettingsStorage.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

extern "C"
{
#include "ViceWrapper.h"
};

static char failureMsg[512];

class CTestResetPausingDebugInterface : public CDebugInterface
{
public:
	CTestResetPausingDebugInterface(CViewC64 *viewC64) : CDebugInterface(viewC64)
	{
		isRunning = true;
	}

	void ResetHard() override
	{
		SetDebugMode(DEBUGGER_MODE_PAUSED);
	}

	void ResetSoft() override
	{
		SetDebugMode(DEBUGGER_MODE_PAUSED);
	}

	void ClearDebugMarkers() override
	{
	}
};

using FramePacingClock = std::chrono::steady_clock;

struct FramePacingSample
{
	unsigned int frames = 0;
	double elapsedMs = 0.0;
	bool counterReset = false;

	double FramesPerSecond() const
	{
		return elapsedMs > 0.0 ? frames * 1000.0 / elapsedMs : 0.0;
	}
};

static FramePacingSample CountFramesFor(CDebugInterfaceVice *di, int durationMs)
{
	// SYS_Sleep is a minimum delay, not an exact measurement interval. A CI
	// worker can wake late while the emulation thread keeps producing frames.
	// Do not reset the running counter (and debug history) to take a sample.
	auto start = FramePacingClock::now();
	unsigned int before = di->GetEmulationFrameNumber();
	SYS_Sleep(durationMs);
	unsigned int after = di->GetEmulationFrameNumber();
	auto end = FramePacingClock::now();
	FramePacingSample sample;
	sample.counterReset = after < before;
	sample.frames = sample.counterReset ? 0 : after - before;
	sample.elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
	return sample;
}

static bool FramePacingRecovered(const FramePacingSample &baseline, const FramePacingSample &resumed)
{
	return !baseline.counterReset && !resumed.counterReset
		&& baseline.elapsedMs > 0.0 && resumed.elapsedMs > 0.0
		&& resumed.FramesPerSecond() >= baseline.FramesPerSecond() * 0.9;
}

static bool WaitForFrameAdvance(CDebugInterfaceVice *di, unsigned int minFrames, int timeoutMs)
{
	unsigned int before = di->GetEmulationFrameNumber();
	auto deadline = FramePacingClock::now() + std::chrono::milliseconds(timeoutMs);
	while (FramePacingClock::now() < deadline)
	{
		unsigned int after = di->GetEmulationFrameNumber();
		// Step 8 queues resets. A backwards counter is not frame progress:
		// unsigned subtraction would otherwise make it look enormous.
		if (after < before)
			before = after;
		if (after - before >= minFrames)
			return true;
		SYS_Sleep(20);
	}
	return false;
}

void CTestVicePlatformAbstraction::Run(ITestCallback *cb)
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

	// --- Step 1: FPS reporting (c64d_display_speed callback) ---
	{
		// Run for a bit to accumulate FPS data
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(1000);
		di->PauseEmulationBlockedWait();

		float fps = di->GetEmulationFPS();

		if (fps <= 0.0f)
		{
			sprintf(failureMsg, "GetEmulationFPS() returned %.2f (expected > 0)", fps);
			allPassed = false;
		}
		else if (fps > 200.0f)
		{
			sprintf(failureMsg, "GetEmulationFPS() returned %.2f (unreasonably high)", fps);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "FPS reporting: %.1f Hz (platform timing functional)", fps);
			StepCompleted(1, true, msg);
		}
		else
		{
			StepCompleted(1, false, failureMsg);
		}
	}

	// --- Step 2: Frame counter (c64d_get_frame_num callback) ---
	if (allPassed)
	{
		unsigned int frame1 = c64d_get_frame_num();

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(200);
		di->PauseEmulationBlockedWait();

		unsigned int frame2 = c64d_get_frame_num();

		if (frame2 <= frame1)
		{
			sprintf(failureMsg, "Frame counter: %u -> %u (did not advance)", frame1, frame2);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Frame counter: advanced %u frames", frame2 - frame1);
			StepCompleted(2, true, msg);
		}
		else
		{
			StepCompleted(2, false, failureMsg);
		}
	}

	// --- Step 3: Emulation frame counter (CDebugInterface level) ---
	if (allPassed)
	{
		di->ResetEmulationFrameCounter();
		unsigned int fc1 = di->GetEmulationFrameNumber();

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(200);
		di->PauseEmulationBlockedWait();

		unsigned int fc2 = di->GetEmulationFrameNumber();

		if (fc2 <= fc1)
		{
			sprintf(failureMsg, "Emulation frame counter: %u -> %u (did not advance)", fc1, fc2);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Emulation frame counter: %u -> %u", fc1, fc2);
			StepCompleted(3, true, msg);
		}
		else
		{
			StepCompleted(3, false, failureMsg);
		}
	}

	// --- Step 4: Screen geometry ---
	if (allPassed)
	{
		di->PauseEmulationBlockedWait();
		di->ResetEmulationFrameCounter();

		unsigned int frameBefore = di->GetEmulationFrameNumber();
		bool stepOk = di->RunEmulationForOneFrame();
		unsigned int frameAfter = di->GetEmulationFrameNumber();

		if (!stepOk)
		{
			sprintf(failureMsg, "RunEmulationForOneFrame returned false");
			allPassed = false;
		}
		else if (di->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			sprintf(failureMsg, "RunEmulationForOneFrame ended in debug mode %d instead of PAUSED", di->GetDebugMode());
			allPassed = false;
		}
		else if (frameAfter != frameBefore + 1)
		{
			sprintf(failureMsg, "RunEmulationForOneFrame advanced %u -> %u instead of exactly one frame", frameBefore, frameAfter);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "RunEmulationForOneFrame: %u -> %u and paused", frameBefore, frameAfter);
			StepCompleted(4, true, msg);
		}
		else
		{
			StepCompleted(4, false, failureMsg);
		}
	}

	// --- Step 5: Screen geometry ---
	if (allPassed)
	{
		int screenW = di->GetScreenSizeX();
		int screenH = di->GetScreenSizeY();

		// PAL C64: typical 384x272 (with borders) or 320x200 (no borders)
		// NTSC: 411x234 or similar
		if (screenW < 100 || screenW > 600 || screenH < 100 || screenH > 400)
		{
			sprintf(failureMsg, "Screen size: %dx%d (out of expected range)", screenW, screenH);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Screen geometry: %dx%d", screenW, screenH);
			StepCompleted(5, true, msg);
		}
		else
		{
			StepCompleted(5, false, failureMsg);
		}
	}

	// --- Step 6: Debug mode control ---
	if (allPassed)
	{
		// Test debug on/off
		di->SetDebugOn(true);
		di->SetDebugOnC64(true);

		// Verify no crash — these affect the breakpoint checking code path
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(100);
		di->PauseEmulationBlockedWait();

		// Test CPU jam detection
		bool isJammed = di->IsCpuJam();
		// CPU should not be jammed under normal operation

		if (allPassed)
		{
			char msg[128];
			sprintf(msg, "Debug control: debugOn=%d, cpuJammed=%d", di->isDebugOn ? 1 : 0, isJammed ? 1 : 0);
			StepCompleted(6, true, msg);
		}
		else
		{
			StepCompleted(6, false, failureMsg);
		}
	}

	// --- Step 7: Resets unpause after backend reset work ---
	if (allPassed)
	{
		bool previousAlwaysUnpause = c64SettingsAlwaysUnpauseEmulationAfterReset;
		c64SettingsAlwaysUnpauseEmulationAfterReset = true;

		CTestResetPausingDebugInterface resetPausingDebugInterface(viewC64);
		resetPausingDebugInterface.SetDebugMode(DEBUGGER_MODE_RUNNING);
		viewC64->debugInterfaces.push_back(&resetPausingDebugInterface);

		viewC64->ResetHard();
		uint8 hardResetDebugMode = resetPausingDebugInterface.GetDebugMode();
		bool hardResetRunning = (hardResetDebugMode == DEBUGGER_MODE_RUNNING);

		resetPausingDebugInterface.SetDebugMode(DEBUGGER_MODE_RUNNING);
		viewC64->ResetSoft();
		uint8 softResetDebugMode = resetPausingDebugInterface.GetDebugMode();
		bool softResetRunning = (softResetDebugMode == DEBUGGER_MODE_RUNNING);

		c64SettingsAlwaysUnpauseEmulationAfterReset = false;
		resetPausingDebugInterface.SetDebugMode(DEBUGGER_MODE_RUNNING);
		viewC64->ResetHard();
		uint8 hardResetPausedMode = resetPausingDebugInterface.GetDebugMode();
		bool hardResetPaused = (hardResetPausedMode == DEBUGGER_MODE_PAUSED);

		resetPausingDebugInterface.SetDebugMode(DEBUGGER_MODE_RUNNING);
		viewC64->ResetSoft();
		uint8 softResetPausedMode = resetPausingDebugInterface.GetDebugMode();
		bool softResetPaused = (softResetPausedMode == DEBUGGER_MODE_PAUSED);

		viewC64->debugInterfaces.erase(std::remove(viewC64->debugInterfaces.begin(),
											viewC64->debugInterfaces.end(),
											&resetPausingDebugInterface),
								 viewC64->debugInterfaces.end());
		c64SettingsAlwaysUnpauseEmulationAfterReset = previousAlwaysUnpause;

		if (!hardResetRunning)
		{
			sprintf(failureMsg, "Hard reset left debug mode %d instead of RUNNING after backend reset",
					hardResetDebugMode);
			allPassed = false;
		}
		else if (!softResetRunning)
		{
			sprintf(failureMsg, "Soft reset left debug mode %d instead of RUNNING after backend reset",
					softResetDebugMode);
			allPassed = false;
		}
		else if (!hardResetPaused)
		{
			sprintf(failureMsg, "Hard reset with auto-unpause disabled left debug mode %d instead of PAUSED",
					hardResetPausedMode);
			allPassed = false;
		}
		else if (!softResetPaused)
		{
			sprintf(failureMsg, "Soft reset with auto-unpause disabled left debug mode %d instead of PAUSED",
					softResetPausedMode);
			allPassed = false;
		}

		if (allPassed)
		{
			StepCompleted(7, true, "Hard/soft reset respect auto-unpause setting after backend reset");
		}
		else
		{
			StepCompleted(7, false, failureMsg);
		}
	}

	// --- Step 8: Rapid hard resets do not leave C64 paused or jammed ---
	if (allPassed)
	{
		bool previousAlwaysUnpause = c64SettingsAlwaysUnpauseEmulationAfterReset;
		c64SettingsAlwaysUnpauseEmulationAfterReset = true;

		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		SYS_Sleep(100);
		unsigned int beforeResetFrames = di->GetEmulationFrameNumber();
		std::atomic<bool> stopSampling(false);
		std::atomic<bool> observedPause(false);
		std::atomic<int> observedPausePc(0);
		std::atomic<int> observedPauseJam(0);
		std::thread sampler([&]() {
			while (!stopSampling.load(std::memory_order_acquire))
			{
				if (di->GetDebugMode() == DEBUGGER_MODE_PAUSED)
				{
					observedPause.store(true, std::memory_order_release);
					observedPausePc.store(di->GetCpuPC(), std::memory_order_release);
					observedPauseJam.store(di->IsCpuJam() ? 1 : 0, std::memory_order_release);
					break;
				}
				std::this_thread::yield();
			}
		});
		for (int i = 0; i < 80; i++)
		{
			viewC64->ResetHard();
			SYS_Sleep(15);
		}
		stopSampling.store(true, std::memory_order_release);
		sampler.join();

		bool framesAdvanced = WaitForFrameAdvance(di, 2, 1500);
		uint8 resetStressDebugMode = di->GetDebugMode();
		bool resetStressJammed = di->IsCpuJam();
		int resetStressPc = di->GetCpuPC();
		unsigned int afterResetFrames = di->GetEmulationFrameNumber();

		c64SettingsAlwaysUnpauseEmulationAfterReset = previousAlwaysUnpause;

		if (observedPause.load(std::memory_order_acquire))
		{
			sprintf(failureMsg, "Rapid hard reset transiently paused at PC=$%04x, jam=%d",
					observedPausePc.load(std::memory_order_acquire),
					observedPauseJam.load(std::memory_order_acquire));
			allPassed = false;
		}
		else if (resetStressDebugMode != DEBUGGER_MODE_RUNNING)
		{
			sprintf(failureMsg, "Rapid hard reset left debug mode %d at PC=$%04x, jam=%d",
					resetStressDebugMode, resetStressPc, resetStressJammed ? 1 : 0);
			allPassed = false;
		}
		else if (resetStressJammed)
		{
			sprintf(failureMsg, "Rapid hard reset left CPU jammed at PC=$%04x", resetStressPc);
			allPassed = false;
		}
		else if (!framesAdvanced)
		{
			sprintf(failureMsg, "Rapid hard reset did not resume frames: before=%u after=%u mode=%d PC=$%04x",
					beforeResetFrames, afterResetFrames, resetStressDebugMode, resetStressPc);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[160];
			sprintf(msg, "Rapid hard reset resumed: frames %u -> %u PC=$%04x",
					beforeResetFrames, afterResetFrames, resetStressPc);
			StepCompleted(8, true, msg);
		}
		else
		{
			StepCompleted(8, false, failureMsg);
		}
	}

	// --- Step 9: C64 menu stop/start resynchronizes frame pacing ---
	if (allPassed)
	{
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);
		di->WaitCpuDebugInterruptTasksApplied();
		// Let the preceding reset stress settle. Use the same settling period
		// after restart and a longer sample to reduce frame-boundary noise.
		SYS_Sleep(300);
		FramePacingSample baseline = CountFramesFor(di, 2000);

		viewC64->StopEmulationThread(di);
		SYS_Sleep(6000);
		viewC64->StartEmulationThread(di);
		di->SetDebugMode(DEBUGGER_MODE_RUNNING);

		FramePacingSample resumed;
		bool restarted = WaitForFrameAdvance(di, 3, 1500);
		if (restarted)
		{
			SYS_Sleep(300);
			resumed = CountFramesFor(di, 2000);
		}
		di->PauseEmulationBlockedWait();

		char measurements[256];
		snprintf(measurements, sizeof(measurements),
				 "baseline=%u frames/%.1fms (%.2fHz), resumed=%u frames/%.1fms (%.2fHz)",
				 baseline.frames, baseline.elapsedMs, baseline.FramesPerSecond(),
				 resumed.frames, resumed.elapsedMs, resumed.FramesPerSecond());

		if (baseline.counterReset || resumed.counterReset)
		{
			snprintf(failureMsg, sizeof(failureMsg), "Frame counter reset during pacing sample: %s", measurements);
			allPassed = false;
		}
		else if (baseline.FramesPerSecond() < 20.0 * 1000.0 / 700.0)
		{
			// Preserve the original minimum (20 frames/700ms), as a rate.
			snprintf(failureMsg, sizeof(failureMsg), "Baseline frame pacing too low before restart: %s", measurements);
			allPassed = false;
		}
		else if (!restarted)
		{
			sprintf(failureMsg, "C64 did not advance frames after stop/start within 1500ms");
			allPassed = false;
		}
		else if (!FramePacingRecovered(baseline, resumed))
		{
			snprintf(failureMsg, sizeof(failureMsg), "Frame pacing did not recover after C64 stop/start: %s", measurements);
			allPassed = false;
		}

		if (allPassed)
		{
			char msg[320];
			snprintf(msg, sizeof(msg), "C64 stop/start frame pacing recovered: %s", measurements);
			StepCompleted(9, true, msg);
		}
		else
		{
			StepCompleted(9, false, failureMsg);
		}
	}

	// --- Step 10: Sound system not crashed ---
	if (allPassed)
	{
		// Sound pause/resume should not crash
		c64d_sound_pause();
		SYS_Sleep(50);
		c64d_sound_resume();
		SYS_Sleep(50);

		StepCompleted(10, true, "Sound pause/resume executed without crash");
	}

	// Restore emulator state
	if (!wasRunning)
		viewC64->StopEmulationThread(di);

	if (allPassed)
		TestCompleted(true, "Platform abstraction verified: FPS, frames, screen, debug control, reset unpause, stop/start sync, sound");
	else
		TestCompleted(false, failureMsg);
#endif
}

void CTestVicePlatformAbstraction::Cancel()
{
	isRunning = false;
}
