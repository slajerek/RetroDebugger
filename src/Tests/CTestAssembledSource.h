#pragma once

#include "CTest.h"
#include <string>

// RD-043: the embedded 64tass as a symbol and source-map producer. A database
// built from an assembler line map; merged on top of loaded debug info; C64
// code assembled into RAM through the plugin API gets its labels and its text
// as a source the source view shows; code assembled over it replaces it; a
// debug-info load drops it; the remote 64tass path returns labels and lines.
class CTestAssembledSource : public CTest
{
public:
	virtual const char *GetName() override { return "AssembledSource"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);

	bool RunDatabase();
	bool RunC64();
};
