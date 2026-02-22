#pragma once

#include "CTest.h"

class CTestVicEditorZoom : public CTest
{
public:
	virtual const char *GetName() override { return "VicEditorZoom"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};
