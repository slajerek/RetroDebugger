#pragma once

#include "CTest.h"

class CTestVicEditorCursor : public CTest
{
public:
	virtual const char *GetName() override { return "VicEditorCursor"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
