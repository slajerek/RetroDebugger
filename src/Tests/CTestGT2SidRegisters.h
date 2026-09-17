#ifndef _CTestGT2SidRegisters_h_
#define _CTestGT2SidRegisters_h_
#include "CTest.h"

// GT2 SID register access behind the generic SID state view.
//
// The claim under test is the one the whole design rests on: GT2's
// playroutine() runs every frame whether the song plays or not and rebuilds
// most of sidreg[] from its own state, so a register write only survives if it
// is applied to the player variable that register is derived from. Each step
// writes a register, runs playroutine(), and checks what actually came out.
class CTestGT2SidRegisters : public CTest
{
public:
	virtual const char *GetName() { return "GT2SidRegisters"; }
	virtual void Run(ITestCallback *callback);
	virtual void Cancel() {}
};
#endif
