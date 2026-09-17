#pragma once

#include "CTest.h"

// Verifies the "Detach Disk Image" action (GUI: File -> Detach Disk Image,
// remote: <platform>/detachDiskImage, MCP: retro_disk_detach).
//
// The point of that action is that it removes the mounted disk WITHOUT
// resetting the machine, unlike DetachEverything() which power-cycles the C64.
// The test attaches a D64, marks RAM, detaches through the debugger API and
// then checks that the disk is gone while RAM, the frame counter and the
// running CPU survived untouched.
class CTestDiskDetach : public CTest
{
public:
	virtual const char *GetName() override { return "DiskDetach"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
