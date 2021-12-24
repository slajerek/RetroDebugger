#ifndef _CVIEWEMULATIONSTATE_H_
#define _CVIEWEMULATIONSTATE_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterfaceC64;

class CViewEmulationState : public CGuiView, CGuiEditHexCallback
{
public:
	CViewEmulationState(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool DoTap(float x, float y);

	CSlrFont *fontBytes;	
	float fontSize;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	CDebugInterfaceC64 *debugInterface;
	
	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};


#endif

