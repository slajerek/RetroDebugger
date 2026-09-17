#include "C64DebuggerPluginGalaxy.h"

#include "CViewC64CrtMaker.h"
#include "GFX_Types.h"
#include "CSlrFileFromOS.h"
#include "C64Tools.h"
#include "CViewC64.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicDisplay.h"
#include <map>

#define	VIC_BANK_ADDR				0xC000
#define BITMAP_ADDR					0xE000
#define SCREEN_ADDR					0xC800
#define SPRITE_POINTERS				(SCREEN_ADDR+0x03F8)


#define ASSEMBLE(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); this->Assemble(assembleTextBuf);
#define A(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); api->Assemble64TassAddLine(assembleTextBuf);
#define PUT(v) this->PutDataByte(v);
#define PC addrAssemble

C64DebuggerPluginGalaxy *pluginGalaxy = NULL;
CViewC64CrtMaker *viewGalaxyC64CrtMaker = NULL;

void PLUGIN_GalaxyInit()
{
	if (pluginGalaxy == NULL)
	{
		pluginGalaxy = new C64DebuggerPluginGalaxy();
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginGalaxy);
	}
}

void PLUGIN_GalaxySetVisible(bool isVisible)
{
	if (pluginGalaxy != NULL)
	{
		pluginGalaxy->crtMaker->SetVisible(isVisible);
	}
}

C64DebuggerPluginGalaxy::C64DebuggerPluginGalaxy()
{
	pluginGalaxy = this;
	assembleTextBuf = new char[1024];
}

C64DebuggerPluginGalaxy::~C64DebuggerPluginGalaxy()
{
	if (pluginGalaxy == this)
		pluginGalaxy = NULL;
}

void C64DebuggerPluginGalaxy::Init()
{
	C64DebuggerPluginCrtMaker::Init();
}

#define SLEEP_FACTOR 1.0

#define ACTION_RESET			0
#define ACTION_LOAD_NEXT_FILE	1
#define ACTION_SHOW_BITMAP		2
#define ACTION_SHOW_NUFFLI		3
#define ACTION_HIDE_NUFFLI		4
#define ACTION_HIDE_SCREEN		5
#define ACTION_DELAY			6
#define ACTION_SONG_FADEOUT		7

void C64DebuggerPluginGalaxy::ThreadRun(void *data)
{
	LOGD("C64DebuggerPluginGalaxy::ThreadRun");
	
	SYS_Sleep(50);
	api->DetachEverything();
	
	api->AddView(crtMaker);
	
	SYS_Sleep(500);

	// prepare RAM
	api->ClearRam(0x0200, 0x10000, 0x00);

//	int nufliDelay = 3;
	int nufliDelay = 1;

	forceRebuildImages = false;

	BACK_COLOR_ADDR  = 0x9000;
	BACK_SCREEN_ADDR = 0x9400;
	BACK_BITMAP_ADDR = 0x9800;

	char *buf = SYS_GetCharBuf();
	char *buf2 = SYS_GetCharBuf();
	
	CByteBuffer *byteBuffer = new CByteBuffer();

#define NUMBER_OF_HIRES	14
	
	hiresPngPath = "/Users/mars/develop/galaxy-marszruta/gfx/hires%02d.png";
	hiresPrgPath = "/Users/mars/develop/galaxy-marszruta/gfx/hires%02d.prg";
	hiresPrgName = "hires%02d.prg";
	nufliPrgPath = "/Users/mars/develop/galaxy-marszruta/nufli/g%02d.prg";
	nufliPrgName = "g%02d.prg";

	// convert images
	for (int i = 0; i <= 14; i++)
	{
		sprintf(buf,  hiresPngPath, i);
		sprintf(buf2, hiresPrgPath, i);

		if (!forceRebuildImages && SYS_FileExists(buf2))
		{
			continue;
		}

		bool isMulticolor = false;
		
//		if (i == 3 || i > 4)
//			isMulticolor = true;
		
		api->CreateNewPicture(isMulticolor ? C64_PICTURE_MODE_TEXT_MULTI : C64_PICTURE_MODE_BITMAP_HIRES, 0x00);
		SYS_Sleep(200);
		
		viewC64->viewVicEditor->SetVicMode(true, isMulticolor, false);
		viewC64->viewVicEditor->SetVicAddresses(0, 0x1C00, 0x0000, 0x2000);

		SYS_Sleep(200);
		
		bool skipFile = false;
		
		if (api->ConvertImageToScreen(buf) == false)
		{
			LOGError("Can't convert file %s", buf);
			skipFile = true;
//			return;
			
			// save prg with skip marker data
			byteBuffer->Clear();
			
			u16 startAddr = BACK_SCREEN_ADDR + 0x03FA;

			// skip marker
			byteBuffer->PutU8( startAddr & 0x00FF);
			byteBuffer->PutU8((startAddr & 0xFF00) >> 8);

			// just one byte with marker to skip this image
			byteBuffer->PutU8(0x01);
			byteBuffer->storeToFile(buf2);
			continue;
		}
		
		// store screen ram & color ram
		u8 *screen_ptr;
		u8 *color_ram_ptr;
		u8 *chargen_ptr;
		u8 *bitmap_low_ptr;
		u8 *bitmap_high_ptr;
		u8 d020colors[0x0F];
	
		viewC64->viewVicEditor->viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
											 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
//		u8 *screenRam = new u8[0x03F8];
//		u8 *colorRam = new u8[0x03F8];
//		for (int i = 0; i < 0x03F8; i++)
//		{
//			screenRam[i] = screen_ptr[i];
//			colorRam[i] = color_ram_ptr[i];
//		}
		
		// save data
		byteBuffer->Clear();

		u16 startAddr = isMulticolor ? BACK_COLOR_ADDR : BACK_SCREEN_ADDR;
		
		// screen
		byteBuffer->PutU8( startAddr & 0x00FF);
		byteBuffer->PutU8((startAddr & 0xFF00) >> 8);
		
		if (isMulticolor)
		{
			// color
			for (int i = 0; i < 0x03F8; i++)
			{
				byteBuffer->PutU8(color_ram_ptr[i]);
			}

			// padding 8 bytes
			for (int i = 0; i < 8; i++)
			{
				byteBuffer->PutU8(0);
			}
		}
		
		for (int i = 0; i < 0x03F8; i++)
		{
			byteBuffer->PutU8(screen_ptr[i]);
		}
		
		// store also $d021 BACK_SCREEN_ADDR + 0x03F8
		byteBuffer->PutU8(api->GetByteWithIo(0xD021));
		
		// BACK_SCREEN_ADDR + 0x03F9
		byteBuffer->PutU8(isMulticolor ? 0x01 : 0x00);

		// do not skip this file BACK_SCREEN_ADDR + 0x03FA
		byteBuffer->PutU8(0x00);
		
		// padding 5 bytes
		for (int i = 0; i < 5; i++)
		{
			byteBuffer->PutU8(0);
		}

		// bitmap
		for (int i = 0; i < 0x1F40; i++)
		{
			byteBuffer->PutU8(api->GetByteFromRamC64(i + 0x2000));
		}
		byteBuffer->storeToFile(buf2);
		
		SYS_Sleep(250);
	}

	delete byteBuffer;
	
	//
	crtMaker->cartName = STRALLOC("GalaxyMarszruta");
	crtMaker->cartOutPath = STRALLOC("/Users/mars/develop/galaxy-marszruta/galaxy.crt");
	crtMaker->rootFolderPath = STRALLOC("/Users/mars/develop/galaxy-marszruta");
	crtMaker->buildFilePath = STRALLOC("");
	crtMaker->binFilesPath = STRALLOC("/Users/mars/develop/galaxy-marszruta/bin");
	crtMaker->exoFilesPath = STRALLOC("/Users/mars/develop/galaxy-marszruta/exo");
	crtMaker->exomizerPath = STRALLOC("/Users/mars/develop/lukhash/tools/exomizer310/src/exomizer");
	crtMaker->exomizerParams = STRALLOC("-B -M256");
	crtMaker->javaPath = STRALLOC("/usr/bin/java");
	crtMaker->kickAssJarPath = STRALLOC("/Users/mars/develop/lukhash/tools/KickAss.jar");
	crtMaker->kickAssParams = STRALLOC("-odir /Users/mars/develop/galaxy-marszruta/bin -showmem -debugdump");
//	view->decrunchBinPath = STRALLOC("/Users/mars/develop/lukhash/tools/exomizer310/exodecrs/decrunch.bin");
	crtMaker->decrunchBinPath = STRALLOC("/Users/mars/develop/galaxy-marszruta/tools/exomizer310/exodecrs/decrunch.bin");
	crtMaker->decrunchBinStartAddr = 0x0400;

	CCrtMakerFile *dataFile = new CCrtMakerFile("/Users/mars/develop/galaxy-marszruta/prg/main.prg", "main.prg");
	crtMaker->AddFile(dataFile);

	dataFile = new CCrtMakerFile("/Users/mars/develop/galaxy-marszruta/prg/music.sid", "music.sid");
	crtMaker->AddFile(dataFile);

	// files
	AddFileBitmap(0);	// 0
	AddFileBitmap(1);	// 1
	AddFileBitmap(2);	// 2
	AddFileBitmap(3);	// 3
	AddFileBitmap(4);	// 4
	AddFileBitmap(5);	// 5
	AddFileNufli(1);	// 6
	AddFileNufli(2);	// 7
	AddFileNufli(3);	// 8
	AddFileBitmap(6);	// 9
	AddFileNufli(4);	// 10
	AddFileNufli(5);	// 11
	AddFileBitmap(7);	// 12
	AddFileNufli(6);	// 13
	AddFileNufli(7);	// 14
	AddFileBitmap(8);	// 15
	AddFileNufli(8);	// 16
	AddFileNufli(9);	// 17
	AddFileBitmap(9);	// 18
	AddFileNufli(10);	// 19
	AddFileNufli(11);	// 20
	AddFileBitmap(10);	// 21
	AddFileNufli(12);	// 22
	AddFileNufli(13);	// 23
	AddFileBitmap(11);	// 24
	AddFileNufli(14);	// 25
	AddFileNufli(15);	// 26
	AddFileBitmap(12);	// 27
	AddFileNufli(16);	// 28
	AddFileNufli(17);	// 29
	AddFileBitmap(13);	// 30
	AddFileNufli(18);	// 31
	AddFileNufli(19);	// 32
	AddFileNufli(20);	// 33
	AddFileBitmap(14);	// 34
	AddFileNufli(21);	// 35

	
	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(buf2);
	
	crtMaker->ProcessFiles();
	crtMaker->MakeCartridge();
	
	// run the cart
	api->debugInterface->ResetHard();
	api->debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);

	SYS_Sleep(300);
	
	api->LoadCRT(crtMaker->cartOutPath);

	SYS_Sleep(500);
	//
	
	u16 entryPoint = 0xC000;
	
	u16 songInit = 0x8000;
	u16 songPlay = 0x8003;
	
	// raster Y=2A, X=50
	
	A("			*=$%04x", entryPoint);

	A("				JMP MAINSTART");
	
	// this is copy of default kernal's irq handler to mimic the same number of cycles and behavior
	A("DefaultKernalIRQ		PHA");
	A("						TXA");
	A("						PHA");
	A("						TYA");
	A("						PHA");
	A("						TSX");
	
	// 8 cycles
	A("						LDA $0104,x");
	A("						AND #$10");
	A("						BEQ irqKernal2");
	
	// replace with 9 cycles
//	A("						LDA #$37");		// 2
//	A("						STA $01");		// 3
////	A("						STA $01");		// 3
	A("irqKernal2     		JMP ($0314)");
	
	A("DefaultIRQ			LDA $01");
	A("						STA DefaultIRQ01val+1");
	A("						LDA #$35");
	A("						STA $01");
	A("						INC $D019");

//	A("						INC $D020");
	A("						JSR $%04x", songPlay);
//	A("						DEC $D020");

	A("DefaultIRQ01val		LDA #$34");
	A("						STA $01");
	A("						PLA");
	A("						TAY");
	A("						PLA");
	A("						TAX");
	A("						PLA");
	A("						RTI");

//	A("IrqRetViewer			INC $D020");
//	A("						JSR $%04x", songPlay);
//	A("						DEC $D020");
	A("IrqRetViewer			JSR $%04x", songPlay);

	// not required because we do not fetch new data when showing Nufli
//	A("IrqRetViewer01val	LDA #$34");
//	A("						STA $01")
	
	A("						PLA");
	A("						TAY");
	A("						PLA");
	A("						TAX");
	A("						PLA");
	A("						RTI");
	
	A("MAINSTART	SEI");
	A("				LDA #$35");
	A("				STA $01");

	A("				LDA #0");
	A("				STA $D020");
	A("				STA $D021");
	A("				STA $D011");

	
	// load SID
	A("			LDY #1");
	A("			JSR $0100");
	
	// test if depacker works
//	A("				LDY #3");
//	A("				JSR $0100");
//	A("JMP*");

	A("			LDA #0");
	A("			JSR $%04x", songInit);
	A("			LDA #$0F");
	A("			STA $800A");	// volume
		
	A("					LDA $DD00");
	A("					AND #$fc");		// VIC BANK at $C000
	A("					STA $DD00");
	
	// raster irq
	A("				LDA #$7f");
	A("				STA $dc0d");
	A("				STA $dd0d");
	A("				LDA $dc0d");
	A("				LDA $dd0d");
	A("				LDA #$01");
	A("				STA $d01a");

	A("				LDA #$0B");
	A("				STA $D011");
	A("				LDA #$F9");
	A("				STA $D012");

	A("				LDA #<DefaultKernalIRQ");
	A("				STA $fffe");
	A("				LDA #>DefaultKernalIRQ");
	A("				STA $ffff");
	A("				LDA #<DefaultIRQ");
	A("				STA $0314");
	A("				STA $0316");
	A("				LDA #>DefaultIRQ");
	A("				STA $0315");
	A("				STA $0317");
	
//	A("				LDA #$34");
//	A("				STA $01");
	
	A("				CLI");
	
//	A("JMP*");
		
	A("NextAction	INC actionID+1");
	A("actionID		LDY #$FF");
	A("				LDA actions,Y");
	
	A("				CMP #%d", ACTION_LOAD_NEXT_FILE);
	A("				BNE checkAction2");
	A("fileID		LDY #2");
	A("				JSR $0100");
	A("				INC fileID+1");
	A("				JMP NextAction");
	
	A("checkAction2	CMP #%d", ACTION_HIDE_SCREEN);
	A("				BNE checkAction3");
	A("				LDA #$0B");
	A("				STA $D011");
	A("				JMP NextAction");
	
	A("checkAction3 CMP #%d", ACTION_SHOW_BITMAP);
	A("				BNE checkAction4");
	A("				JMP ShowBitmap");
	
	A("checkAction4 CMP #%d", ACTION_SHOW_NUFFLI);
	A("				BNE checkAction5");
	// patch NUFLI displayer code
	// patch, replace JMP $EA31 with JMP IrqRetViewer
	A("				LDA #<IrqRetViewer");
	A("				STA $31F3");
	A("				LDA #>IrqRetViewer");
	A("				STA $31F4");

	// replace JMP $3109 with JMP NufliReturn
	A("				LDA #$4C");
	A("				STA $3109");
	A("				LDA #<NufliReturn");
	A("				STA $310A");
	A("				LDA #>NufliReturn");
	A("				STA $310B");
	
	// D012 is F8 when creating speedcode
	A("				LDA #$F9");
	A("				STA $3FD2");

	// warning: sprites on cause garbage on first frame when nufli is diplayed
//	A("				LDA #$FF");
//	A("				STA $D015");
	
	
	// $10 raster is important, other cause glitches :)
	A("				LDA #$10");
	A("				JSR waitForRaster");
	A("				JMP $3010");			// skip setup
	
	A("checkAction5	CMP #%d", ACTION_DELAY);
	A("				BNE checkAction6");
	A("				INY");
	A("				STY actionID+1");
	A("				LDA actions,Y");
	A("				STA waitCnt+1");
	A("waitLoop0	LDY #$FF");
	A("waitLoop1	LDX #$FF");
	A("waitLoop2	DEX");
	A("				BNE waitLoop2");
	A("				DEY");
	A("				BNE waitLoop1");
	A("				DEC waitCnt+1");
	A("waitCnt		LDA #$00");
	A("				BNE waitLoop0");
	A("				JMP NextAction");
	
	A("checkAction6	CMP #%d", ACTION_HIDE_NUFFLI);
	A("				BNE checkAction7");
	// replace IRQ back to music play only
	A("				JSR waitForRaster");
	A("				LDA #$0B");
	A("				STA $D011");
	A("				SEI");
	A("				LDA #<DefaultIRQ");
	A("				STA $0314");
	A("				STA $0316");
	A("				LDA #>DefaultIRQ");
	A("				STA $0315");
	A("				STA $0317");
	A("				LDA #$F9");
	A("				STA $D012");
	A("				CLI");
	A("				JMP NextAction");

	A("checkAction7 CMP #%d", ACTION_SONG_FADEOUT);
	A("				BNE checkAction8");
	A("fadeout		LDA $800A");
	A("				CMP #$02");
	A("				BEQ finishFadeout");
	A("				DEC $800A");
	A("				JSR FadeLoop");
	A("				JMP fadeout");
	A("finishFadeout JSR FadeLoop");
	A("				 JSR FadeLoop")
	A("				 JSR FadeLoop")
	A("				DEC $800A");	// 01
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop")
	A("				DEC $800A");	// 00
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop");
	A("				 JSR FadeLoop")
	A("				 JSR FadeLoop")
	A("				JMP MAINSTART")
	
//	A("checkAction8	CMP #%d", ACTION_RESET);
//	A("				BNE checkAction8");
	A("checkAction8 LDA #$FF");
	A("				STA actionID+1");
	A("				LDA #$02");
	A("				STA fileID+1");
	A("				JMP MAINSTART");

	A("FadeLoop		LDX #$FF");
	A("floop1		LDY #$FF");
	A("floop2		DEY");
	A("				BNE floop2");
	A("				DEX");
	A("				BNE floop1");
	A("				RTS");
	
	A("ShowBitmap	LDA #$22");
	A("				JSR waitForRaster");
	A("				LDA #$0B");
	A("				STA $D011");
	// copy bitmap to its place (c800 screen, e000 bitmap, d800 color)
	A("CopyBitmap	LDX #$00")
	A("bitmapSrc	LDA $%04x,X", BACK_BITMAP_ADDR);
	A("bitmapDest	STA $%04x,X", BITMAP_ADDR);
	A("				DEX");
	A("				BNE bitmapSrc");
	A("				INC bitmapSrc+2");
	A("				INC bitmapDest+2");
	A("				LDA bitmapSrc+2");
	A("				CMP #$%02x", 0x1F + ((BACK_BITMAP_ADDR & 0xFF00) >> 8));
	A("				BNE bitmapSrc");
	A("				LDX #$40");
	A("bitmapLoop2	LDA $%04x,X", BACK_BITMAP_ADDR + 0x1EFF);
	A("				STA $%04x,X", BITMAP_ADDR + 0x1EFF);
	A("				DEX");
	A("				BNE bitmapLoop2");

//	A("				LDA #$35");
//	A("				STA $01");

	// screen & color
	A("				LDX #$FA");

	A("scLoop1		LDA $%04x,x", BACK_SCREEN_ADDR	-1);
	A("				STA $%04x,x", SCREEN_ADDR		-1);
	A("				LDA $%04x,x", BACK_SCREEN_ADDR	+ 0x00F9);
	A("				STA $%04x,x", SCREEN_ADDR 		+ 0x00F9);
	A("				LDA $%04x,x", BACK_SCREEN_ADDR 	+ 0x01F3);
	A("				STA $%04x,x", SCREEN_ADDR 		+ 0x01F3);
	A("				LDA $%04x,x", BACK_SCREEN_ADDR 	+ 0x02ED);
	A("				STA $%04x,x", SCREEN_ADDR 		+ 0x02ED);

	A("				DEX			");
	A("				BNE scLoop1");		// BPL

	A("				LDY #$08");
	A("				LDA $%04x", BACK_SCREEN_ADDR + 0x03F9);
	A("				BEQ noColor");

	A("				LDX #$FA");

	A("scLoop2		LDA $%04x,x", BACK_COLOR_ADDR-1);
	A("				STA $D800,x");
	A("				LDA $%04x,x", BACK_COLOR_ADDR+0x00F9);
	A("				STA $D8F9,x");
	A("				LDA $%04x,x", BACK_COLOR_ADDR+0x01F3);
	A("				STA $D9F3,x");
	A("				LDA $%04x,x", BACK_COLOR_ADDR+0x02ED);
	A("				STA $DAED,x");
	A("				DEX			");
	A("				BNE scLoop2");		// BPL
	A("				LDY #$18");
//
	A("noColor		STY $D016");
	A("				LDA $%04x", BACK_SCREEN_ADDR + 0x03F8);
//	A("				STA $D020");
	A("				STA $D021");

	A("					LDA $DD00");
	A("					AND #$fc");
//	A("					LDA #$02");		// VIC BANK
	A("					STA $DD00");
	
//	A("					LDA #$%02x", useHiresTitles ? 0x08 : 0x18);
//	A("					STA $D016");
	
	A("					LDA #$%02x", ( ((SCREEN_ADDR - VIC_BANK_ADDR)/0x400) << 4) | (BITMAP_ADDR - VIC_BANK_ADDR == 0 ? 0:0x8));
	A("					STA $D018");

	A("				LDA #0");	// sprites off
	A("				STA $D015");

	A("				LDA #$22");
	A("				JSR waitForRaster");

	A("				LDA #$3B");
	A("				STA $D011");

	// reset bitmap copy counters
	A("				LDA #$%02x", ((BACK_BITMAP_ADDR & 0xFF00) >> 8));
	A("				STA bitmapSrc+2");
	A("				LDA #$%02x", ((BITMAP_ADDR & 0xFF00) >> 8));
	A("				STA bitmapDest+2");
	
	A("				JMP NextAction");
	
	// wait loop
	A("NufliReturn	JSR waitForRaster");
	A("				LDA #$3B");
	A("				STA $D011");
	A("				SEI");
	A("				LDA #$10");
	A("				STA $0314");
	A("				LDA #$31");
	A("				STA $0315");
	A("				LDA #$29");
	A("				STA $D012");
	A("				CLI");
	A("				JMP NextAction");

	A("waitForRaster	CMP $D012							");
	A("					BCC waitForRaster					");
	A("					RTS");


	AddActions();

	
	int mainCodeStart, mainCodeSize;
	api->Assemble64TassToRam(&mainCodeStart, &mainCodeSize, "/Users/mars/develop/galaxy-marszruta/prg/main.asm", false);

	LOGD("Save main.prg: %04x to %04x", mainCodeStart, mainCodeStart + mainCodeSize);
	
	api->SavePRG(mainCodeStart, mainCodeStart+mainCodeSize, "/Users/mars/develop/galaxy-marszruta/prg/main.prg");
	
	// recreate CRT
	crtMaker->ProcessFiles();
	crtMaker->MakeCartridge();
	viewC64->mainMenuHelper->InsertCartridge(new CSlrString(crtMaker->cartOutPath), false);
	api->UnPauseEmulation();
}

void C64DebuggerPluginGalaxy::AddFileBitmap(int bitmapID)
{
	char *bufPath = SYS_GetCharBuf();
	char *bufName = SYS_GetCharBuf();
	sprintf(bufPath, hiresPrgPath, bitmapID);
	sprintf(bufName, hiresPrgName, bitmapID);

	CCrtMakerFile *dataFile = new CCrtMakerFile(bufPath, bufName);
	crtMaker->AddFile(dataFile);
	
	SYS_ReleaseCharBuf(bufPath);
	SYS_ReleaseCharBuf(bufName);
}

void C64DebuggerPluginGalaxy::AddFileNufli(int nufliID)
{
	char *bufPath = SYS_GetCharBuf();
	char *bufName = SYS_GetCharBuf();
	sprintf(bufPath, nufliPrgPath, nufliID);
	sprintf(bufName, nufliPrgName, nufliID);

	CCrtMakerFile *dataFile = new CCrtMakerFile(bufPath, bufName);
	crtMaker->AddFile(dataFile);
	
	SYS_ReleaseCharBuf(bufPath);
	SYS_ReleaseCharBuf(bufName);
}

void C64DebuggerPluginGalaxy::AddActions()
{
	// add actions
	actionBuffer = new CByteBuffer();

	int o = 5;
	
	int delayBitmapIntro = o + 12;
	int delayBitmap = o + 20;
	int delayBitmapLoadNufli = o + 8;
	int delayNufli = o + 1;

//	delayBitmapIntro = 1;
//	delayBitmap = 1;
//	delayBitmapLoadNufli = 1;
//	delayNufli = 1;

	// 0 hires
	AddActionLoadNextFile();
	AddActionShowBitmap();
	AddActionDelay(delayBitmapIntro);
	
	// 1 hires
	AddActionLoadNextFile();
	AddActionShowBitmap();
	AddActionDelay(delayBitmapIntro);
	
	// 2 hires
	AddActionLoadNextFile();
	AddActionShowBitmap();
	AddActionDelay(delayBitmapIntro);

	// 3 hires
	AddActionLoadNextFile();
	AddActionShowBitmap();
	AddActionDelay(delayBitmapIntro);

	// 4 hires
	AddActionLoadNextFile();
	AddActionShowBitmap();
	AddActionDelay(delayBitmapIntro);

	// 5 hires
	AddActionLoadNextFile();
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli + 4);

	// 6 nufli
	AddActionLoadNextFile();
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 7 nufli
	AddActionLoadNextFile();
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 8 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 9 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);
	
	// 10 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 11 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 12 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);
	
	// 13 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 14 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 15 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);

	// 16 nufli
	AddActionLoadNextFile();
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 17 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 18 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);

	// 19 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 20 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 21 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);
	
	// 22 nufli
	AddActionLoadNextFile();
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 23 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 24 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);
	
	// 25 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 26 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 27 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);

	// 28 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 29 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 30 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);
	
	// 31 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 32 nufli
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 33 nufli
	AddActionLoadNextFile();	// nufli
	AddActionLoadNextFile();	// hires
	AddActionShowNufli();
	AddActionDelay(delayNufli);
	AddActionHideNufli();

	// 34 hires
	AddActionShowBitmap();
	AddActionDelay(delayBitmapLoadNufli);

	// 35 nufli - smok
	AddActionLoadNextFile();	// nufli
	AddActionShowNufli();
	AddActionDelay(delayNufli + 5);
	AddActionHideNufli();

	// 36 end
	AddActionHideScreen();
	AddActionSongFadeout();
	AddActionFinish();

	sprintf(assembleTextBuf, "actions .byte %d", actionBuffer->data[0]);
	api->Assemble64TassAddLine(assembleTextBuf);

	for (int i = 1; i < actionBuffer->length; i++)
	{
		sprintf(assembleTextBuf, ".byte %d", actionBuffer->data[i]);
		api->Assemble64TassAddLine(assembleTextBuf);
	}
	
	LOGD("actions=%s", assembleTextBuf);
}

void C64DebuggerPluginGalaxy::AddActionFinish()
{
	actionBuffer->PutU8(ACTION_RESET);
}

void C64DebuggerPluginGalaxy::AddActionLoadNextFile()
{
	actionBuffer->PutU8(ACTION_LOAD_NEXT_FILE);
}

void C64DebuggerPluginGalaxy::AddActionShowBitmap()
{
	actionBuffer->PutU8(ACTION_SHOW_BITMAP);
}

void C64DebuggerPluginGalaxy::AddActionShowNufli()
{
	actionBuffer->PutU8(ACTION_SHOW_NUFFLI);
}

void C64DebuggerPluginGalaxy::AddActionHideNufli()
{
	actionBuffer->PutU8(ACTION_HIDE_NUFFLI);
}

void C64DebuggerPluginGalaxy::AddActionHideScreen()
{
	actionBuffer->PutU8(ACTION_HIDE_SCREEN);
}

void C64DebuggerPluginGalaxy::AddActionDelay(int numDelay)
{
	actionBuffer->PutU8(ACTION_DELAY);
	actionBuffer->PutU8(numDelay);
}

void C64DebuggerPluginGalaxy::AddActionSongFadeout()
{
	actionBuffer->PutU8(ACTION_SONG_FADEOUT);
}

void C64DebuggerPluginGalaxy::DoFrame()
{
	
}

u32 C64DebuggerPluginGalaxy::KeyDown(u32 keyCode)
{
	return keyCode;
}

u32 C64DebuggerPluginGalaxy::KeyUp(u32 keyCode)
{
	return keyCode;
}
