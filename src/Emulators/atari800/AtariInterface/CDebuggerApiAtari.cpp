#include "CDebuggerApiAtari.h"
#include "CViewC64.h"
#include "CViewMonitorConsole.h"
#include "SYS_KeyCodes.h"
#include "CViewDisassembly.h"
#include "CSlrFileFromOS.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugInterfaceAtari.h"
#include "CViewDataMap.h"
#include "CViewDataWatch.h"
#include "CMainMenuBar.h"

// atari800 SIO_MAX_DRIVES, kept as a literal so this file stays free of emulator headers
#define ATARI_MAX_DISK_DRIVES 8

CDebuggerApiAtari::CDebuggerApiAtari(CDebugInterface *debugInterface)
: CDebuggerApi(debugInterface)
{
	this->debugInterfaceAtari = (CDebugInterfaceAtari*)debugInterface;
	this->cachedScreenImage = NULL;
}

CDebuggerApiAtari::~CDebuggerApiAtari()
{
	delete cachedScreenImage;
}

void CDebuggerApiAtari::StartThread(CSlrThread *run)
{
	SYS_StartThread(run);
}

void CDebuggerApiAtari::CreateNewPicture(u8 mode, u8 backgroundColor)
{
	LOGTODO("CDebuggerApiAtari::CreateNewPicture: not implemented");
}

void CDebuggerApiAtari::ClearScreen()
{
	LOGTODO("CDebuggerApiAtari::ClearScreen: not implemented");
}

bool CDebuggerApiAtari::ConvertImageToScreen(char *filePath)
{
	LOGTODO("CDebuggerApiAtari::ConvertImageToScreen: not implemented");
	return false;
}

bool CDebuggerApiAtari::ConvertImageToScreen(CImageData *imageData)
{
	LOGTODO("CDebuggerApiAtari::ConvertImageToScreen: not implemented");
	return false;
}

void CDebuggerApiAtari::ClearReferenceImage()
{
	LOGTODO("CDebuggerApiAtari::ClearReferenceImage: not implemented");
}

void CDebuggerApiAtari::LoadReferenceImage(char *filePath)
{
	LOGTODO("CDebuggerApiAtari::LoadReferenceImage: not implemented");
}

void CDebuggerApiAtari::LoadReferenceImage(CImageData *imageData)
{
	LOGTODO("CDebuggerApiAtari::LoadReferenceImage: not implemented");
}

void CDebuggerApiAtari::SetReferenceImageLayerVisible(bool isVisible)
{
	LOGTODO("CDebuggerApiAtari::SetReferenceImageLayerVisible: not implemented");
}

CImageData *CDebuggerApiAtari::GetReferenceImage()
{
	LOGTODO("CDebuggerApiAtari::GetReferenceImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiAtari::GetScreenImage(int *width, int *height)
{
	int factor = debugInterface->screenSupersampleFactor;
	int contentW = debugInterface->GetScreenSizeX();
	int contentH = debugInterface->GetScreenSizeY();
	int texStride = 512 * factor; // texture row width in pixels (hardcoded 512 base)

	if (contentW <= 0 || contentH <= 0)
		return NULL;

	if (width)  *width  = contentW;
	if (height) *height = contentH;

	// Allocate or resize cached buffer
	if (!cachedScreenImage || cachedScreenImage->width != contentW || cachedScreenImage->height != contentH)
	{
		delete cachedScreenImage;
		cachedScreenImage = new CImageData(contentW, contentH, IMG_TYPE_RGBA);
		cachedScreenImage->AllocImage(false, true);
	}

	CImageData *src = debugInterface->AcquireScreenImageForRendering();
	if (!src || !src->resultData)
	{
		debugInterface->ReleaseScreenImageAfterRendering();
		return NULL;
	}

	uint8_t *srcBase = src->resultData;
	uint8_t *dstBase = cachedScreenImage->resultData;
	if (factor <= 1)
	{
		for (int y = 0; y < contentH; y++)
			memcpy(dstBase + y * contentW * 4, srcBase + y * texStride * 4, (size_t)contentW * 4);
	}
	else
	{
		for (int y = 0; y < contentH; y++)
		{
			const uint8_t *srcRow = srcBase + (size_t)(y * factor) * texStride * 4;
			uint8_t *dstRow = dstBase + (size_t)y * contentW * 4;
			for (int x = 0; x < contentW; x++)
				memcpy(dstRow + x * 4, srcRow + (size_t)(x * factor) * 4, 4);
		}
	}

	debugInterface->ReleaseScreenImageAfterRendering();
	return cachedScreenImage;
}

CImageData *CDebuggerApiAtari::GetScreenImageWithoutBorders()
{
	return GetScreenImage(NULL, NULL);
}

void CDebuggerApiAtari::ZoomDisplay(float newScale)
{
	LOGTODO("CDebuggerApiAtari::ZoomDisplay: not implemented");
}

u8 CDebuggerApiAtari::PaintPixel(int x, int y, u8 color)
{
	LOGTODO("CDebuggerApiAtari::PaintPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiAtari::PaintReferenceImagePixel(int x, int y, u8 color)
{
	LOGTODO("CDebuggerApiAtari::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiAtari::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	LOGTODO("CDebuggerApiAtari::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

bool CDebuggerApiAtari::MakeJmp(int addr)
{
	LOGTODO("CDebuggerApiAtari::MakeJMP: not implemented");
	return false;
}

void CDebuggerApiAtari::SetByteWithIo(int addr, u8 v)
{
	LOGTODO("CDebuggerApiAtari::SetByteWithIo: not implemented");
}

void CDebuggerApiAtari::SetByteToRam(int addr, u8 v)
{
	LOGTODO("CDebuggerApiAtari::SetByteToRam: not implemented");
}

void CDebuggerApiAtari::SetWord(int addr, u16 v)
{
	SetByteToRam(addr+1, ( (v) &0xFF00)>>8);
	SetByteToRam(addr  , ( (v) &0x00FF));
}

void CDebuggerApiAtari::DetachEverything()
{
	debugInterfaceAtari->DetachEverything();
}

bool CDebuggerApiAtari::DetachDriveDisk(int deviceNumber)
{
	// atari800 drives are D1: .. D8:, SIO_DisableDrive() indexes SIO_drive_status[deviceNumber-1]
	if (deviceNumber < 1 || deviceNumber > ATARI_MAX_DISK_DRIVES)
	{
		LOGError("CDebuggerApiAtari::DetachDriveDisk: invalid device number %d, expected 1..%d", deviceNumber, ATARI_MAX_DISK_DRIVES);
		return false;
	}
	
	// same code path as the GUI "Detach Disk Image" action: no reset, machine state is kept
	viewC64->mainMenuBar->DetachDiskImageAtari(deviceNumber, true);
	return true;
}

int CDebuggerApiAtari::GetDefaultDiskDriveNumber()
{
	return 1;
}

int CDebuggerApiAtari::Assemble(int addr, char *assembleText)
{
	LOGTODO("CDebuggerApiAtari::Assemble: not implemented");
	return -1;
}

nlohmann::json CDebuggerApiAtari::GetCpuStatusJson()
{
	u16 pc;
	u8 a, x, y, p, sp, irq;
	debugInterfaceAtari->GetCpuRegs(&pc, &a, &x, &y, &p, &sp, &irq);
	
	nlohmann::json cpuStatus;
	cpuStatus["pc"] = pc;
	cpuStatus["a"] = a;
	cpuStatus["x"] = x;
	cpuStatus["y"] = y;
	cpuStatus["sp"] = sp;
	cpuStatus["p"] = p;
	cpuStatus["irq"] = irq;

	return cpuStatus;
}
