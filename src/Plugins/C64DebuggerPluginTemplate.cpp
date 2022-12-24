#include "C64DebuggerPluginTemplate.h"
#include "CConfigStorageHjson.h"
#include "CDebugInterfaceVice.h"
#include <map>
#include <string>

#define A(fmt, ...) sprintf(assembleTextBuf, fmt, ## __VA_ARGS__); api->Assemble64TassAddLine(assembleTextBuf);

//
C64DebuggerPluginTemplate *pluginTemplate = NULL;

void PLUGIN_TemplateInit()
{
	if (pluginTemplate == NULL)
	{
		pluginTemplate = new C64DebuggerPluginTemplate(300, 30, 600, 400);
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginTemplate);
	}
}

void PLUGIN_TemplateSetVisible(bool isVisible)
{
	if (pluginTemplate != NULL)
	{
		pluginTemplate->SetVisible(isVisible);
	}
}

C64DebuggerPluginTemplate::C64DebuggerPluginTemplate(float posX, float posY, float sizeX, float sizeY)
: CGuiView(posX, posY, -1, sizeX, sizeY)
{
	InitImGuiView("Plugin Template config");

	assembleTextBuf = new char[1024];

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

C64DebuggerPluginTemplate::~C64DebuggerPluginTemplate()
{
	
}

void C64DebuggerPluginTemplate::Init()
{
	api->AddView(this);
	api->StartThread(this);
}

//
void C64DebuggerPluginTemplate::StoreToHjson(Hjson::Value hjsonRoot)
{
	char hexStr[16];
	hjsonRoot["ExportToPrg"] = settingExportToPrg;
	hjsonRoot["PrgOutputPath"] = settingPrgOutputPath;

//	sprintf(hexStr, "%04x", PACKED_TEXTURES_ADDR);
//	hjsonRoot["PackedTexturesAddr"] = hexStr;
}

void C64DebuggerPluginTemplate::InitFromHjson(Hjson::Value hjsonRoot)
{
	const char *hexValueStr;
	char *p;
	
	settingExportToPrg 	= static_cast<const bool>(hjsonRoot["ExportToPrg"]);
	strcpy(settingPrgOutputPath, static_cast<const char *>(hjsonRoot["PrgOutputPath"]));	

//	hexValueStr = static_cast<const char *>(hjsonRoot["PackedTexturesAddr"]);
//	PACKED_TEXTURES_ADDR = strtoul( hexValueStr, NULL, 16 );
}


void C64DebuggerPluginTemplate::RenderImGui()
{
	PreRenderImGui();
	
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImVec2 label_size = ImGui::CalcTextSize("@", NULL, true);
	float buttonSizeY = label_size.y + style.FramePadding.y * 2.0f;
	float buttonSizeX = 150;
	
	ImVec2 buttonSize(buttonSizeX, buttonSizeY);

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
	
	PostRenderImGui();
}

void C64DebuggerPluginTemplate::ThreadRun(void *data)
{
	
}

void C64DebuggerPluginTemplate::DoFrame()
{
	
}

void C64DebuggerPluginTemplate::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGD("C64DebuggerPluginTemplate::SystemDialogFileOpenSelected");
	
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

void C64DebuggerPluginTemplate::SystemDialogFileOpenCancelled()
{
	LOGD("C64DebuggerPluginTemplate::SystemDialogFileOpenCancelled");
}

void C64DebuggerPluginTemplate::SystemDialogFileSaveSelected(CSlrString *path)
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

void C64DebuggerPluginTemplate::SystemDialogFileSaveCancelled()
{
	LOGD("C64DebuggerPluginTemplate::SystemDialogFileSaveCancelled");
}

void C64DebuggerPluginTemplate::SystemDialogPickFolderSelected(CSlrString *path)
{
	LOGD("C64DebuggerPluginTemplate::SystemDialogPickFolderSelected");

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

void C64DebuggerPluginTemplate::SystemDialogPickFolderCancelled()
{
}

///
void C64DebuggerPluginTemplate::GenerateCode()
{
	A("                     *=$1000");
	A("START:       		INC $D020");
	A("                     JMP START");

	int codeStartAddr;
	int codeSize;
	
	api->Assemble64Tass(&codeStartAddr, &codeSize);
}

