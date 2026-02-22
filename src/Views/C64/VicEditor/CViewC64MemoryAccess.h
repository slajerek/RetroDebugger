#ifndef _CViewC64MemoryAccess_H_
#define _CViewC64MemoryAccess_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CVicEditorLayerMemoryAccess;
class CViewDisassembly;

class CViewC64MemoryAccess : public CGuiView
{
public:
	CViewC64MemoryAccess(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual void RenderImGui();

	CVicEditorLayerMemoryAccess *layer;
	CViewDisassembly *viewDisassembly;

private:
	char inputAddrStart[8];
	char inputAddrEnd[8];
	char inputLabel[32];
	float inputColor[3];
	int nextColorIndex;

	void RenderWatchList();
	void RenderAccessLog();

	static bool ParseHexAddress(const char *str, uint16_t *outAddr);
};

#endif
