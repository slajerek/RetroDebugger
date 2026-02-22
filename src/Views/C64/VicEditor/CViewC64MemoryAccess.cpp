#include "CViewC64MemoryAccess.h"
#include "CViewC64.h"
#include "CViewC64VicEditor.h"
#include "CVicEditorLayerMemoryAccess.h"
#include "CDebugInterfaceVice.h"
#include "CViewDisassembly.h"
#include "imgui.h"

extern "C"
{
#include "ViceWrapper.h"
};

CViewC64MemoryAccess::CViewC64MemoryAccess(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->layer = NULL;
	this->viewDisassembly = NULL;
	memset(inputAddrStart, 0, sizeof(inputAddrStart));
	memset(inputAddrEnd, 0, sizeof(inputAddrEnd));
	memset(inputLabel, 0, sizeof(inputLabel));
	nextColorIndex = 0;
	CVicEditorLayerMemoryAccess::GenerateGoldenAngleColor(nextColorIndex, &inputColor[0], &inputColor[1], &inputColor[2]);
}

bool CViewC64MemoryAccess::ParseHexAddress(const char *str, uint16_t *outAddr)
{
	if (!str || str[0] == '\0')
		return false;

	const char *p = str;
	if (*p == '$')
		p++;
	if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X'))
		p += 2;

	char *end = NULL;
	unsigned long val = strtoul(p, &end, 16);
	if (end == p || val > 0xFFFF)
		return false;

	*outAddr = (uint16_t)val;
	return true;
}

void CViewC64MemoryAccess::RenderWatchList()
{
	if (layer->watchEntries.empty())
		return;

	if (ImGui::BeginTable("##MemWatchList", 5,
						  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
						  ImGuiTableFlags_SizingFixedFit,
						  ImVec2(0, 0)))
	{
		ImGui::TableSetupColumn("En", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 100);
		ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 35);
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableHeadersRow();

		int removeIdx = -1;
		for (int i = 0; i < (int)layer->watchEntries.size(); i++)
		{
			MemAccessWatchEntry *entry = &layer->watchEntries[i];
			ImGui::PushID(i);

			bool wasDisabled = !entry->enabled;
			if (wasDisabled)
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);

			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			if (ImGui::Checkbox("##en", &entry->enabled))
			{
				layer->RebuildWatchTable();
				layer->SaveToConfig();
			}

			ImGui::TableNextColumn();
			if (entry->addrStart == entry->addrEnd)
				ImGui::Text("$%04X", entry->addrStart);
			else
				ImGui::Text("$%04X-$%04X", entry->addrStart, entry->addrEnd);

			ImGui::TableNextColumn();
			char colorLabel[16];
			snprintf(colorLabel, sizeof(colorLabel), "##col%d", i);
			if (ImGui::ColorEdit3(colorLabel, entry->color,
								  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
			{
				layer->SaveToConfig();
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				entry->enabled = !entry->enabled;
				layer->RebuildWatchTable();
				layer->SaveToConfig();
			}

			ImGui::TableNextColumn();
			ImGui::Text("%s", entry->label);

			ImGui::TableNextColumn();
			if (ImGui::SmallButton("X"))
			{
				removeIdx = i;
			}

			if (wasDisabled)
				ImGui::PopStyleVar();

			ImGui::PopID();
		}

		ImGui::EndTable();

		if (removeIdx >= 0)
		{
			layer->watchEntries.erase(layer->watchEntries.begin() + removeIdx);
			layer->RebuildWatchTable();
			layer->SaveToConfig();
		}
	}
}

void CViewC64MemoryAccess::RenderAccessLog()
{
	if (ImGui::BeginTable("##MemAccessLog", 8,
						  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
						  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
						  ImVec2(0, 0)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, 40);
		ImGui::TableSetupColumn("Cycle", ImGuiTableColumnFlags_WidthFixed, 40);
		ImGui::TableSetupColumn("PC", ImGuiTableColumnFlags_WidthFixed, 40);
		ImGui::TableSetupColumn("R/W", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 45);
		ImGui::TableSetupColumn("Val", ImGuiTableColumnFlags_WidthFixed, 30);
		ImGui::TableSetupColumn("ROM", ImGuiTableColumnFlags_WidthFixed, 35);
		ImGui::TableSetupColumn("Entry", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (int rasterLine = 0; rasterLine < 312; rasterLine++)
		{
			for (int rasterCycle = 0; rasterCycle < 63; rasterCycle++)
			{
				vicii_cycle_state_t *state = c64d_get_vicii_state_for_raster_cycle(rasterLine, rasterCycle);

				if (state->memAccessAddr == -1)
					continue;

				if (state->memAccessIsWrite && !layer->showWrites)
					continue;
				if (!state->memAccessIsWrite && !layer->showReads)
					continue;

				int entryIdx = layer->FindEntryForAddress((uint16_t)state->memAccessAddr);
				if (entryIdx < 0)
					continue;

				MemAccessWatchEntry *entry = &layer->watchEntries[entryIdx];

				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				char rowId[32];
				sprintf(rowId, "##m%d_%d", rasterLine, rasterCycle);
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
				ImGui::Text(state->memAccessIsWrite ? "W" : "R");

				ImGui::TableNextColumn();
				ImVec4 color(entry->color[0], entry->color[1], entry->color[2], 1.0f);
				ImGui::TextColored(color, "$%04X", state->memAccessAddr);

				ImGui::TableNextColumn();
				ImGui::Text("%02X", state->memAccessValue);

				ImGui::TableNextColumn();
				if (state->memAccessIsWrite)
					ImGui::Text("RAM");
				else
					ImGui::Text(CVicEditorLayerMemoryAccess::IsRomAtAddress(state, (uint16_t)state->memAccessAddr) ? "ROM" : "RAM");

				ImGui::TableNextColumn();
				if (entry->label[0] != '\0')
					ImGui::Text("%s", entry->label);
				else if (entry->addrStart == entry->addrEnd)
					ImGui::Text("$%04X", entry->addrStart);
				else
					ImGui::Text("$%04X-$%04X", entry->addrStart, entry->addrEnd);
			}
		}

		ImGui::EndTable();
	}
}

void CViewC64MemoryAccess::RenderImGui()
{
	PreRenderImGui();

	if (layer == NULL)
	{
		layer = viewC64->viewVicEditor->layerMemoryAccess;
	}

	// Alpha slider + checkboxes
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.35f);
	if (ImGui::SliderFloat("##Alpha", &layer->layerAlpha, 0.0f, 1.0f, "%.2f"))
	{
		layer->SaveToConfig();
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Write", &layer->showWrites))
	{
		layer->SaveToConfig();
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Read", &layer->showReads))
	{
		layer->SaveToConfig();
	}

	ImGui::Separator();

	// Add entry section
	ImGui::Text("Add Watch:");
	ImGui::SetNextItemWidth(50);
	ImGui::InputText("Start##addr", inputAddrStart, 5, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::InputText("End##addr", inputAddrEnd, 5, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::InputText("Label##entry", inputLabel, sizeof(inputLabel));
	ImGui::SameLine();
	ImGui::ColorEdit3("##addcolor", inputColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
	ImGui::SameLine();

	if (ImGui::Button("Add"))
	{
		uint16_t addrStart, addrEnd;
		if (ParseHexAddress(inputAddrStart, &addrStart))
		{
			if (!ParseHexAddress(inputAddrEnd, &addrEnd))
				addrEnd = addrStart;

			if (addrEnd < addrStart)
			{
				uint16_t tmp = addrStart;
				addrStart = addrEnd;
				addrEnd = tmp;
			}

			if ((int)layer->watchEntries.size() < MEM_ACCESS_MAX_ENTRIES)
			{
				MemAccessWatchEntry entry;
				entry.addrStart = addrStart;
				entry.addrEnd = addrEnd;
				entry.color[0] = inputColor[0];
				entry.color[1] = inputColor[1];
				entry.color[2] = inputColor[2];
				entry.enabled = true;
				strncpy(entry.label, inputLabel, sizeof(entry.label) - 1);
				entry.label[sizeof(entry.label) - 1] = '\0';

				layer->watchEntries.push_back(entry);
				layer->RebuildWatchTable();
				layer->SaveToConfig();

				// Reset inputs for next entry
				memset(inputAddrStart, 0, sizeof(inputAddrStart));
				memset(inputAddrEnd, 0, sizeof(inputAddrEnd));
				memset(inputLabel, 0, sizeof(inputLabel));
				nextColorIndex++;
				CVicEditorLayerMemoryAccess::GenerateGoldenAngleColor(nextColorIndex, &inputColor[0], &inputColor[1], &inputColor[2]);
			}
		}
	}

	ImGui::Separator();

	// Watch list
	RenderWatchList();

	ImGui::Separator();

	// Access log
	ImGui::Text("Access Log:");
	RenderAccessLog();

	PostRenderImGui();
}
