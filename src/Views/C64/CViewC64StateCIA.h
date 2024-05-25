#ifndef _CVIEWC64STATECIA_H_
#define _CVIEWC64STATECIA_H_

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

// TODO: make base class to have Vice specific state rendering and editing
//       class CViewC64ViceStateCIA : public CViewC64StateCIA

class CViewC64StateCIA : public CGuiView, CGuiEditHexCallback
{
public:
	CViewC64StateCIA(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *fontBytes;
	float fontSize;

	CDebugInterfaceC64 *debugInterface;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	
	
	bool renderCIA1;
	bool renderCIA2;
	
	void SetFontSize(float fontSize);
	float oneCiaSizeX;

	//
	virtual void RenderStateCIA(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId);
	
	// editing registers
	bool showRegistersOnly;
	int editingRegisterValueIndex;		// -1 means no editing
	int editingCIAIndex;
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	
	//
	virtual void RenderFocusBorder();

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};


#endif

