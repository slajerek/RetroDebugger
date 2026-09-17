#pragma once

#include "CTest.h"

// Guards the HiDPI UI-scale machinery: the ImGui .ini geometry transformer, the
// layout-buffer migration, the size-like parameter classification, and the
// no-compounding rule on the ImGui style.
//
// See the HiDPI UI scaling design notes.
class CTestUiScale : public CTest
{
public:
	virtual const char *GetName() override { return "UiScale"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
