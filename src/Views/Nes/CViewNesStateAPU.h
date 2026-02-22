#ifndef _CViewNesStateAPU_H_
#define _CViewNesStateAPU_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include "CDebugInterfaceNes.h"
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
class CViewWaveform;

// TODO: move me to NES APU VIEW
class CViewNesPianoKeyboard;

class CViewNesStateAPU : public CGuiView, CGuiEditHexCallback
{
public:
	CViewNesStateAPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterfaceNes *debugInterface;

	float fontSize;
	bool hasManualFontSize;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void SetVisible(bool isVisible);
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	// [apu num][channel num]
	// square0, square1, triangle, noise, dmc, extChannel
	CViewWaveform *viewChannelWaveform[MAX_NUM_NES_APUS][6];
	CViewWaveform *viewMixWaveform[MAX_NUM_NES_APUS];

	//
	virtual void RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int apuId);
	float GetFrequencyForChannel(int chanNum);
	
	bool IsChannelActive(int chanNum);
	
	// editing registers
	bool showRegistersOnly;
	int editingRegisterValueIndex;		// -1 means no editing
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	
	void UpdateFontSize();

	//
	virtual void RenderFocusBorder();

	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	//
	CViewNesPianoKeyboard *viewPianoKeyboard;
};


#endif

