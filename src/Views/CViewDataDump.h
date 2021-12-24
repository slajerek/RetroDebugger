#ifndef _CVIEWC64DATADUMP_H_
#define _CVIEWC64DATADUMP_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewMemoryMap;
class CSlrMutex;
class CDebugInterface;
class CViewDisassembly;
class CDebugSymbols;

class CViewDataDump : public CGuiView, CGuiEditHexCallback
{
public:
	CViewDataDump(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
				  CDebugSymbols *symbols, CViewMemoryMap *viewMemoryMap, CViewDisassembly *viewDisassemble);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual bool DoTap(float x, float y);
	virtual bool DoRightClick(float x, float y);

	CDebugSymbols *symbols;
	CDebugInterface *debugInterface;
	
	CSlrFont *fontBytes;
	CSlrFont *fontCharacters;
	
	float fontSize;
	
	float fontBytesSize;
	float fontCharactersSize;
	float fontCharactersWidth;
	float markerSizeX, markerSizeY;
	
	u8 numDigitsInAddress;
	char digitsAddressFormat[8];
	char digitsAddressFormatUpperCase[8];
	void SetNumDigitsInAddress(int numDigits);

	int numberOfBytesPerLine;
	
	CDataAdapter *dataAdapter;
	CViewMemoryMap *viewMemoryMap;
	CViewDisassembly *viewDisassemble;

	void SetDataAdapter(CDataAdapter *newDataAdapter);
	
	int dataShowStart;
	int dataShowEnd;
	int dataShowSize;
	
//	bool isVisibleEditCursor;
	int editCursorPositionX;
	int editCursorPositionY;
	int dataAddr;
	int numberOfLines;
	
	void ScrollDataUp();
	void ScrollDataPageUp();
	void ScrollDataDown();
	void ScrollDataPageDown();
	
	bool FindDataPosition(float x, float y, int *dataPositionX, int *dataPositionY, int *dataPositionAddr);
	int GetAddrFromDataPosition(int dataPositionX, int dataPositionY);
	
	virtual void SetPosition(float posX, float posY, float sizeX, float sizeY);
	virtual void SetPosition(float posX, float posY, float sizeX, float sizeY, bool recalculateFontSizes);

	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	void RecalculateFontSizes();
	
	virtual void Render();
	virtual void RenderImGui();
	float gapAddress;
	float gapHexData;
	float gapDataCharacters;

	virtual void DoLogic();

	std::list<CImageData *> charactersImageData;
	std::list<CSlrImage *> charactersImages;
	
	std::list<CImageData *> spritesImageData;
	std::list<CSlrImage *> spritesImages;

	int selectedCharset;
	CSlrFont *fontCBM1;
	CSlrFont *fontCBM2;
	CSlrFont *fontAtari;
	
	CSlrFont *fonts[5];

	
	bool isEditingValue;
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);

	bool isEditingValueAddr;

	CSlrString *strTemp;
	
	volatile bool renderDataWithColors;
	
	int numberOfCharactersToShow;
	void UpdateCharacters(bool useColors, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800);
	
	int numberOfSpritesToShow;
	void UpdateSprites(bool useColors, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027);
	
	void ScrollToAddress(int address);
	void ScrollToAddress(int address, bool updateDataShowStart);
	
	long previousClickTime;
	int previousClickAddr;
	
	void PasteHexValuesFromClipboard();
	void CopyHexValuesToClipboard();
	void CopyHexAddressToClipboard();
	
	bool showDataCharacters;
	bool showCharacters;
	bool showSprites;
	
	// Layout
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);
	
private:
	char localLabelText[MAX_STRING_LENGTH];
};


#endif

