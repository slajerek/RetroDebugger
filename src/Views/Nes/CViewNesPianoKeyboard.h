#ifndef _CVIEWNESPIANOKEYBOARD_H_
#define _CVIEWNESPIANOKEYBOARD_H_

#include "CPianoKeyboard.h"

class CViewNesPianoKeyboard : public CPianoKeyboard
{
public:
	CViewNesPianoKeyboard(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CPianoKeyboardCallback *callback);

	float prevFreq[5];
	virtual void DoLogic();
	virtual void Render();
	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

};

#endif
