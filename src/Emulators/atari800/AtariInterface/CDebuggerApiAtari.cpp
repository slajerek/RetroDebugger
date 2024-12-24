#include "CDebuggerApiAtari.h"
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

CDebuggerApiAtari::CDebuggerApiAtari(CDebugInterface *debugInterface)
: CDebuggerApi(debugInterface)
{
	this->debugInterfaceAtari = (CDebugInterfaceNes*)debugInterface;
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
	LOGTODO("CDebuggerApiAtari::GetScreenImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiAtari::GetScreenImageWithoutBorders()
{
	LOGTODO("CDebuggerApiAtari::GetScreenImageWithoutBorders: not implemented");
	return NULL;
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

void CDebuggerApiAtari::MakeJmp(int addr)
{
	LOGTODO("CDebuggerApiAtari::MakeJMP: not implemented");
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
