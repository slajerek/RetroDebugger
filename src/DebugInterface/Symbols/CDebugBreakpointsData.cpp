#include "CDebugBreakpointsData.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "GUI_Main.h"

#include "CDebugBreakpointEventCallback.h"
#include "CDebuggerServer.h"
#include "CViewC64.h"

const char *comparisonMethodsStr[MEMORY_BREAKPOINT_ARRAY_SIZE] = { "==", "!=", "<", "<=", ">", ">=" };

CDebugBreakpointData *CDebugBreakpointsData::EvaluateBreakpoint(int addr, int value, u32 memoryAccess)
{
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.find(addr);
	if (it != breakpoints.end())
	{
		CDebugBreakpointData *dataBreakpoint = (CDebugBreakpointData *)it->second;
		if (dataBreakpoint->isActive == false)
			return NULL;

		// check memory access (read/write)
		if (!IS_SET(dataBreakpoint->dataAccess, memoryAccess))
		{
			return NULL;
		}
		
		CDebugBreakpointData *evaluateBreakpoint = NULL;
		if (dataBreakpoint->comparison == DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL)
		{
			if (value == dataBreakpoint->value)
			{
				evaluateBreakpoint = dataBreakpoint;
			}
		}
		else if (dataBreakpoint->comparison == DataBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL)
		{
			if (value != dataBreakpoint->value)
			{
				evaluateBreakpoint = dataBreakpoint;
			}
		}
		else if (dataBreakpoint->comparison == DataBreakpointComparison::MEMORY_BREAKPOINT_LESS)
		{
			if (value < dataBreakpoint->value)
			{
				evaluateBreakpoint = dataBreakpoint;
			}
		}
		else if (dataBreakpoint->comparison == DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL)
		{
			if (value <= dataBreakpoint->value)
			{
				evaluateBreakpoint = dataBreakpoint;
			}
		}
		else if (dataBreakpoint->comparison == DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER)
		{
			if (value > dataBreakpoint->value)
			{
				evaluateBreakpoint = dataBreakpoint;
			}
		}
		else if (dataBreakpoint->comparison == DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL)
		{
			if (value >= dataBreakpoint->value)
			{
				evaluateBreakpoint = dataBreakpoint;
			}
		}
		
		if (evaluateBreakpoint && evaluateBreakpoint->callback)
		{
			if (evaluateBreakpoint->callback->DebugBreakpointEvaluateCallback(dataBreakpoint) == false)
				return NULL;
		}
		
		// flag breakpoint to Server API
		if (viewC64->debuggerServer)
		{
			if (evaluateBreakpoint && viewC64->debuggerServer->AreClientsConnected())
			{
				nlohmann::json j;
				evaluateBreakpoint->GetDetailsJson(j);
				j["addr"] = addr;
				j["value"] = value;
				if (memoryAccess == MEMORY_BREAKPOINT_ACCESS_WRITE)
				{
					j["access"] = "write";
				}
				else if (memoryAccess == MEMORY_BREAKPOINT_ACCESS_READ)
				{
					j["access"] = "read";
				}
				viewC64->debuggerServer->BroadcastEvent("breakpoint", j);
			}
		}
		
		return evaluateBreakpoint;
	}
	
	return NULL;
}

void CDebugBreakpointsData::RenderImGui()
{
//	LOGD("CDebugBreakpointsData::RenderImGui");
	
	char *buf = SYS_GetCharBuf();
	
	ImVec4 colorNotActive(0.5, 0.5, 0.5, 1);
	ImVec4 colorActive(1.0, 1.0, 1.0, 1);

	sprintf(buf, "##BreakpointsDataTable_%s", symbols->dataAdapter->adapterID);

	symbols->LockMutex();

	CDebugBreakpointAddr *deleteBreakpoint = NULL;

	// active | address | <= | FF | symbol | delete
	if (ImGui::BeginTable(buf, 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders))	//| ImGuiTableFlags_Sortable  TODO
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

		for (std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints.begin(); it != breakpoints.end(); it++)
		{
			CDebugBreakpointData *bp = (CDebugBreakpointData*)it->second;
					
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
			const char *comparisonStr = DataBreakpointComparisonToStr(bp->comparison);
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

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		ImGui::OpenPopup("addMemBreakpointPopup");
		
		addBreakpointPopupAddr = 0;
		addBreakpointPopupSymbol[0] = '\0';
		addBreakpointPopupValue = 0xFF;
		addBreakpointPopupComparisonMethod = DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL;
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
				else if (hints != NULL && numHints > 0 && comboFilterState.activeIdx < numHints)
				{
					strcpy(comboFilterTextBuf, hints[comboFilterState.activeIdx]);
				}
			}
		}
				
		ImGui::SameLine();
		
		//
		const char *selectedComparisonStr = DataBreakpointComparisonToStr((DataBreakpointComparison)addBreakpointPopupComparisonMethod);
		
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
		
		bool finalizeAddingBreakpoint = !skipClosePopupByEnterPressInThisFrame && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter);
		
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

				CDebugBreakpointData *breakpoint = new CDebugBreakpointData(addBreakpointPopupAddr, MEMORY_BREAKPOINT_ACCESS_WRITE,
																	  (DataBreakpointComparison)addBreakpointPopupComparisonMethod,
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

const char *CDebugBreakpointsData::DataBreakpointComparisonToStr(DataBreakpointComparison comparison)
{
	switch(comparison)
	{
		case DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL:
			return "==";
		case DataBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL:
			return "!=";
		case DataBreakpointComparison::MEMORY_BREAKPOINT_LESS:
			return "<";
		case DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL:
			return "<=";
		case DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER:
			return ">";
		case DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL:
			return ">=";
		default:
			return "???";
	}
}

DataBreakpointComparison CDebugBreakpointsData::StrToDataBreakpointComparison(const char *comparisonStr)
{
	for (int i = 0; i < MEMORY_BREAKPOINT_ARRAY_SIZE; i++)
	{
		if (!strcmp(comparisonStr, comparisonMethodsStr[i]))
		{
			return DataBreakpointComparison(i);
		}
	}
	
	return (DataBreakpointComparison)MEMORY_BREAKPOINT_ARRAY_SIZE;
}


CDebugBreakpointsData::CDebugBreakpointsData(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, const char *addressFormatStr, int minAddr, int maxAddr)
: CDebugBreakpointsAddr(breakpointType, breakpointTypeStr, segment, addressFormatStr, minAddr, maxAddr)
{
	addBreakpointPopupHeadlineStr = "Add Memory Breakpoint";
	addBreakpointPopupAddrStr = "Address";
}

CDebugBreakpoint *CDebugBreakpointsData::CreateEmptyBreakpoint()
{
	return new CDebugBreakpointData(symbols, 0, 0, DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL, 0);
}

CDebugBreakpointsData::~CDebugBreakpointsData()
{
	ClearBreakpoints();
}
		
