#ifndef _CVIEWC64EMULATIONCOUNTERS_H_
#define _CVIEWC64EMULATIONCOUNTERS_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceC64;

class CViewC64EmulationCounters : public CGuiView, CGuiEditHexCallback
{
public:
	CViewC64EmulationCounters(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterfaceC64 *debugInterface;

	float fontSize;	
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	//
	virtual void RenderEmulationCounters(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize);
	
	
	//
	virtual void RenderFocusBorder();

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};


#endif

