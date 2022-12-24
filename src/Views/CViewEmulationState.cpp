#include "CViewEmulationState.h"
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
#include "VID_ImageBinding.h"
#include "CLayoutParameter.h"

CViewEmulationState::CViewEmulationState(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontSize = 10.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationState::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationState::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewEmulationState::DoLogic()
{
}

void CViewEmulationState::Render()
{
	float px = posX;
	float py = posY;
	
	char buf[128];
	sprintf (buf, "Emulation speed: %6.2f%% FPS: %4.1f  %s",
			 debugInterface->emulationSpeed, debugInterface->emulationFrameRate,
			(debugInterface->GetSettingIsWarpSpeed() ? "(Warp)" : ""));
	fontBytes->BlitText(buf, px, py, posZ, fontSize);
}

void CViewEmulationState::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


bool CViewEmulationState::DoTap(float x, float y)
{
	return false;
}


bool CViewEmulationState::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{

	return false;
}

bool CViewEmulationState::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

// Layout
void CViewEmulationState::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewEmulationState::Deserialize(CByteBuffer *byteBuffer)
{
}

