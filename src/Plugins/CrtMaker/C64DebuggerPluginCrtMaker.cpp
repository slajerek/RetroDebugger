#include "C64DebuggerPluginCrtMaker.h"
#include "CViewC64CrtMaker.h"
#include "GFX_Types.h"
#include "CSlrFileFromOS.h"
#include "C64Tools.h"
#include <map>

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
		pluginCrtMaker->view->SetVisible(isVisible);
	}
}

C64DebuggerPluginCrtMaker::C64DebuggerPluginCrtMaker()
{
	pluginCrtMaker = this;
}

void C64DebuggerPluginCrtMaker::Init()
{
	LOGD("C64DebuggerPluginCrtMaker::Init");

	view = new CViewC64CrtMaker(0, 50, -1, 400, 300, this);

	//
	api->StartThread(this);
}

void C64DebuggerPluginCrtMaker::ThreadRun(void *data)
{
	SYS_Sleep(50);
	api->DetachEverything();
	
	api->AddView(view);
	
	view->Run();
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
