#include "CDebugBreakpointAddr.h"
#include "CDebugBreakpointsAddr.h"
#include <algorithm>
#include "CDebugBreakpoint.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "GUI_Main.h"

#include "CDebugBreakpointEventCallback.h"
#include "CDebuggerServer.h"
#include "CViewC64.h"

// std::map<int, CBreakpointMemory *> memoryBreakpoints;

CDebugBreakpointsAddr::CDebugBreakpointsAddr(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, const char *addressFormatStr, int minAddr, int maxAddr)
{
	this->breakpointsType = breakpointType;
	this->breakpointsTypeStr = breakpointTypeStr;
	this->segment = segment;
	this->symbols = segment->symbols;
	this->addressFormatStr = addressFormatStr;
	this->minAddr = minAddr;
	this->maxAddr = maxAddr;
	comboFilterState = {0, false};
	comboFilterTextBuf[0] = 0;

	renderBreakpointsMutex = new CSlrMutex("CDebugBreakpointsAddr::renderBreakpointsMutex");

	addressNameJsonStr = "addr";
	addBreakpointPopupHeadlineStr = "Add PC Breakpoint";
	addBreakpointPopupAddrStr = "Address";
	addBreakpointPopupAddrInputFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
	addBreakpointsTableColumnAddrStr = "Address";
	
	temporaryBreakpointPC = -1;

	// TODO: workaround, remove me when ImGui bug is fixed: https://github.com/ocornut/imgui/issues/1655
	imGuiColumnsWidthWorkaroundFrame = 0;
		
	imGuiOpenPopupFrame = 999;
}

// address -1 means no breakpoint
void CDebugBreakpointsAddr::SetTemporaryBreakpointPC(int address)
{
	this->temporaryBreakpointPC = address;
}

int CDebugBreakpointsAddr::GetTemporaryBreakpointPC()
{
	return this->temporaryBreakpointPC;
}

void CDebugBreakpointsAddr::ClearBreakpoints()
{
	while(!breakpoints.empty())
	{
		std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.begin();
		CDebugBreakpointAddr *breakpoint = it->second;
		
		breakpoints.erase(it);
		delete breakpoint;
	}
}

CDebugBreakpoint *CDebugBreakpointsAddr::CreateEmptyBreakpoint()
{
	return new CDebugBreakpointAddr(symbols, 0);
}

void CDebugBreakpointsAddr::Serialize(Hjson::Value hjsonBreakpoints)
{
	for (std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
	{
		CDebugBreakpointAddr *breakpoint = it->second;
		Hjson::Value hjsonBreakpoint;
		breakpoint->Serialize(hjsonBreakpoint);
		
		CDebugSymbolsCodeLabel *label = segment->FindLabel(breakpoint->addr);
		if (label)
		{
			hjsonBreakpoint["Label"] = label->GetLabelText();
		}
		
		hjsonBreakpoints.push_back(hjsonBreakpoint);
	}
}

void CDebugBreakpointsAddr::Deserialize(Hjson::Value hjsonBreakpoints)
{
	for (int index = 0; index < hjsonBreakpoints.size(); ++index)
	{
		Hjson::Value hjsonBreakpoint = hjsonBreakpoints[index];
		CDebugBreakpointAddr *breakpoint = (CDebugBreakpointAddr*)CreateEmptyBreakpoint();
		breakpoint->Deserialize(hjsonBreakpoint);
		
		// check label
		Hjson::Value hjsonBreakpointLabel = hjsonBreakpoint["Label"];
		if (hjsonBreakpointLabel != Hjson::Type::Undefined)
		{
			const char *labelStr = hjsonBreakpoint["Label"];
			CDebugSymbolsCodeLabel *label = segment->FindLabelByText(labelStr);
			if (label)
			{
				breakpoint->addr = label->address;
			}
		}
		
		AddBreakpoint(breakpoint);
	}
}

void CDebugBreakpointsAddr::AddBreakpoint(CDebugBreakpointAddr *breakpoint)
{
	// check if breakpoint is already in the map and remove it, can be with other addr so we can't use find
	for (std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
	{
		CDebugBreakpointAddr *existingBreakpoint = it->second;
		
		if (existingBreakpoint == breakpoint)
		{
			breakpoints.erase(it);
			break;
		}
	}
	
	// check if there's a breakpoint with the same address and delete it (we are replacing it)
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.find(breakpoint->addr);
	if (it != breakpoints.end())
	{
		CDebugBreakpointAddr *existingBreakpoint = it->second;
		breakpoints.erase(it);
		delete existingBreakpoint;
	}

	// set symbols
	breakpoint->symbols = symbols;

	// add a breakpoint
	breakpoints[breakpoint->addr] = breakpoint;
}

CDebugBreakpointAddr *CDebugBreakpointsAddr::GetBreakpoint(int addr)
{
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it == breakpoints.end())
		return NULL;
	
	return it->second;
}

u64 CDebugBreakpointsAddr::DeleteBreakpoint(int addr)
{
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it != breakpoints.end())
	{
		CDebugBreakpointAddr *breakpoint = it->second;
		u64 breakpointId = breakpoint->breakpointId;
		breakpoints.erase(it);
		delete breakpoint;
		
		return breakpointId;
	}
	
	return UNKNOWN_BREAKPOINT_ID;
}

void CDebugBreakpointsAddr::DeleteBreakpoint(CDebugBreakpointAddr *breakpoint)
{
	this->DeleteBreakpoint(breakpoint->addr);
}

void CDebugBreakpointsAddr::RemoveBreakpoint(CDebugBreakpointAddr *breakpoint)
{
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.find(breakpoint->addr);
	if (it != breakpoints.end())
	{
		breakpoints.erase(it);
	}
	
	UpdateRenderBreakpoints();
}


// TODO: create a condition parser (tree for condition) and parse the condition text
CDebugBreakpointAddr *CDebugBreakpointsAddr::EvaluateBreakpoint(int addr)
{
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it != breakpoints.end())
	{
		CDebugBreakpointAddr *breakpoint = it->second;
		if (breakpoint->isActive == false)
			return NULL;

		// RD-025: a breakpoint bound to bank(s) of a banked image
		if (!breakpoint->imageOffsets.empty())
		{
			u64 imageOffset = 0;
			CDebugInterface *debugInterface = (segment && segment->symbols) ? segment->symbols->debugInterface : NULL;
			if (debugInterface == NULL || !debugInterface->GetFetchImageOffset((u16)addr, &imageOffset)
				|| std::find(breakpoint->imageOffsets.begin(), breakpoint->imageOffsets.end(), imageOffset) == breakpoint->imageOffsets.end())
				return NULL;
		}

		// RD-025: a breakpoint bound to physical byte(s), whatever bank is in
		if (!breakpoint->physicalIdentities.empty())
		{
			CDebugInterface *debugInterface = (segment && segment->symbols) ? segment->symbols->debugInterface : NULL;
			CDebugMemoryMappingResolver *resolver = debugInterface ? debugInterface->GetMemoryMappingResolver() : NULL;
			if (resolver == NULL)
				return NULL;
			CDebugPhysicalAddress fetched = resolver->ResolveFetch((u16)addr).physical;
			if (std::find(breakpoint->physicalIdentities.begin(), breakpoint->physicalIdentities.end(), fetched)
				== breakpoint->physicalIdentities.end())
				return NULL;
		}
		
		if (breakpoint->callback)
		{
			if (breakpoint->callback->DebugBreakpointEvaluateCallback(breakpoint) == false)
				return NULL;
		}
		
		// flag breakpoint to Server API
		if (viewC64->debuggerServer)
		{
			if (breakpoint && viewC64->debuggerServer->AreClientsConnected())
			{
				nlohmann::json j;
				breakpoint->GetDetailsJson(j);
				j[addressNameJsonStr] = addr;
				viewC64->debuggerServer->BroadcastEvent("breakpoint", j);
			}
		}

		return breakpoint;
	}
	
	return NULL;
}

//
bool CDebugBreakpointsAddr::ComboFilterShouldOpenPopupCallback(const char *label, char *buffer, int bufferlen,
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

void CDebugBreakpointsAddr::RenderImGui()
{
//	LOGD("CDebugBreakpointsAddr::RenderImGui");
		
	char *buf = SYS_GetCharBuf();
	
	ImVec4 colorNotActive(0.5, 0.5, 0.5, 1);
	ImVec4 colorActive(1.0, 1.0, 1.0, 1);

	symbols->LockMutex();

	CDebugBreakpointAddr *deleteBreakpoint = NULL;

	sprintf(buf, "##BreakpointsAddrTable_%s", symbols->dataAdapter->adapterID);

	// active | address | symbol | delete
	if (ImGui::BeginTable(buf, 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders))
	{
		u32 i = 0;
		
//		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.65f)));

		ImGui::TableNextColumn();
		ImGui::Text("Active");
		ImGui::TableNextColumn();
		ImGui::Text(addBreakpointsTableColumnAddrStr);
		ImGui::TableNextColumn();
		ImGui::Text("Label");
		ImGui::TableNextColumn();
		ImGui::Text("");

//	   	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0);

		for (std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
		{
			CDebugBreakpointAddr *bp = it->second;
					
			sprintf(buf, "##chkBoxAddr%x%d", this, i);
			
			ImGui::TableNextColumn();
			if (ImGui::Checkbox(buf, &bp->isActive))
			{
				symbols->UpdateRenderBreakpoints();
			}

			ImGui::TableNextColumn();
			ImVec4 color = bp->isActive ? colorActive : colorNotActive;
			
			ImGui::TextColored(color, addressFormatStr, bp->addr);
			
			ImGui::TableNextColumn();
			CDebugSymbolsCodeLabel *label = NULL;
			if (symbols->currentSegment)
			{
				label = symbols->currentSegment->FindLabel(bp->addr);
			}

			ImGui::TextColored(color, label ? label->GetLabelText() : "");
			
			ImGui::TableNextColumn();
			sprintf(buf, "X##%x%d", this, i);
			if (ImGui::Button(buf))
			{
				// delete breakpoint
				deleteBreakpoint = bp;
			}

			i++;
		}
				
		ImGui::EndTable();
	}

	if (deleteBreakpoint)
	{
		DeleteBreakpoint(deleteBreakpoint);
		symbols->UpdateRenderBreakpoints();
	}

	
	///

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		ImGui::OpenPopup("addAddrBreakpointPopup");

		addBreakpointPopupAddr = 0;
		addBreakpointPopupSymbol[0] = '\0';
		comboFilterTextBuf[0] = 0;
		comboFilterState.activeIdx = 0;

		imGuiOpenPopupFrame = 0;
	}

	if (ImGui::BeginPopup("addAddrBreakpointPopup"))
	{
		ImGui::Text(addBreakpointPopupHeadlineStr);
		ImGui::Separator();

		const bool isRasterPopup = (breakpointsType == BREAKPOINT_TYPE_RASTER_LINE);

		sprintf(buf, "##addAddrBreakpointPopupAddress_%s", symbols->dataAdapter->adapterID);

		if (imGuiOpenPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenPopupFrame++;
		}

		// Raster has no labels, so input is hex-only. PC accepts label text (free-form).
		ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
		if (isRasterPopup)
		{
			inputFlags |= ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
		}

		ImGui::SetNextItemWidth(220.0f);
		bool inputActivated = ImGui::InputText(buf, comboFilterTextBuf, IM_ARRAYSIZE(comboFilterTextBuf), inputFlags);

		ImGui::SameLine();
		bool buttonAddClicked = ImGui::Button("Add");

		// Hint list: PC-style popups only (raster has no labels).
		const char **hints = NULL;
		int numHints = 0;
		if (!isRasterPopup && symbols->currentSegment)
		{
			hints = symbols->currentSegment->codeLabelsArray;
			numHints = symbols->currentSegment->numCodeLabelsInArray;
		}

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

		if (comboFilterState.activeIdx >= matchCount) comboFilterState.activeIdx = matchCount > 0 ? matchCount - 1 : 0;
		if (comboFilterState.activeIdx < 0) comboFilterState.activeIdx = 0;

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

		if (hints != NULL && numHints > 0)
		{
			ImGui::BeginChild("##addrBpHints", ImVec2(280.0f, 160.0f), true);
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

		// Enter on a highlighted match commits that label.
		if (inputActivated && matchCount > 0 && !filterIsHex
			&& comboFilterState.activeIdx >= 0 && comboFilterState.activeIdx < matchCount)
		{
			strcpy(comboFilterTextBuf, hints[matchIdx[comboFilterState.activeIdx]]);
		}

		bool finalizeAddingBreakpoint = buttonAddClicked || inputActivated;

		if (finalizeAddingBreakpoint)
		{
			CDebugSymbolsCodeLabel *label = symbols->currentSegment ? symbols->currentSegment->FindLabelByText(comboFilterTextBuf) : NULL;
			if (label)
			{
				addBreakpointPopupAddr = label->address;
			}
			else if (FUN_IsHexNumber(comboFilterTextBuf))
			{
				FUN_ToUpperCaseStr(comboFilterTextBuf);
				addBreakpointPopupAddr = FUN_HexStrToValue(comboFilterTextBuf);
			}
			else
			{
				char *errBuf = SYS_GetCharBuf();
				sprintf(errBuf, "Invalid address or symbol:\n%s", comboFilterTextBuf);
				guiMain->ShowMessageBox("Can't add breakpoint", errBuf);
				SYS_ReleaseCharBuf(errBuf);
				addBreakpointPopupAddr = -1;
			}

			if (addBreakpointPopupAddr >= 0)
			{
				addBreakpointPopupAddr = URANGE(minAddr, addBreakpointPopupAddr, maxAddr);

				CDebugBreakpointAddr *breakpoint = (CDebugBreakpointAddr*)CreateEmptyBreakpoint();
				breakpoint->addr = addBreakpointPopupAddr;
				AddBreakpoint(breakpoint);
				UpdateRenderBreakpoints();

				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	symbols->UnlockMutex();

	SYS_ReleaseCharBuf(buf);
}

void CDebugBreakpointsAddr::UpdateRenderBreakpoints()
{
	renderBreakpointsMutex->Lock();
	symbols->LockMutex();

	renderBreakpoints.clear();
	for (std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.begin();
		 it != breakpoints.end(); it++)
	{
		CDebugBreakpointAddr *breakpoint = it->second;
		renderBreakpoints[breakpoint->addr] = breakpoint;
	}

	symbols->UnlockMutex();
	this->renderBreakpointsMutex->Unlock();
}

CDebugBreakpointsAddr::~CDebugBreakpointsAddr()
{
	ClearBreakpoints();
}

