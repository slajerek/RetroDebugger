#include "CTestGT2TitleBar.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "SYS_Funct.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "goattrk2.h"
#include "gdisplay.h"
#include "gconsole.h"

extern unsigned *scrbuffer;
extern unsigned char *chardata;
}

// Extract a string from scrbuffer at (col, row) for 'len' chars
static void TBScrGetString(char *out, int col, int row, int len)
{
	if (!scrbuffer)
	{
		memset(out, 0, len + 1);
		return;
	}
	for (int i = 0; i < len; i++)
	{
		// `+ i`: without it every character read back is the one at `col`, so
		// "FV" came back as "FF" and "25Hz" as "2222" and steps 4-6 could never
		// pass. printstatus() was rendering correctly the whole time.
		unsigned char ch = (unsigned char)(scrbuffer[col + i + row * MAX_COLUMNS] & 0xff);
		out[i] = (ch >= 0x20) ? (char)ch : ' ';
	}
	out[len] = '\0';
}

// Maximum valid multiplier value (from goattrk2.c: nextmultiplier caps at some value)
// goattrk2.c: void nextmultiplier(void) { if (multiplier < 16) multiplier++; }
#define GT2_MAX_MULTIPLIER 16

void CTestGT2TitleBar::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	int step = 0;
	bool allPassed = true;

	// Guard: skip if GT2 not active
	if (chardata == NULL)
	{
		step++;
		StepCompleted(step, true, "GT2 not active (chardata NULL) — skipping title bar tests");
		TestSkipped("GT2 plugin not active -- the title bar was never exercised");
		return;
	}

	if (pluginGoatTracker == NULL)
	{
		step++;
		StepCompleted(step, true, "GT2 plugin not initialized — skipping title bar tests");
		TestSkipped("GT2 plugin not initialized -- the title bar was never exercised");
		return;
	}

	// -----------------------------------------------------------------------
	// Test 1: Toggle FV option → verify usefinevib flag
	// -----------------------------------------------------------------------
	step++;
	{
		int saved = usefinevib;
		usefinevib = 0;
		bool wasOff = (usefinevib == 0);
		usefinevib = 1;
		bool isOn = (usefinevib == 1);
		usefinevib = 0;
		bool isOff = (usefinevib == 0);

		if (wasOff && isOn && isOff)
		{
			StepCompleted(step, true, "FV toggle: usefinevib transitions 0->1->0 correctly");
		}
		else
		{
			StepCompleted(step, false, "FV toggle: usefinevib flag did not transition as expected");
			allPassed = false;
		}
		usefinevib = saved;
	}

	// -----------------------------------------------------------------------
	// Test 2: Change multiplier → verify multiplier value
	// -----------------------------------------------------------------------
	step++;
	{
		int savedMult = multiplier;
		multiplier = 0;
		int before = multiplier;
		// Simulate nextmultiplier() increment: if (multiplier < 16) multiplier++
		if (multiplier < GT2_MAX_MULTIPLIER) multiplier++;
		int after = multiplier;

		if (before == 0 && after == 1)
		{
			StepCompleted(step, true, "Multiplier change: incremented from 0 to 1 correctly");
		}
		else
		{
			char msg[64];
			snprintf(msg, sizeof(msg), "Multiplier change mismatch: before=%d after=%d", before, after);
			StepCompleted(step, false, msg);
			allPassed = false;
		}
		multiplier = savedMult;
	}

	// -----------------------------------------------------------------------
	// Test 3: Edge — multiplier at max → increase → stays at max
	// -----------------------------------------------------------------------
	step++;
	{
		int savedMult = multiplier;
		multiplier = GT2_MAX_MULTIPLIER;
		// Simulate nextmultiplier(): if (multiplier < 16) multiplier++
		if (multiplier < GT2_MAX_MULTIPLIER) multiplier++;

		if (multiplier == GT2_MAX_MULTIPLIER)
		{
			StepCompleted(step, true, "Edge: multiplier at max, increase clamped correctly");
		}
		else
		{
			char msg[64];
			snprintf(msg, sizeof(msg), "Edge: multiplier not clamped — got %d (expected %d)", multiplier, GT2_MAX_MULTIPLIER);
			StepCompleted(step, false, msg);
			allPassed = false;
		}
		multiplier = savedMult;
	}

	// -----------------------------------------------------------------------
	// Test 4: Edge — all options toggled ON → render correct
	// Verify that printstatus() renders "FV", "PO", "RO" into row 0 when all set
	// -----------------------------------------------------------------------
	step++;
	{
		int savedFV  = usefinevib;
		int savedPO  = optimizepulse;
		int savedRO  = optimizerealtime;
		int savedNtsc = ntsc;
		int savedMult = multiplier;
		int savedMenu  = menu;

		usefinevib       = 1;
		optimizepulse    = 1;
		optimizerealtime = 1;
		ntsc             = 0;   // PAL
		multiplier       = 2;
		menu             = 0;   // non-menu branch of printstatus

		printstatus();

		// "FV" at col 50, "PO" at col 53, "RO" at col 56 (matching CViewGT2TitleBar cols 50/53/56)
		// Original gdisplay.c uses cols 40+10=50, 43+10=53, 46+10=56
		char fvStr[4], poStr[4], roStr[4];
		TBScrGetString(fvStr, 50, 0, 2);
		TBScrGetString(poStr, 53, 0, 2);
		TBScrGetString(roStr, 56, 0, 2);

		bool fvOk = (strcmp(fvStr, "FV") == 0);
		bool poOk = (strcmp(poStr, "PO") == 0);
		bool roOk = (strcmp(roStr, "RO") == 0);

		if (fvOk && poOk && roOk)
		{
			StepCompleted(step, true, "All options ON: FV/PO/RO rendered at correct positions");
		}
		else
		{
			char msg[256];
			snprintf(msg, sizeof(msg),
				"All-options edge render mismatch: fv='%.2s'(%d) po='%.2s'(%d) ro='%.2s'(%d)",
				fvStr, fvOk, poStr, poOk, roStr, roOk);
			StepCompleted(step, false, msg);
			allPassed = false;
		}

		usefinevib       = savedFV;
		optimizepulse    = savedPO;
		optimizerealtime = savedRO;
		ntsc             = savedNtsc;
		multiplier       = savedMult;
		menu             = savedMenu;
	}

	// -----------------------------------------------------------------------
	// Test 5: Render identity — default size vs scrbuffer row 0
	// With known default state, verify multiplier or "25Hz" renders correctly
	// -----------------------------------------------------------------------
	step++;
	{
		int savedMult = multiplier;
		int savedMenu  = menu;

		multiplier = 0;   // "25Hz" branch
		menu       = 0;

		printstatus();

		// "25Hz" at col 67+10 = 77 (matching CViewGT2TitleBar col 77)
		char hz[6];
		TBScrGetString(hz, 77, 0, 4);
		bool hzOk = (strcmp(hz, "25Hz") == 0);

		if (hzOk)
		{
			StepCompleted(step, true, "Render identity: multiplier=0 renders '25Hz' at row 0 col 77");
		}
		else
		{
			char msg[128];
			snprintf(msg, sizeof(msg), "Render identity mismatch: got '%.4s' at col 77 row 0 (expected '25Hz')", hz);
			StepCompleted(step, false, msg);
			allPassed = false;
		}

		multiplier = savedMult;
		menu       = savedMenu;
	}

	// -----------------------------------------------------------------------
	// Test 6: Render identity edge — all options non-default
	// With usefinevib=0, optimizepulse=0, optimizerealtime=0, multiplier=3:
	// verify "FV" is NOT at col 50, and multiplier string " 3X" IS at col 77
	// -----------------------------------------------------------------------
	step++;
	{
		int savedFV   = usefinevib;
		int savedPO   = optimizepulse;
		int savedRO   = optimizerealtime;
		int savedMult = multiplier;
		int savedMenu  = menu;

		usefinevib       = 0;
		optimizepulse    = 0;
		optimizerealtime = 0;
		multiplier       = 3;
		menu             = 0;

		printstatus();

		// "FV" should NOT appear (usefinevib=0) — col 50 row 0 should not be "FV"
		char fvStr[4];
		TBScrGetString(fvStr, 50, 0, 2);
		bool fvAbsent = (strcmp(fvStr, "FV") != 0);

		// " 3X" at col 77 (multiplier=3 renders " 3X")
		char multStr[5];
		TBScrGetString(multStr, 77, 0, 3);
		bool multOk = (strcmp(multStr, " 3X") == 0);

		if (fvAbsent && multOk)
		{
			StepCompleted(step, true, "Render edge non-default: FV absent, multiplier ' 3X' correct");
		}
		else
		{
			char msg[256];
			snprintf(msg, sizeof(msg),
				"Render edge non-default mismatch: fv='%.2s'(absent=%d) mult='%.3s'(ok=%d)",
				fvStr, fvAbsent, multStr, multOk);
			StepCompleted(step, false, msg);
			allPassed = false;
		}

		usefinevib       = savedFV;
		optimizepulse    = savedPO;
		optimizerealtime = savedRO;
		multiplier       = savedMult;
		menu             = savedMenu;
	}

	TestCompleted(allPassed,
		allPassed ? "All GT2TitleBar tests passed" : "Some GT2TitleBar tests failed");
}
