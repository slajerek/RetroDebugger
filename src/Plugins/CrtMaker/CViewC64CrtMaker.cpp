#include "CViewC64.h"
#include "CViewC64CrtMaker.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "CColorsTheme.h"
#include "C64SettingsStorage.h"
#include "SYS_Main.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CDebugInterfaceC64.h"
#include "MTH_Random.h"
#include "VID_ImageBinding.h"
#include "CViewMemoryMap.h"
#include "CGuiMain.h"
#include "CGuiEvent.h"

#include <string>
#include <sstream>
#include "hjson.h"

#include "C64DebuggerPluginCrtMaker.h"

#define ASSEMBLE(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); plugin->Assemble(assembleTextBuf);
//#define A(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); plugin->api->Assemble64TassAddLine(assembleTextBuf);
#define A plugin->api->Assemble64TassAddLine
#define PUT(v) plugin->PutDataByte(v);
#define PC addrAssemble

char *crtMakerConfigFilePath = NULL;

CViewC64CrtMaker::CViewC64CrtMaker(float posX, float posY, float posZ, float sizeX, float sizeY, C64DebuggerPluginCrtMaker *plugin)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CRT maker";
	
	this->plugin = plugin;
	this->api = plugin->api;
	
	font = viewC64->fontCBMShifted;
	fontScale = 1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
//	// TODO: create CSlrImage(width, height, type) that allocs imagedata with raster^2 and does not delete it after binding so can be reused
//	imageDataScreen = new CImageData(MAX_COLUMNS*8, MAX_ROWS*16, IMG_TYPE_RGBA);
//	imageDataScreen->AllocImage(false, true);
//
//	imageScreen = new CSlrImage(true, false);
//	imageScreen->LoadImage(imageDataScreen, RESOURCE_PRIORITY_STATIC, false);
//	imageScreen->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
//	VID_PostImageBinding(imageScreen, NULL);
	
	// this is real imgui window
	imGuiNoWindowPadding = false;
}

CViewC64CrtMaker::~CViewC64CrtMaker()
{
}
//	http://unusedino.de/ec64/technical/aay/c64/zp01.htm
//      00110xxx
// 37   00110111| Cart.+Basic |    I/O    | Kernal ROM |
// 35   00110101|     RAM     |    I/O    |    RAM     |
// 34   00110100|     RAM     |    RAM    |    RAM     |
// 33   00110011| Cart.+Basic | Char. ROM | Kernal ROM |

bool CViewC64CrtMaker::ReadConfigFromFile(char *hjsonFilePath)
{
//	LOGD("CViewC64CrtMaker::ReadConfigFromFile: %s", hjsonFilePath);

	SYS_Print("** CrtMaker read config file: %s", hjsonFilePath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(hjsonFilePath);
	
	if (!file->Exists())
	{
		LOGError("CrtMaker config file does not exist: %s", hjsonFilePath);
		delete file;
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	byteBuffer->ForwardToEnd();
	byteBuffer->PutU8(0x00);
	byteBuffer->Rewind();
	
	char *hjsonData = (char*)byteBuffer->data;

	std::stringstream ss;
	ss.str(hjsonData);
	
	LOGD("...parse");
	try
	{
		Hjson::Value hjsonRoot;
		ss >> hjsonRoot;
		
		Hjson::Value hValue;
		const char *hStr;
		
		hValue = hjsonRoot["cartName"]; hStr = static_cast<const char*>(hValue);
		cartName = STRALLOC(hStr);
		
		hValue = hjsonRoot["cartOutPath"]; hStr = static_cast<const char*>(hValue);
		cartOutPath = STRALLOC(hStr);

		hValue = hjsonRoot["rootFolderPath"]; hStr = static_cast<const char*>(hValue);
		rootFolderPath = STRALLOC(hStr);

		hValue = hjsonRoot["buildFilePath"]; hStr = static_cast<const char*>(hValue);
		buildFilePath = STRALLOC(hStr);

		hValue = hjsonRoot["binFilesPath"]; hStr = static_cast<const char*>(hValue);
		binFilesPath = STRALLOC(hStr);

		hValue = hjsonRoot["exoFilesPath"]; hStr = static_cast<const char*>(hValue);
		exoFilesPath = STRALLOC(hStr);

		hValue = hjsonRoot["exomizerPath"]; hStr = static_cast<const char*>(hValue);
		exomizerPath = STRALLOC(hStr);

		hValue = hjsonRoot["exomizerParams"]; hStr = static_cast<const char*>(hValue);
		exomizerParams = STRALLOC(hStr);

		hValue = hjsonRoot["javaPath"]; hStr = static_cast<const char*>(hValue);
		javaPath = STRALLOC(hStr);

		hValue = hjsonRoot["kickAssJarPath"]; hStr = static_cast<const char*>(hValue);
		kickAssJarPath = STRALLOC(hStr);

		hValue = hjsonRoot["kickAssParams"]; hStr = static_cast<const char*>(hValue);
		kickAssParams = STRALLOC(hStr);

		hValue = hjsonRoot["decrunchBinPath"]; hStr = static_cast<const char*>(hValue);
		decrunchBinPath = STRALLOC(hStr);

	}
	catch(const Hjson::syntax_error& e)
	{
		LOGError("CViewC64CrtMaker::ReadConfigFromFile error: %s", e.what());
		SYS_PrintError("Can't read config file: %s", e.what());
		delete byteBuffer;
		return false;
	}
	catch(const std::exception& e)
	{
		LOGError("CViewC64CrtMaker::ReadConfigFromFile error: %s", e.what());
		SYS_PrintError("Can't read config file: %s", e.what());
		delete byteBuffer;
		return false;
	}

	delete byteBuffer;

	SYS_Print("** CrtMaker config:");
	SYS_Print("cartName=%s", cartName);
	SYS_Print("cartOutPath=%s", cartOutPath);
	SYS_Print("rootFolderPath=%s", rootFolderPath);
	SYS_Print("buildFilePath=%s", buildFilePath);
	SYS_Print("binFilesPath=%s", binFilesPath);
	SYS_Print("exoFilesPath=%s", exoFilesPath);
	SYS_Print("exomizerPath=%s", exomizerPath);
	SYS_Print("exomizerParams=%s", exomizerParams);
	SYS_Print("javaPath=%s", javaPath);
	SYS_Print("kickAssJarPath=%s", kickAssJarPath);
	SYS_Print("kickAssParams=%s", kickAssParams);

	if (!SYS_FileExists(buildFilePath))
	{
		SYS_PrintError("Build file does not exist: %s", buildFilePath);
		return false;
	}
	
	return true;
}

bool CViewC64CrtMaker::ReadBuildFromFile(char *hjsonFilePath)
{
//	LOGD("CViewC64CrtMaker::ReadBuildFromFile: %s", hjsonFilePath);
	SYS_Print("** Read build file");

	CSlrFileFromOS *file = new CSlrFileFromOS(hjsonFilePath);
	
	if (!file->Exists())
	{
		SYS_PrintError("File does not exist: %s", hjsonFilePath);

		delete file;
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	byteBuffer->ForwardToEnd();
	byteBuffer->PutU8(0x00);
	byteBuffer->Rewind();
	
	char *hjsonData = (char*)byteBuffer->data;

	std::stringstream ss;
	ss.str(hjsonData);
	
	LOGD("...parse");
		
	char *buf = SYS_GetCharBuf();
	try
	{
		Hjson::Value hjsonRoot;
		ss >> hjsonRoot;
		
		LOGD("buildFiles=");
		Hjson::Value buildFiles = hjsonRoot["buildFiles"];
		
		for (int index = 0; index < int(buildFiles.size()); ++index)
		{
			Hjson::Value fileToBuild = buildFiles[index];
			
			const char *fileToBuildPath = static_cast<const char*>(fileToBuild);
			LOGD("fileToBuildPath='%s'", fileToBuildPath);
			
			sprintf(buf, "%s%c%s", rootFolderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, fileToBuildPath);
			SYS_FixFileNameSlashes(buf);
			
			LOGD("fullPath='%s'", buf);

			CCrtMakerFile *dataFile = new CCrtMakerFile(buf, fileToBuildPath);
			AddFile(dataFile);
		}
	}
	catch(const Hjson::syntax_error& e)
	{
		LOGError("CViewC64CrtMaker::ReadBuildFromFile error: %s", e.what());
		delete byteBuffer;
		return false;
	}

	SYS_ReleaseCharBuf(buf);
	
	delete byteBuffer;
	return true;
}

bool CViewC64CrtMaker::ProcessFiles()
{
	char *buf = SYS_GetCharBuf();
	
	for (std::list<CCrtMakerFile *>::iterator it = files.begin(); it != files.end(); it++)
	{
		CCrtMakerFile *file = *it;
		
		SYS_Print("** File %s", file->displayName);
		
		if (file->type == CCrtMakerFileType::TypeEXO)
		{
			// file is ready to be included, do nothing
			file->exoFilePath = STRALLOC(file->filePath);
			SYS_FixFileNameSlashes(exoFilesPath);
		}
		else if (file->type == CCrtMakerFileType::TypeSID)
		{
			sprintf(buf, "%s%c%s.prg", binFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->prgFilePath = STRALLOC(buf);

			sprintf(buf, "%s%c%s.exo", exoFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->exoFilePath = STRALLOC(buf);

			bool needsExomize = true;
			if (SYS_FileExists(file->exoFilePath))
			{
				long dateSid = SYS_GetFileModifiedTime(file->filePath);
				long dateExo = SYS_GetFileModifiedTime(file->exoFilePath);

				if (dateExo >= dateSid)
				{
					needsExomize = false;
				}
			}
			
			u8 *sidData;
			u16 fromAddr, toAddr, initAddr, playAddr;
			if (pluginCrtMaker->api->LoadSID(file->filePath, &fromAddr, &toAddr, &initAddr, &playAddr, &sidData))
			{
				// TODO: confirm if the toAddr-fromAddr is correct
				file->size = toAddr - fromAddr;
				
				if (file->destinationAddr < 1)
				{
					file->destinationAddr = fromAddr;
				}
				
				if (needsExomize)
				{
					SYS_Print("Loaded SID from %04x to %04x: %s", file->destinationAddr, file->destinationAddr + file->size, file->filePath);

					CByteBuffer *byteBuffer = new CByteBuffer();
					byteBuffer->PutByte(fromAddr & 0x00FF);
					byteBuffer->PutByte((fromAddr & 0xFF00) >> 8);
					byteBuffer->PutBytes(sidData, file->size);
									
					byteBuffer->storeToFile(file->prgFilePath);
					
					if (ExomizeFile(file) == false)
					{
						SYS_PrintError("Can't exomize SID file: %s", file->filePath);
						return false;
					}
				}
				else
				{
					LOGD("... sid already converted");
					SYS_PrintError("Already converted: %s (exo: %s)", file->prgFilePath, file->exoFilePath);
				}
			}
			else
			{
				SYS_PrintError("Can't load SID file: %s", file->filePath);
				return false;
			}
		}
		else if (file->type == CCrtMakerFileType::TypePRG)
		{
			file->prgFilePath = STRALLOC(file->filePath);
			SYS_FixFileNameSlashes(file->prgFilePath);

			sprintf(buf, "%s%c%s.exo", exoFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->exoFilePath = STRALLOC(buf);
			
			if (ExomizeFile(file) == false)
			{
				SYS_PrintError("Can't exomize SID file: %s", file->filePath);
				return false;
			}
		}
		else if (file->type == CCrtMakerFileType::TypeASM)
		{
			sprintf(buf, "%s%c%s.prg", binFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->prgFilePath = STRALLOC(buf);

			sprintf(buf, "%s%c%s.exo", exoFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->exoFilePath = STRALLOC(buf);

			AssembleFile(file);

			if (ExomizeFile(file) == false)
			{
				SYS_PrintError("Can't exomize PRG file: %s", file->prgFilePath);
				return false;
			}
		}
		else if (file->type == CCrtMakerFileType::TypeBIN)
		{
			SYS_FatalExit("CCrtMakerFileType::BIN not supported yet");
		}
		
		// read exomized data
		CByteBuffer *byteBuffer = new CByteBuffer(file->exoFilePath);
		
		if (byteBuffer->IsEmpty())
		{
			delete byteBuffer;
			SYS_PrintError("Can't find file: %s", file->exoFilePath);
			SYS_ReleaseCharBuf(buf);
			return false;
		}

		file->size = byteBuffer->length;
		file->data = new u8[file->size];
		for (int i = 0; i < file->size; i++)
		{
			file->data[i] = byteBuffer->GetU8();
		}

		delete byteBuffer;
	}

	SYS_ReleaseCharBuf(buf);
	return true;
}

bool CViewC64CrtMaker::AssembleFile(CCrtMakerFile *file)
{
	LOGD("CViewC64CrtMaker::AssembleFile: %s to %s", file->filePath, file->prgFilePath);
	
	if (SYS_FileExists(file->prgFilePath))
	{
		long dateAsm = SYS_GetFileModifiedTime(file->filePath);
		long datePrg = SYS_GetFileModifiedTime(file->prgFilePath);

		if (datePrg >= dateAsm)
		{
			LOGD("... already compiled");
			SYS_Print("Already compiled: %s", file->displayName);
			
			file->UpdateDestinationAddrFromPrg();
			return true;
		}
	}
	
	char *buf = SYS_GetCharBuf();
	
	sprintf(buf, "\"%s\" -jar \"%s\" -odir \"%s\" %s \"%s\"",
			javaPath, kickAssJarPath, binFilesPath, kickAssParams, file->filePath);
	
	LOGD("Assemble: %s", buf);
	int terminationCode;
	const char *ret = SYS_ExecSystemCommand(buf, &terminationCode);
	
	SYS_Print("%s", ret);

	if (terminationCode == 0)
	{
		file->UpdateDestinationAddrFromPrg();
	}
	
	SYS_ReleaseCharBuf(buf);
	return (terminationCode == 0);
}

bool CViewC64CrtMaker::ExomizeFile(CCrtMakerFile *file)
{
	LOGD("CViewC64CrtMaker::ExomizeFile: %s to %s", file->prgFilePath, file->exoFilePath);

	if (SYS_FileExists(file->exoFilePath))
	{
		long datePrg = SYS_GetFileModifiedTime(file->prgFilePath);
		long dateExo = SYS_GetFileModifiedTime(file->exoFilePath);

		if (dateExo >= datePrg)
		{
			LOGD("... already exomized");
			SYS_Print("Already exomized: %s", file->exoFilePath);
			return true;
		}
	}

	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%s level %s %s -o %s",
			exomizerPath, file->prgFilePath, exomizerParams, file->exoFilePath);
		
	int terminationCode;
	const char *ret = SYS_ExecSystemCommand(buf, &terminationCode);
//	LOGD("ExomizeFile ret=%s", ret);

	SYS_Print("%s", ret);

	SYS_ReleaseCharBuf(buf);
	return (terminationCode == 0);
}

void CViewC64CrtMaker::Run()
{
	LOGD("CViewC64CrtMaker::Run");
	char *assembleTextBuf = SYS_GetCharBuf();

	if (crtMakerConfigFilePath == NULL)
	{
		LOGError("CrtMaker config file not set, skipping");
		SYS_PrintError("** CrtMaker config file not set, skipping");
		return;
	}
	
	if (ReadConfigFromFile(crtMakerConfigFilePath) == false)
	{
		return;
	}
	
	if (ReadBuildFromFile(buildFilePath) == false)
	{
		return;
	}
	
	if (ProcessFiles() == false)
	{
		return;
	}
	
	SYS_Print("** Making cartridge image");
	codeEntryPoint = 0x2B00;
	CCrtMakerFile *firstFile = *(files.begin());
	if (firstFile->destinationAddr != -1)
	{
		codeEntryPoint = firstFile->destinationAddr;
		SYS_Print("Code entry point is  %04x", codeEntryPoint);
	}

	SYS_Sleep(100);
	api->debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
	
	cartImageOffset = 0x8000;
	addrFileTables = 0x8009;
	
	// setup file tables
	int numFiles = files.size();

	addrFileTableBank 	= addrFileTables;
	addrFileTableAddrL  = addrFileTables + numFiles*1;
	addrFileTableAddrH 	= addrFileTables + numFiles*2;
	
	// we can put any other code here
	addrCodeStart 		= addrFileTables + numFiles*3;
	

	AddDepackerFramework();
	
	
	// create CRT image
	cartSize = 512 * 1024;	// 512kB, 64 banks x 8192 bytes
	bankSize = 8192;
	numBanks = cartSize/bankSize;
	cartImage = new u8[cartSize];

	// debug
//	// fill with bank number
//	for (int bankNumber = 0; bankNumber < numBanks; bankNumber++)
//	{
//		int bankAddr = bankNumber * bankSize;
//		for (int i = 0; i < bankSize; i++)
//		{
//			cartImage[bankAddr + i] = bankNumber*0x04;
//		}
//	}
	
//#define DUMMY_CRT_INIT
	
	relocCodeStart = 0;
	
	// fill with zero
	memset(cartImage, 0, cartSize);
	
	api->AddCrtEntryPoint(cartImage, addrCodeStart, addrCodeStart);

	int codeStartAddr, codeSize;
	AddInitCode(&codeStartAddr, &codeSize);

	relocCodeStart = codeStartAddr + codeSize;
	LOGD("relocCodeStart=%04x", relocCodeStart);
	
	int addr = relocCodeStart - cartImageOffset;
	LOGD(" $0200 => %04x", addr);
	
	int decrunchCodeEnd = decrunchCodeLen + 0x0200;
	for (int i = 0x0200; i < decrunchCodeEnd; i++)
	{
		u8 v = api->GetByteFromRamC64(i);
		cartImage[addr] = v;
		addr++;
	}
	
	// addr is now:
	//addr = relocCodeStart + decrunchCodeLen - cartImageOffset;

	LOGD(" $0100 => %04x", addr);
	for (int i = 0; i < depackFrameworkLen; i++)
	{
		u8 v = api->GetByteFromRamC64(i + 0x0100);
		cartImage[addr] = v;
		addr++;
	}
	
	LOGD("INIT CODE END AT %04x", addr + cartImageOffset);
	
	// we can put data here
	u16 addrDataStart = addr;

	///
	///
	
	// pass-2 assemble, this time with correct values
	this->AddInitCode(&codeStartAddr, &codeSize);

	LOGD("codeStartAddr=%04x codeEnd=%04x", codeStartAddr, codeStartAddr + codeSize);
	
	LOGD("addrDataStart=%04x", addrDataStart);
	
	///
	// add files data
	addrFileTableBank -= cartImageOffset;
	addrFileTableAddrL -= cartImageOffset;
	addrFileTableAddrH -= cartImageOffset;
//	LOGD("addrFileTableBank   = %04x", addrFileTableBank);
//	LOGD("addrFileTableAddrL  = %04x", addrFileTableAddrL);
//	LOGD("addrFileTableAddrH  = %04x", addrFileTableAddrH);
//	LOGD("addrDataStart       = %04x", addrDataStart);

	SYS_Print("addrFileTableBank  = %05x", addrFileTableBank + cartImageOffset);
	SYS_Print("addrFileTableAddrL = %05x", addrFileTableAddrL + cartImageOffset);
	SYS_Print("addrFileTableAddrH = %05x", addrFileTableAddrH + cartImageOffset);
	SYS_Print("addrDataStart      = %05x", addrDataStart + cartImageOffset);

	addr = addrDataStart;

	SYS_Print("");
	SYS_Print("File#|crtaddr| bank#| baddr| size | dest | file name");
	SYS_Print("-----+-------+------+------+------+------+------------------------------------");
	int fileId = 0;
	for (std::list<CCrtMakerFile *>::iterator it = files.begin(); it != files.end(); it++)
	{
		CCrtMakerFile *file = *it;
		file->fileId = fileId;
		file->sourceCartImageAddr = addr;
		file->sourceBankNum = floor(addr / bankSize);
		file->sourceBankAddr = file->sourceCartImageAddr - (file->sourceBankNum * bankSize) + cartImageOffset;

//		LOGD("adding fileId #%04x | %05x | bank #%04x bankAddr %04x size %04x | %04x | %s",
//			 file->fileId, file->sourceCartImageAddr,
//			 file->sourceBankNum, file->sourceBankAddr, file->size,
//			 file->destinationAddr, file->displayName);

		SYS_Print("%04x | %05x | %04x | %04x | %04x | %04x | %s",
			 file->fileId, file->sourceCartImageAddr,
			 file->sourceBankNum, file->sourceBankAddr, file->size,
			 file->destinationAddr, file->displayName);

		cartImage[addrFileTableBank  + file->fileId] =  file->sourceBankNum;
		cartImage[addrFileTableAddrL + file->fileId] = (file->sourceBankAddr & 0x00FF);
		cartImage[addrFileTableAddrH + file->fileId] = (file->sourceBankAddr & 0xFF00) >> 8;

		for (int i = 0; i < file->size; i++)
		{
			cartImage[addr] = file->data[i];
			addr++;
		}

		fileId++;
	}

	SYS_Print("-----+-------+------+------+------+------+------------------------------------");

	// CREATE CARTRIDGE
	
	crtBuffer = api->MakeCrt(cartName, cartSize, bankSize, cartImage);
	crtBuffer->storeToFile(cartOutPath);
	
	SYS_Print("Cart file stored to: %s", cartOutPath);
	SYS_Print("** CrtMaker completed **");

	delete [] cartImage;

	api->debugInterface->HardReset();
	api->debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);

	SYS_Sleep(300);

	// run the cart
//	api->LoadCRT(crtBuffer);
	api->LoadCRT(cartOutPath);

//	api->ClearRAM(0x0200, 0xFFFF, 0);
//	AddDepackerFramework();

	/*
	api->Assemble64TassClearBuffer();

	A("			*=$2B00		");
	A("songid	LDY #$01	");
	A("			JSR $0100	");
	A("			LDA #0		");
	A("			JSR $0400	");
	A("b4		LDA $D012	");
	A("			CMP #$72	");
	A("			BNE b4		");
	A("			INC $D020	");
	A("			JSR $0403	");
	A("			DEC $D020	");
//	A("			LDA $DC00	");
//	A("			CMP #127	");
	A("			LDA #$7F	");
	A("			STA $DC00	");
	A("			LDA $DC01	");
	A("			AND #$10	");
	A("			BNE b4		");
	A("			LDA #0		");
	A("			STA $D418	");
	A("			INC songid+1	");
	A("			LDA songid+1	");
	A("			CMP #$0B	");
	A("			BNE songid	");
	A("			LDA #1		");
	A("			STA songid+1	");
	A("			JMP songid		");
//	A("			JMP *");
	
	
	int entryCodeStartAddr, entryCodeSize;
	buf = api->Assemble64Tass(&entryCodeStartAddr, &entryCodeSize);
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutU8(entryCodeStartAddr & 0x00FF);
	byteBuffer->PutU8((entryCodeStartAddr & 0xFF00) >> 8);

	addr = entryCodeStartAddr;
	for (int i = 0; i < entryCodeSize; i++)
	{
		api->SetByteToRam(addr, buf[i]);
		byteBuffer->PutU8(buf[i]);
		addr++;
	}
	
	byteBuffer->storeToFile(new CSlrString("/Users/mars/Desktop/entry-2b00.prg"));
	
	delete byteBuffer;
	free(buf);
	 */

	
	SYS_ReleaseCharBuf(assembleTextBuf);
}

void CViewC64CrtMaker::AddInitCode(int *codeStartAddr, int *codeSize)
{
	// init code
	api->Assemble64TassClearBuffer();
	A("			* = $%04x", addrCodeStart);

	A("			SEI			");
	A("			LDA #$00	");
	A("			STA $D020	");
	A("			STA $D021	");
	A("			STA $D011	");
	A("			JSR $FDA3	");	// prepare IRQ
	A("			JSR $FD15	");	// init I/O
//	A("			JSR $FF5B	");	// init video
	
	A("			LDY #0		");
	A("b1		LDA $%04x,Y	", relocCodeStart);
	A("			STA $0200,Y	");
	A("			DEY			");
	A("			BNE b1		");
	A("			LDY #$%02x	", ((decrunchCodeLen-0x0100) & 0xFF) + 1);
	A("b2		LDA $%04x,Y	", relocCodeStart + 0x0100-1);
	A("			STA $02FF,Y	");
	A("			DEY			");
	A("			BNE b2		");
	A("			LDY #$%02x	", depackFrameworkLen+1);
	A("b3		LDA $%04x,Y	", relocCodeStart + decrunchCodeLen-1);
	A("			STA $00FF,Y	");
	A("			DEY			");
	A("			BNE b3		");
	
	// //////////////// FIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIX
#if defined(DUMMY_CRT_INIT)
	A("			JMP *		");
#endif
	
//#else
	// return to the code entry point
	A("			LDA #$%02x	", ((codeEntryPoint-1) & 0xFF00) >> 8);
	A("			PHA			");
	A("			LDA #$%02x	",  (codeEntryPoint-1) & 0x00FF);
	A("			PHA			");
	A("			LDY #0		");
	A("			JMP $0100	");
//#endif

	u8 *buf = api->Assemble64Tass(codeStartAddr, codeSize, NULL, true);
	
	int addr = *codeStartAddr - cartImageOffset;
	for (int i = 0; i < *codeSize; i++)
	{
		cartImage[addr] = buf[i];
		addr++;
	}
	free(buf);
}

void CViewC64CrtMaker::AddDepackerFramework()
{
	//
	// depacker framework
	api->Assemble64TassClearBuffer();

	// LDA #addrL  LDX #addrH  JSR $0200
	decrunchCodeLen = api->LoadBinary(0x0200, decrunchBinPath);
	LOGD("decrunchCodeLen=%d %04x", decrunchCodeLen, decrunchCodeLen);
	
	depackFrameworkAddrLoadFile = 0x0100; //0x0200 + crtCodeStartAddrLoadFile;
	
	u16 addrDoDecrunch = 0x0318;
		
	//
	///
	A("			* = $%04x", depackFrameworkAddrLoadFile);

	// Y = file num
	
//	A("			SEI			")
	
//	A("			LDY #0		");
	
	A("			LDA $01		");
	A("			STA ret01+1	");

	A("			LDA #$37	");
	A("			STA $01		");

	A("			LDA #$00	");
	A("			STA $DE00	");

	A("			LDA $%04x,Y	", addrFileTableBank);
	A("			STA bankSelect+1");		// can be replaced with PHA
	A("			LDA $%04x,Y	", addrFileTableAddrL);
	A("			TAX			");
	A("			LDA $%04x,Y	", addrFileTableAddrH);
	A("			TAY			");
	
	A("bankSelect	LDA #00		");		// can be replaced with PLA
	A("			STA $DE00	");
	
//	A("			LDX #$00");
//	A("			LDY #$40");
	
	A("			JSR $%04x	", addrDoDecrunch);
	
//	A("			LDA #$34	");
//	A("			STA $01		");
//	A("			JMP *");

	A("ret01	LDA #0		");
	A("			STA $01		");
//	A("			CLI			");
	A("			RTS			");
	
//	A("			LDA #$EA	"); // 2
//	A("			STA $0348	"); // 3
//	A("			STA $0349	"); // 3
//	A("			RTS			"); // 1
//	A("			LDA #$C6	"); // 2
//	A("			STA $0348	"); // 3
//	A("			LDA #$01	"); // 2
//	A("			STA $0349	"); // 3
//	A("			RTS			"); // 1
//								// = 20

	A("			LDX #$EA	"); // 2
	A("			LDA #$EA	"); // 2
	A("			BNE store1	"); // 2
	A("			LDX #$C6	"); // 2
	A("			LDA #$01	"); // 2
	A("store1	STX $0348	"); // 3
	A("			STA $0349	"); // 3
	A("			RTS			"); // 1
								// = 17
	
	
	int codeStartAddr;
	u8 *buf = api->Assemble64Tass(&codeStartAddr, &depackFrameworkLen, NULL, true);
	CopyToRam(codeStartAddr, depackFrameworkLen, buf);
	free(buf);
}

void CViewC64CrtMaker::DoLogic()
{
	
}

void CViewC64CrtMaker::Render()
{
}

void CViewC64CrtMaker::RenderImGui()
{
//	LOGD("CViewC64CrtMaker::RenderImGui");
		
//	float w = (float)imageDataScreen->width;
//	float h = (float)imageDataScreen->height;
//
//	this->imGuiWindowAspectRatio = w/h;
//	this->imGuiWindowKeepAspectRatio = true;
//
	PreRenderImGui();
	
	ImGui::Text("CRT maker");
	

	
	
//	ImGui::InputScalar("Zak addr", ImGuiDataType_::ImGuiDataType_U16, &pluginSidWizard->tuneLoadAddr, NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
	
	
//
////		for (int x = 0; x < imageDataScreen->width; x++)
////		{
////			for (int y = 0; y < imageDataScreen->height; y++)
////			{
////				float wx = ((float)x / (float)imageDataScreen->width) * 255.0f;
////				float wy = ((float)y / (float)imageDataScreen->height) * 255.0f;
////
////				imageDataScreen->SetPixelResultRGBA(x, y, (int)wx, (int)wy, 0, 255);
////			}
////		}
//
//	imageScreen->ReBindImageData(imageDataScreen);
//
//	// blit texture of the screen
//	Blit(imageScreen,
//		 posX,
//		 posY, -1,
//		 sizeX,
//		 sizeY);
//	//, 0, 0, 1, 1); //,
////		 0.0f, 0.0f, screenTexEndX, screenTexEndY);
//
//
////	ImGui::Text("Test");
	
	PostRenderImGui();
	
//	LOGD("CViewC64CrtMaker::RenderImGui done");
}

void CViewC64CrtMaker::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewC64CrtMaker::DoTap(float x, float y)
{
	LOGG("CViewC64CrtMaker::DoTap:  x=%f y=%f", x, y);
	
	return true; //CGuiView::DoTap(x, y);
}

bool CViewC64CrtMaker::DoFinishTap(float x, float y)
{
	LOGG("CViewC64CrtMaker::DoFinishTap: %f %f", x, y);

	return true; //CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64CrtMaker::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64CrtMaker::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64CrtMaker::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64CrtMaker::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64CrtMaker::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64CrtMaker::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64CrtMaker::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64CrtMaker::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64CrtMaker::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64CrtMaker::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64CrtMaker::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64CrtMaker::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewC64CrtMaker::KeyDown: keyCode=%d");

	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64CrtMaker::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64CrtMaker::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewC64CrtMaker::KeyUp: keyCode=%d", keyCode);
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64CrtMaker::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64CrtMaker::ActivateView()
{
	LOGG("CViewC64CrtMaker::ActivateView()");
}

void CViewC64CrtMaker::DeactivateView()
{
	LOGG("CViewC64CrtMaker::DeactivateView()");
}

// ------

void CViewC64CrtMaker::AddFile(CCrtMakerFile *file)
{
	files.push_back(file);
}

u8 *CViewC64CrtMaker::Assemble64Tass(int *codeStartAddr, int *codeSize)
{
	return plugin->api->Assemble64Tass(codeStartAddr, codeSize, NULL, false);
}

void CViewC64CrtMaker::CopyToRam(int codeStartAddr, int codeSize, u8 *buf)
{
	int addr = codeStartAddr;
	for (int i = 0; i < codeSize; i++)
	{
		plugin->api->SetByteToRam(addr, buf[i]);
		addr++;
	}
}
