#ifndef _CVIEWC64MEMORYBANK_H_
#define _CVIEWC64MEMORYBANK_H_

#include "SYS_Defs.h"
#include "CGuiView.h"

class CSlrFont;
class CDebugInterfaceC64;

class CViewC64MemoryBank : public CGuiView
{
public:
	CViewC64MemoryBank(const char *name, float posX, float posY, float posZ,
					   float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);

	virtual void RenderImGui();
	virtual void Render();
	virtual void DoLogic();

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	CDebugInterfaceC64 *debugInterface;
	CSlrFont *fontBytes;
	float fontSize;
	bool hasManualFontSize;

	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);
};

#endif
