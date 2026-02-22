#include "CViewC64IoAccess.h"
#include "CViewC64.h"
#include "CViewC64VicEditor.h"
#include "CVicEditorLayerIoAccess.h"
#include "CDebugInterfaceVice.h"
#include "CViewDisassembly.h"
#include "imgui.h"
#include "font_awesome_5.h"

extern "C"
{
#include "ViceWrapper.h"
};

CViewC64IoAccess::CViewC64IoAccess(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->layer = NULL;
	this->viewDisassembly = NULL;
}

void CViewC64IoAccess::RenderChipSection(int chipIndex)
{
	IOAccessChipConfig *chip = &layer->chips[chipIndex];

	// Collapsible header with address range
	static const char *addrRanges[] = { "$D000", "$DC00", "$DD00", "$D400" };
	char headerLabel[64];
	snprintf(headerLabel, sizeof(headerLabel), "%s (%s)###chip%d", chip->chipName, addrRanges[chipIndex], chipIndex);

	ImGui::PushID(chipIndex);

	ImGui::SetNextItemOpen(chip->sectionOpen, ImGuiCond_Once);
	bool isOpen = ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_AllowOverlap);
	if (isOpen != chip->sectionOpen)
	{
		chip->sectionOpen = isOpen;
		layer->SaveColorsToConfig();
	}

	// ON/OFF buttons on the right side of the header bar
	{
		float btnW = ImGui::CalcTextSize("OFF").x + ImGui::GetStyle().FramePadding.x * 2;
		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		float rightEdge = ImGui::GetWindowContentRegionMax().x + ImGui::GetWindowPos().x;
		float btnY = ImGui::GetItemRectMin().y + (ImGui::GetItemRectSize().y - ImGui::GetFrameHeight()) * 0.5f;

		ImGui::SameLine(0, 0);
		ImGui::SetCursorScreenPos(ImVec2(rightEdge - 2 * btnW - spacing - ImGui::GetStyle().FramePadding.x, btnY));
		if (ImGui::Button("ON", ImVec2(btnW, 0)))
		{
			for (int i = 0; i < chip->numRegisters; i++)
				chip->registerEnabled[i] = true;
			layer->SaveColorsToConfig();
		}
		ImGui::SameLine(0, spacing);
		if (ImGui::Button("OFF", ImVec2(btnW, 0)))
		{
			for (int i = 0; i < chip->numRegisters; i++)
				chip->registerEnabled[i] = false;
			layer->SaveColorsToConfig();
		}
	}

	if (!isOpen)
	{
		ImGui::PopID();
		return;
	}

	// Register color grid
	float cellWidth = ImGui::CalcTextSize("$XX").x + ImGui::GetStyle().ItemSpacing.x + ImGui::GetFrameHeight() + ImGui::GetStyle().CellPadding.x * 2;
	int columns = (int)(ImGui::GetContentRegionAvail().x / cellWidth);
	if (columns < 1) columns = 1;

	char tableId[32];
	snprintf(tableId, sizeof(tableId), "##RegColors%d", chipIndex);

	if (ImGui::BeginTable(tableId, columns, ImGuiTableFlags_SizingFixedFit))
	{
		for (int i = 0; i < chip->numRegisters; i++)
		{
			ImGui::TableNextColumn();

			bool enabled = chip->registerEnabled[i];
			if (!enabled)
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);

			ImGui::Text("$%02X", i);
			ImGui::SameLine();

			char label[16];
			snprintf(label, sizeof(label), "##reg%d_%02X", chipIndex, i);
			ImGui::SetNextItemWidth(30);
			if (ImGui::ColorEdit3(label, chip->registerColors[i],
								  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
			{
				layer->SaveColorsToConfig();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				chip->registerEnabled[i] = !chip->registerEnabled[i];
				layer->SaveColorsToConfig();
			}

			if (!enabled)
				ImGui::PopStyleVar();
		}
		ImGui::EndTable();
	}

	ImGui::PopID();
}

// Helper to get chip index and register from a cycle state
static bool GetAccessInfo(vicii_cycle_state_t *state, bool showWrites, bool showReads,
						  int *outChipIdx, int *outReg, bool *outIsWrite, u8 *outValue)
{
	if (showWrites)
	{
		if (state->registerWritten != -1)
		{
			*outChipIdx = IOACCESS_CHIP_VIC; *outReg = state->registerWritten;
			*outIsWrite = true; *outValue = state->regs[state->registerWritten & 0x3F];
			return true;
		}
		if (state->cia1RegisterWritten != -1)
		{
			*outChipIdx = IOACCESS_CHIP_CIA1; *outReg = state->cia1RegisterWritten;
			*outIsWrite = true; *outValue = state->cia1WriteValue;
			return true;
		}
		if (state->cia2RegisterWritten != -1)
		{
			*outChipIdx = IOACCESS_CHIP_CIA2; *outReg = state->cia2RegisterWritten;
			*outIsWrite = true; *outValue = state->cia2WriteValue;
			return true;
		}
		if (state->sidRegisterWritten != -1)
		{
			*outChipIdx = IOACCESS_CHIP_SID; *outReg = state->sidRegisterWritten;
			*outIsWrite = true; *outValue = state->sidWriteValue;
			return true;
		}
	}

	if (showReads)
	{
		if (state->registerRead != -1)
		{
			*outChipIdx = IOACCESS_CHIP_VIC; *outReg = state->registerRead;
			*outIsWrite = false; *outValue = state->regs[state->registerRead & 0x3F];
			return true;
		}
		if (state->cia1RegisterRead != -1)
		{
			*outChipIdx = IOACCESS_CHIP_CIA1; *outReg = state->cia1RegisterRead;
			*outIsWrite = false; *outValue = state->cia1ReadValue;
			return true;
		}
		if (state->cia2RegisterRead != -1)
		{
			*outChipIdx = IOACCESS_CHIP_CIA2; *outReg = state->cia2RegisterRead;
			*outIsWrite = false; *outValue = state->cia2ReadValue;
			return true;
		}
		if (state->sidRegisterRead != -1)
		{
			*outChipIdx = IOACCESS_CHIP_SID; *outReg = state->sidRegisterRead;
			*outIsWrite = false; *outValue = state->sidReadValue;
			return true;
		}
	}

	return false;
}

void CViewC64IoAccess::RenderAccessLogEntries()
{
	if (ImGui::BeginTable("##IoAccessLog", 7,
						  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
						  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
						  ImVec2(0, 0)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, 50);
		ImGui::TableSetupColumn("Cycle", ImGuiTableColumnFlags_WidthFixed, 50);
		ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 50);
		ImGui::TableSetupColumn("Chip", ImGuiTableColumnFlags_WidthFixed, 40);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 30);
		ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed, 70);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 50);
		ImGui::TableHeadersRow();

		for (int rasterLine = 0; rasterLine < 312; rasterLine++)
		{
			for (int rasterCycle = 0; rasterCycle < 63; rasterCycle++)
			{
				vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);

				int chipIdx = -1;
				int reg = -1;
				bool isWrite = false;
				u8 value = 0;

				if (!GetAccessInfo(state, layer->showWrites, layer->showReads, &chipIdx, &reg, &isWrite, &value))
					continue;

				IOAccessChipConfig *chip = &layer->chips[chipIdx];
				bool regEnabled = (reg >= 0 && reg < chip->numRegisters) ? chip->registerEnabled[reg] : true;

				if (!regEnabled)
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);

				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				char rowId[32];
				sprintf(rowId, "##a%d_%d", rasterLine, rasterCycle);
				if (ImGui::Selectable(rowId, false,
									  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
				{
					if (viewDisassembly)
						viewDisassembly->ScrollToAddress(state->pc);
				}
				ImGui::SameLine();
				ImGui::Text("%d", rasterLine);

				ImGui::TableNextColumn();
				ImGui::Text("%d", rasterCycle);

				ImGui::TableNextColumn();
				ImGui::Text("%04X", state->pc);

				ImGui::TableNextColumn();
				ImGui::Text("%s", chip->chipName);

				ImGui::TableNextColumn();
				ImGui::Text(isWrite ? "W" : "R");

				ImGui::TableNextColumn();
				if (reg >= 0 && reg < chip->numRegisters)
				{
					ImVec4 color(chip->registerColors[reg][0],
								 chip->registerColors[reg][1],
								 chip->registerColors[reg][2], 1.0f);
					ImGui::TextColored(color, "%s%02X", chip->addrPrefix, reg);
				}
				else
				{
					ImGui::Text("%s%02X", chip->addrPrefix, reg);
				}

				ImGui::TableNextColumn();
				ImGui::Text("$%02X", value);

				if (!regEnabled)
					ImGui::PopStyleVar();
			}
		}

		ImGui::EndTable();
	}
}

void CViewC64IoAccess::RenderImGui()
{
	PreRenderImGui();

	if (layer == NULL)
	{
		layer = viewC64->viewVicEditor->layerIoAccess;
	}

	// Alpha slider + checkboxes on one line
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.35f);
	if (ImGui::SliderFloat("##Alpha", &layer->layerAlpha, 0.0f, 1.0f, "%.2f"))
	{
		layer->SaveColorsToConfig();
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Write", &layer->showWrites))
	{
		layer->SaveColorsToConfig();
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Read", &layer->showReads))
	{
		layer->SaveColorsToConfig();
	}

	// Collapsible per-chip sections
	for (int c = 0; c < IOACCESS_NUM_CHIPS; c++)
	{
		RenderChipSection(c);
	}

	ImGui::Separator();

	// Access log table
	ImGui::Text("Access Log:");
	RenderAccessLogEntries();

	PostRenderImGui();
}
