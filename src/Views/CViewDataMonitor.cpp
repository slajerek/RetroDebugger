#include "CViewDataMonitor.h"
#include "CViewC64.h"
#include "CConfigStorageHjson.h"
#include "CDataAdapter.h"
#include "CDebugMemory.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbolsSegment.h"
#include "CViewDataMap.h"
#include "CDebugMemoryCell.h"
#include "CViewDisassembly.h"
#include "CViewDataDump.h"
#include "CSnapshotsManager.h"

using namespace ImGui;

// NOTE: dataAdapter is provided to distinguish symbols->dataAdapter, that may include ROM to disassemble code
//       with a RAM-only data adapter for data monitoring used here (i.e. dataAdapter provided in constructor should point to RAM)
CViewDataMonitor::CViewDataMonitor(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								   CDebugSymbols *symbols, CDataAdapter *dataAdapter, CViewDataMap *viewMemoryMap, CViewDisassembly *viewDisassembly)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->symbols = symbols;
	this->debugInterface = symbols->debugInterface;
	this->dataAdapter = dataAdapter;
	this->viewMemoryMap = viewMemoryMap;
	this->viewDisassembly = viewDisassembly;
	
	this->dataAdapterCode = symbols->dataAdapter;
	
	currentDataCapture = NULL;
	dataLen = dataAdapter->AdapterGetDataLength();
	data = new u8[dataLen];

	// this holds addresses for clipper, at max may be whole dataLen (usually should not happen)
	clipperAddresses = new int[dataLen];

	viewC64->config->GetBool("CViewDataMonitorFilterOnlyWhenChanged", &filterShowOnlyChanged, true);
	viewC64->config->GetBool("CViewDataMonitorFilterOnlyWhenMatchesPrevValue", &filterShowOnlyWhenMatchesPrevValue, false);
	viewC64->config->GetInt("CViewDataMonitorFilterPrevValue", &filterPrevValue, 0);
	viewC64->config->GetBool("CViewDataMonitorFilterOnlyWhenMatchesCurrentValue", &filterShowOnlyWhenMatchesCurrentValue, false);
	viewC64->config->GetInt("CViewDataMonitorFilterCurrentValue", &filterCurrentValue, 0);
}

CViewDataMonitor::~CViewDataMonitor()
{
	delete [] data;
	delete [] clipperAddresses;
}

//dropdown cycles/memory states [capture] [delete] | [open memdump in new window] [open map in new window] [open disassembly in new window] -> +MARK CHANGES THERE
//list of changes
//ADDR | label | prev value | current value | code addr that changed | code label that changed | code that changed INC $02 | frame when changed | cycle when changed | go to addr | go to who changed | rewind to change

void CViewDataMonitor::RenderImGui()
{
	PreRenderImGui();
	
	if (dataCaptures.empty())
	{
		PushItemFlag(ImGuiItemFlags_Disabled, true);
		PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

		if (BeginCombo("##CurrentDataCapture", ""))
		{
			EndCombo();
		}

		PopItemFlag();
		PopStyleVar();
	}
	else
	{
		if (BeginCombo("##CurrentDataCapture", currentDataCapture ? currentDataCapture->name : ""))
		{
			for (std::map<u64, CDataCaptureState *>::iterator it = dataCaptures.begin(); it != dataCaptures.end(); it++)
			{
				CDataCaptureState *dataCapture = it->second;
				
				bool isSelected = (dataCapture == currentDataCapture);
				if (Selectable(dataCapture->name, isSelected))
				{
					currentDataCapture = dataCapture;
				}
				
				if (isSelected)
					SetItemDefaultFocus();
			}
			EndCombo();
		}
	}
	
	SameLine();
	if (Button("Capture"))
	{
		debugInterface->LockMutex();
		
		int dataLen = dataAdapter->AdapterGetDataLength();
		if (dataLen != this->dataLen)
		{
			LOGError("CViewDataMonitor::RenderImGui: dataLen from adapter=%d != prev %d", dataLen, this->dataLen);
			Text("Error: dataLen from adapter=%d != prev %d", dataLen, this->dataLen);
			PostRenderImGui();
			return;
		}

		u8 *data = new u8[dataLen];
		dataAdapter->AdapterReadBlockDirect(data, 0, dataLen);
		
		u64 cycle = debugInterface->GetMainCpuCycleCounter();
		u32 frame = debugInterface->GetEmulationFrameNumber();
		
		CDataCaptureState *dataCapture = new CDataCaptureState(cycle, frame, data);
		dataCaptures[cycle] = dataCapture;
		currentDataCapture = dataCapture;
		
		debugInterface->UnlockMutex();
	}

	ImGui::SameLine();
	bool isDisabledDelete = (currentDataCapture == NULL);
	if (isDisabledDelete)
	{
		PushItemFlag(ImGuiItemFlags_Disabled, true);
		PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if (Button("Delete"))
	{
		const auto it = dataCaptures.find(currentDataCapture->cycle);
		if (it != dataCaptures.end())
			dataCaptures.erase(it);
		
		delete currentDataCapture;
		currentDataCapture = NULL;
	}
	if (isDisabledDelete)
	{
		PopItemFlag();
		PopStyleVar();
	}
	
	Text("Filter: ");
	SameLine();
	if (Checkbox("Changed", &filterShowOnlyChanged))
	{
		viewC64->config->SetBool("CViewDataMonitorFilterOnlyWhenChanged", &filterShowOnlyChanged);
	}

	SameLine();
	if (Checkbox("Matches prev", &filterShowOnlyWhenMatchesPrevValue))
	{
		viewC64->config->SetBool("CViewDataMonitorFilterOnlyWhenMatchesPrevValue", &filterShowOnlyWhenMatchesPrevValue);
	}

	SameLine();
	PushItemWidth(ImGui::GetFontSize() * 2.0);
	InputScalar("##DataMonitorPrevValue", ImGuiDataType_::ImGuiDataType_U8, &filterPrevValue, NULL, NULL, "%02X", defaultHexInputFlags);
	if (IsItemDeactivatedAfterEdit())
	{
		viewC64->config->SetInt("CViewDataMonitorFilterPrevValue", &filterPrevValue);
		filterShowOnlyWhenMatchesPrevValue = true;
	}
	PopItemWidth();

	SameLine();
	if (Checkbox("Matches current", &filterShowOnlyWhenMatchesCurrentValue))
	{
		viewC64->config->SetBool("CViewDataMonitorFilterOnlyWhenMatchesCurrentValue", &filterShowOnlyWhenMatchesCurrentValue);
	}

	SameLine();
	PushItemWidth(ImGui::GetFontSize() * 2.0);
	InputScalar("##DataMonitorCurrentValue", ImGuiDataType_::ImGuiDataType_U8, &filterCurrentValue, NULL, NULL, "%02X", defaultHexInputFlags);
	if (IsItemDeactivatedAfterEdit())
	{
		viewC64->config->SetInt("CViewDataMonitorFilterCurrentValue", &filterCurrentValue);
		filterShowOnlyWhenMatchesCurrentValue = true;
	}
	PopItemWidth();
	
	// render list of changes
	if (currentDataCapture)
	{
		// read whole data state (e.g. whole emulator memory)
		dataAdapter->AdapterReadBlockDirect(data, 0, dataLen);

		//ADDR | label | prev value | current value | code addr that changed | code label that changed | disass code that changed INC $02 | frame when changed | cycle when changed | go to addr | go to who changed | rewind to change
		
		ImU32 changedRowBgColor = ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.65f));

		if (BeginTable("##ChangesInCurrentDataCapture", 12, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable))
		{
			TableSetupColumn("Address");
			TableSetupColumn("Label");
			TableSetupColumn("Captured");
			TableSetupColumn("Current");
			TableSetupColumn("Code Addr");
			TableSetupColumn("Code Label");
			TableSetupColumn("Code");
			TableSetupColumn("Frame");
			TableSetupColumn("Cycle");
			TableSetupColumn("##GoToAddr");
			TableSetupColumn("##GoToWhoChanged");
			TableSetupColumn("##RewindToChange");
			TableHeadersRow();
			
			// prepare data for clipper
			int row = 0;
			for (int addr = 0; addr < dataLen; addr++)
			{
				bool shouldShowLine = true;
				
				if (filterShowOnlyWhenMatchesCurrentValue
					&& data[addr] != filterCurrentValue)
				{
					shouldShowLine = false;
				}
				
				if (filterShowOnlyWhenMatchesPrevValue
					&& currentDataCapture->dataCopy[addr] != filterPrevValue)
				{
					shouldShowLine = false;
				}
				
				if (filterShowOnlyChanged
					&& data[addr] == currentDataCapture->dataCopy[addr])
				{
					shouldShowLine = false;
				}
				
				// check if there's a change
				if (shouldShowLine)
				{
					clipperAddresses[row] = addr;
					row++;
				}
			}

			// number of rows is now in 'row', let's init clipper
			ImGuiListClipper clipper;
			clipper.Begin(row);
			while (clipper.Step())
			{
				for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
				{
					int addr = clipperAddresses[row_n];

					TableNextRow();

					if (!filterShowOnlyChanged
						&& data[addr] != currentDataCapture->dataCopy[addr])
					{
						// when changed mode is off let's make changed rows red
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, changedRowBgColor);
					}
					
					// addr
					TableNextColumn();
					Text("%04X", addr);
					
					// label
					TableNextColumn();
					buf[0] = 0;
					if (symbols->currentSegment)
					{
						symbols->currentSegment->FindNearLabelText(addr, buf);
					}
					Text(buf);
					
					// prev value
					TableNextColumn();
					Text("%02X", currentDataCapture->dataCopy[addr]);
					
					// current value
					TableNextColumn();
					Text("%02X", data[addr]);

					// code addr
					CDebugMemoryCell *cell = symbols->memory->GetMemoryCell(addr);

					TableNextColumn();
					if (cell->writePC >= 0)
					{
						Text("%04X", cell->writePC);
						
						// code label
						TableNextColumn();
						buf[0] = 0;
						if (symbols->currentSegment)
						{
							symbols->currentSegment->FindLabelText(cell->writePC, buf);
						}
						Text(buf);

						// code
						TableNextColumn();
						// symbols->dataAdapter is data adapter that includes ROM
						u8 op = symbols->dataAdapter->AdapterReadByteModulus(cell->writePC);
						u8 lo = symbols->dataAdapter->AdapterReadByteModulus(cell->writePC+1);
						u8 hi = symbols->dataAdapter->AdapterReadByteModulus(cell->writePC+2);
						viewDisassembly->MnemonicWithArgumentToStr(addr, op, lo, hi, buf);
						Text(buf);

						// frame when value changed
						TableNextColumn();
						Text("%d", cell->writeFrame);

						// cycle when value changed
						TableNextColumn();
						Text("%d", cell->writeCycle);
					}
					else
					{
						Text("");	// writePC
						TableNextColumn();
						Text("");	// code label
						TableNextColumn();
						Text("");	// code
						TableNextColumn();
						Text("");	// frame
						TableNextColumn();
						Text("");	// cycle
					}

					// go to addr
					TableNextColumn();
					sprintf(buf, "Memory##%0d", addr);
					if (Button(buf))
					{
						viewMemoryMap->viewDataDump->ScrollToAddress(addr);
					}
					
					// go to where changed
					TableNextColumn();
					if (cell->writePC >= 0)
					{
						sprintf(buf, "Code##%0d", addr);
						if (Button(buf))
						{
							viewDisassembly->ScrollToAddress(cell->writePC);
						}
						
						// rewind to where changed
						TableNextColumn();
						sprintf(buf, "Rewind##%0d", addr);
						if (Button(buf))
						{
							debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->writeCycle);
						}
					}
					else
					{
						Text("");
						TableNextColumn();
						Text("");
					}
				}
			}
			
			EndTable();
		}
	}
	
	PostRenderImGui();
}

bool CViewDataMonitor::HasContextMenuItems()
{
	return false;
}

void CViewDataMonitor::RenderContextMenuItems()
{
}

void CViewDataMonitor::ActivateView()
{
	LOGG("CViewDataMonitor::ActivateView()");
}

void CViewDataMonitor::DeactivateView()
{
	LOGG("CViewDataMonitor::DeactivateView()");
}

//
CDataCaptureState::CDataCaptureState(u64 cycle, u32 frame, u8 *dataCopy)
{
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%d frame, %d cycle", frame, cycle);
	this->name = STRALLOC(buf);
	SYS_ReleaseCharBuf(buf);
	
	this->cycle = cycle;
	this->frame = frame;
	this->dataCopy = dataCopy;
}

CDataCaptureState::~CDataCaptureState()
{
	STRFREE(this->name);
	delete [] dataCopy;
}


