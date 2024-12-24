#include "C64DebuggerPluginSlideshow.h"

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

C64DebuggerPluginSlideshow *pluginSlideshow = NULL;
CViewC64CrtMaker *viewSlideshowC64CrtMaker = NULL;

void PLUGIN_SlideshowInit()
{
	if (pluginSlideshow == NULL)
	{
		pluginSlideshow = new C64DebuggerPluginSlideshow();
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginSlideshow);
	}
}

void PLUGIN_SlideshowSetVisible(bool isVisible)
{
	if (pluginSlideshow != NULL)
	{
		pluginSlideshow->crtMaker->SetVisible(isVisible);
	}
}

C64DebuggerPluginSlideshow::C64DebuggerPluginSlideshow()
{
	pluginSlideshow = this;
	assembleTextBuf = new char[1024];
}

void C64DebuggerPluginSlideshow::Init()
{
	C64DebuggerPluginCrtMaker::Init();
}

#define SLEEP_FACTOR 1.0
void C64DebuggerPluginSlideshow::ThreadRun(void *data)
{
	LOGD("C64DebuggerPluginSlideshow::ThreadRun");
	
	SYS_Sleep(50);
	api->DetachEverything();
	
	api->AddView(crtMaker);
	
	SYS_Sleep(500);

	
	//
	api->CreateNewPicture(C64_PICTURE_MODE_BITMAP_MULTI, 0x00);

	SYS_Sleep(1000);
	
	viewC64->viewVicEditor->SetVicMode(true, true, false);
	viewC64->viewVicEditor->SetVicAddresses(0, 0x1C00, 0x0000, 0x2000);

	// prepare RAM
	api->ClearRam(0x0200, 0x10000, 0x00);
	
#define BACK_SCREEN_ADDR	0x1C00
#define BACK_COLOR_ADDR		0x2000
#define BACK_BITMAP_ADDR	0x2400

	char *buf = SYS_GetCharBuf();
	char *buf2 = SYS_GetCharBuf();

	// convert images
	for (int i = 1; i <= 18; i++)
	{
		sprintf(buf,  "/Users/mars/develop/galaxy-marszruta/gfx/GalaxyMarszruta_%02d.png", i);
		sprintf(buf2, "/Users/mars/develop/galaxy-marszruta/gfx/GalaxyMarszruta_%02d_bitmap.prg", i);

		if (SYS_FileExists(buf2))
		{
			continue;
		}
		
		api->ConvertImageToScreen(buf);
		
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
		CByteBuffer *byteBuffer = new CByteBuffer();
		
		// screen
		byteBuffer->PutU8( BACK_SCREEN_ADDR & 0x00FF);
		byteBuffer->PutU8((BACK_SCREEN_ADDR & 0xFF00) >> 8);
		for (int i = 0; i < 0x03F8; i++)
		{
			byteBuffer->PutU8(screen_ptr[i]);
		}
		// store also $d021
		byteBuffer->PutU8(api->GetByteWithIo(0xD021));
		
		// padding 7 bytes
		for (int i = 0; i < 7; i++)
		{
			byteBuffer->PutU8(0);
		}

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

		// bitmap
		for (int i = 0; i < 0x1F40; i++)
		{
			byteBuffer->PutU8(api->GetByteFromRamC64(i + 0x2000));
		}
		byteBuffer->storeToFile(buf2);
		
		SYS_Sleep(50);
	}
	
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
	crtMaker->decrunchBinStartAddr = 0x0200;

	CCrtMakerFile *dataFile = new CCrtMakerFile("/Users/mars/develop/galaxy-marszruta/prg/main.prg", "main.prg");
	crtMaker->AddFile(dataFile);

	dataFile = new CCrtMakerFile("/Users/mars/develop/galaxy-marszruta/prg/music.sid", "music.sid");
	crtMaker->AddFile(dataFile);

	for (int i = 1; i <= 18; i++)
	{
		sprintf(buf, "/Users/mars/develop/galaxy-marszruta/gfx/GalaxyMarszruta_%02d_bitmap.prg", i);
		sprintf(buf2, "GalaxyMarszruta_%02d_bitmap.prg", i);
		dataFile = new CCrtMakerFile(buf, buf2);
		crtMaker->AddFile(dataFile);
	}
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
	
	u16 entryPoint = 0x4340;
	
	u16 songInit = 0x04D2;
	u16 songPlay = 0x04D9;
	
	A("			*=$%04x", entryPoint);

	A("				SEI");
	A("				LDA #$35");
	A("				STA $01");

	A("				LDA #0");
	A("				STA $D020");
	A("				STA $D021");
	A("				STA $D011");

	// load song
	A("			LDY #1");
	A("			JSR $0100");
	
	A("			LDA #1");
	A("			JSR $%04x", songInit);
	
	A("waitForFrameEnd	LDA $D012	");
	A("					CMP #$22	");
	A("					BNE waitForFrameEnd	");
		
	A("					LDA $DD00");
	A("					AND #$fc");
//	A("					LDA #$02");		// VIC BANK
	A("					STA $DD00");
	
	A("					LDA #$18");
	A("					STA $D016");
	
	A("					LDA #$%02x", ( ((SCREEN_ADDR - VIC_BANK_ADDR)/0x400) << 4) | (BITMAP_ADDR - VIC_BANK_ADDR == 0 ? 0:0x8));
	A("					STA $D018");

	// raster irq
	A("				LDA #$7f");
	A("				STA $dc0d");
	A("				STA $dd0d");
	A("				LDA $dc0d");
	A("				LDA $dd0d");
	A("				LDA #$01");
	A("				STA $d01a");
	A("				LDA #$10");
	A("				STA $d012");
	
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
	
	A("				CLI");
	
	// load screen to $1c00, color to $2000, and bitmap to $2400
	A("bitmapFileID	LDY #2");
	A("				JSR $0100");
	
	A("rasterW2		LDA $D012							");
	A("				CMP #$22							");
	A("				BNE rasterW2						");

	A("				LDA #$00");
	A("				STA $D011");

	A("				LDX #$00")
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

	// screen & color
	A("				LDX #$FA");
	A("scLoop1		LDA $%04x,x", BACK_COLOR_ADDR-1);
	A("				STA $D7FF,x");
	A("				LDA $%04x,x", BACK_COLOR_ADDR+0x00F9);
	A("				STA $D8F9,x");
	A("				LDA $%04x,x", BACK_COLOR_ADDR+0x01F3);
	A("				STA $D9F3,x");
	A("				LDA $%04x,x", BACK_COLOR_ADDR+0x02ED);
	A("				STA $DAED,x");
	
	A("				LDA $%04x,x", BACK_SCREEN_ADDR	-1);
	A("				STA $%04x,x", SCREEN_ADDR		-1);
	A("				LDA $%04x,x", BACK_SCREEN_ADDR	+ 0x00F9);
	A("				STA $%04x,x", SCREEN_ADDR 		+ 0x00F9);
	A("				LDA $%04x,x", BACK_SCREEN_ADDR 	+ 0x01F3);
	A("				STA $%04x,x", SCREEN_ADDR 		+ 0x01F3);
	A("				LDA $%04x,x", BACK_SCREEN_ADDR 	+ 0x02ED);
	A("				STA $%04x,x", SCREEN_ADDR 		+ 0x02ED);

	A("				DEX			");
	A("				BNE scLoop1");

	A("loopD011		LDA $D012							");
	A("				CMP #$22							");
	A("				BNE loopD011						");

	A("				LDA $%04x", BACK_SCREEN_ADDR + 0x03F8);
	A("				STA $D021");
	
	A("				LDA #$3B");
	A("				STA $D011");
	
	// reset
	A("				LDA #$%02x", ((BACK_BITMAP_ADDR & 0xFF00) >> 8));
	A("				STA bitmapSrc+2");
	A("				LDA #$%02x", ((BITMAP_ADDR & 0xFF00) >> 8));
	A("				STA bitmapDest+2");

	// wait loop
	A("				LDA #$08");
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
	
	A("				INC bitmapFileID+1");
	A("				LDA bitmapFileID+1");
	A("				CMP #20");	//18 + 2
	A("				BNE nextImage");
	
	// reset image counter
	A("				LDA #$02");
	A("				STA bitmapFileID+1");

	A("nextImage	JMP bitmapFileID");
//	A("				JMP *");

	// this is copy of default kernal's irq handler to mimic the same number of cycles and behavior
	A("DefaultKernalIRQ		PHA");
	A("						TXA");
    A("						PHA");
	A("						TYA");
	A("						PHA");
	A("						TSX");
	A("						LDA $0104,x");
	A("						AND #$10");
	A("						BEQ irqKernal2");
	A("irqKernal2     		JMP ($0314)");
	A("DefaultIRQ      		LDA $01");
	A("						STA irq1val01+1");
	A("						LDA #$35");
	A("						STA $01");
	
	A("						JSR $%04x", songPlay);
	A("						ASL $d019");
	A("irq1val01:      		LDA #$00");
	A("						STA $01");
	A("						PLA");
	A("						TAY");
	A("						PLA");
	A("						TAX");
	A("						PLA");
	A("						RTI");
	
	int mainCodeStart, mainCodeSize;
	api->Assemble64TassToRam(&mainCodeStart, &mainCodeSize, "/Users/mars/develop/galaxy-marszruta/prg/main.asm", false);

	LOGD("Save main.prg: %04x to %04x", mainCodeStart, mainCodeStart + mainCodeSize);
	
	api->SavePRG(mainCodeStart, mainCodeStart+mainCodeSize, "/Users/mars/develop/galaxy-marszruta/prg/main.prg");
	
	// recreate CRT
	crtMaker->ProcessFiles();
	crtMaker->MakeCartridge();
	viewC64->mainMenuHelper->InsertCartridge(new CSlrString(crtMaker->cartOutPath), false);
	//api->MakeJmp(entryPoint);
	api->UnPauseEmulation();
	
}

void C64DebuggerPluginSlideshow::DoFrame()
{
	
}

u32 C64DebuggerPluginSlideshow::KeyDown(u32 keyCode)
{
	return keyCode;
}

u32 C64DebuggerPluginSlideshow::KeyUp(u32 keyCode)
{
	return keyCode;
}
