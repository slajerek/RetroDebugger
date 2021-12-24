#ifndef _CDEBUGGERBREAKPOINTS_H_
#define _CDEBUGGERBREAKPOINTS_H_

#include "SYS_Defs.h"
#include "DebuggerDefs.h"
#include "GUI_Main.h"
#include "CDebugSymbols.h"
#include "imguiComboFilter.h"
#include <map>

#define BREAKPOINT_TYPE_CPU_PC				0
#define BREAKPOINT_TYPE_MEMORY				1
#define NUM_DEFAULT_BREAKPOINT_TYPES		2

#define ADDR_BREAKPOINT_ACTION_STOP				BV01
#define ADDR_BREAKPOINT_ACTION_SET_BACKGROUND	BV02

#define MEMORY_BREAKPOINT_ACCESS_WRITE			BV01
#define MEMORY_BREAKPOINT_ACCESS_READ			BV02


class CDebugSymbols;

class CBreakpointAddr
{
public:
	CBreakpointAddr(int addr);
	bool isActive;

	int addr;		
	u32 actions;
	u8 data;
};

// TODO: refactor this to 2 breakpoints (make list?)
class CBreakpointMemory : public CBreakpointAddr
{
public:
	CBreakpointMemory(int addr,
					  u32 memoryAccess, MemoryBreakpointComparison comparison, int value);
	
	u32 memoryAccess;
	int value;
	MemoryBreakpointComparison comparison;
};

class CDebugBreakpointsAddr : public ImGui::ComboFilterCallback
{
public:
	CDebugBreakpointsAddr(int breakpointType, CDebugSymbols *symbols, char *addressFormatStr, int minAddr, int maxAddr);
	~CDebugBreakpointsAddr();

	int breakpointType;
	
	char *addressFormatStr;
	int minAddr, maxAddr;
	const char *addBreakpointPopupHeadlineStr;
	const char *addBreakpointPopupAddrStr;
	ImGuiInputTextFlags addBreakpointPopupAddrInputFlags;
	
	CDebugSymbols *symbols;
	
	std::map<int, CBreakpointAddr *> breakpoints;
	
	// copy of the breakpoints used for rendering that is not synchronised, to offload mutex switching for emulators during renderings
	CSlrMutex *renderBreakpointsMutex;
	std::map<int, CBreakpointAddr *> renderBreakpoints;

	virtual void UpdateRenderBreakpoints();
	
	virtual void AddBreakpoint(CBreakpointAddr *addrBreakpoint);
	CBreakpointAddr *GetBreakpoint(int addr);
	virtual void DeleteBreakpoint(CBreakpointAddr *addrBreakpoint);
	virtual void DeleteBreakpoint(int addr);

	virtual CBreakpointAddr *EvaluateBreakpoint(int addr);
	
	virtual void ClearBreakpoints();
	virtual void RenderImGui();
	
	// for stepping over JSR, set temporary breakpoint after the JSR and run code
	int temporaryBreakpointPC;
	virtual int GetTemporaryBreakpointPC();
	virtual void SetTemporaryBreakpointPC(int address);

	
	virtual bool ComboFilterShouldOpenPopupCallback(const char *label, char *buffer, int bufferlen,
													const char **hints, int num_hints, ImGui::ComboFilterState *s);

	
protected:
	int addBreakpointPopupAddr;
	char addBreakpointPopupSymbol[256];
	int imGuiColumnsWidthWorkaroundFrame;
	int imGuiOpenPopupFrame;
	ImGui::ComboFilterState comboFilterState = {0, false};
	char comboFilterTextBuf[MAX_LABEL_TEXT_BUFFER_SIZE];
};

class CDebugBreakpointsMemory : public CDebugBreakpointsAddr
{
public:
	CDebugBreakpointsMemory(int breakpointType, CDebugSymbols *symbols, char *addressFormatStr, int minAddr, int maxAddr);
	~CDebugBreakpointsMemory();

	virtual CBreakpointMemory *EvaluateBreakpoint(int addr, int value, u32 memoryAccess);

	virtual void RenderImGui();
	
	const char *comparisonMethodsStr[MEMORY_BREAKPOINT_ARRAY_SIZE] = { "==", "!=", "<", "<=", ">", ">=" };
	static const char *MemoryComparisonToStr(MemoryBreakpointComparison comparison);

protected:
	int addBreakpointPopupValue;
	int addBreakpointPopupComparisonMethod;
};

#endif
