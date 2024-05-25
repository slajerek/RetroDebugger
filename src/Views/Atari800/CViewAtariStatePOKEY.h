#ifndef _CViewAtariStatePOKEY_H_
#define _CViewAtariStatePOKEY_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

extern "C" {
#include "AtariWrapper.h"
}

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceAtari;
class CViewWaveform;

class CViewAtariStatePOKEY : public CGuiView, CGuiEditHexCallback
{
public:
	CViewAtariStatePOKEY(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterfaceAtari *debugInterface;

	float fontSize;	
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void SetVisible(bool isVisible);
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	// [pokey num][channel num]
	CViewWaveform *viewChannelWaveform[MAX_NUM_POKEYS][4];
	CViewWaveform *viewMixWaveform[MAX_NUM_POKEYS];
	
	//
	virtual void RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int pokeyNum);
	
	// editing registers
	bool showRegistersOnly;
	int editingRegisterValueIndex;		// -1 means no editing
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	
	void UpdateFontSize();
	
	//
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	//
	virtual void RenderFocusBorder();

};


#endif

