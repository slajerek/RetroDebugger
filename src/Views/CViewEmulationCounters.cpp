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
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterface.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CLayoutParameter.h"

CViewEmulationCounters::CViewEmulationCounters(char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
													 CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationCounters::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
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

//	sprintf(buf, "CYCLE: %9lld", debugInterface->GetMainCpuCycleCounter());
	sprintf(buf, "CYCLE: %9lld", debugInterface->GetMainCpuDebugCycleCounter());
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
//	py += fontSize;
	
}


bool CViewEmulationCounters::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	guiMain->UnlockMutex();
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

void CViewEmulationCounters::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

void CViewEmulationCounters::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}
