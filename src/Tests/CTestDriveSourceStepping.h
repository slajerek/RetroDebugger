#pragma once

#include "CTest.h"
#include <string>

// RD-044: source debugging on the 1541 drive CPU. The cc65 "drive1541"
// fixture (ca65 code for drive RAM at $0500) is poked into the drive and its
// debug info loaded into the drive's symbols; a source breakpoint on the
// drive stops there; source Over/Into/Out on the drive CPU stop on the
// fixture's lines while the C64 keeps running, and the drive's
// events history records them; the 1541 Source code view follows the drive
// PC. The drive is reset afterwards.
class CTestDriveSourceStepping : public CTest
{
public:
	virtual const char *GetName() override { return "DriveSourceStepping"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
	bool RunDrive();
};
