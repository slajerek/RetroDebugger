#include "CViewAtariScreen.h"
#include "CColorsTheme.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterfaceAtari.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "GAM_GamePads.h"
#include "CMainMenuBar.h"
#include "C64Tools.h"

CViewAtariScreen::CViewAtariScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	int w = 512 * debugInterface->screenSupersampleFactor;
	int h = 512 * debugInterface->screenSupersampleFactor;
	imageDataScreenDefault = new CImageData(w, h, IMG_TYPE_RGBA);
	imageDataScreenDefault->AllocImage(false, true);
	
	//	for (int x = 0; x < w; x++)
	//	{
	//		for (int y = 0; y < h; y++)
	//		{
	//			imageDataScreen->SetPixelResultRGBA(x, y, x % 255, y % 255, 0, 255);
	//		}
	//	}
	
	
	imageScreenDefault = new CSlrImage(true, true);
	imageScreenDefault->LoadImage(imageDataScreenDefault, RESOURCE_PRIORITY_STATIC, false);
	imageScreenDefault->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	imageScreenDefault->resourcePriority = RESOURCE_PRIORITY_STATIC;
	VID_PostImageBinding(imageScreenDefault, NULL);
	
	imageScreen = imageScreenDefault;
	
	// atari screen texture boundaries
	screenTexEndX = (float)debugInterface->GetScreenSizeX() / 512.0f;
	screenTexEndY = 1.0f - (float)debugInterface->GetScreenSizeY() / 512.0f;

	// zoomed screen
	this->zoomedScreenLevel = c64SettingsScreenRasterViewfinderScale;
	this->showZoomedScreen = false;
	
	this->showGridLines = false;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
	
//	/// debug
//	int rasterNum = 0x003A;
//	CBreakpointAddr *addrBreakpoint = new CBreakpointAddr(rasterNum);
//	debugInterface->breakpointsC64Raster[rasterNum] = addrBreakpoint;
//	
//	debugInterface->breakOnC64Raster = true;

	InitRasterColorsFromScheme();
}

void CViewAtariScreen::SetSupersampleFactor(int supersampleFactor)
{
	guiMain->LockMutex();
	debugInterface->LockRenderScreenMutex();
	
	debugInterface->SetSupersampleFactor(supersampleFactor);
	
	CImageData *screenData = debugInterface->GetScreenImageData();
	this->imageScreen->RefreshImageParameters(screenData,  RESOURCE_PRIORITY_STATIC, false);
	VID_PostImageBinding(imageScreenDefault, NULL);
	debugInterface->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
}


CViewAtariScreen::~CViewAtariScreen()
{
}

void CViewAtariScreen::InitRasterColorsFromScheme()
{
	// grid lines
	GetColorsFromScheme(c64SettingsScreenGridLinesColorScheme, 0.0f,
						 &gridLinesColorR, &gridLinesColorG, &gridLinesColorB);
	
	gridLinesColorA = c64SettingsScreenGridLinesAlpha;

	// raster long screen line
	GetColorsFromScheme(c64SettingsScreenRasterCrossLinesColorScheme, 0.0f,
						 &rasterLongScrenLineR, &rasterLongScrenLineG, &rasterLongScrenLineB);
	rasterLongScrenLineA = c64SettingsScreenRasterCrossLinesAlpha;
	
	//c64SettingsScreenRasterCrossAlpha = 0.85

	// exterior
	GetColorsFromScheme(c64SettingsScreenRasterCrossExteriorColorScheme, 0.1f,
						 &rasterCrossExteriorR, &rasterCrossExteriorG, &rasterCrossExteriorB);
	
	rasterCrossExteriorA = 0.8235f * c64SettingsScreenRasterCrossAlpha;		// 0.7
	
	// tip
	GetColorsFromScheme(c64SettingsScreenRasterCrossTipColorScheme, 0.1f,
						 &rasterCrossEndingTipR, &rasterCrossEndingTipG, &rasterCrossEndingTipB);
	
	rasterCrossEndingTipA = 0.1765f * c64SettingsScreenRasterCrossAlpha;	// 0.15
	
	// white interior cross
	GetColorsFromScheme(c64SettingsScreenRasterCrossInteriorColorScheme, 0.1f,
						 &rasterCrossInteriorR, &rasterCrossInteriorG, &rasterCrossInteriorB);

	rasterCrossInteriorA = c64SettingsScreenRasterCrossAlpha;	// 0.85
	
}

void CViewAtariScreen::KeyUpModifierKeys(bool isShift, bool isAlt, bool isControl)
{
	debugInterface->LockIoMutex();
	if (isShift)
	{
		debugInterface->KeyboardUp(MTKEY_LSHIFT);
		debugInterface->KeyboardUp(MTKEY_RSHIFT);
	}
	if (isAlt)
	{
		debugInterface->KeyboardUp(MTKEY_LALT);
		debugInterface->KeyboardUp(MTKEY_RALT);
	}
	if (isControl)
	{
		debugInterface->KeyboardUp(MTKEY_LCONTROL);
		debugInterface->KeyboardUp(MTKEY_RCONTROL);
	}
	debugInterface->UnlockIoMutex();
}


void CViewAtariScreen::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewAtariScreen::RefreshScreen()
{
//	LOGD("CViewAtariScreen::RefreshScreen");
	
	debugInterface->LockRenderScreenMutex();
	
	// refresh texture of Atari's screen
	CImageData *screen = debugInterface->GetScreenImageData();
	
#if !defined(FINAL_RELEASE)
	if (screen == NULL)
	{
		//LOGError("CViewAtariScreen::RefreshScreen: screen is NULL!");
		debugInterface->UnlockRenderScreenMutex();
		return;
	}
#endif
	
	imageScreen->ReBindImageData(screen);
	
	debugInterface->UnlockRenderScreenMutex();
}

void CViewAtariScreen::RenderImGui()
{
//	LOGD("CViewAtariScreen::RenderImGui");
	float w = (float)viewC64->debugInterfaceAtari->GetScreenSizeX();
	float h = (float)viewC64->debugInterfaceAtari->GetScreenSizeY();
	this->SetKeepAspectRatio(true, w/h);

	PreRenderImGui();
	Blit(imageScreen, posX, posY, -1, sizeX, sizeX, 0.0, 0.0, screenTexEndX, screenTexEndX);
	PostRenderImGui();
}


void CViewAtariScreen::Render()
{
//	LOGD("CViewAtariScreen::Render");
	// render texture of Atari's screen
	
//	BlitFilledRectangle(posX, posY, -1, 50, 50, 1.0f, 0.0f, 0, 1);

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
	}
	
	Blit(imageScreen,
		 posX,
		 posY, -1,
		 sizeX,
		 sizeY,
		 0.0f, 1.0f, screenTexEndX, screenTexEndY);
	
	if (showGridLines)
	{
		// raster screen in hex:
		// startx = 68 (88) endx = 1e8 (1c8)
		// starty = 10 (32) endy = 120 ( fa)

		float cys = posY + (float)0x0010 * rasterScaleFactorY  + rasterCrossOffsetY;
		float cye = posY + (float)0x0120 * rasterScaleFactorY  + rasterCrossOffsetY;

		float cxs = posX + (float)0x0068 * rasterScaleFactorX  + rasterCrossOffsetX;
		float cxe = posX + (float)0x01E8 * rasterScaleFactorX  + rasterCrossOffsetX;

		
		// vertical lines
		for (float rasterX = 103.5f; rasterX < 0x01e8; rasterX += 0x08)
		{
			float cx = posX + (float)rasterX * rasterScaleFactorX  + rasterCrossOffsetX;
			
			BlitLine(cx, cys, cx, cye, -1,
					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
		}
		
		// horizontal lines
		for (float rasterY = 18.5f; rasterY < 0x0120; rasterY += 0x08)
		{
			float cy = posY + (float)rasterY * rasterScaleFactorY  + rasterCrossOffsetY;
			
			BlitLine(cxs, cy, cxe, cy, -1,
					 gridLinesColorR, gridLinesColorG, gridLinesColorB, gridLinesColorA);
		}
		
		
//		float cx = posX + (float)rasterX * rasterScaleFactorX  + rasterCrossOffsetX;
//		float cy = posY + (float)rasterY * rasterScaleFactorY  + rasterCrossOffsetY;
	}
	
}

void CViewAtariScreen::SetZoomedScreenPos(float zoomedScreenPosX, float zoomedScreenPosY, float zoomedScreenSizeX, float zoomedScreenSizeY)
{
	this->zoomedScreenPosX = zoomedScreenPosX;
	this->zoomedScreenPosY = zoomedScreenPosY;
	this->zoomedScreenSizeX = zoomedScreenSizeX;
	this->zoomedScreenSizeY = zoomedScreenSizeY;
	
	this->zoomedScreenCenterX = zoomedScreenPosX + zoomedScreenSizeX/2.0f;
	this->zoomedScreenCenterY = zoomedScreenPosY + zoomedScreenSizeY/2.0f;

	this->SetZoomedScreenLevel(this->zoomedScreenLevel);
	
}

void CViewAtariScreen::SetZoomedScreenLevel(float zoomedScreenLevel)
{
	this->zoomedScreenLevel = zoomedScreenLevel;
	
	zoomedScreenImageSizeX = (float)debugInterface->GetScreenSizeX() * zoomedScreenLevel;
	zoomedScreenImageSizeY = (float)debugInterface->GetScreenSizeY() * zoomedScreenLevel;

	zoomedScreenRasterScaleFactorX = zoomedScreenImageSizeX / (float)384; //debugInterface->GetC64ScreenSizeX();
	zoomedScreenRasterScaleFactorY = zoomedScreenImageSizeY / (float)272; //debugInterface->GetC64ScreenSizeY();
	zoomedScreenRasterOffsetX =  -103.787 * zoomedScreenRasterScaleFactorX;
	zoomedScreenRasterOffsetY = -15.500 * zoomedScreenRasterScaleFactorY;
	
}

void CViewAtariScreen::CalcZoomedScreenTextureFromRaster(int rasterX, int rasterY)
{
	float ttrx = (float)rasterX * zoomedScreenRasterScaleFactorX + zoomedScreenRasterOffsetX;
	float ttry = (float)rasterY * zoomedScreenRasterScaleFactorY + zoomedScreenRasterOffsetY;

	zoomedScreenImageStartX = zoomedScreenCenterX - ttrx;
	zoomedScreenImageStartY = zoomedScreenCenterY - ttry;
}


void CViewAtariScreen::RenderZoomedScreen(int rasterX, int rasterY)
{
	CalcZoomedScreenTextureFromRaster(rasterX, rasterY);
	
	VID_SetClipping(zoomedScreenPosX, zoomedScreenPosY, zoomedScreenSizeX, zoomedScreenSizeY);
	
	//LOGD("x=%6.2f %6.2f  y=%6.2f y=%6.2f", zoomedScreenImageStartX, zoomedScreenImageStartY, zoomedScreenImageSizeX, zoomedScreenImageSizeY);

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
	}
	
	Blit(imageScreen,
		 zoomedScreenImageStartX,
		 zoomedScreenImageStartY, -1,
		 zoomedScreenImageSizeX,
		 zoomedScreenImageSizeY,
		 0.0f, 1.0f, screenTexEndX, screenTexEndY);
	
	// clipping
	BlitRectangle(zoomedScreenPosX, zoomedScreenPosY, -1, zoomedScreenSizeX, zoomedScreenSizeY, 0.0f, 1.0f, 1.0f, 1.0f);
	
	
	float rs = 0.3f;
	float rs2 = rs*2.0f;
	BlitFilledRectangle(zoomedScreenCenterX - rs, zoomedScreenPosY, -1, rs2, zoomedScreenSizeY,
						rasterLongScrenLineR, rasterLongScrenLineG, rasterLongScrenLineB, rasterLongScrenLineA);
	BlitFilledRectangle(zoomedScreenPosX, zoomedScreenCenterY, -1, zoomedScreenSizeX, rs2,
						rasterLongScrenLineR, rasterLongScrenLineG, rasterLongScrenLineB, rasterLongScrenLineA);
	
	VID_ResetClipping();
	
	if (this->hasFocus)
	{
		this->RenderFocusBorder();
	}
}


void CViewAtariScreen::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);

	UpdateRasterCrossFactors();
}

void CViewAtariScreen::UpdateRasterCrossFactors()
{
//	LOGTODO("CViewAtariScreen::UpdateRasterCrossFactors");
	
	/*
	if (viewC64->debugInterface->GetEmulatorType() == C64_EMULATOR_VICE)
	{
		if (debugInterface->GetC64MachineType() == C64_MACHINE_PAL)
		{
			/// PAL
			this->rasterScaleFactorX = sizeX / (float)384; //debugInterface->GetC64ScreenSizeX();
			this->rasterScaleFactorY = sizeY / (float)272; //debugInterface->GetC64ScreenSizeY();
//			rasterCrossOffsetX =  -71.787 * rasterScaleFactorX;
			rasterCrossOffsetX =  -103.787 * rasterScaleFactorX;
			rasterCrossOffsetY = -15.500 * rasterScaleFactorY;
		}
		else if (debugInterface->GetC64MachineType() == C64_MACHINE_NTSC)
		{
			/// NTSC uses the same framebuffer
			this->rasterScaleFactorX = sizeX / (float)384; //debugInterface->GetC64ScreenSizeX();
			this->rasterScaleFactorY = sizeY / (float)272; //debugInterface->GetC64ScreenSizeY();
			rasterCrossOffsetX =  -103.787 * rasterScaleFactorX;
			rasterCrossOffsetY = -15.500 * rasterScaleFactorY;
		}
	}
	
	rasterCrossWidth = 1.0f;
	rasterCrossWidth2 = rasterCrossWidth/2.0f;
	
	rasterCrossSizeX = 25.0f * rasterScaleFactorX;
	rasterCrossSizeY = 25.0f * rasterScaleFactorY;
	rasterCrossSizeX2 = rasterCrossSizeX/2.0f;
	rasterCrossSizeY2 = rasterCrossSizeY/2.0f;
	rasterCrossSizeX3 = rasterCrossSizeX/3.0f;
	rasterCrossSizeY3 = rasterCrossSizeY/3.0f;
	rasterCrossSizeX4 = rasterCrossSizeX/4.0f;
	rasterCrossSizeY4 = rasterCrossSizeY/4.0f;
	rasterCrossSizeX6 = rasterCrossSizeX/6.0f;
	rasterCrossSizeY6 = rasterCrossSizeY/6.0f;
	
	rasterCrossSizeX34 = rasterCrossSizeX2+rasterCrossSizeX4;
	rasterCrossSizeY34 = rasterCrossSizeY2+rasterCrossSizeY4;
	 */
}



void CViewAtariScreen::RenderRaster(int rasterX, int rasterY)
{
//	static int min = 999999;
//	static int max = 0;
//
//	if (rasterX > max)
//	{
//		max = rasterX;
//	}
//	
//	if (rasterX < min)
//	{
//		min = rasterX;
//	}
	//	LOGD("min=%d max=%d   sfx=%3.2f sfy=%3.2f", min, max, rasterScaleFactorX, rasterScaleFactorY);
	
	
	float cx = posX + (float)rasterX * rasterScaleFactorX  + rasterCrossOffsetX;
	float cy = posY + (float)rasterY * rasterScaleFactorY  + rasterCrossOffsetY;
	

	/// long screen line
	BlitFilledRectangle(posX, cy - rasterCrossWidth2, posZ, sizeX, rasterCrossWidth,
						rasterLongScrenLineR, rasterLongScrenLineG, rasterLongScrenLineB, rasterLongScrenLineA);
	BlitFilledRectangle(cx - rasterCrossWidth2, posY, posZ, rasterCrossWidth, sizeY,
						rasterLongScrenLineR, rasterLongScrenLineG, rasterLongScrenLineB, rasterLongScrenLineA);

	// red cross
	BlitFilledRectangle(cx - rasterCrossWidth2, cy - rasterCrossSizeY2, posZ, rasterCrossWidth, rasterCrossSizeY,
						rasterCrossExteriorR, rasterCrossExteriorG, rasterCrossExteriorB, rasterCrossExteriorA);
	BlitFilledRectangle(cx - rasterCrossSizeX2, cy - rasterCrossWidth2, posZ, rasterCrossSizeX, rasterCrossWidth,
						rasterCrossExteriorR, rasterCrossExteriorG, rasterCrossExteriorB, rasterCrossExteriorA);

	// cross ending tip
	BlitFilledRectangle(cx - rasterCrossWidth2, cy - rasterCrossSizeY34, posZ, rasterCrossWidth, rasterCrossSizeY4,
						rasterCrossEndingTipR, rasterCrossEndingTipG, rasterCrossEndingTipB, rasterCrossEndingTipA);
	BlitFilledRectangle(cx - rasterCrossWidth2, cy + rasterCrossSizeY2, posZ, rasterCrossWidth, rasterCrossSizeY4,
						rasterCrossEndingTipR, rasterCrossEndingTipG, rasterCrossEndingTipB, rasterCrossEndingTipA);
	BlitFilledRectangle(cx - rasterCrossSizeX34, cy - rasterCrossWidth2, posZ, rasterCrossSizeX4, rasterCrossWidth,
						rasterCrossEndingTipR, rasterCrossEndingTipG, rasterCrossEndingTipB, rasterCrossEndingTipA);
	BlitFilledRectangle(cx + rasterCrossSizeX2, cy - rasterCrossWidth2, posZ, rasterCrossSizeX4, rasterCrossWidth,
						rasterCrossEndingTipR, rasterCrossEndingTipG, rasterCrossEndingTipB, rasterCrossEndingTipA);

	// white interior cross
	BlitFilledRectangle(cx - rasterCrossWidth2, cy - rasterCrossSizeY6, posZ, rasterCrossWidth, rasterCrossSizeY3,
						rasterCrossInteriorR, rasterCrossInteriorG, rasterCrossInteriorB, rasterCrossInteriorA);
	BlitFilledRectangle(cx - rasterCrossSizeX6, cy - rasterCrossWidth2, posZ, rasterCrossSizeX3, rasterCrossWidth,
						rasterCrossInteriorR, rasterCrossInteriorG, rasterCrossInteriorB, rasterCrossInteriorA);
	
}


void CViewAtariScreen::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewAtariScreen::DoTap(float x, float y)
{
	LOGG("CViewAtariScreen::DoTap:  x=%f y=%f", x, y);
	return CGuiView::DoTap(x, y);
}

bool CViewAtariScreen::DoFinishTap(float x, float y)
{
	LOGG("CViewAtariScreen::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewAtariScreen::DoDoubleTap(float x, float y)
{
	LOGG("CViewAtariScreen::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewAtariScreen::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewAtariScreen::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewAtariScreen::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewAtariScreen::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewAtariScreen::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewAtariScreen::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewAtariScreen::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewAtariScreen::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewAtariScreen::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewAtariScreen::DoGamePadButtonDown(CGamePad *gamePad, u8 button)
{
	LOGD("CViewAtariScreen::DoGamePadButtonDown: %s %d", gamePad->name, button);
	if (gamePad->index == viewC64->mainMenuBar->selectedJoystick1-2)
	{
		debugInterface->JoystickDown(0, ConvertSdlAxisToJoystickAxis(button));
	}
	if (gamePad->index == viewC64->mainMenuBar->selectedJoystick2-2)
	{
		debugInterface->JoystickDown(1, ConvertSdlAxisToJoystickAxis(button));
	}
	return false;
}

bool CViewAtariScreen::DoGamePadButtonUp(CGamePad *gamePad, u8 button)
{
	LOGD("CViewAtariScreen::DoGamePadButtonUp: %s %d", gamePad->name, button);
	if (gamePad->index == viewC64->mainMenuBar->selectedJoystick1-2)
	{
		debugInterface->JoystickUp(0, ConvertSdlAxisToJoystickAxis(button));
	}
	if (gamePad->index == viewC64->mainMenuBar->selectedJoystick2-2)
	{
		debugInterface->JoystickUp(1, ConvertSdlAxisToJoystickAxis(button));
	}
	return false;
}

bool CViewAtariScreen::DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value)
{
//	LOGD("CViewAtariScreen::DoGamePadAxisMotion");
	return false;
}

int CViewAtariScreen::GetJoystickAxis(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// because Windows is totally messed up with right-Alt key, let's compare only keyCodes
	if (viewC64->mainMenuBar->kbsJoystickFire->keyCode == keyCode)
	{
		return JOYPAD_FIRE;
	}
	if (viewC64->mainMenuBar->kbsJoystickUp->keyCode == keyCode)
	{
		return JOYPAD_N;
	}
	if (viewC64->mainMenuBar->kbsJoystickDown->keyCode == keyCode)
	{
		return JOYPAD_S;
	}
	if (viewC64->mainMenuBar->kbsJoystickLeft->keyCode == keyCode)
	{
		return JOYPAD_E;	// TODO: fix me?
	}
	if (viewC64->mainMenuBar->kbsJoystickRight->keyCode == keyCode)
	{
		return JOYPAD_W;	// TODO: fix me?
	}
	return JOYPAD_IDLE;

	/*
	if (c64SettingsUseKeyboardAsJoystick)
	{
		// workaround for fire (eg. fire+up = ALT+UP)
		CSlrKeyboardShortcut *shortcut = NULL;
		
		LOGD("CViewAtariScreen::GetJoystickAxis: (shortcut=pressed) keyCode: %08x=%08x isShift: %d=%d isAlt: %d=%d isControl: %d=%d",
			 viewC64->keyboardShortcuts->kbsJoystickFire->keyCode, keyCode,
			 viewC64->keyboardShortcuts->kbsJoystickFire->isShift, isShift,
			 viewC64->keyboardShortcuts->kbsJoystickFire->isAlt, isAlt,
			 viewC64->keyboardShortcuts->kbsJoystickFire->isControl, isControl);
		
		if (viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == keyCode
			&& viewC64->keyboardShortcuts->kbsJoystickFire->isShift == isShift
			&& viewC64->keyboardShortcuts->kbsJoystickFire->isAlt == isAlt
			&& viewC64->keyboardShortcuts->kbsJoystickFire->isControl == isControl)
		{
			return JOYPAD_FIRE;
		}
		else
		{
			if (viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == MTKEY_LALT
				|| viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == MTKEY_RALT)
			{
				isAlt = false;
			}
			else if (viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == MTKEY_LCONTROL
					 || viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == MTKEY_RCONTROL)
			{
				isControl = false;
			}
			else if (viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == MTKEY_LSHIFT
					 || viewC64->keyboardShortcuts->kbsJoystickFire->keyCode == MTKEY_RSHIFT)
			{
				isShift = false;
			}
			
			shortcut = viewC64->keyboardShortcuts->FindShortcut(KBZONE_SCREEN, keyCode, isShift, isAlt, isControl);
		}
		
		if (shortcut != NULL)
		{
			if (shortcut == viewC64->keyboardShortcuts->kbsJoystickUp)
			{
				return JOYPAD_N;
			}
			else if (shortcut == viewC64->keyboardShortcuts->kbsJoystickDown)
			{
				return JOYPAD_S;
			}
			else if (shortcut == viewC64->keyboardShortcuts->kbsJoystickLeft)
			{
				return JOYPAD_E;
			}
			else if (shortcut == viewC64->keyboardShortcuts->kbsJoystickRight)
			{
				return JOYPAD_W;
			}
			else if (shortcut == viewC64->keyboardShortcuts->kbsJoystickFire)
			{
				return JOYPAD_FIRE;
			}
		}
	}
	
	return JOYPAD_IDLE;
	 */
}

bool CViewAtariScreen::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewAtariScreen::KeyDown: keyCode=%d", keyCode);
	
	if (keyCode == MTKEY_ENTER && isAlt)
	{
		viewC64->ToggleFullScreen(this);
		return true;
	}
	
	if (viewC64->mainMenuBar->selectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard
		|| viewC64->mainMenuBar->selectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
	{
		int joyAxis = GetJoystickAxis(keyCode, isShift, isAlt, isControl, isSuper);
		if (joyAxis != JOYPAD_IDLE)
		{
			debugInterface->LockIoMutex();
			if (viewC64->mainMenuBar->selectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickDown(0, joyAxis);
			}
			if (viewC64->mainMenuBar->selectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickDown(1, joyAxis);
			}
			debugInterface->UnlockIoMutex();
			return true;
		}
	}

	//	u32 bareKey = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);

	debugInterface->LockIoMutex();
	
	/*
	std::map<u32, bool>::iterator it = pressedKeyCodes.find(keyCode);

	if (it == pressedKeyCodes.end())
	{
		pressedKeyCodes[keyCode] = true;
	}
	else
	{
		// key is already pressed
		LOGD("key %d is already pressed, skipping...", keyCode);
		debugInterface->UnlockIoMutex();
		return true;
	}
	 */
	
	
//	keyCode = SYS_KeyCodeConvertSpecial(keyCode, isShift, isAlt, isControl, isSuper);
//	LOGI(".........SYS_KeyCodeConvertSpecial converted key is ", keyCode);
	
	
	
	
	debugInterface->KeyboardDown(keyCode); //bareKey);
	debugInterface->UnlockIoMutex();
	return true;
	
	//return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewAtariScreen::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewAtariScreen::KeyUp: keyCode=%d", keyCode);

	if (viewC64->mainMenuBar->selectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard
		|| viewC64->mainMenuBar->selectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
	{
		int joyAxis = GetJoystickAxis(keyCode, isShift, isAlt, isControl, isSuper);
		if (joyAxis != JOYPAD_IDLE)
		{
			debugInterface->LockIoMutex();
			if (viewC64->mainMenuBar->selectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickUp(0, joyAxis);
			}
			if (viewC64->mainMenuBar->selectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickUp(1, joyAxis);
			}
			debugInterface->UnlockIoMutex();
			return true;
		}
	}

	//	u32 bareKey = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);

	debugInterface->LockIoMutex();
	
	/*
	std::map<u32, bool>::iterator it = pressedKeyCodes.find(keyCode);
	
	if (it == pressedKeyCodes.end())
	{
		// key is already not pressed
		LOGD("key %d is already not pressed, skipping...", keyCode);
		debugInterface->UnlockIoMutex();
		
		return true;
	}
	else
	{
		pressedKeyCodes.erase(it);
	}
	 */
	 
//	keyCode = SYS_KeyCodeConvertSpecial(keyCode, isShift, isAlt, isControl, isSuper);
//	LOGI(".........SYS_KeyCodeConvertSpecial converted key is ", keyCode);

	debugInterface->KeyboardUp(keyCode); //bareKey);
	debugInterface->UnlockIoMutex();
	
	return true;

//	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewAtariScreen::JoystickDown(int port, u32 axis)
{
	LOGD("CViewAtariScreen::JoystickDown: axis=%02x", axis);
	debugInterface->LockIoMutex();
	debugInterface->JoystickDown(port, axis);
	debugInterface->UnlockIoMutex();
}

void CViewAtariScreen::JoystickUp(int port, u32 axis)
{
	debugInterface->LockIoMutex();
	debugInterface->JoystickUp(port, axis);
	debugInterface->UnlockIoMutex();
}

bool CViewAtariScreen::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewAtariScreen::ActivateView()
{
	LOGG("CViewAtariScreen::ActivateView()");
}

void CViewAtariScreen::DeactivateView()
{
	LOGG("CViewAtariScreen::DeactivateView()");
}
