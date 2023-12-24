#include "C64DebuggerPluginCommandoTrainer.h"
#include "CConfigStorageHjson.h"
#include "CDebugInterfaceNes.h"
#include <map>
#include <string>

#define A(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); api->Assemble64TassAddLine(assembleTextBuf);

C64DebuggerPluginCommandoTrainer::C64DebuggerPluginCommandoTrainer(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	InitImGuiView("C64 Commando plugin");
	
//	InitConfig("PluginCommandoTrainer");
}

C64DebuggerPluginCommandoTrainer::~C64DebuggerPluginCommandoTrainer()
{
	
}

void C64DebuggerPluginCommandoTrainer::Init()
{
	api->AddView(this);
}

void C64DebuggerPluginCommandoTrainer::RenderImGui()
{
	PreRenderImGui();
	
	int val;
	val = api->GetByte(0x0500);
	if (ImGui::InputInt("Lives", &val))
	{
		api->SetByte(0x0500, val);
		u8 chr = (val & 0x0f) + 0x21;
		api->SetByte(0xE362, chr);
	}
	
	PostRenderImGui();
}

void C64DebuggerPluginCommandoTrainer::ThreadRun(void *data)
{
}

void C64DebuggerPluginCommandoTrainer::DoFrame()
{
	SetVisible(true);
}

///
void C64DebuggerPluginCommandoTrainer::GenerateCode()
{
//	A("                     *=$1000");
//	A("START:       		INC $D020");
//	A("                     JMP START");
//
//	int codeStartAddr;
//	int codeSize;
//	
//	api->Assemble64Tass(&codeStartAddr, &codeSize);
}

