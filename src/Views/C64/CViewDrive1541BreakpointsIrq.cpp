#include "CViewDrive1541BreakpointsIrq.h"
#include "CGuiMain.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsSegmentDrive1541.h"

CViewDrive1541BreakpointsIrq::CViewDrive1541BreakpointsIrq(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugSymbolsDrive1541 *debugSymbolsDrive1541)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugSymbolsDrive1541 = debugSymbolsDrive1541;
}

CViewDrive1541BreakpointsIrq::~CViewDrive1541BreakpointsIrq()
{
}

void CViewDrive1541BreakpointsIrq::RenderImGui()
{
	PreRenderImGui();

	if (debugSymbolsDrive1541)
	{
		if (debugSymbolsDrive1541->currentSegment)
		{
			CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*)debugSymbolsDrive1541->currentSegment;
//			ImGui::Text("Segment: %s", segment->name);
		
			ImGui::Checkbox("VIA1", &segment->breakOnDrive1541IrqVIA1);
			ImGui::SameLine();
			ImGui::Checkbox("VIA2", &segment->breakOnDrive1541IrqVIA2);
			ImGui::SameLine();
			ImGui::Checkbox("IEC", &segment->breakOnDrive1541IrqIEC);
		}
	}

	PostRenderImGui();
}

bool CViewDrive1541BreakpointsIrq::HasContextMenuItems()
{
	return false;
}

void CViewDrive1541BreakpointsIrq::RenderContextMenuItems()
{
}

void CViewDrive1541BreakpointsIrq::ActivateView()
{
	LOGG("CViewC64IrqBreakpoints::ActivateView()");
}

void CViewDrive1541BreakpointsIrq::DeactivateView()
{
	LOGG("CViewC64IrqBreakpoints::DeactivateView()");
}
