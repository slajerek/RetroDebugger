#ifndef _CTestGT2SongSettings_h_
#define _CTestGT2SongSettings_h_
#include "CTest.h"

// Song-level settings: the .sng metadata, the chip and timing model, the
// player options and the tuning. Every one of them is reached through
// CViewGT2SongSettings' static accessors, which touch GT2 globals only -- so
// unlike the GT2 views that fork a text screen, this test runs headlessly and
// needs neither chardata nor a live plugin.
class CTestGT2SongSettings : public CTest
{
public:
	virtual const char *GetName() { return "GT2SongSettings"; }
	virtual void Run(ITestCallback *callback);
	virtual void Cancel() {}
};
#endif
