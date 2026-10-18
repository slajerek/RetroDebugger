#pragma once

// Tests that swap NES cartridges do it the way the app does: on the emulation
// thread, between two frames (a VSync task), never from the test thread while
// the NES thread runs (or sits paused inside Execute) on the board being
// replaced. Each call sets the NES running so the task runs, waits for it,
// and leaves the machine paused (PAUSEAFTER) or running.
// Returns false when the task did not run within TIMEOUTMS or the load failed.

class CDebugInterface;

bool TestNesInsertCartridge(CDebugInterface *di, const char *path, bool pauseAfter, int timeoutMs = 5000);
bool TestNesUnloadCartridge(CDebugInterface *di, bool pauseAfter, int timeoutMs = 5000);
