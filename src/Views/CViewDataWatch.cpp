#include "SYS_Defs.h"
#include "DBG_Log.h"
#include "CViewDataWatch.h"
#include "CDebugSourceWatch.h"
#include "CDebugVariableResolver.h"
#include "CViewC64.h"
#include "CViewC64Screen.h"
#include "CImageData.h"
#include "CSlrImage.h"
#include "CViewC64.h"
#include "MTH_Random.h"
#include "VID_ImageBinding.h"
#include "SYS_KeyCodes.h"
#include "CViewDataDump.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "C64Opcodes.h"
#include "CViewDataMap.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CGuiMain.h"

#include <math.h>

// TODO: Address/Label/Value cell text should be vertically centered against the
// Format combo and trash button on the same row. Attempts using
// AlignTextToFramePadding(), explicit SetCursorPosY with FramePadding.y offset,
// pushing CellPadding.y=0 + TableNextRow(min_row_height), and rendering via
// DrawList->AddText with a reserved InvisibleButton/Dummy slot all FAILED to
// visibly centre the text — ImGui still rendered it anchored to the top of the
// cell for reasons we could not determine. Left as top-aligned for now.

static bool TrashIconButton(const char *id)
{
	ImGuiStyle &style = ImGui::GetStyle();
	const float size = ImGui::GetFrameHeight();
	const ImVec2 padBackup = style.FramePadding;
	style.FramePadding = ImVec2(0.0f, 0.0f);
	const bool clicked = ImGui::Button(id, ImVec2(size, size));
	style.FramePadding = padBackup;

	const ImVec2 bmin = ImGui::GetItemRectMin();
	const ImVec2 bmax = ImGui::GetItemRectMax();
	const float w = bmax.x - bmin.x;
	const float h = bmax.y - bmin.y;
	const float cx = bmin.x + w * 0.5f;
	const float cy = bmin.y + h * 0.5f;
	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	ImDrawList *dl = ImGui::GetWindowDrawList();

	const float bw = w * 0.34f;  // body half-width
	const float bh = h * 0.32f;  // body half-height
	const float lidY = cy - bh * 0.55f;
	const float bodyTop = lidY + 1.5f;
	const float bodyBot = cy + bh;
	const float bodyL = cx - bw * 0.80f;
	const float bodyR = cx + bw * 0.80f;

	// Lid (wider horizontal line)
	dl->AddLine(ImVec2(cx - bw, lidY), ImVec2(cx + bw, lidY), col, 1.5f);
	// Handle (small arch above lid)
	const float handleW = bw * 0.45f;
	const float handleY = lidY - h * 0.10f;
	dl->AddLine(ImVec2(cx - handleW, handleY), ImVec2(cx + handleW, handleY), col, 1.5f);
	dl->AddLine(ImVec2(cx - handleW, handleY), ImVec2(cx - handleW, lidY),   col, 1.5f);
	dl->AddLine(ImVec2(cx + handleW, handleY), ImVec2(cx + handleW, lidY),   col, 1.5f);
	// Body sides (trapezoid-ish — slight taper)
	dl->AddLine(ImVec2(bodyL,                bodyTop), ImVec2(bodyL + w*0.03f, bodyBot), col, 1.5f);
	dl->AddLine(ImVec2(bodyR,                bodyTop), ImVec2(bodyR - w*0.03f, bodyBot), col, 1.5f);
	// Body bottom
	dl->AddLine(ImVec2(bodyL + w*0.03f, bodyBot), ImVec2(bodyR - w*0.03f, bodyBot), col, 1.5f);
	// Vertical slits inside
	dl->AddLine(ImVec2(cx - bw*0.38f, bodyTop + 2), ImVec2(cx - bw*0.38f, bodyBot - 2), col, 1.0f);
	dl->AddLine(ImVec2(cx,            bodyTop + 2), ImVec2(cx,            bodyBot - 2), col, 1.0f);
	dl->AddLine(ImVec2(cx + bw*0.38f, bodyTop + 2), ImVec2(cx + bw*0.38f, bodyBot - 2), col, 1.0f);

	return clicked;
}

CViewDataWatch::CViewDataWatch(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
							   CDebugSymbols *symbols, CViewDataMap *viewMemoryMap)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	viewDataDump = NULL;
	
	this->symbols = symbols;
	this->debugInterface = symbols->debugInterface;
	this->dataAdapter = symbols->dataAdapter;
	this->viewMemoryMap = viewMemoryMap;	
}

void CViewDataWatch::DoLogic()
{
	//
}

void CViewDataWatch::RenderImGui()
{
	PreRenderImGui();

	symbols->LockMutex();
	
	CDebugSymbolsSegment *symbolsSegment = symbols->currentSegment;
	if (!symbolsSegment)
	{
		LOGError("CViewDataWatch::RenderImGui: symbols segment is NULL");
		symbols->UnlockMutex();
		return;
	}
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "##DataWatchTable_%s", dataAdapter->adapterID);

	CDebugSymbolsDataWatch *watchToDelete = NULL;
	CDebugSourceWatch *sourceWatchToDelete = NULL;

	// address | label | value | format | delete
	if (ImGui::BeginTable(buf, 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders))
	{
		const float trashColWidth = ImGui::GetFrameHeight() + ImGui::GetStyle().CellPadding.x * 2.0f;
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed,   ImGui::CalcTextSize("FFFFFF").x);
		ImGui::TableSetupColumn("Label",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("Format",  ImGuiTableColumnFlags_WidthStretch, 1.6f);
		ImGui::TableSetupColumn("",        ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoSort, trashColWidth);
		ImGui::TableHeadersRow();

		for (std::map<int, CDebugSymbolsDataWatch *>::iterator it = symbolsSegment->watches.begin();
			 it != symbolsSegment->watches.end(); it++)
		{
			CDebugSymbolsDataWatch *watch = it->second;

			ImGui::TableNextColumn();
			sprintf(buf, "%04X", watch->address);
			ImGui::Text(buf);

			ImGui::TableNextColumn();
			CDebugSymbolsCodeLabel *codeLabel = symbolsSegment->FindLabel(watch->address);
			if (codeLabel)
			{
				ImGui::Text(codeLabel->GetLabelText());
			}
			else
			{
				ImGui::Text("");
			}

			ImGui::TableNextColumn();

			if (watch->representation == WATCH_REPRESENTATION_TEXT)
			{
				// TODO: WATCH_REPRESENTATION_TEXT   petsci, atasci?
			}
			else
			{
				// determine byte span of this watch
				int numBytes = 1;
				switch (watch->representation)
				{
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						numBytes = 2; break;
					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						numBytes = 4; break;
					default: break;
				}

				// pick strongest recent-access fade color across the byte range
				if (symbols->memory)
				{
					float hr = 0.0f, hg = 0.0f, hb = 0.0f, ha = 0.0f;
					for (int i = 0; i < numBytes; i++)
					{
						CDebugMemoryCell *c = symbols->memory->GetMemoryCell(watch->address + i);
						if (!c) continue;
						u8 v = watch->ReadByte(i, NULL);
						c->UpdateCellColors(v, false, 0);
						if (c->sa > ha) { hr = c->sr; hg = c->sg; hb = c->sb; ha = c->sa; }
					}
					if (ha > 0.0f)
					{
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
							ImGui::ColorConvertFloat4ToU32(ImVec4(hr, hg, hb, ha)));
					}
				}

				// RD-059: a watch bound to its memory reads it there when the CPU
				// has something else mapped at its address
				bool elsewhere = false;
				auto readByte = [&](int i) -> u32 { bool e = false; u8 v = watch->ReadByte(i, &e); elsewhere = elsewhere || e; return v; };
				u32 value = 0;
				switch(watch->representation)
				{
					default:
					case WATCH_REPRESENTATION_BIN:
					case WATCH_REPRESENTATION_HEX_8:
						value = readByte(0);
						break;
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
						value =   readByte(0)
								| readByte(1) << 8;
						break;
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						value =   readByte(0) << 8
								| readByte(1);
						break;

					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
						value =   readByte(0)
								| readByte(1) << 8
								| readByte(2) << 16
								| readByte(3) << 24;
						break;
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						value =   readByte(0) << 24
								| readByte(1) << 16
								| readByte(2) << 8
								| readByte(3);
						break;
				}

				// TODO: colors
//				if (cell->isExecuteCode)
//				{
//					colorExecuteCodeR, colorExecuteCodeG, colorExecuteCodeB, colorExecuteCodeA
//				}
//				else if (cell->isExecuteArgument)
//				{
//					colorExecuteArgumentR, colorExecuteArgumentG, colorExecuteArgumentB, colorExecuteArgumentA
//				}
//				ON TOP: cell->sr, cell->sg, cell->sb, cell->sa);


				switch(watch->representation)
				{
					default:
					case WATCH_REPRESENTATION_HEX_8:
						ImGui::Text("%02x", value);
						break;
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
						ImGui::Text("%04x", value);
						break;
					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
						ImGui::Text("%08x", value);
						break;
					case WATCH_REPRESENTATION_UNSIGNED_DEC_8:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
						ImGui::Text("%u", value);
						break;
					case WATCH_REPRESENTATION_SIGNED_DEC_8:
						ImGui::Text("%d", (i8)value);
						break;
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						ImGui::Text("%d", (i16)value);
						break;
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						ImGui::Text("%d", (i32)value);
						break;
					case WATCH_REPRESENTATION_BIN:
						FUN_IntToBinaryStr(value, buf, 8);
						ImGui::Text(buf);
						break;
				}
				if (elsewhere)
				{
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "(%s)", watch->MemoryName().c_str());
				}
				if (watch->hasPhysical && ImGui::IsItemHovered())
					ImGui::SetTooltip(elsewhere ? "%s, not mapped at $%04X now: read from that memory" : "%s, mapped at $%04X now",
									  watch->MemoryName().c_str(), watch->address);
			}

			ImGui::TableNextColumn();

			sprintf(buf, "##DataWatchCombo%x", watch);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo(buf, &(watch->representation), "Hex 8-bits\0Hex 16-bits LE\0Hex 16-bits BE\0Hex 32-bits LE\0Hex 32-bits BE\0Unsigned Dec 8-bits\0Unsigned Dec 16-bits LE\0Unsigned Dec 16-bits BE\0Unsigned Dec 32-bits LE\0Unsigned Dec 32-bits BE\0Signed Dec 8-bits\0Signed Dec 16-bits LE\0Signed Dec 16-bits BE\0Signed Dec 32-bits LE\0Signed Dec 32-bits BE\0Binary\0\0"); //Text\0\0");

			ImGui::TableNextColumn();
			sprintf(buf, "##del%x", watch);
			if (TrashIconButton(buf))
			{
				watchToDelete = watch;
			}
		}

		// RD-053: watches by C variable name. A fixed global is read live; the
		// rest are resolved while the machine is stopped and keep the last
		// stop's value, greyed, while it runs
		bool paused = debugInterface->GetDebugMode() == DEBUGGER_MODE_PAUSED;
		symbols->UpdateSourceWatches(paused);
		for (CDebugSourceWatch *watch : symbols->sourceWatches)
		{
			const CDebugVariableResolution &r = watch->value;
			bool stale = watch->hasValue && !watch->live;

			ImGui::TableNextColumn();
			if (r.hasAddress)
				ImGui::TextDisabled("%04X", r.address);
			else
				ImGui::TextDisabled("-");

			ImGui::TableNextColumn();
			if (watch->function.empty())
				ImGui::Text("%s", watch->name.c_str());
			else
				ImGui::Text("%s  (%s)", watch->name.c_str(), watch->function.c_str());

			ImGui::TableNextColumn();
			std::string text = watch->FormatValue();
			if (!watch->hasValue)
				ImGui::TextDisabled("<not resolved yet>");
			else if (!watch->state.empty() && text.empty())
				ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "<%s>", watch->state.c_str());
			else if (stale)
				ImGui::TextDisabled("%s", text.c_str());
			else if (r.quality != DebugMappingQuality::Exact)
				ImGui::Text("%s ~", text.c_str());
			else
				ImGui::Text("%s", text.c_str());
			if (ImGui::IsItemHovered())
			{
				std::string tip = (r.type.empty() ? std::string() : r.type + "  ")
					+ (r.variable ? DebugVariableLocationName(r.variable->location) : "");
				if (stale)
					tip += "\nrunning, value at last stop";
				if (!r.reason.empty())
					tip += "\n" + r.reason;
				if (!tip.empty())
					ImGui::SetTooltip("%s", tip.c_str());
			}

			ImGui::TableNextColumn();
			sprintf(buf, "##SourceWatchCombo%p", (void *)watch);
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo(buf, &(watch->representation), "Hex 8-bits\0Hex 16-bits LE\0Hex 16-bits BE\0Hex 32-bits LE\0Hex 32-bits BE\0Unsigned Dec 8-bits\0Unsigned Dec 16-bits LE\0Unsigned Dec 16-bits BE\0Unsigned Dec 32-bits LE\0Unsigned Dec 32-bits BE\0Signed Dec 8-bits\0Signed Dec 16-bits LE\0Signed Dec 16-bits BE\0Signed Dec 32-bits LE\0Signed Dec 32-bits BE\0Binary\0\0");

			ImGui::TableNextColumn();
			sprintf(buf, "##delsrc%p", (void *)watch);
			if (TrashIconButton(buf))
				sourceWatchToDelete = watch;
		}

		ImGui::EndTable();
	}

	if (watchToDelete)
	{
		symbols->currentSegment->DeleteWatch(watchToDelete->address);
	}
	if (sourceWatchToDelete)
	{
		symbols->RemoveSourceWatch(sourceWatchToDelete);
	}

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		ImGui::OpenPopup("addWatchPopup");

		addWatchPopupAddr = 0;
		addWatchPopupSymbol[0] = '\0';
		comboFilterTextBuf[0] = 0;

		imGuiOpenPopupFrame = 0;
	}
	
	if (ImGui::BeginPopup("addWatchPopup"))
	{
		ImGui::Text("Add watch point");
		ImGui::Separator();

		sprintf(buf, "##addWatchPopupAddress%x", this);

		const char **hints = symbols->currentSegment ? symbols->currentSegment->codeLabelsArray : NULL;
		int numHints = symbols->currentSegment ? symbols->currentSegment->numCodeLabelsInArray : 0;

		if (imGuiOpenPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenPopupFrame++;
		}

		ImGui::SetNextItemWidth(220.0f);
		bool inputActivated = ImGui::InputText(buf, comboFilterTextBuf, IM_ARRAYSIZE(comboFilterTextBuf),
											   ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::SameLine();
		bool buttonAddClicked = ImGui::Button("Add");

		// build filtered list of matching label indices
		const bool filterIsHex = (comboFilterTextBuf[0] != 0) && FUN_IsHexNumber(comboFilterTextBuf);
		char needleLower[MAX_LABEL_TEXT_BUFFER_SIZE];
		int nlen = 0;
		for (int i = 0; comboFilterTextBuf[i] != 0 && i < (int)sizeof(needleLower) - 1; i++)
		{
			char c = comboFilterTextBuf[i];
			needleLower[nlen++] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
		}
		needleLower[nlen] = 0;

		const int kMaxMatches = 512;
		int matchIdx[kMaxMatches];
		int matchCount = 0;
		if (hints != NULL && numHints > 0 && !filterIsHex)
		{
			for (int n = 0; n < numHints && matchCount < kMaxMatches; n++)
			{
				const char *hay = hints[n];
				if (hay == NULL) continue;

				bool match = true;
				if (nlen > 0)
				{
					match = false;
					for (const char *p = hay; *p && !match; p++)
					{
						const char *h = p;
						const char *q = needleLower;
						while (*h && *q)
						{
							char hc = *h;
							if (hc >= 'A' && hc <= 'Z') hc += 32;
							if (hc != *q) break;
							h++; q++;
						}
						if (*q == 0) match = true;
					}
				}
				if (match) matchIdx[matchCount++] = n;
			}
		}

		// clamp active index to current match range
		if (comboFilterState.activeIdx >= matchCount) comboFilterState.activeIdx = matchCount > 0 ? matchCount - 1 : 0;
		if (comboFilterState.activeIdx < 0) comboFilterState.activeIdx = 0;

		// arrow-key navigation (single-line InputText doesn't consume up/down)
		bool arrowScrolled = false;
		if (matchCount > 0)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			{
				if (comboFilterState.activeIdx < matchCount - 1) comboFilterState.activeIdx++;
				arrowScrolled = true;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			{
				if (comboFilterState.activeIdx > 0) comboFilterState.activeIdx--;
				arrowScrolled = true;
			}
		}

		// render list (always visible when hints exist, even when no filter typed)
		if (hints != NULL && numHints > 0)
		{
			ImGui::BeginChild("##watchHints", ImVec2(280.0f, 160.0f), true);
			for (int i = 0; i < matchCount; i++)
			{
				int n = matchIdx[i];
				const char *hay = hints[n];
				const bool is_selected = (i == comboFilterState.activeIdx);
				ImGui::PushID(n);
				if (ImGui::Selectable(hay, is_selected))
				{
					strcpy(comboFilterTextBuf, hay);
					comboFilterState.activeIdx = i;
					inputActivated = true;
				}
				if (is_selected && arrowScrolled)
				{
					ImGui::SetScrollHereY();
				}
				ImGui::PopID();
			}
			ImGui::EndChild();
		}

		// if Enter pressed in the input and a highlighted match exists, use it
		if (inputActivated && matchCount > 0 && !filterIsHex
			&& comboFilterState.activeIdx >= 0 && comboFilterState.activeIdx < matchCount)
		{
			strcpy(comboFilterTextBuf, hints[matchIdx[comboFilterState.activeIdx]]);
		}

		bool finalizeAddingBreakpoint = buttonAddClicked || inputActivated;
		
		if (finalizeAddingBreakpoint)
		{
			CDebugSymbolsCodeLabel *label = symbols->currentSegment->FindLabelByText(comboFilterTextBuf);
			
			char *hexNumberBuf = comboFilterTextBuf;
			if (comboFilterTextBuf[0] == '$' && comboFilterTextBuf[1] != 0)
			{
				hexNumberBuf = comboFilterTextBuf + 1;
			}
			
			if (label)
			{
				addWatchPopupAddr = label->address;
			}
			else if (FUN_IsHexNumber(hexNumberBuf))
			{
				FUN_ToUpperCaseStr(comboFilterTextBuf);
				addWatchPopupAddr = FUN_HexStrToValue(hexNumberBuf);
			}
			else
			{
				char *buf = SYS_GetCharBuf();
				sprintf(buf, "Invalid address or symbol:\n%s", comboFilterTextBuf);
				guiMain->ShowMessageBox("Can't add watch", buf);
				SYS_ReleaseCharBuf(buf);
				addWatchPopupAddr = -1;
			}

			if (addWatchPopupAddr >= 0)
			{
				addWatchPopupAddr = URANGE(0, addWatchPopupAddr, dataAdapter->AdapterGetDataLength());

				symbols->currentSegment->AddWatch(addWatchPopupAddr, "");
				// RD-059: the watch keeps the memory at that address now
				CDebugSymbolsDataWatch *added = symbols->currentSegment->FindWatch(addWatchPopupAddr);
				if (added)
					added->BindToMemory();
				
//				CBreakpointAddr *breakpoint = new CBreakpointAddr(addBreakpointPopupAddr);
//				AddBreakpoint(breakpoint);
//				UpdateRenderBreakpoints();

				ImGui::CloseCurrentPopup();
			}
		}
						
		////
		
		ImGui::EndPopup();
	}
	
	symbols->UnlockMutex();
	
	SYS_ReleaseCharBuf(buf);

	PostRenderImGui();
}

void CViewDataWatch::SetViewC64DataDump(CViewDataDump *viewDataDump)
{
	this->viewDataDump = viewDataDump;
}

bool CViewDataWatch::ComboFilterShouldOpenPopupCallback(const char *label, char *buffer, int bufferlen,
												const char **hints, int num_hints, ImGui::ComboFilterState *s)
{
	// do not need to open combo popup when number is hex
	if (FUN_IsHexNumber(buffer))
	{
		return false;
	}

	if (hints == NULL)
	{
		return false;
	}

	if (num_hints == 0)
	{
		return false;
	}

	return (buffer[0] != 0) && strcmp(buffer, hints[s->activeIdx]);
}

CViewDataWatch::~CViewDataWatch()
{
	
}
