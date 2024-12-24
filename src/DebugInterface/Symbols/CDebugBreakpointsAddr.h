#ifndef _CDebugBreakpointsAddr_h_
#define _CDebugBreakpointsAddr_h_

#include "CDebugBreakpointAddr.h"
#include "GUI_Main.h"
#include "CDebugSymbols.h"
#include "imguiComboFilter.h"

class CDebugBreakpointsAddr : public ImGui::ComboFilterCallback
{
public:
	CDebugBreakpointsAddr(int breakpointType, const char *breakpointTypeStr, CDebugSymbolsSegment *segment, const char *addressFormatStr, int minAddr, int maxAddr);
	~CDebugBreakpointsAddr();

	int breakpointsType;
	const char *breakpointsTypeStr;
	const char *addressNameJsonStr;
	const char *addressFormatStr;
	int minAddr, maxAddr;
	const char *addBreakpointPopupHeadlineStr;
	const char *addBreakpointPopupAddrStr;
	ImGuiInputTextFlags addBreakpointPopupAddrInputFlags;
	const char *addBreakpointsTableColumnAddrStr;

	CDebugSymbolsSegment *segment;
	CDebugSymbols *symbols;
	
	std::map<int, CDebugBreakpointAddr *> breakpoints;
	
	// copy of the breakpoints used for rendering that is not synchronised, to offload mutex switching for emulators during renderings
	CSlrMutex *renderBreakpointsMutex;
	std::map<int, CDebugBreakpointAddr *> renderBreakpoints;

	virtual void UpdateRenderBreakpoints();
	
	// factory
	virtual CDebugBreakpoint *CreateEmptyBreakpoint();
	
	virtual void AddBreakpoint(CDebugBreakpointAddr *addrBreakpoint);
	CDebugBreakpointAddr *GetBreakpoint(int addr);
	virtual void RemoveBreakpoint(CDebugBreakpointAddr *breakpoint);
	virtual void DeleteBreakpoint(CDebugBreakpointAddr *addrBreakpoint);
	u64 DeleteBreakpoint(int addr);
	
	virtual CDebugBreakpointAddr *EvaluateBreakpoint(int addr);
	
	virtual void ClearBreakpoints();
	virtual void RenderImGui();
	
	// for stepping over JSR, set temporary breakpoint after the JSR and run code
	int temporaryBreakpointPC;
	virtual int GetTemporaryBreakpointPC();
	virtual void SetTemporaryBreakpointPC(int address);

	
	virtual bool ComboFilterShouldOpenPopupCallback(const char *label, char *buffer, int bufferlen,
													const char **hints, int num_hints, ImGui::ComboFilterState *s);

	virtual void Serialize(Hjson::Value hjsonBreakpoints);
	virtual void Deserialize(Hjson::Value hjsonBreakpoints);

protected:
	int addBreakpointPopupAddr;
	char addBreakpointPopupSymbol[256];
	int imGuiColumnsWidthWorkaroundFrame;
	int imGuiOpenPopupFrame;
	ImGui::ComboFilterState comboFilterState = {0, false};
	char comboFilterTextBuf[MAX_LABEL_TEXT_BUFFER_SIZE];
};
#endif

