#include "C64DebuggerPluginDNDK.h"
#include "CConfigStorageHjson.h"
#include "CDebugInterfaceNes.h"
#include <map>
#include <string>

#define A(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); api->Assemble64TassAddLine(assembleTextBuf);

C64DebuggerPluginDNDK *pluginDDNK = NULL;

C64DebuggerPluginDNDK::C64DebuggerPluginDNDK(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	InitImGuiView("NES Dungeons and Doomknights plugin");

	this->SetEmulatorType(EMULATOR_TYPE_NESTOPIA);
	
//	InitConfig("PluginDNDK");
}

C64DebuggerPluginDNDK::~C64DebuggerPluginDNDK()
{
	
}

void C64DebuggerPluginDNDK::Init()
{
	api->AddView(this);
}

void C64DebuggerPluginDNDK::RenderImGui()
{
	PreRenderImGui();
	
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImVec2 label_size = ImGui::CalcTextSize("@", NULL, true);
	float buttonSizeY = label_size.y + style.FramePadding.y * 2.0f;
	float buttonSizeX = 150;
	
	ImVec2 buttonSize(buttonSizeX, buttonSizeY);

	
	//
	//HEARTS 06d4
	//MAX HEARTS 06D6
	//skulls 06d7
	//max skulls 06e7
	//
	//init code c0d7
	//
	//keys 06d8 right
	//keys 06d9 left
	//
	//gold 06da right
	//gold 06db left
	//
	//current location 06d0

	
	int val;
	val = debugInterfaceNes->GetByte(0x06D4);
	if (ImGui::InputInt("Hearts", &val))
	{
		debugInterfaceNes->SetByte(0x06D4, val);
	}

	val = debugInterfaceNes->GetByte(0x06D6);
	if (ImGui::InputInt("Max Hearts", &val))
	{
		debugInterfaceNes->SetByte(0x06D6, val);
	}

	val = debugInterfaceNes->GetByte(0x06D7);
	if (ImGui::InputInt("Skulls", &val))
	{
		debugInterfaceNes->SetByte(0x06D7, val);
	}

	val = debugInterfaceNes->GetByte(0x06E7);
	if (ImGui::InputInt("Max Skulls", &val))
	{
		debugInterfaceNes->SetByte(0x06E7, val);
	}

	u8 vr, vl;
	vr = debugInterfaceNes->GetByte(0x06d8);
	vl = debugInterfaceNes->GetByte(0x06d9);
	val = vr + vl * 10;
	if (ImGui::InputInt("Keys", &val))
	{
		vl = floor((float)val / 10.0f);
		vr = val - vl*10;
		debugInterfaceNes->SetByte(0x06d8, vr);
		debugInterfaceNes->SetByte(0x06d9, vl);
	}

	vr = debugInterfaceNes->GetByte(0x06da);
	vl = debugInterfaceNes->GetByte(0x06db);
	val = vr + vl * 10;
	if (ImGui::InputInt("Gold", &val))
	{
		vl = floor((float)val / 10.0f);
		vr = val - vl*10;
		debugInterfaceNes->SetByte(0x06da, vr);
		debugInterfaceNes->SetByte(0x06db, vl);
	}

	val = debugInterfaceNes->GetByte(0x06d0);
	if (ImGui::InputScalar("Location", ImGuiDataType_::ImGuiDataType_U8, &val, NULL, NULL, "%02X", defaultHexInputFlags))
	{
		debugInterfaceNes->SetByte(0x06d0, val);
	}

	PostRenderImGui();
}

void C64DebuggerPluginDNDK::ThreadRun(void *data)
{
	
}

void C64DebuggerPluginDNDK::DoFrame()
{
	
}

//
void PLUGIN_DdnkInit()
{
	if (pluginDDNK == NULL)
	{
		pluginDDNK = new C64DebuggerPluginDNDK(300, 30, 0, 600, 400);
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginDDNK);
	}
}

void PLUGIN_DdnkSetVisible(bool isVisible)
{
	if (pluginDDNK != NULL)
	{
		pluginDDNK->SetVisible(isVisible);
	}
}


///
void C64DebuggerPluginDNDK::GenerateCode()
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

