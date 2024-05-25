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
	this->debugInterfaceNes = (CDebugInterfaceNes*)debugInterface;
}

void CDebuggerApiAtari::StartThread(CSlrThread *run)
{
	SYS_StartThread(run);
}

void CDebuggerApiAtari::CreateNewPicture(u8 mode, u8 backgroundColor)
{
	SYS_FatalExit("CDebuggerApiAtari::CreateNewPicture: not implemented");
}

void CDebuggerApiAtari::ClearScreen()
{
	SYS_FatalExit("CDebuggerApiAtari::ClearScreen: not implemented");
}

bool CDebuggerApiAtari::ConvertImageToScreen(char *filePath)
{
	SYS_FatalExit("CDebuggerApiAtari::ConvertImageToScreen: not implemented");
	return false;
}

bool CDebuggerApiAtari::ConvertImageToScreen(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApiAtari::ConvertImageToScreen: not implemented");
	return false;
}

void CDebuggerApiAtari::ClearReferenceImage()
{
	SYS_FatalExit("CDebuggerApiAtari::ClearReferenceImage: not implemented");
}

void CDebuggerApiAtari::LoadReferenceImage(char *filePath)
{
	SYS_FatalExit("CDebuggerApiAtari::LoadReferenceImage: not implemented");
}

void CDebuggerApiAtari::LoadReferenceImage(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApiAtari::LoadReferenceImage: not implemented");
}

void CDebuggerApiAtari::SetReferenceImageLayerVisible(bool isVisible)
{
	SYS_FatalExit("CDebuggerApiAtari::SetReferenceImageLayerVisible: not implemented");
}

CImageData *CDebuggerApiAtari::GetReferenceImage()
{
	SYS_FatalExit("CDebuggerApiAtari::GetReferenceImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiAtari::GetScreenImage(int *width, int *height)
{
	SYS_FatalExit("CDebuggerApiAtari::GetScreenImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApiAtari::GetScreenImageWithoutBorders()
{
	SYS_FatalExit("CDebuggerApiAtari::GetScreenImageWithoutBorders: not implemented");
	return NULL;
}

void CDebuggerApiAtari::ZoomDisplay(float newScale)
{
	SYS_FatalExit("CDebuggerApiAtari::ZoomDisplay: not implemented");
}

u8 CDebuggerApiAtari::PaintPixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApiAtari::PaintPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiAtari::PaintReferenceImagePixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApiAtari::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApiAtari::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	SYS_FatalExit("CDebuggerApiAtari::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

void CDebuggerApiAtari::ResetMachine()
{
	SYS_FatalExit("CDebuggerApiAtari::ResetMachine: not implemented");
}

void CDebuggerApiAtari::MakeJmp(int addr)
{
	SYS_FatalExit("CDebuggerApiAtari::MakeJMP: not implemented");
}

void CDebuggerApiAtari::SetByteWithIo(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApiAtari::SetByteWithIo: not implemented");
}

void CDebuggerApiAtari::SetByteToRam(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApiAtari::SetByteToRam: not implemented");
}

void CDebuggerApiAtari::SetWord(int addr, u16 v)
{
	SetByteToRam(addr+1, ( (v) &0xFF00)>>8);
	SetByteToRam(addr  , ( (v) &0x00FF));
}

void CDebuggerApiAtari::DetachEverything()
{
	SYS_FatalExit("CDebuggerApiAtari::DetachEverything: not implemented");
}

int CDebuggerApiAtari::Assemble(int addr, char *assembleText)
{
	SYS_FatalExit("CDebuggerApiAtari::Assemble: not implemented");
	return -1;
}

void CDebuggerApiAtari::SaveBinary(u16 fromAddr, u16 toAddr, char *filePath)
{
	// TODO: we can implement this generic via SetRam(...
	SYS_FatalExit("CDebuggerApiAtari::SaveBinary: not implemented");
}

int CDebuggerApiAtari::LoadBinary(u16 fromAddr, char *filePath)
{
	// TODO: we can implement this generic via SetRam(...
	SYS_FatalExit("CDebuggerApiAtari::LoadBinary: not implemented");
	return -1;
}
