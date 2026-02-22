#ifndef _C64DebuggerPluginTemplate_h_
#define _C64DebuggerPluginTemplate_h_

#include "CDebuggerEmulatorPluginVice.h"
#include "CDebuggerApi.h"
#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "CFeatureConfig.h"
#include <list>

class CImageData;
class CConfigStorageHjson;

// replace CDebuggerEmulatorPluginVice with selected class
class C64DebuggerPluginTemplate : public CDebuggerEmulatorPluginVice, public CGuiView, CSlrThread,
CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CFeatureConfig
{
public:
	C64DebuggerPluginTemplate(float posX, float posY, float sizeX, float sizeY);
	virtual ~C64DebuggerPluginTemplate();

	virtual void Init();
	virtual void RenderImGui();
	virtual void ThreadRun(void *data);
	virtual void DoFrame();

	//
	void InitFromHjson(Hjson::Value hjsonRoot);
	void StoreToHjson(Hjson::Value hjsonRoot);

	std::list<CSlrString *> extensionsConfig;
	std::list<CSlrString *> extensionsPRG;
	std::list<CSlrString *> extensionsPRGandSID;
	std::list<CSlrString *> extensionsPNG;
	
	// settings
	bool settingExportToPrg;
	char *settingPrgOutputPath;
	
	//
	char *assembleTextBuf;
	void GenerateCode();

	const ImGuiInputTextFlags defaultHexInputFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase;

	// for save/open dialogs
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	virtual void SystemDialogPickFolderSelected(CSlrString *path);
	virtual void SystemDialogPickFolderCancelled();
	char *pathToSet;
	
};

#endif
