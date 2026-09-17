#include "CViewGT2Instrument.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "CGT2FontAtlas.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2InstrumentsBrowser.h"
#include "imgui.h"
#include "imgui_internal.h"   // BeginDragDropTargetCustom / ImRect
#include "SYS_KeyCodes.h"
#include <cstdio>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "ginstr.h"
#include "goattrk2.h"
#include "gtable.h"

extern int cursorcolortable[];
extern unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
extern unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
extern int etnum, etpos, etcolumn;
}

// tableViewOffset[] in the header is sized without gcommon.h in scope.
static_assert(MAX_TABLES == 4, "CViewGT2Instrument::tableViewOffset[] must match MAX_TABLES");

// GT2 color constants (from gdisplay.h)
#define CNORMAL  8
#define CEDIT    10
#define CCOMMAND 7
#define CTITLE   15

// Edit mode constants (from goattrk2.h)
#define EDIT_INSTRUMENT 2
#define EDIT_TABLES     3

int GT2WtblSelectedWaveformIndex(unsigned char leftValue)
{
	if (leftValue < 0x10 || leftValue > 0xEF)
		return -1;
	return ((int)leftValue >> 4) - 1;
}

int GT2WtblSelectedCommandIndex(unsigned char leftValue)
{
	if (leftValue < 0xF0 || leftValue > 0xFE)
		return -1;
	return (int)leftValue - 0xF0;
}

unsigned char GT2WtblApplyWaveformSelection(unsigned char leftValue, int waveformIndex)
{
	if (waveformIndex < 0) waveformIndex = 0;
	if (waveformIndex > 13) waveformIndex = 13;
	int controlBits = GT2WtblSelectedWaveformIndex(leftValue) >= 0
		? (leftValue & 0x0F) : 0x01;
	return (unsigned char)(((waveformIndex + 1) << 4) | controlBits);
}

unsigned char GT2WtblApplyCommandSelection(int commandIndex)
{
	if (commandIndex < 0) commandIndex = 0;
	if (commandIndex > 14) commandIndex = 14;
	return (unsigned char)(0xF0 + commandIndex);
}

bool GT2WtblContextHasValidRow(int tableStart, int tableLength, int position)
{
	return tableStart >= 0 && tableLength > 0
		&& position >= tableStart && position < tableStart + tableLength;
}

int GT2WtblContextRowFromClick(int tableStart, int tableLength, int clickedOffset)
{
	if (tableStart < 0 || tableLength <= 0
		|| clickedOffset < 0 || clickedOffset >= tableLength)
		return -1;
	return tableStart + clickedOffset;
}

bool GT2WtblContextShouldEnterEditMode(int tableStart, int tableLength, int clickedOffset)
{
	return GT2WtblContextShouldCreateRowOnSelection(tableStart, tableLength, clickedOffset)
		|| GT2WtblContextRowFromClick(tableStart, tableLength, clickedOffset) >= 0;
}

bool GT2WtblContextShouldCreateRowOnSelection(int tableStart, int tableLength, int clickedOffset)
{
	return tableStart < 0 && tableLength <= 0 && clickedOffset == 0;
}

int GT2WtblContextSelectableFlags()
{
	// Keep the popup open after a click so the user can keep choosing
	// passband / waveform / command without having to right-click again.
	// ESC closes it (see CViewGT2Instrument::KeyDown).
	return ImGuiSelectableFlags_NoAutoClosePopups;
}

int GT2WtblContextTextColorMode(bool selected, bool hovered)
{
	if (selected)
		return 2;
	if (hovered)
		return 1;
	return 0;
}

int GT2InstrumentTableFromGridCol(int gridCol)
{
	if (gridCol < 0)
		return -1;
	int tableNum = gridCol / 10;
	int colInTable = gridCol - tableNum * 10;
	if (tableNum < 0 || tableNum >= MAX_TABLES || colInTable < 0 || colInTable > 7)
		return -1;
	return tableNum;
}

bool GT2InstrumentTableCursorStep(int direction, const int *sliceLen,
								  int *inOutTable, int *inOutRow, int *inOutColumn)
{
	if (direction != 1 && direction != -1) return false;
	if (sliceLen == NULL || inOutTable == NULL || inOutRow == NULL || inOutColumn == NULL)
		return false;

	int table = *inOutTable;
	if (table < 0 || table >= MAX_TABLES) return false;

	// Four columns per table -- left byte hi/lo, right byte hi/lo -- same as
	// native gtable.c. Inside them nothing but the column moves.
	int column = *inOutColumn + direction;
	if (column >= 0 && column <= 3)
	{
		*inOutColumn = column;
		return true;
	}

	// Ran off the edge: hop to the next table THIS instrument owns rows in,
	// skipping the ones it does not. The last candidate is the current table
	// itself, so an instrument with a single populated table wraps within it
	// instead of escaping into the pool.
	for (int i = 1; i <= MAX_TABLES; i++)
	{
		int t = (table + direction * i) % MAX_TABLES;
		if (t < 0) t += MAX_TABLES;
		if (sliceLen[t] <= 0) continue;

		int row = *inOutRow;
		if (row < 0) row = 0;
		if (row > sliceLen[t] - 1) row = sliceLen[t] - 1;

		*inOutTable  = t;
		*inOutRow    = row;
		*inOutColumn = (direction > 0) ? 0 : 3;
		return true;
	}
	return false;
}

// Per-table help shown (as ImGui tables) in the right-click context menu.
// Which byte of the table row a help line is about, so the line describing
// what the cursor is actually on can be picked out. etcolumn 0/1 are the left
// byte's nybbles and 2/3 the right byte's (gtable.c), which is the whole
// mapping. GT2_HELP_BOTH is for lines that genuinely describe both bytes at
// once -- the pulse/filter modes, where the left value selects the mode and
// the right is its parameter.
enum
{
	GT2_HELP_LEFT = 0,
	GT2_HELP_RIGHT,
	GT2_HELP_BOTH,
};

// `loValue`/`hiValue` are the range of LEFT-byte values the line describes, so
// only the line that actually applies to the row under the cursor lights up.
// -1/-1 means the line has no value range (it describes the right byte, or a
// speedtable usage rather than a byte value) and follows its column alone.
struct GT2HelpRow { const char *key; const char *desc; int column; int loValue; int hiValue; };

static const char *GT2_TableIntro[MAX_TABLES] =
{
	"Sets the voice's waveform and pitch over time.",
	"Modulates the pulse width over time.",
	"Modulates the SID filter over time.",
	"Speed parameters for vibrato / portamento / funktempo.",
};

static const GT2HelpRow GT2_WtblRows[] =
{
	{ "left $00-$0F", "delay (wait that many frames)",              GT2_HELP_LEFT,  0x00, 0x0F },
	{ "left $10-$EF", "waveform byte (see below)",                  GT2_HELP_LEFT,  0x10, 0xEF },
	{ "left $F0-$FE", "command (see below)",                        GT2_HELP_LEFT,  0xF0, 0xFE },
	{ "left $FF",     "jump, right = target ($00 = end)",           GT2_HELP_LEFT,  0xFF, 0xFF },
	{ "right",        "note: $00-$7F relative, $81-$DF absolute",   GT2_HELP_RIGHT,   -1,   -1 },
	{ 0, 0, 0, 0, 0 },
};
static const GT2HelpRow GT2_PtblRows[] =
{
	{ "left $01-$7F", "modulate (left = frames, right = speed)",    GT2_HELP_BOTH,  0x01, 0x7F },
	{ "left $80-$FE", "set pulse width directly (12-bit)",          GT2_HELP_BOTH,  0x80, 0xFE },
	{ "left $FF",     "jump, right = target ($00 = stop)",          GT2_HELP_BOTH,  0xFF, 0xFF },
	{ 0, 0, 0, 0, 0 },
};
static const GT2HelpRow GT2_FtblRows[] =
{
	{ "left $00",     "set cutoff (right = cutoff value)",          GT2_HELP_BOTH,  0x00, 0x00 },
	{ "left $01-$7F", "modulate cutoff (left = frames, right = speed)", GT2_HELP_BOTH, 0x01, 0x7F },
	{ "left $80-$FE", "set passband / resonance / routing",         GT2_HELP_BOTH,  0x80, 0xFE },
	{ "left $FF",     "jump, right = target ($00 = stop)",          GT2_HELP_BOTH,  0xFF, 0xFF },
	{ 0, 0, 0, 0, 0 },
};
static const GT2HelpRow GT2_StblRows[] =
{
	// No value ranges: a speedtable row is read as vibrato / portamento /
	// funktempo depending on what points at it, not on the byte it holds.
	{ "vibrato",    "left = speed, right = depth",                  GT2_HELP_BOTH,    -1,   -1 },
	{ "portamento", "left:right = 16-bit speed per tick",           GT2_HELP_BOTH,    -1,   -1 },
	{ "funktempo",  "left & right = the two tempo values",          GT2_HELP_BOTH,    -1,   -1 },
	{ 0, 0, 0, 0, 0 },
};
static const GT2HelpRow *GT2_TableRows[MAX_TABLES] =
	{ GT2_WtblRows, GT2_PtblRows, GT2_FtblRows, GT2_StblRows };

// Wavetable waveform mixes (high nibble) and commands ($F0-$FE).
static const char *GT2_Waveforms[14] =
{
	"$1 triangle",        "$2 sawtooth",
	"$3 tri+saw",         "$4 pulse",
	"$5 tri+pulse",       "$6 saw+pulse",
	"$7 tri+saw+pulse",   "$8 noise",
	"$9 tri+noise",       "$A saw+noise",
	"$B tri+saw+noise",   "$C pulse+noise",
	"$D tri+pulse+noise", "$E saw+pulse+noise",
};
static const char *GT2_WaveCmds[15] =
{
	"F0 do nothing",         "F1 portamento up",
	"F2 portamento down",    "F3 tone portamento",
	"F4 vibrato",            "F5 set attack/decay",
	"F6 set sustain/release","F7 set waveform",
	"F8 set wavetable ptr",  "F9 set pulsetable ptr",
	"FA set filtertable ptr","FB set filter control",
	"FC set filter cutoff",  "FD set master volume",
	"FE funktempo",
};

// Filter-table passband presets — left-byte high nibble selects SID $D418
// bits 4..7. Low nibble is always 0 in a passband row.
static const char *GT2_FilterPassbands[8] =
{
	"$80 off",          "$90 lowpass",
	"$A0 bandpass",     "$B0 LP+BP",
	"$C0 highpass",     "$D0 HP+LP",
	"$E0 HP+BP",        "$F0 LP+BP+HP",
};

static int GT2FtblSelectedPassbandIndex(unsigned char leftValue)
{
	// What the player calls a passband row, not what the grid happens to
	// print. gplay.c:447 takes $FF as the jump and then treats EVERY value
	// >= $80 as a passband set, reading only bits 4-6: filtertype =
	// ltable[FTBL][ptr] & 0x70. greloc.c:1574 packs it the same way for the
	// 6502 player -- ((v & 0x70) >> 1) | 0x80, which mt_setfilt shifts back --
	// so the low nibble is discarded by both and $B5 plays exactly as $B0.
	//
	// This used to demand a zero low nibble and a value no higher than $F0,
	// so a row the player treats as LP+BP showed no selection at all.
	if (leftValue < 0x80 || leftValue == 0xff) return -1;
	return ((int)leftValue & 0x70) >> 4;
}

static unsigned char GT2FtblApplyPassbandSelection(int index)
{
	// Writes the canonical $80 + (index << 4). Clearing the low nibble is
	// safe: both players discard it, and the packer would drop it anyway.
	if (index < 0) index = 0;
	if (index > 7) index = 7;
	return (unsigned char)(0x80 + (index << 4));
}

static ImVec4 GT2_SelectedContextMenuColor(CGT2FontAtlas *fontAtlas)
{
	if (fontAtlas)
		return ImGui::ColorConvertU32ToFloat4(fontAtlas->palette[CEDIT]);
	return ImVec4(0.30f, 1.00f, 0.30f, 1.0f);
}

static void GT2_RenderHelpTitle(const char *text, bool active)
{
	if (active)
		ImGui::TextUnformatted(text);
	else
		ImGui::TextDisabled("%s", text);
}

// Render `count` strings across `cols` content-sized columns.
static void GT2_RenderHelpGrid(const char *id, const char *const *items, int count, int cols)
{
	if (ImGui::BeginTable(id, cols,
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
	{
		for (int i = 0; i < count; i++)
		{
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", items[i]);
		}
		ImGui::EndTable();
	}
}

static bool GT2_RenderSelectableHelpGrid(const char *id, const char *const *items,
										 int count, int cols, int selectedIndex,
										 ImVec4 selectedColor, int *clickedIndex)
{
	bool changed = false;
	if (ImGui::BeginTable(id, cols,
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
	{
		// The selected entry is a FILLED green chip with a dark glyph, not green
		// text on ImGui's default grey selection bar. Green on grey is what the
		// selected passband looked like, and it read as not selected at all --
		// the resonance chips right below it, and the pattern command picker,
		// have used the filled form all along.
		ImVec4 selBgIdle   (selectedColor.x, selectedColor.y, selectedColor.z, 0.75f);
		ImVec4 selBgHovered(selectedColor.x, selectedColor.y, selectedColor.z, 0.90f);
		ImVec4 selBgActive (selectedColor.x, selectedColor.y, selectedColor.z, 1.00f);
		ImVec4 selGlyph    (0.05f, 0.05f, 0.05f, 1.00f);

		for (int i = 0; i < count; i++)
		{
			ImGui::TableNextColumn();
			bool selected = (i == selectedIndex);
			ImVec2 textSize = ImGui::CalcTextSize(items[i]);
			const ImGuiStyle &style = ImGui::GetStyle();
			const float textGapX = 2.0f;
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
			if (selected)
			{
				ImGui::PushStyleColor(ImGuiCol_Header,        selBgIdle);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, selBgHovered);
				ImGui::PushStyleColor(ImGuiCol_HeaderActive,  selBgActive);
			}
			if (ImGui::Selectable(items[i], selected,
				(ImGuiSelectableFlags)GT2WtblContextSelectableFlags(),
				ImVec2(textSize.x + textGapX, 0.0f)))
			{
				if (clickedIndex) *clickedIndex = i;
				changed = true;
			}
			bool hovered = ImGui::IsItemHovered();
			ImVec2 textMin = ImGui::GetItemRectMin();
			ImVec2 textMax = ImGui::GetItemRectMax();
			if (selected)
				ImGui::PopStyleColor(3);
			ImGui::PopStyleColor();
			float textInsetX = style.ItemSpacing.x * 0.5f + textGapX;
			ImVec2 textPos(textMin.x + textInsetX,
				textMin.y + (textMax.y - textMin.y - textSize.y) * 0.5f);
			int colorMode = GT2WtblContextTextColorMode(selected, hovered);
			ImVec4 color = colorMode == 2 ? selGlyph
				: colorMode == 1 ? ImGui::GetStyleColorVec4(ImGuiCol_Text)
				: ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
			ImDrawList *dl = ImGui::GetWindowDrawList();
			dl->PushClipRect(textMin, textMax, true);
			dl->AddText(textPos, ImGui::GetColorU32(color), items[i]);
			dl->PopClipRect();
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndTable();
	}
	return changed;
}

// Result of the filter-table editor in the table-help context menu. Each
// `*Changed` flag tells the caller which byte to commit through SetFiltertable*.
struct GT2FtblEdit
{
	bool leftChanged;
	bool rightChanged;
	unsigned char newLeft;
	unsigned char newRight;
};

// Render the table-specific help block in the context menu.
static bool GT2_RenderTableHelp(int t, CGT2FontAtlas *fontAtlas,
								 int currentWtblLeft, bool allowWtblSelection,
								 unsigned char *newWtblLeft,
								 int currentFtblLeft, int currentFtblRight,
								 bool allowFtblSelection, GT2FtblEdit *ftblEdit,
								 int activeColumn, int currentLeftValue)
{
	if (t < 0 || t >= MAX_TABLES)
		return false;

	bool changed = false;
	ImGui::TextDisabled("%s", GT2_TableIntro[t]);
	ImGui::Spacing();
	if (ImGui::BeginTable("##gt2tblranges", 2,
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
	{
		// activeColumn < 0 means "no row is being edited", so nothing is
		// singled out. Otherwise the lines describing the byte the cursor is
		// on are drawn in full colour and the rest stay dimmed -- the same
		// treatment the waveform / command lists below already get.
		int activeSide = -1;
		if (activeColumn >= 0)
			activeSide = (activeColumn <= 1) ? GT2_HELP_LEFT : GT2_HELP_RIGHT;

		ImVec4 activeColor = GT2_SelectedContextMenuColor(fontAtlas);
		int previousColumnKind = -1;

		for (const GT2HelpRow *r = GT2_TableRows[t]; r->key; r++)
		{
			// A rule line between the left-byte lines and the right-byte ones:
			// "right note: ..." on its own reads as if it were another left
			// value rather than the other half of the row.
			if (previousColumnKind == GT2_HELP_LEFT && r->column == GT2_HELP_RIGHT)
			{
				ImGui::TableNextColumn(); ImGui::Separator();
				ImGui::TableNextColumn(); ImGui::Separator();
			}
			previousColumnKind = r->column;

			// The column has to match AND, for a line that names a range of
			// left-byte values, the row's actual left byte has to fall in it.
			// Without the value test every left line lit up at once: a
			// pulsetable row holding $85 highlighted "$01-$7F", "$80-$FE" and
			// "$FF" together, which says nothing about the row.
			bool columnMatches = (activeSide >= 0)
				&& (r->column == GT2_HELP_BOTH || r->column == activeSide);
			bool valueMatches = (r->loValue < 0)
				|| (currentLeftValue >= r->loValue && currentLeftValue <= r->hiValue);
			bool active = columnMatches && valueMatches;

			ImGui::TableNextColumn();
			if (active) ImGui::TextColored(activeColor, "%s", r->key);
			else        ImGui::TextDisabled("%s", r->key);

			ImGui::TableNextColumn();
			if (active) ImGui::TextColored(activeColor, "%s", r->desc);
			else        ImGui::TextDisabled("%s", r->desc);
		}
		ImGui::EndTable();
	}
	if (t == WTBL)
	{
		bool interactive = allowWtblSelection;
		int selectedWaveform = currentWtblLeft >= 0
			? GT2WtblSelectedWaveformIndex((unsigned char)currentWtblLeft) : -1;
		int selectedCommand = currentWtblLeft >= 0
			? GT2WtblSelectedCommandIndex((unsigned char)currentWtblLeft) : -1;
		ImVec4 selectedColor = GT2_SelectedContextMenuColor(fontAtlas);

		// Commands before waveforms, and a rule between every section: the
		// blocks ran together as one wall of text, and the ranges table above
		// lists $F0-$FE (command) before $10-$EF (waveform), so this order
		// reads in the same direction.
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		GT2_RenderHelpTitle("Commands ($F0-$FE):", selectedCommand >= 0);
		int clickedCommand = -1;
		if (interactive && GT2_RenderSelectableHelpGrid("##gt2cmd", GT2_WaveCmds, 15, 2,
			selectedCommand, selectedColor, &clickedCommand))
		{
			if (newWtblLeft)
				*newWtblLeft = GT2WtblApplyCommandSelection(clickedCommand);
			changed = true;
		}
		else if (!interactive)
		{
			GT2_RenderHelpGrid("##gt2cmd", GT2_WaveCmds, 15, 2);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		GT2_RenderHelpTitle("Waveform = mix (high nibble) + control (low nibble).",
			selectedWaveform >= 0);
		ImGui::TextDisabled("Control bits: $x1 gate, $x2 sync, $x4 ring mod, $x8 test.");
		int clickedWaveform = -1;
		if (interactive && GT2_RenderSelectableHelpGrid("##gt2wf", GT2_Waveforms, 14, 2,
			selectedWaveform, selectedColor, &clickedWaveform))
		{
			if (newWtblLeft)
				*newWtblLeft = GT2WtblApplyWaveformSelection(
					currentWtblLeft >= 0 ? (unsigned char)currentWtblLeft : 0x01,
					clickedWaveform);
			changed = true;
		}
		else if (!interactive)
		{
			GT2_RenderHelpGrid("##gt2wf", GT2_Waveforms, 14, 2);
		}
	}
	if (t == FTBL && currentFtblLeft >= 0)
	{
		// Clickable passband / resonance / routing editor. Click on any
		// passband entry to set / convert the row into a $80-$FE passband
		// row; the resonance + routing controls appear once the row is
		// in that range. A modulate ($01-$7F), cutoff-set ($00) or jump
		// ($FF) row reports no current passband selection (highlight =
		// none) but the grid stays clickable so the user can switch the
		// row into passband mode.
		ImVec4 selectedColor = GT2_SelectedContextMenuColor(fontAtlas);
		int passband = GT2FtblSelectedPassbandIndex((unsigned char)currentFtblLeft);
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		GT2_RenderHelpTitle("Passband (left high nibble):", passband >= 0);
		int clickedPassband = -1;
		if (allowFtblSelection)
		{
			if (GT2_RenderSelectableHelpGrid("##gt2fp", GT2_FilterPassbands, 8, 2,
				passband, selectedColor, &clickedPassband))
			{
				if (ftblEdit)
				{
					ftblEdit->newLeft = GT2FtblApplyPassbandSelection(clickedPassband);
					ftblEdit->leftChanged = true;
				}
				changed = true;
			}
		}
		else
		{
			GT2_RenderHelpGrid("##gt2fp", GT2_FilterPassbands, 8, 2);
		}

		if (passband >= 0 && currentFtblRight >= 0)
		{
			int rt = currentFtblRight & 0xFF;
			int resonance = (rt >> 4) & 0x0F;
			int routing   = rt & 0x07;     // bits 0..2 select ch1/ch2/ch3

			ImGui::Spacing();
			ImGui::TextUnformatted("Resonance (right high nibble):");
			ImVec4 selBgIdle    (selectedColor.x, selectedColor.y, selectedColor.z, 0.75f);
			ImVec4 selBgHovered (selectedColor.x, selectedColor.y, selectedColor.z, 0.90f);
			ImVec4 selBgActive  (selectedColor.x, selectedColor.y, selectedColor.z, 1.00f);
			ImVec4 selFg        (0.05f, 0.05f, 0.05f, 1.00f);   // dark glyph on the green chip
			for (int i = 0; i < 16; i++)
			{
				if (i > 0 && (i % 8) != 0) ImGui::SameLine();
				char rb[8]; snprintf(rb, sizeof(rb), "%X##fres%d", i, i);
				bool sel = (i == resonance);
				if (sel)
				{
					ImGui::PushStyleColor(ImGuiCol_Button,        selBgIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selBgHovered);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  selBgActive);
					ImGui::PushStyleColor(ImGuiCol_Text,          selFg);
				}
				if (ImGui::SmallButton(rb))
				{
					if (allowFtblSelection && ftblEdit)
					{
						ftblEdit->newRight = (unsigned char)((i << 4) | routing);
						ftblEdit->rightChanged = true;
						changed = true;
					}
				}
				if (sel)
					ImGui::PopStyleColor(4);
			}

			ImGui::Spacing();
			ImGui::TextUnformatted("Routing (right low nibble — channels to filter):");
			bool ch1 = (routing & 0x01) != 0;
			bool ch2 = (routing & 0x02) != 0;
			bool ch3 = (routing & 0x04) != 0;
			bool routingChanged = false;
			if (ImGui::Checkbox("Ch 1##fr1", &ch1)) routingChanged = true;
			ImGui::SameLine();
			if (ImGui::Checkbox("Ch 2##fr2", &ch2)) routingChanged = true;
			ImGui::SameLine();
			if (ImGui::Checkbox("Ch 3##fr3", &ch3)) routingChanged = true;
			if (routingChanged && allowFtblSelection && ftblEdit)
			{
				int newRouting = (ch1 ? 0x01 : 0) | (ch2 ? 0x02 : 0) | (ch3 ? 0x04 : 0);
				ftblEdit->newRight = (unsigned char)((resonance << 4) | newRouting);
				ftblEdit->rightChanged = true;
				changed = true;
			}
		}
	}
	return changed;
}

// --- Instrument-parameter help (right-click context) -----------------------

static const char *GT2_InstrParamName[9] =
{
	"Attack / Decay", "Sustain / Release", "Wavetable Pos",
	"Pulsetable Pos", "Filtertable Pos", "Vibrato Param",
	"Vibrato Delay", "HR / Gate Timer", "1st-Frame Waveform",
};

static const char *GT2_InstrParamDesc[9] =
{
	"High nibble = attack rate, low nibble = decay rate.\n"
	"0 = fastest, F = slowest. Times below.",

	"High nibble = sustain level (0 = silent, F = loudest).\n"
	"Low nibble = release rate (0 = fastest, F = slowest);\n"
	"release timing matches the decay column below.",

	"Wavetable start position (1-based; 0 = none).\n"
	"00 stops wavetable execution - rarely useful.",

	"Pulsetable start position (1-based; 0 = none).\n"
	"00 leaves pulse execution untouched.",

	"Filtertable start position (1-based; 0 = none).\n"
	"00 leaves filter execution untouched. Best to run a\n"
	"filter instrument on only one channel at a time.",

	"Instrument vibrato - an index into the speedtable\n"
	"(same parameter as command 4XY). 00 = none.",

	"Ticks before the instrument vibrato starts.\n"
	"00 turns the instrument vibrato off.",

	"Ticks before note start for note-fetch, gate-off and\n"
	"hard restart. At most tempo-1 (tempo 4 -> max 3).\n"
	"Bit $80 disables hard restart, $40 disables gate-off.",

	"Waveform on the note's init frame, usually $09\n"
	"(gate + test = hard restart). $00 = leave waveform\n"
	"and gate unchanged; $FE = gate off; $FF = gate on.",
};

// SID ADSR rate -> time (see https://www.c64-wiki.com/wiki/ADSR).
static const char *GT2_AttackTime[16] =
{
	"2 ms", "8 ms", "16 ms", "24 ms", "38 ms", "56 ms", "68 ms", "80 ms",
	"100 ms", "250 ms", "500 ms", "800 ms", "1 s", "3 s", "5 s", "8 s",
};
static const char *GT2_DecayRelTime[16] =
{
	"6 ms", "24 ms", "48 ms", "72 ms", "114 ms", "168 ms", "204 ms", "240 ms",
	"300 ms", "750 ms", "1.5 s", "2.4 s", "3 s", "9 s", "15 s", "24 s",
};

// The 16-row ADSR rate / time reference table.
static void GT2_RenderAdsrTable()
{
	if (ImGui::BeginTable("##gt2adsr", 3, ImGuiTableFlags_SizingFixedFit
		| ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Val");
		ImGui::TableSetupColumn("Attack");
		ImGui::TableSetupColumn("Decay / Release");
		ImGui::TableHeadersRow();
		for (int i = 0; i < 16; i++)
		{
			ImGui::TableNextColumn(); ImGui::TextDisabled("%X", i);
			ImGui::TableNextColumn(); ImGui::TextDisabled("%s", GT2_AttackTime[i]);
			ImGui::TableNextColumn(); ImGui::TextDisabled("%s", GT2_DecayRelTime[i]);
		}
		ImGui::EndTable();
	}
}

// Render the help block for instrument parameter `eipos` (0..8).
static void GT2_RenderInstrParamHelp(int eipos)
{
	if (eipos < 0 || eipos > 8)
		return;
	ImGui::TextUnformatted(GT2_InstrParamName[eipos]);
	ImGui::TextDisabled("%s", GT2_InstrParamDesc[eipos]);
	if (eipos == 0 || eipos == 1)   // Attack/Decay, Sustain/Release
	{
		ImGui::Spacing();
		GT2_RenderAdsrTable();
	}
}

// Toggle button with a text label - highlighted (ButtonActive colour) when lit.
static bool GT2_ToggleButton(const char *label, bool lit)
{
	float uiScale = GT2EffectiveUIScale();
	if (lit)
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	bool clicked = ImGui::Button(label, ImVec2(46.0f * uiScale, 24.0f * uiScale));
	if (lit)
		ImGui::PopStyleColor();
	return clicked;
}

// Button with a drawn waveform-shape icon (0 triangle, 1 saw, 2 pulse,
// 3 noise). Highlighted when lit. Returns true when clicked.
static bool GT2_WaveformButton(const char *id, int shapeKind, bool lit, const char *tooltip)
{
	float uiScale = GT2EffectiveUIScale();
	if (lit)
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	bool clicked = ImGui::Button(id, ImVec2(36.0f * uiScale, 24.0f * uiScale));
	if (lit)
		ImGui::PopStyleColor();
	if (tooltip)
		ImGui::SetItemTooltip("%s", tooltip);

	ImVec2 a = ImGui::GetItemRectMin();
	ImVec2 b = ImGui::GetItemRectMax();
	float pad = 6.0f * uiScale;
	float x0 = a.x + pad, x1 = b.x - pad;
	float yt = a.y + pad, yb = b.y - pad;
	float xm = (x0 + x1) * 0.5f, ym = (yt + yb) * 0.5f;
	ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 pts[6];
	int n = 0;
	switch (shapeKind)
	{
		case 0:  // triangle wave
			pts[n++] = ImVec2(x0, yb);
			pts[n++] = ImVec2(xm, yt);
			pts[n++] = ImVec2(x1, yb);
			break;
		case 1:  // sawtooth
			pts[n++] = ImVec2(x0, yb);
			pts[n++] = ImVec2(x1, yt);
			pts[n++] = ImVec2(x1, yb);
			break;
		case 2:  // pulse / square
			pts[n++] = ImVec2(x0, yb);
			pts[n++] = ImVec2(x0, yt);
			pts[n++] = ImVec2(xm, yt);
			pts[n++] = ImVec2(xm, yb);
			pts[n++] = ImVec2(x1, yb);
			break;
		default: // noise
			pts[n++] = ImVec2(x0, ym);
			pts[n++] = ImVec2(x0 + (x1 - x0) * 0.25f, yt);
			pts[n++] = ImVec2(x0 + (x1 - x0) * 0.45f, yb);
			pts[n++] = ImVec2(x0 + (x1 - x0) * 0.65f, yt);
			pts[n++] = ImVec2(x0 + (x1 - x0) * 0.85f, yb);
			pts[n++] = ImVec2(x1, ym);
			break;
	}
	dl->AddPolyline(pts, n, col, 0, 1.6f * uiScale);
	return clicked;
}

// Render the 4 waveform-shape + 4 control-bit toggle buttons for byte `wf`.
// Returns the new byte value if a bit was toggled, otherwise -1.
static int GT2_RenderWaveformToggles(int wf)
{
	int result = -1;
	ImGui::TextDisabled("Wave"); ImGui::SameLine();
	if (GT2_WaveformButton("##wtri", 0, (wf & 0x10) != 0, "Triangle  $10")) result = wf ^ 0x10;
	ImGui::SameLine();
	if (GT2_WaveformButton("##wsaw", 1, (wf & 0x20) != 0, "Sawtooth  $20")) result = wf ^ 0x20;
	ImGui::SameLine();
	if (GT2_WaveformButton("##wpul", 2, (wf & 0x40) != 0, "Pulse  $40"))    result = wf ^ 0x40;
	ImGui::SameLine();
	if (GT2_WaveformButton("##wnse", 3, (wf & 0x80) != 0, "Noise  $80"))    result = wf ^ 0x80;
	ImGui::SameLine();
	ImGui::TextDisabled(" Ctrl"); ImGui::SameLine();
	if (GT2_ToggleButton("GATE", (wf & 0x01) != 0)) result = wf ^ 0x01;
	ImGui::SameLine();
	if (GT2_ToggleButton("SYNC", (wf & 0x02) != 0)) result = wf ^ 0x02;
	ImGui::SameLine();
	if (GT2_ToggleButton("RING", (wf & 0x04) != 0)) result = wf ^ 0x04;
	ImGui::SameLine();
	if (GT2_ToggleButton("TEST", (wf & 0x08) != 0)) result = wf ^ 0x08;
	return result;
}

u8 CViewGT2Instrument::ApplyInstrumentCellBackground(u8 colorIndex, int backgroundColor) const
{
	if (backgroundColor < 0) return colorIndex;
	return (u8)((colorIndex & 0x0F) | ((backgroundColor & 0x0F) << 4));
}

static void DrawInstrumentTextGT2(CViewGT2Instrument *view, ImDrawList *dl, CGT2FontAtlas *fontAtlas,
								  float px, float py, int firstColumn, int row,
								  u8 colorIndex, const char *text, int cursorBackgroundColor)
{
	int cursorCol = -1;
	int cursorRow = -1;
	if (editmode == EDIT_INSTRUMENT && !eamode)
	{
		if (eipos < 9)
		{
			cursorCol = 16 + eicolumn + 20 * (eipos / 5);
			cursorRow = 1 + (eipos % 5);
		}
		else
		{
			cursorCol = 20 + (int)strlen(ginstr[einum].name);
			cursorRow = 0;
		}
	}

	char ch[2] = { 0, 0 };
	for (int i = 0; text[i]; i++)
	{
		int backgroundColor = (row == cursorRow && firstColumn + i == cursorCol) ? cursorBackgroundColor : -1;
		ch[0] = text[i];
		DrawTextGT2(dl, fontAtlas, px + i * GT2CellW(), py,
					view->ApplyInstrumentCellBackground(colorIndex, backgroundColor), ch);
	}
}

static void DrawInstrumentTableTextGT2(CViewGT2Instrument *view, ImDrawList *dl, CGT2FontAtlas *fontAtlas,
									   float px, float py, int tableNum, int tableRow,
									   u8 colorIndex, const char *text, int cursorBackgroundColor)
{
	char ch[2] = { 0, 0 };
	for (int i = 0; text[i]; i++)
	{
		int backgroundColor = -1;
		if (editmode == EDIT_TABLES && !eamode && etnum == tableNum && etpos == tableRow)
		{
			int cursorColumn = 3 + (etcolumn & 1) + (etcolumn / 2) * 3;
			if (i == cursorColumn)
				backgroundColor = cursorBackgroundColor;
		}

		ch[0] = text[i];
		DrawTextGT2(dl, fontAtlas, px + i * GT2CellW(), py,
					view->ApplyInstrumentCellBackground(colorIndex, backgroundColor), ch);
	}
}

// Hidden-row marker: a small filled triangle, drawn with primitives rather
// than a glyph. GT2's chargen.bin has no arrow characters, and FontAwesome is
// not baked into any atlas in this build (CGuiFontManager bakes Inter /
// JetBrains with Latin ranges only, and nothing initialises imguial_fonts), so
// an ICON_FA_* string would render as a missing glyph. A primitive also stays
// small enough to sit clear of the slice's hex digits at any GT2 zoom.
static void DrawGT2HiddenRowsMarker(ImDrawList *dl, CGT2FontAtlas *fontAtlas,
									float colX, float rowY, bool pointingUp)
{
	// Centred in the two-column gap that follows each slice's "XX:XX XX",
	// so the marker never crowds the last hex pair.
	float cx = colX + GT2CellW() * 0.55f;
	float cy = rowY + GT2CellH() * 0.5f;
	float hw = GT2CellW() * 0.28f;
	float hh = GT2CellH() * 0.16f;
	ImU32 col = fontAtlas->palette[CTITLE & 0x0F];

	if (pointingUp)
	{
		dl->AddTriangleFilled(ImVec2(cx, cy - hh),
							  ImVec2(cx - hw, cy + hh),
							  ImVec2(cx + hw, cy + hh), col);
	}
	else
	{
		dl->AddTriangleFilled(ImVec2(cx, cy + hh),
							  ImVec2(cx + hw, cy - hh),
							  ImVec2(cx - hw, cy - hh), col);
	}
}

CViewGT2Instrument::CViewGT2Instrument(const char *name, float posX, float posY, float posZ,
										float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
	this->tableContextHasClickedRow = false;
	this->tableContextCanEdit = false;
	for (int i = 0; i < MAX_TABLES; i++)
		this->tableViewOffset[i] = 0;
	this->lastInstrumentNum = -1;
	this->lastCursorTable = -1;
	this->lastCursorPos = -1;
	this->lastCursorColumn = -1;
	this->tableWheelAccum = 0.0f;
	this->tableWheelLastTable = -1;
	// The slices scroll themselves now, so the window must not also scroll
	// under the wheel -- and a window that cannot scroll keeps the
	// visible-row measurement in RenderImGui() exact.
	imGuiNoScrollbar = true;
}

CViewGT2Instrument::~CViewGT2Instrument()
{
}

// Light-blue translucent fill marking an editable text field. Derived from the
// current ImGui theme — its blue text-selection accent, lightened toward white
// and given a soft alpha — so it tracks theme changes instead of being a
// hard-coded colour.
static ImU32 GT2_EditableFieldColor()
{
	ImVec4 base = ImGui::GetStyleColorVec4(ImGuiCol_TextSelectedBg);
	const float lighten = 0.45f;
	ImVec4 c;
	c.x = base.x + (1.0f - base.x) * lighten;
	c.y = base.y + (1.0f - base.y) * lighten;
	c.z = base.z + (1.0f - base.z) * lighten;
	c.w = 0.30f;
	return ImGui::GetColorU32(c);
}

// Insert a table row for the current instrument's wavetable / pulsetable /
// filtertable. On an empty table it allocates a fresh program at the pool end;
// inserttable() shifts every instrument's pointers and jump targets. The
// speedtable (STBL) is atomic and intentionally not handled here.
void CViewGT2Instrument::InsertTableRow()
{
	if (etnum < 0 || etnum >= STBL)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	int start = ginstr[einum].ptr[etnum] ? ginstr[einum].ptr[etnum] - 1 : -1;
	int newstart = (start < 0) ? gettablelen(etnum) : 0;
	if (start < 0 && newstart + 2 > MAX_TABLELEN)
		return;                              // no room for a row + end marker

	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	if (start < 0)
	{
		// A fresh table program needs an 0xff end marker — without it
		// gettablepartlen() runs to the end of the pool (255 rows). Place the
		// marker, point the instrument at it, then inserttable() inserts the
		// content row in front and shifts the marker to newstart+1.
		ltable[etnum][newstart] = 0xff;
		rtable[etnum][newstart] = 0;
		ginstr[einum].ptr[etnum] = newstart + 1;
		inserttable(etnum, newstart, 1);
		etpos = newstart;
	}
	else
	{
		inserttable(etnum, etpos, 1);
	}
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

bool CViewGT2Instrument::CreateWavetableRowWithLeft(unsigned char value)
{
	if (einum < 0 || einum >= MAX_INSTR)
		return false;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return false;
	int start = ginstr[einum].ptr[WTBL] ? ginstr[einum].ptr[WTBL] - 1 : -1;
	if (start >= 0)
		return false;
	int newstart = gettablelen(WTBL);
	if (newstart + 2 > MAX_TABLELEN)
		return false;

	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	ltable[WTBL][newstart] = 0xff;
	rtable[WTBL][newstart] = 0;
	ginstr[einum].ptr[WTBL] = newstart + 1;
	inserttable(WTBL, newstart, 1);
	editmode = EDIT_TABLES;
	etnum = WTBL;
	etpos = newstart;
	etcolumn = 0;
	ltable[WTBL][etpos] = value;
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
	return true;
}

void CViewGT2Instrument::DeleteTableRow()
{
	if (etnum < 0 || etnum >= STBL)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	int start = ginstr[einum].ptr[etnum] ? ginstr[einum].ptr[etnum] - 1 : -1;
	if (start < 0)
		return;
	if (ltable[etnum][etpos] == 0xff)        // keep the program terminated
		return;
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	deletetable(etnum, etpos);
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

// Give the current instrument a speedtable entry (the empty Speed Table
// placeholder). The speedtable is a flat pool of atomic entries, so this
// just points the instrument at a free slot — no row insert.
void CViewGT2Instrument::AllocateSpeedtableEntry()
{
	if (einum < 0 || einum >= MAX_INSTR)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	int slot = findfreespeedtable();
	if (slot < 0)
		return;                          // speedtable pool full
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	ginstr[einum].ptr[STBL] = (unsigned char)(slot + 1);
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
	etpos = slot;
}

// Set the current instrument's 1st-frame waveform byte as one undo step.
void CViewGT2Instrument::SetFirstwave(unsigned char value)
{
	if (einum < 0 || einum >= MAX_INSTR)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	ginstr[einum].firstwave = value;
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

// Set the left / right byte of the focused wavetable cell, one undo step.
void CViewGT2Instrument::SetWavetableLeft(unsigned char value)
{
	if (editmode != EDIT_TABLES || etnum != WTBL)
		return;
	if (etpos < 0 || etpos >= MAX_TABLELEN)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	ltable[WTBL][etpos] = value;
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

void CViewGT2Instrument::SetWavetableRight(unsigned char value)
{
	if (editmode != EDIT_TABLES || etnum != WTBL)
		return;
	if (etpos < 0 || etpos >= MAX_TABLELEN)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	rtable[WTBL][etpos] = value;
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

// The table-name heading and the interactive help block for one
// instrument-table row, plus applying whatever the user changes there.
//
// Shared so there is exactly one implementation of it: the right-click context
// menu renders it under its Insert / Delete items, and CViewGT2InstrumentTableRow
// renders it on its own, following the live cursor. The two differ only in how
// they decide `hasRow` / `canEdit` -- the menu from the row that was clicked,
// the view from where the cursor is now.
void CViewGT2Instrument::RenderTableRowHelp(int tableNum, int rowPos,
											bool hasRow, bool canEdit,
											int activeColumn)
{
	if (tableNum < 0 || tableNum >= MAX_TABLES)
		return;

	static const char *kTableName[MAX_TABLES] =
		{ "Wavetable", "Pulsetable", "Filtertable", "Speedtable" };

	int start = ginstr[einum].ptr[tableNum]
				? ginstr[einum].ptr[tableNum] - 1 : -1;
	int len = (start >= 0) ? gettablepartlen(tableNum, start) : 0;

	ImGui::TextUnformatted(kTableName[tableNum]);

	bool hasCurrentWtblRow = (tableNum == WTBL)
		&& hasRow
		&& GT2WtblContextHasValidRow(start, len, rowPos);
	bool canCreateWtblRow = (tableNum == WTBL)
		&& canEdit && start < 0;
	int currentWtblLeft = hasCurrentWtblRow
		? (int)ltable[WTBL][rowPos] : -1;
	unsigned char newWtblLeft = 0;
	bool hasCurrentFtblRow = (tableNum == FTBL)
		&& hasRow
		&& start >= 0 && rowPos >= 0 && rowPos < MAX_TABLELEN;
	int currentFtblLeft  = hasCurrentFtblRow ? (int)ltable[FTBL][rowPos] : -1;
	int currentFtblRight = hasCurrentFtblRow ? (int)rtable[FTBL][rowPos] : -1;
	GT2FtblEdit ftblEdit = { false, false, 0, 0 };

	// The row's left byte, for every table -- the ranges in the help rows are
	// left-byte ranges, so the value decides which single line applies.
	// -1 when there is no row, which matches no range and highlights nothing.
	int currentLeftValue = (hasRow && rowPos >= 0 && rowPos < MAX_TABLELEN)
		? (int)ltable[tableNum][rowPos] : -1;

	if (GT2_RenderTableHelp(tableNum, fontAtlas, currentWtblLeft,
		hasCurrentWtblRow || canCreateWtblRow, &newWtblLeft,
		currentFtblLeft, currentFtblRight,
		hasCurrentFtblRow && canEdit, &ftblEdit,
		hasRow ? activeColumn : -1, currentLeftValue))
	{
		if (hasCurrentWtblRow && currentWtblLeft != (int)newWtblLeft)
			SetWavetableLeft(newWtblLeft);
		else if (canCreateWtblRow)
			CreateWavetableRowWithLeft(newWtblLeft);
		if (ftblEdit.leftChanged)
			SetFiltertableLeft(ftblEdit.newLeft);
		if (ftblEdit.rightChanged)
			SetFiltertableRight(ftblEdit.newRight);
	}
}

void CViewGT2Instrument::SetFiltertableLeft(unsigned char value)
{
	if (editmode != EDIT_TABLES || etnum != FTBL)
		return;
	if (etpos < 0 || etpos >= MAX_TABLELEN)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	ltable[FTBL][etpos] = value;
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

void CViewGT2Instrument::SetFiltertableRight(unsigned char value)
{
	if (editmode != EDIT_TABLES || etnum != FTBL)
		return;
	if (etpos < 0 || etpos >= MAX_TABLELEN)
		return;
	if (!pluginGoatTracker || !pluginGoatTracker->viewPatterns)
		return;
	pluginGoatTracker->viewPatterns->BeginPatternUndoStep();
	rtable[FTBL][etpos] = value;
	pluginGoatTracker->viewPatterns->CommitPatternUndoStep();
}

// Contextual value palette below the instrument parameters. Edits the focused
// wavetable cell (row-type selector, per-type controls, right-column nudge) or
// the instrument's 1st-frame waveform when the cursor is on that field.
void CViewGT2Instrument::RenderTablePalette()
{
	float uiScale = GT2EffectiveUIScale();
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec2 windowPadding(6.0f * uiScale, 5.0f * uiScale);
	ImVec2 framePadding(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale);
	ImVec2 itemSpacing(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale);
	ImVec2 itemInnerSpacing(style.ItemInnerSpacing.x * uiScale, style.ItemInnerSpacing.y * uiScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, itemInnerSpacing);
	ImGui::BeginChild("##gt2tblpalette", ImVec2(0, 66.0f * uiScale), 0, ImGuiWindowFlags_NoScrollbar);
	ImGui::SetWindowFontScale(uiScale);

	bool fwFocused = (editmode == EDIT_INSTRUMENT && eipos == 8);
	bool wtFocused = (editmode == EDIT_TABLES && etnum == WTBL
					  && etpos >= 0 && etpos < MAX_TABLELEN);

	if (fwFocused)
	{
		// The instrument's 1st-frame waveform is a waveform byte too.
		ImGui::AlignTextToFramePadding();
		int fnv = GT2_RenderWaveformToggles(ginstr[einum].firstwave);
		if (fnv >= 0)
			SetFirstwave((unsigned char)fnv);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("$09 hard restart (gate+test)   $00 no override"
							"   $FE/$FF gate off/on");
		ImGui::EndChild();
		ImGui::PopStyleVar(4);
		return;
	}
	if (!wtFocused)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Click a wavetable cell or the 1st-frame waveform"
							" to edit it here.");
		ImGui::EndChild();
		ImGui::PopStyleVar(4);
		return;
	}

	int wf = ltable[WTBL][etpos];
	int rt = rtable[WTBL][etpos];
	// Row type: 0 waveform, 1 command, 2 delay, 3 jump.
	int rowType = (wf <= 0x0F) ? 2 : (wf <= 0xEF) ? 0 : (wf <= 0xFE) ? 1 : 3;

	// --- Row 1: row-type selector + right-column nudge ---
	ImGui::AlignTextToFramePadding();
	if (GT2_ToggleButton("Wave",  rowType == 0) && rowType != 0) SetWavetableLeft(0x11);
	ImGui::SameLine();
	if (GT2_ToggleButton("Cmd",   rowType == 1) && rowType != 1) SetWavetableLeft(0xF0);
	ImGui::SameLine();
	if (GT2_ToggleButton("Delay", rowType == 2) && rowType != 2) SetWavetableLeft(0x01);
	ImGui::SameLine();
	if (GT2_ToggleButton("Jump",  rowType == 3) && rowType != 3) SetWavetableLeft(0xFF);

	ImGui::SameLine();
	ImGui::TextDisabled("    ");
	ImGui::SameLine();
	const char *rcLabel = (rowType == 1) ? "Param" : (rowType == 3) ? "Target" : "Note";
	ImGui::TextDisabled("%s", rcLabel); ImGui::SameLine();
	if (ImGui::Button("-##rc", ImVec2(24.0f * uiScale, 24.0f * uiScale)) && rt > 0)
		SetWavetableRight((unsigned char)(rt - 1));
	ImGui::SameLine();
	{
		char vb[8]; snprintf(vb, sizeof(vb), "%02X", rt);
		ImGui::TextDisabled("%s", vb);
	}
	ImGui::SameLine();
	if (ImGui::Button("+##rc", ImVec2(24.0f * uiScale, 24.0f * uiScale)) && rt < 0xFF)
		SetWavetableRight((unsigned char)(rt + 1));
	ImGui::SameLine();
	if (ImGui::Button("0##rc", ImVec2(24.0f * uiScale, 24.0f * uiScale)) && rt != 0)
		SetWavetableRight(0);

	// --- Row 2: per-type left-column control ---
	if (rowType == 0)
	{
		ImGui::AlignTextToFramePadding();
		int nv = GT2_RenderWaveformToggles(wf);
		if (nv >= 0)
			SetWavetableLeft((unsigned char)nv);
	}
	else if (rowType == 1)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Command"); ImGui::SameLine();
		int cmdIdx = wf - 0xF0;
		if (cmdIdx < 0)  cmdIdx = 0;
		if (cmdIdx > 14) cmdIdx = 14;
		char popupButton[64];
		snprintf(popupButton, sizeof(popupButton), "%s##wcmd", GT2_WaveCmds[cmdIdx]);
		if (ImGui::Button(popupButton, ImVec2(220.0f * uiScale, 0.0f)))
			ImGui::OpenPopup("##wcmd_popup");

		bool changed = false;
		// Combo popups are not part of GT2 zoom; restore app-wide style while open.
		ImGui::PopStyleVar(4);
		if (ImGui::BeginPopup("##wcmd_popup"))
		{
			for (int i = 0; i < 15; i++)
			{
				bool selected = (cmdIdx == i);
				if (ImGui::Selectable(GT2_WaveCmds[i], selected))
				{
					cmdIdx = i;
					changed = true;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, itemInnerSpacing);

		if (changed)
			SetWavetableLeft((unsigned char)(0xF0 + cmdIdx));
	}
	else if (rowType == 2)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Delay frames"); ImGui::SameLine();
		if (ImGui::Button("-##dly", ImVec2(24.0f * uiScale, 24.0f * uiScale)) && wf > 0)
			SetWavetableLeft((unsigned char)(wf - 1));
		ImGui::SameLine();
		{
			char vb[8]; snprintf(vb, sizeof(vb), "%2d", wf);
			ImGui::TextDisabled("%s", vb);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##dly", ImVec2(24.0f * uiScale, 24.0f * uiScale)) && wf < 0x0F)
			SetWavetableLeft((unsigned char)(wf + 1));
	}
	else
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Jump - 'Target' is the row to jump to ($00 = end of table).");
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(4);
}

void CViewGT2Instrument::RenderImGui()
{
	PreRenderImGui();
	if (!fontAtlas->TryLoad()) { PostRenderImGui(); return; }
	ImGui::SetWindowFontScale(GT2EffectiveUIScale());

	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetCursorScreenPos();

	int cc = cursorcolortable[cursorflash];

	// Table-section row constants, relative to tableY (set below the palette):
	// the header is row 0, the slices start at row 1.
	const int kTblHeaderRow = 0;
	const int kTblFirstRow  = 1;

	char textbuffer[256];

	// Forked from gdisplay.c:338-387
	// Row 0: title + instrument name
	sprintf(textbuffer, "INSTRUMENT NUM. %02X  %-16s", einum, ginstr[einum].name);
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(0),
				0, 0, CTITLE, textbuffer, cc);
	if (editmode == EDIT_INSTRUMENT && !eamode && eipos >= 9
		&& 20 + (int)strlen(ginstr[einum].name) >= (int)strlen(textbuffer))
	{
		DrawTextGT2(dl, fontAtlas,
					origin.x + GT2ColToPixel(20 + (int)strlen(ginstr[einum].name)),
					origin.y + GT2RowToPixel(0),
					ApplyInstrumentCellBackground(CTITLE, cc), " ");
	}

	// Light-blue translucent fill over the 16-char instrument-name field
	// (row 0, cols 20-35) so it reads as an editable text box.
	dl->AddRectFilled(
		ImVec2(origin.x + GT2ColToPixel(20), origin.y + GT2RowToPixel(0)),
		ImVec2(origin.x + GT2ColToPixel(36), origin.y + GT2RowToPixel(1)),
		GT2_EditableFieldColor());

	// Small gap below the title row before the instrument value rows. Shifting
	// origin keeps every row 1+ and the click handler in sync automatically.
	origin.y += GT2CellH() / 3.0f;

	u8 color;

	// Left column (eipos 0-4): value hex at col 16, rows 1-5
	sprintf(textbuffer, "Attack/Decay    %02X", ginstr[einum].ad);
	color = (eipos == 0) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(1),
				0, 1, color, textbuffer, cc);

	sprintf(textbuffer, "Sustain/Release %02X", ginstr[einum].sr);
	color = (eipos == 1) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(2),
				0, 2, color, textbuffer, cc);

	sprintf(textbuffer, "Wavetable Pos   %02X", ginstr[einum].ptr[WTBL]);
	color = (eipos == 2) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(3),
				0, 3, color, textbuffer, cc);

	sprintf(textbuffer, "Pulsetable Pos  %02X", ginstr[einum].ptr[PTBL]);
	color = (eipos == 3) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(4),
				0, 4, color, textbuffer, cc);

	sprintf(textbuffer, "Filtertable Pos %02X", ginstr[einum].ptr[FTBL]);
	color = (eipos == 4) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(0),
				origin.y + GT2RowToPixel(5),
				0, 5, color, textbuffer, cc);

	// Right column (eipos 5-8): value hex at col 36, rows 1-4
	sprintf(textbuffer, "Vibrato Param   %02X", ginstr[einum].ptr[STBL]);
	color = (eipos == 5) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(20),
				origin.y + GT2RowToPixel(1),
				20, 1, color, textbuffer, cc);

	sprintf(textbuffer, "Vibrato Delay   %02X", ginstr[einum].vibdelay);
	color = (eipos == 6) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(20),
				origin.y + GT2RowToPixel(2),
				20, 2, color, textbuffer, cc);

	sprintf(textbuffer, "HR/Gate Timer   %02X", ginstr[einum].gatetimer);
	color = (eipos == 7) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(20),
				origin.y + GT2RowToPixel(3),
				20, 3, color, textbuffer, cc);

	sprintf(textbuffer, "1stFrame Wave   %02X", ginstr[einum].firstwave);
	color = (eipos == 8) ? CEDIT : CNORMAL;
	DrawInstrumentTextGT2(this, dl, fontAtlas,
				origin.x + GT2ColToPixel(20),
				origin.y + GT2RowToPixel(4),
				20, 4, color, textbuffer, cc);

	// Contextual value palette — between the instrument parameters and the
	// table slices.
	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + GT2RowToPixel(6)));
	RenderTablePalette();
	float tableY = ImGui::GetCursorScreenPos().y + GT2CellH() * 0.3f;

	// Clamp the GT2 table cursor to the current instrument's slice.
	// tablecommands() may move etpos outside the slice; pull it back each frame.
	// This runs BEFORE the slice rendering below, so the scroll offset never
	// chases an out-of-slice etpos for a frame.
	if (editmode == EDIT_TABLES && etnum >= 0 && etnum < MAX_TABLES)
	{
		int start = ginstr[einum].ptr[etnum] ? ginstr[einum].ptr[etnum] - 1 : -1;
		int len   = (start >= 0) ? gettablepartlen(etnum, start) : 0;
		if (len > 0)
		{
			if (etpos < start)           etpos = start;
			if (etpos > start + len - 1) etpos = start + len - 1;
		}
	}

	// Switching instrument shows a different set of slices; start them at
	// the top rather than at the previous instrument's scroll position.
	bool instrumentChanged = (einum != lastInstrumentNum);
	if (instrumentChanged)
	{
		for (int i = 0; i < MAX_TABLES; i++)
			tableViewOffset[i] = 0;
		lastInstrumentNum = einum;
	}

	// How many slice rows actually fit below the header, measured from the
	// window -- not the native VISIBLETABLEROWS, which knows nothing about
	// this view's height.
	// Not GetWindowContentRegionMax(): that one is an obsolete ImGui API
	// (compiled only without IMGUI_DISABLE_OBSOLETE_FUNCTIONS) and its value
	// shifts with window scroll. Bottom edge minus the padding is exact here
	// -- the window carries no bottom decoration and cannot scroll.
	float sliceRowsTop  = tableY + GT2RowToPixel(kTblFirstRow);
	float contentBottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y
						- ImGui::GetStyle().WindowPadding.y;
	int visibleRows = GT2TableVisibleRows(contentBottom - sliceRowsTop, GT2CellH());

	// Follow the edit cursor only when it actually moved -- a keypress, a
	// click, gototable(). Following it every frame would undo a mouse-wheel
	// scroll on the very next frame and make peeking down a long table
	// impossible. etcolumn counts as movement too, so typing hex digits into
	// a row the user had scrolled away from brings that row back.
	bool followCursor = (etnum != lastCursorTable)
					 || (etpos != lastCursorPos)
					 || (etcolumn != lastCursorColumn)
					 || instrumentChanged;
	lastCursorTable  = etnum;
	lastCursorPos    = etpos;
	lastCursorColumn = etcolumn;

	// Mouse wheel over a slice column scrolls that column alone, leaving the
	// edit cursor where it is. The accumulator carries the fractional part so
	// a trackpad scrolls smoothly rather than rounding every delta to zero.
	int wheelTable = -1;
	int wheelRows  = 0;
	if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f)
	{
		ImVec2 wheelPos = ImGui::GetIO().MousePos;
		// Guard on the pixel, not the column: GT2PixelToCol() truncates
		// toward zero, so a mouse left of origin would map to column 0.
		int wheelCol = GT2PixelToCol(wheelPos.x - origin.x);
		if (wheelPos.y >= tableY && wheelPos.x >= origin.x && wheelCol / 10 < MAX_TABLES)
		{
			wheelTable = wheelCol / 10;
			if (wheelTable != tableWheelLastTable)
			{
				tableWheelAccum = 0.0f;
				tableWheelLastTable = wheelTable;
			}
			// Three rows per notch, the ImGui default feel.
			tableWheelAccum -= ImGui::GetIO().MouseWheel * 3.0f;
			wheelRows = (int)tableWheelAccum;
			tableWheelAccum -= (float)wheelRows;
		}
	}

	// Table slice rendering — four compact slices below the palette
	DrawTextGT2(dl, fontAtlas, origin.x + GT2ColToPixel(0),
		tableY + GT2RowToPixel(kTblHeaderRow), CTITLE,
		"WAVE TBL  PULSETBL  FILT.TBL  SPEEDTBL");

	for (int c = 0; c < MAX_TABLES; c++)
	{
		int start = ginstr[einum].ptr[c] ? ginstr[einum].ptr[c] - 1 : -1;
		int len   = (start >= 0) ? gettablepartlen(c, start) : 0;
		if (len == 0)
		{
			tableViewOffset[c] = 0;
			// Editable placeholder — click it to start the table program
			// (the speedtable just gets one atomic entry).
			float px = origin.x + GT2ColToPixel(10*c);
			float py = tableY + GT2RowToPixel(kTblFirstRow);
			DrawTextGT2(dl, fontAtlas, px, py, CNORMAL, "--:-- --");
			dl->AddRectFilled(ImVec2(px, py),
				ImVec2(px + 8 * GT2CellW(), py + GT2CellH()),
				GT2_EditableFieldColor());
			continue;
		}
		if (c == wheelTable)
			tableViewOffset[c] += wheelRows;

		// Keep the row being edited on screen, and range-clamp the offset so
		// a slice shortened by deletetable() cannot leave a gap at the bottom.
		int cursorRow = (followCursor && editmode == EDIT_TABLES && etnum == c)
						? etpos - start : -1;
		tableViewOffset[c] = GT2TableScrollOffset(tableViewOffset[c],
									len, visibleRows, cursorRow);
		int firstRow = tableViewOffset[c];
		int lastRow  = (firstRow + visibleRows < len) ? firstRow + visibleRows : len;

		for (int d = firstRow; d < lastRow; d++)
		{
			int p = start + d;
			int sliceColor = CNORMAL;
			switch (c)
			{
				case WTBL: if (ltable[c][p] >= WAVECMD) sliceColor = CCOMMAND; break;
				case PTBL: if (ltable[c][p] >= 0x80) sliceColor = CCOMMAND; break;
				case FTBL: if ((ltable[c][p] >= 0x80) || ((!ltable[c][p]) && rtable[c][p])) sliceColor = CCOMMAND; break;
				default: break;
			}
			if ((p == etpos) && (etnum == c) && (editmode == EDIT_TABLES)) sliceColor = CEDIT;
			char tb[32];
			sprintf(tb, "%02X:%02X %02X", p + 1, ltable[c][p], rtable[c][p]);
			DrawInstrumentTableTextGT2(this, dl, fontAtlas, origin.x + GT2ColToPixel(10*c),
				tableY + GT2RowToPixel(kTblFirstRow + (d - firstRow)), c, p,
				(u8)sliceColor, tb, cc);
		}

		// Hidden-row markers, so a slice longer than the window says so.
		// The up marker rides the header row above this column, the down
		// marker the last visible row.
		if (firstRow > 0)
		{
			DrawGT2HiddenRowsMarker(dl, fontAtlas,
				origin.x + GT2ColToPixel(10*c + 8),
				tableY + GT2RowToPixel(kTblHeaderRow), true);
		}
		if (lastRow < len)
		{
			DrawGT2HiddenRowsMarker(dl, fontAtlas,
				origin.x + GT2ColToPixel(10*c + 8),
				tableY + GT2RowToPixel(kTblFirstRow + (lastRow - firstRow - 1)),
				false);
		}
	}

	// Mouse click handling — set editmode and cursor position
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		int gridCol = GT2PixelToCol(mousePos.x - origin.x);
		int gridRow = GT2PixelToRow(mousePos.y - origin.y);

		editmode = EDIT_INSTRUMENT;

		// Left column values: rows 1-5, cols 16-17
		if (gridRow >= 1 && gridRow <= 5 && gridCol >= 16 && gridCol <= 17)
		{
			eipos = gridRow - 1;
			eicolumn = gridCol - 16;
		}
		// Right column values: rows 1-4, cols 36-37
		else if (gridRow >= 1 && gridRow <= 4 && gridCol >= 36 && gridCol <= 37)
		{
			eipos = (gridRow - 1) + 5;
			eicolumn = gridCol - 36;
		}
		// Name: row 0, col >= 20
		else if (gridRow == 0 && gridCol >= 20)
		{
			eipos = 9;
		}
		// Table slice: below tableY, columns 10*c .. 10*c+7
		else if (mousePos.y >= tableY)
		{
			int tableGridRow = GT2PixelToRow(mousePos.y - tableY);
			int c = gridCol / 10;
			int colInTable = gridCol - c * 10;
			if (c >= 0 && c < MAX_TABLES && colInTable >= 0 && colInTable <= 7)
			{
				int start = ginstr[einum].ptr[c] ? ginstr[einum].ptr[c] - 1 : -1;
				int len   = (start >= 0) ? gettablepartlen(c, start) : 0;
				// Screen row -> slice row, through this column's scroll offset.
				int d = tableGridRow - kTblFirstRow + tableViewOffset[c];
				if (len > 0 && d >= 0 && d < len)
				{
					editmode = EDIT_TABLES;
					etnum = c;
					etpos = start + d;
					if (colInTable <= 3)      etcolumn = 0;
					else if (colInTable == 4) etcolumn = 1;
					else if (colInTable == 6) etcolumn = 2;
					else if (colInTable == 7) etcolumn = 3;
					else                      etcolumn = 0;
				}
				else if (len == 0 && d == 0)
				{
					// Clicked the empty-table placeholder — allocate the table
					// program for this instrument and focus it.
					editmode = EDIT_TABLES;
					etnum = c;
					etcolumn = 0;
					if (c < STBL)
						InsertTableRow();
					else
						AllocateSpeedtableEntry();
				}
			}
		}
	}

	// Right-click: a table slice opens the table menu, an instrument
	// parameter opens a help popup describing it.
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		tableContextHasClickedRow = false;
		tableContextCanEdit = false;
		ImVec2 mp = ImGui::GetIO().MousePos;
		int gridCol = GT2PixelToCol(mp.x - origin.x);
		int gridRow = GT2PixelToRow(mp.y - origin.y);

		int paramEipos = -1;
		if (gridRow >= 1 && gridRow <= 5 && gridCol >= 16 && gridCol <= 17)
			paramEipos = gridRow - 1;
		else if (gridRow >= 1 && gridRow <= 4 && gridCol >= 36 && gridCol <= 37)
			paramEipos = (gridRow - 1) + 5;

		if (paramEipos >= 0)
		{
			editmode = EDIT_INSTRUMENT;
			eipos = paramEipos;
			ImGui::OpenPopup("GT2InstrumentParamCtx");
		}
		else if (mp.y >= tableY)
		{
			int tableGridRow = GT2PixelToRow(mp.y - tableY);
			int c = GT2InstrumentTableFromGridCol(gridCol);
			if (c >= 0 && c < MAX_TABLES)
			{
				etnum = c;
				int start = ginstr[einum].ptr[c] ? ginstr[einum].ptr[c] - 1 : -1;
				int len   = (start >= 0) ? gettablepartlen(c, start) : 0;
				int d = tableGridRow - kTblFirstRow;
				bool canCreateRow = GT2WtblContextShouldCreateRowOnSelection(start, len, d);
				editmode = GT2WtblContextShouldEnterEditMode(start, len, d)
					? EDIT_TABLES : EDIT_INSTRUMENT;
				int clickedRow = GT2WtblContextRowFromClick(start, len, d);
				if (clickedRow >= 0)
				{
					etpos = clickedRow;
					tableContextHasClickedRow = true;
					tableContextCanEdit = true;
				}
				else if (canCreateRow)
				{
					etpos = -1;
					tableContextCanEdit = true;
				}
				else
				{
					etpos = -1;
				}
				ImGui::OpenPopup("GT2InstrumentTblCtx");
			}
		}
	}

	// Context menu — table description + insert / delete a row. Opens only
	// for a right-click on a table slice. Speedtable rows are atomic, so its
	// insert/delete items stay disabled.
	if (ImGui::BeginPopup("GT2InstrumentTblCtx"))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();
		if (etnum >= 0 && etnum < MAX_TABLES)
		{
			int start = ginstr[einum].ptr[etnum]
						? ginstr[einum].ptr[etnum] - 1 : -1;
			bool canEdit = (etnum < STBL) && tableContextCanEdit;

			if (ImGui::MenuItem("Insert table row", "Insert / Shift+Down", false, canEdit))
				InsertTableRow();
			if (ImGui::MenuItem("Delete table row", "Delete / Shift+Up", false,
								canEdit && start >= 0 && tableContextHasClickedRow))
				DeleteTableRow();

			// The help / edit reference that used to sit here now lives in its
			// own view -- GoatTracker -> GT2 Instrument Table Row -- which
			// renders this exact call and follows the cursor instead of a
			// click target. Two copies of it on screen is one too many, so the
			// menu keeps only its actions.
			//
			// Deliberately kept, not deleted: this is the other half of the
			// shared-renderer arrangement described in the
			// instrument-table-row notes, and uncommenting
			// the line is how the menu gets it back.
			//
			//   ImGui::Separator();
			//   RenderTableRowHelp(etnum, etpos, tableContextHasClickedRow,
			//                      tableContextCanEdit, etcolumn);
		}
		// If the popup grew (e.g. picking a passband revealed the
		// resonance + routing controls), pull it back inside the viewport
		// so the new lines stay visible. ImGui only auto-positions popups
		// on open, so we have to nudge it ourselves on every frame.
		{
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 winSize = ImGui::GetWindowSize();
			ImGuiViewport *vp = ImGui::GetMainViewport();
			float maxX = vp->Pos.x + vp->Size.x;
			float maxY = vp->Pos.y + vp->Size.y;
			float newX = winPos.x;
			float newY = winPos.y;
			if (winPos.y + winSize.y > maxY)
				newY = std::max(vp->Pos.y, maxY - winSize.y);
			if (winPos.x + winSize.x > maxX)
				newX = std::max(vp->Pos.x, maxX - winSize.x);
			if (newX != winPos.x || newY != winPos.y)
				ImGui::SetWindowPos(ImVec2(newX, newY));
		}
		ImGui::EndPopup();
	}

	// Context help for the right-clicked instrument parameter.
	if (ImGui::BeginPopup("GT2InstrumentParamCtx"))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			ImGui::CloseCurrentPopup();
		if (eipos >= 0 && eipos <= 8)
			GT2_RenderInstrParamHelp(eipos);
		ImGui::EndPopup();
	}

	// Whole-window drop target: an instrument dragged from the browser lands
	// in the instrument this view is editing.
	if (ImGui::BeginDragDropTargetCustom(ImRect(ImGui::GetWindowPos(),
			ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
				   ImGui::GetWindowPos().y + ImGui::GetWindowSize().y)),
			ImGui::GetCurrentWindow()->GetID("##gt2InstrEditorDrop")))
	{
		const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(GT2_INSTRUMENT_FILE_PAYLOAD);
		if (payload != NULL && payload->Data != NULL && pluginGoatTracker != NULL)
		{
			bool preview = pluginGoatTracker->viewInstrumentsBrowser != NULL
				? pluginGoatTracker->viewInstrumentsBrowser->previewOnClick : false;
			pluginGoatTracker->LoadInstrumentFromFile((const char *)payload->Data, einum, preview);
		}
		ImGui::EndDragDropTarget();
	}

	GT2_PropagateChildWindowFocus(this);
	PostRenderImGui();
}

bool CViewGT2Instrument::DoDropFile(char *filePath)
{
	if (filePath == NULL) return false;
	if (!CViewGT2InstrumentsBrowser::IsInstrumentFile(filePath)) return false;
	if (pluginGoatTracker == NULL) return false;

	bool preview = pluginGoatTracker->viewInstrumentsBrowser != NULL
		? pluginGoatTracker->viewInstrumentsBrowser->previewOnClick : false;
	return pluginGoatTracker->LoadInstrumentFromFile(filePath, einum, preview);
}

bool CViewGT2Instrument::HandleInstrumentTablePointerEnter(bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editmode != EDIT_INSTRUMENT) return false;
	if (eipos < 2 || eipos > 5) return false;
	int table = eipos - 2;
	if (einum <= 0 || einum >= MAX_INSTR) return true;
	// Shift+Enter on a populated Speed Table pointer detaches this
	// instrument's vibrato from any other instrument sharing the same
	// speed-table entry — `makespeedtable(...,1)` allocates a fresh slot,
	// copies the current data, and returns its index. Mirrors the
	// `case 5 + shiftpressed` branch in gt2/ginstr.c:152. We call it
	// directly now instead of forwarding to native GT2's gconsole loop
	// (which is dead in Renoise mode after the routing rewrite).
	if (table == STBL && isShift && ginstr[einum].ptr[STBL])
	{
		ginstr[einum].ptr[STBL] = (unsigned char)(makespeedtable(ginstr[einum].ptr[STBL], finevibrato, 1) + 1);
		return true;
	}
	(void)isAlt; (void)isControl; (void)isSuper;

	if (ginstr[einum].ptr[table])
	{
		gototable(table, ginstr[einum].ptr[table] - 1);
	}
	else
	{
		editmode = EDIT_TABLES;
		etnum = table;
		etcolumn = 0;
		if (table < STBL)
			InsertTableRow();
		else
			AllocateSpeedtableEntry();
	}
	return true;
}

bool CViewGT2Instrument::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_ENTER
		&& HandleInstrumentTablePointerEnter(isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Left / Right walk the table columns. Native tablecommands() (gtable.c:99)
	// wraps to the next table through etview[], a pool-absolute scroll offset
	// shared by every instrument — so crossing a table edge here would drop the
	// cursor onto rows this instrument does not own, rows only the GT2 Tables
	// view shows. Keep the walk inside the instrument's own slices instead.
	// Ctrl/Cmd+Left/Right belong to Renoise (order-list pattern number), so
	// they stay out of this block and reach the dispatcher below.
	if ((keyCode == MTKEY_ARROW_LEFT || keyCode == MTKEY_ARROW_RIGHT)
		&& !isControl && !isSuper
		&& editmode == EDIT_TABLES && !eamode
		&& einum > 0 && einum < MAX_INSTR
		&& etnum >= 0 && etnum < MAX_TABLES)
	{
		int sliceStart[MAX_TABLES];
		int sliceLen[MAX_TABLES];
		for (int c = 0; c < MAX_TABLES; c++)
		{
			sliceStart[c] = ginstr[einum].ptr[c] ? ginstr[einum].ptr[c] - 1 : -1;
			sliceLen[c]   = (sliceStart[c] >= 0) ? gettablepartlen(c, sliceStart[c]) : 0;
		}
		if (sliceLen[etnum] > 0)
		{
			int table     = etnum;
			int row       = etpos - sliceStart[etnum];
			int column    = etcolumn;
			int direction = (keyCode == MTKEY_ARROW_RIGHT) ? 1 : -1;
			if (GT2InstrumentTableCursorStep(direction, sliceLen, &table, &row, &column))
			{
				etnum    = table;
				etpos    = sliceStart[table] + row;
				etcolumn = column;
				if (isShift) etmarknum = -1;   // mirrors gtable.c:107
			}
			return true;
		}
	}

	// Insert / Delete a table row — wavetable / pulsetable / filtertable only.
	if (editmode == EDIT_TABLES && etnum >= 0 && etnum < STBL)
	{
		bool doInsert = (isShift && keyCode == MTKEY_ARROW_DOWN) || keyCode == MTKEY_INSERT;
		bool doDelete = (isShift && keyCode == MTKEY_ARROW_UP)   || keyCode == MTKEY_DELETE;
		if (doInsert) { InsertTableRow(); return true; }
		if (doDelete) { DeleteTableRow(); return true; }
	}

	// Native ginstr.c is the source of truth for hex-digit field entry
	// (attack/decay, pulse/wave/filter pointers, wave table value, …) —
	// forward unconditionally to native so the user can type into fields
	// even under KEY_RENOISE.
	return GT2_HandleRenoiseOrForwardKeyDownToNative(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2Instrument::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2Instrument::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}
