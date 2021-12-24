#ifndef _CDebugSymbolsSegmentDrive1541_h_
#define _CDebugSymbolsSegmentDrive1541_h_

#include "CDebugSymbolsSegment.h"

class CDebugSymbolSegmentDrive1541 : public CDebugSymbolsSegment
{
public:
	CDebugSymbolSegmentDrive1541(CDebugSymbols *debugSymbols, CSlrString *name, int segmentNum);
	~CDebugSymbolSegmentDrive1541();
	
	int driveNum;
	
	bool breakOnDrive1541IrqVIA1;
	bool breakOnDrive1541IrqVIA2;
	bool breakOnDrive1541IrqIEC;
	
	void AddBreakpointVIA1();
	void AddBreakpointVIA2();
	void AddBreakpointIEC();

	virtual void ClearBreakpoints();
	virtual void UpdateRenderBreakpoints();
	
	// parse segment-specific symbols
	virtual bool ParseSymbolsXML(CSlrString *command, std::vector<CSlrString *> *words);
};

#endif //_CDebugSymbolsSegmentDrive1541_h_
