#include "CByteBuffer.h"
#include "CSlrString.h"
#include "CSlrFileFromOS.h"
#include "SYS_Threading.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Main.h"
#include "CViewC64.h"
#include "CViewDisassembly.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbolsSegment.h"
#include "CViewDataWatch.h"
#include "CGuiMain.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsSegmentC64.h"
#include "CSlrDate.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "utf8.h"
#include "std_membuf.h"

CDebugSymbols::CDebugSymbols(CDebugInterface *debugInterface, CDataAdapter *dataAdapter)
{
	this->debugInterface = debugInterface;
	this->dataAdapter = dataAdapter;
	this->asmSource = NULL;
	this->currentSegment = NULL;
	this->currentSegmentNum = -1;
}

CDebugSymbols::~CDebugSymbols()
{
	this->DeactivateSegment();

	while (!segments.empty())
	{
		CDebugSymbolsSegment *segment = segments.back();
		segments.pop_back();
		
		// TODO: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA THE BUG!
		//       observation: on macOS when here segment is deleted then POKEY wavetables stop receiving channels data
		//delete segment;
	}
}

void CDebugSymbols::ParseSymbols(CSlrString *fileName)
{
	char *fname = fileName->GetStdASCII();
	
	LOGD("C64Symbols::ParseSymbols: %s", fname);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(fname);
	
	if (file->Exists() == false)
	{
		LOGError("C64Symbols::ParseSymbols: file %s does not exist", fname);
		delete [] fname;
		return;
	}
	delete [] fname;

	ParseSymbols(file);

	delete file;
}

void CDebugSymbols::ParseSymbols(CSlrFile *file)
{
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	ParseSymbols(byteBuffer);
	
	delete byteBuffer;
}

// this should be used by ParseSymbols with
void CDebugSymbols::DeleteAllSymbols()
{
	while (!segments.empty())
	{
		CDebugSymbolsSegment *segment = segments.back();
		segments.pop_back();
		
		// TODO: On windows delete segment crashes on -pass   needs to be investigated, what about Linux and valgrind? Xcode sanitizer shows nothing
		// TODO: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA THE BUG!
		//delete segment;
	}
	
	currentSegment = NULL;
}

// parse Vice's vs format
void CDebugSymbols::ParseSymbols(CByteBuffer *byteBuffer)
{
	LOGM("C64Symbols::ParseSymbols");
	
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	DeleteAllSymbols();
	
	// the Vice symbols format does not support segments, so create default
	CreateDefaultSegment();
	
	if (byteBuffer->length < 8)
	{
		LOGError("Empty symbols file");
		CreateDefaultSegment();
		
		debugInterface->UnlockMutex();
		guiMain->UnlockMutex();
		return;
	}
	
	byteBuffer->removeCRLFinQuotations();
	
	std_membuf mb(byteBuffer->data, byteBuffer->length);
	std::istream reader(&mb);
	
	unsigned lineNum = 1;
	std::string line;
	line.clear();
	
	std::list<u16> splitChars;
	splitChars.push_back(' ');
	splitChars.push_back('\t');
	splitChars.push_back('=');
	
	// Play with all the lines in the file
	while (getline(reader, line))
	{
		//LOGD(".. line=%d", lineNum);
		// check for invalid utf-8 (for a simple yes/no check, there is also utf8::is_valid function)
		std::string::iterator end_it = utf8::find_invalid(line.begin(), line.end());
		if (end_it != line.end())
		{
			LOGError("Invalid UTF-8 encoding detected at line %d", lineNum);
		}
		
		// Get the line length (at least for the valid part)
		//int length = utf8::distance(line.begin(), end_it);
//		LOGD("========================= Length of line %d is %d", lineNum, length);
		
		// Convert it to utf-16
		std::vector<unsigned short> utf16line;
		utf8::utf8to16(line.begin(), end_it, back_inserter(utf16line));
		
		LOGD("LINE #%d", lineNum);
		CSlrString *str = new CSlrString(utf16line);
		str->DebugPrint("str=");
		
		std::vector<CSlrString *> *words = str->SplitWithChars(splitChars);
		
		if (words->size() == 0)
		{
			lineNum++;
			continue;
		}
		
		//LOGD("words->size=%d", words->size());
		
		CSlrString *command = (*words)[0];
		
		// comment?
		if (command->GetChar(0) == '#')
		{
			lineNum++;
			continue;
		}
		
		if (words->size() >= 5 && (command->Equals("al") || command->Equals("AL")))
		{
//			if (words->size() < 5)
//			{
//				LOGError("ParseSymbols: error in line %d", lineNum);
//				break;
//			}
			
			CSlrString *deviceAndAddr = (*words)[2];
			CSlrString *labelName = (*words)[4];
			
//			deviceAndAddr->DebugPrint("deviceAndAddr=");
			int deviceId = 0;
			
			char deviceIdChar = deviceAndAddr->GetChar(0);
			
			if (deviceIdChar == 'C' || deviceIdChar == 'c')
			{
				deviceId = C64_SYMBOL_DEVICE_COMMODORE;
			}
			else if (deviceIdChar == 'D' || deviceIdChar == 'd')
			{
				deviceId = C64_SYMBOL_DEVICE_DRIVE1541;
			}
			else
			{
				LOGError("ParseSymbols: unknown device in line %d", lineNum);
				break;
			}
			
//			addrStr[0] = deviceAndAddr->GetChar(2);
//			addrStr[1] = deviceAndAddr->GetChar(3);
//			addrStr[2] = deviceAndAddr->GetChar(4);
//			addrStr[3] = deviceAndAddr->GetChar(5);
//			addrStr[4] = 0x00;
			char addrStr[5] = { 0, 0, 0, 0, 0 };
			int ps;
			for (ps = 0 ; ps < 4; ps++)
			{
				if (ps + 2 < deviceAndAddr->GetLength())
				{
					addrStr[0 + ps] = deviceAndAddr->GetChar(2 + ps);
				}
			}
			
			int addr;
			sscanf(addrStr, "%x", &addr);

			LOGD("addr=%06x", addr);
			
			// remove leading dot
			if (labelName->GetChar(0) == '.')
			{				
				labelName->RemoveCharAt(0);
			}
			
//			labelName->DebugPrint("labelName=");

			char *labelNameStr = labelName->GetStdASCII();
			
			LOGD("labelNameStr='%s'  addr=%04x", labelNameStr, addr);

			if (deviceId == C64_SYMBOL_DEVICE_COMMODORE)
			{
				if (labelNameStr[0] != 0)
				{
					currentSegment->AddCodeLabel(addr, labelNameStr);
				}
			}
			else if (deviceId == C64_SYMBOL_DEVICE_DRIVE1541)
			{
				LOGError("CDebugSymbols::ParseSymbols: TODO: C64_SYMBOL_DEVICE_DRIVE1541");
			}
		}
		else if (words->size() > 3)
		{
			// assume tass64 label
			// example: DRIVERDONE      = $0c48
			
			//			LOGD("words->size=%d", words->size());

			int index = 0;
			CSlrString *labelName = (*words)[index++];
			CSlrString *equals = (*words)[words->size()-3];
//			equals->DebugPrint("equals=");
			if (equals->GetChar(0)== '=')
			{
				CSlrString *labelName = (*words)[0];
				char *labelNameStr = labelName->GetStdASCII();

//				LOGD("labelNameStr='%s'", labelNameStr);
				
				CSlrString *addrSlrStr = (*words)[words->size()-1];
				char *addrStr = addrSlrStr->GetStdASCII();

//				addrSlrStr->DebugPrint("addrStr=");
				
				int addr;
				if (addrStr[0] == '$')
				{
					sscanf((addrStr+1), "%x", &addr);
				}
				else
				{
					sscanf(addrStr, "%d", &addr);
				}
				
				LOGD("labelNameStr=%s  addr=%04x", labelNameStr, addr);
				
				if (labelNameStr[0] != 0)
				{
					currentSegment->AddCodeLabel(addr, labelNameStr);
				}

				delete []addrStr;
//				delete []labelNameStr;	TODO: AddCodeLabel does not allocate own string
			}
			else
			{
				LOGError("ParseSymbols: error in line %d (unknown label type)", lineNum);
				break;
			}
		}
		else
		{
			LOGError("ParseSymbols: error in line %d (unknown label type)", lineNum);
			break;
		}
		
		delete str;
		for (int i = 0; i < words->size(); i++)
		{
			delete (*words)[i];
		}
		delete  words;
		
		lineNum++;
	}

	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
	
	LOGD("C64Symbols::ParseSymbols: done");
}


void CDebugSymbols::DeleteAllBreakpoints()
{
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	for (std::vector<CDebugSymbolsSegment *>::iterator it = segments.begin(); it != segments.end(); it++)
	{
		CDebugSymbolsSegment *segment = *it;
		segment->ClearBreakpoints();
	}
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

void CDebugSymbols::ParseBreakpoints(CSlrString *fileName)
{
	char *fname = fileName->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(fname);
	
	if (file->Exists() == false)
	{
		LOGError("C64Symbols::ParseBreakpoints: file %s does not exist", fname);
		delete [] fname;
		return;
	}
	delete [] fname;
	
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	ParseBreakpoints(byteBuffer);
	
	delete byteBuffer;
	delete file;
}

void CDebugSymbols::ParseBreakpoints(CByteBuffer *byteBuffer)
{
	LOGM("C64Symbols::ParseBreakpoints");
	
	debugInterface->LockMutex();
	
	if (byteBuffer->length < 8)
	{
		LOGError("Empty breakpoints file");
		debugInterface->UnlockMutex();
		return;
	}

	byteBuffer->removeCRLFinQuotations();
	
	std_membuf mb(byteBuffer->data, byteBuffer->length);
	std::istream reader(&mb);
	
	unsigned lineNum = 1;
	std::string line;
	line.clear();
	
	std::list<u16> splitChars;
	splitChars.push_back(' ');
	splitChars.push_back('<');
	splitChars.push_back('>');
	splitChars.push_back('=');
	splitChars.push_back('!');
	splitChars.push_back('#');
	
	// Play with all the lines in the file
	while (getline(reader, line))
	{
		//LOGD(".. line=%d", lineNum);
		// check for invalid utf-8 (for a simple yes/no check, there is also utf8::is_valid function)
		std::string::iterator end_it = utf8::find_invalid(line.begin(), line.end());
		if (end_it != line.end())
		{
			LOGError("Invalid UTF-8 encoding detected at line %d", lineNum);
		}
		
		// Get the line length (at least for the valid part)
		//int length = utf8::distance(line.begin(), end_it);
		//LOGD("========================= Length of line %d is %d", lineNum, length);
		
		// Convert it to utf-16
		std::vector<unsigned short> utf16line;
		utf8::utf8to16(line.begin(), end_it, back_inserter(utf16line));
		
		CSlrString *str = new CSlrString(utf16line);
		//str->DebugPrint("str=");
		
		std::vector<CSlrString *> *words = str->SplitWithChars(splitChars);
		
		if (words->size() == 0)
		{
			lineNum++;
			continue;
		}
		
		//LOGD("words->size=%d", words->size());
		
		CSlrString *command = (*words)[0];
		
		// comment?
		if (command->GetChar(0) == '#')
		{
			lineNum++;
			continue;
		}
		
		command->ConvertToLowerCase();
		
		if (command->Equals("break") || command->Equals("breakpc") || command->Equals("breakonpc"))
		{
			if (words->size() < 3)
			{
				LOGError("ParseBreakpoints: error in line %d", lineNum);
				break;
			}
			
			// pc breakpoint
			CSlrString *arg = (*words)[2];
			//arg->DebugPrint(" arg=");
			int address = arg->ToIntFromHex();
			
			LOGD(".. adding breakOnPC %4.4x", address);
			
			std::map<int, CBreakpointAddr *>::iterator it = currentSegment->breakpointsPC->breakpoints.find(address);
			if (it == currentSegment->breakpointsPC->breakpoints.end())
			{
				// not found
				CBreakpointAddr *addrBreakpoint = new CBreakpointAddr(address);
				addrBreakpoint->actions = ADDR_BREAKPOINT_ACTION_STOP;
				currentSegment->breakpointsPC->breakpoints[address] = addrBreakpoint;
			}
			else
			{
				LOGD("...... exists %4.4x", address);
				CBreakpointAddr *addrBreakpoint = it->second;
				SET_BIT(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP);
			}
		}
		else if (command->Equals("setbkg") || command->Equals("setbackground"))
		{
			if (words->size() < 5)
			{
				LOGError("ParseBreakpoints: error in line %d", lineNum);
				break;
			}
			
			// pc breakpoint
			CSlrString *arg = (*words)[2];
			//arg->DebugPrint(" arg=");
			int address = arg->ToIntFromHex();
			
			arg = (*words)[4];
			int value = arg->ToIntFromHex();
			
			LOGD(".. adding setBkg %4.4x %2.2x", address, value);
			
			std::map<int, CBreakpointAddr *>::iterator it = currentSegment->breakpointsPC->breakpoints.find(address);
			if (it == currentSegment->breakpointsPC->breakpoints.end())
			{
				// not found
				CBreakpointAddr *addrBreakpoint = new CBreakpointAddr(address);
				addrBreakpoint->actions = ADDR_BREAKPOINT_ACTION_SET_BACKGROUND;
				addrBreakpoint->data = value;
				currentSegment->breakpointsPC->breakpoints[address] = addrBreakpoint;
			}
			else
			{
				LOGD("...... exists %4.4x", address);
				CBreakpointAddr *addrBreakpoint = it->second;
				addrBreakpoint->data = value;
				SET_BIT(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_SET_BACKGROUND);
			}
		}
		else if (currentSegment->ParseSymbolsXML(command, words))
		{
			//
		}
		else if (command->Equals("breakmemory") || command->Equals("breakonmemory") || command->Equals("breakmem"))
		{
			if (words->size() < 4)
			{
				LOGError("ParseBreakpoints: error in line %d", lineNum);
				break;
			}
			
			CSlrString *addressStr = (*words)[2];
			//addressStr->DebugPrint(" addressStr=");
			int address = addressStr->ToIntFromHex();

			int index = 3;
			CSlrString *op = new CSlrString();
			
			while (index < words->size()-1)
			{
				CSlrString *f = (*words)[index];
				f->ConvertToLowerCase();
				
			//	f->DebugPrint(".... f= ");

				u16 chr = f->GetChar(0);
				if (chr == ' ')
				{
					index++;
					continue;
				}
				
				if ( (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') )
				{
					break;
				}
				
				op->Concatenate(f);
				
				index++;
			}
			
			if (index >= words->size())
			{
				LOGError("ParseBreakpoints: error in line %d", lineNum);
				break;
			}
			
			CSlrString *arg = (*words)[index];
			//arg->DebugPrint(" arg=");
			
			int value = arg->ToIntFromHex();

			MemoryBreakpointComparison memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
			
			if (op->Equals("==") || op->Equals("="))
			{
				memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
			}
			else if (op->Equals("!="))
			{
				memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL;
			}
			else if (op->Equals("<"))
			{
				memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS;
			}
			else if (op->Equals("<=") || op->Equals("=<"))
			{
				memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL;
			}
			else if (op->Equals(">"))
			{
				memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_GREATER;
			}
			else if (op->Equals(">=") || op->Equals("=>"))
			{
				memBreakComparison = MemoryBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL;
			}
			else
			{
				LOGError("ParseBreakpoints: error in line %d (unknown operator for memory breakpoint)", lineNum);
				break;
			}

			LOGD(".. adding breakOnMemory");
			LOGD("..... addr=%4.4x", address);
			op->DebugPrint("..... op=");
			LOGD("..... value=%2.2x", value);
			
			CBreakpointMemory *memBreakpoint = new CBreakpointMemory(address, MEMORY_BREAKPOINT_ACCESS_WRITE, memBreakComparison, value);
			currentSegment->breakpointsMemory->breakpoints[address] = memBreakpoint;
		}
		else
		{
			LOGError("ParseBreakpoints: error in line %d (unknown breakpoint type)", lineNum);
			break;
		}
		
		delete str;
		for (int i = 0; i < words->size(); i++)
		{
			delete (*words)[i];
		}
		delete  words;
		
		lineNum++;
	}
	
	debugInterface->UnlockMutex();
	
	debugInterface->UpdateRenderBreakpoints();
	
	LOGD("C64Symbols::ParseBreakpoints: done");
}

///
void CDebugSymbols::DeleteAllWatches()
{
	LOGD("C64Symbols::DeleteAllWatches");
	
	// bug: this freezes gui at startup, and is not needed
//	debugInterface->LockMutex();
	
	for (std::vector<CDebugSymbolsSegment *>::iterator it = segments.begin(); it != segments.end(); it++)
	{
		CDebugSymbolsSegment *segment = *it;
		segment->DeleteAllWatches();
	}

	// TODO: this below breaks the idea, we *must* have a specific debug interface for the C64 drives
//	if (viewC64->debugInterfaceC64)
//	{
//		viewC64->viewDrive1541MemoryDataWatch->DeleteAllWatches();
//	}
	
//	debugInterface->UnlockMutex();
}

void CDebugSymbols::ParseWatches(CSlrString *fileName)
{
	char *fname = fileName->GetStdASCII();
	
	LOGD("C64Symbols::ParseWatches: %s", fname);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(fname);
	
	if (file->Exists() == false)
	{
		LOGError("C64Symbols::ParseWatches: file %s does not exist", fname);
		delete [] fname;
		return;
	}
	delete [] fname;
	
	ParseWatches(file);
	
	delete file;
}

void CDebugSymbols::ParseWatches(CSlrFile *file)
{
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	ParseWatches(byteBuffer);
	
	delete byteBuffer;
}


void CDebugSymbols::ParseWatches(CByteBuffer *byteBuffer)
{
	LOGM("C64Symbols::ParseWatches");
	
	// bug: this freezes gui at startup, and is not needed
//	debugInterface->LockMutex();
	
	byteBuffer->removeCRLFinQuotations();
	
	std_membuf mb(byteBuffer->data, byteBuffer->length);
	std::istream reader(&mb);
	
	unsigned lineNum = 1;
	std::string line;
	line.clear();
	
	std::list<u16> splitChars;
	splitChars.push_back(' ');
	splitChars.push_back('\t');
	splitChars.push_back('=');

	std::list<u16> splitCharsComma;
	splitCharsComma.push_back(',');

	// Play with all the lines in the file
	while (getline(reader, line))
	{
		//LOGD(".. line=%d", lineNum);
		// check for invalid utf-8 (for a simple yes/no check, there is also utf8::is_valid function)
		std::string::iterator end_it = utf8::find_invalid(line.begin(), line.end());
		if (end_it != line.end())
		{
			LOGError("Invalid UTF-8 encoding detected at line %d", lineNum);
		}
		
		// Get the line length (at least for the valid part)
		//int length = utf8::distance(line.begin(), end_it);
		//LOGD("========================= Length of line %d is %d", lineNum, length);
		
		// Convert it to utf-16
		std::vector<unsigned short> utf16line;
		utf8::utf8to16(line.begin(), end_it, back_inserter(utf16line));
		
		CSlrString *str = new CSlrString(utf16line);
		str->DebugPrint("str=");
		
		std::vector<CSlrString *> *words = str->SplitWithChars(splitChars);
		
		LOGD("words->size=%d", words->size());
		for (int i = 0; i < words->size(); i++)
		{
			LOGD("...words[%d]", i);
			(*words)[i]->DebugPrint("...=");
		}
		
		LOGD("------");

		if (words->size() < 3)
		{
			lineNum++;
			continue;
		}
		
		CSlrString *dataSlrStr = (*words)[0];
		
		// comment?
		if (dataSlrStr->GetChar(0) == '#')
		{
			lineNum++;
			continue;
		}
		
		
		if (words->size() == 3)
		{
			// addr 0
			// watch name 2
			
			CSlrString *watchName = (*words)[2];
			watchName->DebugPrint("watchName=");

			char *watchNameStr = watchName->GetStdASCII();
			
			std::vector<CSlrString *> *dataWords = dataSlrStr->SplitWithChars(splitCharsComma);
			
//			for (int i = 0; i < dataWords->size(); i++)
//			{
//				LOGD("...dataWords[%d]", i);
//				(*dataWords)[i]->DebugPrint("...=");
//			}
			
			CSlrString *addrSlrStr = dataSlrStr; //(*dataWords)[0];
			int addr = addrSlrStr->ToIntFromHex();
			
			LOGD("addr=%x", addr);
			
			// Not finished / TODO:
//			int representation = WATCH_REPRESENTATION_HEX;
//			int numberOfValues = 1;
//			int bits = WATCH_BITS_8;
//			
//			if (dataWords->size() > 2)
//			{
//				CSlrString *repStr = (*dataWords)[2];
//				repStr->DebugPrint("repStr=");
//				
//				if (repStr->CompareWith("hex") || repStr->CompareWith("HEX"))
//				{
//					representation = WATCH_REPRESENTATION_HEX;
//				}
//				else if (repStr->CompareWith("bin") || repStr->CompareWith("BIN"))
//				{
//					representation = WATCH_REPRESENTATION_BIN;
//				}
//				else if (repStr->CompareWith("dec") || repStr->CompareWith("DEC"))
//				{
//					representation = WATCH_REPRESENTATION_UNSIGNED_DEC;
//				}
//				else if (repStr->CompareWith("sdec") || repStr->CompareWith("SDEC") || repStr->CompareWith("signed"))
//				{
//					representation = WATCH_REPRESENTATION_SIGNED_DEC;
//				}
//				else if (repStr->CompareWith("text") || repStr->CompareWith("TEXT"))
//				{
//					representation = WATCH_REPRESENTATION_TEXT;
//				}
//			}
//			
//			if (dataWords->size() > 4)
//			{
//				CSlrString *nrValsStr = (*dataWords)[4];
//				numberOfValues = nrValsStr->ToInt();
//			}
//			
//			if (dataWords->size() > 6)
//			{
//				CSlrString *bitsStr = (*dataWords)[6];
//				if (bitsStr->CompareWith("8"))
//				{
//					bits = WATCH_BITS_8;
//				}
//				else if (bitsStr->CompareWith("16"))
//				{
//					bits = WATCH_BITS_16;
//				}
//				else if (bitsStr->CompareWith("32"))
//				{
//					bits = WATCH_BITS_32;
//				}
//			}
			
			if (debugInterface->symbols && debugInterface->symbols->currentSegment)
			{
				debugInterface->symbols->currentSegment->AddWatch(addr, watchNameStr);
			}
			else
			{
				LOGError("CDebugSymbols::ParseWatches: no default symbols segment");
			}
			
			delete [] watchNameStr;
			delete addrSlrStr; //[] addrStr;

		}
		else if (words->size() > 3)
		{
			// assume tass64 label
			// example: DRIVERDONE      = $0c48
			
						LOGD("words->size=%d", words->size());
			
			int index = 0;
			CSlrString *labelName = (*words)[index++];
			CSlrString *equals = (*words)[words->size()-3];
						equals->DebugPrint("equals=");
			if (equals->GetChar(0)== '=')
			{
				CSlrString *labelName = (*words)[0];
				char *labelNameStr = labelName->GetStdASCII();
				
								LOGD("labelNameStr='%s'", labelNameStr);
				
				CSlrString *addrSlrStr = (*words)[words->size()-1];
				char *addrStr = addrSlrStr->GetStdASCII();
				
								addrSlrStr->DebugPrint("addrStr=");
				
				int addr;
				if (addrStr[0] == '$')
				{
					sscanf((addrStr+1), "%x", &addr);
				}
				else
				{
					sscanf(addrStr, "%d", &addr);
				}
				
				LOGD("watchNameStr=%s  addr=%04x", labelNameStr, addr);
				
				//				LOGTODO("!!!!!!!!!!!!!!!!!! REMOVE ME / TESTING VIEW !!!!!!!!!!!!");
				//				if (addr >= 0x0900 && addr < 0x0B60)
				//				{
				//					viewC64->viewC64MemoryDataWatch->AddWatch(labelNameStr, addr);
				//				}

				if (debugInterface->symbols && debugInterface->symbols->currentSegment)
				{
					debugInterface->symbols->currentSegment->AddWatch(addr, labelNameStr);
				}
				else
				{
					LOGError("CDebugSymbols::ParseWatches: no default symbols segment");
				}
				
				delete []addrStr;
				delete []labelNameStr;
			}
			else
			{
				LOGError("ParseSymbols: error in line %d (unknown label type)", lineNum);
				break;
			}
		}
		else
		{
			LOGError("ParseWatches: error in line %d (unknown watch format)", lineNum);
			break;
		}
		
		delete str;
		for (int i = 0; i < words->size(); i++)
		{
			delete (*words)[i];
		}
		delete  words;
		
		lineNum++;
	}
	
	//LOGTODO("update watches max length for display");
	
//	debugInterface->UnlockMutex();
	
	LOGD("C64Symbols::Watches: done");
}

///

// parse C64 Debugger symbols (i.e. KickAss dbg format)
void CDebugSymbols::ParseSourceDebugInfo(CSlrString *fileName)
{
	LOGM("C64Symbols::ParseSourceDebugInfo");
	fileName->DebugPrint("C64Symbols::ParseSourceDebugInfo fileName=");
	
	char *fname = fileName->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(fname);
	
	if (file->Exists() == false)
	{
		LOGError("C64Symbols::ParseSourceDebugInfo: file %s does not exist", fname);
		delete [] fname;
		return;
	}
	delete [] fname;
	
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	ParseSourceDebugInfo(byteBuffer);
	
	delete byteBuffer;
	delete file;
}

void CDebugSymbols::ParseSourceDebugInfo(CSlrFile *file)
{
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	
	ParseSourceDebugInfo(byteBuffer);
	
	delete byteBuffer;
}

void CDebugSymbols::DeleteSourceDebugInfo()
{
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	if (this->asmSource)
	{
		delete this->asmSource;
	}
	this->asmSource = NULL;
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

void CDebugSymbols::ParseSourceDebugInfo(CByteBuffer *byteBuffer)
{
	LOGM("C64Symbols::ParseSourceDebugInfo: byteBuffer=%x", byteBuffer);
	
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	LOGTODO("move this->asmSource to debugInterfce->asmSource");
	if (this->asmSource != NULL)
	{
		delete this->asmSource;
		asmSource = NULL;
	}
	
	LOGD("create asmSource C64AsmSourceSymbols");
	this->asmSource = new CDebugAsmSource(byteBuffer, debugInterface, dataAdapter);
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();

	LOGD("C64Symbols::ParseSymbols: done");
}

//
void CDebugSymbols::LoadLabelsRetroDebuggerFormat(CSlrString *filePath)
{
	LOGD("CDebugSymbols::LoadLabelsRetroDebuggerFormat");
	filePath->DebugPrint("filePath=");
	
	CByteBuffer *byteBuffer = new CByteBuffer(filePath);
	if (!byteBuffer->IsEmpty())
	{
		char *jsonText = new char[byteBuffer->length+2];
		memcpy(jsonText, byteBuffer->data, byteBuffer->length);
		jsonText[byteBuffer->length] = 0;
		
		Hjson::Value hjsonRoot;
		std::stringstream ss;
		ss.str(jsonText);
		
		try
		{
			ss >> hjsonRoot;
			DeserializeLabelsFromHjson(hjsonRoot);
		}
		catch (const std::exception& e)
		{
			LOGError("CDebugSymbols::DeserializeSegmentsAndLabelsFromHjson error: %s", e.what());
			
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "Loading labels failed. Error:\n%s", e.what());
			guiMain->ShowMessageBox("Error", buf);
			SYS_ReleaseCharBuf(buf);
		}
	}
	
	delete byteBuffer;
	
	if (segments.empty())
	{
		CreateDefaultSegment();
	}
	
	if (currentSegment == NULL)
	{
		currentSegment = segments.front();
	}
}

void CDebugSymbols::SaveLabelsRetroDebuggerFormat(CSlrString *filePath)
{
	LOGD("CDebugSymbols::SaveLabelsRetroDebuggerFormat");
	filePath->DebugPrint("filePath=");
	
	const char *cstrPath = filePath->GetStdASCII();
	
	FILE *fp = fopen(cstrPath, "wb");
	if (!fp)
	{
		LOGError("CDebugSymbols::SaveLabelsRetroDebuggerFormat: can't write to %s", cstrPath);
		
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "Can't write to file\n%s", cstrPath);
		guiMain->ShowMessageBox("Error", buf);
		SYS_ReleaseCharBuf(buf);
		delete [] cstrPath;
		return;
	}
	delete [] cstrPath;

	Hjson::Value hjsonRoot;

	CSlrDate *date = new CSlrDate();
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", date->year, date->month, date->day, date->hour, date->minute, date->second);

	std::string comment = "// Labels exported by RetroDebugger v";
	comment += RETRODEBUGGER_VERSION_STRING;
	comment += " on ";
	comment += buf;
	comment += "\n\r";
	hjsonRoot.set_comment_before(comment);
	
	SYS_ReleaseCharBuf(buf);

	//
	hjsonRoot["Version"] = "1";

	SerializeLabelsToHjson(hjsonRoot);

	std::stringstream ss;
	ss << Hjson::Marshal(hjsonRoot);
	
	std::string s = ss.str();
	const char *cstrHjson = s.c_str();

	fprintf(fp, "%s", cstrHjson);
	
	fclose(fp);
}

// TODO: add bool export segments, labels, code mapping, breakpoints
bool CDebugSymbols::SerializeLabelsToHjson(Hjson::Value hjsonRoot)
{
	Hjson::Value hjsonSegmentsArray;
	for (auto segment : segments)
	{
		Hjson::Value hjsonSegment;
		char *cName = segment->name->GetStdASCII();
		hjsonSegment["Name"] = cName;
		STRFREE(cName);

		Hjson::Value hjsonCodeLabels;
		segment->SerializeLabels(hjsonCodeLabels);
		hjsonSegment["CodeLabels"] = hjsonCodeLabels;

		hjsonSegmentsArray.push_back(hjsonSegment);
	}
	hjsonRoot["Segments"] = hjsonSegmentsArray;
	return true;
}

bool CDebugSymbols::DeserializeLabelsFromHjson(Hjson::Value hjsonRoot)
{
	Hjson::Value hjsonSegmentsArray = hjsonRoot["Segments"];
	for (int i = 0; i < hjsonSegmentsArray.size(); i++)
	{
		Hjson::Value hjsonSegment = hjsonSegmentsArray[i];
		const char *cName = hjsonSegment["Name"];
		CSlrString *name = new CSlrString(cName);
		
		CDebugSymbolsSegment *segment = FindSegment(name);
		if (segment == NULL)
		{
			segment = CreateNewDebugSymbolsSegment(new CSlrString(name));
			segments.push_back(segment);
		}
		delete name;
		
		Hjson::Value hjsonCodeLabels = hjsonSegment["CodeLabels"];
		segment->DeserializeLabels(hjsonCodeLabels);
	}
	return true;
}

//
// TODO: generalize me as this is mainly copy pasted code from above!
void CDebugSymbols::LoadWatchesRetroDebuggerFormat(CSlrString *filePath)
{
	LOGD("CDebugSymbols::LoadWatchsRetroDebuggerFormat");
	filePath->DebugPrint("filePath=");
	
	CByteBuffer *byteBuffer = new CByteBuffer(filePath);
	if (!byteBuffer->IsEmpty())
	{
		char *jsonText = new char[byteBuffer->length+2];
		memcpy(jsonText, byteBuffer->data, byteBuffer->length);
		jsonText[byteBuffer->length] = 0;
		
		Hjson::Value hjsonRoot;
		std::stringstream ss;
		ss.str(jsonText);
		
		try
		{
			ss >> hjsonRoot;
			DeserializeWatchesFromHjson(hjsonRoot);
		}
		catch (const std::exception& e)
		{
			LOGError("CDebugSymbols::DeserializeWatchesFromHjson error: %s", e.what());
			
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "Loading watches failed. Error:\n%s", e.what());
			guiMain->ShowMessageBox("Error", buf);
			SYS_ReleaseCharBuf(buf);
		}
	}
	
	delete byteBuffer;
	
	if (segments.empty())
	{
		CreateDefaultSegment();
	}

	if (currentSegment == NULL)
	{
		currentSegment = segments.front();
	}
}

void CDebugSymbols::SaveWatchesRetroDebuggerFormat(CSlrString *filePath)
{
	LOGD("CDebugSymbols::SaveWatchesRetroDebuggerFormat");
	filePath->DebugPrint("filePath=");
	
	const char *cstrPath = filePath->GetStdASCII();
	
	FILE *fp = fopen(cstrPath, "wb");
	if (!fp)
	{
		LOGError("CDebugSymbols::SaveWatchesRetroDebuggerFormat: can't write to %s", cstrPath);
		
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "Can't write to file\n%s", cstrPath);
		guiMain->ShowMessageBox("Error", buf);
		SYS_ReleaseCharBuf(buf);
		delete [] cstrPath;
		return;
	}
	delete [] cstrPath;

	Hjson::Value hjsonRoot;

	CSlrDate *date = new CSlrDate();
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", date->year, date->month, date->day, date->hour, date->minute, date->second);

	std::string comment = "// Watches exported by RetroDebugger v";
	comment += RETRODEBUGGER_VERSION_STRING;
	comment += " on ";
	comment += buf;
	comment += "\n\r";
	hjsonRoot.set_comment_before(comment);
	
	SYS_ReleaseCharBuf(buf);

	//
	hjsonRoot["Version"] = "1";

	SerializeWatchesToHjson(hjsonRoot);

	std::stringstream ss;
	ss << Hjson::Marshal(hjsonRoot);
	
	std::string s = ss.str();
	const char *cstrHjson = s.c_str();

	fprintf(fp, "%s", cstrHjson);
	
	fclose(fp);
}

bool CDebugSymbols::SerializeWatchesToHjson(Hjson::Value hjsonRoot)
{
	Hjson::Value hjsonSegmentsArray;
	for (auto segment : segments)
	{
		Hjson::Value hjsonSegment;
		char *cName = segment->name->GetStdASCII();
		hjsonSegment["Name"] = cName;
		STRFREE(cName);

		Hjson::Value hjsonWatches;
		segment->SerializeWatches(hjsonWatches);
		hjsonSegment["Watches"] = hjsonWatches;

		hjsonSegmentsArray.push_back(hjsonSegment);
	}
	hjsonRoot["Segments"] = hjsonSegmentsArray;
	return true;
}

bool CDebugSymbols::DeserializeWatchesFromHjson(Hjson::Value hjsonRoot)
{
	Hjson::Value hjsonSegmentsArray = hjsonRoot["Segments"];
	for (int i = 0; i < hjsonSegmentsArray.size(); i++)
	{
		Hjson::Value hjsonSegment = hjsonSegmentsArray[i];
		const char *cName = hjsonSegment["Name"];

		CSlrString *name = new CSlrString(cName);
		CDebugSymbolsSegment *segment = FindSegment(name);
		
		if (segment == NULL)
		{
			segment = CreateNewDebugSymbolsSegment(new CSlrString(name));
			segments.push_back(segment);
		}
		delete name;

		Hjson::Value hjsonWatches = hjsonSegment["Watches"];
		segment->DeserializeWatches(hjsonWatches);
	}
	return true;
}

///
// TODO: generalize me as this is mainly copy pasted code from above!
void CDebugSymbols::LoadBreakpointsRetroDebuggerFormat(CSlrString *filePath)
{
	LOGD("CDebugSymbols::LoadBreakpointsRetroDebuggerFormat");
	filePath->DebugPrint("filePath=");
	
	CByteBuffer *byteBuffer = new CByteBuffer(filePath);
	if (!byteBuffer->IsEmpty())
	{
		char *jsonText = new char[byteBuffer->length+2];
		memcpy(jsonText, byteBuffer->data, byteBuffer->length);
		jsonText[byteBuffer->length] = 0;
		
		Hjson::Value hjsonRoot;
		std::stringstream ss;
		ss.str(jsonText);
		
		try
		{
			ss >> hjsonRoot;
			DeserializeBreakpointsFromHjson(hjsonRoot);
		}
		catch (const std::exception& e)
		{
			LOGError("CDebugSymbols::DeserializeBreakpointsFromHjson error: %s", e.what());
			
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "Loading breakpoints failed. Error:\n%s", e.what());
			guiMain->ShowMessageBox("Error", buf);
			SYS_ReleaseCharBuf(buf);
		}
	}
	
	delete byteBuffer;
	
	if (segments.empty())
	{
		CreateDefaultSegment();
	}
		
	if (currentSegment == NULL)
	{
		currentSegment = segments.front();
	}

	debugInterface->symbols->UpdateRenderBreakpoints();

}

void CDebugSymbols::SaveBreakpointsRetroDebuggerFormat(CSlrString *filePath)
{
	LOGD("CDebugSymbols::SaveBreakpointsRetroDebuggerFormat");
	filePath->DebugPrint("filePath=");
	
	const char *cstrPath = filePath->GetStdASCII();
	
	FILE *fp = fopen(cstrPath, "wb");
	if (!fp)
	{
		LOGError("CDebugSymbols::SaveBreakpointsRetroDebuggerFormat: can't write to %s", cstrPath);
		
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "Can't write to file\n%s", cstrPath);
		guiMain->ShowMessageBox("Error", buf);
		SYS_ReleaseCharBuf(buf);
		delete [] cstrPath;
		return;
	}
	delete [] cstrPath;

	Hjson::Value hjsonRoot;

	CSlrDate *date = new CSlrDate();
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", date->year, date->month, date->day, date->hour, date->minute, date->second);

	std::string comment = "// Breakpoints exported by RetroDebugger v";
	comment += RETRODEBUGGER_VERSION_STRING;
	comment += " on ";
	comment += buf;
	comment += "\n\r";
	hjsonRoot.set_comment_before(comment);
	
	SYS_ReleaseCharBuf(buf);

	//
	hjsonRoot["Version"] = "1";

	SerializeBreakpointsToHjson(hjsonRoot);

	std::stringstream ss;
	ss << Hjson::Marshal(hjsonRoot);
	
	std::string s = ss.str();
	const char *cstrHjson = s.c_str();

	fprintf(fp, "%s", cstrHjson);
	
	fclose(fp);
}

bool CDebugSymbols::SerializeBreakpointsToHjson(Hjson::Value hjsonRoot)
{
	Hjson::Value hjsonSegmentsArray;
	for (auto segment : segments)
	{
		Hjson::Value hjsonSegment;
		char *cName = segment->name->GetStdASCII();
		hjsonSegment["Name"] = cName;
		STRFREE(cName);

		Hjson::Value hjsonBreakpointsTypes;
		segment->SerializeBreakpoints(hjsonBreakpointsTypes);
		hjsonSegment["Breakpoints"] = hjsonBreakpointsTypes;

		hjsonSegmentsArray.push_back(hjsonSegment);
	}
	hjsonRoot["Segments"] = hjsonSegmentsArray;
	return true;
}

bool CDebugSymbols::DeserializeBreakpointsFromHjson(Hjson::Value hjsonRoot)
{
	Hjson::Value hjsonSegmentsArray = hjsonRoot["Segments"];
	for (int i = 0; i < hjsonSegmentsArray.size(); i++)
	{
		Hjson::Value hjsonSegment = hjsonSegmentsArray[i];
		const char *cName = hjsonSegment["Name"];

		CSlrString *name = new CSlrString(cName);
		CDebugSymbolsSegment *segment = FindSegment(name);
		
		if (segment == NULL)
		{
			segment = CreateNewDebugSymbolsSegment(new CSlrString(name));
			segments.push_back(segment);
		}
		delete name;

		Hjson::Value hjsonBreakpointsTypes = hjsonSegment["Breakpoints"];
		segment->DeserializeBreakpoints(hjsonBreakpointsTypes);
	}
	return true;}


CDebugSymbolsSegment *CDebugSymbols::FindSegment(CSlrString *segmentName)
{
//	LOGD("CDebugSymbols::FindSegment");
//	segmentName->DebugPrint("segmentName=");
//	segmentName->DebugPrintVector("segmentName=");
	// TODO: create map of segment names. this function is rarely used, and there are so not many segments normally, so just iterate for now.
	for (std::vector<CDebugSymbolsSegment *>::iterator it = segments.begin(); it != segments.end(); it++)
	{
		CDebugSymbolsSegment *segment = *it;
		if (segmentName->CompareWith(segment->name))
		{
			return segment;
		}
	}
	
	return NULL;
}

void CDebugSymbols::ActivateSegment(CDebugSymbolsSegment *segment)
{
	LOGD("CDebugSymbols::ActivateSegment: segment=%x", segment);
	
	// TODO: we should store this directly and in a generic way.
	//       this now is temporary and needs proper refactor:
	
	// first, copy current breakpoints to segment in case they have been changed
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	this->UpdateRenderBreakpoints();

	this->currentSegment = segment;
	this->currentSegmentNum = segment->segmentNum;

	segment->Activate();
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

void CDebugSymbols::UpdateRenderBreakpoints()
{
	for (std::vector<CDebugSymbolsSegment *>::iterator it = segments.begin(); it != segments.end(); it++)
	{
		CDebugSymbolsSegment *segment = *it;
		segment->UpdateRenderBreakpoints();
	}
}

void CDebugSymbols::DeactivateSegment()
{
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	this->currentSegment = NULL;
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

//
void CDebugSymbols::SelectNextSegment()
{
	guiMain->LockMutex();
	debugInterface->LockMutex();

	this->currentSegmentNum++;
	
	if (this->currentSegmentNum == this->segments.size())
	{
		this->currentSegmentNum = 0;
	}
	
	CDebugSymbolsSegment *segment = this->segments[this->currentSegmentNum];
	this->ActivateSegment(segment);
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

void CDebugSymbols::SelectPreviousSegment()
{
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	if (this->currentSegmentNum == 0)
	{
		this->currentSegmentNum = this->segments.size()-1;
	}
	else
	{
		this->currentSegmentNum--;
	}
	CDebugSymbolsSegment *segment = this->segments[this->currentSegmentNum];
	this->ActivateSegment(segment);
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

void CDebugSymbols::ClearTemporaryBreakpoint()
{
	for (std::vector<CDebugSymbolsSegment *>::iterator it = segments.begin(); it != segments.end(); it++)
	{
		CDebugSymbolsSegment *segment = *it;
		segment->breakpointsPC->SetTemporaryBreakpointPC(-1);
	}
}

void CDebugSymbols::CreateDefaultSegment()
{
	// create default segment, note we need to have at least one segment always available (symbols->currentSegment)
	LOGD("CDebugSymbols::CreateDefaultSegment");
	
	CSlrString *defaultSegmentName = new CSlrString("Default");
	this->currentSegment = this->CreateNewDebugSymbolsSegment(defaultSegmentName, 0);
	this->segments.push_back(this->currentSegment);
}

// virtual method to create specific symbols segment (for C64 including raster line, VIC, CIA, NMI irqs, for Drive including VIA irqs, etc.)
CDebugSymbolsSegment *CDebugSymbols::CreateNewDebugSymbolsSegment(CSlrString *name)
{
	int segmentNum = segments.size();
	return CreateNewDebugSymbolsSegment(name, segmentNum);
}

CDebugSymbolsSegment *CDebugSymbols::CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum)
{
	LOGD("CDebugSymbols::CreateNewDebugSymbolsSegment");
	
	CDebugSymbolsSegment *segment = new CDebugSymbolsSegment(this, name, segmentNum);
	segment->Init();
	
	return segment;
}

// get currently executed addr (PC) to be checked against breakpoint, note it is virtual to have it different for C64 Drive CPU
int CDebugSymbols::GetCurrentExecuteAddr()
{
	return debugInterface->GetCpuPC();
}

void CDebugSymbols::LockMutex()
{
	debugInterface->LockMutex();
}

void CDebugSymbols::UnlockMutex()
{
	debugInterface->UnlockMutex();
}
