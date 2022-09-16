#ifndef _CVIEWC64VIEWFINDER_H_
#define _CVIEWC64VIEWFINDER_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewC64Screen;
class CDebugInterfaceC64;

class CViewC64ScreenViewfinder : public CGuiView
{
public:
	CViewC64ScreenViewfinder(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64Screen *viewC64Screen);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	CViewC64Screen *viewC64Screen;
	float fontSize;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();	

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};


#endif

