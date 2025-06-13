#include "CDebugSymbolsSegment.h"
#include "CDebugSymbols.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "CDebugAsmSource.h"
#include "CViewDisassembly.h"
#include "CViewDataWatch.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugSymbolsDataWatch.h"
#include "C64SettingsStorage.h"

CDebugSymbolsSegment::CDebugSymbolsSegment(CDebugSymbols *debugSymbols, CSlrString *name, int segmentNum, bool supportBreakpoints)
{
	LOGD("CDebugSymbolsSegment: %x", this);
	name->DebugPrint("CDebugSymbolsSegment name=");
	
	this->symbols = debugSymbols;
	this->name = name;
	this->segmentNum = segmentNum;
	this->supportBreakpoints = supportBreakpoints;
	
	int maxAddress = debugSymbols->dataAdapter->AdapterGetDataLength();
	codeSourceLineByMemoryAddress = new CDebugAsmSourceLine* [maxAddress];
	for (int i = 0; i < maxAddress; i++)
	{
		codeSourceLineByMemoryAddress[i] = NULL;
	}
	
	numCodeLabelsInArray = 0;
	codeLabelsArray = NULL;
}

void CDebugSymbolsSegment::Init()
{
	breakOnPC = true;
	breakOnMemory = true;

	breakpointsPC = new CDebugBreakpointsAddr(BREAKPOINT_TYPE_ADDR, "CpuPC", this, "%04X", 0, 0xFFFF);
	breakpointsByType[BREAKPOINT_TYPE_ADDR] = breakpointsPC;
	
	breakpointsData = new CDebugBreakpointsData(BREAKPOINT_TYPE_DATA, "Memory", this, "%04X", 0, 0xFFFF);
	breakpointsByType[BREAKPOINT_TYPE_DATA] = breakpointsData;
}

CDebugSymbolsSegment::~CDebugSymbolsSegment()
{
	name->DebugPrint("~CDebugSymbolsSegment");
	delete name;
	
	int maxAddress = symbols->dataAdapter->AdapterGetDataLength();
	for (int i = 0; i < maxAddress; i++)
	{
		if (codeSourceLineByMemoryAddress[i])
			delete codeSourceLineByMemoryAddress[i];
	}
	delete [] codeSourceLineByMemoryAddress;
	
	while (!blocks.empty())
	{
		CDebugAsmSourceBlock *b = blocks.back();
		blocks.pop_back();
		delete b;
	}
	
	ClearBreakpoints();
	DeleteCodeLabels();
	DeleteAllWatches();
}

void CDebugSymbolsSegment::UpdateRenderBreakpoints()
{
	breakpointsPC->UpdateRenderBreakpoints();
	breakpointsData->UpdateRenderBreakpoints();
}

void CDebugSymbolsSegment::AddBreakpointPC(int address)
{
	CDebugBreakpointAddr *addrBreakpoint = this->breakpointsPC->GetBreakpoint(address);
	if (addrBreakpoint == NULL)
	{
		addrBreakpoint = new CDebugBreakpointAddr(symbols, address);
		addrBreakpoint->actions = ADDR_BREAKPOINT_ACTION_STOP;
		this->breakpointsPC->AddBreakpoint(addrBreakpoint);
	}
	else
	{
		SET_BIT(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP);
	}
}

void CDebugSymbolsSegment::AddBreakpointSetBackground(int address, int value)
{
	CDebugBreakpointAddr *addrBreakpoint = this->breakpointsPC->GetBreakpoint(address);
	if (addrBreakpoint == NULL)
	{
		addrBreakpoint = new CDebugBreakpointAddr(address);
		addrBreakpoint->actions = ADDR_BREAKPOINT_ACTION_SET_BACKGROUND;
		addrBreakpoint->data = value;
		this->breakpointsPC->AddBreakpoint(addrBreakpoint);
	}
	else
	{
		SET_BIT(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_SET_BACKGROUND);
	}
}

CDebugBreakpointData *CDebugSymbolsSegment::AddBreakpointMemory(int address, u32 memoryAccess, DataBreakpointComparison comparison, int value)
{
	CDebugBreakpointData *memBreakpoint = new CDebugBreakpointData(address, memoryAccess, comparison, value);
	this->breakpointsData->AddBreakpoint(memBreakpoint);
	this->breakOnMemory = true;
	return memBreakpoint;
}

void CDebugSymbolsSegment::ClearBreakpoints()
{
	breakpointsPC->ClearBreakpoints();
	breakpointsData->ClearBreakpoints();
}

// code labels
CDebugSymbolsCodeLabel *CDebugSymbolsSegment::CreateCodeLabel(int address, char *text)
{
	if (text[0] == 0)
		LOGError("CDebugSymbolsSegment::CreateCodeLabel: empty label text");

	CDebugSymbolsCodeLabel *label = new CDebugSymbolsCodeLabel(this);
	label->address = address;
	label->SetText(text);
	
	return label;
}

void CDebugSymbolsSegment::AddCodeLabel(int address, char *text)
{
	// sanitize, do not add empty labels
	if (text[0] == 0)
		return;
	
	CDebugSymbolsCodeLabel *codeLabel = CreateCodeLabel(address, text);
	AddCodeLabel(codeLabel, true);
}

void CDebugSymbolsSegment::AddCodeLabel(CDebugSymbolsCodeLabel *codeLabel, bool updateCodeLabels)
{
	guiMain->LockMutex();

	// check if exists
	std::map<int, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByAddress.find(codeLabel->address);
	
	if (it != codeLabelByAddress.end())
	{
		CDebugSymbolsCodeLabel *oldLabel = it->second;
		u64 textHashCode = GetHashCode64(oldLabel->GetLabelText());
		codeLabelByTextHashCode.erase(textHashCode);

		codeLabelByAddress.erase(it);
		delete oldLabel;
	}

	codeLabelByAddress[codeLabel->address] = codeLabel;

	u64 textHashCode = GetHashCode64(codeLabel->GetLabelText());
	codeLabelByTextHashCode[textHashCode] = codeLabel;

	if (updateCodeLabels)
	{
		UpdateCodeLabelsArray();
	}

	guiMain->UnlockMutex();
}

void CDebugSymbolsSegment::RemoveCodeLabel(CDebugSymbolsCodeLabel *codeLabel, bool updateCodeLabels)
{
	codeLabelByAddress.erase(codeLabel->address);
	codeLabelByTextHashCode.erase(codeLabel->textHashCode);
	if (updateCodeLabels)
		UpdateCodeLabelsArray();
}

void CDebugSymbolsSegment::DeleteCodeLabels()
{
	guiMain->LockMutex();
	
	while (!codeLabelByAddress.empty())
	{
		std::map<int, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByAddress.begin();
		CDebugSymbolsCodeLabel *label = it->second;
		
		codeLabelByTextHashCode.erase(label->textHashCode);
		codeLabelByAddress.erase(it);
		delete label;
	}
	
	UpdateCodeLabelsArray();
	
	guiMain->UnlockMutex();
}

// setup the code labels hints table, TODO: sort etc.
void CDebugSymbolsSegment::UpdateCodeLabelsArray()
{
	guiMain->LockMutex();
	
	if (codeLabelsArray)
	{
		// do not free char* items, the buffer is owned by CDebugSymbolsCodeLabel, delete array only
		delete [] codeLabelsArray;
	}

	codeLabelsArray = new const char *[codeLabelByAddress.size()];
	int i = 0;
	for (std::map<int, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByAddress.begin(); it != codeLabelByAddress.end(); it++)
	{
		CDebugSymbolsCodeLabel *label = it->second;
		codeLabelsArray[i] = label->GetLabelText();
		
		// sanity check, should not happen
		if (codeLabelsArray[i][0] == 0)
		{
			// empty string. delete label and restart process.
			RemoveCodeLabel(label, false);
			delete label;
			
			// recursion. bad, but should be fine. we do not expect many empty labels as this is sanity check only.
			UpdateCodeLabelsArray();
			
			guiMain->UnlockMutex();
			return;
		}
		i++;
	}
	
	numCodeLabelsInArray = codeLabelByAddress.size();
	
	// update map
	codeLabelByTextHashCode.clear();
	for (std::map<int, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByAddress.begin(); it != codeLabelByAddress.end(); it++)
	{
		CDebugSymbolsCodeLabel *label = it->second;
		codeLabelByTextHashCode[label->textHashCode] = label;
	}
	
	guiMain->UnlockMutex();
}

CDebugSymbolsCodeLabel *CDebugSymbolsSegment::FindLabel(int address)
{
//	LOGD("CDebugSymbolsSegment::FindLabel: this=%x address=%x", this, address);
	// check if exists
	std::map<int, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByAddress.find(address);
	
	if (it != codeLabelByAddress.end())
	{
		CDebugSymbolsCodeLabel *label = it->second;
//		LOGD(".. return %x", label);
		return label;
	}
	
//	LOGD(".. return NULL");
	return NULL;
}

// searches for label, if not found searches for -1, -2, -3, etc. and +1, +2, +3, ...
CDebugSymbolsCodeLabel *CDebugSymbolsSegment::FindNearLabel(int address, int *offset)
{
	CDebugSymbolsCodeLabel *label = FindLabel(address);
	if (label)
	{
		*offset = 0;
		return label;
	}
	
	// scan labels to find most near
	const int maxLabelAddrOffset = c64SettingsDisassemblyNearLabelMaxOffset;

	for (int i = 0; i < maxLabelAddrOffset; i++)
	{
		// scan left
		int addrL = address - i;
		if (addrL >= 0)
		{
			label = FindLabel(addrL);
			if (label)
			{
				*offset = +i;
				return label;
			}
		}

		// scan right
		int addrR = address + i;
		if (addrR < symbols->dataAdapter->AdapterGetDataLength())
		{
			label = FindLabel(addrR);
			if (label)
			{
				*offset = -i;
				return label;
			}
		}
	}
	return NULL;
}

bool CDebugSymbolsSegment::FindLabelText(int address, char *labelText)
{
	CDebugSymbolsCodeLabel *label = FindLabel(address);
	if (label)
	{
		strcpy(labelText, label->GetLabelText());
		return true;
	}
	// return empty label text
	labelText[0] = 0;
	return false;
}

bool CDebugSymbolsSegment::FindNearLabelText(int address, char *labelText)
{
	int offset;
	CDebugSymbolsCodeLabel *label = FindNearLabel(address, &offset);
	if (label)
	{
		strcpy(labelText, label->GetLabelText());
		if (offset == 0)
		{
			return true;
		}
		
		if (offset > 0)
		{
			strcat(labelText, "+");
		}
		
		char offsetStr[16];
		sprintf(offsetStr, "%d", offset);
		strcat(labelText, offsetStr);
		return true;
	}
	
	// return empty label text
	labelText[0] = 0;
	return false;
}

CDebugSymbolsCodeLabel *CDebugSymbolsSegment::FindLabelByText(const char *text)
{
	char *buf = SYS_GetCharBuf();
	strncpy(buf, text, MAX_STRING_LENGTH-1);
	u64 hashCode = GetHashCode64(buf);
	SYS_ReleaseCharBuf(buf);

	std::map<u64, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByTextHashCode.find(hashCode);
	
	if (it != codeLabelByTextHashCode.end())
	{
		CDebugSymbolsCodeLabel *label = it->second;
//		LOGD(".. return %x", label);
		return label;
	}
	
//	LOGD(".. return NULL");
	return NULL;
}


void CDebugSymbolsSegment::AddWatch(int address, char *name, uint8 representation, int numberOfValues)
{
	LOGD("CDebugSymbolsSegment::AddWatch: %04x, %s, %d %d", address, name, representation, numberOfValues);
	
	guiMain->LockMutex();

	// check if exists
	std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.find(address);
	
	if (it != watches.end())
	{
		CDebugSymbolsDataWatch *watch = it->second;
		watches.erase(it);
		delete watch;
	}
	
	CDebugSymbolsDataWatch *watch = this->CreateWatch(address, name, representation, numberOfValues);
	
	watches[address] = watch;
	
	guiMain->UnlockMutex();
}

void CDebugSymbolsSegment::AddWatch(int addr, char *watchName)
{
	this->AddWatch(addr, watchName, WATCH_REPRESENTATION_HEX_8, 1);
}

void CDebugSymbolsSegment::AddWatch(CDebugSymbolsDataWatch *watch)
{
	LOGD("CDebugSymbolsSegment::AddWatch: %04x, %d %d", watch->address, watch->representation, watch->numberOfValues);
	
	guiMain->LockMutex();

	// check if exists
	std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.find(watch->address);
	
	if (it != watches.end())
	{
		CDebugSymbolsDataWatch *w = it->second;
		watches.erase(it);
		delete w;
	}
	
	watches[watch->address] = watch;
	
	guiMain->UnlockMutex();
}

CDebugSymbolsDataWatch *CDebugSymbolsSegment::FindWatch(int address)
{
	guiMain->LockMutex();

	// check if exists
	std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.find(address);
	
	if (it != watches.end())
	{
		CDebugSymbolsDataWatch *w = it->second;
		return w;
	}
	
	guiMain->UnlockMutex();
	return NULL;
}

CDebugSymbolsDataWatch *CDebugSymbolsSegment::CreateWatch(int address, char *watchName, uint8 representation, int numberOfValues)
{
	CDebugSymbolsDataWatch *watch = new CDebugSymbolsDataWatch(this, watchName, address, representation, numberOfValues);
	return watch;
}

void CDebugSymbolsSegment::RemoveWatch(CDebugSymbolsDataWatch *watch)
{
	guiMain->LockMutex();
	
	std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.find(watch->address);
	
	if (it != watches.end())
	{
		watches.erase(it);
	}

	guiMain->UnlockMutex();
}

void CDebugSymbolsSegment::RemoveWatch(int addr)
{
	guiMain->LockMutex();
	std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.find(addr);
	
	if (it != watches.end())
	{
		CDebugSymbolsDataWatch *watch = it->second;
		watches.erase(it);
	}

	guiMain->UnlockMutex();
}

void CDebugSymbolsSegment::DeleteWatch(int addr)
{
	guiMain->LockMutex();
	std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.find(addr);
	
	if (it != watches.end())
	{
		CDebugSymbolsDataWatch *watch = it->second;
		watches.erase(it);
		
		delete watch;
	}

	guiMain->UnlockMutex();
}

void CDebugSymbolsSegment::DeleteAllWatches()
{
	guiMain->LockMutex();

	while(!watches.empty())
	{
		std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.begin();
		CDebugSymbolsDataWatch *watch = it->second;
		watches.erase(it);
		
		delete watch;
	}
	
	guiMain->UnlockMutex();
}

void CDebugSymbolsSegment::AddWatch(int address, int numberOfValues, CSlrString *strRepresentation)
{
	int representation = WATCH_REPRESENTATION_HEX_8;
	
	if (strRepresentation != NULL)
	{
		if (strRepresentation->CompareWith("hex")
			|| strRepresentation->CompareWith("h")
			|| strRepresentation->CompareWith("hex8")
			|| strRepresentation->CompareWith("h8"))
		{
			representation = WATCH_REPRESENTATION_HEX_8;
		}
		else if (strRepresentation->CompareWith("hex16")
				 || strRepresentation->CompareWith("h16"))
		{
			representation = WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN;
		}
		else if (strRepresentation->CompareWith("hex32")
				 || strRepresentation->CompareWith("h32"))
		{
			representation = WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN;
		}
		
		if (strRepresentation->CompareWith("dec")
			|| strRepresentation->CompareWith("dec8")
			|| strRepresentation->CompareWith("unsigned")
			|| strRepresentation->CompareWith("unsigned8")
			|| strRepresentation->CompareWith("u")
			|| strRepresentation->CompareWith("u8"))
		{
			representation = WATCH_REPRESENTATION_UNSIGNED_DEC_8;
		}
		else if (strRepresentation->CompareWith("dec16")
				 || strRepresentation->CompareWith("unsigned16")
				 || strRepresentation->CompareWith("u16"))
		{
			representation = WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN;
		}
		else if (strRepresentation->CompareWith("dec32")
				 || strRepresentation->CompareWith("unsigned32")
				 || strRepresentation->CompareWith("u32"))
		{
			representation = WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN;
		}
		
		if (strRepresentation->CompareWith("signed")
			|| strRepresentation->CompareWith("signed8")
			|| strRepresentation->CompareWith("s8"))
		{
			representation = WATCH_REPRESENTATION_SIGNED_DEC_8;
		}
		else if (strRepresentation->CompareWith("signed16")
				 || strRepresentation->CompareWith("s16"))
		{
			representation = WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN;
		}
		else if (strRepresentation->CompareWith("signed32")
				 || strRepresentation->CompareWith("s32"))
		{
			representation = WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN;
		}
		
		if (strRepresentation->CompareWith("bin")
			|| strRepresentation->CompareWith("bin8")
			|| strRepresentation->CompareWith("b")
			|| strRepresentation->CompareWith("b8"))
		{
			representation = WATCH_REPRESENTATION_BIN;
		}
		else if (strRepresentation->CompareWith("bin16")
				 || strRepresentation->CompareWith("b16"))
		{
			representation = WATCH_REPRESENTATION_BIN;
		}
		else if (strRepresentation->CompareWith("bin32")
				 || strRepresentation->CompareWith("b32"))
		{
			representation = WATCH_REPRESENTATION_BIN;
		}
		
		else if (strRepresentation->CompareWith("text")
				 || strRepresentation->CompareWith("t"))
		{
			representation = WATCH_REPRESENTATION_TEXT;
		}
	}
	
	CDebugSymbolsCodeLabel *label = this->FindLabel(address);
	
	char *watchLabelText;
	if (label == NULL)
	{
		watchLabelText = new char[6];
		sprintf(watchLabelText, "%04x", address);
	}
	else
	{
		char *labelText = label->GetLabelText();
		watchLabelText = new char[strlen(labelText)];
		strcpy(watchLabelText, labelText);
	}

	this->AddWatch(address, watchLabelText, representation, numberOfValues);
}


// this activates the segment in debug interface.
void CDebugSymbolsSegment::Activate()
{
	LOGD("CDebugSymbolsSegment::Activate");
	this->name->DebugPrint("segment=");
}

bool CDebugSymbolsSegment::ParseSymbolsXML(CSlrString *command, std::vector<CSlrString *> *words)
{
	// segment specific commands from XML symbols
	return false;
}

bool CDebugSymbolsSegment::SerializeLabels(Hjson::Value hjsonCodeLabels)
{
	for (std::map<int, CDebugSymbolsCodeLabel *>::iterator it = codeLabelByAddress.begin(); it != codeLabelByAddress.end(); it++)
	{
		CDebugSymbolsCodeLabel *codeLabel = it->second;
		
		Hjson::Value hjsonCodeLabel;
		codeLabel->Serialize(hjsonCodeLabel);
		hjsonCodeLabels.push_back(hjsonCodeLabel);
	}
	return true;
}

bool CDebugSymbolsSegment::DeserializeLabels(Hjson::Value hjsonCodeLabels)
{
	for (int i = 0; i < hjsonCodeLabels.size(); i++)
	{
		Hjson::Value hjsonCodeLabel = hjsonCodeLabels[i];
		CDebugSymbolsCodeLabel *codeLabel = new CDebugSymbolsCodeLabel(this);
		codeLabel->Deserialize(hjsonCodeLabel);
		
		char *labelText = codeLabel->GetLabelText();
		if (labelText[0] == 0)
		{
			delete codeLabel;
			continue;
		}
		
		// add code label, but do not sort table, it will be sorted at the end of deserialize
		AddCodeLabel(codeLabel, false);
	}
	
	UpdateCodeLabelsArray();
	return false;
}

bool CDebugSymbolsSegment::SerializeWatches(Hjson::Value hjsonWatches)
{
	for (std::map<int, CDebugSymbolsDataWatch *>::iterator it = watches.begin(); it != watches.end(); it++)
	{
		CDebugSymbolsDataWatch *watch = it->second;
		
		Hjson::Value hjsonWatch;
		watch->Serialize(hjsonWatch);
		hjsonWatches.push_back(hjsonWatch);
	}
	return true;
}

bool CDebugSymbolsSegment::DeserializeWatches(Hjson::Value hjsonWatches)
{
	for (int i = 0; i < hjsonWatches.size(); i++)
	{
		Hjson::Value hjsonWatch = hjsonWatches[i];
		CDebugSymbolsDataWatch *watch = new CDebugSymbolsDataWatch(this);
		watch->Deserialize(hjsonWatch);
		
		// add code
		AddWatch(watch);
	}
	
	UpdateCodeLabelsArray();
	return false;
}

bool CDebugSymbolsSegment::SerializeBreakpoints(Hjson::Value hjsonBreakpointsTypes)
{
	LOGD("CDebugSymbolsSegment::SerializeBreakpoints");
	for (std::map<int, CDebugBreakpointsAddr *>::iterator it = breakpointsByType.begin(); it != breakpointsByType.end(); it++)
	{
		CDebugBreakpointsAddr *breakpoints = it->second;
		LOGD("...breakpointsTypeStr=%s", breakpoints->breakpointsTypeStr);

		Hjson::Value hjsonBreakpointsType;
		hjsonBreakpointsType["Type"] = breakpoints->breakpointsTypeStr;
		
		Hjson::Value hjsonBreakpoints;
		breakpoints->Serialize(hjsonBreakpoints);
		hjsonBreakpointsType["Items"] = hjsonBreakpoints;
		
		hjsonBreakpointsTypes.push_back(hjsonBreakpointsType);
	}
	return true;
}

bool CDebugSymbolsSegment::DeserializeBreakpoints(Hjson::Value hjsonBreakpointsTypes)
{
	LOGD("CDebugSymbolsSegment::DeserializeBreakpoints");
	for (int i = 0; i < hjsonBreakpointsTypes.size(); i++)
	{
		Hjson::Value hjsonBreakpointsType = hjsonBreakpointsTypes[i];
		const char *breakpointsTypeStr = hjsonBreakpointsType["Type"];
		LOGD("...breakpointsTypeStr=%s", breakpointsTypeStr);
		
		CDebugBreakpointsAddr *foundBreakpointsType = NULL;
		
		// TODO: local breakpoints types as hash of name string, remove breakpointsType
		for (std::map<int, CDebugBreakpointsAddr *>::iterator it = breakpointsByType.begin(); it != breakpointsByType.end(); it++)
		{
			CDebugBreakpointsAddr *breakpoints = it->second;
			if (!strcmp(breakpoints->breakpointsTypeStr, breakpointsTypeStr))
			{
				foundBreakpointsType = breakpoints;
				break;
			}
		}
		
		if (foundBreakpointsType)
		{
			Hjson::Value hjsonBreakpoints = hjsonBreakpointsType["Items"];
			foundBreakpointsType->Deserialize(hjsonBreakpoints);
		}
	}
	
	return true;
}

