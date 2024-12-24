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
	LOGTODO("CDebuggerApiNestopia::GetScreenImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiNestopia::GetScreenImageWithoutBorders()
{
	LOGTODO("CDebuggerApiNestopia::GetScreenImageWithoutBorders: not implemented");
	return NULL;
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

void CDebuggerApiNestopia::MakeJmp(int addr)
{
	LOGTODO("CDebuggerApiNestopia::MakeJMP: not implemented");
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
