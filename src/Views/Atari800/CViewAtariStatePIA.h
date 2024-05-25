#ifndef _CViewAtariStatePIA_H_
#define _CViewAtariStatePIA_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceAtari;

class CViewAtariStatePIA : public CGuiView, CGuiEditHexCallback
{
public:
	CViewAtariStatePIA(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterfaceAtari *debugInterface;

	float fontSize;	
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	//
	virtual void RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId);
	
	// editing registers
	bool showRegistersOnly;
	int editingRegisterValueIndex;		// -1 means no editing
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	
	//
	virtual void RenderFocusBorder();

};


#endif

