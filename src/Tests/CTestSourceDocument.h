#pragma once

#include "CTest.h"
#include <string>

// RD-003/RD-004/RD-005: the Source Code view's model, without ImGui.
//  - CSourceDocument: BOM, LF/CRLF/CR, tab expansion;
//  - CSourceDocumentRegistry: remaps, source roots, reload after an edit;
//  - the lexers against golden token files in
//    tests/data/source-debug/lexer/<sample>.tokens (MT_UPDATE_GOLDEN=1
//    rewrites them from the current lexer);
//  - CViewSourceCode on the C64 with the Oscar64 basic-flow debug info in
//    scratch symbols: the source found through a source root, ShowAddress,
//    the addresses of a line, gutter clicks toggling source breakpoints.
class CTestSourceDocument : public CTest
{
public:
	virtual const char *GetName() override { return "SourceDocument"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);

	bool RunDocument();
	bool RunRegistry();
	bool RunLexers();
	bool RunView();
};
