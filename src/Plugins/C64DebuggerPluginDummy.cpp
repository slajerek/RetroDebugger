#include "C64DebuggerPluginDummy.h"
#include "GFX_Types.h"
#include "CViewC64.h"
#include "CViewC64VicEditor.h"
#include <map>

#define ASSEMBLE(fmt, ...) sprintf(buf, fmt, ## __VA_ARGS__); this->Assemble(buf);
#define A(fmt, ...) sprintf(buf, fmt, ## __VA_ARGS__); this->Assemble(buf);
#define PUT(v) this->PutDataByte(v);
#define PC addrAssemble


C64DebuggerPluginDummy::C64DebuggerPluginDummy()
{
}

void C64DebuggerPluginDummy::Init()
{
	LOGD("C64DebuggerPluginDummy::Init");

	api->SwitchToVicEditor();
	
	//
	api->StartThread(this);
}

void C64DebuggerPluginDummy::ThreadRun(void *data)
{
	char *buf = SYS_GetCharBuf();
	
	api->DetachEverything();
	api->Sleep(500);
	
	// prepare RAM
	api->ClearRam(0x0800, 0x10000, 0x00);
	
	PC = 0x0F00;
	A("SEI");
	A("INC D020");
	A("JMP %04x", PC-3);
	
	
	// load sid
	u16 fromAddr, toAddr, sidInitAddr, sidPlayAddr;
	api->LoadSID("music.sid", &fromAddr, &toAddr, &sidInitAddr, &sidPlayAddr);
	
	//
	api->CreateNewPicture(C64_PICTURE_MODE_BITMAP_MULTI, 0x00);

	api->Sleep(100);
	
//	imageDataRef = new CImageData("reference.png");
//	api->LoadReferenceImage(imageDataRef);
//	api->SetReferenceImageLayerVisible(true);
//	api->ClearReferenceImage();
//	
//	api->ConvertImageToScreen(imageDataRef);
	
	api->ClearScreen();

	api->SetReferenceImageLayerVisible(true);

	api->SetupVicEditorForScreenOnly();
	
//		frame = 0;
//		viewC64->viewVicEditor->SetVicModeRegsOnly(true, true, false);
//		viewC64->viewVicEditor->SetVicAddresses(0, 0x0C00, 0x0000, 0x2000);

	api->Sleep(500);
	
	SYS_ReleaseCharBuf(buf);
}

void C64DebuggerPluginDummy::DoFrame()
{
	// do anything you need after each emulation frame, vsync is here:
	if (api->GetCurrentFrameNumber() > 25)
	{
		static float cr = 0;	float sr = 0.21f;
		static float cg = 64;	float sg = 0.73f;
		static float cb = 128;	float sb = 0.66f;

		for (int y = 50; y < 150; y++)
		{
			for (int x = 80; x < 240; x++)
			{
				u8 r = (u8)((y/200.0f) * 255 + cr) % 255;
				u8 g = (u8)((x/320.0f) * 255 + cg) % 255;
				u8 b = (u8)cb % 255;
//				LOGD("r=%d g=%d b=%d", r, g, b);
				u8 color = api->FindC64Color(r, g, b);
				api->PaintPixel(x, y, color);
	//			api->PaintReferenceImagePixel(x, y, color);
	//			api->PaintReferenceImagePixel(x, y, r, g, b, 255);
			}
		}

		cr += sr;
		cg += sg;
		cb += sb;
	}
	
	
//	LOGD("C64DebuggerPluginDummy::DoFrame finished");
}

u32 C64DebuggerPluginDummy::KeyDown(u32 keyCode)
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
		api->SaveExomizerPRG(0x1000, 0x3000, 0x0F00, "out.prg");
	}
	
	return keyCode;
}

u32 C64DebuggerPluginDummy::KeyUp(u32 keyCode)
{
	return keyCode;
}

///
void C64DebuggerPluginDummy::Assemble(char *buf)
{
	//	LOGD("Assemble: %04x %s", addrAssemble, buf);
	addrAssemble += api->Assemble(addrAssemble, buf);
}

void C64DebuggerPluginDummy::PutDataByte(u8 v)
{
	//	LOGD("PutDataByte: %04x %02x", addrAssemble, v);
	api->SetByteToRam(addrAssemble, v);
	addrAssemble++;
}
