#pragma once

#include "CTest.h"

// Regression test for: "CTRL+SHIFT+0 does not detach cartridge".
//
// Detach Cartridge was routed through interrupt_maincpu_trigger_trap(), and a
// maincpu trap only ever runs when the CPU executes an instruction. Paused in
// the debugger the trap sat in the queue forever: the cartridge stayed mapped
// while the UI still announced "Cartridge detached". Detach Everything looked
// fine only because CMainMenuBar::DetachEverything() forces
// DEBUGGER_MODE_RUNNING right after queueing its own trap, which drains it.
//
// The test attaches a CRT, pauses the machine, detaches, and requires the
// EXROM/GAME lines to go inactive while still paused.
class CTestDetachCartridgePaused : public CTest
{
public:
	virtual const char *GetName() override { return "DetachCartridgePaused"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
