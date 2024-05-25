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
	SYS_FatalExit("CDebuggerApiNestopia::CreateNewPicture: not implemented");
}

void CDebuggerApiNestopia::ClearScreen()
{
	SYS_FatalExit("CDebuggerApiNestopia::ClearScreen: not implemented");
}

bool CDebuggerApiNestopia::ConvertImageToScreen(char *filePath)
{
	SYS_FatalExit("CDebuggerApiNestopia::ConvertImageToScreen: not implemented");
	return false;
}

bool CDebuggerApiNestopia::ConvertImageToScreen(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApiNestopia::ConvertImageToScreen: not implemented");
	return false;
}

void CDebuggerApiNestopia::ClearReferenceImage()
{
	SYS_FatalExit("CDebuggerApiNestopia::ClearReferenceImage: not implemented");
}

void CDebuggerApiNestopia::LoadReferenceImage(char *filePath)
{
	SYS_FatalExit("CDebuggerApiNestopia::LoadReferenceImage: not implemented");
}

void CDebuggerApiNestopia::LoadReferenceImage(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApiNestopia::LoadReferenceImage: not implemented");
}

void CDebuggerApiNestopia::SetReferenceImageLayerVisible(bool isVisible)
{
	SYS_FatalExit("CDebuggerApiNestopia::SetReferenceImageLayerVisible: not implemented");
}

CImageData *CDebuggerApiNestopia::GetReferenceImage()
{
	SYS_FatalExit("CDebuggerApiNestopia::GetReferenceImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiNestopia::GetScreenImage(int *width, int *height)
{
	SYS_FatalExit("CDebuggerApiNestopia::GetScreenImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiNestopia::GetScreenImageWithoutBorders()
{
	SYS_FatalExit("CDebuggerApiNestopia::GetScreenImageWithoutBorders: not implemented");
	return NULL;
}

void CDebuggerApiNestopia::ZoomDisplay(float newScale)
{
	SYS_FatalExit("CDebuggerApiNestopia::ZoomDisplay: not implemented");
}

u8 CDebuggerApiNestopia::PaintPixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApiNestopia::PaintPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiNestopia::PaintReferenceImagePixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApiNestopia::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiNestopia::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	SYS_FatalExit("CDebuggerApiNestopia::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

void CDebuggerApiNestopia::ResetMachine()
{
	SYS_FatalExit("CDebuggerApiNestopia::ResetMachine: not implemented");
}

void CDebuggerApiNestopia::MakeJmp(int addr)
{
	SYS_FatalExit("CDebuggerApiNestopia::MakeJMP: not implemented");
}

void CDebuggerApiNestopia::SetByteWithIo(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApiNestopia::SetByteWithIo: not implemented");
}

void CDebuggerApiNestopia::SetByteToRam(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApiNestopia::SetByteToRam: not implemented");
}

void CDebuggerApiNestopia::SetWord(int addr, u16 v)
{
	SetByteToRam(addr+1, ( (v) &0xFF00)>>8);
	SetByteToRam(addr  , ( (v) &0x00FF));
}

void CDebuggerApiNestopia::DetachEverything()
{
	SYS_FatalExit("CDebuggerApiNestopia::DetachEverything: not implemented");
}

int CDebuggerApiNestopia::Assemble(int addr, char *assembleText)
{
	SYS_FatalExit("CDebuggerApiNestopia::Assemble: not implemented");
	return -1;
}

void CDebuggerApiNestopia::SaveBinary(u16 fromAddr, u16 toAddr, char *filePath)
{
	// TODO: we can implement this generic via SetRam(...
	SYS_FatalExit("CDebuggerApiNestopia::SaveBinary: not implemented");
}

int CDebuggerApiNestopia::LoadBinary(u16 fromAddr, char *filePath)
{
	// TODO: we can implement this generic via SetRam(...
	SYS_FatalExit("CDebuggerApiNestopia::LoadBinary: not implemented");
	return -1;
}
