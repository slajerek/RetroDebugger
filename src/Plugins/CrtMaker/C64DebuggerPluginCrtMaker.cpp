#include "C64DebuggerPluginCrtMaker.h"
#include "CViewC64CrtMaker.h"
#include "GFX_Types.h"
#include "CSlrFileFromOS.h"
#include "C64Tools.h"
#include <map>

// Usage: when crt is packed then you can get 'file' by calling in your code
//  LDY #<file num>  JSR $0100

// .var Depack = $0100
// .var SetDepackWithIo = $0129				// $01 = $35 depack with i/o, e.g. directly to color ram
// .var SetDepackDirectToRam = $012F		// $01 = $34 depack direct to ram (default)
//
// the $02-$0C (incl.) addresses at zero page are reserved during depack
//
// 0100-03f8 are reserved for depacker framework, from 03f8 all up is free
// during cartridge read the kernal is on thus IRQ needs to run via 0314/0316 vectors
// but during ram store the kernal is off thus IRQ needs to run via FFFE/FFFF vectors
// also remember to store state of $01 in IRQ as depacker alternates between #$37 for read and #$34 for store
//
// you can use default handler:
// this is copy of default kernal's irq handler to mimic the same number of cycles and behavior
//
//	A("				LDA #<DefaultKernalIRQ");
//	A("				STA $fffe");
//	A("				LDA #>DefaultKernalIRQ");
//	A("				STA $ffff");
//	A("				LDA #<DefaultIRQ");
//	A("				STA $0314");
//	A("				STA $0316");
//	A("				LDA #>DefaultIRQ");
//	A("				STA $0315");
//	A("				STA $0317");

//  A("DefaultKernalIRQ		PHA");
//  A("						TXA");
//  A("						PHA");
//  A("						TYA");
//  A("						PHA");
//  A("						TSX");
//  A("						LDA $0104,x");
//  A("						AND #$10");
//  A("						BEQ irqKernal2");
//  A("irqKernal2     		JMP ($0314)");
//  A("DefaultIRQ      		LDA $01");
//  A("						STA irq1val01+1");
//  A("						LDA #$35");
//  A("						STA $01");
//
//  A("						JSR $%04x", songPlay);
//  A("						ASL $d019");
//  A("irq1val01:      		LDA #$00");
//  A("						STA $01");
//  A("						PLA");
//  A("						TAY");
//  A("						PLA");
//  A("						TAX");
//  A("						PLA");
//  A("						RTI");

#define ASSEMBLE(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); this->Assemble(assembleTextBuf);
#define A(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); api->Assemble64TassAddLine(assembleTextBuf);
#define PUT(v) this->PutDataByte(v);
#define PC addrAssemble

C64DebuggerPluginCrtMaker *pluginCrtMaker = NULL;
CViewC64CrtMaker *viewC64CrtMaker = NULL;

void PLUGIN_CrtMakerInit()
{
	if (pluginCrtMaker == NULL)
	{
		pluginCrtMaker = new C64DebuggerPluginCrtMaker();
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginCrtMaker);
	}
}

void PLUGIN_CrtMakerSetVisible(bool isVisible)
{
	if (pluginCrtMaker != NULL)
	{
		pluginCrtMaker->crtMaker->SetVisible(isVisible);
	}
}

C64DebuggerPluginCrtMaker::C64DebuggerPluginCrtMaker()
{
	pluginCrtMaker = this;
}

void C64DebuggerPluginCrtMaker::Init()
{
	LOGD("C64DebuggerPluginCrtMaker::Init");

	crtMaker = new CViewC64CrtMaker(0, 50, -1, 400, 300, this);

	//
	api->StartThread(this);
}

void C64DebuggerPluginCrtMaker::ThreadRun(void *data)
{
	SYS_Sleep(50);
	api->DetachEverything();
	
	api->AddView(crtMaker);
	
	crtMaker->Run();
}

void C64DebuggerPluginCrtMaker::DoFrame()
{
	// do anything you need after each emulation frame, vsync is here:

//	LOGD("C64DebuggerPluginCrtMaker::DoFrame finished");
}

u32 C64DebuggerPluginCrtMaker::KeyDown(u32 keyCode)
{
	if (keyCode == MTKEY_ARROW_UP)
	{
	}
	
	if (keyCode == MTKEY_ARROW_DOWN)
	{
	}
	
	if (keyCode == MTKEY_ARROW_LEFT)
	{
	}
	if (keyCode == MTKEY_ARROW_RIGHT)
	{
	}
	
	if (keyCode == MTKEY_SPACEBAR)
	{
//		api->SaveExomizerPRG(0x1000, 0x3000, 0x0F00, "out.prg");
	}
	
	return keyCode;
}

u32 C64DebuggerPluginCrtMaker::KeyUp(u32 keyCode)
{
	return keyCode;
}

///
void C64DebuggerPluginCrtMaker::Assemble(char *buf)
{
	//	LOGD("Assemble: %04x %s", addrAssemble, buf);
//	addrAssemble += api->Assemble(addrAssemble, buf);
	api->Assemble64TassAddLine(buf);
}

void C64DebuggerPluginCrtMaker::PutDataByte(u8 v)
{
	//	LOGD("PutDataByte: %04x %02x", addrAssemble, v);
	api->SetByteToRam(addrAssemble, v);
	addrAssemble++;
}

CCrtMakerFile::CCrtMakerFile(const char *filePath, const char *displayName)
{
	this->Init(filePath, displayName, -1);
}

CCrtMakerFile::CCrtMakerFile(const char *filePath, const char *displayName, int destinationAddr)
{
	this->Init(filePath, displayName, destinationAddr);
}

void CCrtMakerFile::Init(const char *filePath, const char *displayName, int destinationAddr)
{
	LOGD("CCrtMakerFile: filePath=%s displayName=%s destinationAddr=%d", filePath, displayName, destinationAddr);
	this->filePath = STRALLOC(filePath);
	this->displayName = STRALLOC(displayName); //SYS_GetFileNameWithExtensionFromFullPath(filePath);
	this->fileName = SYS_GetFileName(filePath);
	this->prgFilePath = NULL;
	this->exoFilePath = NULL;
	this->destinationAddr = destinationAddr;
	this->status = CCrtMakerFileStatus::StatusPending;
	
	this->size = 0;
	this->data = NULL;
	
	char *ext = SYS_GetFileExtension(filePath);
	for (int i = 0; i < strlen(ext); i++)
	{
		ext[i] = tolower(ext[i]);
	}
	
	if (!strcmp(ext, "prg"))
	{
		this->type = CCrtMakerFileType::TypePRG;
		
		this->prgFilePath = STRALLOC(this->filePath);
		
		UpdateDestinationAddrFromPrg();
		
//		this->size = byteBuffer->length-2;
//		this->data = new u8[size];
//		for (int i = 0; i < size; i++)
//		{
//			this->data[i] = byteBuffer->GetU8();
//		}
//
//		delete byteBuffer;
		
		LOGD("PRG from %04x to %04x: %s", this->destinationAddr, this->destinationAddr + size, filePath);
	}
	else if (!strcmp(ext, "sid"))
	{
		this->type = CCrtMakerFileType::TypeSID;
	}
	else
	{
		if (!strcmp(ext, "asm"))
		{
			this->type = CCrtMakerFileType::TypeASM;
		}
		else if (!strcmp(ext, "exo"))
		{
			this->type = CCrtMakerFileType::TypeEXO;
		}
		else
		{
			this->type = CCrtMakerFileType::TypeBIN;
		}

		/*
		// binary
		CByteBuffer *byteBuffer = new CByteBuffer(filePath);
		
		if (byteBuffer->IsEmpty())
		{
			delete byteBuffer;
			return;
		}

		size = byteBuffer->length;
		data = new u8[size];
		for (int i = 0; i < size; i++)
		{
			data[i] = byteBuffer->GetU8();
		}
		
		if (destinationAddr >= 0)
		{
			LOGD("Loaded file from %04x to %04x: %s", this->destinationAddr, this->destinationAddr + size, filePath);
		}
		else
		{
			if (type == CCrtMakerFileType::ASM)
			{
				LOGD("Loaded asm file size %d: %s", size, filePath);
			}
			else if (type == CCrtMakerFileType::EXO)
			{
				LOGD("Loaded EXO file size %04x: %s", size, filePath);
			}
			else
			{
				LOGD("Loaded file size %04x: %s", size, filePath);
			}
		}
		 */
	}
	
}

void CCrtMakerFile::UpdateDestinationAddrFromPrg()
{
	CByteBuffer *byteBuffer = new CByteBuffer(prgFilePath);
	
	if (byteBuffer->IsEmpty())
	{
		delete byteBuffer;
		return;
	}

	u16 b1 = byteBuffer->GetByte();
	u16 b2 = byteBuffer->GetByte();
			
	if (destinationAddr < 1)
	{
		u16 loadPoint = (b2 << 8) | b1;
		this->destinationAddr = loadPoint;
	}

	delete byteBuffer;
}

CCrtMakerFile::~CCrtMakerFile()
{
	STRFREE(filePath);
	STRFREE(displayName);
}


////////
