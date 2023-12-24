#include "DBG_Log.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CSlrString.h"

// TODO: refactor this to have a proper constructor, review who deletes the char *labelText buffer
//       also add comment
CDebugSymbolsCodeLabel::CDebugSymbolsCodeLabel(CDebugSymbolsSegment *segment)
{
	this->segment = segment;

	textHashCode = 0;
	memset(labelText, 0, MAX_LABEL_TEXT_BUFFER_SIZE);
	comment = NULL;
}

CDebugSymbolsCodeLabel::~CDebugSymbolsCodeLabel()
{
}

char *CDebugSymbolsCodeLabel::GetLabelText()
{
	return this->labelText;
}

void CDebugSymbolsCodeLabel::SetText(char *txt)
{
	strncpy(labelText, txt, MAX_LABEL_TEXT_BUFFER_SIZE-1);
	
	UpdateHashCode();
	segment->UpdateCodeLabelsArray();
}

void CDebugSymbolsCodeLabel::UpdateHashCode()
{
//	char *buf = SYS_GetCharBuf();
//	strncpy(buf, labelText, MAX_STRING_LENGTH-1);
//	FUN_ToUpperCaseStr(buf);
	
	textHashCode = GetHashCode64(labelText);
	
//	SYS_ReleaseCharBuf(buf);
}

void CDebugSymbolsCodeLabel::Serialize(Hjson::Value hjsonCodeLabel)
{
	char hexStr[9];

	sprintf(hexStr, "%04x", address);
	hjsonCodeLabel["Address"] = hexStr;
	hjsonCodeLabel["Name"] = labelText;
	if (comment)
	{
		hjsonCodeLabel["Comment"] = comment;
	}
}

void CDebugSymbolsCodeLabel::Deserialize(Hjson::Value hjsonCodeLabel)
{
	const char *hexValueStr;
	hexValueStr = static_cast<const char *>(hjsonCodeLabel["Address"]);
	address = strtoul( hexValueStr, NULL, 16 );
	
	const char *labelTextStr;
	labelTextStr = static_cast<const char *>(hjsonCodeLabel["Name"]);
	strncpy(labelText, labelTextStr, MAX_LABEL_TEXT_BUFFER_SIZE-1);
	
	UpdateHashCode();
	
	if (comment != NULL)
		STRFREE(comment);
	
	try {
		const char *commentTextStr;
		commentTextStr = static_cast<const char *>(hjsonCodeLabel["Comment"]);
		comment = STRALLOC(commentTextStr);
	}
	catch (const std::exception& e)
	{
		// comment not found
	}
}

