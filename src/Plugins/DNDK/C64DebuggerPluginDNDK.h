#ifndef _C64DebuggerPluginDDNK_h_
#define _C64DebuggerPluginDDNK_h_

#include "CDebuggerEmulatorPluginNestopia.h"
#include "CDebuggerApi.h"
#include "CGuiView.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "CFeatureConfig.h"
#include <list>

// TODO: move me to generic plugin apis
void PLUGIN_DdnkInit();
void PLUGIN_DdnkSetVisible(bool isVisible);

class CImageData;
class CConfigStorageHjson;

class C64DebuggerPluginDNDK : public CDebuggerEmulatorPluginNestopia, public CGuiView, CSlrThread,
CGuiButtonCallback, CGuiViewMenuCallback, CSystemFileDialogCallback, CFeatureConfig
{
public:
	C64DebuggerPluginDNDK(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~C64DebuggerPluginDNDK();

	virtual void Init();
	virtual void RenderImGui();
	virtual void ThreadRun(void *data);
	virtual void DoFrame();

	//
	char *assembleTextBuf;
	void GenerateCode();

	//
	const ImGuiInputTextFlags defaultHexInputFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue;

};

#endif
