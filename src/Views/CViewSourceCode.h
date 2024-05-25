#ifndef _CVIEWC64SOURCECODE_H_
#define _CVIEWC64SOURCECODE_H_

#include "CGuiView.h"
#include "CGuiEditHex.h"
#include "CGuiEditBoxText.h"
#include "CSlrTextParser.h"
#include "C64Opcodes.h"
#include <list>
#include <vector>

class CDataAdapter;
class CSlrFont;
class CDebugInterface;
class CSlrMutex;
class CSlrString;
class CBreakpointAddr;
class CViewDataMap;
class CSlrKeyboardShortcut;
class CViewDisassembly;
class CDebugSymbols;

class CViewSourceCode : public CGuiView, CGuiEditHexCallback, CGuiEditBoxTextCallback
{
public:
	CViewSourceCode(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					CDebugInterface *debugInterface, CDebugSymbols *debugSymbols,
					CViewDisassembly *viewDisassembly);
	virtual ~CViewSourceCode();

	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void DoLogic();

	virtual bool DoTap(float x, float y);

	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	CViewDisassembly *viewDisassembly;
	CDebugSymbols *debugSymbols;
	CDataAdapter *dataAdapter;
	CDebugInterface *debugInterface;
	
	CSlrFont *font;
	float fontSize;
	
	void ScrollDown();
	void ScrollUp();
	
	void ScrollToAddress(int addr);
	
	int currentPC;

	void SetViewParameters(float posX, float posY, float posZ, float sizeX, float sizeY, CSlrFont *font, float fontSize);
	
	bool changedByUser;
	int cursorAddress;
	
	bool showLineNumbers;
	bool showFilePath;
	
	int editCursorPos;
	
	// local copy of ram
	uint8 *memory;
	int memoryLength;
	void UpdateLocalMemoryCopy(int startAddress, int endAddress);

	void SetCursorToNearExecuteCodeAddress(int newCursorAddress);
	
	//
	virtual bool IsInside(float x, float y);

	virtual void RenderFocusBorder();

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};



#endif //_CVIEWC64SOURCECODE_H_
