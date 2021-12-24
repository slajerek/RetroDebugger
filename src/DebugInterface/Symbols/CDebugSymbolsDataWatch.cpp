#include "DBG_Log.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugSymbolsDataWatch.h"
#include "CSlrString.h"

CDebugSymbolsDataWatch::CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment)
{
	this->segment = segment;
}

CDebugSymbolsDataWatch::CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr)
{
	this->segment = segment;
	this->address = addr;
	this->representation = WATCH_REPRESENTATION_HEX_8;
	this->numberOfValues = 1;

	// TODO: remove me, we have labels
	this->watchName = STRALLOC(name);
}

CDebugSymbolsDataWatch::CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr, int representation, int numberOfValues)
{
	this->segment = segment;
	this->address = addr;
	this->representation = representation;
	this->numberOfValues = numberOfValues;

	// TODO: remove me, we have labels
	this->watchName = STRALLOC(name);
}

void CDebugSymbolsDataWatch::SetName(char *name)
{
	if (this->watchName != NULL)
		STRFREE(this->watchName);
	
	this->watchName = STRALLOC(name);
}

void CDebugSymbolsDataWatch::Serialize(Hjson::Value hjsonWatch)
{
	char hexStr[9];

	sprintf(hexStr, "%04x", address);
	hjsonWatch["Address"] = hexStr;
	hjsonWatch["Format"] = RepresentationToStr(representation);
}

void CDebugSymbolsDataWatch::Deserialize(Hjson::Value hjsonWatch)
{
	const char *hexValueStr;
	hexValueStr = static_cast<const char *>(hjsonWatch["Address"]);
	address = strtoul( hexValueStr, NULL, 16 );
	
	const char *representationStr = hjsonWatch["Format"];
	representation = StrToRepresentation(representationStr);
}

CDebugSymbolsDataWatch::~CDebugSymbolsDataWatch()
{
	STRFREE(watchName);
}

static const char *representationNames[] = { "hex8", "hex16le", "hex16be", "hex32le", "hex32be", "udec8", "udec16le", "udec16be", "udec32le", "udec32be", "sdec8", "sdec16le", "sdec16be", "sdec32le", "sdec32be", "bin", "text" };

const char *CDebugSymbolsDataWatch::RepresentationToStr(int representation)
{
	return representationNames[representation];
}

const int CDebugSymbolsDataWatch::StrToRepresentation(const char *str)
{
	for (int i = 0; i < WATCH_REPRESENTATION_MAX; i++)
	{
		if (!strcmp(str, representationNames[i]))
		{
			return i;
		}
	}
	return -1;
}
