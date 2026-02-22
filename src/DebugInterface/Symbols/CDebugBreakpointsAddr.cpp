#include "CDebugBreakpointAddr.h"
#include "CDebugBreakpointsAddr.h"
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
	
	// shold we evaluate Enter press as adding the breakpoint or closing the combo?
	bool skipClosePopupByEnterPressInThisFrame = false;

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		ImGui::OpenPopup("addAddrBreakpointPopup");
		
		addBreakpointPopupAddr = 0;
		addBreakpointPopupSymbol[0] = '\0';
		comboFilterTextBuf[0] = 0;
		
		imGuiOpenPopupFrame = 0;
		skipClosePopupByEnterPressInThisFrame = true;
	}
	
	if (ImGui::BeginPopup("addAddrBreakpointPopup"))
	{
		ImGui::Text(addBreakpointPopupHeadlineStr);
//		ImGui::SameLine();
//		ImGui::Text("Address: %04x", )
		ImGui::Separator();
		
		if (imGuiOpenPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenPopupFrame++;
		}

		sprintf(buf, "##addPCBreakpointPopupAddress_%s", symbols->dataAdapter->adapterID);

		bool buttonAddClicked = false;
		if (symbols->currentSegment)
		{
			const char **hints = symbols->currentSegment->codeLabelsArray;
			int numHints = symbols->currentSegment->numCodeLabelsInArray;
			
			// https://github.com/ocornut/imgui/issues/1658   | IM_ARRAYSIZE(hints)
			if( ComboFilter(buf, comboFilterTextBuf, IM_ARRAYSIZE(comboFilterTextBuf), hints, numHints, comboFilterState, this) )
			{
				LOGD("SELECTED %d comboTextBuf='%s'", comboFilterState.activeIdx, comboFilterTextBuf);
				skipClosePopupByEnterPressInThisFrame = false; //true;
				
				// if that is not address then replace by selected symbol
				if (FUN_IsHexNumber(comboFilterTextBuf))
				{
					FUN_ToUpperCaseStr(comboFilterTextBuf);
					addBreakpointPopupAddr = FUN_HexStrToValue(comboFilterTextBuf);
				}
				else if (hints != NULL && numHints > 0 && comboFilterState.activeIdx < numHints)
				{
					strcpy(comboFilterTextBuf, hints[comboFilterState.activeIdx]);
				}
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Add"))
			{
				buttonAddClicked = true;
			}
		}
		
		bool finalizeAddingBreakpoint = buttonAddClicked
			|| (!skipClosePopupByEnterPressInThisFrame && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
		
//		if (ImGui::Button("Create PC Breakpoint"))
//		{
//			finalizeAddingBreakpoint = true;
//		}
		
		if (finalizeAddingBreakpoint)
		{
			CDebugSymbolsCodeLabel *label = symbols->currentSegment->FindLabelByText(comboFilterTextBuf);
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
				char *buf = SYS_GetCharBuf();
				sprintf(buf, "Invalid address or symbol:\n%s", comboFilterTextBuf);
				guiMain->ShowMessageBox("Can't add breakpoint", buf);
				SYS_ReleaseCharBuf(buf);
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
						
		////
		
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

