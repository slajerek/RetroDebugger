#ifndef _CVIEWC64MONITORCONSOLE_H_
#define _CVIEWC64MONITORCONSOLE_H_

#include "CGuiView.h"
#include "CGuiViewConsole.h"
#include "CViewC64.h"
#include "SYS_FileSystem.h"
#include "CDebugInterface.h"

class CDataAdapter;

class CViewMonitorConsole : public CGuiView, CGuiViewConsoleCallback, CSystemFileDialogCallback, CDebugInterfaceCodeMonitorCallback
{
public:
	CViewMonitorConsole(char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);

	CDebugInterface *debugInterface;
	
	CGuiViewConsole *viewConsole;
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY, float fontScale);
	
	float fontScale;
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	virtual void Render();
	virtual void RenderImGui();

	virtual void GuiViewConsoleExecuteCommand(char *commandText);
	
	void PrintInitPrompt();
	
	// tokenizer
	CSlrString *strCommandText;
	std::vector<CSlrString *> *tokens;
	
	int tokenIndex;
	
	bool GetToken(CSlrString **token);
	bool GetTokenValueHex(int *value);
	
	void UpdateDataAdapters();
	
	void CommandHelp();
	void CommandDevice();
	void CommandFill();
	void CommandCompare();
	void CommandTransfer();
	
	std::map<int, int> previousHuntAddrs;
	void CommandHunt();
	void CommandHuntContinue();
	
	void CommandMemoryDump();
	void CommandMemorySave();
	void CommandMemorySavePRG();
	void CommandMemorySaveDump();
	void CommandMemoryLoad();
	void CommandMemoryLoadDump();
	void CommandGoJMP();
	void CommandDisassemble();
	void CommandDoDisassemble();
	
	int saveFileType;
	
	bool memoryDumpAsPRG;

	void DoMemoryDump(int addrStart, int addrEnd);
	bool DoMemoryDumpToFile(int addrStart, int addrEnd, bool isPRG, CSlrString *filePath);
	bool DoMemoryDumpFromFile(int addrStart, bool isPRG, CSlrString *filePath);

	bool DoDisassembleMemory(int addrStart, int addrEnd, bool withLabels, CSlrString *filePath);
	void DisassembleHexLine(int addr, CByteBuffer *buffer);
	int DisassembleLine(int addr, uint8 op, uint8 lo, uint8 hi, CByteBuffer *buffer);
	void DisassemblePrint(CByteBuffer *byteBuffer, char *text);
	bool disassembleHexCodes;
	bool disassembleShowLabels;
	
	uint8 device;
	
	CViewDataDump *viewDataDump;
	
	CDataAdapter *dataAdapter;
	
	int addrStart, addrEnd;
	uint8 *memory;
	int memoryLength;
	CViewMemoryMap *memoryMap;
	
	std::list<CSlrString *> memoryExtensions;
	std::list<CSlrString *> prgExtensions;
	std::list<CSlrString *> disassembleExtensions;
	
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();

	//
	void StoreMonitorHistory();
	void RestoreMonitorHistory();

	// callback
	virtual void CodeMonitorCallbackPrintLine(CSlrString *printLine);

	//
	virtual void ActivateView();

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};

#endif //_CVIEWC64MONITORCONSOLE_H_

