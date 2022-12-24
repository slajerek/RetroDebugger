// NOTE THIS IS TO BE DELETED

#include "CViewC64MemoryDebuggerLayoutToolbar.h"
#include "GUI_Main.h"
#include "CDebugInterface.h"
#include "CSnapshotsManager.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "CViewC64.h"
#include "CViewDisassembly.h"
#include "CViewDataDump.h"
#include "CViewMemoryMap.h"
#include "CDebugInterfaceC64.h"

CViewC64MemoryDebuggerLayoutToolbar::CViewC64MemoryDebuggerLayoutToolbar(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewC64MemoryDebuggerLayoutToolbar";
	this->debugInterface = debugInterface;
	
	fontSize = 8.0f;
	
	font = viewC64->fontCBMShifted;
	fontScale = 0.8;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	float px = posX;
	float py = posY;
	float buttonSizeX = 31.0f;//28.0f;
	float buttonSizeY = 10.0f;
	float gap = 5.0f;
	float mgap = 2.5f;
	
	btnMemoryDump1IsFromDisk = new CGuiButtonSwitch(NULL, NULL, NULL,
														 px, py, posZ, buttonSizeX, buttonSizeY,
														 new CSlrString("DRIVE"),
														 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
														 font, fontScale,
														 1.0, 1.0, 1.0, 1.0,
														 1.0, 1.0, 1.0, 1.0,
														 0.3, 0.3, 0.3, 1.0,
														 this);
	btnMemoryDump1IsFromDisk->SetOn(false);
//	SetSwitchButtonDefaultColors(btnMemoryDump1IsFromDisk);
	this->AddGuiElement(btnMemoryDump1IsFromDisk);

	px += buttonSizeX + gap;

	btnMemoryDump2IsFromDisk = new CGuiButtonSwitch(NULL, NULL, NULL,
													px, py, posZ, buttonSizeX, buttonSizeY,
													new CSlrString("DRIVE"),
													FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
													font, fontScale,
													1.0, 1.0, 1.0, 1.0,
													1.0, 1.0, 1.0, 1.0,
													0.3, 0.3, 0.3, 1.0,
													this);
	btnMemoryDump2IsFromDisk->SetOn(false);
	//	SetSwitchButtonDefaultColors(btnMemoryDump2IsFromDisk);
	this->AddGuiElement(btnMemoryDump2IsFromDisk);

	px += buttonSizeX + gap;
	
	btnMemoryDump3IsFromDisk = new CGuiButtonSwitch(NULL, NULL, NULL,
													px, py, posZ, buttonSizeX, buttonSizeY,
													new CSlrString("DRIVE"),
													FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
													font, fontScale,
													1.0, 1.0, 1.0, 1.0,
													1.0, 1.0, 1.0, 1.0,
													0.3, 0.3, 0.3, 1.0,
													this);
	btnMemoryDump3IsFromDisk->SetOn(false);
	//	SetSwitchButtonDefaultColors(btnMemoryDump3IsFromDisk);
	this->AddGuiElement(btnMemoryDump3IsFromDisk);
}

CViewC64MemoryDebuggerLayoutToolbar::~CViewC64MemoryDebuggerLayoutToolbar()
{
}

void CViewC64MemoryDebuggerLayoutToolbar::SetPosition(float posX, float posY)
{
	float px = posX;
	float py = posY;
	float buttonSizeX = 31.0f;//28.0f;
	float buttonSizeY = 10.0f;
	float gap = 5.0f;

	btnMemoryDump1IsFromDisk->SetPosition(px, py);
	px += buttonSizeX + gap;
	btnMemoryDump2IsFromDisk->SetPosition(px, py);
	px += buttonSizeX + gap;
	btnMemoryDump3IsFromDisk->SetPosition(px, py);
}

void CViewC64MemoryDebuggerLayoutToolbar::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64MemoryDebuggerLayoutToolbar::Render()
{
	CGuiView::Render();
}

void CViewC64MemoryDebuggerLayoutToolbar::UpdateStateFromButtons()
{
	bool drive1 = btnMemoryDump1IsFromDisk->IsOn();
	viewC64->viewC64Disassembly->SetVisible(!drive1);
	viewC64->viewC64MemoryDataDump->SetVisible(!drive1);
	viewC64->viewDrive1541Disassembly->SetVisible(drive1);
	viewC64->viewDrive1541MemoryDataDump->SetVisible(drive1);
	btnMemoryDump1IsFromDisk->SetText(drive1 ? "DRIVE" : "C64");

	bool drive2 = btnMemoryDump2IsFromDisk->IsOn();
	viewC64->viewC64Disassembly2->SetVisible(!drive2);
	viewC64->viewC64MemoryDataDump2->SetVisible(!drive2);
	viewC64->viewDrive1541Disassembly2->SetVisible(drive2);
	viewC64->viewDrive1541MemoryDataDump2->SetVisible(drive2);
	btnMemoryDump2IsFromDisk->SetText(drive2 ? "DRIVE" : "C64");
	
	bool drive3 = btnMemoryDump3IsFromDisk->IsOn();
	viewC64->viewC64MemoryDataDump3->SetVisible(!drive3);
	viewC64->viewC64MemoryMap->SetVisible(!drive3);
	viewC64->viewDrive1541MemoryDataDump3->SetVisible(drive3);
	viewC64->viewDrive1541MemoryMap->SetVisible(drive3);
	btnMemoryDump3IsFromDisk->SetText(drive3 ? "DRIVE" : "C64");
	
	// skip debug on not active/visible device
	if (drive1 == true || drive2 == true)
	{
		viewC64->debugInterfaceC64->debugOnDrive1541 = true;
	}
	else
	{
		viewC64->debugInterfaceC64->debugOnDrive1541 = false;
	}
	
	if (drive1 == false || drive2 == false)
	{
		viewC64->debugInterfaceC64->isDebugOn = true;
	}
	else
	{
		viewC64->debugInterfaceC64->isDebugOn = false;
	}
	
	if (drive1 == false)
	{
		if (viewC64->viewDrive1541Disassembly->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewC64Disassembly);
		}
		
		if (viewC64->viewDrive1541MemoryDataDump->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewC64MemoryDataDump);
		}
	}
	else
	{
		if (viewC64->viewC64Disassembly->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewDrive1541Disassembly);
		}
		
		if (viewC64->viewC64MemoryDataDump->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewDrive1541MemoryDataDump);
		}
	}

	if (drive2 == false)
	{
		if (viewC64->viewDrive1541Disassembly2->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewC64Disassembly2);
		}
		
		if (viewC64->viewDrive1541MemoryDataDump2->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewC64MemoryDataDump2);
		}
	}
	else
	{
		if (viewC64->viewC64Disassembly2->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewDrive1541Disassembly2);
		}
		
		if (viewC64->viewC64MemoryDataDump2->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewDrive1541MemoryDataDump2);
		}
	}

	if (drive3 == false)
	{
		if (viewC64->viewDrive1541MemoryDataDump3->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewC64MemoryDataDump3);
		}
	}
	else
	{
		if (viewC64->viewC64MemoryDataDump3->HasFocus())
		{
			guiMain->SetFocus(viewC64->viewDrive1541MemoryDataDump3);
		}
	}
}

void CViewC64MemoryDebuggerLayoutToolbar::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewC64MemoryDebuggerLayoutToolbar::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewC64MemoryDebuggerLayoutToolbar::ButtonPressed(CGuiButton *button)
{
	/*
	if (button == btnDone)
	{
		guiMain->SetView((CGuiView*)guiMain->viewMainEditor);
		GUI_SetPressConsumed(true);
		return true;
	}
	*/
	return false;
}

bool CViewC64MemoryDebuggerLayoutToolbar::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	UpdateStateFromButtons();

	return true;
}

//@returns is consumed
bool CViewC64MemoryDebuggerLayoutToolbar::DoTap(float x, float y)
{
	LOGG("CViewC64MemoryDebuggerLayoutToolbar::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoFinishTap(float x, float y)
{
	LOGG("CViewC64MemoryDebuggerLayoutToolbar::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64MemoryDebuggerLayoutToolbar::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64MemoryDebuggerLayoutToolbar::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64MemoryDebuggerLayoutToolbar::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64MemoryDebuggerLayoutToolbar::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64MemoryDebuggerLayoutToolbar::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64MemoryDebuggerLayoutToolbar::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64MemoryDebuggerLayoutToolbar::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64MemoryDebuggerLayoutToolbar::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64MemoryDebuggerLayoutToolbar::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64MemoryDebuggerLayoutToolbar::IsFocusableElement()
{
	return false;
}

void CViewC64MemoryDebuggerLayoutToolbar::ActivateView()
{
	LOGD("CViewC64MemoryDebuggerLayoutToolbar::ActivateView()");
	
	UpdateStateFromButtons();

	viewC64->viewC64MemoryMap->SetDataDumpView(viewC64->viewC64MemoryDataDump3);
	viewC64->viewDrive1541MemoryMap->SetDataDumpView(viewC64->viewDrive1541MemoryDataDump3);
}

void CViewC64MemoryDebuggerLayoutToolbar::DeactivateView()
{
	LOGD("CViewC64MemoryDebuggerLayoutToolbar::DeactivateView()");
	
	UpdateStateFromButtons();

	viewC64->viewC64MemoryMap->SetDataDumpView(viewC64->viewC64MemoryDataDump);
	viewC64->viewDrive1541MemoryMap->SetDataDumpView(viewC64->viewDrive1541MemoryDataDump);
}


