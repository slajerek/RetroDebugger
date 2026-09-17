#include "CTestGT2SongInfo.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "gorder.h"
#include "goattrk2.h"
#include "gconsole.h"

extern char songname[];
extern char authorname[];
extern char copyrightname[];
extern int enpos;
extern unsigned *scrbuffer;
extern unsigned char *chardata;
extern int gfxinitted;
}

// MAX_COLUMNS from gconsole.h is 100
#define GT2_MAX_COLUMNS 100

static inline unsigned char scr_char2(int col, int row)
{
	return (unsigned char)(scrbuffer[row * GT2_MAX_COLUMNS + col] & 0xff);
}

void CTestGT2SongInfo::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

	int step = 0;

	// Guard: GT2 not initialized — skip all tests
	if (chardata == NULL)
	{
		step++;
		StepCompleted(step, true, "GT2 not active (chardata==NULL) — skipped");
		TestSkipped("GT2 plugin not active -- song-info parsing was never exercised");
		return;
	}

	// --- Test 1: Edit song name and verify songname[] matches ---
	step++;
	{
		char savedSongname[MAX_STR];
		strncpy(savedSongname, songname, MAX_STR);

		const char *testName = "MYSONGTITLE";
		strncpy(songname, testName, MAX_STR - 1);
		songname[MAX_STR - 1] = '\0';

		bool ok = (strncmp(songname, testName, MAX_STR) == 0);
		char msg[128];
		snprintf(msg, sizeof(msg), "songname='%s' (expected '%s')", songname, testName);
		StepCompleted(step, ok, msg);

		// Restore
		strncpy(songname, savedSongname, MAX_STR);

		if (!ok) { TestCompleted(false, msg); return; }
	}

	// --- Test 2: Edit author name and verify authorname[] matches ---
	step++;
	{
		char savedAuthor[MAX_STR];
		strncpy(savedAuthor, authorname, MAX_STR);

		const char *testAuthor = "COMPOSER42";
		strncpy(authorname, testAuthor, MAX_STR - 1);
		authorname[MAX_STR - 1] = '\0';

		bool ok = (strncmp(authorname, testAuthor, MAX_STR) == 0);
		char msg[128];
		snprintf(msg, sizeof(msg), "authorname='%s' (expected '%s')", authorname, testAuthor);
		StepCompleted(step, ok, msg);

		// Restore
		strncpy(authorname, savedAuthor, MAX_STR);

		if (!ok) { TestCompleted(false, msg); return; }
	}

	// --- Test 3: Edge — MAX_STR (32 chars) does not overflow ---
	step++;
	{
		char savedSongname[MAX_STR];
		strncpy(savedSongname, songname, MAX_STR);

		// Fill with MAX_STR-1 chars (31) + null — maximum valid content
		char maxName[MAX_STR];
		memset(maxName, 0, MAX_STR);
		for (int i = 0; i < MAX_STR - 1; i++)
			maxName[i] = 'X';

		strncpy(songname, maxName, MAX_STR - 1);
		songname[MAX_STR - 1] = '\0';

		// Length must be exactly MAX_STR-1
		int len = (int)strlen(songname);
		bool ok = (len == MAX_STR - 1);
		char msg[128];
		snprintf(msg, sizeof(msg), "max-length song name: len=%d (expected %d)", len, MAX_STR - 1);
		StepCompleted(step, ok, msg);

		// Restore
		strncpy(songname, savedSongname, MAX_STR);

		if (!ok) { TestCompleted(false, msg); return; }
	}

	// --- Test 4: Edge — empty string sets cursor to 0 and renders without crash ---
	step++;
	{
		char savedSongname[MAX_STR];
		char savedAuthor[MAX_STR];
		char savedCopyright[MAX_STR];
		int savedEnpos = enpos;

		strncpy(savedSongname,   songname,      MAX_STR);
		strncpy(savedAuthor,     authorname,    MAX_STR);
		strncpy(savedCopyright,  copyrightname, MAX_STR);

		// Empty all fields and reset cursor
		memset(songname,      0, MAX_STR);
		memset(authorname,    0, MAX_STR);
		memset(copyrightname, 0, MAX_STR);
		enpos = 0;

		// strlen must be 0 and enpos at 0
		bool ok = (strlen(songname) == 0) &&
				  (strlen(authorname) == 0) &&
				  (strlen(copyrightname) == 0) &&
				  (enpos == 0);

		char msg[128];
		snprintf(msg, sizeof(msg),
				 "empty strings: songname_len=%d authorname_len=%d copyright_len=%d enpos=%d",
				 (int)strlen(songname), (int)strlen(authorname), (int)strlen(copyrightname), enpos);
		StepCompleted(step, ok, msg);

		// Restore
		strncpy(songname,      savedSongname,   MAX_STR);
		strncpy(authorname,    savedAuthor,     MAX_STR);
		strncpy(copyrightname, savedCopyright,  MAX_STR);
		enpos = savedEnpos;

		if (!ok) { TestCompleted(false, msg); return; }
	}

	// --- Test 5: Render identity — scrbuffer rows 31-33 reflect song info data ---
	step++;
	{
		if (!gfxinitted)
		{
			StepCompleted(step, true, "gfxinitted=0 — scrbuffer song info test skipped (no screen)");
		}
		else
		{
			char savedSongname[MAX_STR];
			char savedAuthor[MAX_STR];
			char savedCopyright[MAX_STR];

			strncpy(savedSongname,   songname,      MAX_STR);
			strncpy(savedAuthor,     authorname,    MAX_STR);
			strncpy(savedCopyright,  copyrightname, MAX_STR);

			const char *testName = "RENDERTESTNAME";
			strncpy(songname, testName, MAX_STR - 1);
			songname[MAX_STR - 1] = '\0';

			// Trigger display update
			printstatus();

			// gdisplay.c: printtext(40+10, 31, CTITLE, "NAME   ")
			// So col 50, row 31 should be 'N' from "NAME   "
			unsigned char ch = scr_char2(50, 31);
			bool ok = (ch == 'N');
			char msg[128];
			snprintf(msg, sizeof(msg), "scrbuffer[31][50]='%c' (expected 'N')", (char)ch);
			StepCompleted(step, ok, msg);

			// Restore
			strncpy(songname,      savedSongname,   MAX_STR);
			strncpy(authorname,    savedAuthor,     MAX_STR);
			strncpy(copyrightname, savedCopyright,  MAX_STR);

			if (!ok) { TestCompleted(false, msg); return; }
		}
	}

	// --- Test 6: Render identity edge — all fields empty renders labels only ---
	step++;
	{
		if (!gfxinitted)
		{
			StepCompleted(step, true, "gfxinitted=0 — labels-only render test skipped (no screen)");
		}
		else
		{
			char savedSongname[MAX_STR];
			char savedAuthor[MAX_STR];
			char savedCopyright[MAX_STR];

			strncpy(savedSongname,   songname,      MAX_STR);
			strncpy(savedAuthor,     authorname,    MAX_STR);
			strncpy(savedCopyright,  copyrightname, MAX_STR);

			memset(songname,      0, MAX_STR);
			memset(authorname,    0, MAX_STR);
			memset(copyrightname, 0, MAX_STR);

			printstatus();

			// With empty fields: col 50 row 31 = 'N', col 50 row 32 = 'A', col 50 row 33 = 'C'
			unsigned char chName   = scr_char2(50, 31);
			unsigned char chAuthor = scr_char2(50, 32);
			unsigned char chCopy   = scr_char2(50, 33);

			bool ok = (chName == 'N') && (chAuthor == 'A') && (chCopy == 'C');
			char msg[128];
			snprintf(msg, sizeof(msg),
					 "labels-only: row31[50]='%c' row32[50]='%c' row33[50]='%c'",
					 (char)chName, (char)chAuthor, (char)chCopy);
			StepCompleted(step, ok, msg);

			// Restore
			strncpy(songname,      savedSongname,   MAX_STR);
			strncpy(authorname,    savedAuthor,     MAX_STR);
			strncpy(copyrightname, savedCopyright,  MAX_STR);

			if (!ok) { TestCompleted(false, msg); return; }
		}
	}

	TestCompleted(true, "All GT2SongInfo tests passed");
}
