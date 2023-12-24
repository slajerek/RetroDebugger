#ifndef _CDebugSymbolsCodeLabel_h_
#define _CDebugSymbolsCodeLabel_h_

#include "SYS_Defs.h"
#include "CDebugSymbols.h"
#include "hjson.h"

class CDebugSymbols;
class CDebugSymbolsSegment;

class CDebugSymbolsCodeLabel
{
public:
	CDebugSymbolsCodeLabel(CDebugSymbolsSegment *segment);
	virtual ~CDebugSymbolsCodeLabel();
	void SetText(char *txt);
	u16 address;
	
	u64 textHashCode;
	void UpdateHashCode();
	
//	CDebugSymbols *symbols;
	CDebugSymbolsSegment *segment;

	char *GetLabelText();
	
	char *comment;
	
	virtual void Serialize(Hjson::Value hjsonCodeLabel);
	virtual void Deserialize(Hjson::Value hjsonCodeLabel);
	
private:
	char labelText[MAX_STRING_LENGTH];

};

#endif

