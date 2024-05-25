extern "C" {
#include "c64.h"
}
#include "CViewInputEvents.h"
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
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CSnapshotsManager.h"

#include "CDebugInterfaceNes.h"
#include "NstApiMachine.hpp"
#include "NstMachine.hpp"
#include "NstApiEmulator.hpp"
#include "NstApiInput.hpp"
#include "NstCpu.hpp"
#include "NstPpu.hpp"


CViewInputEvents::CViewInputEvents(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
													 CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontSize = 7.0f;
	fontBytes = viewC64->fontDisassembly;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewInputEvents::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewInputEvents::DoLogic()
{
}

void CViewInputEvents::Render()
{
}

u8 nesd_get_api_input_buttons();

void CViewInputEvents::RenderImGui()
{
//	LOGD("CViewInputEvents");
	
	PreRenderImGui();
	
	ImGui::Checkbox("Store", &(debugInterface->snapshotsManager->isStoreInputEventsEnabled));
	ImGui::Checkbox("Replay", &(debugInterface->snapshotsManager->isReplayInputEventsEnabled));
	ImGui::Checkbox("Overwrite", &(debugInterface->snapshotsManager->isOverwriteInputEventsEnabled));

	ImGui::Text("Joystick #1");

	// NES
	u8 nesInputApiButtons = nesd_get_api_input_buttons();
	
	bool buttonA = nesInputApiButtons & 0x01;
	bool buttonB = nesInputApiButtons & 0x02;
	bool buttonSelect = nesInputApiButtons & 0x04;
	bool buttonStart = nesInputApiButtons & 0x08;
	bool buttonUp = nesInputApiButtons & 0x10;
	bool buttonDown = nesInputApiButtons & 0x20;
	bool buttonLeft = nesInputApiButtons & 0x40;
	bool buttonRight = nesInputApiButtons & 0x80;
	
	ImGui::Checkbox("A", &buttonA);
	ImGui::Checkbox("B", &buttonB);
	ImGui::Checkbox("SELECT", &buttonSelect);
	ImGui::Checkbox("START", &buttonStart);
	ImGui::Checkbox("UP", &buttonUp);
	ImGui::Checkbox("DOWN", &buttonDown);
	ImGui::Checkbox("LEFT", &buttonLeft);
	ImGui::Checkbox("RIGHT", &buttonRight);

//	enum
//	{
//		A      = 0x01,
//		B      = 0x02,
//		SELECT = 0x04,
//		START  = 0x08,
//		UP     = 0x10,
//		DOWN   = 0x20,
//		LEFT   = 0x40,
//		RIGHT  = 0x80
//	};
//	volatile uint buttons;

	/*
	float px = posX;
	float py = posY;
	
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

//	sprintf(buf, "CYCLE: %9d", debugInterface->GetMainCpuCycleCounter());
	sprintf(buf, "CYCLE: %9d", debugInterface->GetMainCpuDebugCycleCounter());
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	
//	py += fontSize;
	*/
	
	PostRenderImGui();
}


bool CViewInputEvents::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	guiMain->UnlockMutex();
	return false;
}

//
bool CViewInputEvents::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewInputEvents::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewInputEvents::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

