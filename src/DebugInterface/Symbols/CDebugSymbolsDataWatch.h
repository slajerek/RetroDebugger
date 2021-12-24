#ifndef _CDebugSymbolsDataWatch_h_
#define _CDebugSymbolsDataWatch_h_

#include "SYS_Defs.h"
#include "CDebugSymbols.h"
#include "DebuggerDefs.h"
#include "hjson.h"

class CDebugSymbolsDataWatch
{
public:
	CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment);
	CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr);
	CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr, int representation, int numberOfValues);
	~CDebugSymbolsDataWatch();
	
	CDebugSymbolsSegment *segment;
	
	void SetName(char *name);
	
	char *watchName;
	int address;
	
	int representation;
	int numberOfValues;
	
	virtual void Serialize(Hjson::Value hjsonWatch);
	virtual void Deserialize(Hjson::Value hjsonWatch);

	static const char *RepresentationToStr(int representation);
	static const int StrToRepresentation(const char *str);
};


#endif
