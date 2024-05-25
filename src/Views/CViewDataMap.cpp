#include "SYS_Defs.h"
#include "EmulatorsConfig.h"
#include "DBG_Log.h"
#include "CViewDataMap.h"
#include "CViewC64.h"
#include "CViewC64Screen.h"
#include "CImageData.h"
#include "CSlrImage.h"
#include "CViewC64.h"
#include "MTH_Random.h"
#include "VID_ImageBinding.h"
#include "SYS_KeyCodes.h"
#include "CViewDataDump.h"
#include "CViewDisassembly.h"
#include "CSnapshotsManager.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceVice.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "C64Opcodes.h"
#include "CGuiMain.h"
#include "CMainMenuBar.h"

#include <math.h>

////
// TODO: generalise/abstract this (i.e. remove isFromDisk param)
// TODO: use to view map of files

// generalise using adaptor to read data ("C64MemoryAdaptor", "DiskAdaptor") with GetByte returning bool SKIP to not update display
// (but will it be performant enough?)

float speedDivider = 1.0f;

float dataColorSpeed; // = 0.020 / speedDivider;
float dataAlphaSpeed; // = 0.020 / speedDivider;

float codeColorSpeed; // = 0.07 / speedDivider;
float codeAlphaSpeed; // = 0.10 / speedDivider;

void C64DebuggerSetMemoryMapCellsFadeSpeed(float fadeSpeed)
{
	speedDivider = 0.33f / fadeSpeed;

#if defined(RUN_ATARI)
	dataColorSpeed = 0.020 / speedDivider;
	dataAlphaSpeed = 0.020 / speedDivider;
	
	codeColorSpeed = 0.035 / speedDivider;
	codeAlphaSpeed = 0.05 / speedDivider;
#else
	dataColorSpeed = 0.020 / speedDivider;
	dataAlphaSpeed = 0.020 / speedDivider;
	
	codeColorSpeed = 0.07 / speedDivider;
	codeAlphaSpeed = 0.10 / speedDivider;
#endif

}

CViewDataMap::CViewDataMap(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
							   CDebugDataAdapter *dataAdapter,
							   int imageWidth, int imageHeight, bool showCurrentExecutePC)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	viewDataDump = NULL;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->dataAdapter = dataAdapter;
	this->debugInterface = dataAdapter->debugInterface;
	this->debugSymbols = dataAdapter->debugSymbols;
	this->debugMemory = debugSymbols->memory;
	
	this->showCurrentExecutePC = showCurrentExecutePC;
	this->imageWidth = imageWidth;
	this->imageHeight = imageHeight;
	this->ramSize = dataAdapter->AdapterGetDataLength();
	
	this->isBeingMoved = false;
	this->isForcedMovingMap = false;
	
	this->updateMapIsAnimateEvents = true;
	this->updateMapNumberOfFps = 25.0f;
	this->shouldRebindImage = false;
	
	this->font = viewC64->fontCBMShifted;
	this->fontScale = 0.11f;

	// alloc with safe margin to avoid comparison in cells marking (it is quicker)
	int numCells = ramSize + DATA_CELLS_PADDING_LENGTH;
;

	// local copy of memory
	this->memoryBuffer = new uint8[numCells];

	// images
	imageDataMemoryMap = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA);
	imageDataMemoryMap->AllocImage(false, true);
	
	imageMemoryMap = new CSlrImage(true, false);
	imageMemoryMap->LoadImageForRebinding(imageDataMemoryMap, RESOURCE_PRIORITY_STATIC);
	VID_PostImageBinding(imageMemoryMap, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);

	// color mapping
	C64DebuggerComputeMemoryMapColorTables(c64SettingsMemoryValuesStyle);

	// 
	frameCounter = 0;
	nextScreenUpdateFrame = 0;
	
	cursorInside = false;
	cursorX = cursorY = 0.5f;
	
	ClearZoom();
	
	previousClickAddr = -1;
	previousClickTime = 0;
	
//	/// debug
//	cursorInside = true;
//	DoZoomBy(0, 0, 230, 600);
//	////
	
//	UpdateWholeMap();
	
	/// TODO:
//	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	
	SYS_StartThread(this);
}

void CViewDataMap::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
//	LOGD("CViewMemoryMap::SetPosition");
	
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	UpdateMapPosition();
}


void CViewDataMap::DoLogic()
{

	////
}

void CViewDataMap::SetDataAdapter(CDataAdapter *newDataAdapter)
{
	this->dataAdapter = newDataAdapter;
}

void CViewDataMap::UpdateWholeMap()
{
	//LOGD("FRAMES_PER_SECOND=%d", FRAMES_PER_SECOND);
	
	int pc = 0;
	
	if (showCurrentExecutePC)
	{
		pc = debugSymbols->GetCurrentExecuteAddr();
	}

	// use bitmap
	int vx = 0;
	int vy = 0;

	// read whole map from data adapter
	int len = dataAdapter->AdapterGetDataLength();
	dataAdapter->AdapterReadBlockDirect(memoryBuffer, 0, len);
	
	int addr = 0;
	
	int index = 0;
	
	for (int y = 0; y < imageHeight; y++)
	{
		vx = 0;
		//px = startX;
		
		for (int x = 0; x < imageWidth; x++)
		{
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(index);
			addr = cell->addr;
		
			u8 v = memoryBuffer[addr];
			
			cell->UpdateCellColors(v, showCurrentExecutePC, pc);
			
			imageDataMemoryMap->SetPixelResultRGBA(vx, vy, cell->rr*255.0f, cell->rg*255.0f, cell->rb*255.0f, cell->ra*255.0f);
			
			index++;
			vx += 1;
		}
		
		vy += 1;
	}
}

// TODO: animate cells - REFACTOR: once per frame when consumers visible
void CViewDataMap::ThreadRun(void *data)
{
//	return;
	ThreadSetName("CViewMemoryMap update");
	//SYS_SetThreadPriority(LOW);

	while(true)
	{
		unsigned long lastFrameTime = SYS_GetCurrentTimeInMillis();

		double targetFPS = updateMapNumberOfFps;
		if (debugInterface->GetSettingIsWarpSpeed() == true)
		{
			targetFPS = 0.5f;
		}
		
		double targetMS = 1000.0f / targetFPS;
		
		if (debugInterface->isRunning && this->visible)
		{
			UpdateWholeMap();
		}
		
		if (this->visible ||
			(viewDataDump != NULL && viewDataDump->visible))
		{
			CellsAnimationLogic(updateMapNumberOfFps);
		}
		
		shouldRebindImage = true;
		
		unsigned long currentFrameTime = SYS_GetCurrentTimeInMillis();
		double d = (double)(currentFrameTime-lastFrameTime);
		
		if (d < targetMS)
		{
			double s = targetMS - d;
			SYS_Sleep(s);
		}
		else
		{
			SYS_Sleep(5);
		}
	}
}

void CViewDataMap::CellsAnimationLogic(double targetFPS)
{
	if (updateMapIsAnimateEvents == false)
		return;
	
	float runDataColorSpeed;
	float runDataAlphaSpeed;
	float runCodeColorSpeed;
	float runCodeAlphaSpeed;
	if (debugInterface->GetDebugMode() == DEBUGGER_MODE_PAUSED)
	{
//		runDataColorSpeed = 0.0f; //dataColorSpeed/500.0f;
//		runDataAlphaSpeed = 0.0f; //dataAlphaSpeed/500.0f;
//		runCodeColorSpeed = 0.0f; //dataColorSpeed/500.0f;
//		runCodeAlphaSpeed = 0.0f; //dataAlphaSpeed/500.0f;
		return;
	}
	else
	{
		float fpsAdjust = 25.0f / targetFPS;
		
		runDataColorSpeed = dataColorSpeed * fpsAdjust;
		runDataAlphaSpeed = dataAlphaSpeed * fpsAdjust;
		runCodeColorSpeed = codeColorSpeed * fpsAdjust;
		runCodeAlphaSpeed = codeAlphaSpeed * fpsAdjust;
	}
	
	u16 addr = 0x0000;

	float colorSpeed;
	float alphaSpeed;
	
	for (int y = 0; y < imageHeight; y++)
	{
		for (int x = 0; x < imageWidth; x++)
		{
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
			
			if (cell->isExecuteCode || cell->isExecuteArgument)
			{
				colorSpeed = runCodeColorSpeed;
				alphaSpeed = runCodeAlphaSpeed;
			}
			else
			{
				colorSpeed = runDataColorSpeed;
				alphaSpeed = runDataAlphaSpeed;
			}
			
			if (cell->sr > 0.0f)
			{
				cell->sr -= colorSpeed; if (cell->sr < 0.0f) cell->sr = 0.0f;
			}
			if (cell->sg > 0.0f)
			{
				cell->sg -= colorSpeed; if (cell->sg < 0.0f) cell->sg = 0.0f;
			}
			if (cell->sb > 0.0f)
			{
				cell->sb -= colorSpeed; if (cell->sb < 0.0f) cell->sb = 0.0f;
			}
			if (cell->sa > 0.0f)
			{
				cell->sa -= alphaSpeed; if (cell->sa < 0.0f) cell->sa = 0.0f;
			}
			
			addr++;
		}
	}
	
//	if (accelerateX > 0.0f || accelerateY > 0.0f)
//	{
//		LOGD("accelerate=%f %f", accelerateX, accelerateY);
//		accelerateX -= 2.0f;
//		accelerateY -= 2.0f;
//		
//		if (accelerateX < 0.0f)
//			accelerateX = 0.0f;
//
//		if (accelerateY < 0.0f)
//			accelerateY = 0.0f;
//
//		MoveMap(accelerateX, accelerateY);
//	}
}

/*
void CViewMemoryMap::DriveROMCellsAnimationLogic()
{
	//LOGTODO:("DriveROMCellsAnimationLogic - generalise this");
	float runDataColorSpeed;
	float runDataAlphaSpeed;
	float runCodeColorSpeed;
	float runCodeAlphaSpeed;
	if (debugInterface->GetDebugMode() == DEBUGGER_MODE_PAUSED)
	{
		runDataColorSpeed = 0.0f; //dataColorSpeed/500.0f;
		runDataAlphaSpeed = 0.0f; //dataAlphaSpeed/500.0f;
		runCodeColorSpeed = 0.0f; //dataColorSpeed/500.0f;
		runCodeAlphaSpeed = 0.0f; //dataAlphaSpeed/500.0f;
	}
	else
	{
		runDataColorSpeed = dataColorSpeed;
		runDataAlphaSpeed = dataAlphaSpeed;
		runCodeColorSpeed = codeColorSpeed;
		runCodeAlphaSpeed = codeAlphaSpeed;
	}
	
	float colorSpeed;
	float alphaSpeed;
	
	for (int addr = 0x8000; addr < 0x10000; addr++)
	{
		CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
		
		if (cell->isExecuteCode || cell->isExecuteArgument)
		{
			colorSpeed = runCodeColorSpeed;
			alphaSpeed = runCodeAlphaSpeed;
		}
		else
		{
			colorSpeed = runDataColorSpeed;
			alphaSpeed = runDataAlphaSpeed;
		}
		
		if (cell->sr > 0.0f)
		{
			cell->sr -= colorSpeed; if (cell->sr < 0.0f) cell->sr = 0.0f;
		}
		if (cell->sg > 0.0f)
		{
			cell->sg -= colorSpeed; if (cell->sg < 0.0f) cell->sg = 0.0f;
		}
		if (cell->sb > 0.0f)
		{
			cell->sb -= colorSpeed; if (cell->sb < 0.0f) cell->sb = 0.0f;
		}
		if (cell->sa > 0.0f)
		{
			cell->sa -= alphaSpeed; if (cell->sa < 0.0f) cell->sa = 0.0f;
		}
	}
}
*/

bool CViewDataMap::DoScrollWheel(float deltaX, float deltaY)
{
	LOGG("CViewMemoryMap::DoScrollWheel: %5.2f %5.2f isFromDisk=%s", deltaX, deltaY);

	if (c64SettingsUseMultiTouchInMemoryMap)
	{
		MoveMap(deltaX, deltaY);
	}
	else
	{
		if (cursorInside == false)
		{
			return false;
		}
		
		if (c64SettingsMemoryMapInvertControl)
		{
			deltaY = -deltaY;
		}
		
		currentZoom = currentZoom + deltaY / 5.0f * cellSizeX * 0.1f;
		
		if (currentZoom < 1.0f)
			currentZoom = 1.0f;
		
		if (currentZoom > 60.0f)
			currentZoom = 60.0f;
		
		this->ZoomMap(currentZoom);
		
		return true;
	}
	
	return true;
}

bool CViewDataMap::InitZoom()
{
//	LOGD("CViewMemoryMap::InitZoom");
	return true;
}

bool CViewDataMap::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	LOGD("CViewMemoryMap::DoZoomBy: x=%5.2f y=%5.2f zoomValue=%5.2f diff=%5.2f", x, y, zoomValue, difference);

	// TODO: make it working also on Drive 1541
//	if (isFromDisk)
//		return false;

	
	if (c64SettingsUseMultiTouchInMemoryMap)
	{
		if (cursorInside == false)
			return false;
		
//		if (c64SettingsMemoryMapInvertControl)
//		{
//			difference = -difference;
//		}
		
		currentZoom = currentZoom + difference / 5.0f;
		
		if (currentZoom < 1.0f)
			currentZoom = 1.0f;
		
		if (currentZoom > 60.0f)
			currentZoom = 60.0f;
		
		this->ZoomMap(currentZoom);
		
		return true;
	}
	
	return false;
}

bool CViewDataMap::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
//	LOGD("CViewMemoryMap::DoMove: x=%5.2f y=%5.2f distX=%5.2f distY=%5.2f diffX=%5.2f diffY=%5.2f",
//		 x, y, distX, distY, diffX, diffY);
	
	return true;
}

bool CViewDataMap::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
//	LOGD("CViewMemoryMap::DoMove: x=%5.2f y=%5.2f distX=%5.2f distY=%5.2f accelerationX=%5.2f accelerationY=%5.2f",
//		 x, y, distX, distY, accelerationX, accelerationY);

	return true;
}

bool CViewDataMap::DoRightClick(float x, float y)
{
	if (c64SettingsUseMultiTouchInMemoryMap == false)
	{
		if (IsInside(x, y))
		{
			this->accelerateX = 0.0f;
			this->accelerateY = 0.0f;
			isBeingMoved = true;
			return true;
		}
	}
	
	return false;
}

bool CViewDataMap::DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
//	LOGD("CViewMemoryMap::DoRightClickMove");
	if (c64SettingsUseMultiTouchInMemoryMap == false)
	{
		this->accelerateX = 0.0f;
		this->accelerateY = 0.0f;
		MoveMap(diffX / sizeX * mapSizeX * 5.0f, diffY / sizeY * mapSizeY * 5.0f);
		return true;
	}
	
	return false;
}

bool CViewDataMap::FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	if (c64SettingsUseMultiTouchInMemoryMap == false && isBeingMoved)
	{
//		this->accelerateX = accelerationX / 10.0f;
//		this->accelerateY = accelerationY / 10.0f;
		isBeingMoved = false;
		return true;
	}
	
	return false;
}


bool CViewDataMap::DoNotTouchedMove(float x, float y)
{
//	LOGG("CViewMemoryMap::DoNotTouchedMove: x=%f y=%f", x, y);
	
	if (isForcedMovingMap)
	{
		float dx = -(prevMousePosX - x);
		float dy = -(prevMousePosY - y);
		
		MoveMap(dx / sizeX * mapSizeX * 5.0f, dy / sizeY * mapSizeY * 5.0f);
		
		prevMousePosX = x;
		prevMousePosY = y;

		this->accelerateX = 0.0f;
		this->accelerateY = 0.0f;
		return true;
	}
	
	// change x, y into position within memory map area (cursorX, cursorY)
	
	float px = (x - posX) / sizeX;
	float py = (y - posY) / sizeY;
	
	if (px < 0.0f
		|| px > 1.0f
		|| py < 0.0f
		|| py > 1.0f)
	{
		cursorInside = false;
		return false;
	}
	
	cursorX = px;
	cursorY = py;
	
	cursorInside = true;
	
	return false;
}

void CViewDataMap::ClearZoom()
{
	currentZoom = 1.0f;
	textureStartX = textureStartY = 0.0f;
	textureEndX = textureEndY = 1.0f;
	UpdateTexturePosition(textureStartX, textureStartY, textureEndX, textureEndY);
	mapPosX = 0.0f;
	mapPosY = 0.0f;
	mapSizeX = 1.0f;
	mapSizeY = 1.0f;
	UpdateMapPosition();
}

void CViewDataMap::ZoomMap(float zoom)
{
//	LOGD("CViewMemoryMap::ZoomMap: %f", zoom);
	if (zoom < 1.0f)
	{
		ClearZoom();
		return;
	}
	
//	LOGD("cursorX=%f cursorY=%f", cursorX, cursorY);

	float fcx = (-mapPosX + cursorX) / mapSizeX;
//	LOGD("fcx=%f", fcx);
	
	mapSizeX = 1.0f*zoom;
	mapPosX = -fcx*mapSizeX + cursorX;
	
	float fcy = (-mapPosY + cursorY) / mapSizeY;
//	LOGD("fcy=%f", fcy);

	mapSizeY = 1.0f*zoom;
	mapPosY = -fcy*mapSizeY + cursorY;

	UpdateMapPosition();	
}

void CViewDataMap::MoveMap(float diffX, float diffY)
{
	mapPosX += diffX*0.2f / mapSizeX;
	mapPosY += diffY*0.2f / mapSizeY;
	
	UpdateMapPosition();
}

// NOTE: textureStart/End is deprecated and is always displaying full texture (start=0, end=1)
void CViewDataMap::UpdateTexturePosition(float newStartX, float newStartY, float newEndX, float newEndY)
{
	this->renderTextureStartX = newStartX;
	this->renderTextureEndX = newEndX;
	
//	if (isFromDisk == false)
//	{
	
		this->renderTextureStartY = newEndY;
		this->renderTextureEndY = newStartY;
	
//	}
//	else
//	{
//		// TODO: make this general, now temporary display only part of texture when memory is from disk
//		
//		// from 1.0 to 0.965
//		const float d = 0.035f;
//		
//		float ty1 = newStartY * d + 0.965f;
//		float ty2 = newEndY * d + 0.965f;
//		
//		this->renderTextureStartY = ty2;
//		this->renderTextureEndY = ty1;
//	}
}

void CViewDataMap::UpdateMapPosition()
{
	// convert (0.0, 1.0) to screen pos/size
	
	if (mapPosX > 0.0f)
		mapPosX = 0.0f;
	
	if (mapPosY > 0.0f)
		mapPosY = 0.0f;
	
	if (mapPosX - 1.0f < -mapSizeX)
		mapPosX = -mapSizeX+1.0f;
	
	if (mapPosY - 1.0f < -mapSizeY)
		mapPosY = -mapSizeY+1.0f;
	
	
	guiMain->LockMutex(); //"CViewMemoryMap::UpdateMapPosition");
	
	float sx = sizeX;
	float sy = sizeY;

	renderMapPosX = mapPosX * sx + posX;
	renderMapPosY = mapPosY * sy + posY;
	
	renderMapSizeX = mapSizeX * sx;
	renderMapSizeY = mapSizeY * sy;
	
	float iw = (float)imageWidth;
	float ih = (float)imageHeight;
	
	float fsx = (mapSizeX / iw);
	float fsy = (mapSizeY / ih);
	
	cellSizeX = fsx * sx;
	cellSizeY = fsy * sy;
	
	float cx = floor( (-mapPosX) / fsx);
	float cy = floor( (-mapPosY) / fsy);
	
	cellStartX = cellSizeX * cx + renderMapPosX;
	cellStartY = cellSizeY * cy + renderMapPosY;
	cellStartIndex = cy * iw + cx;
	
	float nx = ceil(sx / cellSizeX);
	float ny = ceil(sy / cellSizeY);
	numCellsInWidth = (int)(nx+1);
	numCellsInHeight = (int)(ny+1);
	
	////
	currentFontDataScale = fontScale * cellSizeX;
	float heightData = this->font->GetCharHeight(' ', currentFontDataScale) * 1.05f;
	float widthData = this->font->GetCharWidth(' ', currentFontDataScale);

	currentFontAddrScale = currentFontDataScale * 0.35f;
	float heightAddr = this->font->GetCharHeight(' ', currentFontAddrScale) * 1.25f;
	float widthAddr = this->font->GetCharWidth(' ', currentFontAddrScale);
	
	currentFontCodeScale = currentFontDataScale * 0.20f;
	float heightCode = this->font->GetCharHeight(' ', currentFontCodeScale) * 1.0f;
	textCodeWidth = this->font->GetCharWidth(' ', currentFontCodeScale);
	textCodeWidth3 = textCodeWidth*3.0f;
	textCodeWidth3h = textCodeWidth*3.0f / 2.0f;
	textCodeWidth6h = textCodeWidth*6.0f / 2.0f;
	textCodeWidth7h = textCodeWidth*7.0f / 2.0f;
	textCodeWidth8h = textCodeWidth*8.0f / 2.0f;
	textCodeWidth9h = textCodeWidth*9.0f / 2.0f;
	textCodeWidth10h = textCodeWidth*10.0f / 2.0f;

	
	float py;
	py = 0.0f;

	textAddrGapX = cellSizeX/2.0f - widthAddr*2.0f;
	textAddrGapY = py;

	py += heightAddr;

	textDataGapX = cellSizeX/2.0f - widthData;
	textDataGapY = py;

	py += heightData;

	textCodeCenterX = cellSizeX/2.0f;
	textCodeGapY = py;
	
	py += heightCode;
	
	float ty = cellSizeY/2.0f - py/2.0f;
	textAddrGapY += ty;
	textDataGapY += ty;
	textCodeGapY += ty;
	
	
	guiMain->UnlockMutex(); //"CViewMemoryMap::UpdateMapPosition");
	
//	LOGD("UpdateMapPosition: mapSize=%5.2f %5.2f", mapSizeX, mapSizeY);
}

void CViewDataMap::HexDigitToBinary(uint8 hexDigit, char *buf)
{
	switch(hexDigit)
	{
		case 0x00:
			buf[0] = '0'; buf[1] = '0'; buf[2] = '0'; buf[3] = '0'; break;
		case 0x01:
			buf[0] = '0'; buf[1] = '0'; buf[2] = '0'; buf[3] = '1'; break;
		case 0x02:
			buf[0] = '0'; buf[1] = '0'; buf[2] = '1'; buf[3] = '0'; break;
		case 0x03:
			buf[0] = '0'; buf[1] = '0'; buf[2] = '1'; buf[3] = '1'; break;
		case 0x04:
			buf[0] = '0'; buf[1] = '1'; buf[2] = '0'; buf[3] = '0'; break;
		case 0x05:
			buf[0] = '0'; buf[1] = '1'; buf[2] = '0'; buf[3] = '1'; break;
		case 0x06:
			buf[0] = '0'; buf[1] = '1'; buf[2] = '1'; buf[3] = '0'; break;
		case 0x07:
			buf[0] = '0'; buf[1] = '1'; buf[2] = '1'; buf[3] = '1'; break;
		case 0x08:
			buf[0] = '1'; buf[1] = '0'; buf[2] = '0'; buf[3] = '0'; break;
		case 0x09:
			buf[0] = '1'; buf[1] = '0'; buf[2] = '0'; buf[3] = '1'; break;
		case 0x0A:
			buf[0] = '1'; buf[1] = '0'; buf[2] = '1'; buf[3] = '0'; break;
		case 0x0B:
			buf[0] = '1'; buf[1] = '0'; buf[2] = '1'; buf[3] = '1'; break;
		case 0x0C:
			buf[0] = '1'; buf[1] = '1'; buf[2] = '0'; buf[3] = '0'; break;
		case 0x0D:
			buf[0] = '1'; buf[1] = '1'; buf[2] = '0'; buf[3] = '1'; break;
		case 0x0E:
			buf[0] = '1'; buf[1] = '1'; buf[2] = '1'; buf[3] = '0'; break;
		case 0x0F:
			buf[0] = '1'; buf[1] = '1'; buf[2] = '1'; buf[3] = '1'; break;
	}
}

void CViewDataMap::Render()
{
	LOGD("CViewMemoryMap::Render");

	bool renderMapValuesInThisFrame;

	if (mapSizeX > 8.0f)
	{
		renderMapValuesInThisFrame = true;
	}
	else
	{
		renderMapValuesInThisFrame = false;
	}


	
	// TODO: fix me
	if (debugInterface->GetSettingIsWarpSpeed() == false)
	{
//		// CellsAnimationLogic();
//
//		// do not update too often
//		if (nextScreenUpdateFrame < frameCounter)
//		{
////			UpdateWholeMap();
//			imageMemoryMap->ReBindImage();
//			nextScreenUpdateFrame = frameCounter + c64SettingsMemoryMapRefreshRate;
//		}
	}
	else
	{
//		// warp speed
//		if (nextScreenUpdateFrame < frameCounter)
//		{
////			CellsAnimationLogic();
////			UpdateWholeMap();
//			imageMemoryMap->ReBindImage();
//			nextScreenUpdateFrame = frameCounter+50;
//		}
		renderMapValuesInThisFrame = false;
	}
	
	if (shouldRebindImage)
	{
		imageMemoryMap->ReBindImage();
		shouldRebindImage = false;
	}

	frameCounter++;
	
	
	float gap = 1.0f;
	
	BlitRectangle(posX-gap, posY-gap, -1, sizeX+gap*2, sizeY+gap*2, 0.3, 0.3, 0.3, 0.7f);
	
	// blit
	
	VID_SetClipping(posX, posY, sizeX, sizeY);
	
	
//	LOGD("renderTextureStartX=%f renderTextureEndX=%f renderTextureStartY=%f renderTextureEndY=%f",
//		 renderTextureStartX, renderTextureEndX, renderTextureStartY, renderTextureEndY);
	
	Blit(imageMemoryMap, renderMapPosX, renderMapPosY, -1, renderMapSizeX, renderMapSizeY,
		 renderTextureStartX,
		 renderTextureStartY,
		 renderTextureEndX,
		 renderTextureEndY);

	if (renderMapValuesInThisFrame)
	{
		LOGD("...renderMapValuesInThisFrame");
		float cx, cy;
		
		char buf[128];
		
		cy = cellStartY;
		int lineAddr = cellStartIndex;
		for (int iy = 0; iy < numCellsInHeight; iy++)
		{
			cx = cellStartX;
			int addr = lineAddr;
			for (int ix = 0; ix < numCellsInWidth; ix++)
			{
				if (addr > ramSize)
					continue;

				CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
				
				float z = 1.0f - (cell->sr + cell->sg + cell->sb)/2.5f;
				
				float tcr = z; //1.0f - cell->sr;
				float tcg = z; //1.0f - cell->sg;
				float tcb = z; //1.0f - cell->sb;
				
				float px = cx + textAddrGapX;
				float py = cy + textAddrGapY;
				sprintf(buf, "%04X", addr);
				font->BlitTextColor(buf, px, py, posZ, currentFontAddrScale, tcr, tcg, tcb, 1.0f);
				
				px = cx + textDataGapX;
				py = cy + textDataGapY;
				
				uint8 val = memoryBuffer[addr];
				sprintf(buf, "%02X", val);
				font->BlitTextColor(buf, px, py, posZ, currentFontDataScale, tcr, tcg, tcb, 1.0f);
				
				px = cx + textCodeCenterX;
				py = cy + textCodeGapY;
			
				if (cell->isExecuteCode)
				{
					int mode = opcodes[val].addressingMode;
					const char *name = opcodes[val].name;
					
					buf[0] = name[0];
					buf[1] = name[1];
					buf[2] = name[2];
					
					char *buf3 = buf+3;
					
					switch(mode)
					{
						default:
						case ADDR_IMP:
							*buf3 = 0x00;
							px -= textCodeWidth3h;
							break;
						case ADDR_IMM:
							strcpy(buf3, " #..");
							px -= textCodeWidth7h;
							break;
						case ADDR_ZP:
							strcpy(buf3, " ..");
							px -= textCodeWidth6h;
							break;
						case ADDR_ZPX:
							strcpy(buf3, " ..,X");
							px -= textCodeWidth8h;
							break;
						case ADDR_ZPY:
							strcpy(buf3, " ..,Y");
							px -= textCodeWidth8h;
							break;
						case ADDR_IZX:
							strcpy(buf3, " (..,X)");
							px -= textCodeWidth10h;
							break;
						case ADDR_IZY:
							strcpy(buf3, " (..),Y");
							px -= textCodeWidth10h;
							break;
						case ADDR_ABS:
							strcpy(buf3, " ....");
							px -= textCodeWidth8h;
							break;
						case ADDR_ABX:
							strcpy(buf3, " ....,X");
							px -= textCodeWidth10h;
							break;
						case ADDR_ABY:
							strcpy(buf3, " ....,Y");
							px -= textCodeWidth10h;
							break;
						case ADDR_IND:
							strcpy(buf3, " (....)");
							px -= textCodeWidth10h;
							break;
						case ADDR_REL:
							strcpy(buf3, " ....");
							px -= textCodeWidth8h;
							break;
					}
					font->BlitTextColor(buf, px, py, posZ, currentFontCodeScale, tcr, tcg, tcb, 1.0f);
				}
				else
				{
					HexDigitToBinary(((val >> 4) & 0x0F), buf);
					buf[4] = ' ';
					HexDigitToBinary((val & 0x0F), buf + 5);
					buf[9] = 0x00;
					font->BlitTextColor(buf, px - textCodeWidth9h, py, posZ, currentFontCodeScale, tcr, tcg, tcb, 1.0f);
				}
				
				addr++;
				cx += cellSizeX;
			}
			
			lineAddr += imageWidth;
			
			if (lineAddr > ramSize)
				break;
			
			cy += cellSizeY;
		}
		
		LOGTODO("...paint grid not implemented");
//		// paint grid
//		cy = cellStartY;
//		for (int iy = 0; iy < numCellsInHeight; iy++)
//		{
//			BlitLine(posX, cy, posEndX, cy, posZ, 1.0f, 0.0f, 1.0f, 0.9f);
//			cy += cellSizeY;
//		}
//
//		cx = cellStartX;
//		for (int ix = 0; ix < numCellsInWidth; ix++)
//		{
//			BlitLine(cx, posY, cx, posEndY, posZ, 1.0f, 0.0f, 0.0f, 0.9f);
//			cx += cellSizeX;
//		}
	}
	
	
	VID_ResetClipping();
	
//	// debug cursor
//	float px = cursorX * sizeX + posX;
//	float py = cursorY * sizeY + posY;
//	BlitPlus(px, py, posZ, 15, 15, 1, 0, 0, 1);
	
}

void CViewDataMap::RenderImGui()
{
//	LOGD("CViewMemoryMap::RenderImGui");
	PreRenderImGui();
	
	bool renderMapValuesInThisFrame;

//	LOGD("..mapSizeX=%f", mapSizeX);

	if (mapSizeX > 8.0f)
	{
		renderMapValuesInThisFrame = true;
	}
	else
	{
		renderMapValuesInThisFrame = false;
	}

//	LOGD("..renderMapValuesInThisFrame=%s", STRBOOL(mapSizeX));

	// TODO: fix me
	if (debugInterface->GetSettingIsWarpSpeed() == false)
	{
//		CellsAnimationLogic();
		
		// do not update too often
		if (nextScreenUpdateFrame < frameCounter)
		{
//			UpdateWholeMap();
			imageMemoryMap->ReBindImage();
			nextScreenUpdateFrame = frameCounter + c64SettingsMemoryMapRefreshRate;
		}
	}
	else
	{
		// warp speed
		if (nextScreenUpdateFrame < frameCounter)
		{
//			CellsAnimationLogic();
//			UpdateWholeMap();
			imageMemoryMap->ReBindImage();
			nextScreenUpdateFrame = frameCounter+35;
		}
		renderMapValuesInThisFrame = false;
//		LOGD("..warp speed renderMapValuesInThisFrame=%s", STRBOOL(mapSizeX));
	}
	
	frameCounter++;
	
	
	float gap = 1.0f;
	
	BlitRectangle(posX-gap, posY-gap, -1, sizeX+gap*2, sizeY+gap*2, 0.3, 0.3, 0.3, 0.7f);
	
	// blit
	
	VID_SetClipping(posX, posY, sizeX, sizeY);
	
	
	//	LOGD("renderTextureStartX=%f renderTextureEndX=%f renderTextureStartY=%f renderTextureEndY=%f",
	//		 renderTextureStartX, renderTextureEndX, renderTextureStartY, renderTextureEndY);
	
	Blit(imageMemoryMap, renderMapPosX, renderMapPosY, -1, renderMapSizeX, renderMapSizeY,
		 renderTextureStartX,
		 1.0-renderTextureStartY,
		 renderTextureEndX,
		 1.0-renderTextureEndY);
	
//	LOGD("..if renderMapValuesInThisFrame=%s", STRBOOL(mapSizeX));
	if (renderMapValuesInThisFrame)
	{
		float cx, cy;
		
		char buf[128];
		
		cy = cellStartY;
		int lineAddr = cellStartIndex;
		for (int iy = 0; iy < numCellsInHeight; iy++)
		{
			cx = cellStartX;
			int addr = lineAddr;
			for (int ix = 0; ix < numCellsInWidth; ix++)
			{
				if (addr > ramSize)
					continue;
				
				CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
				
				float z = 1.0f - (cell->sr + cell->sg + cell->sb)/2.5f;
				
				float tcr = z; //1.0f - cell->sr;
				float tcg = z; //1.0f - cell->sg;
				float tcb = z; //1.0f - cell->sb;
				
				float px = cx + textAddrGapX;
				float py = cy + textAddrGapY;
				sprintf(buf, "%04X", addr);
				font->BlitTextColor(buf, px, py, posZ, currentFontAddrScale, tcr, tcg, tcb, 1.0f);
				
				px = cx + textDataGapX;
				py = cy + textDataGapY;
				
				uint8 val = memoryBuffer[addr];
				sprintf(buf, "%02X", val);
				font->BlitTextColor(buf, px, py, posZ, currentFontDataScale, tcr, tcg, tcb, 1.0f);
				
				px = cx + textCodeCenterX;
				py = cy + textCodeGapY;
				
				if (cell->isExecuteCode)
				{
					int mode = opcodes[val].addressingMode;
					const char *name = opcodes[val].name;
					
					buf[0] = name[0];
					buf[1] = name[1];
					buf[2] = name[2];
					
					char *buf3 = buf+3;
					
					switch(mode)
					{
						default:
						case ADDR_IMP:
							*buf3 = 0x00;
							px -= textCodeWidth3h;
							break;
						case ADDR_IMM:
							strcpy(buf3, " #..");
							px -= textCodeWidth7h;
							break;
						case ADDR_ZP:
							strcpy(buf3, " ..");
							px -= textCodeWidth6h;
							break;
						case ADDR_ZPX:
							strcpy(buf3, " ..,X");
							px -= textCodeWidth8h;
							break;
						case ADDR_ZPY:
							strcpy(buf3, " ..,Y");
							px -= textCodeWidth8h;
							break;
						case ADDR_IZX:
							strcpy(buf3, " (..,X)");
							px -= textCodeWidth10h;
							break;
						case ADDR_IZY:
							strcpy(buf3, " (..),Y");
							px -= textCodeWidth10h;
							break;
						case ADDR_ABS:
							strcpy(buf3, " ....");
							px -= textCodeWidth8h;
							break;
						case ADDR_ABX:
							strcpy(buf3, " ....,X");
							px -= textCodeWidth10h;
							break;
						case ADDR_ABY:
							strcpy(buf3, " ....,Y");
							px -= textCodeWidth10h;
							break;
						case ADDR_IND:
							strcpy(buf3, " (....)");
							px -= textCodeWidth10h;
							break;
						case ADDR_REL:
							strcpy(buf3, " ....");
							px -= textCodeWidth8h;
							break;
					}
					font->BlitTextColor(buf, px, py, posZ, currentFontCodeScale, tcr, tcg, tcb, 1.0f);
				}
				else
				{
					HexDigitToBinary(((val >> 4) & 0x0F), buf);
					buf[4] = ' ';
					HexDigitToBinary((val & 0x0F), buf + 5);
					buf[9] = 0x00;
					font->BlitTextColor(buf, px - textCodeWidth9h, py, posZ, currentFontCodeScale, tcr, tcg, tcb, 1.0f);
				}
				
				addr++;
				cx += cellSizeX;
			}
			
			lineAddr += imageWidth;
			
			if (lineAddr > ramSize)
				break;
			
			cy += cellSizeY;
		}
		
//		LOGD("... paint grid");
		
		// paint grid
		cy = cellStartY;
		for (int iy = 0; iy < numCellsInHeight; iy++)
		{
			BlitLine(posX, cy, posEndX, cy, posZ, 1.0f, 0.0f, 1.0f, 0.9f);
			cy += cellSizeY;
		}
		
		cx = cellStartX;
		for (int ix = 0; ix < numCellsInWidth; ix++)
		{
			BlitLine(cx, posY, cx, posEndY, posZ, 1.0f, 0.0f, 0.0f, 0.9f);
			cx += cellSizeX;
		}
	}
	
	
	VID_ResetClipping();
	
	//	// debug cursor
	//	float px = cursorX * sizeX + posX;
	//	float py = cursorY * sizeY + posY;
	//	BlitPlus(px, py, posZ, 15, 15, 1, 0, 0, 1);

	PostRenderImGui();
}


// TODO: fix this for Drive 1541
bool CViewDataMap::DoTap(float x, float y)
{
//	LOGD("CViewMemoryMap::DoTap: x=%f y=%f", x, y);
	
	if (viewDataDump == NULL)
		return true;
	
	float cx, cy;
	
	cy = cellStartY;
	int lineAddr = cellStartIndex;
	for (int iy = 0; iy < numCellsInHeight; iy++)
	{
		cx = cellStartX;
		int addr = lineAddr;
		for (int ix = 0; ix < numCellsInWidth; ix++)
		{
			if (cx <= x && x <= cx+cellSizeX
				&& cy <= y && y <= cy+cellSizeY)
			{
				if (addr < 0)
				{
					addr = 0;
				}
				else if (addr > 0xFFFF)
				{
					addr = 0xFFFF;
				}
				
				///
				if (addr == previousClickAddr)
				{
					unsigned long time = SYS_GetCurrentTimeInMillis() - previousClickTime;
					if (time < c64SettingsDoubleClickMS)
					{
						// double click
						viewDataDump->viewDisassembly->ScrollToAddress(addr);
					}
				}
				
				previousClickTime = SYS_GetCurrentTimeInMillis();
				previousClickAddr = addr;
				
				if (guiMain->isControlPressed)
				{
					CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
					
					if (guiMain->isShiftPressed)
					{
						if (cell->readPC != -1)
						{
							viewDataDump->viewDisassembly->ScrollToAddress(cell->readPC);
						}
						
						if (guiMain->isAltPressed)
						{
							if (debugInterface->snapshotsManager)
							{
								LOGM("============######################### RESTORE TO READ CYCLE=%d", cell->readCycle);
								debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->readCycle);
							}
						}
					}
					else
					{
						if (cell->writePC != -1)
						{
							viewDataDump->viewDisassembly->ScrollToAddress(cell->writePC);
						}
						
						if (guiMain->isAltPressed)
						{
							if (debugInterface->snapshotsManager)
							{
								LOGM("============######################### RESTORE TO WRITE CYCLE=%d", cell->writeCycle);
								debugInterface->snapshotsManager->RestoreSnapshotByCycle(cell->writeCycle);
							}
						}
					}
				}

				viewDataDump->ScrollToAddress(addr);
				
				return true;
			}
			
			addr++;
			cx += cellSizeX;
		}
		
		lineAddr += imageWidth;
		
		if (lineAddr > ramSize)
			break;
		
		cy += cellSizeY;
	}
	
	return true;
}

bool CViewDataMap::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
//	LOGD("CViewMemoryMap::KeyDown: keyCode=%x", keyCode);
	if (keyCode == MTKEY_SPACEBAR)
	{
		isForcedMovingMap = true;
		prevMousePosX = guiMain->mousePosX;
		prevMousePosY = guiMain->mousePosY;
		return true;
	}
	
	CSlrKeyboardShortcut *keyboardShortcut = guiMain->keyboardShortcuts->FindShortcut(KBZONE_DISASSEMBLY, keyCode, isShift, isAlt, isControl, isSuper);
	if (keyboardShortcut == viewC64->mainMenuBar->kbsToggleTrackPC)
	{
		if (viewDataDump != NULL)
		{
			return viewDataDump->viewDisassembly->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
		}
	}

	return false;
}

bool CViewDataMap::KeyDownOnMouseHover(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_SPACEBAR && !isShift && !isAlt && !isControl)
	{
//		guiMain->SetFocus(this);
		if (this->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
				return true;
	}
	
	return false;
}

bool CViewDataMap::KeyUpGlobal(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
//	LOGD("CViewMemoryMap::KeyUp: keyCode=%x", keyCode);

	if (keyCode == MTKEY_SPACEBAR)
	{
		isForcedMovingMap = false;
		return true;
	}
	
	return false;
}


void CViewDataMap::SetDataDumpView(CViewDataDump *viewDataDump)
{
	this->viewDataDump = viewDataDump;
}

//
bool CViewDataMap::HasContextMenuItems()
{
	// THIS IS A QUICK HACK FOR BACCHUS. PROPER IMPLEMENTATION IS TO CREATE A NEW CViewMemoryMapC64 : CViewMemoryMap that overrides this method
	if (debugInterface == viewC64->debugInterfaceC64)
	{
		return true;
	}
	
	return false;
}

void CViewDataMap::RenderContextMenuItems()
{
	CGuiView::RenderContextMenuItems();
	

	
//	LOGD("CViewMemoryMap::RenderContextMenuItems");
	
//	// TODO: store CONTEXT MENU SELECTED ITEM id
//	if (ImGui::Button("Scroll to last written"))
//	{
//		if (cell->readPC != -1)
//		{
//			viewDataDump->viewDisassembly->ScrollToAddress(cell->readPC);
//		}
//	}

	// THIS IS A QUICK HACK FOR BACCHUS. PROPER IMPLEMENTATION IS TO CREATE A NEW CViewMemoryMapC64 : CViewMemoryMap that overrides this method
	if (debugInterface == viewC64->debugInterfaceC64)
	{
		bool ramOnly = viewC64->isDataDirectlyFromRAM;
		if (ImGui::MenuItem("RAM-Only Data View", viewC64->mainMenuBar->kbsIsDataDirectlyFromRam->cstr, &ramOnly))
		{
			viewC64->SwitchIsDataDirectlyFromRam(ramOnly);
		}
	}
	
}

CViewDataMap::~CViewDataMap()
{
	
}

// Layout
void CViewDataMap::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewDataMap::Deserialize(CByteBuffer *byteBuffer)
{
}

