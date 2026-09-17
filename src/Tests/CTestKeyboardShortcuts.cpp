#include "CTestKeyboardShortcuts.h"
#include "CViewC64.h"
#include "CMainMenuBar.h"
#include "CGuiMain.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrString.h"
#include "SYS_Main.h"
#include "SYS_KeyCodes.h"
#include "CViewC64Screen.h"
#include <cstdio>
#include <cstring>
#include <string>

static char failureMsg[1024];

// Records which shortcut actually reached its callback, so the dispatch check
// can run without invoking the real (destructive) action.
class CProbeShortcutCallback : public CSlrKeyboardShortcutCallback
{
public:
	CProbeShortcutCallback() { fired = NULL; }
	CSlrKeyboardShortcut *fired;

	virtual bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut) override
	{
		fired = keyboardShortcut;
		return true;
	}
};

void CTestKeyboardShortcuts::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

	if (guiMain == NULL || guiMain->keyboardShortcuts == NULL)
	{
		TestCompleted(false, "guiMain keyboard shortcuts are not initialized");
		return;
	}

	std::map<u32, CSlrKeyboardShortcutsZone *>::iterator itZone =
		guiMain->keyboardShortcuts->mapOfZones->find(KBZONE_GLOBAL);
	if (itZone == guiMain->keyboardShortcuts->mapOfZones->end())
	{
		TestCompleted(false, "global keyboard shortcut zone does not exist");
		return;
	}

	CSlrKeyboardShortcutsZone *zone = itZone->second;

	// --- Step 1: the shortcut table is populated at all ---------------------
	if (zone->shortcuts.empty())
	{
		TestCompleted(false, "global keyboard shortcut zone is empty");
		return;
	}
	StepCompleted(1, true, "global shortcut zone populated");

	// --- Step 2: every shortcut survives the SDL -> bare-key round trip -----
	//
	// This is the whole point of the test. VID_ProcessEvents() hands
	// CGuiMain::KeyDown() the shifted key; CheckKeyboardShortcut() folds it
	// back with SYS_GetBareKey() and looks it up. Replay exactly that.
	int checked = 0;
	int broken = 0;
	std::string brokenNames;

	for (std::list<CSlrKeyboardShortcut *>::iterator it = zone->shortcuts.begin();
		 it != zone->shortcuts.end(); it++)
	{
		CSlrKeyboardShortcut *shortcut = *it;

		// keyCode <= 0 means "unbound" (LoadFromByteBuffer parks conflicting
		// shortcuts there). Nothing to press, nothing to check.
		if (shortcut->keyCode <= 0)
			continue;

		checked++;

		u32 keyValue = SYS_GetShiftedKey(shortcut->keyCode,
										 shortcut->isShift, shortcut->isAlt,
										 shortcut->isControl, shortcut->isSuper);
		u32 keyBare = SYS_GetBareKey(keyValue,
									 shortcut->isShift, shortcut->isAlt,
									 shortcut->isControl, shortcut->isSuper);

		CSlrKeyboardShortcut *found = zone->FindShortcut(keyBare,
														 shortcut->isShift, shortcut->isAlt,
														 shortcut->isControl, shortcut->isSuper);

		if (found != shortcut)
		{
			broken++;
			if (!brokenNames.empty())
				brokenNames += ", ";
			char buf[256];
			sprintf(buf, "'%s' (declared key 0x%02X, pressed 0x%02X, folded back to 0x%02X -> %s)",
					shortcut->name ? shortcut->name : "?",
					(unsigned int)shortcut->keyCode, (unsigned int)keyValue, (unsigned int)keyBare,
					found ? (found->name ? found->name : "?") : "nothing");
			brokenNames += buf;
		}
	}

	if (broken > 0)
	{
		snprintf(failureMsg, sizeof(failureMsg),
				 "%d of %d global shortcuts cannot be triggered from the keyboard: %s",
				 broken, checked, brokenNames.c_str());
		TestCompleted(false, failureMsg);
		return;
	}

	char stepMsg[128];
	sprintf(stepMsg, "%d global shortcuts round-trip SDL key -> bare key -> lookup", checked);
	StepCompleted(2, true, stepMsg);

	// --- Step 3: the shortcuts named in the bug report, by name ------------
	//
	// Detach Cartridge (Ctrl+Shift+0) was reported dead while Detach
	// Everything (Ctrl+Shift+D) worked, so pin both explicitly: a future
	// rebinding that lands on a key the fold-back cannot reproduce must fail
	// here with the shortcut named, not just as a count.
	CMainMenuBar *menuBar = viewC64 ? viewC64->mainMenuBar : NULL;
	if (menuBar == NULL)
	{
		TestCompleted(false, "main menu bar is not available");
		return;
	}

	struct NamedCase
	{
		CSlrKeyboardShortcut *shortcut;
		const char *label;
	};
	NamedCase cases[] =
	{
		{ menuBar->kbsDetachCartridge,  "Detach Cartridge"  },
		{ menuBar->kbsDetachEverything, "Detach Everything" },
		{ menuBar->kbsDetachDiskImage,  "Detach Disk Image" },
		{ menuBar->kbsDetachExecutable, "Detach Executable" },
	};

	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++)
	{
		CSlrKeyboardShortcut *shortcut = cases[i].shortcut;
		if (shortcut == NULL)
		{
			snprintf(failureMsg, sizeof(failureMsg), "%s shortcut object is NULL", cases[i].label);
			TestCompleted(false, failureMsg);
			return;
		}

		if (shortcut->keyCode <= 0)
		{
			snprintf(failureMsg, sizeof(failureMsg),
					 "%s is unbound (keyCode=%d) so it can never fire from the keyboard",
					 cases[i].label, (int)shortcut->keyCode);
			TestCompleted(false, failureMsg);
			return;
		}

		u32 keyValue = SYS_GetShiftedKey(shortcut->keyCode,
										 shortcut->isShift, shortcut->isAlt,
										 shortcut->isControl, shortcut->isSuper);
		u32 keyBare = SYS_GetBareKey(keyValue,
									 shortcut->isShift, shortcut->isAlt,
									 shortcut->isControl, shortcut->isSuper);
		CSlrKeyboardShortcut *found = zone->FindShortcut(keyBare,
														 shortcut->isShift, shortcut->isAlt,
														 shortcut->isControl, shortcut->isSuper);
		if (found != shortcut)
		{
			snprintf(failureMsg, sizeof(failureMsg),
					 "%s (%s): pressing it yields key 0x%02X, folded back to 0x%02X, which resolves to %s",
					 cases[i].label, shortcut->cstr ? shortcut->cstr : "?",
					 (unsigned int)keyValue, (unsigned int)keyBare,
					 found ? (found->name ? found->name : "?") : "nothing");
			TestCompleted(false, failureMsg);
			return;
		}
	}
	StepCompleted(3, true, "detach shortcuts resolve to their own actions");

	// --- Step 4: the whole dispatch chain, not just the lookup -------------
	//
	// A resolvable table entry still fires nothing if some view earlier in
	// CGuiMain::KeyDown() eats the key first, which is exactly what the bug
	// report suspected. Replay a real keystroke -- shifted key value, live
	// modifier state, C64 screen focused, same as a user pressing it while a
	// cartridge runs -- and require the shortcut's own callback to be reached.
	//
	// The callback is swapped for a probe so the check costs nothing: no
	// detach, no power cycle, no settings write.
	CProbeShortcutCallback probe;

	bool savedShift   = guiMain->isShiftPressed;
	bool savedAlt     = guiMain->isAltPressed;
	bool savedControl = guiMain->isControlPressed;
	bool savedSuper   = guiMain->isSuperPressed;
	CGuiView *savedFocus = guiMain->focusedView;

	if (viewC64->viewC64Screen != NULL && viewC64->viewC64Screen->visible)
	{
		guiMain->SetFocus(viewC64->viewC64Screen);
	}

	bool dispatchOk = true;
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])) && dispatchOk; i++)
	{
		CSlrKeyboardShortcut *shortcut = cases[i].shortcut;

		CSlrKeyboardShortcutCallback *savedCallback = shortcut->callback;
		shortcut->callback = &probe;
		probe.fired = NULL;

		guiMain->isShiftPressed   = shortcut->isShift;
		guiMain->isAltPressed     = shortcut->isAlt;
		guiMain->isControlPressed = shortcut->isControl;
		guiMain->isSuperPressed   = shortcut->isSuper;

		u32 keyValue = SYS_GetShiftedKey(shortcut->keyCode,
										 shortcut->isShift, shortcut->isAlt,
										 shortcut->isControl, shortcut->isSuper);
		guiMain->KeyDown(keyValue);

		shortcut->callback = savedCallback;

		if (probe.fired != shortcut)
		{
			snprintf(failureMsg, sizeof(failureMsg),
					 "%s (%s): pressing it did not reach the action -- key 0x%02X was consumed before the shortcut table%s",
					 cases[i].label, shortcut->cstr ? shortcut->cstr : "?",
					 (unsigned int)keyValue,
					 probe.fired ? " (a different shortcut fired)" : "");
			dispatchOk = false;
		}
	}

	guiMain->isShiftPressed   = savedShift;
	guiMain->isAltPressed     = savedAlt;
	guiMain->isControlPressed = savedControl;
	guiMain->isSuperPressed   = savedSuper;
	if (savedFocus != NULL)
		guiMain->SetFocus(savedFocus);

	if (!dispatchOk)
	{
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(4, true, "detach shortcuts survive the full CGuiMain::KeyDown dispatch");

	TestCompleted(true, "All global keyboard shortcuts are reachable from the keyboard");
}

void CTestKeyboardShortcuts::Cancel()
{
	isRunning = false;
}
