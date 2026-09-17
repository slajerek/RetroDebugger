#pragma once

#include "CTest.h"

// Verifies that every registered global keyboard shortcut can actually be
// triggered from the keyboard.
//
// A shortcut is declared with the BARE key ('0', 'd', '8'), but what arrives
// from SDL and is then run through SYS_GetShiftedKey() in VID_ProcessEvents()
// is the SHIFTED key (')', 'D', '*'). CGuiMain::CheckKeyboardShortcut() folds
// that back with SYS_GetBareKey() before the table lookup, so declaration and
// lookup only meet if the two tables are exact inverses for that key.
//
// The test walks the live shortcut table and, for each entry, replays that
// pipeline and requires the lookup to return the very same shortcut. It is a
// pure table/lookup check -- no action is invoked, nothing in the emulator is
// touched.
class CTestKeyboardShortcuts : public CTest
{
public:
	virtual const char *GetName() override { return "KeyboardShortcuts"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
