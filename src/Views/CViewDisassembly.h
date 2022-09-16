#ifndef _CVIEWC64DISASSEMBLE_H_
#define _CVIEWC64DISASSEMBLE_H_

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
class CDebugBreakpointsAddr;
class CSlrMutex;
class CSlrString;
class CBreakpointAddr;
class CViewMemoryMap;
class CViewDataDump;
class CDebugSymbols;
class CSlrKeyboardShortcut;

enum AssembleToken : uint8
{
	TOKEN_UNKNOWN,
	TOKEN_HEX_VALUE,
	TOKEN_IMMEDIATE,
	TOKEN_LEFT_PARENTHESIS,
	TOKEN_RIGHT_PARENTHESIS,
	TOKEN_COMMA,
	TOKEN_X,
	TOKEN_Y,
	TOKEN_EOF
};

struct addrPosition_t
{
	float y;
	int addr;
};

class CViewDisassembly : public CGuiView, CGuiEditHexCallback, CGuiEditBoxTextCallback
{
public:
	CViewDisassembly(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					 CDebugSymbols *symbols, CViewDataDump *viewDataDump, CViewMemoryMap *viewMemoryMap);
	virtual ~CViewDisassembly();

	virtual void Render();
	virtual void Render(float posX, float posY);
	virtual void RenderImGui();
	virtual void DoLogic();

	virtual bool DoTap(float x, float y);

	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	CViewMemoryMap *viewMemoryMap;
	CViewDataDump *viewDataDump;
	CDataAdapter *dataAdapter;
	CDebugInterface *debugInterface;
	
	
	void SetViewDataDump(CViewDataDump *viewDataDump);
	void SetViewMemoryMap(CViewMemoryMap *viewMemoryMap);

	CSlrFont *fontDisassemble;
	float fontSize;
	float fontSize3;
	float fontSize5;
	float fontSize9;
	
	int numberOfCharactersInLabel;
	float disassembledCodeOffsetX;

	int renderStartAddress;
	void CalcDisassembleStart(int startAddress, int *newStart, int *renderLinesBefore);
	void RenderDisassembly(int startAddress, int endAddress);

	int RenderDisassembleLine(float px, float py, int addr, uint8 op, uint8 lo, uint8 hi);
	void MnemonicWithArgumentToStr(u16 addr, u8 op, u8 lo, u8 hi, char *buf);
	void MnemonicWithDollarArgumentToStr(u16 addr, u8 op, u8 lo, u8 hi, char *buf);
	void RenderHexLine(float px, float py, int addr);
	
	int UpdateDisassembleOpcodeLine(float py, int addr, uint8 op, uint8 lo, uint8 hi);
	void UpdateDisassembleHexLine(float py, int addr);
	void UpdateDisassemble(int startAddress, int endAddress);

	void CalcDisassembleStartNotExecuteAware(int startAddress, int *newStart, int *renderLinesBefore);
	void RenderDisassemblyNotExecuteAware(int startAddress, int endAddress);
	void UpdateDisassembleNotExecuteAware(int startAddress, int endAddress);
	bool DoTapNotExecuteAware(float x, float y);
	
	void TogglePCBreakpoint(int addr);
	
	void ScrollDown();
	void ScrollUp();
	
	void ScrollToAddress(int addr);
	
	int currentPC;

	// these point to real breakpoints via symbols->currentSegment (emulation mutex will be locked when these are edited)
	CDebugSymbols *symbols;
	 
	int previousOpAddr;
	int nextOpAddr;
	
	float markerSizeX;
	
	int numberOfLinesBack;
	int numberOfLinesBack3;
	
	float startRenderY;
	int startRenderAddr;
	int endRenderAddr;
	int renderSkipLines;
	
	virtual void SetPosition(float posX, float posY, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	void UpdateNumberOfVisibleLines();
	float prevFrameSizeY;
	
	void SetViewParameters(float posX, float posY, float posZ, float sizeX, float sizeY, CSlrFont *font, float fontSize, int numberOfLines,
						   float mnemonicsDisplayOffsetX,
						   bool showHexCodes,
						   bool showCodeCycles, float codeCyclesDisplayOffsetX,
						   bool showLabels, int labelNumCharacters);


	void SetCurrentPC(int pc);
	
	float mnemonicsDisplayOffsetX;
	float codeCyclesDisplayOffsetX;
	
	float mnemonicsOffsetX;
	bool showHexCodes;
	bool showCodeCycles;
	float codeCyclesOffsetX;
	bool showLabels;
	bool showArgumentLabels;
	int labelNumCharacters;
	
	bool isTrackingPC;
	bool changedByUser;
	int cursorAddress;
	
	int editCursorPos;
	int wantedEditCursorPos;	// "wanted" edit cursor pos - to hold pos when moving around with arrow up/down
	
	bool isEnteringGoto;	// is user entering "goto" address?
	
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);

	CGuiEditBoxText *editBoxText;
	virtual void EditBoxTextValueChanged(CGuiEditBoxText *editBox, char *text);
	virtual void EditBoxTextFinished(CGuiEditBoxText *editBox, char *text);
	CSlrString *strCodeLine;
	
	void StartEditingAtCursorPosition(int newCursorPos, bool goLeft);
	void FinalizeEditing();
	
	void Assemble(int assembleAddress);
	int Assemble(int assembleAddress, char *lineBuffer, bool showMessage);
	int Assemble(int assembleAddress, char *lineBuffer, int *instructionOpCode, uint16 *instructionValue, char *errorMessageBuf);

	AssembleToken AssembleGetToken(CSlrTextParser *textParser);
	int AssembleFindOp(char *mnemonic);
	int AssembleFindOp(char *mnemonic, OpcodeAddressingMode addressingMode);
	
	bool isErrorCode;
	
	// local copy of ram
	uint8 *memory;
	int memoryLength;
	void UpdateLocalMemoryCopy(int startAddress, int endAddress);

	void SetCursorToNearExecuteCodeAddress(int newCursorAddress);

	void StepOverJsr();
	
	void MakeJMPToCursor();
	void MakeJMPToAddress(int address);
	void SetBreakpointPC(int address, bool setOn);
	void SetBreakpointPCIsActive(int address, bool setActive);
	
	addrPosition_t *addrPositions;
	int numAddrPositions;
	void CreateAddrPositions();
	int addrPositionCounter;
	
	std::list<u32> shortcutZones;
	
	std::vector<int> traverseHistoryAddresses;
	void MoveAddressHistoryBack();
	void MoveAddressHistoryForward();
	void MoveAddressHistoryForwardWithAddr(int addr);
	
	void PasteHexValuesFromClipboard();
	void CopyAssemblyToClipboard();
	void CopyHexAddressToClipboard();
	
	void PasteKeysFromClipboard();
	
	void SetIsTrackingPC(bool followPC);
	
	//
	virtual void GetDisassemblyBackgroundColor(float *colorR, float *colorG, float *colorB);
	
	// Layout
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

private:
	char localLabelText[MAX_STRING_LENGTH];

};



#endif //_CVIEWC64DISASSEMBLE_H_
