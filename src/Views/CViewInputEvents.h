#ifndef _CViewInputEvents_H_
#define _CViewInputEvents_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterface;

class CViewInputEvents : public CGuiView, CGuiEditHexCallback
{
public:
	CViewInputEvents(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterface *debugInterface;

	float fontSize;	
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	//
	virtual void RenderFocusBorder();

};


#endif

