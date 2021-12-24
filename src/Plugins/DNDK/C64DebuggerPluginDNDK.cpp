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
	
	extensionsConfig.push_back(new CSlrString("hjson"));
	extensionsPRG.push_back(new CSlrString("prg"));
	extensionsPRGandSID.push_back(new CSlrString("prg"));
	extensionsPRGandSID.push_back(new CSlrString("sid"));
	extensionsPNG.push_back(new CSlrString("png"));

	//
	EMPTY_PATH_ALLOC(settingPrgOutputPath);
	settingExportToPrg = false;

	InitConfig("PluginTemplate");
}

C64DebuggerPluginDNDK::~C64DebuggerPluginDNDK()
{
	
}

void C64DebuggerPluginDNDK::Init()
{
	api->AddView(this);
}

//
void C64DebuggerPluginDNDK::StoreToHjson(Hjson::Value hjsonRoot)
{
	char hexStr[16];
	hjsonRoot["ExportToPrg"] = settingExportToPrg;
	hjsonRoot["PrgOutputPath"] = settingPrgOutputPath;

//	sprintf(hexStr, "%04x", PACKED_TEXTURES_ADDR);
//	hjsonRoot["PackedTexturesAddr"] = hexStr;
}

void C64DebuggerPluginDNDK::InitFromHjson(Hjson::Value hjsonRoot)
{
	const char *hexValueStr;
	char *p;
	
	settingExportToPrg 	= static_cast<const bool>(hjsonRoot["ExportToPrg"]);
	strcpy(settingPrgOutputPath, static_cast<const char *>(hjsonRoot["PrgOutputPath"]));	

//	hexValueStr = static_cast<const char *>(hjsonRoot["PackedTexturesAddr"]);
//	PACKED_TEXTURES_ADDR = strtoul( hexValueStr, NULL, 16 );
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
	if (ImGui::InputScalar(
						   "Location", ImGuiDataType_::ImGuiDataType_U8, &val, NULL, NULL, "%02X", defaultHexInputFlags))
	{
		debugInterfaceNes->SetByte(0x06d0, val);
	}

	
	/*
	
	// init consts
	if (ImGui::Button("Load", ImVec2(buttonSizeX/2.0f - style.FramePadding.x, buttonSizeY)))
	{
		pathToSet = featureConfigPath;
		SYS_DialogOpenFile(this, &extensionsConfig, featureDefaultFolder, NULL);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save", ImVec2(buttonSizeX/2.0f - style.FramePadding.x, buttonSizeY)))
	{
		pathToSet = featureConfigPath;
		SYS_DialogSaveFile(this, &extensionsConfig, NULL, featureDefaultFolder, NULL);
	}
	ImGui::SameLine();
	ImGui::Text(featureConfigPath);
	
	if (ImGui::CollapsingHeader("Paths"))
	{
		if (ImGui::Button("Root folder", buttonSize))
		{
			pathToSet = featureRootFolderPath;
			SYS_DialogPickFolder(this, featureDefaultFolder);
		}
		ImGui::SameLine();
		ImGui::Text(featureRootFolderPath);

		if (ImGui::Button("PRG output", buttonSize))
		{
			pathToSet = settingPrgOutputPath;
			SYS_DialogSaveFile(this, &extensionsPRG, NULL, featureDefaultFolder, NULL);
		}
		ImGui::SameLine();
		ImGui::Text(settingPrgOutputPath);
	}
	if (ImGui::CollapsingHeader("Settings"))
	{
		ImGui::Columns(2);
		ImGui::Checkbox("Export to PRG", &settingExportToPrg);
		ImGui::NextColumn();
		ImGui::NextColumn();
		
	}
	*/
	
	PostRenderImGui();
}

void C64DebuggerPluginDNDK::ThreadRun(void *data)
{
	
}

void C64DebuggerPluginDNDK::DoFrame()
{
	
}

void C64DebuggerPluginDNDK::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGD("C64DebuggerPluginDDNK::SystemDialogFileOpenSelected");
	
	guiMain->LockMutex();
	
	if (pathToSet == featureConfigPath)
	{
		LoadConfig(path);
	}
	else
	{
		path->RemoveFromBeginningSlrString(featureDefaultFolder);
		char *cstr = path->GetStdASCII();
		strcpy(pathToSet, cstr);
		delete [] cstr;
	}
	
	guiMain->UnlockMutex();
}

void C64DebuggerPluginDNDK::SystemDialogFileOpenCancelled()
{
	LOGD("C64DebuggerPluginDDNK::SystemDialogFileOpenCancelled");
}

void C64DebuggerPluginDNDK::SystemDialogFileSaveSelected(CSlrString *path)
{
	LOGD("CViewPluginMapper::SystemDialogFileSaveSelected");

	guiMain->LockMutex();
	
	if (pathToSet == featureConfigPath)
	{
		SaveConfig(path);
	}
	else
	{
		path->RemoveFromBeginningSlrString(featureDefaultFolder);
		char *cstr = path->GetStdASCII();
		strcpy(pathToSet, cstr);
		delete [] cstr;
	}
	
	guiMain->UnlockMutex();
}

void C64DebuggerPluginDNDK::SystemDialogFileSaveCancelled()
{
	LOGD("C64DebuggerPluginDDNK::SystemDialogFileSaveCancelled");
}

void C64DebuggerPluginDNDK::SystemDialogPickFolderSelected(CSlrString *path)
{
	LOGD("C64DebuggerPluginDDNK::SystemDialogPickFolderSelected");

	guiMain->LockMutex();
	
	if (pathToSet == featureRootFolderPath)
	{
		path->AddPathSeparatorAtEnd();
		char *cstr = path->GetStdASCII();
		strcpy(featureRootFolderPath, cstr);
		delete [] cstr;
		
		featureSettings->SetSlrString("DefaultFolder", &path);
		featureDefaultFolder->Set(path);
	}
	else
	{
		char *cstr = path->GetStdASCII();
		strcpy(pathToSet, cstr);
		delete [] cstr;
	}
	
	guiMain->UnlockMutex();
}

void C64DebuggerPluginDNDK::SystemDialogPickFolderCancelled()
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

