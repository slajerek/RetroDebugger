#pragma once

#include "CTest.h"

// Verifies the attach-only debugger API with autorun enabled in settings:
// no program load, reset, or paused CPU/RAM changes. Run in an isolated
// emulator without a disk already mounted.
class CTestDiskAttach : public CTest
{
public:
	virtual const char *GetName() override { return "DiskAttach"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
