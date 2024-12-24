#ifndef _CDebugSymbolsSegmentC64_h_
#define _CDebugSymbolsSegmentC64_h_

#include "CDebugSymbolsSegment.h"

#define BREAKPOINT_TYPE_RASTER_LINE			2

class CDebugSymbolsSegmentC64 : public CDebugSymbolsSegment
{
public:
	CDebugSymbolsSegmentC64(CDebugSymbols *debugSymbols, CSlrString *name, int segmentNum);
	virtual ~CDebugSymbolsSegmentC64();
	
	// additional breakpoints, c64
	bool breakOnRaster;
	CDebugBreakpointsAddr *breakpointsRasterLine;

	bool breakOnC64IrqVIC;
	bool breakOnC64IrqCIA;
	bool breakOnC64IrqNMI;

	//
	virtual void Init();

	//
	CDebugBreakpointRasterLine *AddBreakpointRaster(int rasterNum);
	u64 RemoveBreakpointRaster(int rasterNum);
	void AddBreakpointVIC();
	void AddBreakpointCIA();
	void AddBreakpointNMI();

	virtual void ClearBreakpoints();
	virtual void UpdateRenderBreakpoints();
	
	// parse segment-specific symbols
	virtual bool ParseSymbolsXML(CSlrString *command, std::vector<CSlrString *> *words);
};

#endif //_CDebugSymbolsSegmentC64_h_
