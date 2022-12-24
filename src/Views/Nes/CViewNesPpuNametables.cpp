#include "NstApiMachine.hpp"
#include "NstMachine.hpp"
#include "NstApiEmulator.hpp"
#include "NstCpu.hpp"
#include "NstPpu.hpp"

#include "CViewNesPpuNametables.h"
#include "CColorsTheme.h"
#include "GUI_Main.h"
#include "CGuiMain.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterfaceNes.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "CViewDataDump.h"

extern Nes::Api::Emulator nesEmulator;

CViewNesPpuNametables::CViewNesPpuNametables(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	int w = 512; // * debugInterface->screenSupersampleFactor;
	int h = 512; // * debugInterface->screenSupersampleFactor;
	imageData = new CImageData(w, h, IMG_TYPE_RGBA);
	imageData->AllocImage(false, true);
	
	//	for (int x = 0; x < w; x++)
	//	{
	//		for (int y = 0; y < h; y++)
	//		{
	//			imageDataScreen->SetPixelResultRGBA(x, y, x % 255, y % 255, 0, 255);
	//		}
	//	}
	
	
	imageScreenDefault = new CSlrImage(true, false);
	imageScreenDefault->LoadImage(imageData, RESOURCE_PRIORITY_STATIC, false);
	imageScreenDefault->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	imageScreenDefault->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(imageScreenDefault, NULL);
	
	imageScreen = imageScreenDefault;
	
	// screen texture boundaries
	screenTexEndX = (float)debugInterface->GetScreenSizeX() / 512.0f;
	screenTexEndY = 1.0f - (float)debugInterface->GetScreenSizeY() / 512.0f;

	showGridLines = true;
	isLockedCursor = false;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}


CViewNesPpuNametables::~CViewNesPpuNametables()
{
}

void CViewNesPpuNametables::DoLogic()
{
	CGuiView::DoLogic();
}

u8 CViewNesPpuNametables::GetChr(int nameTableAddr, int x, int y)
{
//	if (MMC5Hack) {
//					if (MMC5HackCHRMode == 1) {
//							uint8 *C = MMC5HackVROMPTR;
//							C += (((MMC5HackExNTARAMPtr[refreshaddr & 0x3ff]) & 0x3f & MMC5HackVROMMask) << 12) + (vadr & 0x
//							C += (MMC50x5130 & 0x3) << 18;  //11-jun-2009 for kuja_killer
//							return C;
//					} else {
//							return MMC5BGVRAMADR(vadr);
//					}

	
	Nes::Core::Machine& machine = nesEmulator;
	int chrAddr = (y * 32) + x;
	return machine.ppu.nmt.Peek(nameTableAddr + chrAddr);
}

int CViewNesPpuNametables::GetAttr(int nameTableAddr, int x, int y)
{
//	int refreshaddr = xt + yt * 32;
//			if (MMC5Hack && MMC5HackCHRMode == 1)
//					return (MMC5HackExNTARAMPtr[refreshaddr & 0x3ff] & 0xC0) >> 6;
//			else
//					return (vnapage[ntnum][attraddr] & (3 << temp)) >> temp;

	int attrAddr = 0x03C0 + ((y >> 2) << 3) + (x >> 2);
	
	Nes::Core::Machine& machine = nesEmulator;
	u8 attrVal = machine.ppu.nmt.Peek(nameTableAddr + attrAddr);

	int temp = ((y & 2) << 1) + (x & 2);
	return (attrVal & (3 << temp)) >> temp;
}


// NOTE: this code below is based on FCEUX DrawNameTable code for ppu viewer
void CViewNesPpuNametables::RefreshScreen()
{
//	LOGD("CViewNesPpuNametables::RefreshScreen");
	// TODO: debugInterface->screenSupersampleFactor
	//(float)debugInterface->GetScreenSizeX()

	
	Nes::Core::Machine &machine = nesEmulator;
	Nes::Core::Video::Screen screen = machine.ppu.GetScreen();

//	*value = machine.ppu.nmt.Peek(pointer); // + 0x2000);

	int chrTableAddr = 0;

	if (machine.ppu.regs.ctrl[0] & 0x10)
	{
		// use the correct pattern table based on this bit
		chrTableAddr = 0x1000;
	}
	
	int nameTableNum = 0;
	int nameTableAddr = nameTableNum * 0x0400;
	
	for (int y = 0; y < 30; y++)
	{
		for (int x = 0; x < 32; x++)
		{
			int attr = GetAttr(nameTableAddr, x, y);
			int chr = GetChr(nameTableAddr, x, y);
			
			int chrAddr = chrTableAddr + chr * 16;

			int index=0;

			for (int cy =0; cy < 8; cy++)
			{
				u8 chr0 = machine.ppu.chr.Peek(chrAddr + index);
				u8 chr1 = machine.ppu.chr.Peek(chrAddr + index + 8);
				int tmp = 7;
				
				for (int cx = 0; cx < 8; cx++)
				{
					int p  =  (chr0 >> tmp) & 1;
					    p |= ((chr1 >> tmp) & 1) << 1;
					
					int pp = p + (attr*4);
					int chrPixelColorIndex = machine.ppu.palette.ram[pp];
					tmp--;

					u32 rgba = screen.palette[chrPixelColorIndex];

					u8 r = (rgba & 0x000000FF)                   ;
					u8 g = (rgba & 0x0000FF00) >> 8  & 0x000000FF;
					u8 b = (rgba & 0x00FF0000) >> 16 & 0x000000FF;
					
					imageData->SetPixelResultRGBA(x*8 + cx, y*8 + cy, r, g, b, 255);
				}
				index++;
			}
		}
	}
	
//	adahidhifoasfhiods
//
//
//	for (int y = 0; y < 512; y++)
//	{
//		for (int x = 0; x < 512; x++)
//		{
//			imageData->SetPixelResultRGBA(x, y, x%256, y%256, 0, 255);
//		}
//	}
}

void CViewNesPpuNametables::RenderImGui()
{
//	LOGD("CViewNesPpuNametables::RenderImGui");
	float w = (float)viewC64->debugInterfaceNes->GetScreenSizeX();
	float h = (float)viewC64->debugInterfaceNes->GetScreenSizeY();
//	this->imGuiWindowAspectRatio = w/h;
//	this->imGuiWindowKeepAspectRatio = true;
//	
	this->SetKeepAspectRatio(true, w/h);

	RefreshScreen();
	imageScreen->ReBindImageData(imageData);
	
	PreRenderImGui();
	Blit(imageScreen, posX, posY, -1, sizeX, sizeX, 0.0, 0.0, screenTexEndX, screenTexEndX);
	PostRenderImGui();
}

void CViewNesPpuNametables::Render()
{
//	LOGD("CViewNesPpuNmt::Render");
	// render texture of Nes screen
	
//	BlitFilledRectangle(posX, posY, -1, 50, 50, 1.0f, 0.0f, 0, 1);

	RefreshScreen();
	
	imageScreen->ReBindImageData(imageData);

	/*
	if (c64SettingsRenderScreenNearest)
	{
		// nearest neighbour
		{
			glBindTexture(GL_TEXTURE_2D, imageScreen->textureId);
			
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		}
	}
	else
	{
		// billinear interpolation
		{
			glBindTexture(GL_TEXTURE_2D, imageScreen->textureId);
			
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		}
	}*/
	
//	BlitFilledRectangle(posX, posY, -1, sizeX, sizeY, 1, 0, 0.5, 1);
	
	Blit(imageScreen,
		 posX,
		 posY, -1,
		 sizeX,
		 sizeY,
		 0.0f, 1.0f, screenTexEndX, screenTexEndY);
	
//	if (showGridLines)
//	{
//		// raster screen in hex:
//		// startx = 68 (88) endx = 1e8 (1c8)
//		// starty = 10 (32) endy = 120 ( fa)
//
//		float cys = posY + (float)0x0010 * rasterScaleFactorY  + rasterCrossOffsetY;
//		float cye = posY + (float)0x0120 * rasterScaleFactorY  + rasterCrossOffsetY;
//
//		float cxs = posX + (float)0x0068 * rasterScaleFactorX  + rasterCrossOffsetX;
//		float cxe = posX + (float)0x01E8 * rasterScaleFactorX  + rasterCrossOffsetX;
//
//
//		// vertical lines
//		for (float rasterX = 103.5f; rasterX < 0x01e8; rasterX += 0x08)
//		{
//			float cx = posX + (float)rasterX * rasterScaleFactorX  + rasterCrossOffsetX;
//
//			BlitLine(cx, cys, cx, cye, -1,
//					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
//		}
//
//		// horizontal lines
//		for (float rasterY = 18.5f; rasterY < 0x0120; rasterY += 0x08)
//		{
//			float cy = posY + (float)rasterY * rasterScaleFactorY  + rasterCrossOffsetY;
//
//			BlitLine(cxs, cy, cxe, cy, -1,
//					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
//		}
//	}
	
}

void CViewNesPpuNametables::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewNesPpuNametables::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewNesPpuNametables::DoTap(float x, float y)
{
	isLockedCursor = !isLockedCursor;
	
	return CGuiView::DoTap(x, y);
}
bool CViewNesPpuNametables::DoNotTouchedMove(float x, float y)
{
//	LOGG("CViewNesPpuNmt::DoNotTouchedMove:  x=%f y=%f", x, y);
	// TODO: debugInterface->screenSupersampleFactor
	//(float)debugInterface->GetScreenSizeX()

	if (IsInside(x, y) == false)
		return false;
	
	if (isLockedCursor)
		return false;
	
	float emuScreenSizeX = (float)debugInterface->GetScreenSizeX();
	float emuScreenSizeY = (float)debugInterface->GetScreenSizeY();

	float emuNumCharsX = 32.0f;
	float emuNumCharsY = 30.0f;
	
	float charSizeX = sizeX / emuNumCharsX;
	float charSizeY = sizeY / emuNumCharsY;

	float px = x-posX;
	float py = y-posY;
	
	float pcx = px / charSizeX;
	float pcy = py / charSizeY;

//	LOGD("px=%f py=%f  pcx=%f pcy=%f", px, py, pcx, pcy);
	
	int tx = pcx; int ty = pcy;
//	LOGD("tx=%d/32 ty=%d/30", tx, ty);
	
	int addr = ty * 32 + tx;
	viewC64->viewNesPpuNametableMemoryDataDump->ScrollToAddress(addr, false);
	
	return true;
}

bool CViewNesPpuNametables::DoFinishTap(float x, float y)
{
	LOGG("CViewNesPpuNmt::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewNesPpuNametables::DoDoubleTap(float x, float y)
{
	LOGG("CViewNesPpuNmt::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewNesPpuNametables::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewNesPpuNmt::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewNesPpuNametables::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewNesPpuNametables::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewNesPpuNametables::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewNesPpuNametables::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewNesPpuNametables::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewNesPpuNametables::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewNesPpuNametables::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewNesPpuNametables::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewNesPpuNmt::KeyDown: keyCode=%d", keyCode);
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewNesPpuNametables::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewNesPpuNmt::KeyUp: keyCode=%d", keyCode);
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewNesPpuNametables::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewNesPpuNametables::ActivateView()
{
	LOGG("CViewNesPpuNmt::ActivateView()");
}

void CViewNesPpuNametables::DeactivateView()
{
	LOGG("CViewNesPpuNmt::DeactivateView()");
}
