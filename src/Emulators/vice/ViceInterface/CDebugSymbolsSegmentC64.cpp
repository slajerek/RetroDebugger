#include "CDebugSymbolsSegmentC64.h"
#include "CDebugSymbols.h"

CDebugSymbolsSegmentC64::CDebugSymbolsSegmentC64(CDebugSymbols *debugSymbols, CSlrString *name, int segmentNum)
: CDebugSymbolsSegment(debugSymbols, name, segmentNum)
{
}

void CDebugSymbolsSegmentC64::Init()
{
	this->numBreakpointsByType = NUM_C64_BREAKPOINT_TYPES;

	CDebugSymbolsSegment::Init();

	breakOnRaster = true;
	breakpointsRasterLine = new CDebugBreakpointsAddr(BREAKPOINT_TYPE_RASTER_LINE, symbols, "%03X", 0, 0x137);
	breakpointsRasterLine->addBreakpointPopupHeadlineStr = "Add Raster line Breakpoint";
	
	breakpointsByType[BREAKPOINT_TYPE_RASTER_LINE] = breakpointsRasterLine;

	breakOnC64IrqVIC = false;
	breakOnC64IrqCIA = false;
	breakOnC64IrqNMI = false;
}

void CDebugSymbolsSegmentC64::AddBreakpointRaster(int rasterNum)
{
	CBreakpointAddr *rasterBreakpoint = new CBreakpointAddr(rasterNum);
	this->breakpointsRasterLine->AddBreakpoint(rasterBreakpoint);
	
	this->breakOnRaster = true;
}

void CDebugSymbolsSegmentC64::UpdateRenderBreakpoints()
{
	CDebugSymbolsSegment::UpdateRenderBreakpoints();
	
	breakpointsRasterLine->UpdateRenderBreakpoints();
}

CDebugSymbolsSegmentC64::~CDebugSymbolsSegmentC64()
{
	CDebugSymbolsSegment::~CDebugSymbolsSegment();
}

void CDebugSymbolsSegmentC64::AddBreakpointVIC()
{
	this->breakOnC64IrqVIC = true;
}

void CDebugSymbolsSegmentC64::AddBreakpointCIA()
{
	this->breakOnC64IrqCIA = true;
}

void CDebugSymbolsSegmentC64::AddBreakpointNMI()
{
	this->breakOnC64IrqNMI = true;
}

void CDebugSymbolsSegmentC64::ClearBreakpoints()
{
	CDebugSymbolsSegment::ClearBreakpoints();
	
	breakpointsRasterLine->ClearBreakpoints();
}

bool CDebugSymbolsSegmentC64::ParseSymbolsXML(CSlrString *command, std::vector<CSlrString *> *words)
{
	if (command->Equals("breakraster") || command->Equals("breakonraster")
			 || command->Equals("raster"))
	{
		if (words->size() < 3)
		{
			LOGError("ParseBreakpoints: error with breakraster"); //in line %d", lineNum);
			return false;
		}
		
		// raster breakpoint
		CSlrString *arg = (*words)[2];
		//arg->DebugPrint(" arg=");
		int rasterNum = arg->ToIntFromHex();
		
		LOGD(".. adding breakOnRaster %4.4x", rasterNum);

		this->AddBreakpointRaster(rasterNum);

		return true;
	}
	else if (command->Equals("breakvic") || command->Equals("breakonvic") || command->Equals("breakonirqvic")
			 || command->Equals("vic"))
	{
		LOGD(".. adding breakOnC64IrqVIC");
		
		this->AddBreakpointVIC();
		return true;
	}
	else if (command->Equals("breakcia") || command->Equals("breakoncia") || command->Equals("breakonirqcia")
			 || command->Equals("cia"))
	{
		this->AddBreakpointCIA();
		return true;
	}
	else if (command->Equals("breaknmi") || command->Equals("breakonnmi") || command->Equals("breakonirqnmi")
			 || command->Equals("nmi"))
	{
		this->AddBreakpointNMI();
		return true;
	}
	
	return false;
}

