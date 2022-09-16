#include "CDebugBreakpoints.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "GUI_Main.h"

CDebugBreakpoint::CDebugBreakpoint()
{
}

CDebugBreakpoint::~CDebugBreakpoint()
{
}

void CDebugBreakpoint::Serialize(Hjson::Value hjsonRoot)
{
}

void CDebugBreakpoint::Deserialize(Hjson::Value hjsonRoot)
{
}

CBreakpointAddr::CBreakpointAddr(int addr)
{
	this->isActive = true;
	this->addr = addr;
	this->actions = ADDR_BREAKPOINT_ACTION_STOP;
	this->data = 0x00;
}

void CBreakpointAddr::Serialize(Hjson::Value hjsonRoot)
{
	hjsonRoot["IsActive"] = isActive;
	hjsonRoot["Addr"] = addr;
	hjsonRoot["Actions"] = actions;
	hjsonRoot["Data"] = data;
}

void CBreakpointAddr::Deserialize(Hjson::Value hjsonRoot)
{
	isActive = (bool)hjsonRoot["IsActive"];
	addr = hjsonRoot["Addr"];
	actions = hjsonRoot["Actions"];
	data = hjsonRoot["Data"];
}

CBreakpointAddr::~CBreakpointAddr()
{
}

CBreakpointMemory::CBreakpointMemory(int addr,
									u32 memoryAccess, MemoryBreakpointComparison comparison, int value)
: CBreakpointAddr(addr)
{
	this->memoryAccess = memoryAccess;
	this->value = value;
	this->comparison = comparison;
}

void CBreakpointMemory::Serialize(Hjson::Value hjsonRoot)
{
	CBreakpointAddr::Serialize(hjsonRoot);
	
	hjsonRoot["MemoryAccess"] = memoryAccess;
	hjsonRoot["Value"] = value;
	hjsonRoot["Comparison"] = (int)comparison;
}

void CBreakpointMemory::Deserialize(Hjson::Value hjsonRoot)
{
	CBreakpointAddr::Deserialize(hjsonRoot);
	
	memoryAccess = hjsonRoot["MemoryAccess"];
	value = hjsonRoot["Value"];
	comparison = (MemoryBreakpointComparison) ((int)hjsonRoot["Comparison"]);
}

//std::map<int, CBreakpointMemory *> memoryBreakpoints;

CDebugBreakpointsAddr::CDebugBreakpointsAddr(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, char *addressFormatStr, int minAddr, int maxAddr)
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

	addBreakpointPopupHeadlineStr = "Add PC Breakpoint";
	addBreakpointPopupAddrStr = "Address";
	addBreakpointPopupAddrInputFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;
	
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
		std::map<int, CBreakpointAddr *>::iterator it = breakpoints.begin();
		CBreakpointAddr *breakpoint = it->second;
		
		breakpoints.erase(it);
		delete breakpoint;
	}
}

CDebugBreakpoint *CDebugBreakpointsAddr::CreateEmptyBreakpoint()
{
	return new CBreakpointAddr(0);
}

void CDebugBreakpointsAddr::Serialize(Hjson::Value hjsonBreakpoints)
{
	for (std::map<int, CBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
	{
		CBreakpointAddr *breakpoint = it->second;
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
		CBreakpointAddr *breakpoint = (CBreakpointAddr*)CreateEmptyBreakpoint();
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

CDebugBreakpointsMemory::CDebugBreakpointsMemory(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, char *addressFormatStr, int minAddr, int maxAddr)
: CDebugBreakpointsAddr(breakpointType, breakpointTypeStr, segment, addressFormatStr, minAddr, maxAddr)
{
	addBreakpointPopupHeadlineStr = "Add Memory Breakpoint";
	addBreakpointPopupAddrStr = "Address";
}

CDebugBreakpoint *CDebugBreakpointsMemory::CreateEmptyBreakpoint()
{
	return new CBreakpointMemory(0, 0, MemoryBreakpointComparison::MEMORY_BREAKPOINT_EQUAL, 0);
}

void CDebugBreakpointsAddr::AddBreakpoint(CBreakpointAddr *breakpoint)
{
	// check if breakpoint is already in the map and remove it, can be with other addr so we can't use find
	for (std::map<int, CBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
	{
		CBreakpointAddr *existingBreakpoint = it->second;
		
		if (existingBreakpoint == breakpoint)
		{
			breakpoints.erase(it);
			break;
		}
	}
	
	// check if there's a breakpoint with the same address and delete it (we are replacing it)
	std::map<int, CBreakpointAddr *>::iterator it = breakpoints.find(breakpoint->addr);
	if (it != breakpoints.end())
	{
		CBreakpointAddr *existingBreakpoint = it->second;
		breakpoints.erase(it);
		delete existingBreakpoint;
	}
	
	// add a breakpoint
	breakpoints[breakpoint->addr] = breakpoint;
}

CBreakpointAddr *CDebugBreakpointsAddr::GetBreakpoint(int addr)
{
	std::map<int, CBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it == breakpoints.end())
		return NULL;
	
	return it->second;
}

void CDebugBreakpointsAddr::DeleteBreakpoint(int addr)
{
	std::map<int, CBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it != breakpoints.end())
	{
		CBreakpointAddr *breakpoint = it->second;
		breakpoints.erase(it);
		delete breakpoint;
	}
}

void CDebugBreakpointsAddr::DeleteBreakpoint(CBreakpointAddr *breakpoint)
{
	this->DeleteBreakpoint(breakpoint->addr);
}

void CDebugBreakpointsAddr::RemoveBreakpoint(CBreakpointAddr *breakpoint)
{
	std::map<int, CBreakpointAddr *>::iterator it = breakpoints.find(breakpoint->addr);
	if (it != breakpoints.end())
	{
		breakpoints.erase(it);
	}
	
	UpdateRenderBreakpoints();
}


// TODO: create a condition parser (tree for condition) and parse the condition text
CBreakpointAddr *CDebugBreakpointsAddr::EvaluateBreakpoint(int addr)
{
	std::map<int, CBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it != breakpoints.end())
	{
		CBreakpointAddr *breakpoint = it->second;
		if (breakpoint->isActive == false)
			return NULL;
		
		return breakpoint;
	}
	
	return NULL;
}

CBreakpointMemory *CDebugBreakpointsMemory::EvaluateBreakpoint(int addr, int value, u32 memoryAccess)
{
	std::map<int, CBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it != breakpoints.end())
	{
		CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory *)it->second;
		if (memoryBreakpoint->isActive == false)
			return NULL;

		// check memory access (read/write)
		if (!IS_SET(memoryBreakpoint->memoryAccess, memoryAccess))
		{
			return NULL;
		}
		
		if (memoryBreakpoint->comparison == MemoryBreakpointComparison::MEMORY_BREAKPOINT_EQUAL)
		{
			if (value == memoryBreakpoint->value)
			{
				return memoryBreakpoint;
			}
		}
		else if (memoryBreakpoint->comparison == MemoryBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL)
		{
			if (value != memoryBreakpoint->value)
			{
				return memoryBreakpoint;
			}
		}
		else if (memoryBreakpoint->comparison == MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS)
		{
			if (value < memoryBreakpoint->value)
			{
				return memoryBreakpoint;
			}
		}
		else if (memoryBreakpoint->comparison == MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL)
		{
			if (value <= memoryBreakpoint->value)
			{
				return memoryBreakpoint;
			}
		}
		else if (memoryBreakpoint->comparison == MemoryBreakpointComparison::MEMORY_BREAKPOINT_GREATER)
		{
			if (value > memoryBreakpoint->value)
			{
				return memoryBreakpoint;
			}
		}
		else if (memoryBreakpoint->comparison == MemoryBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL)
		{
			if (value >= memoryBreakpoint->value)
			{
				return memoryBreakpoint;
			}
		}
	}
	
	return NULL;
}

CDebugBreakpointsAddr::~CDebugBreakpointsAddr()
{
	ClearBreakpoints();
}

CDebugBreakpointsMemory::~CDebugBreakpointsMemory()
{
	ClearBreakpoints();
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

	CBreakpointAddr *deleteBreakpoint = NULL;

	sprintf(buf, "##BreakpointsAddrTable_%s", symbols->dataAdapter->adapterID);

	// active | address | symbol | delete
	if (ImGui::BeginTable(buf, 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders))
	{
		u32 i = 0;
		
//		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.65f)));

		ImGui::TableNextColumn();
		ImGui::Text("Active");
		ImGui::TableNextColumn();
		ImGui::Text("Address");
		ImGui::TableNextColumn();
		ImGui::Text("Label");
		ImGui::TableNextColumn();
		ImGui::Text("");

//	   	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0);

		for (std::map<int, CBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
		{
			CBreakpointAddr *bp = it->second;
					
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

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))))
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
				else if (hints != NULL)
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
			|| (!skipClosePopupByEnterPressInThisFrame && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)));
		
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

				CBreakpointAddr *breakpoint = new CBreakpointAddr(addBreakpointPopupAddr);
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
	for (std::map<int, CBreakpointAddr *>::iterator it = breakpoints.begin();
		 it != breakpoints.end(); it++)
	{
		CBreakpointAddr *breakpoint = it->second;
		renderBreakpoints[breakpoint->addr] = breakpoint;
	}

	symbols->UnlockMutex();
	this->renderBreakpointsMutex->Unlock();
}

void CDebugBreakpointsMemory::RenderImGui()
{
//	LOGD("CDebugBreakpointsMemory::RenderImGui");
	
	char *buf = SYS_GetCharBuf();
	
	ImVec4 colorNotActive(0.5, 0.5, 0.5, 1);
	ImVec4 colorActive(1.0, 1.0, 1.0, 1);

	sprintf(buf, "##BreakpointsMemoryTable_%s", symbols->dataAdapter->adapterID);

	symbols->LockMutex();

	CBreakpointAddr *deleteBreakpoint = NULL;

	// active | address | <= | FF | symbol | delete
	if (ImGui::BeginTable(buf, 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders))
	{
		u32 i = 0;
		
//		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.65f)));

		ImGui::TableNextColumn();
		ImGui::Text("Active");
		ImGui::TableNextColumn();
		ImGui::Text("Address");
		ImGui::TableNextColumn();
		ImGui::Text("Comparison");
		ImGui::TableNextColumn();
		ImGui::Text("Value");
		ImGui::TableNextColumn();
		ImGui::Text("Label");
		ImGui::TableNextColumn();
		ImGui::Text("");

//	   	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0);

		for (std::map<int, CBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
		{
			CBreakpointMemory *bp = (CBreakpointMemory*)it->second;
					
			sprintf(buf, "##chkBoxMem%x%d", this, i);
			
			ImGui::TableNextColumn();
			if (ImGui::Checkbox(buf, &bp->isActive))
			{
				// we do not need to update anything
			}

			ImGui::TableNextColumn();
			ImVec4 color = bp->isActive ? colorActive : colorNotActive;
			ImGui::TextColored(color, addressFormatStr, bp->addr);

			ImGui::TableNextColumn();
			const char *comparisonStr = MemoryComparisonToStr(bp->comparison);
			ImGui::TextColored(color, comparisonStr, bp->addr);

			ImGui::TableNextColumn();
			ImGui::TextColored(color, "%02X", bp->value);
			
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
	
	// shold we evaluate Enter press as adding the breakpoint or closing the combo?
	bool skipClosePopupByEnterPressInThisFrame = false;

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))))
	{
		ImGui::OpenPopup("addMemBreakpointPopup");
		
		addBreakpointPopupAddr = 0;
		addBreakpointPopupSymbol[0] = '\0';
		addBreakpointPopupValue = 0xFF;
		addBreakpointPopupComparisonMethod = MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL;
		comboFilterTextBuf[0] = 0; //memcpy(buf, hints[0], strlen(hints[0]) + 1);

		imGuiOpenPopupFrame = 0;
		skipClosePopupByEnterPressInThisFrame = true;
	}
	
	if (ImGui::BeginPopup("addMemBreakpointPopup"))
	{
		ImGui::Text(addBreakpointPopupHeadlineStr);
		ImGui::Separator();
		
		if (imGuiOpenPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenPopupFrame++;
		}
		
		sprintf(buf, "##addMemBreakpointPopupAddress_%s", symbols->dataAdapter->adapterID);

		if (symbols->currentSegment)
		{
			const char **hints = symbols->currentSegment->codeLabelsArray;
			int numHints = symbols->currentSegment->numCodeLabelsInArray;
						
			// https://github.com/ocornut/imgui/issues/1658   | IM_ARRAYSIZE(hints)
			if( ComboFilter(buf, comboFilterTextBuf, IM_ARRAYSIZE(comboFilterTextBuf), hints, numHints, comboFilterState, this) )
			{
				LOGD("SELECTED %d comboTextBuf='%s'", comboFilterState.activeIdx, comboFilterTextBuf);
				skipClosePopupByEnterPressInThisFrame = true;

				// TODO: fix this logic, we need to first check if label exists, if yes then use label first even if label is some hexcode string
				
				// if that is not address then replace by selected symbol
				if (FUN_IsHexNumber(comboFilterTextBuf))
				{
					FUN_ToUpperCaseStr(comboFilterTextBuf);
					addBreakpointPopupAddr = FUN_HexStrToValue(comboFilterTextBuf);
				}
				else if (hints != NULL)
				{
					strcpy(comboFilterTextBuf, hints[comboFilterState.activeIdx]);
				}
			}
		}
				
		ImGui::SameLine();
		
		//
		const char *selectedComparisonStr = MemoryComparisonToStr((MemoryBreakpointComparison)addBreakpointPopupComparisonMethod);
		
		sprintf(buf, "##addMemBreakpointPopupComboComparison%x", this);
		if (ImGui::BeginCombo(buf, selectedComparisonStr))
		{
			skipClosePopupByEnterPressInThisFrame = true;
			for (int n = 0; n < MEMORY_BREAKPOINT_ARRAY_SIZE; n++)
			{
				bool is_selected = (addBreakpointPopupComparisonMethod == n);
				if (ImGui::Selectable(comparisonMethodsStr[n], is_selected))
				{
					addBreakpointPopupComparisonMethod = n;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}		
		
		ImGui::SameLine();
		
		sprintf(buf, "##addMemBreakpointPopupValue%x", this);
		ImGui::InputScalar(buf, ImGuiDataType_::ImGuiDataType_U8, &addBreakpointPopupValue, NULL, NULL, "%02X",
						   ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
		
//		ImGui::SameLine();
		
		bool finalizeAddingBreakpoint = !skipClosePopupByEnterPressInThisFrame && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter));
		
		if (ImGui::Button("Create Breakpoint"))
		{
			finalizeAddingBreakpoint = true;
		}
		
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
				addBreakpointPopupValue = URANGE(0, addBreakpointPopupValue, 0xFF);

				CBreakpointMemory *breakpoint = new CBreakpointMemory(addBreakpointPopupAddr, MEMORY_BREAKPOINT_ACCESS_WRITE,
																	  (MemoryBreakpointComparison)addBreakpointPopupComparisonMethod,
																	  addBreakpointPopupValue);
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

const char *CDebugBreakpointsMemory::MemoryComparisonToStr(MemoryBreakpointComparison comparison)
{
	switch(comparison)
	{
		case MemoryBreakpointComparison::MEMORY_BREAKPOINT_EQUAL:
			return "==";
		case MemoryBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL:
			return "!=";
		case MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS:
			return "<";
		case MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL:
			return "<=";
		case MemoryBreakpointComparison::MEMORY_BREAKPOINT_GREATER:
			return ">";
		case MemoryBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL:
			return ">=";
		default:
			return "???";
	}
}
