#ifndef _CDebugSymbolsSegment_h_
#define _CDebugSymbolsSegment_h_

#include "SYS_Defs.h"
#include "CDebugBreakpoints.h"
#include "hjson.h"
#include <vector>

class CDebugSymbols;
class CDebugAsmSource;
class CDebugAsmSourceLine;
class CDebugAsmSourceBlock;
class CDebugSymbolsCodeLabel;
class CDebugSymbolsDataWatch;
class CSlrString;

class CDebugSymbolsSegment
{
public:
	CDebugSymbolsSegment(CDebugSymbols *debugSymbols, CSlrString *name, int segmentNum);
	virtual ~CDebugSymbolsSegment();
	
	int segmentNum;
	CSlrString *name;
	
	CDebugSymbols *symbols;
	
	CDebugAsmSourceLine **codeSourceLineByMemoryAddress;
	std::vector<CDebugAsmSourceBlock *> blocks;

	// breakpoints
	
	// all types of breakpoints in one table
	int numBreakpointsByType;
	CDebugBreakpointsAddr **breakpointsByType;
	
	bool breakOnPC;
	CDebugBreakpointsAddr *breakpointsPC;
	bool breakOnMemory;
	CDebugBreakpointsMemory *breakpointsMemory;

	virtual void Init();
	virtual void UpdateRenderBreakpoints();

	//
	virtual void AddBreakpointPC(int address);
	virtual void AddBreakpointSetBackground(int address, int value);
	virtual void AddBreakpointMemory(int address, u32 memoryAccess, MemoryBreakpointComparison comparison, int value);
	
	virtual void ClearBreakpoints();

	// labels
	std::map<int, CDebugSymbolsCodeLabel *> codeLabelByAddress;
	std::map<u64, CDebugSymbolsCodeLabel *> codeLabelByTextHashCode;
	virtual void AddCodeLabel(int address, char *text);
	virtual void AddCodeLabel(CDebugSymbolsCodeLabel *codeLabel, bool updateCodeLabels);
	virtual void RemoveCodeLabel(CDebugSymbolsCodeLabel *codeLabel, bool updateCodeLabels);
	virtual CDebugSymbolsCodeLabel *CreateCodeLabel(int address, char *text);
	virtual CDebugSymbolsCodeLabel *FindLabel(int address);
	virtual CDebugSymbolsCodeLabel *FindLabelByText(const char *text);
	virtual void DeleteCodeLabels();
	virtual void UpdateCodeLabelsArray();
	const char **codeLabelsArray;
	int numCodeLabelsInArray;

	// watches
	std::map<int, CDebugSymbolsDataWatch *> watches;
	virtual void AddWatch(int addr, char *watchName);
	virtual void AddWatch(int address, int numberOfValues, CSlrString *strRepresentation);
	virtual void AddWatch(int address, char *watchName, uint8 representation, int numberOfValues);
	virtual void AddWatch(CDebugSymbolsDataWatch *watch);
	virtual CDebugSymbolsDataWatch *CreateWatch(int address, char *watchName, uint8 representation, int numberOfValues);
	virtual void DeleteWatch(int addr);
	virtual void DeleteAllWatches();

	//
	virtual void Activate();
	
	// parse segment-specific symbols
	virtual bool ParseSymbolsXML(CSlrString *command, std::vector<CSlrString *> *words);
	
	//
	virtual bool SerializeLabels(Hjson::Value hjsonCodeLabels);
	virtual bool DeserializeLabels(Hjson::Value hjsonCodeLabels);
	virtual bool SerializeWatches(Hjson::Value hjsonWatches);
	virtual bool DeserializeWatches(Hjson::Value hjsonWatches);
};

#endif

