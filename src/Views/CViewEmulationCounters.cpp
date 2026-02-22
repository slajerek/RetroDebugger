extern "C" {
#include "c64.h"
}
#include "CViewEmulationCounters.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterface.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CLayoutParameter.h"
#include "CMainMenuBar.h"

CViewEmulationCounters::CViewEmulationCounters(char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
													 CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontSize = 7.0f;
	hasManualFontSize = false;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;

	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationCounters::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	bool sizeChanged = (fabs(sizeX - this->sizeX) > 0.5f || fabs(sizeY - this->sizeY) > 0.5f);

	if (hasManualFontSize && !sizeChanged)
	{
		// Size unchanged (startup/layout restore): keep manually set font size
	}
	else
	{
		// Auto-scale: 16 chars wide, 3 rows tall
		fontSize = fmin(sizeX / 16.0f, sizeY / 3.0f);

		if (sizeChanged)
			hasManualFontSize = false;
	}

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationCounters::DoLogic()
{
}

void CViewEmulationCounters::Render()
{
	this->RenderEmulationCounters(posX, posY, posZ, fontBytes, fontSize);
}

void CViewEmulationCounters::RenderImGui()
{
	PreRenderImGui();
	RenderEmulationCounters(posX, posY, posZ, fontBytes, fontSize);
	PostRenderImGui();
}

extern "C" {
	unsigned int c64d_get_maincpu_clock();
}

void CViewEmulationCounters::RenderEmulationCounters(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize)
{
//	LOGD("RenderEmulationCounters");
	
	char buf[256];
	
	int frameNum = debugInterface->GetEmulationFrameNumber();
	sprintf(buf, "FRAME: %9d", frameNum);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	float emulationFPS = debugInterface->GetEmulationFPS();
	
	float t = (float)frameNum / emulationFPS;
	float mins = floor(t / 60.0f);
	float secs = t - mins*60.0f;
	
	sprintf(buf, " TIME:%4.0f:%05.2f", mins, secs);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	sprintf(buf, "CYCLE: %9lld", debugInterface->GetMainCpuCycleCounter());
//	sprintf(buf, "CYCLE: %9lld", debugInterface->GetMainCpuDebugCycleCounter());
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
//	py += fontSize;
	
}


bool CViewEmulationCounters::DoTap(float x, float y)
{
//	guiMain->LockMutex();
//	guiMain->UnlockMutex();
	return false;
}

//
bool CViewEmulationCounters::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewEmulationCounters::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewEmulationCounters::HasContextMenuItems()
{
	return true;
}

void CViewEmulationCounters::RenderContextMenuItems()
{
	if (ImGui::MenuItem("Reset counters", viewC64->mainMenuBar->kbsResetCpuCycleAndFrameCounters->cstr, false))
	{
		viewC64->mainMenuBar->kbsResetCpuCycleAndFrameCounters->Run();
	}
	ImGui::Separator();
}

void CViewEmulationCounters::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	if (layoutParameter != NULL)
	{
		// User manually changed font size via context menu
		hasManualFontSize = true;
	}
	else
	{
		// Layout restore: check if saved fontSize differs from auto-scaled value
		float autoFontSize = fmin(sizeX / 16.0f, sizeY / 3.0f);

		if (fabs(fontSize - autoFontSize) > 0.01f)
		{
			hasManualFontSize = true;
		}
	}

	CGuiView::LayoutParameterChanged(layoutParameter);
}
