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

#define POKEY_WAVEFORM_LENGTH 128

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
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
	CViewWaveform *pokeyChannelWaveform[MAX_NUM_POKEYS][4];
	CViewWaveform *pokeyMixWaveform[MAX_NUM_POKEYS];

	int waveformPos[MAX_NUM_POKEYS];
	void AddWaveformData(int pokeyNumber, int v1, int v2, int v3, int v4, short mix);

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

