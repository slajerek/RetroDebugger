#include "CDebuggerApiNestopia.h"
#include "CViewC64.h"
#include "CViewMonitorConsole.h"
#include "SYS_KeyCodes.h"
#include "CViewDisassembly.h"
#include "CSlrFileFromOS.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugInterfaceNes.h"
#include "CViewDataMap.h"
#include "CViewDataWatch.h"

CDebuggerApiNestopia::CDebuggerApiNestopia(CDebugInterface *debugInterface)
: CDebuggerApi(debugInterface)
{
	this->debugInterfaceNes = (CDebugInterfaceNes*)debugInterface;
	this->cachedScreenImage = NULL;
}

CDebuggerApiNestopia::~CDebuggerApiNestopia()
{
	delete cachedScreenImage;
}

void CDebuggerApiNestopia::StartThread(CSlrThread *run)
{
	SYS_StartThread(run);
}

void CDebuggerApiNestopia::CreateNewPicture(u8 mode, u8 backgroundColor)
{
	LOGTODO("CDebuggerApiNestopia::CreateNewPicture: not implemented");
}

void CDebuggerApiNestopia::ClearScreen()
{
	LOGTODO("CDebuggerApiNestopia::ClearScreen: not implemented");
}

bool CDebuggerApiNestopia::ConvertImageToScreen(char *filePath)
{
	LOGTODO("CDebuggerApiNestopia::ConvertImageToScreen: not implemented");
	return false;
}

bool CDebuggerApiNestopia::ConvertImageToScreen(CImageData *imageData)
{
	LOGTODO("CDebuggerApiNestopia::ConvertImageToScreen: not implemented");
	return false;
}

void CDebuggerApiNestopia::ClearReferenceImage()
{
	LOGTODO("CDebuggerApiNestopia::ClearReferenceImage: not implemented");
}

void CDebuggerApiNestopia::LoadReferenceImage(char *filePath)
{
	LOGTODO("CDebuggerApiNestopia::LoadReferenceImage: not implemented");
}

void CDebuggerApiNestopia::LoadReferenceImage(CImageData *imageData)
{
	LOGTODO("CDebuggerApiNestopia::LoadReferenceImage: not implemented");
}

void CDebuggerApiNestopia::SetReferenceImageLayerVisible(bool isVisible)
{
	LOGTODO("CDebuggerApiNestopia::SetReferenceImageLayerVisible: not implemented");
}

CImageData *CDebuggerApiNestopia::GetReferenceImage()
{
	LOGTODO("CDebuggerApiNestopia::GetReferenceImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiNestopia::GetScreenImage(int *width, int *height)
{
	int factor = debugInterface->screenSupersampleFactor;
	int contentW = debugInterface->GetScreenSizeX();
	int contentH = debugInterface->GetScreenSizeY();
	int texStride = 512 * factor;

	if (contentW <= 0 || contentH <= 0)
		return NULL;

	if (width)  *width  = contentW;
	if (height) *height = contentH;

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

CImageData *CDebuggerApiNestopia::GetScreenImageWithoutBorders()
{
	return GetScreenImage(NULL, NULL);
}

void CDebuggerApiNestopia::ZoomDisplay(float newScale)
{
	LOGTODO("CDebuggerApiNestopia::ZoomDisplay: not implemented");
}

u8 CDebuggerApiNestopia::PaintPixel(int x, int y, u8 color)
{
	LOGTODO("CDebuggerApiNestopia::PaintPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiNestopia::PaintReferenceImagePixel(int x, int y, u8 color)
{
	LOGTODO("CDebuggerApiNestopia::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiNestopia::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	LOGTODO("CDebuggerApiNestopia::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

bool CDebuggerApiNestopia::MakeJmp(int addr)
{
	LOGTODO("CDebuggerApiNestopia::MakeJMP: not implemented");
	return false;
}

void CDebuggerApiNestopia::SetByteWithIo(int addr, u8 v)
{
	LOGTODO("CDebuggerApiNestopia::SetByteWithIo: not implemented");
}

void CDebuggerApiNestopia::SetByteToRam(int addr, u8 v)
{
	LOGTODO("CDebuggerApiNestopia::SetByteToRam: not implemented");
}

void CDebuggerApiNestopia::SetWord(int addr, u16 v)
{
	SetByteToRam(addr+1, ( (v) &0xFF00)>>8);
	SetByteToRam(addr  , ( (v) &0x00FF));
}

void CDebuggerApiNestopia::DetachEverything()
{
	debugInterfaceNes->DetachEverything();
}

int CDebuggerApiNestopia::Assemble(int addr, char *assembleText)
{
	LOGTODO("CDebuggerApiNestopia::Assemble: not implemented");
	return -1;
}

nlohmann::json CDebuggerApiNestopia::GetCpuStatusJson()
{
	u16 pc;
	u8 a, x, y, p, sp, irq;
	debugInterfaceNes->GetCpuRegs(&pc, &a, &x, &y, &p, &sp, &irq);
	
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
