#include "CViewC64.h"
#include "CViewMonitorConsole.h"
#include "CViewVicEditor.h"
#include "CViewVicEditorCreateNewPicture.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerImage.h"
#include "CViewVicEditorLayers.h"
#include "CVicEditorLayerC64Screen.h"
#include "CViewC64Sprite.h"
#include "SYS_KeyCodes.h"
#include "C64Tools.h"
#include "CViewDisassembly.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CSlrFileFromOS.h"
#include "CViewMemoryMap.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CViewDataWatch.h"
#include "CGuiMain.h"
#include "CSlrFont.h"
#include "CDataAdapter.h"
#include "CDebuggerApiVice.h"
#include "CDebuggerApiNestopia.h"
#include "CDebuggerApiAtari.h"

// factory
CDebuggerApi *CDebuggerApi::GetDebuggerApi(u8 emulatorType)
{
	CDebugInterface *debugInterface = viewC64->GetDebugInterface(emulatorType);

	switch(emulatorType)
	{
		case EMULATOR_TYPE_C64_VICE:
			return new CDebuggerApiVice(debugInterface);
		case EMULATOR_TYPE_NESTOPIA:
			return new CDebuggerApiNestopia(debugInterface);
		case EMULATOR_TYPE_ATARI800:
			return new CDebuggerApiAtari(debugInterface);
	}
	SYS_FatalExit("CDebuggerAPI::GetDebuggerApi: emulatorType=%d not supported", emulatorType);
	return NULL;
}

CDebuggerApi::CDebuggerApi(CDebugInterface *debugInterface)
{
	this->debugInterface = debugInterface;
	this->byteBufferAssembleText = new CByteBuffer();
}

void CDebuggerApi::StartThread(CSlrThread *run)
{
	SYS_StartThread(run);
}

void CDebuggerApi::CreateNewPicture(u8 mode, u8 backgroundColor)
{
	SYS_FatalExit("CDebuggerApi::CreateNewPicture: not implemented");
}

void CDebuggerApi::ClearScreen()
{
	SYS_FatalExit("CDebuggerApi::ClearScreen: not implemented");
}

void CDebuggerApi::ConvertImageToScreen(char *filePath)
{
	SYS_FatalExit("CDebuggerApi::ConvertImageToScreen: not implemented");
}

void CDebuggerApi::ConvertImageToScreen(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApi::ConvertImageToScreen: not implemented");
}

void CDebuggerApi::ClearReferenceImage()
{
	SYS_FatalExit("CDebuggerApi::ClearReferenceImage: not implemented");
}

void CDebuggerApi::LoadReferenceImage(char *filePath)
{
	SYS_FatalExit("CDebuggerApi::LoadReferenceImage: not implemented");
}

void CDebuggerApi::LoadReferenceImage(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApi::LoadReferenceImage: not implemented");
}

void CDebuggerApi::SetReferenceImageLayerVisible(bool isVisible)
{
	SYS_FatalExit("CDebuggerApi::SetReferenceImageLayerVisible: not implemented");
}

CImageData *CDebuggerApi::GetReferenceImage()
{
	SYS_FatalExit("CDebuggerApi::GetReferenceImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApi::GetScreenImage(int *width, int *height)
{
	SYS_FatalExit("CDebuggerApi::GetScreenImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApi::GetScreenImageWithoutBorders()
{
	SYS_FatalExit("CDebuggerApi::GetScreenImageWithoutBorders: not implemented");
	return NULL;
}

void CDebuggerApi::ZoomDisplay(float newScale)
{
	SYS_FatalExit("CDebuggerApi::ZoomDisplay: not implemented");
}

u8 CDebuggerApi::PaintPixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApi::PaintPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApi::PaintReferenceImagePixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApi::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApi::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	SYS_FatalExit("CDebuggerApi::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

void CDebuggerApi::Sleep(long milliseconds)
{
	SYS_Sleep(milliseconds);
}

long CDebuggerApi::GetCurrentTimeInMilliseconds()
{
	return SYS_GetCurrentTimeInMillis();
}

void CDebuggerApi::ResetMachine()
{
	SYS_FatalExit("CDebuggerApi::ResetMachine: not implemented");
}

void CDebuggerApi::MakeJmp(int addr)
{
	SYS_FatalExit("CDebuggerApi::MakeJMP: not implemented");
}

void CDebuggerApi::SetByte(int addr, u8 v)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (addr >= 0 && addr < dataAdapter->AdapterGetDataLength())
	{
		dataAdapter->AdapterWriteByte(addr, v);
	}
}

void CDebuggerApi::SetByteWithIo(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApi::SetByteWithIo: not implemented");
}

void CDebuggerApi::SetByteToRam(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApi::SetByteToRam: not implemented");
}

u8 CDebuggerApi::GetByte(int addr)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (addr >= 0 && addr < dataAdapter->AdapterGetDataLength())
	{
		u8 val;
		dataAdapter->AdapterReadByte(addr, &val);
		return val;
	}
	
	return 0;
}

u8 CDebuggerApi::GetByteWithIo(int addr)
{
	SYS_FatalExit("CDebuggerApi::GetByteWithIo: not implemented");
	return 0;
}

u8 CDebuggerApi::GetByteFromRam(int addr)
{
	SYS_FatalExit("CDebuggerApi::GetByteFromRam: not implemented");
	return 0;
}


void CDebuggerApi::SetWord(int addr, u16 v)
{
	SetByte(addr+1, ( (v) &0xFF00)>>8);
	SetByte(addr  , ( (v) &0x00FF));
}

void CDebuggerApi::DetachEverything()
{
	SYS_FatalExit("CDebuggerApi::DetachEverything: not implemented");
}

void CDebuggerApi::ClearRam(int startAddr, int endAddr, u8 value)
{
	for (int i = startAddr; i < endAddr; i++)
	{
		this->SetByteToRam(i, value);
	}
}

extern "C" {
	unsigned char *assemble_64tass(void *userData, char *assembleText, int assembleTextSize, int *codeStartAddr, int *codeSize);
	void assemble_64tass_setquiet(int isQuiet);
}

u8 *CDebuggerApi::Assemble64Tass(char *assembleText, int *codeStartAddr, int *codeSize)
{
	u8 *buf = assemble_64tass((void*)this, assembleText, strlen(assembleText), codeStartAddr, codeSize);
	
	if (buf == NULL)
	{
		*codeStartAddr = 0;
		*codeSize = 0;
		LOGError("CDebuggerAPI::Assemble64Tass: assemble failed");
		return NULL;
	}

	return buf;
}

bool CDebuggerApi::Assemble64TassToRam(int *codeStartAddr, int *codeSize)
{
	return Assemble64TassToRam(codeStartAddr, codeSize, NULL, false);
}

bool CDebuggerApi::Assemble64TassToRam(int *codeStartAddr, int *codeSize, char *fileName, bool quiet)
{
	u8 *buf = this->Assemble64Tass(codeStartAddr, codeSize, fileName, quiet);
	
	if (buf == NULL)
	{
		*codeStartAddr = 0;
		*codeSize = 0;
		LOGError("CDebuggerAPI::Assemble64Tass: assemble failed");
		return false;
	}

	int addr = *codeStartAddr;
	for (int i = 0; i < *codeSize; i++)
	{
		this->SetByteToRam(addr, buf[i]);
		addr++;
	}
	free(buf);
	
	return true;
}


void CDebuggerApi::Assemble64TassClearBuffer()
{
	byteBufferAssembleText->Clear();
}

void CDebuggerApi::Assemble64TassAddLine(const char *format, ...)
{
	char *assembleText = SYS_GetCharBuf();

	va_list args;

	va_start(args, format);
	vsnprintf(assembleText, MAX_STRING_LENGTH, format, args);
	va_end(args);

	char *ptr = assembleText;
	while (*ptr != '\0')
	{
		byteBufferAssembleText->PutU8(*ptr);
		ptr++;
	}
	byteBufferAssembleText->PutU8('\n');
	
	SYS_ReleaseCharBuf(assembleText);
}

u8 *CDebuggerApi::Assemble64Tass(int *codeStartAddr, int *codeSize)
{
	return this->Assemble64Tass(codeStartAddr, codeSize, NULL, false);
}

u8 *CDebuggerApi::Assemble64Tass(int *codeStartAddr, int *codeSize, char *storeAsmFileName, bool quiet)
{
	assemble_64tass_setquiet(quiet);
	
	byteBufferAssembleText->PutU8(0x00);
	
	char *assembleText = (char*)byteBufferAssembleText->data;
	if (storeAsmFileName != NULL)
	{
		FILE *fp = fopen(storeAsmFileName, "wb");
		if (fp != NULL)
		{
			fprintf(fp, "%s", assembleText);
			fclose(fp);
		}
		else
		{
			LOGError("CDebuggerAPI::Assemble64Tass: file not writable %s", storeAsmFileName);
		}
	}
	
	LOGD("assembleText='%s'", assembleText);
	
	u8 *buf = assemble_64tass((void*)this, assembleText, byteBufferAssembleText->length-1, codeStartAddr, codeSize);
	
	if (buf == NULL)
	{
		*codeStartAddr = 0;
		*codeSize = 0;
		LOGError("CDebuggerAPI::Assemble64Tass: assemble failed");
		return NULL;
	}
	
	byteBufferAssembleText->Reset();
	
	return buf;
}

int CDebuggerApi::Assemble(int addr, char *assembleText)
{
	SYS_FatalExit("CDebuggerApi::Assemble: not implemented");
	return -1;
}

void CDebuggerApi::AddWatch(CSlrString *segmentName, int address, CSlrString *watchName, uint8 representation, int numberOfValues)
{
	if (debugInterface->symbols)
	{
		CDebugSymbolsSegment *segment = debugInterface->symbols->FindSegment(segmentName);
		if (segment == NULL)
		{
			segmentName->DebugPrint("segment=");
			LOGError("CDebuggerAPI::AddWatch: segment not found");
			return;
		}

		// TODO: convert watch name in symbols to CSlrString
		char *cWatchName = watchName->GetStdASCII();
		segment->AddWatch(address, cWatchName, representation, numberOfValues);
		delete [] cWatchName;
	}
	else
	{
		LOGError("CDebuggerAPI::AddWatch: no symbols");
	}
}

void CDebuggerApi::AddWatch(int address, char *watchName, uint8 representation, int numberOfValues)
{
	if (debugInterface->symbols)
	{
		CDebugSymbolsSegment *segment = debugInterface->symbols->currentSegment;
		if (segment == NULL)
		{
			LOGError("CDebuggerAPI::AddWatch: default segment not found");
			return;
		}
		
		segment->AddWatch(address, watchName, representation, numberOfValues);
	}
	else
	{
		LOGError("CDebuggerAPI::AddWatch: no symbols");
	}
}

void CDebuggerApi::AddWatch(int address, char *watchName)
{
	this->AddWatch(address, watchName, WATCH_REPRESENTATION_HEX_8, 1);
}

u8 *CDebuggerApi::ExomizerMemoryRaw(u16 fromAddr, u16 toAddr, int *compressedSize)
{
	return C64ExomizeMemoryRaw(fromAddr, toAddr, compressedSize);
}

void CDebuggerApi::SaveBinary(u16 fromAddr, u16 toAddr, char *filePath)
{
	// TODO: we can implement this generic via SetRam(...
	SYS_FatalExit("CDebuggerApi::SaveBinary: not implemented");
}

int CDebuggerApi::LoadBinary(u16 fromAddr, char *filePath)
{
	// TODO: we can implement this generic via SetRam(...
	SYS_FatalExit("CDebuggerApi::LoadBinary: not implemented");
	return -1;
}

void CDebuggerApi::ShowMessage(const char *text)
{
	guiMain->ShowMessage((char*)text);
}

void CDebuggerApi::BlitText(const char *text, float posX, float posY, float fontSize)
{
	CSlrFont *font = viewC64->fontDisassembly;
	font->BlitText((char*)text, posX, posY, -1, fontSize);
}

void CDebuggerApi::AddView(CGuiView *view)
{
	guiMain->LockMutex();
	guiMain->AddView(view);
	debugInterface->AddView(view);
	guiMain->UnlockMutex();
}

