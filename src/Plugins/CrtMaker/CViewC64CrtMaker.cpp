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
#include "CViewDataMap.h"
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
	
	font = viewC64->fontDefaultCBMShifted;
	fontScale = 1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	// default
	decrunchBinStartAddr = 0x0200;
	
	mutex = new CSlrMutex("CGuiViewMessages");
	AutoScroll = true;
	Clear();
}

CViewC64CrtMaker::~CViewC64CrtMaker()
{
}

// IO/ROM/RAM Setup, $01
//      00110xxx
// 37   00110111| Cart.+Basic |    I/O    | Kernal ROM |
// 35   00110101|     RAM     |    I/O    |    RAM     |
// 34   00110100|     RAM     |    RAM    |    RAM     |
// 33   00110011| Cart.+Basic | Char. ROM | Kernal ROM |

bool CViewC64CrtMaker::ReadConfigFromFile(char *hjsonFilePath)
{
//	LOGD("CViewC64CrtMaker::ReadConfigFromFile: %s", hjsonFilePath);

	Print("** CrtMaker read config file: %s", hjsonFilePath);
	
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
		PrintError("Can't read config file: %s", e.what());
		delete byteBuffer;
		return false;
	}
	catch(const std::exception& e)
	{
		LOGError("CViewC64CrtMaker::ReadConfigFromFile error: %s", e.what());
		PrintError("Can't read config file: %s", e.what());
		delete byteBuffer;
		return false;
	}

	delete byteBuffer;

	Print("** CrtMaker config:");
	Print("cartName=%s", cartName);
	Print("cartOutPath=%s", cartOutPath);
	Print("rootFolderPath=%s", rootFolderPath);
	Print("buildFilePath=%s", buildFilePath);
	Print("binFilesPath=%s", binFilesPath);
	Print("exoFilesPath=%s", exoFilesPath);
	Print("exomizerPath=%s", exomizerPath);
	Print("exomizerParams=%s", exomizerParams);
	Print("javaPath=%s", javaPath);
	Print("kickAssJarPath=%s", kickAssJarPath);
	Print("kickAssParams=%s", kickAssParams);

	if (!SYS_FileExists(buildFilePath))
	{
		PrintError("Build file does not exist: %s", buildFilePath);
		return false;
	}
	
	return true;
}

bool CViewC64CrtMaker::ReadBuildFromFile(char *hjsonFilePath)
{
//	LOGD("CViewC64CrtMaker::ReadBuildFromFile: %s", hjsonFilePath);
	Print("** Read build file");

	CSlrFileFromOS *file = new CSlrFileFromOS(hjsonFilePath);
	
	if (!file->Exists())
	{
		PrintError("File does not exist: %s", hjsonFilePath);

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
		
		Print("** File %s", file->displayName);
		
		if (file->type == CCrtMakerFileType::TypeEXO)
		{
			// file is ready to be included, do nothing
			file->exoFilePath = STRALLOC(file->filePath);
			SYS_FixFileNameSlashes(exoFilesPath);
			file->status = CCrtMakerFileStatus::StatusOK;
		}
		else if (file->type == CCrtMakerFileType::TypeSID)
		{
			sprintf(buf, "%s%c%s.prg", binFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->prgFilePath = STRALLOC(buf);

			sprintf(buf, "%s%c%s.exo", exoFilesPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, file->fileName);
			SYS_FixFileNameSlashes(buf);
			file->exoFilePath = STRALLOC(buf);

			file->status = CCrtMakerFileStatus::StatusLoad;

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
					Print("Loaded SID from %04x to %04x: %s", file->destinationAddr, file->destinationAddr + file->size, file->filePath);

					CByteBuffer *byteBuffer = new CByteBuffer();
					byteBuffer->PutByte(fromAddr & 0x00FF);
					byteBuffer->PutByte((fromAddr & 0xFF00) >> 8);
					byteBuffer->PutBytes(sidData, file->size);
									
					byteBuffer->storeToFile(file->prgFilePath);
					
					if (ExomizeFile(file) == false)
					{
						PrintError("Can't exomize SID file: %s", file->filePath);
						file->status = CCrtMakerFileStatus::StatusFailed;
						return false;
					}
				}
				else
				{
					LOGD("... sid already converted");
					PrintError("Already converted: %s (exo: %s)", file->prgFilePath, file->exoFilePath);
				}
			}
			else
			{
				PrintError("Can't load SID file: %s", file->filePath);
				file->status = CCrtMakerFileStatus::StatusFailed;
				return false;
			}
			file->status = CCrtMakerFileStatus::StatusOK;
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
				PrintError("Can't exomize PRG file: %s", file->filePath);
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
				PrintError("Can't exomize PRG file: %s", file->prgFilePath);
				file->status = CCrtMakerFileStatus::StatusFailed;
				return false;
			}
			file->status = CCrtMakerFileStatus::StatusOK;
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
			PrintError("Can't find file: %s", file->exoFilePath);
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
			Print("Already compiled: %s", file->displayName);
			
			file->UpdateDestinationAddrFromPrg();
			return true;
		}
	}
	
	file->status = CCrtMakerFileStatus::StatusAssemble;

	char *buf = SYS_GetCharBuf();
	
	sprintf(buf, "\"%s\" -jar \"%s\" -odir \"%s\" %s \"%s\"",
			javaPath, kickAssJarPath, binFilesPath, kickAssParams, file->filePath);
	
	LOGD("Assemble: %s", buf);
	int terminationCode;
	const char *ret = SYS_ExecSystemCommand(buf, &terminationCode);
	
	Print("%s", ret);

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
			Print("Already exomized: %s", file->exoFilePath);
			return true;
		}
	}

	file->status = CCrtMakerFileStatus::StatusExomize;
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%s level %s %s -o %s",
			exomizerPath, file->prgFilePath, exomizerParams, file->exoFilePath);
		
	int terminationCode;
	const char *ret = SYS_ExecSystemCommand(buf, &terminationCode);
//	LOGD("ExomizeFile ret=%s", ret);

	Print("%s", ret);

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
		PrintError("** CrtMaker config file not set, skipping");
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
	
	if (MakeCartridge() == false)
	{
		return;
	}

	// run the cart
	api->debugInterface->ResetHard();
	api->debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);

	SYS_Sleep(300);
	
//	api->LoadCRT(crtBuffer);
	api->LoadCRT(cartOutPath);

//	api->ClearRAM(0x0200, 0xFFFF, 0);
//	AddDepackerFramework();

	/* example code
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

bool CViewC64CrtMaker::MakeCartridge()
{
	Print("** Making cartridge image");
	codeEntryPoint = 0x2B00;
	CCrtMakerFile *firstFile = *(files.begin());
	if (firstFile->destinationAddr != -1)
	{
		codeEntryPoint = firstFile->destinationAddr;
	}

	Print("Code entry point is %04x", codeEntryPoint);

	SYS_Sleep(100);
	api->debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
	
	cartImageOffset = 0x8000;
	addrFileTables = 0x8009;
	
	// setup file tables
	size_t numFiles = files.size();

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
	LOGD(" $%04x => %04x", decrunchBinStartAddr, addr);
	
	int decrunchCodeEnd = decrunchBinStartAddr + decrunchCodeLen;
	for (int i = decrunchBinStartAddr; i < decrunchCodeEnd; i++)
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

	Print("addrFileTableBank  = %05x", addrFileTableBank + cartImageOffset);
	Print("addrFileTableAddrL = %05x", addrFileTableAddrL + cartImageOffset);
	Print("addrFileTableAddrH = %05x", addrFileTableAddrH + cartImageOffset);
	Print("addrDataStart      = %05x", addrDataStart + cartImageOffset);

	addr = addrDataStart;

	byteBufferMapText = new CByteBuffer();
	
	Print("");
	PrintMap("File#|crtaddr| bank#| baddr| size | dest | file name");
	PrintMap("-----+-------+------+------+------+------+------------------------------------");
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

		PrintMap("%04x | %05x | %04x | %04x | %04x | %04x | %s",
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
	PrintMap("-----+-------+------+------+------+------+------------------------------------");

	PrintMap("Cartridge data size: %d bytes (%dkB of 512kB)", addr, addr/1024);
	
	// CREATE CARTRIDGE
	
	crtBuffer = api->MakeCrt(cartName, cartSize, bankSize, cartImage);
	if (!crtBuffer->storeToFile(cartOutPath))
	{
		LOGError("Failed to store cartridge image to %s", cartOutPath);
		delete [] cartImage;
		return false;
	}
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%s.txt", cartOutPath);
	if (!byteBufferMapText->storeToFile(buf))
	{
		LOGError("Failed to store cartridge map to %s", buf);
		delete [] cartImage;
		return false;
	}
	SYS_ReleaseCharBuf(buf);
	
	Print("Cart file stored to: %s", cartOutPath);
	Print("** CrtMaker completed **");

	delete [] cartImage;

	return true;
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
	A("			STA $%04x,Y	", decrunchBinStartAddr);
	A("			DEY			");
	A("			BNE b1		");
	A("			LDY #$%02x	", ((decrunchCodeLen-0x0100) & 0xFF) + 1);
	A("b2		LDA $%04x,Y	", relocCodeStart + 0x0100-1);
	A("			STA $%04x,Y	", decrunchBinStartAddr + 0x00FF);
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
	// RTS will return to the code entry point
	A("			LDA #$%02x	", ((codeEntryPoint-1) & 0xFF00) >> 8);
	A("			PHA			");
	A("			LDA #$%02x	",  (codeEntryPoint-1) & 0x00FF);
	A("			PHA			");
	
	// read file #0 (main code)
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
	decrunchCodeLen = api->LoadBinary(decrunchBinStartAddr, decrunchBinPath);
	LOGD("decrunchCodeLen=%d %04x", decrunchCodeLen, decrunchCodeLen);
	
	depackFrameworkAddrLoadFile = 0x0100; //0x0200 + crtCodeStartAddrLoadFile;
	
	u16 addrDoDecrunch = decrunchBinStartAddr + 0x0118; //0x0318;
		
	//
	///
	A("			* = $%04x", depackFrameworkAddrLoadFile);

	// Y = file num
	
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

	// decrunch
	A("			JSR $%04x	", addrDoDecrunch);
	
	A("ret01	LDA #0		");
	A("			STA $01		");
	A("			RTS			");
	
	A("			LDA #$35");		// with io
	A("			STA $%04x", decrunchBinStartAddr + 0x0142);
	A("			RTS");
	A("			LDA #$34");		// without io
	A("			STA $%04x", decrunchBinStartAddr + 0x0142);
	A("			RTS");
	
	
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

void CViewC64CrtMaker::Clear()
{
	mutex->Lock();
	Buf.clear();
	LineOffsets.clear();
	LineOffsets.push_back(0);
	mutex->Unlock();
}

void CViewC64CrtMaker::Print(const char* fmt, ...) IM_FMTARGS(2)
{
	mutex->Lock();

	int old_size = Buf.size();
	va_list args;
	va_start(args, fmt);
	Buf.appendfv(fmt, args);
	Buf.append("\n");
	va_end(args);
	for (int new_size = Buf.size(); old_size < new_size; old_size++)
		if (Buf[old_size] == '\n')
			LineOffsets.push_back(old_size + 1);

	SYS_Print(fmt, args);
	
	mutex->Unlock();
}

void CViewC64CrtMaker::PrintError(const char* fmt, ...) IM_FMTARGS(2)
{
	mutex->Lock();

	int old_size = Buf.size();
	va_list args;
	va_start(args, fmt);
	Buf.append("Error: ");
	Buf.appendfv(fmt, args);
	Buf.append("\n");
	va_end(args);
	for (int new_size = Buf.size(); old_size < new_size; old_size++)
		if (Buf[old_size] == '\n')
			LineOffsets.push_back(old_size + 1);

	SYS_PrintError(fmt, args);

	mutex->Unlock();
}


void CViewC64CrtMaker::RenderImGui()
{
//	LOGD("CViewC64CrtMaker::RenderImGui");
		
	PreRenderImGui();
	
	if (ImGui::CollapsingHeader("Files", ImGuiTreeNodeFlags_DefaultOpen))
	{
		
		//	File#|status|crtaddr| bank#| baddr| size | dest | file name

		if (ImGui::BeginTable("##crtMaker", 8, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders))	//| ImGuiTableFlags_Sortable  TODO
		{
			u32 i = 0;
			
	//		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.65f)));

			ImGui::TableNextColumn();
			ImGui::Text("File#");
			ImGui::TableNextColumn();
			ImGui::Text("Status");
			ImGui::TableNextColumn();
			ImGui::Text("CrtAddr");
			ImGui::TableNextColumn();
			ImGui::Text("Bank#");
			ImGui::TableNextColumn();
			ImGui::Text("BankAddr");
			ImGui::TableNextColumn();
			ImGui::Text("Size");
			ImGui::TableNextColumn();
			ImGui::Text("Dest");
			ImGui::TableNextColumn();
			ImGui::Text("File name");

	//	   	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0);

			for (std::list<CCrtMakerFile *>::iterator it = files.begin(); it != files.end(); it++)
			{
				CCrtMakerFile *file = *it;

				int sourceBankNum = floor(file->sourceCartImageAddr / bankSize);
				int sourceBankAddr = file->sourceCartImageAddr - (file->sourceBankNum * bankSize) + cartImageOffset;

				ImGui::TableNextColumn();
				ImGui::Text("%04x", file->fileId);

				ImGui::TableNextColumn();
				ImGui::Text("%s", "EXOMIZING");

				ImGui::TableNextColumn();
				ImGui::Text("%05x", file->sourceCartImageAddr);
				
				ImGui::TableNextColumn();
				ImGui::Text("%04x", sourceBankNum);

				ImGui::TableNextColumn();
				ImGui::Text("%04x", sourceBankAddr);

				ImGui::TableNextColumn();
				ImGui::Text("%04x", file->size);

				ImGui::TableNextColumn();
				ImGui::Text("%04x", file->destinationAddr);

				ImGui::TableNextColumn();
				ImGui::Text("%s", file->displayName);

			}
			
			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			ImGui::Checkbox("Auto-scroll", &AutoScroll);
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clear)
			Clear();
		if (copy)
			ImGui::LogToClipboard();


		// render lines
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		
		mutex->Lock();
		
		if (Filter.IsActive())
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of
			// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
			for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				if (Filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}
		}
		else
		{
			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
			// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
			// within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
			// on your side is recommended. Using ImGuiListClipper requires
			// - A) random access into your data
			// - B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display
			// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
			// it possible (and would be recommended if you want to search through tens of thousands of entries).
			ImGuiListClipper clipper;
			clipper.Begin(LineOffsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					ImGui::TextUnformatted(line_start, line_end);
				}
			}
			clipper.End();
		}
		
		mutex->Unlock();
		
		ImGui::PopStyleVar();

		if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
	}
	
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
	mutex->Lock();
	files.push_back(file);
	mutex->Unlock();
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

void CViewC64CrtMaker::PrintMap(const char *format, ...)
{
	char *buffer = SYS_GetCharBuf();
	va_list args;
	
	va_start(args, format);
	vsnprintf(buffer, MAX_STRING_LENGTH-1, format, args);
	va_end(args);

//	LOGD("CByteBuffer::printf %s", buffer);
	byteBufferMapText->putBytes((uint8 *)buffer, 0, strlen(buffer));
	byteBufferMapText->putByte(0x0A);
	Print(buffer);

	SYS_ReleaseCharBuf(buffer);
}
