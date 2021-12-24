#include "C64D_Version.h"

#include "NstApiMachine.hpp"
#include "NstMachine.hpp"
#include "NstApiEmulator.hpp"
#include "NstCpu.hpp"
#include "NstPpu.hpp"

#include "CDataAdapterNesPpuNmt.h"
#include "CDebugInterfaceNes.h"
#include "CViewNesPpuPalette.h"
#include "CColorsTheme.h"
#include "GUI_Main.h"
#include "CGuiMain.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterfaceNes.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"

extern Nes::Api::Emulator nesEmulator;

CViewNesPpuPalette::CViewNesPpuPalette(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	this->showGridLines = true;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}


CViewNesPpuPalette::~CViewNesPpuPalette()
{
}

void CViewNesPpuPalette::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewNesPpuPalette::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewNesPpuPalette::Render()
{
//	LOGD("CViewNesPpuPalette::Render");

	Nes::Core::Machine& machine = nesEmulator;
	Nes::Core::Video::Screen screen = machine.ppu.GetScreen();

	float sx = sizeX/16.0f;
	float sy = sizeY/2.0f;
	
	int colorIndex = 0;
	float py = posY;
	for (int y = 0; y < 2; y++)
	{
		float px = posX;
		for (int x = 0; x < 16; x++)
		{
			u32 pixel = machine.ppu.output.palette[colorIndex];
			u32 rgba = screen.palette[pixel];

			u8 r = (rgba & 0x000000FF)       & 0x000000FF;
			u8 g = (rgba & 0x0000FF00) >> 8  & 0x000000FF;
			u8 b = (rgba & 0x00FF0000) >> 16 & 0x000000FF;

			float fr = (float)r / 255.0f;
			float fg = (float)g / 255.0f;
			float fb = (float)b / 255.0f;
			
			BlitFilledRectangle(px, py, -1, sx, sy, fr, fg, fb, 1);

			colorIndex++;
			px += sx;
		}
		py += sy;
	}
	
	// grid
	float px = posX;
	const float s = 0.5f;
	const float s2 = s*2.0f;
	for (int x = 0; x < 16; x++)
	{
		BlitFilledRectangle(px - s, posY, -1, s2, sizeY, 0.0f, 0.0f, 0.0f, 0.75f);
		px += sx;
	}
	BlitFilledRectangle(posX, posY + sy - s, -1, sizeX, s2, 0.0f, 0.0f, 0.0f, 0.75f);

	if (HasFocus())
	{
		BlitRectangle(posX, posY, -1, sizeX, sizeY, 1.0f, 0.0f, 0, 1);
	}
	else
	{
		BlitRectangle(posX, posY, -1, sizeX, sizeY, 0.5f, 0.5f, 0.5f, 1);
	}
}

void CViewNesPpuPalette::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewNesPpuPalette::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewNesPpuPalette::DoTap(float x, float y)
{
	LOGG("CViewNesPpuPalette::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewNesPpuPalette::DoFinishTap(float x, float y)
{
	LOGG("CViewNesPpuPalette::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewNesPpuPalette::DoDoubleTap(float x, float y)
{
	LOGG("CViewNesPpuPalette::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewNesPpuPalette::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewNesPpuPalette::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewNesPpuPalette::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewNesPpuPalette::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewNesPpuPalette::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewNesPpuPalette::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewNesPpuPalette::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewNesPpuPalette::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewNesPpuPalette::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewNesPpuPalette::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewNesPpuPalette::KeyDown: keyCode=%d", keyCode);
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewNesPpuPalette::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewNesPpuPalette::KeyUp: keyCode=%d", keyCode);
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewNesPpuPalette::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewNesPpuPalette::ActivateView()
{
	LOGG("CViewNesPpuPalette::ActivateView()");
}

void CViewNesPpuPalette::DeactivateView()
{
	LOGG("CViewNesPpuPalette::DeactivateView()");
}
