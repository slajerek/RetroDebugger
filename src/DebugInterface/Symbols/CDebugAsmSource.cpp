#include "CDebugAsmSource.h"
#include "CGuiMain.h"
#include "CByteBuffer.h"
#include "CViewC64.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "utf8.h"
#include "pugixml.h"
#include "std_membuf.h"
#include "CSlrFileFromOS.h"
#include "CViewDisassembly.h"
#include "CViewDataWatch.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceC64.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"

// TODO: the code below is still just a POC written in hurry, meaning this might leak memory a lot and needs proper refactoring
//       hopefully the labels are not loaded very often... be warned!

#define MODE_IDLE				0
#define MODE_READING_SOURCES	1
#define MODE_READING_MAP		2

CDebugAsmSource::CDebugAsmSource(CByteBuffer *byteBuffer, CDebugInterface *debugInterface, CDataAdapter *dataAdapter)
{
	LOGD("CDebugAsmSource::CDebugAsmSource");
	
	this->debugInterface = debugInterface;
	this->dataAdapter = dataAdapter;
	
	this->maxMemoryAddress = dataAdapter->AdapterGetDataLength();

//	ParseOldFormat(byteBuffer, debugInterface);
	ParseXML(byteBuffer, debugInterface);
}

CDebugAsmSource::~CDebugAsmSource()
{
	LOGD("~CDebugAsmSource");
	
	guiMain->LockMutex();
	debugInterface->LockMutex();

	while (!codeSourceFilesById.empty())
	{
		std::map<u32, CDebugAsmSourceFile *>::iterator it = codeSourceFilesById.begin();
		CDebugAsmSourceFile *file = it->second;
		codeSourceFilesById.erase(it);
		delete file;
	}
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
	
	LOGD("~CDebugAsmSource finished");
}

//	"   &quot;
//	'   &apos;
//	<   &lt;
//	>   &gt;
//	&   &amp;

void CDebugAsmSource::ParseXML(CByteBuffer *byteBuffer, CDebugInterface *debugInterface)
{
	LOGD("CDebugAsmSource::ParseXML byteBuffer=%x debugInterface=%x", byteBuffer, debugInterface);

	byteBuffer->removeCRLFinQuotations();
	
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(byteBuffer->data, byteBuffer->length);
	
	if (!result)
	{
		LOGError("Loading debug symbols failed: %s at %d", result.description(), result.offset);
		return;
	}
	
	LOGD("CDebugAsmSource::ParseXML: parsed OK");

	pugi::xml_node rootTag = doc.first_child();
	pugi::xml_attribute rootAttr = rootTag.first_attribute();
	
	if (strcmp(rootAttr.name(), "version"))
	{
		LOGError("Version tag mismatch (%s)", rootAttr.name());
		return;
	}
	if (strcmp(rootAttr.value(), "1.0"))
	{
		LOGError("Unknown version of debug symbols (%s)", rootAttr.value());
		return;
	}
	
	//
	//	for (pugi::xml_node tag = rootTag.first_child(); tag; tag = tag.next_sibling())
	//	{
	//		LOGD(">>> %s:", tag.name());
	//
	//		for (pugi::xml_attribute attr = tag.first_attribute(); attr; attr = attr.next_attribute())
	//		{
	//			LOGD("%s=%s", attr.name(), attr.value());
	//			LOGD("   %s", tag.child_value());
	//		}
	//	}

	
	// parse_escapes
	std::list<u16> splitCharsLine;
	splitCharsLine.push_back('\n');
	splitCharsLine.push_back('\r');

	std::list<u16> splitCharsComma;
	splitCharsComma.push_back(',');
	
	std::list<u16> splitCharsBreakpoints;
	splitCharsBreakpoints.push_back(' ');
	splitCharsBreakpoints.push_back('<');
	splitCharsBreakpoints.push_back('>');
	splitCharsBreakpoints.push_back('=');
	splitCharsBreakpoints.push_back('!');
	splitCharsBreakpoints.push_back('#');

	LOGD("== Sources");
	pugi::xml_node nodeSources = rootTag.child("Sources");
	
	if (nodeSources.empty())
	{
		LOGError("Sources list not found");
		return;
	}
	
	// parse sources
	CSlrString *strSources = new CSlrString(nodeSources.child_value());
	std::vector<CSlrString *> *lines = strSources->SplitWithChars(splitCharsLine);
	
	LOGD("lines=");
	for (std::vector<CSlrString *>::iterator it = lines->begin(); it != lines->end(); it++)
	{
		CSlrString *oneLine = *it;
		if (oneLine->GetLength() < 2)
			continue;
		
		oneLine->RemoveFromBeginningSelectedCharacter(' ');
		oneLine->RemoveFromBeginningSelectedCharacter('\t');
		
		//				oneLine->DebugPrint("oneLine");
		//				oneLine->DebugPrintVector("oneLine");
		
		std::vector<CSlrString *> *words = oneLine->SplitWithChars(splitCharsComma);
		
		if (words->size() == 0)
			continue;
		
		if (words->size() != 3)
		{
			LOGError("Wrong format for sources list (%d words)", words->size()); // at line #%d", tag.l);
			continue;
		}
		
		CDebugAsmSourceFile *asmSourceFile = new CDebugAsmSourceFile();
		
		int sourceId = (*words)[0]->ToInt();
		
		asmSourceFile->sourceId = sourceId;
		asmSourceFile->sourceFilePath = new CSlrString((*words)[2]);
		asmSourceFile->sourceFileName = asmSourceFile->sourceFilePath->GetFileNameComponentFromPath();
		
		codeSourceFilesById[sourceId] = asmSourceFile;
		
		LOGD("... added source id: %d", asmSourceFile->sourceId);
		asmSourceFile->sourceFilePath->DebugPrint("...    source file path=");
		
		LOGD("CSlrString::DeleteVector(words): 1 words=%x", words);
		CSlrString::DeleteVector(words);
	}
	LOGD("	CSlrString::DeleteVector(lines): 2 lines=%x", lines);
	CSlrString::DeleteVector(lines);
	delete strSources;
	
	LOGD("== Segments");

	CDebugSymbols *symbols = debugInterface->symbols;
	
	int segmentNum = 0;
	for (pugi::xml_node segmentNode = rootTag.child("Segment"); segmentNode; segmentNode = segmentNode.next_sibling("Segment"))
	{
		pugi::xml_attribute attrSegmentName = segmentNode.attribute("name");
		LOGD("Segment: %s", attrSegmentName.value());
		
		CSlrString *segmentName = new CSlrString(attrSegmentName.value());
		
		CDebugSymbolsSegment *segment = symbols->CreateNewDebugSymbolsSegment(segmentName, segmentNum++);
		symbols->segments.push_back(segment);

		for (pugi::xml_node blockNode = segmentNode.child("Block"); blockNode; blockNode = blockNode.next_sibling("Block"))
		{
			pugi::xml_attribute attrBlockName = blockNode.attribute("name");
			LOGD("  Block: %s", attrBlockName.value());
			
			CSlrString *blockName = new CSlrString(attrBlockName.value());
			
			CDebugAsmSourceBlock *block = new CDebugAsmSourceBlock(segment, blockName);
			segment->blocks.push_back(block);
			
			// parse sources
			CSlrString *strBlock = new CSlrString(blockNode.child_value());
			std::vector<CSlrString *> *lines = strBlock->SplitWithChars(splitCharsLine);
			
			for (std::vector<CSlrString *>::iterator it = lines->begin(); it != lines->end(); it++)
			{
				CSlrString *oneLine = *it;
				if (oneLine->GetLength() < 2)
					continue;
				
				oneLine->RemoveFromBeginningSelectedCharacter(' ');
				oneLine->RemoveFromBeginningSelectedCharacter('\t');
				
//				oneLine->DebugPrint("  > ");
				//			oneLine->DebugPrintVector("oneLine");
				
				std::vector<CSlrString *> *words = oneLine->SplitWithChars(splitCharsComma);
				
				if (words->size() == 0)
					continue;
				
//				LOGD("words->size=%d", words->size());
				
				if (words->size() == 13)
				{
//								LOGD("... parsing");
					
					int memoryAddrStart = (*words)[0]->ToIntFromHex();
								LOGD("   memoryAddrStart=%04x", memoryAddrStart);
					
					int memoryAddrEnd = (*words)[2]->ToIntFromHex();
								LOGD("   memoryAddrEnd=%04x", memoryAddrEnd);
					
					int sourceId = (*words)[4]->ToInt();
								LOGD("   sourceId=%d", sourceId);
					
					int lineNumStart = (*words)[6]->ToInt();
								LOGD("   lineNumStart=%d", lineNumStart);
					
					int columnNumStart = (*words)[8]->ToInt();
								LOGD("   columnNumStart=%d", columnNumStart);
					
					int lineNumEnd = (*words)[10]->ToInt();
								LOGD("   lineNumEnd=%d", lineNumEnd);
					
					int columnNumEnd = (*words)[12]->ToInt();
								LOGD("   columnNumEnd=%d", columnNumEnd);
					
					
					std::map<u32, CDebugAsmSourceFile *>::iterator it = codeSourceFilesById.find(sourceId);
					
					if (it != codeSourceFilesById.end())
					{
						CDebugAsmSourceFile *asmSourceFile = it->second;
						
						CDebugAsmSourceLine *asmSourceLine = new CDebugAsmSourceLine();
						asmSourceLine->codeFile = asmSourceFile;
						asmSourceLine->block = block;
						asmSourceLine->codeLineNumberStart = lineNumStart;
						asmSourceLine->codeColumnNumberStart = columnNumStart;
						asmSourceLine->codeLineNumberEnd = lineNumEnd;
						asmSourceLine->codeColumnNumberEnd = columnNumEnd;
						asmSourceLine->memoryAddressStart = memoryAddrStart;
						asmSourceLine->memoryAddressEnd = memoryAddrEnd;
						
						asmSourceFile->asmSourceLines.push_back(asmSourceLine);
						
						for (int addr = memoryAddrStart; addr <= memoryAddrEnd; addr++)
						{
							if (addr >= 0 && addr < maxMemoryAddress)
							{
								segment->codeSourceLineByMemoryAddress[addr] = asmSourceLine;
//								block->codeSourceLineByMemoryAddress[addr] = asmSourceLine;
								LOGD("added code segment=%x at %x asmSourceLine=%x", segment, addr, asmSourceLine);
							}
							else
							{
								LOGError("Address %04x is out of memory range", addr); // at line #%d", addr, lineNum);
								break;
							}
						}

					}
					else
					{
						LOGError("Source code id #%d not found", sourceId); // at line #%d", sourceId, lineNum);
					}
				}
				else
				{
					LOGError("Wrong format for source code address mapping"); // at line #%d", lineNum);
				}
				LOGD("CSlrString::DeleteVector(words): 3 words=%x", words);
				CSlrString::DeleteVector(words);
			}
			LOGD("CSlrString::DeleteVector(lines): 4 lines=%x", lines);
			CSlrString::DeleteVector(lines);
			delete strBlock;
		}
	}
	
	LOGD("== Labels");
	pugi::xml_node nodeLabels = rootTag.child("Labels");
	
	if (!nodeLabels.empty())
	{
		// parse labels
		CSlrString *strLabels = new CSlrString(nodeLabels.child_value());
		std::vector<CSlrString *> *lines = strLabels->SplitWithChars(splitCharsLine);
		
		LOGD("lines=");
		for (std::vector<CSlrString *>::iterator it = lines->begin(); it != lines->end(); it++)
		{
			CSlrString *oneLine = *it;
			if (oneLine->GetLength() < 2)
				continue;
			
			oneLine->RemoveFromBeginningSelectedCharacter(' ');
			oneLine->RemoveFromBeginningSelectedCharacter('\t');
			
							oneLine->DebugPrint("> ");
			//				oneLine->DebugPrintVector("oneLine");
			
			std::vector<CSlrString *> *words = oneLine->Split(splitCharsComma);
			
			if (words->size() == 0)
				continue;
			
			// Note, Mads changed format recently and now labels also include information about code lines which are not needed for us now
			if (words->size() < 3)
			{
				LOGError("Not enough words for label definition (%d words)", words->size()); // at line #%d", tag.line);
				CSlrString::DeleteVector(words);
				continue;
			}
//			if (words->size() > 3)
//			{
//				LOGWarning("Wrong format for labels (%d words)", words->size()); // at line #%d", tag.line);
//			}

			CSlrString *segmentName = (*words)[0];
			CSlrString *strAddr = (*words)[1];
			int address = strAddr->ToIntFromHex();
			CSlrString *labelName = (*words)[2];
			char *labelNameStr = labelName->GetStdASCII();
			
			CDebugSymbolsSegment *segment = symbols->FindSegment(segmentName);
			if (segment == NULL)
			{
				segmentName->DebugPrint("segment=");
				LOGError("ParseLabels: segment not found");
				CSlrString::DeleteVector(words);
				continue;
			}

			// do not add empty labels
			if (labelNameStr[0] != 0)
			{
				segment->AddCodeLabel(address, labelNameStr);
			}

			LOGD("CSlrString::DeleteVector(words): 5");
			CSlrString::DeleteVector(words);
		}
	}
	
	LOGD("== Watchpoints");
	pugi::xml_node nodeWatches = rootTag.child("Watchpoints");
	
	if (!nodeWatches.empty())
	{
		// parse watches
		CSlrString *strWatches = new CSlrString(nodeWatches.child_value());
		std::vector<CSlrString *> *lines = strWatches->SplitWithChars(splitCharsLine);
		
		LOGD("lines=");
		for (std::vector<CSlrString *>::iterator it = lines->begin(); it != lines->end(); it++)
		{
			CSlrString *oneLine = *it;
			if (oneLine->GetLength() < 2)
				continue;
			
			oneLine->RemoveFromBeginningSelectedCharacter(' ');
			oneLine->RemoveFromBeginningSelectedCharacter('\t');
			
			oneLine->DebugPrint("> ");
			//				oneLine->DebugPrintVector("oneLine");
			
			std::vector<CSlrString *> *words = oneLine->Split(splitCharsComma);
			
			if (words->size() == 0)
				continue;
			
			if (words->size() < 2)
			{
				LOGError("Wrong format for watches (%d words)", words->size()); // at line #%d", tag.line);
				CSlrString::DeleteVector(words);
				continue;
			}
			
			CSlrString *segmentName = (*words)[0];
			CSlrString *strAddr = (*words)[1];
			int address = strAddr->ToIntFromHex();
			
			int numberOfValues = 1;
			CSlrString *strRepresentation = NULL;
			
			if (words->size() > 2)
			{
				CSlrString *strNumberOfValues = (*words)[2];
				numberOfValues = strNumberOfValues->ToInt();
				if (numberOfValues < 1)
				{
					numberOfValues = 1;
				}
			}
			
			if (words->size() > 3)
			{
				strRepresentation = (*words)[3];
			}
			
			CDebugSymbolsSegment *segment = symbols->FindSegment(segmentName);
			if (segment == NULL)
			{
				segmentName->DebugPrint("segment=");
				LOGError("ParseWatches: segment not found");
				CSlrString::DeleteVector(words);
				continue;
			}
			
			segment->AddWatch(address, numberOfValues, strRepresentation);

			LOGD("CSlrString::DeleteVector(words): 6 words=%x", words);
			CSlrString::DeleteVector(words);
		}
	}


	LOGD("== Breakpoints");
	pugi::xml_node nodeBreakpoints = rootTag.child("Breakpoints");
	
	if (!nodeBreakpoints.empty())
	{
		// parse sources
		CSlrString *strBreakpoints = new CSlrString(nodeBreakpoints.child_value());
		std::vector<CSlrString *> *lines = strBreakpoints->SplitWithChars(splitCharsLine);
		
		for (std::vector<CSlrString *>::iterator it = lines->begin(); it != lines->end(); it++)
		{
			CSlrString *oneLine = *it;
			if (oneLine->GetLength() < 2)
				continue;
			
			oneLine->RemoveFromBeginningSelectedCharacter(' ');
			oneLine->RemoveFromBeginningSelectedCharacter('\t');
			
			oneLine->DebugPrint("  > ");
//			oneLine->DebugPrintVector("oneLine");
			
			std::vector<CSlrString *> *breakpointWords = oneLine->SplitWithChars(splitCharsComma);
			
			if (breakpointWords->size() == 0)
				continue;
			
//			LOGD("breakpointWords->size=%d", breakpointWords->size());
			
			//
			CSlrString *segmentName = (*breakpointWords)[0];
			CSlrString *strAddr = (*breakpointWords)[2];
			int breakpointAddress = strAddr->ToIntFromHex();
			
			CDebugSymbolsSegment *segment = symbols->FindSegment(segmentName);
			if (segment == NULL)
			{
				segmentName->DebugPrint("segment=");
				LOGError("ParseBreakpoints: segment not found");
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}

			if (breakpointWords->size() == 4)
			{
				segment->AddBreakpointPC(breakpointAddress);

				LOGD("CSlrString::DeleteVector(breakpointWords): 7 breakpointWords=%x", breakpointWords);
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}

			if (breakpointWords->size() != 5)
			{
				LOGError("ParseBreakpoints: unknown format (num elements=%d)", breakpointWords->size());
				LOGD("CSlrString::DeleteVector(breakpointWords): 8 breakpointWords=%x", breakpointWords);
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}
			
			CSlrString *breakpointDefinitionStr = (*breakpointWords)[4];
//				breakpointDefinitionStr->DebugPrint("breakpointDefinitionStr=");
			std::vector<CSlrString *> *words = breakpointDefinitionStr->SplitWithChars(splitCharsBreakpoints);
			
//				LOGD(">>>> words->size=%d:", words->size());
//				for (int i = 0; i < words->size(); i++)
//				{
//					(*words)[i]->DebugPrint();
//				}
//				LOGD("<<<<");
			
			CSlrString *command = (*words)[0];
			
//				command->DebugPrint("command=");
			
			// comment?
			if (command->GetChar(0) == '#')
			{
				continue;
			}
			
			command->ConvertToLowerCase();
			
			if (command->Equals("break") || command->Equals("breakpc") || command->Equals("breakonpc"))
			{
				LOGD(".. adding breakOnPC %4.4x", breakpointAddress);
				
				segment->AddBreakpointPC(breakpointAddress);
			}
			else if (command->Equals("setbkg") || command->Equals("setbackground"))
			{
				if (words->size() < 3)
				{
					LOGError("ParseBreakpoints: error with setbkg"); //in line %d", lineNum);
					break;
				}
				
				// pc breakpoint
				CSlrString *arg = (*words)[2];
				int value = arg->ToIntFromHex();
				
				LOGD(".. adding setBkg %4.4x %2.2x", breakpointAddress, value);
				
				segment->AddBreakpointSetBackground(breakpointAddress, value);
				
			}
			else if (command->Equals("breakmemory") || command->Equals("breakonmemory") || command->Equals("breakmem")
					 || command->Equals("mem"))
			{
				if (words->size() < 4)
				{
					LOGError("ParseBreakpoints: error with breakmemory"); //in line %d", lineNum);
					break;
				}
				
				CSlrString *addressStr = (*words)[2];
//					addressStr->DebugPrint(" addressStr=");
				int address = addressStr->ToIntFromHex();
				
				int index = 3;
				CSlrString *op = new CSlrString();
				
				while (index < words->size()-1)
				{
					CSlrString *f = (*words)[index];
					f->ConvertToLowerCase();
					
//							f->DebugPrint(".... f= ");
					
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
					LOGError("ParseBreakpoints: error"); //in line %d", lineNum);
					break;
				}
				
				CSlrString *arg = (*words)[index];
//					arg->DebugPrint(" arg=");
				
				int value = arg->ToIntFromHex();
				
				DataBreakpointComparison memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
				
				if (op->Equals("==") || op->Equals("="))
				{
					memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
				}
				else if (op->Equals("!="))
				{
					memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL;
				}
				else if (op->Equals("<"))
				{
					memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_LESS;
				}
				else if (op->Equals("<=") || op->Equals("=<"))
				{
					memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL;
				}
				else if (op->Equals(">"))
				{
					memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER;
				}
				else if (op->Equals(">=") || op->Equals("=>"))
				{
					memBreakComparison = DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL;
				}
				else
				{
					LOGError("ParseBreakpoints: unknown operator for memory breakpoint"); //, lineNum);
					break;
				}
				
				LOGD(".. adding breakOnMemory");
				LOGD("..... addr=%4.4x", address);
				op->DebugPrint("..... op=");
				LOGD("..... value=%2.2x", value);
				
				segment->AddBreakpointMemory(address, MEMORY_BREAKPOINT_ACCESS_WRITE, memBreakComparison, value);
				
			}
			else if (segment->ParseSymbolsXML(command, words))
			{
				// specific for segment
			}
			else
			{
				LOGError("ParseBreakpoints: unknown breakpoint type"); //, lineNum);
				break;
			}
			
			LOGD("CSlrString::DeleteVector(words): 9 words=%x", words);
			CSlrString::DeleteVector(words);
			LOGD("CSlrString::DeleteVector(breakpointWords): 10 breakpointWords=%x", breakpointWords);
			CSlrString::DeleteVector(breakpointWords);
		}
		LOGD("CSlrString::DeleteVector(lines): 11 lines=%x", lines);
		CSlrString::DeleteVector(lines);
		delete strBreakpoints;
	}
	

	LOGD("== ");

	//
	// load source files
	//
	
	for (std::map<u32, CDebugAsmSourceFile *>::iterator it = codeSourceFilesById.begin();
		 it != codeSourceFilesById.end(); it++)
	{
		CDebugAsmSourceFile *asmSourceFile = it->second;
		
		CSlrFile *file = new CSlrFileFromOS(asmSourceFile->sourceFilePath);
		
		//
		if (file->Exists())
		{
			asmSourceFile->sourceFilePath->DebugPrint("<<<<<< OPENED  sourceFilePath=");
			LOGD("File opened");
			
			this->LoadSource(asmSourceFile, file);
			
			asmSourceFile->sourceFilePath->DebugPrint(">>>>> FINISHED loading source=");
		}
	}
	
	// activate first or selected segment
	symbols->ActivateSelectedSegment();
	
	LOGM("CDebugAsmSource::ParseXML: symbols loaded");
}

// this is parser for "old" debug symbols format discussed with Mads long time ago (not XML-based, not used anymore)
void CDebugAsmSource::ParseOldFormat(CByteBuffer *byteBuffer, CDebugInterface *debugInterface)
{
	int currentMode = MODE_IDLE;
	
	CDebugSymbols *symbols = debugInterface->symbols;
	CDebugSymbolsSegment *segment = debugInterface->symbols->currentSegment;
		
	// parse
	byteBuffer->removeCRLFinQuotations();
	
	std_membuf mb(byteBuffer->data, byteBuffer->length);
	std::istream reader(&mb);
	
	unsigned lineNum = 1;
	std::string line;
	line.clear();
	
	std::list<u16> splitChars;
	splitChars.push_back(' ');
	splitChars.push_back(',');
	
	// Play with all the lines in the file
	while (true)
	{
		if (!getline(reader, line))
			break;
		
		LOGD("---- line=%d", lineNum);

		if (line.length() == 0)
		{
			lineNum++;
			continue;
		}

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
		
		CSlrString *strLine = new CSlrString(utf16line);
		strLine->RemoveEndLineCharacter();	// workaround for stupid Windows getline implementation
		
		strLine->DebugPrint("line str=");
		
		std::vector<CSlrString *> *words = strLine->SplitWithChars(splitChars);
		
		if (words->size() == 0)
		{
			lineNum++;
			continue;
		}
		
//		LOGD("words->size=%d", words->size());
//		for (int i = 0; i < words->size(); i++)
//		{
//			LOGD("... words[%d]", i);
//			CSlrString *str = (*words)[i];
//			str->DebugPrint("... =");
//		}

		CSlrString *command = (*words)[0];
		
		// comment?
		if (command->GetChar(0) == '#')
		{
			lineNum++;
			continue;
		}

		if (currentMode == MODE_IDLE)
		{
			command->ConvertToLowerCase();
			command->DebugPrint("command=");
			if (command->CompareWith("sources={"))
			{
				// load list of file sources
				LOGD("...sources:");
				currentMode = MODE_READING_SOURCES;
			}
			else
			{
				LOGError("Unknown command at line #%d (MODE_IDLE)", lineNum);
				continue;
			}
		}
		else if (currentMode == MODE_READING_SOURCES)
		{
			LOGD("... one source");
			
			if (command->CompareWith("}"))
			{
				LOGD("... done reading sources");
				currentMode = MODE_READING_MAP;
				continue;
			}
			
			if (words->size() != 3)
			{
				LOGError("Wrong format for sources list at line #%d", lineNum);
				continue;
			}
			
			CDebugAsmSourceFile *asmSourceFile = new CDebugAsmSourceFile();
			
			int sourceId = (*words)[0]->ToInt();

			asmSourceFile->sourceId = sourceId;
			asmSourceFile->sourceFilePath = new CSlrString((*words)[2]);
			asmSourceFile->sourceFileName = asmSourceFile->sourceFilePath->GetFileNameComponentFromPath();
			
			codeSourceFilesById[sourceId] = asmSourceFile;
			
			LOGD("... added source id: %d", asmSourceFile->sourceId);
			asmSourceFile->sourceFilePath->DebugPrint("...    source file path=");
		}
		else if (currentMode == MODE_READING_MAP)
		{
			LOGD("... map line");
			
			if (words->size() != 13)
			{
				LOGError("Wrong format for source code address mapping at line #%d", lineNum);
				continue;
			}
			
//			LOGD("... parsing");
			
			int memoryAddrStart = (*words)[0]->ToIntFromHex();
//			LOGD("   memoryAddrStart=%04x", memoryAddrStart);

			int memoryAddrEnd = (*words)[2]->ToIntFromHex();
//			LOGD("   memoryAddrEnd=%04x", memoryAddrEnd);

			int sourceId = (*words)[4]->ToInt();
//			LOGD("   sourceId=%d", sourceId);

			int lineNumStart = (*words)[6]->ToInt();
//			LOGD("   lineNumStart=%d", lineNumStart);

			int columnNumStart = (*words)[8]->ToInt();
//			LOGD("   columnNumStart=%d", columnNumStart);

			int lineNumEnd = (*words)[10]->ToInt();
//			LOGD("   lineNumEnd=%d", lineNumEnd);
			
			int columnNumEnd = (*words)[12]->ToInt();
//			LOGD("   columnNumEnd=%d", columnNumEnd);
			
			
			std::map<u32, CDebugAsmSourceFile *>::iterator it = codeSourceFilesById.find(sourceId);
			
			if (it == codeSourceFilesById.end())
			{
				LOGError("Source code id #%d not found at line #%d", sourceId, lineNum);
				continue;
			}
			
			CDebugAsmSourceFile *asmSourceFile = it->second;
			
			CDebugAsmSourceLine *asmSourceLine = new CDebugAsmSourceLine();
			asmSourceLine->codeFile = asmSourceFile;
			asmSourceLine->block = NULL;
			asmSourceLine->codeLineNumberStart = lineNumStart;
			asmSourceLine->codeColumnNumberStart = columnNumStart;
			asmSourceLine->codeLineNumberEnd = lineNumEnd;
			asmSourceLine->codeColumnNumberEnd = columnNumEnd;
			asmSourceLine->memoryAddressStart = memoryAddrStart;
			asmSourceLine->memoryAddressEnd = memoryAddrEnd;
			
			asmSourceFile->asmSourceLines.push_back(asmSourceLine);
			
			for (int addr = memoryAddrStart; addr <= memoryAddrEnd; addr++)
			{
				if (addr >= 0 && addr < maxMemoryAddress)
				{
					segment->codeSourceLineByMemoryAddress[addr] = asmSourceLine;
				}
				else
				{
					LOGError("Address %04x is out of memory range at line #%d", addr, lineNum);
					break;
				}
			}
			
//			LOGD("---");
			
		}
		
		
		delete strLine;
		for (int i = 0; i < words->size(); i++)
		{
			delete (*words)[i];
		}
		delete  words;
		
		lineNum++;
	}
	
	// load source files
	
	// 	std::map<u32, CDebugAsmSourceFile *> codeSourceFilesById;

	std::map<u32, CDebugAsmSourceFile *>::iterator it = codeSourceFilesById.begin();
	
	// TODO: convert makefiles to c11
	for (
		 // it
		 ;  it != codeSourceFilesById.end();
			it++)
	{
		CDebugAsmSourceFile *asmSourceFile = it->second;
		
		CSlrFile *file = new CSlrFileFromOS(asmSourceFile->sourceFilePath);
		
		//
		if (file->Exists())
		{
			asmSourceFile->sourceFilePath->DebugPrint("<<<<<< OPENED  sourceFilePath=");
			LOGD("File opened");
			
			this->LoadSource(asmSourceFile, file);
			
			asmSourceFile->sourceFilePath->DebugPrint(">>>>> FINISHED loading source=");
		}
	}
	
	symbols->ActivateSegment(segment);

	/////////////////////////////////////////

	LOGD("CDebugAsmSource::CDebugAsmSource: ParseOldFormat finished");

}

void CDebugAsmSource::LoadSource(CDebugAsmSourceFile *asmSourceFile, CSlrFile *file)
{
	CByteBuffer *byteBuffer = file->GetWholeFileAsByteBuffer();
	//			byteBuffer->DebugPrint();
	
	LOGD("LoadSource...");
	
	//byteBuffer->removeCRLFinQuotations();

	std_membuf mb(byteBuffer->data, byteBuffer->length);
	std::istream reader(&mb);
	
	unsigned lineNum = 1;
	std::string line;
	line.clear();
	
	// push zero line
	asmSourceFile->codeTextByLineNum.push_back(new CSlrString(""));

	// TODO: UI -> replace tabs with 4 spaces
	CSlrString *strTabs = new CSlrString("    ");
	
	while (true)
	{
		if (!getline(reader, line))
			break;
		
//		LOGD("---- line=%d len=%d eof=%s", lineNum, line.length(), STRBOOL(reader.eof()));
		
//		std::cout << line.length() << std::endl;
//		
//		LOGD("Line is:");
//		std::cout << line << std::endl;
//
//		LOGD("---- parse line %d", lineNum);
//
		
		// check for invalid utf-8 (for a simple yes/no check, there is also utf8::is_valid function)
		std::string::iterator end_it = utf8::find_invalid(line.begin(), line.end());
		if (end_it != line.end())
		{
			LOGError("Invalid UTF-8 encoding detected at line %d", lineNum);
		}
		
		// Get the line length (at least for the valid part)
		int length = utf8::distance(line.begin(), end_it);
//		LOGD("========================= Length of line %d is %d", lineNum, length);
		
		// Convert it to utf-16
		std::vector<unsigned short> utf16line;
		utf8::utf8to16(line.begin(), end_it, back_inserter(utf16line));
		
		CSlrString *strLine = new CSlrString(utf16line);
		strLine->RemoveEndLineCharacter();	// workaround for stupid Windows getline implementation
//		strLine->DebugPrint("line str=");
		
//		if (lineNum == 1)
//		{
//			LOGError("TODO: BUG: line without UTF8 header is imported without 4 letters");
//					strLine->DebugPrint("line str=");
//		}

		u16 tabChar = (u16)('\t');
		strLine->ReplaceCharacter(tabChar, strTabs);
		
		asmSourceFile->codeTextByLineNum.push_back(strLine);

		//		do not delete strLine, it is used

		lineNum++;
	}
	
	delete strTabs;
	
	LOGD("LoadSource done");
	

//	LOGD("LoadSource debug check");
//	asmSourceFile->sourceFilePath->DebugPrint("sourceFilePath=");
//	
//	for (int i = 0; i < 10; i++)
//	{
//		LOGD("         ========== printing source line %d", i);
//		asmSourceFile->codeTextByLineNum[i]->DebugPrint("");
//		
//	}
//	LOGD("check done");
	
}

CDebugAsmSourceBlock::CDebugAsmSourceBlock(CDebugSymbolsSegment *segment, CSlrString *name)
{
	this->segment = segment;
	this->name = name;
	
//	codeSourceLineByMemoryAddress = new C64AsmSourceLine* [symbols->maxMemoryAddress];
//	for (int i = 0; i < symbols->maxMemoryAddress; i++)
//	{
//		codeSourceLineByMemoryAddress[i] = NULL;
//	}
}

CDebugAsmSourceBlock::~CDebugAsmSourceBlock()
{
	name->DebugPrint("~C64AsmSourceBlock: ");
	delete name;
}

CDebugAsmSourceFile::~CDebugAsmSourceFile()
{
	sourceFileName->DebugPrint("~C64AsmSourceFile: ");
	
	delete sourceFilePath;
	delete sourceFileName;

	while (!codeTextByLineNum.empty())
	{
		CSlrString *t = codeTextByLineNum.back();
		codeTextByLineNum.pop_back();
		delete t;
	}

	while (!asmSourceLines.empty())
	{
		CDebugAsmSourceLine *l = asmSourceLines.back();
		asmSourceLines.pop_back();
		delete l;
	}
}
