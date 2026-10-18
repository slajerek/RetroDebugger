#include "CSourceRendering.h"
#include "CSyntaxLexer.h"
#include <vector>

// one colour per SyntaxCategory, readable on the dark theme
const ImVec4 kSyntaxColors[] = {
	ImVec4(0.86f, 0.86f, 0.86f, 1.0f),	// Text
	ImVec4(0.80f, 0.55f, 0.95f, 1.0f),	// Keyword
	ImVec4(0.40f, 0.75f, 0.95f, 1.0f),	// Type
	ImVec4(0.70f, 0.90f, 0.60f, 1.0f),	// Number
	ImVec4(0.95f, 0.75f, 0.45f, 1.0f),	// String
	ImVec4(0.50f, 0.60f, 0.50f, 1.0f),	// Comment
	ImVec4(0.90f, 0.55f, 0.55f, 1.0f),	// Preprocessor
	ImVec4(0.75f, 0.75f, 0.75f, 1.0f),	// Operator
	ImVec4(0.95f, 0.90f, 0.50f, 1.0f),	// Label
	ImVec4(0.45f, 0.85f, 0.95f, 1.0f),	// Mnemonic
	ImVec4(0.90f, 0.55f, 0.80f, 1.0f),	// Directive
};

const ImU32 kColorBreakpoint = IM_COL32(230, 60, 60, 255);
const ImU32 kColorPc = IM_COL32(250, 220, 60, 255);
const ImU32 kColorPcLine = IM_COL32(120, 110, 30, 110);
const ImU32 kColorSelectedLine = IM_COL32(70, 110, 160, 90);
const ImU32 kColorCodeDot = IM_COL32(110, 160, 110, 255);
const ImVec4 kColorLineNumber(0.50f, 0.50f, 0.50f, 1.0f);
const ImVec4 kColorWarning(0.95f, 0.75f, 0.30f, 1.0f);

void SourceRenderLineText(const std::string &text, const CSyntaxLexer *lexer, int *state)
{
	if (lexer == NULL)
	{
		ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
		return;
	}
	std::vector<SyntaxSpan> spans;
	lexer->LexLine(text, state, spans);
	const char *base = text.c_str();
	int at = 0;
	bool first = true;
	auto emit = [&](int start, int end, const ImVec4 &color) {
		if (end <= start)
			return;
		if (!first)
			ImGui::SameLine(0.0f, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(base + start, base + end);
		ImGui::PopStyleColor();
		first = false;
	};
	for (const SyntaxSpan &span : spans)
	{
		emit(at, span.start, kSyntaxColors[(int)SyntaxCategory::Text]);
		emit(span.start, span.start + span.length, kSyntaxColors[(int)span.category]);
		at = span.start + span.length;
	}
	emit(at, (int)text.size(), kSyntaxColors[(int)SyntaxCategory::Text]);
	if (first)
		ImGui::TextUnformatted("");
}
