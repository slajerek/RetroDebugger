#ifndef _CViewC64Palette_H_
#define _CViewC64Palette_H_

#include "SYS_Defs.h"
#include "CGuiWindow.h"
#include "CGuiEditHex.h"
#include "CGuiViewFrame.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterfaceC64;
class CViewC64VicEditor;
class CSlrFont;

class CViewC64PaletteCallback
{
public:
	// callback from palette on change color
	virtual void PaletteColorChanged(u8 colorSource, u8 newColorValue);
};

class CViewC64Palette : public CGuiView, public CGuiEditHexCallback
{
public:
	CViewC64Palette(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64PaletteCallback *callback);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	virtual bool DoRightClick(float x, float y);
//	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();

	virtual void SetIsVertical(bool isVertical);

	void RenderPalette(bool renderBackgroundInformation);

	//
	CViewC64PaletteCallback *callback;

	void SetPaletteRectScale(float scale);
	
	int GetColorIndex(float x, float y);
	
	void SetColorLMB(u8 color);
	void SetColorRMB(u8 color);
	
	bool isVertical;

	float gap1;
	float gap2;
	float rectSize;
	float rectSize4;
	float rectSizeBig;
	
	CSlrFont *font;
	float fontScale;
	float fontCharacterWidth;
	float fontCharacterHeight;

	//
	u8 colorD020;
	u8 colorD021;
	u8 colorLMB;
	u8 colorRMB;

	bool renderColorHexNumber;
	
	//
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
	void RenderMenuItemsAvailablePalettes();
	void SwitchSelectedColors();
};


#endif

