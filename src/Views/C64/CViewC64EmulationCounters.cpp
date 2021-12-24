extern "C" {
#include "c64.h"
}
#include "CViewC64EmulationCounters.h"
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
#include "CDebugInterfaceC64.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "CLayoutParameter.h"
#include "VID_ImageBinding.h"

CViewC64EmulationCounters::CViewC64EmulationCounters(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	fontSize = 7.0f;
	
	fontBytes = viewC64->fontDisassembly;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64EmulationCounters::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewC64EmulationCounters::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64EmulationCounters::DoLogic()
{
}

void CViewC64EmulationCounters::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64EmulationCounters::Render()
{
	this->RenderEmulationCounters(posX, posY, posZ, fontBytes, fontSize);
}

extern "C" {
	unsigned int c64d_get_maincpu_clock();
}

void CViewC64EmulationCounters::RenderEmulationCounters(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize)
{
//	LOGD("RenderEmulationCounters");
	
	char buf[256];
	
	int frameNum = debugInterface->GetEmulationFrameNumber();
	sprintf(buf, "FRAME: %9d", frameNum);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	float emulationFPS = viewC64->debugInterfaceC64->GetEmulationFPS();
	
	float t = (float)frameNum / emulationFPS;
	float mins = floor(t / 60.0f);
	float secs = t - mins*60.0f;
	
	sprintf(buf, " TIME:%4.0f:%05.2f", mins, secs);
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	//	sprintf(buf, "CYCLE: %9d", debugInterface->GetMainCpuCycleCounter());
	sprintf(buf, "CYCLE: %9d", debugInterface->GetMainCpuCycleCounter());
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
//	py += fontSize;
	
}


bool CViewC64EmulationCounters::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	guiMain->UnlockMutex();
	return false;
}

//
bool CViewC64EmulationCounters::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewC64EmulationCounters::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewC64EmulationCounters::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

// Layout
void CViewC64EmulationCounters::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewC64EmulationCounters::Deserialize(CByteBuffer *byteBuffer)
{
}


