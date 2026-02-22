#include "C64DebuggerPluginGoatTracker.h"
// workaround: SDL defines int8_t as different type
#define int8_t blah
#include "goattrk2.h"
#include "gconsole.h"
#include "SYS_CommandLine.h"
#include "GFX_Types.h"
#include "CViewC64GoatTracker.h"
#include "CAudioChannelGoatTracker.h"
#include "SND_Main.h"
#include "CDebuggerApiVice.h"

#include <map>

#define ASSEMBLE(fmt, ...) sprintf(buf, fmt, ## __VA_ARGS__); this->Assemble(buf);
#define A(fmt, ...) sprintf(buf, fmt, ## __VA_ARGS__); this->Assemble(buf);
#define PUT(v) this->PutDataByte(v);
#define PC addrAssemble

//#define  100
//#define  37

C64DebuggerPluginGoatTracker *pluginGoatTracker = NULL;

void PLUGIN_GoatTrackerInit()
{
	if (pluginGoatTracker == NULL)
	{
		pluginGoatTracker = new C64DebuggerPluginGoatTracker();
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginGoatTracker);
	}
}

void PLUGIN_GoatTrackerSetVisible(bool isVisible)
{
	if (pluginGoatTracker != NULL)
	{
		pluginGoatTracker->view->SetVisible(isVisible);
	}
}


extern "C" {
	unsigned char *gtGetRgbaPixelsBuffer()
	{
		LOGD("pluginGoatTracker->imageDataScreen=%x pluginGoatTracker->imageDataScreen->resultData=%x",
			 pluginGoatTracker->view->imageDataScreen, pluginGoatTracker->view->imageDataScreen->resultData);
		return pluginGoatTracker->view->imageDataScreen->resultData;
	}

	unsigned int gtGetGfxPitch()
	{
		return pluginGoatTracker->view->imageDataScreen->width * 4;
	}
	
	void gtForwardEvents()
	{
		pluginGoatTracker->view->ForwardEvents();
	}
}

C64DebuggerPluginGoatTracker::C64DebuggerPluginGoatTracker()
{
	LOGD("C64DebuggerPluginGoatTracker");
	pluginGoatTracker = this;
	LOGD("pluginGoatTracker=%x", pluginGoatTracker);
}

void C64DebuggerPluginGoatTracker::Init()
{
	LOGD("C64DebuggerPluginGoatTracker::Init");

	//
	view = new CViewC64GoatTracker(0, 0, -1, MAX_COLUMNS*8, MAX_ROWS*16);

	//
	api->StartThread(this);
}

extern "C" {
	int gtmain(int argc, const char **argv);
}

void C64DebuggerPluginGoatTracker::ThreadRun(void *data)
{
	LOGD("TODO: GT commandline");
	
	SYS_Sleep(50);
	api->AddView(view);

	audioChannel = new CAudioChannelGoatTracker(this);
	audioChannel->Start();
	SND_AddChannel(audioChannel);

//	gtmain(SYS_GetArgc(), SYS_GetArgv());
	gtmain(0, SYS_GetArgv());
}

void C64DebuggerPluginGoatTracker::DoFrame()
{
	// do anything you need after each emulation frame, vsync is here:
	
	
//	LOGD("C64DebuggerPluginGoatTracker::DoFrame finished");
}

void C64DebuggerPluginGoatTracker::SetupShadowRegsPlayer()
{
	// setup routine that will copy regs from shadow memory location to SID
	char *buf = SYS_GetCharBuf();

	api->DetachEverything();
	api->Sleep(200);
	
	// prepare RAM
	api->ClearRam(0x0800, 0x10000, 0x00);
	
	// TODO: player copy to d400
	PC = 0x0F00;
	A("SEI");
//	A("INC D020");
	A("JMP %04x", PC);
	
	
//	// load sid
//	u16 fromAddr, toAddr, sidInitAddr, sidPlayAddr;
//	api->LoadSID("music.sid", &fromAddr, &toAddr, &sidInitAddr, &sidPlayAddr);
//
//	//
//	api->CreateNewPicture(C64_PICTURE_MODE_BITMAP_MULTI, 0x00);
//
//	api->Sleep(100);
	
//	imageDataRef = new CImageData("reference.png");
//	api->LoadReferenceImage(imageDataRef);
//	api->SetReferenceImageLayerVisible(true);
//	api->ClearReferenceImage();
//
//	api->ConvertImageToScreen(imageDataRef);
	
	api->ClearScreen();

	api->SetReferenceImageLayerVisible(true);

	api->SetupVicEditorForScreenOnly();

	api->Sleep(100);

	SYS_ReleaseCharBuf(buf);
}

u32 C64DebuggerPluginGoatTracker::KeyDown(u32 keyCode)
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

u32 C64DebuggerPluginGoatTracker::KeyUp(u32 keyCode)
{
	return keyCode;
}

///
void C64DebuggerPluginGoatTracker::Assemble(char *buf)
{
	//	LOGD("Assemble: %04x %s", addrAssemble, buf);
	addrAssemble += api->Assemble(addrAssemble, buf);
}

void C64DebuggerPluginGoatTracker::PutDataByte(u8 v)
{
	//	LOGD("PutDataByte: %04x %02x", addrAssemble, v);
	api->SetByteToRam(addrAssemble, v);
	addrAssemble++;
}
