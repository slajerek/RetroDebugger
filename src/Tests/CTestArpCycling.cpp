#include "CTestArpCycling.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gplay.h"
#include "gsong.h"
}

static char failureMsg[512];

// rebuildarp is declared in gplay.h

void CTestArpCycling::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	failureMsg[0] = '\0';

	CHN testchn;
	memset(&testchn, 0, sizeof(testchn));

	// Enable arp columns for testing
	int savedArpCols = numarpcolumns;
	numarpcolumns = 4;

	// --- Test 1: No arp data → arpcount should be 0 ---
	testchn.gate = 0xff;
	testchn.note = 0;
	testchn.newnote = 0;
	memset(testchn.arpcolnotes, 0, sizeof(testchn.arpcolnotes));
	rebuildarp(&testchn);

	if (testchn.arpcount != 0)
	{
		sprintf(failureMsg, "Test 1 FAIL: expected arpcount=0, got %d", testchn.arpcount);
		TestCompleted(false, failureMsg);
		return;
	}

	// --- Test 2: Base note only → arpcount should be 1 (no cycling) ---
	testchn.note = 24; // C-3
	testchn.newnote = 0;
	testchn.gate = 0xff;
	rebuildarp(&testchn);

	if (testchn.arpcount != 1)
	{
		sprintf(failureMsg, "Test 2 FAIL: expected arpcount=1, got %d", testchn.arpcount);
		TestCompleted(false, failureMsg);
		return;
	}
	if (testchn.arpnotes[0] != 24)
	{
		sprintf(failureMsg, "Test 2 FAIL: expected arpnotes[0]=24, got %d", testchn.arpnotes[0]);
		TestCompleted(false, failureMsg);
		return;
	}

	// --- Test 3: Base + 2 arp columns → arpcount=3, cycling ---
	testchn.note = 24; // C-3
	testchn.arpcolnotes[0] = 28; // E-3
	testchn.arpcolnotes[1] = 31; // G-3
	testchn.gate = 0xff;
	rebuildarp(&testchn);

	if (testchn.arpcount != 3)
	{
		sprintf(failureMsg, "Test 3 FAIL: expected arpcount=3, got %d", testchn.arpcount);
		TestCompleted(false, failureMsg);
		return;
	}
	if (testchn.arpnotes[0] != 24 || testchn.arpnotes[1] != 28 || testchn.arpnotes[2] != 31)
	{
		sprintf(failureMsg, "Test 3 FAIL: notes [%d,%d,%d] expected [24,28,31]",
				testchn.arpnotes[0], testchn.arpnotes[1], testchn.arpnotes[2]);
		TestCompleted(false, failureMsg);
		return;
	}

	// --- Test 4a: KEYOFF (gate off) keeps the base in the cycle ---
	// The gate belongs to the main track and the arp cycle ignores it:
	// after a KEYOFF the chord keeps cycling through the release.
	testchn.gate = 0xfe; // gate off
	testchn.note = 24;
	testchn.newnote = 0;
	rebuildarp(&testchn);

	if (testchn.arpcount != 3 || !testchn.arpbase)
	{
		sprintf(failureMsg, "Test 4a FAIL: expected arpcount=3 arpbase=1 with gate off, got %d/%d",
				testchn.arpcount, testchn.arpbase);
		TestCompleted(false, failureMsg);
		return;
	}
	if (testchn.arpnotes[0] != 24 || testchn.arpnotes[1] != 28 || testchn.arpnotes[2] != 31)
	{
		sprintf(failureMsg, "Test 4a FAIL: notes [%d,%d,%d] expected [24,28,31]",
				testchn.arpnotes[0], testchn.arpnotes[1], testchn.arpnotes[2]);
		TestCompleted(false, failureMsg);
		return;
	}

	// --- Test 4b: No base note at all, arp columns remain → arpcount=2 ---
	testchn.note = 0;
	testchn.newnote = 0;
	rebuildarp(&testchn);

	if (testchn.arpcount != 2 || testchn.arpbase)
	{
		sprintf(failureMsg, "Test 4b FAIL: expected arpcount=2 arpbase=0, got %d/%d",
				testchn.arpcount, testchn.arpbase);
		TestCompleted(false, failureMsg);
		return;
	}
	if (testchn.arpnotes[0] != 28 || testchn.arpnotes[1] != 31)
	{
		sprintf(failureMsg, "Test 4b FAIL: notes [%d,%d] expected [28,31]",
				testchn.arpnotes[0], testchn.arpnotes[1]);
		TestCompleted(false, failureMsg);
		return;
	}

	// --- Test 4c: pending newnote replaces the base immediately ---
	testchn.note = 24;
	testchn.newnote = FIRSTNOTE + 36;
	rebuildarp(&testchn);
	if (testchn.arpcount != 3 || testchn.arpnotes[0] != 36)
	{
		sprintf(failureMsg, "Test 4c FAIL: expected [36,28,31], got count=%d first=%d",
				testchn.arpcount, testchn.arpnotes[0]);
		TestCompleted(false, failureMsg);
		return;
	}
	testchn.note = 0;
	testchn.newnote = 0;
	rebuildarp(&testchn);

	// --- Test 5: Cycling position wraps ---
	testchn.arppos = 0;
	// Simulate 5 ticks of cycling
	unsigned char seq[5];
	for (int i = 0; i < 5; i++)
	{
		seq[i] = testchn.arpnotes[testchn.arppos];
		testchn.arppos++;
		if (testchn.arppos >= testchn.arpcount)
			testchn.arppos = 0;
	}
	// Expected: 28, 31, 28, 31, 28
	if (seq[0] != 28 || seq[1] != 31 || seq[2] != 28 || seq[3] != 31 || seq[4] != 28)
	{
		sprintf(failureMsg, "Test 5 FAIL: cycling [%d,%d,%d,%d,%d] expected [28,31,28,31,28]",
				seq[0], seq[1], seq[2], seq[3], seq[4]);
		TestCompleted(false, failureMsg);
		return;
	}

	// --- Test 6: All arp columns OFF → arpcount=0 ---
	testchn.arpcolnotes[0] = 0;
	testchn.arpcolnotes[1] = 0;
	testchn.gate = 0xfe;
	rebuildarp(&testchn);

	if (testchn.arpcount != 0)
	{
		sprintf(failureMsg, "Test 6 FAIL: expected arpcount=0, got %d", testchn.arpcount);
		TestCompleted(false, failureMsg);
		return;
	}

	numarpcolumns = savedArpCols;  // Restore
	TestCompleted(true, "All arp cycling tests passed (8/8)");
}

void CTestArpCycling::Cancel()
{
	isRunning = false;
}
