#include "CViewC64BreakpointsIrq.h"
#include "CGuiMain.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegmentC64.h"

CViewC64BreakpointsIrq::CViewC64BreakpointsIrq(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugSymbolsC64 *debugSymbolsC64)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugSymbolsC64 = debugSymbolsC64;
}

CViewC64BreakpointsIrq::~CViewC64BreakpointsIrq()
{
}

void CViewC64BreakpointsIrq::RenderImGui()
{
	PreRenderImGui();

	if (debugSymbolsC64)
	{
		if (debugSymbolsC64->currentSegment)
		{
			CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*)debugSymbolsC64->currentSegment;
//			ImGui::Text("Segment: %s", segment->name);
		
			ImGui::Checkbox("VIC", &segment->breakOnC64IrqVIC);
			ImGui::SameLine();
			ImGui::Checkbox("CIA", &segment->breakOnC64IrqCIA);
			ImGui::SameLine();
			ImGui::Checkbox("NMI", &segment->breakOnC64IrqNMI);
		}
	}

	PostRenderImGui();
}

bool CViewC64BreakpointsIrq::HasContextMenuItems()
{
	return false;
}

void CViewC64BreakpointsIrq::RenderContextMenuItems()
{
}

void CViewC64BreakpointsIrq::ActivateView()
{
	LOGG("CViewC64IrqBreakpoints::ActivateView()");
}

void CViewC64BreakpointsIrq::DeactivateView()
{
	LOGG("CViewC64IrqBreakpoints::DeactivateView()");
}
