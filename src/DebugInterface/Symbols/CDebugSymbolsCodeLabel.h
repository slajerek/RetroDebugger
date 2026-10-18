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

	// RD-025: a label in one bank of banked code: the image file offset of
	// its byte, so the label shown at address is the one of the bank in
	bool hasImageOffset = false;
	u64 imageOffset = 0;
	
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

