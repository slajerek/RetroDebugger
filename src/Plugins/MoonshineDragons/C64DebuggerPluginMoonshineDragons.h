#ifndef _C64DebuggerPluginMoonshineDragons_h_
#define _C64DebuggerPluginMoonshineDragons_h_

#include "CDebuggerEmulatorPluginVice.h"
#include "CDebuggerApi.h"
#include "CGuiView.h"
#include "SYS_FileSystem.h"
#include <list>
#include <vector>

class C64DebuggerPluginMoonshineDragons :
	public CDebuggerEmulatorPluginVice,
	public CGuiView,
	CSystemFileDialogCallback
{
public:
	C64DebuggerPluginMoonshineDragons(float posX, float posY, float sizeX, float sizeY);
	virtual ~C64DebuggerPluginMoonshineDragons();

	virtual void Init();
	virtual void RenderImGui();

	void GenerateAndRun();
	void RequestSavePrgDialog();

	// Exposed for testability — same bytes used by GenerateAndRun / Save PRG.
	std::vector<u8> BuildPRG();

	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	virtual void SystemDialogFileOpenSelected(CSlrString *path) {}
	virtual void SystemDialogFileOpenCancelled() {}
	virtual void SystemDialogPickFolderSelected(CSlrString *path) {}
	virtual void SystemDialogPickFolderCancelled() {}

private:
	void WritePrgToFile(const char *filePath);

	std::list<CSlrString *> extensionsPRG;
	char statusText[256];
};

extern C64DebuggerPluginMoonshineDragons *pluginMoonshineDragons;

void PLUGIN_MoonshineDragonsInit();
void PLUGIN_MoonshineDragonsSetVisible(bool isVisible);

#endif
