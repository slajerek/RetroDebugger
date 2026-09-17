#include "CViewGT2PatternRow.h"
#include "CViewGT2Patterns.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2FontAtlas.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "imgui.h"

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "gpattern.h"
#include "goattrk2.h"
}

CViewGT2PatternRow::CViewGT2PatternRow(const char *name, float posX, float posY, float posZ,
									   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
}

CViewGT2PatternRow::~CViewGT2PatternRow()
{
}

bool CViewGT2PatternRow::CursorRowIsEditable()
{
	if (epchn < 0 || epchn >= MAX_CHN) return false;
	int pn = epnum[epchn];
	if (pn < 0 || pn >= MAX_PATT) return false;
	if (eppos < 0 || eppos >= pattlen[pn]) return false;
	// The ENDPATT marker is not a row anything may be written to.
	return pattern[pn][eppos * 4] != ENDPATT;
}

void CViewGT2PatternRow::RenderImGui()
{
	PreRenderImGui();

	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
						ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));

	// The command picker is a 4-column table of fixed-width chips and the
	// argument editors are rows of nibble buttons -- none of it reflows, so a
	// narrow window would clip the right-hand column rather than wrap it.
	ImGui::BeginChild("##gt2PatternRowContent", ImVec2(0, 0), false,
					  ImGuiWindowFlags_HorizontalScrollbar);

	CViewGT2Patterns *patternsView =
		pluginGoatTracker ? pluginGoatTracker->viewPatterns : NULL;

	if (patternsView == NULL)
	{
		ImGui::TextDisabled("GT2 patterns view is not available.");
	}
	else if (epchn < 0 || epchn >= MAX_CHN)
	{
		ImGui::TextDisabled("Put the cursor on a pattern row.");
	}
	else
	{
		// Which cell this is about -- the view is read at a glance while the
		// cursor moves, so say what it is showing.
		ImGui::TextDisabled("Channel %d   pattern %02X   row %02X",
							epchn + 1, epnum[epchn], eppos);
		ImGui::Separator();

		// The same block the context menu renders, from the same function.
		// It reads the live cursor (epchn / eppos) and handles a row that
		// cannot be edited itself, so nothing is gated here.
		patternsView->RenderPatternCommandEditor();
	}

	ImGui::EndChild();

	ImGui::PopStyleVar(2);

	GT2_PropagateChildWindowFocus(this);
	PostRenderImGui();
}

bool CViewGT2PatternRow::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Nothing of its own: the cursor lives in the patterns view, so keys go
	// where they always go.
	return GT2_HandleRenoiseOrForwardKeyDownToNative(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2PatternRow::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2PatternRow::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
