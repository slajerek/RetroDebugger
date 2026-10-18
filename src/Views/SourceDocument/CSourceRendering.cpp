#include "CSourceRendering.h"
#include <cctype>
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

// the identifier of TEXT[start, end) under the mouse, the item just drawn
static void HoveredIdentifier(const char *base, int start, int end, std::string *hovered)
{
	if (hovered == NULL || !ImGui::IsItemHovered())
		return;
	float x = ImGui::GetIO().MousePos.x - ImGui::GetItemRectMin().x;
	int at = start;
	while (at < end && ImGui::CalcTextSize(base + start, base + at + 1).x <= x)
		at++;
	auto identifier = [](char c) { return isalnum((unsigned char)c) || c == '_'; };
	if (at >= end || !identifier(base[at]))
		return;
	int first = at, last = at;
	while (first > start && identifier(base[first - 1]))
		first--;
	while (last + 1 < end && identifier(base[last + 1]))
		last++;
	if (!isdigit((unsigned char)base[first]))
		hovered->assign(base + first, base + last + 1);
}

void SourceRenderLineText(const std::string &text, const CSyntaxLexer *lexer, int *state, std::string *hovered)
{
	if (lexer == NULL)
	{
		ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
		HoveredIdentifier(text.c_str(), 0, (int)text.size(), hovered);
		return;
	}
	std::vector<SyntaxSpan> spans;
	lexer->LexLine(text, state, spans);
	const char *base = text.c_str();
	int at = 0;
	bool first = true;
	auto emit = [&](int start, int end, const ImVec4 &color, bool identifiers) {
		if (end <= start)
			return;
		if (!first)
			ImGui::SameLine(0.0f, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(base + start, base + end);
		ImGui::PopStyleColor();
		if (identifiers)
			HoveredIdentifier(base, start, end, hovered);
		first = false;
	};
	for (const SyntaxSpan &span : spans)
	{
		emit(at, span.start, kSyntaxColors[(int)SyntaxCategory::Text], true);
		emit(span.start, span.start + span.length, kSyntaxColors[(int)span.category], span.category == SyntaxCategory::Text);
		at = span.start + span.length;
	}
	emit(at, (int)text.size(), kSyntaxColors[(int)SyntaxCategory::Text], true);
	if (first)
		ImGui::TextUnformatted("");
}
