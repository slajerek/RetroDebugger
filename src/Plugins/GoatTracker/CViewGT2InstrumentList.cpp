#include "CViewGT2InstrumentList.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "CGT2FontAtlas.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2Favorites.h"
#include "CViewGT2InstrumentsBrowser.h"
#include "CGuiMain.h"
#include "SYS_FileSystem.h"
#include "imgui.h"
#include "imgui_internal.h"   // BeginDragDropTargetCustom -- the rows are drawn by hand
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "ginstr.h"
#include "goattrk2.h"
#include "ginstrops.h"
extern int cursorcolortable[];
}

static INSTRPACKAGE gInstrListClipboard;

#define CNORMAL  8
#define CEDIT    10

static const int GT2_INSTR_LIST_TEXT_COLS = 21;

static ImU32 GT2InstrumentListSelectedBgColor()
{
	return IM_COL32(18, 48, 108, 230);
}

static ImU32 GT2InstrumentListFrameColor()
{
	return IM_COL32(95, 125, 180, 145);
}

static void DrawInstrumentListTextGT2(ImDrawList *dl, CGT2FontAtlas *font,
	float px, float py, u8 colorIndex, const char *text, bool selected)
{
	if (!selected)
	{
		DrawTextGT2(dl, font, px, py, colorIndex, text);
		return;
	}

	ImU32 fgColor = font->palette[colorIndex & 0x0F];
	ImU32 bgColor = GT2InstrumentListSelectedBgColor();
	float cw = GT2CellW();
	for (int i = 0; text[i]; i++)
	{
		DrawCharGT2(dl, font, px + i * cw, py, (u8)text[i], fgColor, bgColor);
	}
}

CViewGT2InstrumentList::CViewGT2InstrumentList(const char *name, float posX, float posY, float posZ,
	float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
	this->renaming = false;
	this->renameFocusPending = false;
	this->renameBuffer[0] = 0;
	this->lastRenderOriginX = 0.0f;
	this->lastRenderOriginY = 0.0f;
	this->lastScrolledToInstrument = -1;
}

CViewGT2InstrumentList::~CViewGT2InstrumentList() {}

void CViewGT2InstrumentList::SelectInstrument(int instrumentNum)
{
	if (instrumentNum < 0) return;
	if (instrumentNum >= MAX_INSTR) return;

	einum = instrumentNum;
	showinstrtable();
}

int CViewGT2InstrumentList::GetInstrumentGridRow(int instrumentNum)
{
	return instrumentNum - 1;
}

GT2InstrumentListRect CViewGT2InstrumentList::GetInstrumentRowBackgroundRect(float originX, float originY, int row)
{
	float xPad = GT2CellW() * 0.35f;
	GT2InstrumentListRect rect;
	rect.x1 = originX - xPad;
	rect.y1 = originY + GT2RowToPixel(row);
	rect.x2 = originX + GT2ColToPixel(GT2_INSTR_LIST_TEXT_COLS) + xPad;
	rect.y2 = originY + GT2RowToPixel(row + 1);
	return rect;
}

GT2InstrumentListRect CViewGT2InstrumentList::GetInstrumentListFrameRect(float originX, float originY)
{
	float xPad = GT2CellW() * 0.75f;
	float yPad = GT2EffectiveUIScale() * 2.0f;
	GT2InstrumentListRect rect;
	rect.x1 = originX - xPad;
	rect.y1 = originY + GT2RowToPixel(0) - yPad;
	rect.x2 = originX + GT2ColToPixel(GT2_INSTR_LIST_TEXT_COLS) + xPad;
	rect.y2 = originY + GT2RowToPixel(GT2_LAST_INSTR) + yPad;
	return rect;
}

void CViewGT2InstrumentList::RenderImGui()
{
	PreRenderImGui();
	if (!fontAtlas->TryLoad()) { PostRenderImGui(); return; }
	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec2 framePadding(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale);
	ImVec2 itemSpacing(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale);
	ImVec2 itemInnerSpacing(style.ItemInnerSpacing.x * uiScale, style.ItemInnerSpacing.y * uiScale);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, itemInnerSpacing);

	ImDrawList *dl = ImGui::GetWindowDrawList();
	// Visible height, before anything is drawn -- used for the scrolling below.
	float visibleHeight = ImGui::GetContentRegionAvail().y;

	ImVec2 origin = ImGui::GetCursorScreenPos();
	lastRenderOriginX = origin.x;
	lastRenderOriginY = origin.y;
	char textbuffer[64];
	GT2InstrumentListRect frameRect = GetInstrumentListFrameRect(origin.x, origin.y);
	dl->AddRectFilled(
		ImVec2(frameRect.x1, frameRect.y1),
		ImVec2(frameRect.x2, frameRect.y2),
		IM_COL32(8, 14, 30, 55),
		2.0f * uiScale);
	dl->AddRect(
		ImVec2(frameRect.x1, frameRect.y1),
		ImVec2(frameRect.x2, frameRect.y2),
		GT2InstrumentListFrameColor(),
		2.0f * uiScale,
		0,
		1.0f * uiScale);

	for (int n = 1; n <= GT2_LAST_INSTR; n++)
	{
		int row = GetInstrumentGridRow(n);
		bool selected = n == einum;
		u8 color = selected ? CEDIT : CNORMAL;
		const char *usedMark = instr_referenced_in_patterns(n) ? "*" : " ";
		if (selected)
		{
			GT2InstrumentListRect rowRect = GetInstrumentRowBackgroundRect(origin.x, origin.y, row);
			dl->AddRectFilled(
				ImVec2(rowRect.x1, rowRect.y1),
				ImVec2(rowRect.x2, rowRect.y2),
				GT2InstrumentListSelectedBgColor(),
				2.0f * uiScale);
		}

		if (renaming && n == einum)
		{
			// Draw the NN index as normal text; replace the name field with InputText.
			snprintf(textbuffer, sizeof(textbuffer), "%02X", n);
			DrawInstrumentListTextGT2(dl, fontAtlas, origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(row), color, textbuffer, selected);

			ImGui::SetCursorScreenPos(ImVec2(origin.x + GT2ColToPixel(3),
				origin.y + GT2RowToPixel(row)));
			ImGui::SetNextItemWidth(GT2ColToPixel(17) - GT2ColToPixel(3));
			if (renameFocusPending)
			{
				ImGui::SetKeyboardFocusHere();
				renameFocusPending = false;
			}
			bool done = ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if (done) CommitRename();
			if (ImGui::IsKeyPressed(ImGuiKey_Escape)) renaming = false;
			if (!ImGui::IsItemActive() && !ImGui::IsItemFocused() && renaming
				&& !ImGui::IsItemDeactivatedAfterEdit())
			{
				// focus lost -> commit
				CommitRename();
			}

			// Draw the used-mark after the name field.
			snprintf(textbuffer, sizeof(textbuffer), " %s", usedMark);
			DrawInstrumentListTextGT2(dl, fontAtlas, origin.x + GT2ColToPixel(19),
				origin.y + GT2RowToPixel(row), color, textbuffer, selected);
		}
		else
		{
			snprintf(textbuffer, sizeof(textbuffer), "%02X %-16s %s", n, ginstr[n].name, usedMark);
			DrawInstrumentListTextGT2(dl, fontAtlas, origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(row), color, textbuffer, selected);
		}
	}

	// The rows are drawn by hand, so the drop targets are registered as
	// explicit rectangles rather than riding on a widget's item rect. Only
	// the row under the cursor is registered -- ImGui needs one target, and
	// this keeps the highlight on the slot that will actually receive the
	// instrument.
	if (ImGui::IsDragDropActive())
	{
		int hoverRow = GT2PixelToRow(guiMain->mousePosY - origin.y);
		if (hoverRow >= 0 && hoverRow < GT2_LAST_INSTR)
		{
			GT2InstrumentListRect rowRect = GetInstrumentRowBackgroundRect(origin.x, origin.y, hoverRow);
			ImRect dropRect(ImVec2(rowRect.x1, rowRect.y1), ImVec2(rowRect.x2, rowRect.y2));
			ImGuiID dropId = ImGui::GetCurrentWindow()->GetID("##gt2InstrDrop");
			if (ImGui::BeginDragDropTargetCustom(dropRect, dropId))
			{
				const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(GT2_INSTRUMENT_FILE_PAYLOAD);
				if (payload != NULL && payload->Data != NULL)
				{
					const char *droppedPath = (const char *)payload->Data;
					if (pluginGoatTracker != NULL)
						pluginGoatTracker->LoadInstrumentFromFile(droppedPath, hoverRow + 1,
							pluginGoatTracker->viewInstrumentsBrowser != NULL
								? pluginGoatTracker->viewInstrumentsBrowser->previewOnClick : false);
				}
				ImGui::EndDragDropTarget();
			}
			dl->AddRect(ImVec2(rowRect.x1, rowRect.y1), ImVec2(rowRect.x2, rowRect.y2),
				IM_COL32(255, 210, 90, 220), 2.0f * uiScale, 0, 1.5f * uiScale);
		}
	}

	// Left click selects the current instrument.
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 m = ImGui::GetIO().MousePos;
		int gridRow = GT2PixelToRow(m.y - origin.y);
		if (gridRow >= 0 && gridRow < GT2_LAST_INSTR)
			SelectInstrument(gridRow + 1);
	}

	// Right click opens the context menu, navigating to the clicked instrument first.
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImVec2 m = ImGui::GetIO().MousePos;
		int gridRow = GT2PixelToRow(m.y - origin.y);
		if (gridRow >= 0 && gridRow < GT2_LAST_INSTR) SelectInstrument(gridRow + 1);
		ImGui::OpenPopup("gt2InstrCtx");
	}
	// Tell ImGui how tall the hand-drawn content actually is. Everything above
	// goes straight to the draw list, so without this the window believes it
	// holds nothing: no scrollbar, and the mouse wheel does nothing at all --
	// while the list is 62 instruments tall and does not fit any sane window.
	// GetCursorScreenPos() at the top already carries the scroll offset, so the
	// drawing and the click hit-tests follow it for free.
	ImGui::SetCursorScreenPos(origin);
	ImGui::Dummy(ImVec2(frameRect.x2 - frameRect.x1, frameRect.y2 - frameRect.y1));

	// Follow a selection made from outside the list (keyboard, the pattern
	// editor) into view. Only when it actually changed, so scrolling the wheel
	// to look elsewhere is not yanked back on the next frame -- the same rule
	// CViewGT2Tables uses for its cursor.
	if (einum != lastScrolledToInstrument)
	{
		lastScrolledToInstrument = einum;
		if (einum >= 1 && einum <= GT2_LAST_INSTR && visibleHeight > 0.0f)
		{
			float rowTop = GT2RowToPixel(GetInstrumentGridRow(einum));
			float rowBottom = rowTop + GT2CellH();
			float scroll = ImGui::GetScrollY();
			if (rowTop < scroll)
				ImGui::SetScrollY(rowTop);
			else if (rowBottom > scroll + visibleHeight)
				ImGui::SetScrollY(rowBottom - visibleHeight);
		}
	}

	ImGui::PopStyleVar(3);

	// Context-menu popups intentionally keep the app-wide ImGui scale.
	RenderContextMenu();

	PostRenderImGui();
}

bool CViewGT2InstrumentList::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (!isAlt && !isShift && (isControl || isSuper))
	{
		if (keyCode == 'x' || keyCode == 'X' || keyCode == SDLK_X)
		{
			instrpackage_capture(einum, &gInstrListClipboard);
			freeinstrtable_partial(einum);
			clearinstr(einum);
			return true;
		}
		if (keyCode == 'c' || keyCode == 'C' || keyCode == SDLK_C)
		{
			instrpackage_capture(einum, &gInstrListClipboard);
			return true;
		}
		if (keyCode == 'v' || keyCode == 'V' || keyCode == SDLK_V)
		{
			if (gInstrListClipboard.valid) instrpackage_apply(einum, &gInstrListClipboard);
			return true;
		}
		if (keyCode == 'r' || keyCode == 'R' || keyCode == SDLK_R)
		{
			BeginRename();
			return true;
		}
	}
	// Instrument-name rename / native ginstr.c selection flows need raw
	// keys at native side. Forward unconditionally.
	return GT2_HandleRenoiseOrForwardKeyDownToNative(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2InstrumentList::DoDropFile(char *filePath)
{
	if (filePath == NULL) return false;
	if (!CViewGT2InstrumentsBrowser::IsInstrumentFile(filePath)) return false;
	if (pluginGoatTracker == NULL) return false;

	// The drop carries no coordinates of its own; guiMain kept the cursor
	// position it used to pick this view.
	int gridRow = GT2PixelToRow(guiMain->mousePosY - lastRenderOriginY);
	if (gridRow < 0 || gridRow >= GT2_LAST_INSTR) return false;

	bool preview = pluginGoatTracker->viewInstrumentsBrowser != NULL
		? pluginGoatTracker->viewInstrumentsBrowser->previewOnClick : false;
	return pluginGoatTracker->LoadInstrumentFromFile(filePath, gridRow + 1, preview);
}

bool CViewGT2InstrumentList::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2InstrumentList::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewGT2InstrumentList::RenderContextMenu()
{
	if (ImGui::BeginPopup("gt2InstrCtx"))
	{
		if (ImGui::MenuItem("Insert"))
		{
			// insertinstrument returns 0 if the table is full; do nothing in that case.
			insertinstrument(einum);
		}
		if (ImGui::MenuItem("Delete"))   deleteinstrument(einum);
		if (ImGui::MenuItem("Clear"))
		{
			freeinstrtable_partial(einum);
			clearinstr(einum);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Cut", "Cmd+X"))
		{
			instrpackage_capture(einum, &gInstrListClipboard);
			freeinstrtable_partial(einum);
			clearinstr(einum);
		}
		if (ImGui::MenuItem("Copy", "Cmd+C"))
			instrpackage_capture(einum, &gInstrListClipboard);
		if (ImGui::MenuItem("Paste", "Cmd+V", false, gInstrListClipboard.valid))
			instrpackage_apply(einum, &gInstrListClipboard);
		ImGui::Separator();
		if (ImGui::MenuItem("Rename", "Cmd+R")) BeginRename();
		ImGui::Separator();
		if (ImGui::MenuItem("Delete unused instruments"))
			deleteunusedinstruments();
		ImGui::Separator();
		if (ImGui::MenuItem("Load instrument..."))  pluginGoatTracker->OpenLoadInstrumentDialog();
		if (ImGui::MenuItem("Save instrument as...")) pluginGoatTracker->OpenSaveInstrumentDialog();
		ImGui::Separator();
		if (ImGui::BeginMenu("Favorite instruments"))
		{
			CGT2Favorites *fav = pluginGoatTracker->gt2Favorites;
			for (int i = 0; i < (int)fav->entries.size(); i++)
			{
				ImGui::PushID(i);
				if (ImGui::SmallButton("x")) { fav->RemoveAt(i); ImGui::PopID(); break; }
				ImGui::SameLine();
				if (ImGui::MenuItem(fav->entries[i].displayName.c_str()))
					fav->ApplyTo(einum, i);
				ImGui::PopID();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Add instrument to favorites"))
				fav->AddFromInstrument(einum);
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
}

void CViewGT2InstrumentList::BeginRename()
{
	if (einum < 1) return;
	memcpy(renameBuffer, ginstr[einum].name, MAX_INSTRNAMELEN);
	renameBuffer[MAX_INSTRNAMELEN] = 0;     // force NUL-termination
	renaming = true;
	renameFocusPending = true;
}

void CViewGT2InstrumentList::CommitRename()
{
	if (!renaming) return;
	memset(ginstr[einum].name, 0, MAX_INSTRNAMELEN);
	// editstring caps names at MAX_INSTRNAMELEN-1 visible chars.
	strncpy(ginstr[einum].name, renameBuffer, MAX_INSTRNAMELEN - 1);
	renaming = false;
}
