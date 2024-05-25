#ifndef _CVIEWC64STATEVIC_H_
#define _CVIEWC64STATEVIC_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
extern "C"
{
#include "ViceWrapper.h"
};

#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceC64;

// TODO: make base class to have Vice specific state rendering and editing
//       class CViewC64ViceStateVIC : public CViewC64StateVIC

class CViewC64StateVIC : public CGuiView, CGuiEditHexCallback
{
public:
	CViewC64StateVIC(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	virtual bool DoRightClick(float x, float y);

	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	float fontSize;
	
	float fontBytesSize;

	virtual void SetPosition(float posX, float posY);
	virtual void SetPosition(float posX, float posY, float sizeX, float sizeY);
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	void RenderColorRectangle(float px, float py, float ledSizeX, float ledSizeY, float gap, bool isLocked, u8 color);
	
	void UpdateSpritesImages();
	
	// render states
	virtual void RenderStateVIC(vicii_cycle_state_t *viciiState,
								float posX, float posY, float posZ, bool isVertical, bool showSprites, CSlrFont *fontBytes, float fontSize,
								bool showRegistersOnly,
								std::vector<CImageData *> *spritesImageData, std::vector<CSlrImage *> *spritesImages, bool renderDataWithColors);
	void PrintVicInterrupts(uint8 flags, char *buf);
	void UpdateVICSpritesImages(vicii_cycle_state_t *viciiState,
								std::vector<CImageData *> *spritesImageData,
								std::vector<CSlrImage *> *spritesImages, bool renderDataWithColors);
	

	CDebugInterfaceC64 *debugInterface;

	std::vector<CImageData *> *spritesImageData;
	std::vector<CSlrImage *> *spritesImages;

	bool isVertical;
	bool showSprites;
	
	volatile bool isLockedState;
	void SetIsLockedState(bool newIsLockedState);
	bool GetIsLockedState();
	u64 previousIsLockedStateFrameNum;
	
	// force colors D020-D02E, -1 = don't force
	int forceColors[0x0F];
	int forceColorD800;
	
	bool showRegistersOnly;
	
	// editing registers
	int editingRegisterValueIndex;		// -1 means no editing
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);
	int numValuesPerColumn;
	
	//
	virtual void RenderFocusBorder();

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};



#endif

