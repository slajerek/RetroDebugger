#include "SYS_Defs.h"
#include "DBG_Log.h"
#include "CViewDataWatch.h"
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

	// address | label | representation | value
	if (ImGui::BeginTable(buf, 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders))
	{
		ImGui::TableNextColumn();
		ImGui::Text("Address");
		ImGui::TableNextColumn();
		ImGui::Text("Label");
		ImGui::TableNextColumn();
		ImGui::Text("Value");
		ImGui::TableNextColumn();
		ImGui::Text("Format");
		ImGui::TableNextColumn();
		ImGui::Text("");

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
				u32 value = 0;
				switch(watch->representation)
				{
					default:
					case WATCH_REPRESENTATION_BIN:
					case WATCH_REPRESENTATION_HEX_8:
						value = dataAdapter->AdapterReadByteModulus(watch->address);
						break;
					case WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    )
								| dataAdapter->AdapterReadByteModulus(watch->address + 1) << 8;
						break;
					case WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    ) << 8
								| dataAdapter->AdapterReadByteModulus(watch->address + 1);
						break;

					case WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    )
								| dataAdapter->AdapterReadByteModulus(watch->address + 1) << 8
								| dataAdapter->AdapterReadByteModulus(watch->address + 2) << 16
								| dataAdapter->AdapterReadByteModulus(watch->address + 3) << 24;
						break;
					case WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN:
					case WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN:
						value =   dataAdapter->AdapterReadByteModulus(watch->address    ) << 24
								| dataAdapter->AdapterReadByteModulus(watch->address + 1) << 16
								| dataAdapter->AdapterReadByteModulus(watch->address + 2) << 8
								| dataAdapter->AdapterReadByteModulus(watch->address + 3);
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
			}

			ImGui::TableNextColumn();
			
			sprintf(buf, "##DataWatchCombo%x", watch);
			ImGui::Combo(buf, &(watch->representation), "Hex 8-bits\0Hex 16-bits LE\0Hex 16-bits BE\0Hex 32-bits LE\0Hex 32-bits BE\0Unsigned Dec 8-bits\0Unsigned Dec 16-bits LE\0Unsigned Dec 16-bits BE\0Unsigned Dec 32-bits LE\0Unsigned Dec 32-bits BE\0Signed Dec 8-bits\0Signed Dec 16-bits LE\0Signed Dec 16-bits BE\0Signed Dec 32-bits LE\0Signed Dec 32-bits BE\0Binary\0\0"); //Text\0\0");

			ImGui::TableNextColumn();
			sprintf(buf, "X##%x", watch);
			if (ImGui::Button(buf))
			{
				watchToDelete = watch;
			}
		}
		
		ImGui::EndTable();
	}

	if (watchToDelete)
	{
		symbols->currentSegment->DeleteWatch(watchToDelete->address);
	}

	// shold we evaluate Enter press as adding the watch or closing the combo?
	bool skipClosePopupByEnterPressInThisFrame = false;

	if (ImGui::Button("+") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
	{
		ImGui::OpenPopup("addWatchPopup");
		
		addWatchPopupAddr = 0;
		addWatchPopupSymbol[0] = '\0';
		comboFilterTextBuf[0] = 0;
		
		imGuiOpenPopupFrame = 0;
		skipClosePopupByEnterPressInThisFrame = true;
	}
	
	if (ImGui::BeginPopup("addWatchPopup"))
	{
		ImGui::Text("Add watch point");
		ImGui::Separator();
		
		if (imGuiOpenPopupFrame < 2)
		{
			ImGui::SetKeyboardFocusHere();
			imGuiOpenPopupFrame++;
		}

		sprintf(buf, "##addWatchPopupAddress%x", this);

		bool buttonAddClicked = false;
		if (symbols->currentSegment)
		{
			const char **hints = symbols->currentSegment->codeLabelsArray;
			int numHints = symbols->currentSegment->numCodeLabelsInArray;
			
			// https://github.com/ocornut/imgui/issues/1658   | IM_ARRAYSIZE(hints)
			if( ComboFilter(buf, comboFilterTextBuf, IM_ARRAYSIZE(comboFilterTextBuf), hints, numHints, comboFilterState, this) )
			{
				LOGD("SELECTED %d comboTextBuf='%s'", comboFilterState.activeIdx, comboFilterTextBuf);
				skipClosePopupByEnterPressInThisFrame = false;	/// true
				
				// if that is not address then replace by selected symbol
				if (FUN_IsHexNumber(comboFilterTextBuf))
				{
					FUN_ToUpperCaseStr(comboFilterTextBuf);
					addWatchPopupAddr = FUN_HexStrToValue(comboFilterTextBuf);
				}
				else if (hints != NULL && numHints > 0)
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
		
		/*
		ImGui::InputScalar(addBreakpointPopupAddrStr, ImGuiDataType_::ImGuiDataType_U32, &addBreakpointPopupAddr, NULL, NULL, addressFormatStr, addBreakpointPopupAddrInputFlags);
		
		if (addBreakpointPopupShowSymbolField)
		{
			sprintf(buf, "Symbol##%x", this);
			if (ImGui::InputText(buf, addBreakpointPopupSymbol, 255, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				LOGTODO("search for symbol......");
				// search for symbol
				
				ImGui::CloseCurrentPopup();
			}
		}
		 */
		
		bool finalizeAddingBreakpoint = buttonAddClicked
			|| (!skipClosePopupByEnterPressInThisFrame && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter));
		
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
