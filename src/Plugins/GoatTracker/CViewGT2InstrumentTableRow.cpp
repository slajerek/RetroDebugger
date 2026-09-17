#include "CViewGT2InstrumentTableRow.h"
#include "CViewGT2Instrument.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2FontAtlas.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "imgui.h"

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "ginstr.h"
#include "gtable.h"
#include "goattrk2.h"
}

CViewGT2InstrumentTableRow::CViewGT2InstrumentTableRow(const char *name, float posX, float posY, float posZ,
													   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
}

CViewGT2InstrumentTableRow::~CViewGT2InstrumentTableRow()
{
}

bool CViewGT2InstrumentTableRow::CursorHasRow(int tableNum, int rowPos)
{
	if (tableNum < 0 || tableNum >= MAX_TABLES) return false;
	if (rowPos < 0 || rowPos >= MAX_TABLELEN) return false;
	if (einum < 1 || einum >= MAX_INSTR) return false;

	// The row has to fall inside the edited instrument's own slice of the
	// shared pool -- the cursor can otherwise sit on rows belonging to another
	// instrument, which this view must not offer to edit.
	int start = ginstr[einum].ptr[tableNum] ? ginstr[einum].ptr[tableNum] - 1 : -1;
	if (start < 0) return false;
	int len = gettablepartlen(tableNum, start);
	return GT2WtblContextHasValidRow(start, len, rowPos);
}

bool CViewGT2InstrumentTableRow::CursorCanEdit(int tableNum, int rowPos)
{
	// Speedtable rows are atomic; the context menu keeps them read-only too.
	if (tableNum == STBL) return false;
	return CursorHasRow(tableNum, rowPos);
}

void CViewGT2InstrumentTableRow::RenderImGui()
{
	PreRenderImGui();

	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
						ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));

	// The help block is a fixed-width reference table -- it is as wide as it is
	// and does not reflow. In a popup that is fine because the popup sizes
	// itself to its content; a docked view of any width would just clip it, so
	// scroll instead of losing the right-hand column.
	ImGui::BeginChild("##gt2TableRowContent", ImVec2(0, 0), false,
					  ImGuiWindowFlags_HorizontalScrollbar);

	CViewGT2Instrument *instrumentView =
		pluginGoatTracker ? pluginGoatTracker->viewInstrument : NULL;

	if (instrumentView == NULL)
	{
		ImGui::TextDisabled("GT2 instrument view is not available.");
	}
	else if (etnum < 0 || etnum >= MAX_TABLES)
	{
		ImGui::TextDisabled("Put the cursor on an instrument table row.");
	}
	else
	{
		// Which instrument and row this is about -- the view is read at a
		// glance while the cursor moves, so say what it is showing.
		// Say which byte the cursor is on, so the highlight below has a label.
		ImGui::TextDisabled("Instrument %02X   row %d   %s", einum, etpos,
							(etcolumn <= 1) ? "left" : "right");
		ImGui::Separator();

		// The same block the context menu renders, from the same function.
		instrumentView->RenderTableRowHelp(etnum, etpos,
										   CursorHasRow(etnum, etpos),
										   CursorCanEdit(etnum, etpos),
										   etcolumn);
	}

	ImGui::EndChild();

	ImGui::PopStyleVar(2);

	GT2_PropagateChildWindowFocus(this);
	PostRenderImGui();
}

bool CViewGT2InstrumentTableRow::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Nothing of its own: the cursor lives in the instrument / tables views, so
	// keys go where they always go.
	return GT2_HandleRenoiseOrForwardKeyDownToNative(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2InstrumentTableRow::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2InstrumentTableRow::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
