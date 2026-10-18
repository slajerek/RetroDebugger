#ifndef _CSourceRendering_h_
#define _CSourceRendering_h_

// The look of source text, shared by the Source code views and the project
// editor (RD-018): one colour per SyntaxCategory, the gutter's marks, and a
// line drawn in its lexer's colours.

#include "imgui.h"
#include <string>

class CSyntaxLexer;

extern const ImVec4 kSyntaxColors[];
extern const ImU32 kColorBreakpoint;
extern const ImU32 kColorPc;
extern const ImU32 kColorPcLine;
extern const ImU32 kColorSelectedLine;
extern const ImU32 kColorCodeDot;
extern const ImVec4 kColorLineNumber;
extern const ImVec4 kColorWarning;

// TEXT as one row of coloured runs; STATE carries the lexer across lines
// HOVERED (may be NULL): set to the identifier under the mouse, if any (RD-022)
void SourceRenderLineText(const std::string &text, const CSyntaxLexer *lexer, int *state, std::string *hovered = NULL);

#endif
