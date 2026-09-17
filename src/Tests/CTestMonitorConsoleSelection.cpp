#include "CTestMonitorConsoleSelection.h"
#include "CGuiViewConsole.h"
#include "CViewC64.h"
#include <cstring>
#include <string>
#include <vector>

namespace
{
	// satisfies CGuiViewConsoleCallback; also captures executed commands for future PasteText tests
	class CaptureCallback : public CGuiViewConsoleCallback
	{
	public:
		std::vector<std::string> executed;
		CGuiViewConsole *console = NULL;
		void GuiViewConsoleExecuteCommand(char *commandText) override
		{
			executed.push_back(std::string(commandText));
			// emulate the real host: clear the command line after executing
			commandText[0] = 0;
			// emulate ResetCommandLine()'s cursor reset so PasteText can rely on
			// the callback leaving commandLineCursorPos in the correct state
			if (console) console->commandLineCursorPos = 0;
		}
	};

	CGuiViewConsole *MakeConsole(CaptureCallback *cb)
	{
		CGuiViewConsole *c = new CGuiViewConsole(0, 0, 0, 800, 400,
			viewC64->fontDefaultCBMShifted, 2.0f, 10, true, cb);
		c->SetPosition(0, 0, 0, 800, 400);
		c->SetPrompt((char *)".");
		cb->console = c;
		return c;
	}
}

void CTestMonitorConsoleSelection::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	int step = 0;
	bool allOk = true;

	// --- Task 1: hit-testing ---
	{
		CaptureCallback cc;
		CGuiViewConsole *c = MakeConsole(&cc);
		c->PrintSingleLine((char *)"HELLO WORLD");

		int lastRow = CGuiViewConsole::CONSOLE_ROW_COMMANDLINE - 1; // newest line
		float yRow = (c->posY + 3.0f) + (float)(c->numLines - 1) * c->lineHeight + 1.0f;
		CGuiViewConsole::CConsoleTextPos p = c->ScreenToConsolePos(c->posX, yRow);
		bool scrollOk = (p.row == lastRow && p.col == 0);
		StepCompleted(step++, scrollOk, scrollOk ? "last-scroll-row hit-test" : "last-scroll-row FAILED");

		float yCmd = (c->posY + 3.0f) + (float)(c->numLines) * c->lineHeight + 1.0f;
		CGuiViewConsole::CConsoleTextPos pc = c->ScreenToConsolePos(c->posX, yCmd);
		bool cmdOk = (pc.row == CGuiViewConsole::CONSOLE_ROW_COMMANDLINE);
		StepCompleted(step++, cmdOk, cmdOk ? "commandline-row hit-test" : "commandline-row FAILED");

		allOk = allOk && scrollOk && cmdOk;
		delete c;
	}

	// --- Task 2: BuildSelectionText spanning screen -> prompt, verbatim ---
	{
		CaptureCallback cc;
		CGuiViewConsole *c = MakeConsole(&cc);
		c->PrintSingleLine((char *)"ABCDE");     // older
		c->PrintSingleLine((char *)"FGHIJ");     // newer (lastRow)
		strcpy(c->commandLine, "XY");
		c->commandLineCursorPos = 2;

		int lastRow = CGuiViewConsole::CONSOLE_ROW_COMMANDLINE - 1; // "FGHIJ"
		int prevRow = lastRow - 1;                                   // "ABCDE"

		// select from col 2 of "ABCDE" through col 1 of the command line "XY"
		c->selAnchor.row = prevRow;
		c->selAnchor.col = 2;
		c->selCaret.row  = CGuiViewConsole::CONSOLE_ROW_COMMANDLINE;
		c->selCaret.col  = 1;
		c->hasSelection = true;

		std::string s = c->BuildSelectionText();
		// rows: "CDE" + "\n" + "FGHIJ" + "\n" + "X"
		bool ok = (s == "CDE\nFGHIJ\nX");

		// no-selection -> empty
		c->hasSelection = false;
		ok = ok && c->BuildSelectionText().empty();

		StepCompleted(step++, ok, ok ? "BuildSelectionText" : "BuildSelectionText FAILED");
		allOk = allOk && ok;
		delete c;
	}

	// --- Task 3: PasteText preserves prefix, executes on newline, leaves tail ---
	{
		CaptureCallback cc;
		CGuiViewConsole *c = MakeConsole(&cc);
		strcpy(c->commandLine, "M ");
		c->commandLineCursorPos = 2;

		c->PasteText("1000\n2000\nDEAD");
		// executes "M 1000", then "2000"; "DEAD" left editable
		bool ok = (cc.executed.size() == 2)
			&& (cc.executed[0] == "M 1000")
			&& (cc.executed[1] == "2000")
			&& (strcmp(c->commandLine, "DEAD") == 0)
			&& (c->commandLineCursorPos == 4);

		// \r is stripped
		CaptureCallback cc2;
		CGuiViewConsole *c2 = MakeConsole(&cc2);
		c2->PasteText("PRINT\r\n");
		ok = ok && (cc2.executed.size() == 1) && (cc2.executed[0] == "PRINT")
			&& (c2->commandLine[0] == 0);

		StepCompleted(step++, ok, ok ? "PasteText" : "PasteText FAILED");
		allOk = allOk && ok;
		delete c; delete c2;
	}

	// --- Task 4: SelectAll bounds + empty console ---
	{
		CaptureCallback cc;
		CGuiViewConsole *c = MakeConsole(&cc);
		c->PrintSingleLine((char *)"ONE");
		c->PrintSingleLine((char *)"TWO");
		strcpy(c->commandLine, "CMD");
		c->commandLineCursorPos = 3;

		c->SelectAll();
		int firstRow = CGuiViewConsole::CONSOLE_ROW_COMMANDLINE - 2; // "ONE"
		bool ok = c->hasSelection
			&& (c->selAnchor.row == firstRow && c->selAnchor.col == 0)
			&& (c->selCaret.row == CGuiViewConsole::CONSOLE_ROW_COMMANDLINE)
			&& (c->selCaret.col == 3);
		std::string s = c->BuildSelectionText();
		ok = ok && (s == "ONE\nTWO\nCMD");

		// empty console -> no selection
		CaptureCallback cc2;
		CGuiViewConsole *e = MakeConsole(&cc2);
		e->SelectAll();
		ok = ok && (e->hasSelection == false);

		StepCompleted(step++, ok, ok ? "SelectAll" : "SelectAll FAILED");
		allOk = allOk && ok;
		delete c; delete e;
	}

	// --- Task 5: appending a line shifts selection rows down by one ---
	{
		CaptureCallback cc;
		CGuiViewConsole *c = MakeConsole(&cc);
		c->PrintSingleLine((char *)"KEEP");
		int row = CGuiViewConsole::CONSOLE_ROW_COMMANDLINE - 1; // "KEEP"
		c->selAnchor.row = row;
		c->selAnchor.col = 1;
		c->selCaret.row  = row;
		c->selCaret.col  = 3;
		c->hasSelection = true;

		c->PrintSingleLine((char *)"NEW");   // shifts buffer up by one
		// "KEEP" moved to row-1; selection must follow so copy is still "EE"
		bool ok = c->hasSelection
			&& (c->selAnchor.row == row - 1)
			&& (c->selCaret.row == row - 1)
			&& (c->BuildSelectionText() == "EE");

		StepCompleted(step++, ok, ok ? "OnLineAppended shift" : "OnLineAppended FAILED");
		allOk = allOk && ok;
		delete c;
	}

	// --- Bug 1: anchor floors to the cell under the cursor ---
	{
		CaptureCallback cc;
		CGuiViewConsole *c = MakeConsole(&cc);
		c->PrintSingleLine((char *)".f 1000 2000");
		int row = CGuiViewConsole::CONSOLE_ROW_COMMANDLINE - 1;
		float yRow = (c->posY + 3.0f) + (float)(c->numLines - 1) * c->lineHeight + 1.0f;

		// x at ~90% into the 'f' cell (index 1). Left edge of 'f' = RowColToPixelX(row,1).
		float fLeft = c->RowColToPixelX(row, 1);
		float fW = c->font->GetCharWidth('f', c->fontScale);
		float xInF = fLeft + fW * 0.9f;

		// floor (anchor) must give col 1 ('f'); nearest must give col 2 (snaps past half-width).
		CGuiViewConsole::CConsoleTextPos pFloor = c->ScreenToConsolePos(xInF, yRow, false);
		CGuiViewConsole::CConsoleTextPos pNear  = c->ScreenToConsolePos(xInF, yRow, true);
		bool ok = (pFloor.row == row && pFloor.col == 1) && (pNear.col == 2);

		// end-to-end: select from the floored 'f' to end of line, copy includes 'f'
		c->selAnchor = pFloor;
		c->selCaret = { row, (int)strlen(".f 1000 2000") };
		c->hasSelection = true;
		ok = ok && (c->BuildSelectionText() == "f 1000 2000");

		StepCompleted(step++, ok, ok ? "anchor floors to cell" : "anchor floor FAILED");
		allOk = allOk && ok;
		delete c;
	}

	TestCompleted(allOk, allOk ? "selection tests passed" : "selection tests failed");
	this->isRunning = false;
}
