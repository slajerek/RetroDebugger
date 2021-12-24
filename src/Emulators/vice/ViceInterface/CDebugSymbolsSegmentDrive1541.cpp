#include "CDebugSymbolsSegmentDrive1541.h"
#include "CDebugSymbols.h"

CDebugSymbolSegmentDrive1541::CDebugSymbolSegmentDrive1541(CDebugSymbols *debugSymbols, CSlrString *name, int segmentNum)
: CDebugSymbolsSegment(debugSymbols, name, segmentNum)
{
	driveNum = 0;
	breakOnDrive1541IrqVIA1 = false;
	breakOnDrive1541IrqVIA2 = false;
	breakOnDrive1541IrqIEC = false;
}

void CDebugSymbolSegmentDrive1541::UpdateRenderBreakpoints()
{
	CDebugSymbolsSegment::UpdateRenderBreakpoints();
}

CDebugSymbolSegmentDrive1541::~CDebugSymbolSegmentDrive1541()
{
	CDebugSymbolsSegment::~CDebugSymbolsSegment();
}

void CDebugSymbolSegmentDrive1541::AddBreakpointVIA1()
{
	this->breakOnDrive1541IrqVIA1 = true;
}

void CDebugSymbolSegmentDrive1541::AddBreakpointVIA2()
{
	this->breakOnDrive1541IrqVIA2 = true;
}

void CDebugSymbolSegmentDrive1541::AddBreakpointIEC()
{
	this->breakOnDrive1541IrqIEC = true;
}

void CDebugSymbolSegmentDrive1541::ClearBreakpoints()
{
	CDebugSymbolsSegment::ClearBreakpoints();
}

bool CDebugSymbolSegmentDrive1541::ParseSymbolsXML(CSlrString *command, std::vector<CSlrString *> *words)
{
	if (command->Equals("breakvia1") || command->Equals("breakonvia1") || command->Equals("breakonirqvia1")
			 || command->Equals("via1"))
	{
		LOGD(".. adding breakOnDrive1541IrqVIA1");
		
		this->AddBreakpointVIA1();
		return true;
	}
	else if (command->Equals("breakvia2") || command->Equals("breakonvia2") || command->Equals("breakonirqvia2")
			 || command->Equals("via2"))
	{
		LOGD(".. adding breakOnDrive1541IrqVIA2");
		
		this->AddBreakpointVIA2();
		return true;
	}
	else if (command->Equals("breakiec") || command->Equals("breakoniec") || command->Equals("breakonirqiec")
			 || command->Equals("iec"))
	{
		LOGD(".. adding breakOnDrive1541IrqIEC");
		
		this->AddBreakpointIEC();
		return true;
	}
	
	return false;
}

