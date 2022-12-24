#ifndef _CVIEWC64STATEDRIVE1541_H_
#define _CVIEWC64STATEDRIVE1541_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterfaceC64;

// TODO: make base class to have Vice specific state rendering and editing
//       class CViewDrive1541ViceStateVIA : public CViewDrive1541StateVIA

class CViewDrive1541StateVIA : public CGuiView, CGuiEditHexCallback
{
public:
	CViewDrive1541StateVIA(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool DoTap(float x, float y);

	CSlrFont *fontBytes;	
	float fontSize;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	CDebugInterfaceC64 *debugInterface;
	
	void DumpViaInts(uint8 i, char *buf);
	
	bool renderVIA1;
	bool renderVIA2;
	bool renderDriveLED;
	bool isVertical;
	
	float stateSizeX;
	
	virtual void RenderStateDrive1541(float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize,
									  int driveId, 
									  bool renderVia1, bool renderVia2, bool renderDriveLed, bool isVertical);
	bool showRegistersOnly;
	
	// editing registers
	int editingRegisterValueIndex;		// -1 means no editing
	int editingVIAIndex;
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	
	//
	virtual void RenderFocusBorder();
	
	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);
};


#endif

