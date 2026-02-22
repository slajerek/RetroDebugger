#include "CViewDebugEventsHistory.h"
#include "CGuiMain.h"
#include "CDebugInterface.h"
#include "CDebugEventsHistory.h"
#include "CSnapshotsManager.h"

using namespace ImGui;

CViewDebugEventsHistory::CViewDebugEventsHistory(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
}

CViewDebugEventsHistory::~CViewDebugEventsHistory()
{
}

void CViewDebugEventsHistory::RenderImGui()
{
	PreRenderImGui();

	if (debugInterface->symbols == NULL)
	{
		Text("Debug symbols are not available.");
		PostRenderImGui();
		return;
	}
	
	//
	CDebugEventsHistory *eventsHistory = debugInterface->symbols->debugEventsHistory;
	u64 currentCycle = debugInterface->GetMainCpuCycleCounter();

	eventsHistory->mutex->Lock();
	if (Button("MARK"))
	{
		eventsHistory->CreateEventManual();
	}
	SameLine();
	if (Button("PREV"))
	{
		std::map<u64, CDebugEvent *>::iterator itNext = eventsHistory->events.lower_bound(currentCycle);
		if (itNext != eventsHistory->events.end() && itNext != eventsHistory->events.begin())
		{
			std::map<u64, CDebugEvent *>::iterator itPrev = std::prev(itNext);
			CDebugEvent *eventPrev = itPrev->second;
			debugInterface->snapshotsManager->RestoreSnapshotByCycle(eventPrev->cycle);
		}
	}
	SameLine();
	if (Button("NEXT"))
	{
		std::map<u64, CDebugEvent *>::iterator itNext = eventsHistory->events.upper_bound(currentCycle);
		if (itNext != eventsHistory->events.end())
		{
			CDebugEvent *eventNext = itNext->second;
			debugInterface->snapshotsManager->RestoreSnapshotByCycle(eventNext->cycle);
		}
	}
	
	if (BeginTable("##CViewDebugEventsHistory", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable))
	{
		TableSetupColumn("Cycle");
		TableSetupColumn("Frame");
		TableSetupColumn("Type");
		TableSetupColumn("Comment");
		TableHeadersRow();
		
		char *buf = SYS_GetCharBuf();

		ImU32 hoveredRowBgColor = ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.65f));

		int i = 0;
		for (std::map<u64, CDebugEvent *>::iterator it = eventsHistory->events.begin(); it != eventsHistory->events.end(); it++)
		{
			CDebugEvent *event = it->second;
			
//			if (Selectable("##nolabel", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
//			{
//				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, hoveredRowBgColor);
//			}
//			SameLine();
			
			TableNextColumn();
			
			sprintf(buf, "%d", event->cycle);
			
			const bool itemIsSelected = (event->cycle == currentCycle);
			ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
			if (ImGui::Selectable(buf, itemIsSelected, selectableFlags, ImVec2(0, 0)))
			{
				debugInterface->snapshotsManager->RestoreSnapshotByCycle(event->cycle);
			}
			
//			Text("%d", event->cycle);
			TableNextColumn();
			Text("%d", event->frame);
			TableNextColumn();
			if (event->eventType == DEBUG_EVENT_TYPE_BREAKPOINT)
			{
				Text("Breakpoint");
			}
			else if (event->eventType == DEBUG_EVENT_TYPE_MANUAL)
			{
				Text("Manual");
			}
			else
			{
				Text("");
			}
			TableNextColumn();
			sprintf(buf, "##comment%d", i);
			InputText(buf, event->comment, DEBUG_EVENTS_HISTORY_MAX_COMMENT_LENGTH);
			i++;
		}
		
		EndTable();
		SYS_ReleaseCharBuf(buf);
	}
	eventsHistory->mutex->Unlock();

	PostRenderImGui();
}

bool CViewDebugEventsHistory::HasContextMenuItems()
{
	return false;
}

void CViewDebugEventsHistory::RenderContextMenuItems()
{
}

void CViewDebugEventsHistory::ActivateView()
{
}

void CViewDebugEventsHistory::DeactivateView()
{
}
