#include "CDebuggerApiVice.h"
#include "CViewC64.h"
#include "CViewMonitorConsole.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicEditorCreateNewPicture.h"
#include "CViewC64VicDisplay.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerImage.h"
#include "CViewC64VicEditorLayers.h"
#include "CVicEditorLayerC64Screen.h"
#include "CViewC64Sprite.h"
#include "CViewC64Charset.h"
#include "CMainMenuBar.h"
#include "SYS_KeyCodes.h"
#include "C64Tools.h"
#include "CViewDisassembly.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CSlrFileFromOS.h"
#include "CViewDataMap.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CViewDataWatch.h"
#include "CGuiMain.h"
#include "CSlrFont.h"

CDebuggerApiVice::CDebuggerApiVice(CDebugInterface *debugInterface)
: CDebuggerApi(debugInterface)
{
	assembleTarget = ASSEMBLE_TARGET_MAIN_CPU;
}

void CDebuggerApiVice::SwitchToVicEditor()
{
	LOGTODO("CDebuggerApiVice::SwitchToVicEditor: not supported");
	return;
//	viewC64->screenVicEditor->SwitchToVicEditor();
}

void CDebuggerApiVice::CreateNewPicture(u8 mode, u8 backgroundColor)
{
	viewC64->viewVicEditor->viewCreateNewPicture->CreateNewPicture(mode, backgroundColor, false);
}

void CDebuggerApiVice::ClearScreen()
{
	viewC64->viewVicEditor->viewVicDisplay->currentCanvas->ClearScreen();
}

bool CDebuggerApiVice::ConvertImageToScreen(char *filePath)
{
	CImageData *imageData = new CImageData(filePath);
	if (imageData->resultData == NULL)
		return false;
	
//	viewC64->viewVicEditor->viewVicDisplay->currentCanvas->ConvertFrom(imageData);
	viewC64->viewVicEditor->ImportImage(imageData);
	delete imageData;
	return true;
}

bool CDebuggerApiVice::ConvertImageToScreen(CImageData *imageData)
{
	return viewC64->viewVicEditor->ImportImage(imageData);
}

void CDebuggerApiVice::ClearReferenceImage()
{
	viewC64->viewVicEditor->layerReferenceImage->ClearScreen();
}

void CDebuggerApiVice::LoadReferenceImage(char *filePath)
{
	CImageData *imageData = new CImageData(filePath);
	viewC64->viewVicEditor->layerReferenceImage->LoadFrom(imageData);
	delete imageData;

}

void CDebuggerApiVice::LoadReferenceImage(CImageData *imageData)
{
	viewC64->viewVicEditor->layerReferenceImage->LoadFrom(imageData);
}

void CDebuggerApiVice::SetReferenceImageLayerVisible(bool isVisible)
{
	viewC64->viewVicEditor->viewLayers->SetLayerVisible(viewC64->viewVicEditor->layerReferenceImage, isVisible);
}

CImageData *CDebuggerApiVice::GetReferenceImage()
{
	return viewC64->viewVicEditor->layerReferenceImage->GetScreenImage();
}

CImageData *CDebuggerApiVice::GetScreenImage(int *width, int *height)
{
	return viewC64->viewVicEditor->layerC64Screen->GetScreenImage(width, height);
}

CImageData *CDebuggerApiVice::GetScreenImageWithoutBorders()
{
	return viewC64->viewVicEditor->layerC64Screen->GetInteriorScreenImage();
}


void CDebuggerApiVice::SetTopBarVisible(bool isVisible)
{
	LOGTODO("CDebuggerApiVice::SetTopBarVisible: not implemented");
//	viewC64->viewVicEditor->SetTopBarVisible(isVisible);
}

void CDebuggerApiVice::SetViewPaletteVisible(bool isVisible)
{
	viewC64->viewVicEditor->viewPalette->SetVisible(isVisible);
}

void CDebuggerApiVice::SetViewCharsetVisible(bool isVisible)
{
	viewC64->viewVicEditor->viewCharset->SetVisible(isVisible);
}

void CDebuggerApiVice::SetViewSpriteVisible(bool isVisible)
{
	viewC64->viewVicEditor->viewSprite->SetVisible(isVisible);
}

void CDebuggerApiVice::SetViewPreviewVisible(bool isVisible)
{
	LOGTODO("CDebuggerApiVice::SetViewPreviewVisible: not implemented");
//	viewC64->viewVicEditor->viewVicDisplaySmall->SetVisible(isVisible);
}

void CDebuggerApiVice::SetViewLayersVisible(bool isVisible)
{
	viewC64->viewVicEditor->viewLayers->SetVisible(isVisible);
}

void CDebuggerApiVice::SetSpritesFramesVisible(bool isVisible)
{
	viewC64->viewVicEditor->SetSpritesFramesVisible(isVisible);
}

void CDebuggerApiVice::ZoomDisplay(float newScale)
{
	viewC64->viewVicEditor->ZoomDisplay(newScale);
}

void CDebuggerApiVice::SetupVicEditorForScreenOnly()
{
	SetTopBarVisible(false);
	SetViewPaletteVisible(false);
	SetViewCharsetVisible(false);
	SetViewSpriteVisible(false);
	SetViewLayersVisible(false);
	SetViewPreviewVisible(false);
	SetSpritesFramesVisible(false);
	ZoomDisplay(1.80f);
}

u8 CDebuggerApiVice::FindC64Color(u8 r, u8 g, u8 b)
{
	return ::FindC64Color(r, g, b, viewC64->viewVicEditor->viewVicDisplay->debugInterface);
}

u8 CDebuggerApiVice::PaintPixel(int x, int y, u8 color)
{
	return viewC64->viewVicEditor->PaintPixelColor(x, y, color);
}

u8 CDebuggerApiVice::PaintReferenceImagePixel(int x, int y, u8 color)
{
	return viewC64->viewVicEditor->layerReferenceImage->PutPixelImage(x, y, color);
}

u8 CDebuggerApiVice::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	return viewC64->viewVicEditor->layerReferenceImage->PutPixelImage(x, y, r, g, b, a);
}

void CDebuggerApiVice::Sleep(long milliseconds)
{
	SYS_Sleep(milliseconds);
}

long CDebuggerApiVice::GetCurrentTimeInMilliseconds()
{
	return SYS_GetCurrentTimeInMillis();
}

void CDebuggerApiVice::ResetMachine()
{
	viewC64->debugInterfaceC64->HardReset();
}

void CDebuggerApiVice::MakeJmp(int addr)
{
	viewC64->debugInterfaceC64->MakeJmpC64(addr);
}

void CDebuggerApiVice::SetByte(int addr, u8 v)
{
	SetByteToRam(addr, v);
}

void CDebuggerApiVice::SetByteWithIo(int addr, u8 v)
{
	viewC64->debugInterfaceC64->SetByteC64(addr, v);
}

void CDebuggerApiVice::SetByteToRam(int addr, u8 v)
{
//	LOGD("CDebuggerApiVice::SetByteToRam: %04x %02x", addr, v);
	viewC64->debugInterfaceC64->SetByteToRamC64(addr, v);
}

u8 CDebuggerApiVice::GetByteWithIo(int addr)
{
	return viewC64->debugInterfaceC64->GetByteC64(addr);
}

u8 CDebuggerApiVice::GetByteFromRam(int addr)
{
	return GetByteFromRamC64(addr);
}

void CDebuggerApiVice::SetByteToRamC64(int addr, u8 v)
{
	viewC64->debugInterfaceC64->SetByteToRamC64(addr, v);
}

u8 CDebuggerApiVice::GetByteFromRamC64(int addr)
{
	return viewC64->debugInterfaceC64->GetByteFromRamC64(addr);
}

void CDebuggerApiVice::DetachEverything()
{
	viewC64->mainMenuBar->DetachEverything(false, false);
}

void CDebuggerApiVice::ClearRam(int startAddr, int endAddr, u8 value)
{
	for (int i = startAddr; i < endAddr; i++)
	{
		this->SetByteToRamC64(i, value);
	}
}

void CDebuggerApiVice::SetAssembleTarget(u8 target)
{
	this->assembleTarget = target;
}

extern "C" {
	void c64debugger_set_assemble_result_to_memory(void *userData, int addr, unsigned char v)
	{
//		LOGD("c64debugger_set_assemble_result_to_memory: %04x %02x", addr, v);
		
		CDebuggerApiVice *debuggerAPI = (CDebuggerApiVice*)userData;
		if (debuggerAPI->assembleTarget == ASSEMBLE_TARGET_NONE)
		{
			// skip
		}
		else if (debuggerAPI->assembleTarget == ASSEMBLE_TARGET_MAIN_CPU)
		{
			debuggerAPI->SetByteToRamC64(addr, v);
		}
		else
		{
			SYS_FatalExit("TODO: assemble target");
		}
	}
};


int CDebuggerApiVice::Assemble(int addr, char *assembleText)
{
	if (assembleTarget == ASSEMBLE_TARGET_MAIN_CPU)
	{
		return viewC64->viewC64Disassembly->Assemble(addr, assembleText, false);
	}
	else if (assembleTarget == ASSEMBLE_TARGET_DISK_DRIVE1)
	{
		return viewC64->viewDrive1541Disassembly->Assemble(addr, assembleText, false);
	}
	return -1;
}

void CDebuggerApiVice::AddWatch(CSlrString *segmentName, int address, CSlrString *watchName, uint8 representation, int numberOfValues)
{
	if (debugInterface->symbols)
	{
		CDebugSymbolsSegment *segment = debugInterface->symbols->FindSegment(segmentName);
		if (segment == NULL)
		{
			segmentName->DebugPrint("segment=");
			LOGError("CDebuggerApiVice::AddWatch: segment not found");
			return;
		}

		// TODO: convert watch name in symbols to CSlrString
		char *cWatchName = watchName->GetStdASCII();
		segment->AddWatch(address, cWatchName, representation, numberOfValues);
		delete [] cWatchName;
	}
	else
	{
		LOGError("CDebuggerApiVice::AddWatch: no symbols");
	}
}

void CDebuggerApiVice::AddWatch(int address, char *watchName, uint8 representation, int numberOfValues)
{
	// TODO: Generalize me
	CDebugInterface *debugInterface = NULL;
	if (viewC64->debugInterfaceC64)
	{
		debugInterface = viewC64->debugInterfaceC64;
	}
	else if (viewC64->debugInterfaceAtari)
	{
		debugInterface = viewC64->debugInterfaceAtari;
	}
	
	
	if (debugInterface->symbols)
	{
		CDebugSymbolsSegment *segment = debugInterface->symbols->currentSegment;
		if (segment == NULL)
		{
			LOGError("CDebuggerApiVice::AddWatch: default segment not found");
			return;
		}
		
		segment->AddWatch(address, watchName, representation, numberOfValues);
	}
	else
	{
		LOGError("CDebuggerApiVice::AddWatch: no symbols");
	}
}

void CDebuggerApiVice::AddWatch(int address, char *watchName)
{
	this->AddWatch(address, watchName, WATCH_REPRESENTATION_HEX_8, 1);
}

bool CDebuggerApiVice::LoadPRG(const char *filePath)
{
	u16 fromAddr, toAddr;
	return this->LoadPRG(filePath, &fromAddr, &toAddr);
}

bool CDebuggerApiVice::LoadPRG(const char *filePath, u16 *fromAddr, u16 *toAddr)
{
	CSlrFileFromOS *file = new CSlrFileFromOS(filePath);
	if (file->Exists())
	{
		CByteBuffer *byteBuffer = new CByteBuffer(file, false);

		viewC64->viewC64MainMenu->LoadPRG(byteBuffer, fromAddr, toAddr);
		
		delete byteBuffer;
		delete file;
		return true;
	}
	
	delete file;
	return false;
}

bool CDebuggerApiVice::LoadPRG(CByteBuffer *byteBuffer, u16 *fromAddr, u16 *toAddr)
{
	viewC64->viewC64MainMenu->LoadPRG(byteBuffer, fromAddr, toAddr);
	return true;
}

bool CDebuggerApiVice::LoadPRG(CByteBuffer *byteBuffer, bool autoStart, bool forceFastReset)
{
	return viewC64->viewC64MainMenu->LoadPRG(byteBuffer, autoStart, false, forceFastReset);
}

bool CDebuggerApiVice::LoadCRT(CByteBuffer *byteBuffer)
{
	LOGD("CDebuggerApiVice::LoadCRT");
	
	CSlrString *filePath = new CSlrString(gUTFPathToTemp);
	filePath->Concatenate("temp.crt");
	
	byteBuffer->storeToFile(filePath);
	
	filePath->DebugPrint();
	
	bool ret = viewC64->viewC64MainMenu->InsertCartridge(filePath, false);
	
	delete filePath;
	
	return ret;
}

bool CDebuggerApiVice::LoadCRT(const char *filePath)
{
	CSlrString *path = new CSlrString(filePath);
	bool ret = viewC64->viewC64MainMenu->InsertCartridge(path, false);
	delete path;
	return ret;
}

bool CDebuggerApiVice::LoadSID(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr)
{
	return C64LoadSIDToRam(filePath, fromAddr, toAddr, initAddr, playAddr);
}

bool CDebuggerApiVice::LoadSID(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr, u8 **buffer)
{
	return C64LoadSIDToBuffer(filePath, fromAddr, toAddr, initAddr, playAddr, buffer);
}

bool CDebuggerApiVice::LoadKLA(const char *filePath)
{
	CSlrString *path = new CSlrString(filePath);
	bool ret = viewC64->viewVicEditor->ImportKoala(path, false);
	delete path;
	return ret;
}

bool CDebuggerApiVice::LoadKLA(const char *filePath, u16 bitmapAddress, u16 screenAddress, u16 colorRamAddress, u8 *colorD020, u8 *colorD021)
{
	CSlrString *path = new CSlrString(filePath);
	bool ret = viewC64->viewVicEditor->ImportKoala(path, bitmapAddress, screenAddress, colorRamAddress, colorD020, colorD021);
	delete path;
	return ret;
}

void CDebuggerApiVice::SaveExomizerPRG(u16 fromAddr, u16 toAddr, u16 jmpAddr, const char *filePath)
{
	LOGM("SaveExomizerPRG: fromAddr=%04x toAddr=%04x jmpAddr=%04x filePath='%s'", fromAddr, toAddr, jmpAddr, filePath);
	C64SaveMemoryExomizerPRG(fromAddr, toAddr, jmpAddr, filePath);
}

u8 *CDebuggerApiVice::ExomizerMemoryRaw(u16 fromAddr, u16 toAddr, int *compressedSize)
{
	return C64ExomizeMemoryRaw(fromAddr, toAddr, compressedSize);
}



void CDebuggerApiVice::SavePRG(u16 fromAddr, u16 toAddr, const char *filePath)
{
	LOGM("SavePRG: fromAddr=%04x toAddr=%04x filePath='%s'", fromAddr, toAddr, filePath);
	C64SaveMemory(fromAddr, toAddr, true, viewC64->debugInterfaceC64->dataAdapterC64DirectRam, filePath);
}

void CDebuggerApiVice::SaveBinary(u16 fromAddr, u16 toAddr, const char *filePath)
{
	C64SaveMemory(fromAddr, toAddr, false, viewC64->debugInterfaceC64->dataAdapterC64DirectRam, filePath);
}

int CDebuggerApiVice::LoadBinary(u16 fromAddr, const char *filePath)
{
	return C64LoadMemory(fromAddr, viewC64->debugInterfaceC64->dataAdapterC64DirectRam, filePath);
}

void CDebuggerApiVice::BasicUpStart(u16 jmpAddr)
{
	int lineNumber = 666;
	
	char buf[16];
	sprintf(buf, "%d", jmpAddr);
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0801, 0x0D);
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0802, 0x08);
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0803, (u8) (lineNumber & 0xff));
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0804, (u8) (lineNumber >> 8));
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0805, 0x9E);
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0806, 0x20);

	int a = 0x0807;
	for (int i = 0; i < strlen(buf); i++)
	{
		viewC64->debugInterfaceC64->SetByteToRamC64(a++, buf[i]);
	}
	viewC64->debugInterfaceC64->SetByteToRamC64(a++, 0x00);
}

void CDebuggerApiVice::AddCrtEntryPoint(u8 *cartImage, u16 coldStartAddr, u16 warmStartAddr)
{
	// fill with necessary entry point
	cartImage[0x00] = coldStartAddr & 0x00FF;
	cartImage[0x01] = (coldStartAddr & 0xFF00) >> 8;
	cartImage[0x02] = warmStartAddr & 0x00FF;
	cartImage[0x03] = (warmStartAddr & 0xFF00) >> 8;
	cartImage[0x04] = 0xC3;	// C
	cartImage[0x05] = 0xC2;	// B
	cartImage[0x06] = 0xCD;	// M
	cartImage[0x07] = 0x38;	// 8
	cartImage[0x08] = 0x30;	// 0
}

CByteBuffer *CDebuggerApiVice::MakeCrt(const char *cartName, int cartSize, int bankSize, u8 *cartImage)
{
	int numBanks = cartSize / bankSize;
	
	LOGD("MakeCrt: cartSize=%d bankSize=%d numBanks=%d", cartSize, bankSize, numBanks);
	
	// create CRT file
	const char *crtMarker = "C64 CARTRIDGE   ";
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	// C64 CARTRIDGE marker
	byteBuffer->PutBytes((u8*)crtMarker, 0x10);
	
	// header length 00000040
	byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x40);
	
	// cartridge version (1.00)
	byteBuffer->PutByte(0x01); byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00);

	// cartridge type 0x13 = Magic Desk
	byteBuffer->PutByte(0x13);
	
	// EXROM
	byteBuffer->PutByte(0x00);
	
	// GAME
	byteBuffer->PutByte(0x01);

	// reserved
	byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00);
	byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00);

	// cart name, pad with zeros
	char buf[0x20];
	strncpy(buf, cartName, 0x20);
	byteBuffer->PutBytes((u8*)buf, 0x20);
	
	// header completed
	
	// CHIPs
	for (int bankNumber = 0; bankNumber < numBanks; bankNumber++)
	{
		// CHIP marker
		byteBuffer->PutByte('C'); byteBuffer->PutByte('H'); byteBuffer->PutByte('I'); byteBuffer->PutByte('P');
		
		// total length
		byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x20); byteBuffer->PutByte(0x10);

		// chip type = ROM
		byteBuffer->PutByte(0x00); byteBuffer->PutByte(0x00);
		
		// bank number
		int b = bankNumber;
		byteBuffer->PutByte((b & 0xFF00) >> 8);
		byteBuffer->PutByte((b & 0x00FF));

		// starting load address 0x8000
		byteBuffer->PutByte(0x80); byteBuffer->PutByte(0x00);
		
		// ROM image in bytes 0x2000
		byteBuffer->PutByte(0x20); byteBuffer->PutByte(0x00);
		
		// ROM data
		int bankAddr = bankNumber * bankSize;
		int p = bankAddr;
		for (int i = 0; i < bankSize; i++)
		{
			byteBuffer->PutByte(cartImage[p++]);
		}
	}
		
	return byteBuffer;
}

void CDebuggerApiVice::SetCiaRegister(u8 ciaId, u8 registerNum, u8 value)
{
	viewC64->debugInterfaceC64->SetCiaRegister(ciaId, registerNum, value);
}

void CDebuggerApiVice::SetVicRegister(u16 registerNum, u8 value)
{
	u8 reg = registerNum & 0x00FF;
	viewC64->debugInterfaceC64->SetVicRegister(reg, value);
}

void CDebuggerApiVice::SetScreenAndCharsetAddress(u16 screenAddr, u16 charsetAddr)
{
	LOGD("CDebuggerApiVice::SetScreenAndCharsetAddress: screenAddr=%04x charsetAddr=%04x", screenAddr, charsetAddr);

	// SET VIC BANK
	int bankNum = floor((float)charsetAddr / 16384.0f);
	
	LOGD("bankNum=%d", bankNum);
	
	// (skip) set direction, DD02
	//SetCiaRegister(0, 2, xxxxxxx);

	// set DD00
	SetCiaRegister(0, 0, 3-bankNum);
	
	u16 bankAddr = bankNum*0x4000;
	u16 diff = charsetAddr-bankAddr;
	int characterSetNum = floor((float)diff / 2048.0f);

	diff = screenAddr-bankAddr;
	int screenNum = floor((float)diff / 1024.0f);
	
	u8 d018val = ((u8)screenNum << 4 | (u8)characterSetNum << 1);
	
	LOGD("d018val=%02x", d018val);
	
	SetVicRegister(0x18, d018val);
}

void CDebuggerApiVice::GetCBMColor(u8 colorNum, u8 *r, u8 *g, u8 *b)
{
	return viewC64->debugInterfaceC64->GetCBMColor(colorNum, r, g, b);
}

void CDebuggerApiVice::ShowMessage(const char *text)
{
	viewC64->ShowMessageInfo((char*)text);
}

void CDebuggerApiVice::BlitText(const char *text, float posX, float posY, float fontSize)
{
	CSlrFont *font = viewC64->fontDisassembly;
	font->BlitText((char*)text, posX, posY, -1, fontSize);
}

void CDebuggerApiVice::AddView(CGuiView *view)
{
	guiMain->LockMutex();
	guiMain->AddView(view);
	debugInterface->AddView(view);
	guiMain->UnlockMutex();
}

