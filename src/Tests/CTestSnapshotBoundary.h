#pragma once

#include "CTest.h"

// Regression test for MCP/remote snapshot load & save from a non-emulation
// thread. Reproduces the bug where retro_snapshot_load restored RAM/VIC but
// the live 6510 kept executing the pre-load program because CPU-loop registers
// were never re-imported at an instruction boundary.
class CTestSnapshotBoundary : public CTest
{
public:
	virtual const char *GetName() override { return "SnapshotBoundary"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
