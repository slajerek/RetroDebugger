#pragma once

#include "CTest.h"
#include <string>

// RD-007: the code check -- the code a cc65 build's debug info maps, in the
// C64's memory, against the program file at the mappings' file offsets
// (remote source/verify). The basic-flow PRG poked into RAM matches; one
// changed code byte is reported at its address; another build's PRG differs.
class CTestSourceVerify : public CTest
{
public:
	virtual const char *GetName() override { return "SourceVerify"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);
};
