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
#include "MT_UiScale.h"


CViewInputEvents::CViewInputEvents(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
													 CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontSize = MT_UiScaled(7.0f);
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

void CViewInputEvents::RenderImGui()
{
	PreRenderImGui();

	ImGui::Checkbox("Store", &(debugInterface->snapshotsManager->isStoreInputEventsEnabled));
	ImGui::Checkbox("Replay", &(debugInterface->snapshotsManager->isReplayInputEventsEnabled));

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

