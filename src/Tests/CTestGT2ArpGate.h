#pragma once

#include "CTest.h"

// GT2 C player: the main track (column 0) owns the gate and the hard
// restart; arp columns only add pitches to the cycle. Drives gplay.c's
// playroutine()/playtestnote()/triggerpatternrow() directly and asserts
// on chn[] and sidreg[] — no VICE, no audio thread.
class CTestGT2ArpGate : public CTest
{
public:
	virtual const char *GetName() override { return "GT2ArpGate"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
