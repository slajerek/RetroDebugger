#include "DBG_Log.h"
#include <cstdio>
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugSymbolsDataWatch.h"
#include "CSlrString.h"
#include "CDebugInterface.h"
#include "CDebugDataAdapter.h"

CDebugSymbolsDataWatch::CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment)
{
	this->segment = segment;
	this->address = 0xBEBEBEBE;
	this->representation = WATCH_REPRESENTATION_HEX_8;
	this->numberOfValues = 1;
	this->hasPhysical = false;

	// TODO: remove me, we have labels
	this->watchName = STRALLOC("");
}

CDebugSymbolsDataWatch::CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr)
{
	this->segment = segment;
	this->address = addr;
	this->representation = WATCH_REPRESENTATION_HEX_8;
	this->numberOfValues = 1;
	this->hasPhysical = false;

	// TODO: remove me, we have labels
	this->watchName = STRALLOC(name);
}

CDebugSymbolsDataWatch::CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr, int representation, int numberOfValues)
{
	this->segment = segment;
	this->address = addr;
	this->representation = representation;
	this->numberOfValues = numberOfValues;
	this->hasPhysical = false;

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
	CDebugSymbolsCodeLabel *label = segment->FindLabel(address);
	if (label)
	{
		hjsonWatch["Label"] = label->GetLabelText();
	}
	char hexStr[9];
	sprintf(hexStr, "%04x", address);
	hjsonWatch["Address"] = hexStr;
	
	hjsonWatch["Format"] = RepresentationToStr(representation);
	if (hasPhysical)
	{
		Hjson::Value hjsonMemory;
		hjsonMemory["Device"] = physical.device;
		hjsonMemory["Kind"] = DebugMemoryKindName(physical.kind);
		hjsonMemory["Region"] = (int)physical.region;
		hjsonMemory["Offset"] = (int)physical.offset;
		hjsonWatch["Memory"] = hjsonMemory;
	}
}

void CDebugSymbolsDataWatch::Deserialize(Hjson::Value hjsonWatch)
{
	Hjson::Value hjsonLabel = hjsonWatch["Label"];
	if (hjsonLabel.type() != Hjson::Type::Undefined)
	{
		const char *labelStr = hjsonWatch["Label"];
		CDebugSymbolsCodeLabel *label = segment->FindLabelByText(labelStr);
		if (label)
		{
			address = label->address;
			if (watchName)
			{
				STRFREE(watchName);
			}
			watchName = STRALLOC(label->GetLabelText());
		}
		else
		{
			watchName = STRALLOC("");
			const char *hexValueStr;
			hexValueStr = static_cast<const char *>(hjsonWatch["Address"]);
			address = strtoul( hexValueStr, NULL, 16 );
		}
	}
	else
	{
		const char *hexValueStr;
		hexValueStr = static_cast<const char *>(hjsonWatch["Address"]);
		address = strtoul( hexValueStr, NULL, 16 );
	}
	
	const char *representationStr = hjsonWatch["Format"];
	representation = StrToRepresentation(representationStr);

	Hjson::Value hjsonMemory = hjsonWatch["Memory"];
	hasPhysical = false;
	if (hjsonMemory.type() == Hjson::Type::Map && hjsonMemory["Device"].type() == Hjson::Type::String
		&& hjsonMemory["Kind"].type() == Hjson::Type::String)
	{
		std::string kindName = hjsonMemory["Kind"].to_string();
		for (int k = (int)DebugMemoryKind::Unknown; k <= (int)DebugMemoryKind::ExtendedRam; k++)
		{
			if (kindName == DebugMemoryKindName((DebugMemoryKind)k))
			{
				physical.kind = (DebugMemoryKind)k;
				physical.device = hjsonMemory["Device"].to_string();
				physical.region = (u32)hjsonMemory["Region"].to_int64();
				physical.offset = (u32)hjsonMemory["Offset"].to_int64();
				hasPhysical = physical.IsKnown();
				break;
			}
		}
	}
}

// RD-059
static CDebugMemoryMappingResolver *ResolverFor(CDebugSymbolsSegment *segment)
{
	CDebugSymbols *symbols = segment ? segment->symbols : NULL;
	CDebugInterface *debugInterface = symbols ? symbols->debugInterface : NULL;
	// the machine's resolver maps its main CPU: not a drive's symbols
	if (debugInterface == NULL || debugInterface->symbols != symbols)
		return NULL;
	return debugInterface->GetMemoryMappingResolver();
}

bool CDebugSymbolsDataWatch::BindToMemory()
{
	hasPhysical = false;
	CDebugMemoryMappingResolver *resolver = ResolverFor(segment);
	if (resolver == NULL || address < 0 || address > 0xFFFF)
		return false;
	CDebugMappedAddress written = resolver->ResolveWrite((u16)address);
	DebugMemoryKind k = written.physical.kind;
	if (k == DebugMemoryKind::Ram || k == DebugMemoryKind::ExtendedRam || k == DebugMemoryKind::CartridgeRam)
	{
		physical = written.physical;
		hasPhysical = true;
		return true;
	}
	CDebugMappedAddress read = resolver->ResolveRead((u16)address);
	k = read.physical.kind;
	if (k == DebugMemoryKind::Ram || k == DebugMemoryKind::ExtendedRam || k == DebugMemoryKind::CartridgeRam
		|| k == DebugMemoryKind::Rom || k == DebugMemoryKind::CartridgeRom)
	{
		physical = read.physical;
		hasPhysical = true;
		return true;
	}
	return false;
}

u8 CDebugSymbolsDataWatch::ReadByte(int index, bool *elsewhere)
{
	if (elsewhere)
		*elsewhere = false;
	int at = (address + index) & 0xFFFF;
	CDebugMemoryMappingResolver *resolver = hasPhysical ? ResolverFor(segment) : NULL;
	if (resolver)
	{
		CDebugPhysicalAddress p = physical;
		p.offset += (u32)index;
		u8 v = 0;
		if (!(resolver->ResolveRead((u16)at).physical == p) && resolver->ReadPhysical(p, &v))
		{
			if (elsewhere)
				*elsewhere = true;
			return v;
		}
	}
	return segment->symbols->dataAdapter->AdapterReadByteModulus(at);
}

std::string CDebugSymbolsDataWatch::MemoryName() const
{
	if (!hasPhysical)
		return "";
	char buf[64];
	if (physical.kind == DebugMemoryKind::ExtendedRam)
		snprintf(buf, sizeof(buf), "%s bank %u", physical.device.c_str(), physical.region);
	else if (physical.device == "c64-cart")
		snprintf(buf, sizeof(buf), "%s bank %u", physical.region == 0 ? "ROML" : "ROMH", physical.offset >> 13);
	else if (physical.kind == DebugMemoryKind::CartridgeRom || physical.kind == DebugMemoryKind::CartridgeRam)
		snprintf(buf, sizeof(buf), "%s $%05X", physical.device.c_str(), physical.offset);
	else
		snprintf(buf, sizeof(buf), "%s", physical.device.c_str());
	return buf;
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
