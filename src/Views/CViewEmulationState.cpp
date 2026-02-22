#include "CViewEmulationState.h"
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
	hasManualFontSize = false;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	fontBytes = viewC64->fontDisassembly;
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationState::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	bool sizeChanged = (fabs(sizeX - this->sizeX) > 0.5f || fabs(sizeY - this->sizeY) > 0.5f);

	if (hasManualFontSize && !sizeChanged)
	{
		// Size unchanged (startup/layout restore): keep manually set font size
	}
	else
	{
		// Auto-scale: ~43 chars wide, 1 row tall
		fontSize = fmin(sizeX / 35.0f, sizeY);

		if (sizeChanged)
			hasManualFontSize = false;
	}

	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewEmulationState::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	if (layoutParameter != NULL)
	{
		// User manually changed font size via context menu
		hasManualFontSize = true;
	}
	else
	{
		// Layout restore: check if saved fontSize differs from auto-scaled value
		float autoFontSize = fmin(sizeX / 35.0f, sizeY);

		if (fabs(fontSize - autoFontSize) > 0.01f)
		{
			hasManualFontSize = true;
		}
	}

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
	bool isWarp = debugInterface->GetSettingIsWarpSpeed();
	sprintf (buf, "Emulation speed: %6.2f%% FPS: %4.1f  %s",
			 debugInterface->emulationSpeed, debugInterface->emulationFrameRate,
			(isWarp ? "(Warp)" : ""));
	if (isWarp)
	{
		fontBytes->BlitTextColor(buf, px, py, posZ, fontSize, 1.0f, 0.8f, 0.2f, 1.0f);
	}
	else
	{
		fontBytes->BlitText(buf, px, py, posZ, fontSize);
	}
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

