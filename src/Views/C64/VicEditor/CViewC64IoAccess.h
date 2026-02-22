#ifndef _CViewC64IoAccess_H_
#define _CViewC64IoAccess_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CVicEditorLayerIoAccess;
class CViewDisassembly;

class CViewC64IoAccess : public CGuiView
{
public:
	CViewC64IoAccess(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual void RenderImGui();

	CVicEditorLayerIoAccess *layer;
	CViewDisassembly *viewDisassembly;

private:
	void RenderChipSection(int chipIndex);
	void RenderAccessLogEntries();
};

#endif
