#ifndef _CViewC64Sprite_H_
#define _CViewC64Sprite_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include "CGuiView.h"
#include "C64Sprite.h"
#include "CGuiButtonSwitch.h"
#include "CDebugInterfaceC64.h"
#include "CSystemFileDialogCallback.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterfaceC64;
class CViewC64VicEditor;
class C64Sprite;

class CViewC64Sprite : public CGuiView, CGuiEditHexCallback, CGuiButtonSwitchCallback, public CSystemFileDialogCallback
{
public:
	CViewC64Sprite(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64VicEditor *vicEditor);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoRightClick(float x, float y);
//	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	//
	CViewC64VicEditor *vicEditor;
	
	CImageData *imageDataSprite;
	CSlrImage *imageSprite;
	
	bool isSpriteLocked;
	
	int spriteRasterX;
	int spriteRasterY;
	
	float scale;
	
	CSlrFont *font;
	float fontScale;
	float fontWidth;
	float fontHeight;
	
	float fontSize;
	float spriteSizeX;
	float spriteSizeY;
	float buttonSizeX;
	float buttonSizeY;

	CGuiButtonSwitch *btnScanForSprites;

	CGuiButtonSwitch *btnIsMultiColor;
	CGuiButtonSwitch *btnIsStretchX;
	CGuiButtonSwitch *btnIsStretchY;

	int prevSpriteId;
	
	int selectedColor;
	
	u8 paintColorD021;
	u8 paintColorSprite;
	u8 paintColorD025;
	u8 paintColorD026;
	
	u8 GetPaintColorByNum(u8 colorNum);
	
	vicii_cycle_state_t *viciiState;
	
	// callback from palette on change color
	virtual void PaletteColorChanged(u8 colorSource, u8 newColorValue);

	void UpdateSelectedColorInPalette();
	
	//
	void MoveSelectedSprite(int deltaX, int deltaY);

	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);

	//
	CSlrImage *imgIconExport;
	CSlrImage *imgIconImport;

	virtual void ToolBoxIconPressed(CSlrImage *imgIcon);
	
	std::list<CSlrString *> spriteFileExtensions;
	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	
	uint8 currentSpriteData[63];

	// returns sprite addr
	int ImportSprite(CSlrString *path);
	void ExportSprite(CSlrString *path);

};


#endif

