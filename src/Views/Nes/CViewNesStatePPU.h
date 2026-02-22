#ifndef _CViewNesStatePPU_H_
#define _CViewNesStatePPU_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

extern "C" {
#include "NesWrapper.h"
}

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceNes;

// TODO: move me to NES APU VIEW
class CViewNesPianoKeyboard;

class CViewNesStatePPU : public CGuiView, CGuiEditHexCallback
{
public:
	CViewNesStatePPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterfaceNes *debugInterface;

	float fontSize;
	bool hasManualFontSize;

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);
	virtual void SetVisible(bool isVisible);
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	//
	virtual void RenderState(float px, float py, float posZ);
	// editing registers
	bool showRegistersOnly;
	int editingRegisterValueIndex;		// -1 means no editing
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	
	//
	virtual void RenderFocusBorder();

};


#endif

