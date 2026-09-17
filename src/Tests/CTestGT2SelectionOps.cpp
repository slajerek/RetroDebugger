#include "CTestGT2SelectionOps.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2PatternList.h"
#include "CGT2RenoiseInput.h"
#include "SYS_Main.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
extern int epnum[MAX_CHN];
extern int eppos, epchn;
extern int esnum, eseditpos, editmode;
extern int defaultpatternlength;
}
extern int numarpcolumns;
extern bool gt2RenoiseBulkPatternNumberChange;

void CTestGT2SelectionOps::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	int step = 0;

	if (pluginGoatTracker == NULL || pluginGoatTracker->viewPatterns == NULL)
	{
		step++;
		StepCompleted(step, true, "GT2 plugin/viewPatterns not available - skipped");
		TestSkipped("GT2 plugin not available -- selection operations were never exercised");
		return;
	}
	CViewGT2Patterns *vp = pluginGoatTracker->viewPatterns;

	// Save state.
	int savedEpchn = epchn, savedEppos = eppos, savedEparpcol = vp->eparpcol;
	int savedEpnum0 = epnum[0], savedPattlen0 = pattlen[0], savedNumArp = numarpcolumns;
	unsigned char savedPat[MAX_PATTROWS * 4 + 4];
	memcpy(savedPat, pattern[0], sizeof(savedPat));

	auto restoreState = [&]() {
		memcpy(pattern[0], savedPat, sizeof(savedPat));
		pattlen[0] = savedPattlen0;
		epnum[0] = savedEpnum0;
		epchn = savedEpchn;
		eppos = savedEppos;
		vp->eparpcol = savedEparpcol;
		numarpcolumns = savedNumArp;
		vp->ClearPatternSelection();
		vp->ClearPatternUndoHistory();   // also clears the channel-row spill stash
		countpatternlengths();
	};

	// Setup: pattern 0 = 8 REST rows + ENDPATT, channel 0, no arp columns.
	numarpcolumns = 0;
	epchn = 0;
	epnum[0] = 0;
	vp->eparpcol = -1;
	memset(pattern[0], 0, sizeof(savedPat));
	for (int r = 0; r < 8; r++)
		pattern[0][r * 4] = REST;
	pattern[0][8 * 4] = ENDPATT;
	countpatternlengths();

	// --- Test 1: no selection -> transpose only the cursor cell ---
	step++;
	{
		pattern[0][3 * 4] = (unsigned char)(FIRSTNOTE + 20);
		pattern[0][4 * 4] = (unsigned char)(FIRSTNOTE + 30);
		eppos = 3;
		vp->ClearPatternSelection();
		bool rc = vp->TransposeAtCursor(1);
		bool ok = rc
			&& pattern[0][3 * 4] == (unsigned char)(FIRSTNOTE + 21)
			&& pattern[0][4 * 4] == (unsigned char)(FIRSTNOTE + 30);
		char msg[160];
		snprintf(msg, sizeof(msg), "no-sel transpose: rc=%d row3=+%d(exp 21) row4=+%d(exp 30)",
			(int)rc, pattern[0][3 * 4] - FIRSTNOTE, pattern[0][4 * 4] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 2: selection -> transpose the whole selected range ---
	step++;
	{
		pattern[0][3 * 4] = (unsigned char)(FIRSTNOTE + 20);
		pattern[0][4 * 4] = (unsigned char)(FIRSTNOTE + 30);
		int track = vp->GetPatternTrackIndex(0, -1);
		vp->BeginMousePatternSelection(track, 3);
		vp->UpdateMousePatternSelection(track, 4);
		bool rc = vp->TransposeAtCursor(12);
		bool ok = rc
			&& pattern[0][3 * 4] == (unsigned char)(FIRSTNOTE + 32)
			&& pattern[0][4 * 4] == (unsigned char)(FIRSTNOTE + 42);
		char msg[160];
		snprintf(msg, sizeof(msg), "sel transpose: rc=%d row3=+%d(exp 32) row4=+%d(exp 42)",
			(int)rc, pattern[0][3 * 4] - FIRSTNOTE, pattern[0][4 * 4] - FIRSTNOTE);
		vp->ClearPatternSelection();
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 3: no selection -> erase only the cursor cell ---
	step++;
	{
		pattern[0][3 * 4] = (unsigned char)(FIRSTNOTE + 20);
		pattern[0][4 * 4] = (unsigned char)(FIRSTNOTE + 30);
		eppos = 3;
		vp->ClearPatternSelection();
		bool rc = vp->EraseAtCursor();
		bool ok = rc
			&& pattern[0][3 * 4] == (unsigned char)REST
			&& pattern[0][4 * 4] == (unsigned char)(FIRSTNOTE + 30);
		char msg[160];
		snprintf(msg, sizeof(msg), "no-sel erase: rc=%d row3=0x%02X(exp REST) row4=+%d(exp 30)",
			(int)rc, pattern[0][3 * 4], pattern[0][4 * 4] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 4: no selection -> cut clears the cursor cell ---
	step++;
	{
		pattern[0][3 * 4] = (unsigned char)(FIRSTNOTE + 20);
		eppos = 3;
		vp->ClearPatternSelection();
		bool rc = vp->CutAtCursor();
		bool ok = rc && pattern[0][3 * 4] == (unsigned char)REST;
		char msg[160];
		snprintf(msg, sizeof(msg), "no-sel cut: rc=%d row3=0x%02X(exp REST)",
			(int)rc, pattern[0][3 * 4]);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 5: shrink halves the block, keeping every 2nd row ---
	step++;
	{
		for (int i = 0; i < 5; i++)
			pattern[0][i * 4] = (unsigned char)(FIRSTNOTE + i);
		int track = vp->GetPatternTrackIndex(0, -1);
		vp->BeginMousePatternSelection(track, 0);
		vp->UpdateMousePatternSelection(track, 4);
		bool rc = vp->ShrinkPatternSelection();
		bool ok = rc
			&& pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 0)
			&& pattern[0][1 * 4] == (unsigned char)(FIRSTNOTE + 2)
			&& pattern[0][2 * 4] == (unsigned char)(FIRSTNOTE + 4)
			&& pattern[0][3 * 4] == (unsigned char)REST
			&& pattern[0][4 * 4] == (unsigned char)REST;
		char msg[200];
		snprintf(msg, sizeof(msg),
			"shrink: rc=%d r0=+%d r1=+%d(exp 2) r2=+%d(exp 4) r3=0x%02X r4=0x%02X",
			(int)rc, pattern[0][0] - FIRSTNOTE, pattern[0][4] - FIRSTNOTE,
			pattern[0][8] - FIRSTNOTE, pattern[0][12], pattern[0][16]);
		vp->ClearPatternSelection();
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 6: expand doubles the block spacing ---
	step++;
	{
		for (int i = 0; i < 8; i++) pattern[0][i * 4] = REST;
		pattern[0][0 * 4] = (unsigned char)(FIRSTNOTE + 10);
		pattern[0][1 * 4] = (unsigned char)(FIRSTNOTE + 11);
		pattern[0][2 * 4] = (unsigned char)(FIRSTNOTE + 12);
		int track = vp->GetPatternTrackIndex(0, -1);
		vp->BeginMousePatternSelection(track, 0);
		vp->UpdateMousePatternSelection(track, 2);
		bool rc = vp->ExpandPatternSelection();
		bool ok = rc
			&& pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 10)
			&& pattern[0][1 * 4] == (unsigned char)REST
			&& pattern[0][2 * 4] == (unsigned char)(FIRSTNOTE + 11)
			&& pattern[0][3 * 4] == (unsigned char)REST
			&& pattern[0][4 * 4] == (unsigned char)(FIRSTNOTE + 12);
		char msg[200];
		snprintf(msg, sizeof(msg),
			"expand: rc=%d r0=+%d r1=0x%02X r2=+%d(exp 11) r3=0x%02X r4=+%d(exp 12)",
			(int)rc, pattern[0][0] - FIRSTNOTE, pattern[0][4],
			pattern[0][8] - FIRSTNOTE, pattern[0][12], pattern[0][16] - FIRSTNOTE);
		vp->ClearPatternSelection();
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 7: expand clamps at the pattern end (overflow dropped) ---
	step++;
	{
		for (int i = 0; i < 8; i++) pattern[0][i * 4] = REST;
		pattern[0][5 * 4] = (unsigned char)(FIRSTNOTE + 20);
		pattern[0][6 * 4] = (unsigned char)(FIRSTNOTE + 21);
		pattern[0][7 * 4] = (unsigned char)(FIRSTNOTE + 22);
		int track = vp->GetPatternTrackIndex(0, -1);
		vp->BeginMousePatternSelection(track, 5);
		vp->UpdateMousePatternSelection(track, 7);
		bool rc = vp->ExpandPatternSelection();
		bool ok = rc
			&& pattern[0][5 * 4] == (unsigned char)(FIRSTNOTE + 20)
			&& pattern[0][6 * 4] == (unsigned char)REST
			&& pattern[0][7 * 4] == (unsigned char)(FIRSTNOTE + 21);
		char msg[200];
		snprintf(msg, sizeof(msg),
			"expand clamp: rc=%d r5=+%d r6=0x%02X r7=+%d(exp 21, +22 dropped)",
			(int)rc, pattern[0][20] - FIRSTNOTE, pattern[0][24], pattern[0][28] - FIRSTNOTE);
		vp->ClearPatternSelection();
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 8: track copy/paste use a clipboard separate from Ctrl+C ---
	step++;
	{
		for (int i = 0; i < 8; i++)
			pattern[0][i * 4] = (unsigned char)(FIRSTNOTE + 40 + i);
		bool rcCopy = vp->CopyTrack();           // track clipboard <- whole track
		// Put something else in the cell/selection clipboard.
		eppos = 0;
		vp->ClearPatternSelection();
		vp->CopyAtCursor();                      // selection clipboard <- one cell
		// Wipe the track, then paste from the track clipboard.
		for (int i = 0; i < 8; i++)
			pattern[0][i * 4] = REST;
		bool rcPaste = vp->PasteTrack();
		bool ok = rcCopy && rcPaste;
		for (int i = 0; i < 8; i++)
			ok = ok && pattern[0][i * 4] == (unsigned char)(FIRSTNOTE + 40 + i);
		char msg[200];
		snprintf(msg, sizeof(msg),
			"track copy/paste: rcCopy=%d rcPaste=%d r0=+%d r7=+%d (exp 40,47)",
			(int)rcCopy, (int)rcPaste, pattern[0][0] - FIRSTNOTE, pattern[0][28] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 9: transpose the whole track, REST rows untouched ---
	step++;
	{
		for (int i = 0; i < 8; i++) pattern[0][i * 4] = REST;
		pattern[0][0 * 4] = (unsigned char)(FIRSTNOTE + 10);
		pattern[0][7 * 4] = (unsigned char)(FIRSTNOTE + 20);
		bool rc = vp->TransposeTrack(5);
		bool ok = rc
			&& pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 15)
			&& pattern[0][3 * 4] == (unsigned char)REST
			&& pattern[0][7 * 4] == (unsigned char)(FIRSTNOTE + 25);
		char msg[200];
		snprintf(msg, sizeof(msg),
			"track transpose: rc=%d r0=+%d(exp 15) r3=0x%02X(REST) r7=+%d(exp 25)",
			(int)rc, pattern[0][0] - FIRSTNOTE, pattern[0][12], pattern[0][28] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 10: cut clears the whole track ---
	step++;
	{
		for (int i = 0; i < 8; i++)
			pattern[0][i * 4] = (unsigned char)(FIRSTNOTE + 50 + i);
		bool rc = vp->CutTrack();
		bool ok = rc;
		for (int i = 0; i < 8; i++)
			ok = ok && pattern[0][i * 4] == (unsigned char)REST;
		char msg[200];
		snprintf(msg, sizeof(msg), "track cut: rc=%d r0=0x%02X r7=0x%02X (exp all REST)",
			(int)rc, pattern[0][0], pattern[0][28]);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 11: shrink the whole track (pattlen 8 -> keep rows 0,2,4,6) ---
	step++;
	{
		for (int i = 0; i < 8; i++)
			pattern[0][i * 4] = (unsigned char)(FIRSTNOTE + i);
		bool rc = vp->ShrinkTrack();
		bool ok = rc
			&& pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 0)
			&& pattern[0][1 * 4] == (unsigned char)(FIRSTNOTE + 2)
			&& pattern[0][2 * 4] == (unsigned char)(FIRSTNOTE + 4)
			&& pattern[0][3 * 4] == (unsigned char)(FIRSTNOTE + 6)
			&& pattern[0][4 * 4] == (unsigned char)REST
			&& pattern[0][7 * 4] == (unsigned char)REST;
		char msg[200];
		snprintf(msg, sizeof(msg),
			"track shrink: rc=%d r0=+%d r1=+%d(2) r2=+%d(4) r3=+%d(6) r4=0x%02X r7=0x%02X",
			(int)rc, pattern[0][0] - FIRSTNOTE, pattern[0][4] - FIRSTNOTE,
			pattern[0][8] - FIRSTNOTE, pattern[0][12] - FIRSTNOTE, pattern[0][16], pattern[0][28]);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 12: expand the whole track (rows double, tail dropped) ---
	step++;
	{
		for (int i = 0; i < 8; i++) pattern[0][i * 4] = REST;
		pattern[0][0 * 4] = (unsigned char)(FIRSTNOTE + 30);
		pattern[0][1 * 4] = (unsigned char)(FIRSTNOTE + 31);
		pattern[0][2 * 4] = (unsigned char)(FIRSTNOTE + 32);
		bool rc = vp->ExpandTrack();
		bool ok = rc
			&& pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 30)
			&& pattern[0][1 * 4] == (unsigned char)REST
			&& pattern[0][2 * 4] == (unsigned char)(FIRSTNOTE + 31)
			&& pattern[0][3 * 4] == (unsigned char)REST
			&& pattern[0][4 * 4] == (unsigned char)(FIRSTNOTE + 32)
			&& pattern[0][5 * 4] == (unsigned char)REST;
		char msg[200];
		snprintf(msg, sizeof(msg),
			"track expand: rc=%d r0=+%d r1=0x%02X r2=+%d(31) r3=0x%02X r4=+%d(32)",
			(int)rc, pattern[0][0] - FIRSTNOTE, pattern[0][4],
			pattern[0][8] - FIRSTNOTE, pattern[0][12], pattern[0][16] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// === Whole-phrase ops (channels 0, 1, 2) ===
	{
		int savedEpnum1 = epnum[1], savedEpnum2 = epnum[2];
		int savedPattlen1 = pattlen[1], savedPattlen2 = pattlen[2];
		unsigned char savedPat1[MAX_PATTROWS * 4 + 4];
		unsigned char savedPat2[MAX_PATTROWS * 4 + 4];
		memcpy(savedPat1, pattern[1], sizeof(savedPat1));
		memcpy(savedPat2, pattern[2], sizeof(savedPat2));

		auto restorePhrase = [&]() {
			memcpy(pattern[1], savedPat1, sizeof(savedPat1));
			memcpy(pattern[2], savedPat2, sizeof(savedPat2));
			epnum[1] = savedEpnum1;
			epnum[2] = savedEpnum2;
			pattlen[1] = savedPattlen1;
			pattlen[2] = savedPattlen2;
			countpatternlengths();
		};

		// --- Test 13: transpose the whole phrase (3 distinct patterns) ---
		step++;
		{
			epnum[0] = 0; epnum[1] = 1; epnum[2] = 2;
			for (int p = 0; p < 3; p++)
			{
				memset(pattern[p], 0, MAX_PATTROWS * 4 + 4);
				for (int r = 0; r < 8; r++) pattern[p][r * 4] = REST;
				pattern[p][0 * 4] = (unsigned char)(FIRSTNOTE + 10 + p * 10);
				pattern[p][8 * 4] = ENDPATT;
			}
			countpatternlengths();
			bool rc = vp->TransposePhrase(3);
			bool ok = rc
				&& pattern[0][0] == (unsigned char)(FIRSTNOTE + 13)
				&& pattern[1][0] == (unsigned char)(FIRSTNOTE + 23)
				&& pattern[2][0] == (unsigned char)(FIRSTNOTE + 33);
			char msg[200];
			snprintf(msg, sizeof(msg),
				"phrase transpose: rc=%d c0=+%d(13) c1=+%d(23) c2=+%d(33)",
				(int)rc, pattern[0][0] - FIRSTNOTE, pattern[1][0] - FIRSTNOTE,
				pattern[2][0] - FIRSTNOTE);
			StepCompleted(step, ok, msg);
			if (!ok) { restorePhrase(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 14: transpose dedupes channels sharing one pattern ---
		step++;
		{
			epnum[0] = 0; epnum[1] = 0; epnum[2] = 0;   // all share pattern 0
			memset(pattern[0], 0, MAX_PATTROWS * 4 + 4);
			for (int r = 0; r < 8; r++) pattern[0][r * 4] = REST;
			pattern[0][0 * 4] = (unsigned char)(FIRSTNOTE + 50);
			pattern[0][8 * 4] = ENDPATT;
			countpatternlengths();
			bool rc = vp->TransposePhrase(1);
			// Shared pattern transposed once (+51), not three times (+53).
			bool ok = rc && pattern[0][0] == (unsigned char)(FIRSTNOTE + 51);
			char msg[200];
			snprintf(msg, sizeof(msg),
				"phrase transpose dedup: rc=%d c0=+%d (exp 51, not 53)",
				(int)rc, pattern[0][0] - FIRSTNOTE);
			StepCompleted(step, ok, msg);
			if (!ok) { restorePhrase(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 15: phrase copy/paste use a clipboard of their own ---
		step++;
		{
			epnum[0] = 0; epnum[1] = 1; epnum[2] = 2;
			for (int p = 0; p < 3; p++)
			{
				memset(pattern[p], 0, MAX_PATTROWS * 4 + 4);
				for (int r = 0; r < 8; r++)
					pattern[p][r * 4] = (unsigned char)(FIRSTNOTE + 20 + p * 5 + r);
				pattern[p][8 * 4] = ENDPATT;
			}
			countpatternlengths();
			bool rcCopy = vp->CopyPhrase();
			// Put something else in the track clipboard.
			epchn = 0; eppos = 0; vp->eparpcol = -1;
			vp->ClearPatternSelection();
			vp->CopyTrack();
			// Wipe all three patterns, then paste the phrase back.
			for (int p = 0; p < 3; p++)
				for (int r = 0; r < 8; r++) pattern[p][r * 4] = REST;
			bool rcPaste = vp->PastePhrase();
			bool ok = rcCopy && rcPaste;
			for (int p = 0; p < 3; p++)
				for (int r = 0; r < 8; r++)
					ok = ok && pattern[p][r * 4] == (unsigned char)(FIRSTNOTE + 20 + p * 5 + r);
			char msg[200];
			snprintf(msg, sizeof(msg),
				"phrase copy/paste: rcCopy=%d rcPaste=%d c0r0=+%d c2r7=+%d (exp 20,37)",
				(int)rcCopy, (int)rcPaste, pattern[0][0] - FIRSTNOTE,
				pattern[2][28] - FIRSTNOTE);
			StepCompleted(step, ok, msg);
			if (!ok) { restorePhrase(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 16: shrink the whole phrase ---
		step++;
		{
			epnum[0] = 0; epnum[1] = 1; epnum[2] = 2;
			for (int p = 0; p < 3; p++)
			{
				memset(pattern[p], 0, MAX_PATTROWS * 4 + 4);
				for (int r = 0; r < 8; r++)
					pattern[p][r * 4] = (unsigned char)(FIRSTNOTE + p * 10 + r);
				pattern[p][8 * 4] = ENDPATT;
			}
			countpatternlengths();
			bool rc = vp->ShrinkPhrase();
			bool ok = rc;
			for (int p = 0; p < 3; p++)
				ok = ok
					&& pattern[p][0 * 4] == (unsigned char)(FIRSTNOTE + p * 10 + 0)
					&& pattern[p][1 * 4] == (unsigned char)(FIRSTNOTE + p * 10 + 2)
					&& pattern[p][3 * 4] == (unsigned char)(FIRSTNOTE + p * 10 + 6)
					&& pattern[p][4 * 4] == (unsigned char)REST;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"phrase shrink: rc=%d c1 r0=+%d r1=+%d(12) r3=+%d(16) r4=0x%02X",
				(int)rc, pattern[1][0] - FIRSTNOTE, pattern[1][4] - FIRSTNOTE,
				pattern[1][12] - FIRSTNOTE, pattern[1][16]);
			StepCompleted(step, ok, msg);
			if (!ok) { restorePhrase(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 17: expand the whole phrase ---
		step++;
		{
			epnum[0] = 0; epnum[1] = 1; epnum[2] = 2;
			for (int p = 0; p < 3; p++)
			{
				memset(pattern[p], 0, MAX_PATTROWS * 4 + 4);
				for (int r = 0; r < 8; r++) pattern[p][r * 4] = REST;
				pattern[p][0 * 4] = (unsigned char)(FIRSTNOTE + p * 10 + 1);
				pattern[p][1 * 4] = (unsigned char)(FIRSTNOTE + p * 10 + 2);
				pattern[p][8 * 4] = ENDPATT;
			}
			countpatternlengths();
			bool rc = vp->ExpandPhrase();
			bool ok = rc;
			for (int p = 0; p < 3; p++)
				ok = ok
					&& pattern[p][0 * 4] == (unsigned char)(FIRSTNOTE + p * 10 + 1)
					&& pattern[p][1 * 4] == (unsigned char)REST
					&& pattern[p][2 * 4] == (unsigned char)(FIRSTNOTE + p * 10 + 2)
					&& pattern[p][3 * 4] == (unsigned char)REST;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"phrase expand: rc=%d c2 r0=+%d r1=0x%02X r2=+%d r3=0x%02X",
				(int)rc, pattern[2][0] - FIRSTNOTE, pattern[2][4],
				pattern[2][8] - FIRSTNOTE, pattern[2][12]);
			StepCompleted(step, ok, msg);
			if (!ok) { restorePhrase(); restoreState(); TestCompleted(false, msg); return; }
		}

		restorePhrase();
	}

	// --- Test 18: jump to an absolute row, clamped to pattern length ---
	step++;
	{
		epchn = 0; epnum[0] = 0;
		for (int r = 0; r < 8; r++) pattern[0][r * 4] = REST;
		pattern[0][8 * 4] = ENDPATT;
		countpatternlengths();
		eppos = 0;
		vp->JumpToPatternRow(4);
		int afterJump = eppos;
		vp->JumpToPatternRow(999);   // clamps to pattlen
		int afterClamp = eppos;
		bool ok = afterJump == 4 && afterClamp == pattlen[0];
		char msg[160];
		snprintf(msg, sizeof(msg), "jump row: to4=%d(exp 4) to999=%d(exp pattlen %d)",
			afterJump, afterClamp, pattlen[0]);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 19: move to next / previous row with content ---
	step++;
	{
		epchn = 0; epnum[0] = 0;
		for (int r = 0; r < 8; r++) pattern[0][r * 4] = REST;
		pattern[0][8 * 4] = ENDPATT;
		pattern[0][2 * 4] = (unsigned char)(FIRSTNOTE + 10);   // note on row 2
		pattern[0][5 * 4 + 1] = 0x07;                          // instrument byte on row 5
		countpatternlengths();
		eppos = 3;
		vp->MoveToRowWithNote(1);
		int next = eppos;          // expect 5 (instrument byte counts as content)
		eppos = 3;
		vp->MoveToRowWithNote(-1);
		int prev = eppos;          // expect 2 (note)
		eppos = 0;
		vp->MoveToRowWithNote(-1);
		int prevNone = eppos;      // expect 0 (nothing above row 0)
		bool ok = next == 5 && prev == 2 && prevNone == 0;
		char msg[160];
		snprintf(msg, sizeof(msg), "move-to-note: next=%d(exp 5) prev=%d(exp 2) prevNone=%d(exp 0)",
			next, prev, prevNone);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 20: insert row shifts the channel down, bottom row spills ---
	step++;
	{
		epchn = 0; epnum[0] = 0;
		for (int r = 0; r < 8; r++)
			pattern[0][r * 4] = (unsigned char)(FIRSTNOTE + r);
		pattern[0][8 * 4] = ENDPATT;
		countpatternlengths();
		vp->ClearPatternUndoHistory();
		eppos = 2;
		vp->InsertChannelRow();
		bool ok = pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 0)
			&& pattern[0][1 * 4] == (unsigned char)(FIRSTNOTE + 1)
			&& pattern[0][2 * 4] == (unsigned char)REST
			&& pattern[0][3 * 4] == (unsigned char)(FIRSTNOTE + 2)
			&& pattern[0][7 * 4] == (unsigned char)(FIRSTNOTE + 6);
		char msg[200];
		snprintf(msg, sizeof(msg),
			"insert row: r0=+%d r2=0x%02X(REST) r3=+%d(2) r7=+%d(6)",
			pattern[0][0] - FIRSTNOTE, pattern[0][8], pattern[0][12] - FIRSTNOTE,
			pattern[0][28] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 21: insert then remove at the same row restores the bottom ---
	step++;
	{
		epchn = 0; epnum[0] = 0;
		for (int r = 0; r < 8; r++)
			pattern[0][r * 4] = (unsigned char)(FIRSTNOTE + r);
		pattern[0][8 * 4] = ENDPATT;
		countpatternlengths();
		vp->ClearPatternUndoHistory();
		eppos = 2;
		vp->InsertChannelRow();
		eppos = 2;
		vp->RemoveChannelRow();
		bool ok = true;
		for (int r = 0; r < 8; r++)
			ok = ok && pattern[0][r * 4] == (unsigned char)(FIRSTNOTE + r);
		char msg[200];
		snprintf(msg, sizeof(msg),
			"insert+remove round-trip: r0=+%d r2=+%d r7=+%d (all exp +r)",
			pattern[0][0] - FIRSTNOTE, pattern[0][8] - FIRSTNOTE, pattern[0][28] - FIRSTNOTE);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 22: remove row shifts up; with no spill the bottom is empty ---
	step++;
	{
		epchn = 0; epnum[0] = 0;
		for (int r = 0; r < 8; r++)
			pattern[0][r * 4] = (unsigned char)(FIRSTNOTE + r);
		pattern[0][8 * 4] = ENDPATT;
		countpatternlengths();
		vp->ClearPatternUndoHistory();   // empties the spill stash
		eppos = 2;
		vp->RemoveChannelRow();
		bool ok = pattern[0][0 * 4] == (unsigned char)(FIRSTNOTE + 0)
			&& pattern[0][1 * 4] == (unsigned char)(FIRSTNOTE + 1)
			&& pattern[0][2 * 4] == (unsigned char)(FIRSTNOTE + 3)
			&& pattern[0][6 * 4] == (unsigned char)(FIRSTNOTE + 7)
			&& pattern[0][7 * 4] == (unsigned char)REST;
		char msg[200];
		snprintf(msg, sizeof(msg),
			"remove row: r0=+%d r2=+%d(3) r6=+%d(7) r7=0x%02X(REST)",
			pattern[0][0] - FIRSTNOTE, pattern[0][8] - FIRSTNOTE,
			pattern[0][24] - FIRSTNOTE, pattern[0][28]);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// --- Test 23: an unrelated edit invalidates the spill stash ---
	step++;
	{
		epchn = 0; epnum[0] = 0;
		for (int r = 0; r < 8; r++)
			pattern[0][r * 4] = (unsigned char)(FIRSTNOTE + r);
		pattern[0][8 * 4] = ENDPATT;
		countpatternlengths();
		vp->ClearPatternUndoHistory();
		eppos = 2;
		vp->InsertChannelRow();          // spills +7 onto the stash
		eppos = 0;
		vp->ClearPatternSelection();
		vp->TransposeAtCursor(1);        // unrelated edit -> clears the stash
		eppos = 2;
		vp->RemoveChannelRow();          // no spill -> bottom stays empty
		bool ok = pattern[0][7 * 4] == (unsigned char)REST;
		char msg[200];
		snprintf(msg, sizeof(msg),
			"spill invalidation: r7=0x%02X (exp REST, +7 not restored)",
			pattern[0][7 * 4]);
		StepCompleted(step, ok, msg);
		if (!ok) { restoreState(); TestCompleted(false, msg); return; }
	}

	// === Song order-list operations ===
	if (pluginGoatTracker->renoiseInput != NULL)
	{
		int savedEsnum = esnum;
		int savedEseditpos = eseditpos;
		int savedEditmode = editmode;
		bool savedBulk = gt2RenoiseBulkPatternNumberChange;
		int olSong = (savedEsnum >= 0 && savedEsnum < MAX_SONGS) ? savedEsnum : 0;
		int savedSonglenAll[MAX_CHN];
		unsigned char savedOrderAll[MAX_CHN][MAX_SONGLEN + 2];
		for (int c = 0; c < MAX_CHN; c++)
		{
			savedSonglenAll[c] = songlen[olSong][c];
			memcpy(savedOrderAll[c], songorder[olSong][c], sizeof(savedOrderAll[c]));
		}

		auto restoreOrder = [&]() {
			for (int c = 0; c < MAX_CHN; c++)
			{
				memcpy(songorder[olSong][c], savedOrderAll[c], sizeof(savedOrderAll[c]));
				songlen[olSong][c] = savedSonglenAll[c];
			}
			esnum = savedEsnum;
			eseditpos = savedEseditpos;
			editmode = savedEditmode;
			gt2RenoiseBulkPatternNumberChange = savedBulk;
		};

		// --- Test 24: insert pattern adds a row AFTER the cursor; undo restores it ---
		// HandleInsertPattern inserts after the cursor and moves the cursor onto
		// the new row (commit 46f8f76b "insert pattern after cursor"). With the
		// cursor on index 1 (pattern 11) of [10,11,12], the result is
		// [10, 11, NEW, 12]: index 1 is unchanged, the fresh pattern lands at
		// index 2, and 12 shifts to index 3.
		step++;
		{
			esnum = olSong;
			editmode = 0;          // EDIT_PATTERN -> non-bulk uses epchn
			epchn = 0;
			gt2RenoiseBulkPatternNumberChange = false;
			// All channels populated so findusedpatterns() scans this subtune.
			for (int c = 0; c < MAX_CHN; c++)
			{
				songorder[olSong][c][0] = 10;
				songorder[olSong][c][1] = 11;
				songorder[olSong][c][2] = 12;
				songorder[olSong][c][3] = (unsigned char)LOOPSONG;
				songorder[olSong][c][4] = 0;
				songlen[olSong][c] = 3;
			}
			eseditpos = 1;
			vp->ClearPatternUndoHistory();

			bool rc = pluginGoatTracker->renoiseInput->HandleInsertPattern();
			countpatternlengths();
			int insertedPatt = songorder[olSong][0][2];     // inserted after the cursor (index 1) -> index 2
			bool okInsert = rc
				&& songlen[olSong][0] == 4
				&& songorder[olSong][0][0] == 10
				&& songorder[olSong][0][1] == 11             // cursor entry stays put
				&& songorder[olSong][0][3] == 12             // shifted down by the insert
				&& insertedPatt < MAX_PATT
				&& insertedPatt != 10 && insertedPatt != 11 && insertedPatt != 12
				&& pattlen[insertedPatt] == defaultpatternlength;

			vp->UndoPatternEdit();
			bool okUndo = songlen[olSong][0] == 3
				&& songorder[olSong][0][0] == 10
				&& songorder[olSong][0][1] == 11
				&& songorder[olSong][0][2] == 12;

			bool ok = okInsert && okUndo;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"insert pattern: rc=%d len=%d ins=%d shifted=%d undoLen=%d undoO1=%d",
				(int)rc, songlen[olSong][0] + (okUndo ? 1 : 0), insertedPatt,
				songorder[olSong][0][2], songlen[olSong][0], songorder[olSong][0][1]);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 25: delete pattern shifts the order list up ---
		step++;
		{
			esnum = olSong;
			editmode = 0;
			epchn = 0;
			gt2RenoiseBulkPatternNumberChange = false;
			songorder[olSong][0][0] = 20;
			songorder[olSong][0][1] = 21;
			songorder[olSong][0][2] = 22;
			songorder[olSong][0][3] = (unsigned char)LOOPSONG;
			songorder[olSong][0][4] = 0;
			songlen[olSong][0] = 3;
			eseditpos = 1;
			vp->ClearPatternUndoHistory();

			bool rc = pluginGoatTracker->renoiseInput->HandleDeletePattern();
			bool ok = rc
				&& songlen[olSong][0] == 2
				&& songorder[olSong][0][0] == 20
				&& songorder[olSong][0][1] == 22;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"delete pattern: rc=%d len=%d(2) o0=%d(20) o1=%d(22)",
				(int)rc, songlen[olSong][0], songorder[olSong][0][0], songorder[olSong][0][1]);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 26: Ctrl+Up/Down step the song order position ---
		step++;
		{
			esnum = olSong;
			for (int c = 0; c < MAX_CHN; c++)
			{
				songorder[olSong][c][0] = (unsigned char)(0 + c);
				songorder[olSong][c][1] = (unsigned char)(3 + c);
				songorder[olSong][c][2] = (unsigned char)(6 + c);
				songorder[olSong][c][3] = (unsigned char)LOOPSONG;
				songorder[olSong][c][4] = 0;
				songlen[olSong][c] = 3;
			}
			eseditpos = 0;
			bool rc1 = pluginGoatTracker->renoiseInput->HandleSongPositionShortcut(1);
			int posNext = eseditpos, e0Next = epnum[0], e2Next = epnum[2];
			bool rc2 = pluginGoatTracker->renoiseInput->HandleSongPositionShortcut(-1);
			int posPrev = eseditpos;
			pluginGoatTracker->renoiseInput->HandleSongPositionShortcut(-1);  // clamps at 0
			int posClampLo = eseditpos;
			eseditpos = 2;
			pluginGoatTracker->renoiseInput->HandleSongPositionShortcut(1);   // clamps at last
			int posClampHi = eseditpos;
			bool ok = rc1 && rc2
				&& posNext == 1 && e0Next == 3 && e2Next == 5
				&& posPrev == 0 && posClampLo == 0 && posClampHi == 2;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"song pos: next=%d(1) e0=%d(3) e2=%d(5) prev=%d(0) clampLo=%d(0) clampHi=%d(2)",
				posNext, e0Next, e2Next, posPrev, posClampLo, posClampHi);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 27: pattern-number shortcut — bulk vs non-bulk ---
		step++;
		{
			esnum = olSong;
			editmode = 0;   // EDIT_PATTERN -> non-bulk targets epchn
			epchn = 1;
			for (int c = 0; c < MAX_CHN; c++)
			{
				songorder[olSong][c][1] = (unsigned char)LOOPSONG;
				songorder[olSong][c][2] = 0;
				songlen[olSong][c] = 1;
			}
			eseditpos = 0;

			// Non-bulk: only the cursor channel (epchn=1) shifts, by 1.
			gt2RenoiseBulkPatternNumberChange = false;
			for (int c = 0; c < MAX_CHN; c++)
				songorder[olSong][c][0] = (unsigned char)(10 + c * 5);
			pluginGoatTracker->renoiseInput->HandleOrderListPatternNumberShortcut(1);
			bool okNonBulk = songorder[olSong][0][0] == 10
				&& songorder[olSong][1][0] == 16 && epnum[1] == 16
				&& songorder[olSong][2][0] == 20;

			// Bulk: every channel shifts by the channel count.
			gt2RenoiseBulkPatternNumberChange = true;
			for (int c = 0; c < MAX_CHN; c++)
				songorder[olSong][c][0] = (unsigned char)(20 + c);
			pluginGoatTracker->renoiseInput->HandleOrderListPatternNumberShortcut(1);
			bool okBulkUp = songorder[olSong][0][0] == 20 + MAX_CHN
				&& songorder[olSong][2][0] == 22 + MAX_CHN
				&& epnum[0] == 20 + MAX_CHN && epnum[2] == 22 + MAX_CHN;
			pluginGoatTracker->renoiseInput->HandleOrderListPatternNumberShortcut(-1);
			bool okBulkDown = songorder[olSong][0][0] == 20
				&& songorder[olSong][2][0] == 22;
			gt2RenoiseBulkPatternNumberChange = savedBulk;

			bool ok = okNonBulk && okBulkUp && okBulkDown;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"pattnum shortcut: nonBulk=%d bulkUp=%d bulkDown=%d (o0=%d o1=%d o2=%d)",
				(int)okNonBulk, (int)okBulkUp, (int)okBulkDown,
				songorder[olSong][0][0], songorder[olSong][1][0], songorder[olSong][2][0]);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 28: delete keeps the last row; insert recovers an empty list ---
		step++;
		{
			esnum = olSong;
			editmode = 0;
			epchn = 0;
			gt2RenoiseBulkPatternNumberChange = false;
			songorder[olSong][0][0] = 30;
			songorder[olSong][0][1] = (unsigned char)LOOPSONG;
			songorder[olSong][0][2] = 0;
			songlen[olSong][0] = 1;
			eseditpos = 0;

			// Delete must refuse when only one row is left.
			pluginGoatTracker->renoiseInput->HandleDeletePattern();
			int lenAfterDel = songlen[olSong][0];
			bool okKeepLast = lenAfterDel == 1 && songorder[olSong][0][0] == 30;

			// Force the list empty, then "+" must recover it with a fresh row.
			songorder[olSong][0][0] = (unsigned char)LOOPSONG;
			songorder[olSong][0][1] = 0;
			songlen[olSong][0] = 0;
			eseditpos = 0;
			pluginGoatTracker->renoiseInput->HandleInsertPattern();
			countpatternlengths();
			int lenAfterIns = songlen[olSong][0];
			bool okRecover = lenAfterIns == 1 && songorder[olSong][0][0] < MAX_PATT;

			bool ok = okKeepLast && okRecover;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"keep-last=%d (len=%d, want 1)  recover=%d (len=%d o0=%d)",
				(int)okKeepLast, lenAfterDel, (int)okRecover, lenAfterIns,
				songorder[olSong][0][0]);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 29: SortPatterns renumbers blocks to the order sequence ---
		step++;
		{
			esnum = olSong;
			int blocks[4] = { 0, 3, 1, 2 };
			for (int c = 0; c < MAX_CHN; c++)
			{
				for (int i = 0; i < 4; i++)
					songorder[olSong][c][i] = (unsigned char)(blocks[i] * MAX_CHN + c);
				songorder[olSong][c][4] = (unsigned char)LOOPSONG;
				songorder[olSong][c][5] = 0;
				songlen[olSong][c] = 4;
			}
			eseditpos = 0;
			pluginGoatTracker->viewPatternList->SortPatterns();

			bool ok = (songlen[olSong][0] == 4);
			for (int i = 0; i < 4 && ok; i++)
				for (int c = 0; c < MAX_CHN; c++)
					if (songorder[olSong][c][i] != (unsigned char)(i * MAX_CHN + c))
						ok = false;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"sort [0,3,1,2]->[0,1,2,3]: blocks=%d,%d,%d,%d",
				songorder[olSong][0][0] / MAX_CHN, songorder[olSong][0][1] / MAX_CHN,
				songorder[olSong][0][2] / MAX_CHN, songorder[olSong][0][3] / MAX_CHN);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 30: RemoveUnusedPatterns drops gaps, compacts ascending ---
		step++;
		{
			esnum = olSong;
			int blocksA[4] = { 0, 3, 4, 5 };   // gaps at 1,2 -> compact to 0,1,2,3
			for (int c = 0; c < MAX_CHN; c++)
			{
				for (int i = 0; i < 4; i++)
					songorder[olSong][c][i] = (unsigned char)(blocksA[i] * MAX_CHN + c);
				songorder[olSong][c][4] = (unsigned char)LOOPSONG;
				songorder[olSong][c][5] = 0;
				songlen[olSong][c] = 4;
			}
			eseditpos = 0;
			pluginGoatTracker->viewPatternList->RemoveUnusedPatterns();
			bool okA = (songlen[olSong][0] == 4);
			for (int i = 0; i < 4 && okA; i++)
				for (int c = 0; c < MAX_CHN; c++)
					if (songorder[olSong][c][i] != (unsigned char)(i * MAX_CHN + c))
						okA = false;

			int blocksB[3] = { 0, 5, 3 };      // ascending compact -> 0,2,1
			int expB[3]    = { 0, 2, 1 };
			for (int c = 0; c < MAX_CHN; c++)
			{
				for (int i = 0; i < 3; i++)
					songorder[olSong][c][i] = (unsigned char)(blocksB[i] * MAX_CHN + c);
				songorder[olSong][c][3] = (unsigned char)LOOPSONG;
				songorder[olSong][c][4] = 0;
				songlen[olSong][c] = 3;
			}
			eseditpos = 0;
			pluginGoatTracker->viewPatternList->RemoveUnusedPatterns();
			bool okB = (songlen[olSong][0] == 3);
			for (int i = 0; i < 3 && okB; i++)
				for (int c = 0; c < MAX_CHN; c++)
					if (songorder[olSong][c][i] != (unsigned char)(expB[i] * MAX_CHN + c))
						okB = false;

			bool ok = okA && okB;
			char msg[200];
			snprintf(msg, sizeof(msg),
				"remove unused: A[0,3,4,5]->compact=%d  B[0,5,3]->[0,2,1]=%d",
				(int)okA, (int)okB);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		// --- Test 31: DuplicateSelection on a 1-row song adds a clone row ---
		step++;
		{
			esnum = olSong;
			for (int c = 0; c < MAX_CHN; c++)
			{
				songorder[olSong][c][0] = (unsigned char)c;          // block 0
				songorder[olSong][c][1] = (unsigned char)LOOPSONG;
				songorder[olSong][c][2] = 0;
				songlen[olSong][c] = 1;
			}
			eseditpos = 0;
			pluginGoatTracker->viewPatternList->DuplicateSelection();

			bool ok = (songlen[olSong][0] == 2);
			for (int c = 0; c < MAX_CHN && ok; c++)
			{
				int row1 = songorder[olSong][c][1];
				if (row1 >= MAX_PATT)                  ok = false;  // a real pattern
				if (row1 == songorder[olSong][c][0])   ok = false;  // a clone, not the original
			}
			char msg[200];
			snprintf(msg, sizeof(msg),
				"duplicate 1-row: len=%d(2) row1=%d,%d,%d",
				songlen[olSong][0], songorder[olSong][0][1],
				songorder[olSong][1][1], songorder[olSong][2][1]);
			StepCompleted(step, ok, msg);
			if (!ok) { restoreOrder(); restoreState(); TestCompleted(false, msg); return; }
		}

		restoreOrder();
	}

	restoreState();
	TestCompleted(true, "All GT2SelectionOps tests passed");
}
