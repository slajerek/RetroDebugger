#ifndef _CVIEWEMULATIONCOUNTERS_H_
#define _CVIEWEMULATIONCOUNTERS_H_

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

class CViewEmulationCounters : public CGuiView, CGuiEditHexCallback
{
public:
	CViewEmulationCounters(char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	CDebugInterface *debugInterface;

	float fontSize;
	bool hasManualFontSize;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	//
	virtual void RenderEmulationCounters(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize);
	
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

};


#endif

