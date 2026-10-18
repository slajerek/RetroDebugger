#include "CDebugInfoLoader.h"
#include "CDebugDatabase.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugInterface.h"
#include "CDebugDataAdapter.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "CSlrFileFromOS.h"
#include "CGuiMain.h"
#include "DBG_Log.h"
#include "pugixml.h"
#include "hjson.h"
#include "std_membuf.h"
#include "utf8.h"
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <istream>
#include <list>
#include <memory>
#include <string>
#include <vector>

const char *DebugInfoFormatName(DebugInfoFormat format)
{
	switch (format)
	{
		case DebugInfoFormat::Unknown:		return "unknown";
		case DebugInfoFormat::KickAssDbg:	return "KickAssembler .dbg";
		case DebugInfoFormat::Cc65Dbg:		return "cc65 .dbg";
		case DebugInfoFormat::Oscar64Dbj:	return "Oscar64 .dbj";
		case DebugInfoFormat::ViceLabels:	return "VICE labels";
		case DebugInfoFormat::Tass64Labels:	return "64tass labels";
	}
	return "?";
}

static bool StartsWith(const char *text, int length, const char *prefix)
{
	int n = (int)strlen(prefix);
	return length >= n && memcmp(text, prefix, n) == 0;
}

static bool Contains(const char *text, int length, const char *needle)
{
	int n = (int)strlen(needle);
	for (int i = 0; i + n <= length; i++)
	{
		if (memcmp(text + i, needle, n) == 0)
			return true;
	}
	return false;
}

DebugInfoFormat DebugInfoProbe(const u8 *data, int length)
{
	if (data == NULL || length <= 0)
		return DebugInfoFormat::Unknown;

	const char *text = (const char *)data;
	// Only the head of a file decides; skip a UTF-8 BOM and leading blanks.
	int head = length < 4096 ? length : 4096;
	if (head >= 3 && (u8)text[0] == 0xEF && (u8)text[1] == 0xBB && (u8)text[2] == 0xBF)
	{
		text += 3;
		head -= 3;
	}
	while (head > 0 && (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n'))
	{
		text++;
		head--;
	}

	if (StartsWith(text, head, "<C64debugger")
		|| (StartsWith(text, head, "<?xml") && Contains(text, head, "<C64debugger")))
		return DebugInfoFormat::KickAssDbg;
	if (StartsWith(text, head, "version\tmajor="))
		return DebugInfoFormat::Cc65Dbg;
	if (StartsWith(text, head, "{") && (Contains(text, head, "\"memory\"") || Contains(text, head, "\"functions\"")))
		return DebugInfoFormat::Oscar64Dbj;

	// Label files: the first line that is not empty and not a comment decides.
	int pos = 0;
	while (pos < head)
	{
		int end = pos;
		while (end < head && text[end] != '\n')
			end++;
		const char *line = text + pos;
		int lineLength = end - pos;
		while (lineLength > 0 && (*line == ' ' || *line == '\t'))
		{
			line++;
			lineLength--;
		}
		if (lineLength > 0 && line[0] != '#' && line[0] != ';' && line[0] != '\r')
		{
			if (lineLength >= 5 && (line[0] == 'a' || line[0] == 'A') && (line[1] == 'l' || line[1] == 'L')
				&& (line[2] == ' ' || line[2] == '\t'))
				return DebugInfoFormat::ViceLabels;
			if (memchr(line, '=', lineLength) != NULL)
				return DebugInfoFormat::Tass64Labels;
			return DebugInfoFormat::Unknown;
		}
		pos = end + 1;
	}
	return DebugInfoFormat::Unknown;
}

DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	DebugInfoFormat format = DebugInfoLoadFile(symbols, cPath, CDebugInfoLoadOptions());
	delete [] cPath;
	return format;
}

DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, const char *path)
{
	return DebugInfoLoadFile(symbols, path, CDebugInfoLoadOptions());
}

DebugInfoFormat DebugInfoLoadFile(CDebugSymbols *symbols, const char *cPath, const CDebugInfoLoadOptions &loadOptions)
{
	CSlrFileFromOS *file = new CSlrFileFromOS((char *)cPath);
	if (!file->Exists())
	{
		LOGError("DebugInfoLoadFile: %s does not exist", cPath);
		delete file;
		return DebugInfoFormat::Unknown;
	}
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	delete file;

	DebugInfoFormat format = DebugInfoProbe(byteBuffer->data, byteBuffer->length);
	LOGM("DebugInfoLoadFile: %s is %s", cPath, DebugInfoFormatName(format));

	CDebugInfoLoadOptions options = loadOptions;
	if (options.debugFileDir.empty())
	{
		std::string dir(cPath);
		size_t slash = dir.find_last_of("/\\");
		options.debugFileDir = (slash == std::string::npos) ? std::string(".") : dir.substr(0, slash);
	}

	switch (format)
	{
		case DebugInfoFormat::KickAssDbg:
			DebugInfoLoadKickAss(symbols, byteBuffer, options);
			break;
		case DebugInfoFormat::Oscar64Dbj:
			DebugInfoLoadOscar64(symbols, byteBuffer, options);
			break;
		case DebugInfoFormat::ViceLabels:
		case DebugInfoFormat::Tass64Labels:
			symbols->ParseSymbols(byteBuffer);
			break;
		case DebugInfoFormat::Cc65Dbg:
			DebugInfoLoadCc65(symbols, byteBuffer, options);
			break;
		case DebugInfoFormat::Unknown:
			LOGError("DebugInfoLoadFile: %s is not a recognized symbols or debug info format", cPath);
			break;
	}

	if (format == DebugInfoFormat::KickAssDbg || format == DebugInfoFormat::Oscar64Dbj || format == DebugInfoFormat::Cc65Dbg)
	{
		// the program is not known here; the autoload next to it adds that check
		symbols->debugInfoPath = cPath;
		symbols->debugInfoWarnings = DebugInfoCheckStale(symbols->debugDatabase.get(), cPath, NULL);
		for (const std::string &warning : symbols->debugInfoWarnings)
			LOGWarning("DebugInfoLoadFile: %s", warning.c_str());
	}

	delete byteBuffer;
	return format;
}

// Reads a source file's text, 1-based, tabs expanded to four spaces.
static bool LoadSourceText(CDebugSourceFile *sourceFile, const std::string &path)
{
	CSlrFileFromOS *file = new CSlrFileFromOS((char *)path.c_str());
	if (!file->Exists())
	{
		delete file;
		return false;
	}
	CByteBuffer *byteBuffer = file->GetWholeFileAsByteBuffer();
	delete file;

	std_membuf mb(byteBuffer->data, byteBuffer->length);
	std::istream reader(&mb);
	std::string line;

	sourceFile->lines.push_back(new CSlrString(""));
	CSlrString *strTabs = new CSlrString("    ");
	unsigned lineNum = 1;
	while (getline(reader, line))
	{
		std::string::iterator endIt = utf8::find_invalid(line.begin(), line.end());
		if (endIt != line.end())
			LOGError("LoadSourceText: invalid UTF-8 in %s line %u", path.c_str(), lineNum);

		std::vector<unsigned short> utf16line;
		utf8::utf8to16(line.begin(), endIt, back_inserter(utf16line));
		CSlrString *strLine = new CSlrString(utf16line);
		strLine->RemoveEndLineCharacter();
		strLine->ReplaceCharacter((u16)'\t', strTabs);
		sourceFile->lines.push_back(strLine);
		lineNum++;
	}
	delete strTabs;
	delete byteBuffer;
	sourceFile->loadedPath = path;
	return true;
}

// Loads SOURCEFILE's text from the first place that has it: remapped path,
// recorded path, or its file name in the debug file's directory.
static void LoadSourceTextFor(CDebugSourceFile *sourceFile, const CDebugInfoLoadOptions &options)
{
	const std::string &path = sourceFile->originalPath;
	for (const auto &remap : options.pathRemaps)
	{
		if (!remap.first.empty() && path.compare(0, remap.first.size(), remap.first) == 0
			&& LoadSourceText(sourceFile, remap.second + path.substr(remap.first.size())))
			return;
	}
	// A relative path is relative to the build, i.e. to the debug file, not to
	// this process's working directory.
	bool relative = !path.empty() && path[0] != '/' && path[0] != '\\'
		&& !(path.size() > 1 && path[1] == ':');
	if (relative && !options.debugFileDir.empty() && LoadSourceText(sourceFile, options.debugFileDir + "/" + path))
		return;
	if (LoadSourceText(sourceFile, path))
		return;
	if (!options.debugFileDir.empty() && !sourceFile->fileName.empty())
		LoadSourceText(sourceFile, options.debugFileDir + "/" + sourceFile->fileName);
}

static std::string ToStd(CSlrString *str)
{
	char *ascii = str->GetStdASCII();
	std::string s(ascii);
	delete [] ascii;
	return s;
}

//	"   &quot;
//	'   &apos;
//	<   &lt;
//	>   &gt;
//	&   &amp;

bool DebugInfoLoadKickAss(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const char *debugFileDir)
{
	CDebugInfoLoadOptions options;
	if (debugFileDir != NULL)
		options.debugFileDir = debugFileDir;
	return DebugInfoLoadKickAss(symbols, byteBuffer, options);
}

bool DebugInfoLoadKickAss(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const CDebugInfoLoadOptions &options)
{
	LOGD("DebugInfoLoadKickAss");

	byteBuffer->removeCRLFinQuotations();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer(byteBuffer->data, byteBuffer->length);
	if (!result)
	{
		LOGError("Loading debug symbols failed: %s at %d", result.description(), result.offset);
		return false;
	}

	pugi::xml_node rootTag = doc.first_child();
	pugi::xml_attribute rootAttr = rootTag.first_attribute();
	if (strcmp(rootAttr.name(), "version"))
	{
		LOGError("Version tag mismatch (%s)", rootAttr.name());
		return false;
	}
	if (strcmp(rootAttr.value(), "1.0"))
	{
		LOGError("Unknown version of debug symbols (%s)", rootAttr.value());
		return false;
	}

	pugi::xml_node nodeSources = rootTag.child("Sources");
	if (nodeSources.empty())
	{
		LOGError("Sources list not found");
		return false;
	}

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

	std::shared_ptr<CDebugDatabase> database = std::make_shared<CDebugDatabase>("KickAssembler");
	database->capabilities.lineMappings = true;
	database->capabilities.columns = true;

	// == Sources: "INDEX,FILE"; SplitWithChars keeps the separators as words
	CSlrString *strSources = new CSlrString(nodeSources.child_value());
	std::vector<CSlrString *> *lines = strSources->SplitWithChars(splitCharsLine);
	for (CSlrString *oneLine : *lines)
	{
		if (oneLine->GetLength() < 2)
			continue;
		oneLine->RemoveFromBeginningSelectedCharacter(' ');
		oneLine->RemoveFromBeginningSelectedCharacter('\t');

		std::vector<CSlrString *> *words = oneLine->SplitWithChars(splitCharsComma);
		if (words->size() == 3)
		{
			database->AddFile((*words)[0]->ToInt(), ToStd((*words)[2]));
		}
		else if (words->size() != 0)
		{
			LOGError("Wrong format for sources list (%d words)", words->size());
		}
		CSlrString::DeleteVector(words);
	}
	CSlrString::DeleteVector(lines);
	delete strSources;

	// Everything below replaces SYMBOLS' contents: lock as the old loader did.
	guiMain->LockMutex();
	symbols->debugInterface->LockMutex();

	// A .dbg defines the segment set. The old loader appended to whatever was
	// there, so an import from the menu (which, unlike autoload and the
	// command line, did not clear first) left stale segments of the same name
	// that FindSegment then preferred (RD-032).
	symbols->DeleteAllSymbols();

	int maxAddress = symbols->dataAdapter->AdapterGetDataLength();

	// == Segments: "START,END,FILE_IDX,LINE1,COL1,LINE2,COL2" per <Block>, END inclusive
	int segmentNum = 0;
	for (pugi::xml_node segmentNode = rootTag.child("Segment"); segmentNode; segmentNode = segmentNode.next_sibling("Segment"))
	{
		const char *segmentNameStr = segmentNode.attribute("name").value();
		u32 regionId = database->AddRegion(segmentNameStr, segmentNode.attribute("dest").value());

		CDebugSymbolsSegment *segment = symbols->CreateNewDebugSymbolsSegment(new CSlrString(segmentNameStr), segmentNum++);
		segment->debugRegionId = (int)regionId;
		symbols->segments.push_back(segment);

		for (pugi::xml_node blockNode = segmentNode.child("Block"); blockNode; blockNode = blockNode.next_sibling("Block"))
		{
			u32 blockId = database->AddBlock(blockNode.attribute("name").value(), regionId);

			CSlrString *strBlock = new CSlrString(blockNode.child_value());
			std::vector<CSlrString *> *blockLines = strBlock->SplitWithChars(splitCharsLine);
			for (CSlrString *oneLine : *blockLines)
			{
				if (oneLine->GetLength() < 2)
					continue;
				oneLine->RemoveFromBeginningSelectedCharacter(' ');
				oneLine->RemoveFromBeginningSelectedCharacter('\t');

				std::vector<CSlrString *> *words = oneLine->SplitWithChars(splitCharsComma);
				if (words->size() == 13)
				{
					int start = (*words)[0]->ToIntFromHex();
					int endInclusive = (*words)[2]->ToIntFromHex();
					u32 fileId = (*words)[4]->ToInt();

					if (database->GetFile(fileId) == NULL)
					{
						LOGError("Source code id #%d not found", fileId);
					}
					else if (start < 0 || start >= maxAddress || endInclusive < start)
					{
						LOGError("Address range %04x-%04x is out of memory range", start, endInclusive);
					}
					else
					{
						if (endInclusive >= maxAddress)
						{
							LOGError("Address %04x is out of memory range", maxAddress);
							endInclusive = maxAddress - 1;
						}
						CDebugSourceMapping mapping;
						mapping.start = (u32)start;
						mapping.end = (u32)endInclusive + 1;
						mapping.regionId = regionId;
						mapping.blockId = blockId;
						mapping.fileId = fileId;
						mapping.lineStart = (*words)[6]->ToInt();
						mapping.columnStart = (*words)[8]->ToInt();
						mapping.lineEnd = (*words)[10]->ToInt();
						mapping.columnEnd = (*words)[12]->ToInt();
						mapping.quality = DebugMappingQuality::Exact;
						database->AddMapping(mapping);
					}
				}
				else if (words->size() != 0)
				{
					LOGError("Wrong format for source code address mapping");
				}
				CSlrString::DeleteVector(words);
			}
			CSlrString::DeleteVector(blockLines);
			delete strBlock;
		}
	}

	if (symbols->segments.empty())
	{
		// a symbols scope must always exist (symbols->currentSegment)
		symbols->CreateDefaultSegment();
	}

	// == Labels: "SEGMENT,ADDRESS,NAME[,...]"
	pugi::xml_node nodeLabels = rootTag.child("Labels");
	if (!nodeLabels.empty())
	{
		CSlrString *strLabels = new CSlrString(nodeLabels.child_value());
		std::vector<CSlrString *> *labelLines = strLabels->SplitWithChars(splitCharsLine);
		for (CSlrString *oneLine : *labelLines)
		{
			if (oneLine->GetLength() < 2)
				continue;
			oneLine->RemoveFromBeginningSelectedCharacter(' ');
			oneLine->RemoveFromBeginningSelectedCharacter('\t');

			std::vector<CSlrString *> *words = oneLine->Split(splitCharsComma);
			if (words->size() == 0)
			{
				CSlrString::DeleteVector(words);
				continue;
			}
			// KickAssembler added code-line columns after NAME; only the first three matter here
			if (words->size() < 3)
			{
				LOGError("Not enough words for label definition (%d words)", words->size());
				CSlrString::DeleteVector(words);
				continue;
			}

			CDebugSymbolsSegment *segment = symbols->FindSegment((*words)[0]);
			if (segment == NULL)
			{
				LOGError("ParseLabels: segment not found");
				CSlrString::DeleteVector(words);
				continue;
			}
			int address = (*words)[1]->ToIntFromHex();
			char *labelNameStr = (*words)[2]->GetStdASCII();
			if (labelNameStr[0] != 0)
			{
				segment->AddCodeLabel(address, labelNameStr);	// copies the text
			}
			delete [] labelNameStr;
			CSlrString::DeleteVector(words);
		}
		CSlrString::DeleteVector(labelLines);
		delete strLabels;
	}

	// == Watchpoints. Two layouts, told apart by the values attribute:
	//    old  "SEGMENT,ADDRESS[,NUMBER_OF_VALUES[,REPRESENTATION]]"
	//    new  "SEGMENT,ADDRESS1,ADDRESS2,ARGUMENT" (KickAssembler .watch: a range,
	//         its size is the number of values; ARGUMENT is not a representation)
	pugi::xml_node nodeWatches = rootTag.child("Watchpoints");
	bool watchRangeLayout = strstr(nodeWatches.attribute("values").value(), "ADDRESS2") != NULL;
	if (!nodeWatches.empty())
	{
		CSlrString *strWatches = new CSlrString(nodeWatches.child_value());
		std::vector<CSlrString *> *watchLines = strWatches->SplitWithChars(splitCharsLine);
		for (CSlrString *oneLine : *watchLines)
		{
			if (oneLine->GetLength() < 2)
				continue;
			oneLine->RemoveFromBeginningSelectedCharacter(' ');
			oneLine->RemoveFromBeginningSelectedCharacter('\t');

			std::vector<CSlrString *> *words = oneLine->Split(splitCharsComma);
			if (words->size() == 0)
			{
				CSlrString::DeleteVector(words);
				continue;
			}
			if (words->size() < 2)
			{
				LOGError("Wrong format for watches (%d words)", words->size());
				CSlrString::DeleteVector(words);
				continue;
			}

			int address = (*words)[1]->ToIntFromHex();
			int numberOfValues = 1;
			CSlrString *strRepresentation = NULL;
			if (watchRangeLayout)
			{
				// ADDRESS2 may be empty ("Default,$c000,,store") and Split drops
				// empty fields, so only a $-address in the third place is ADDRESS2
				if (words->size() > 2 && (*words)[2]->GetLength() > 1 && (*words)[2]->GetChar(0) == '$')
				{
					int address2 = (*words)[2]->ToIntFromHex();
					if (address2 > address)
						numberOfValues = address2 - address + 1;
				}
			}
			else
			{
				if (words->size() > 2)
				{
					numberOfValues = (*words)[2]->ToInt();
					if (numberOfValues < 1)
						numberOfValues = 1;
				}
				if (words->size() > 3)
				{
					strRepresentation = (*words)[3];
				}
			}

			CDebugSymbolsSegment *segment = symbols->FindSegment((*words)[0]);
			if (segment == NULL)
			{
				LOGError("ParseWatches: segment not found");
			}
			else
			{
				segment->AddWatch(address, numberOfValues, strRepresentation);
			}
			CSlrString::DeleteVector(words);
		}
		CSlrString::DeleteVector(watchLines);
		delete strWatches;
	}

	// == Breakpoints: "SEGMENT,ADDRESS,ARGUMENT"
	pugi::xml_node nodeBreakpoints = rootTag.child("Breakpoints");
	if (!nodeBreakpoints.empty())
	{
		CSlrString *strBreakpoints = new CSlrString(nodeBreakpoints.child_value());
		std::vector<CSlrString *> *breakpointLines = strBreakpoints->SplitWithChars(splitCharsLine);
		for (CSlrString *oneLine : *breakpointLines)
		{
			if (oneLine->GetLength() < 2)
				continue;
			oneLine->RemoveFromBeginningSelectedCharacter(' ');
			oneLine->RemoveFromBeginningSelectedCharacter('\t');

			std::vector<CSlrString *> *breakpointWords = oneLine->SplitWithChars(splitCharsComma);
			if (breakpointWords->size() < 3)
			{
				if (breakpointWords->size() != 0)
					LOGError("ParseBreakpoints: unknown format (num elements=%d)", breakpointWords->size());
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}

			CDebugSymbolsSegment *segment = symbols->FindSegment((*breakpointWords)[0]);
			int breakpointAddress = (*breakpointWords)[2]->ToIntFromHex();
			if (segment == NULL)
			{
				LOGError("ParseBreakpoints: segment not found");
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}

			// "SEGMENT,ADDRESS," -- a plain PC breakpoint
			if (breakpointWords->size() == 4)
			{
				segment->AddBreakpointPC(breakpointAddress);
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}
			if (breakpointWords->size() != 5)
			{
				LOGError("ParseBreakpoints: unknown format (num elements=%d)", breakpointWords->size());
				CSlrString::DeleteVector(breakpointWords);
				continue;
			}

			std::vector<CSlrString *> *words = (*breakpointWords)[4]->SplitWithChars(splitCharsBreakpoints);
			CSlrString *command = words->empty() ? NULL : (*words)[0];
			bool parsed = true;

			if (command == NULL || command->GetChar(0) == '#')
			{
				// empty or a comment
			}
			else
			{
				command->ConvertToLowerCase();
				if (command->Equals("break") || command->Equals("breakpc") || command->Equals("breakonpc"))
				{
					segment->AddBreakpointPC(breakpointAddress);
				}
				else if (command->Equals("setbkg") || command->Equals("setbackground"))
				{
					if (words->size() < 3)
					{
						LOGError("ParseBreakpoints: error with setbkg");
						parsed = false;
					}
					else
					{
						segment->AddBreakpointSetBackground(breakpointAddress, (*words)[2]->ToIntFromHex());
					}
				}
				else if (command->Equals("breakmemory") || command->Equals("breakonmemory") || command->Equals("breakmem")
						 || command->Equals("mem"))
				{
					if (words->size() < 4)
					{
						LOGError("ParseBreakpoints: error with breakmemory");
						parsed = false;
					}
					else
					{
						int address = (*words)[2]->ToIntFromHex();
						int index = 3;
						CSlrString *op = new CSlrString();
						while (index < (int)words->size() - 1)
						{
							CSlrString *f = (*words)[index];
							f->ConvertToLowerCase();
							u16 chr = f->GetChar(0);
							if (chr == ' ')
							{
								index++;
								continue;
							}
							if ((chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f'))
								break;
							op->Concatenate(f);
							index++;
						}

						DataBreakpointComparison comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
						if (index >= (int)words->size())
						{
							LOGError("ParseBreakpoints: error");
							parsed = false;
						}
						else if (op->Equals("==") || op->Equals("="))
							comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
						else if (op->Equals("!="))
							comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_NOT_EQUAL;
						else if (op->Equals("<"))
							comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_LESS;
						else if (op->Equals("<=") || op->Equals("=<"))
							comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_LESS_OR_EQUAL;
						else if (op->Equals(">"))
							comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER;
						else if (op->Equals(">=") || op->Equals("=>"))
							comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_GREATER_OR_EQUAL;
						else
						{
							LOGError("ParseBreakpoints: unknown operator for memory breakpoint");
							parsed = false;
						}

						if (parsed)
						{
							int value = (*words)[index]->ToIntFromHex();
							segment->AddBreakpointMemory(address, MEMORY_BREAKPOINT_ACCESS_WRITE, comparison, value);
						}
						delete op;
					}
				}
				else if (segment->ParseSymbolsXML(command, words))
				{
					// segment-specific (raster line, IRQs, ...)
				}
				else
				{
					LOGError("ParseBreakpoints: unknown breakpoint type");
					parsed = false;
				}
			}

			CSlrString::DeleteVector(words);
			CSlrString::DeleteVector(breakpointWords);
			if (!parsed)
				break;	// as the old loader: stop at the first malformed breakpoint
		}
		CSlrString::DeleteVector(breakpointLines);
		delete strBreakpoints;
	}

	// == Source text
	for (auto &it : database->Files())
		LoadSourceTextFor(it.second, options);

	database->Finalize();
	symbols->SetDebugDatabase(database);

	symbols->ActivateSelectedSegment();

	symbols->debugInterface->UnlockMutex();
	guiMain->UnlockMutex();

	LOGM("DebugInfoLoadKickAss: %d files, %d segments, %d mappings", (int)database->NumFiles(),
		 (int)database->NumRegions(), (int)database->NumMappings());
	return true;
}

bool DebugInfoLoadOscar64(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const CDebugInfoLoadOptions &options)
{
	Hjson::Value root;
	try
	{
		root = Hjson::Unmarshal((const char *)byteBuffer->data, (size_t)byteBuffer->length);
	}
	catch (const std::exception &e)
	{
		LOGError("DebugInfoLoadOscar64: not valid JSON: %s", e.what());
		return false;
	}
	Hjson::Value functions = root["functions"];
	if (functions.type() != Hjson::Type::Vector)
	{
		LOGError("DebugInfoLoadOscar64: no \"functions\" array");
		return false;
	}

	std::shared_ptr<CDebugDatabase> database = std::make_shared<CDebugDatabase>("Oscar64");
	database->capabilities.lineMappings = true;
	database->capabilities.functions = true;
	u32 regionId = database->AddRegion("Default", "");
	int maxAddress = symbols->dataAdapter->AdapterGetDataLength();

	// Oscar64 names files by path in every entry; number them in order of appearance.
	std::map<std::string, u32> fileIds;
	auto fileIdFor = [&](const std::string &path) -> u32 {
		auto it = fileIds.find(path);
		if (it != fileIds.end())
			return it->second;
		u32 id = (u32)fileIds.size();
		fileIds[path] = id;
		database->AddFile(id, path);
		return id;
	};

	struct Label { u32 address; std::string name; };
	std::vector<Label> labels;

	for (size_t i = 0; i < functions.size(); i++)
	{
		const Hjson::Value &f = functions[(int)i];
		CDebugFunction function;
		function.name = f["name"].to_string();
		function.signature = f["xname"].to_string();
		function.start = (u32)f["start"].to_int64();
		function.end = (u32)f["end"].to_int64();
		function.regionId = regionId;
		function.fileId = fileIdFor(f["source"].to_string());
		function.line = (i32)f["line"].to_int64();
		if (function.start >= (u32)maxAddress)
		{
			LOGError("DebugInfoLoadOscar64: %s at %x is out of memory range", function.name.c_str(), function.start);
			continue;
		}
		if (function.end > (u32)maxAddress)
			function.end = (u32)maxAddress;
		database->AddFunction(function);
		labels.push_back(Label{ function.start, function.name });

		u32 blockId = database->AddBlock(function.name, regionId);
		const Hjson::Value &lines = f["lines"];
		for (size_t l = 0; l < lines.size(); l++)
		{
			const Hjson::Value &line = lines[(int)l];
			CDebugSourceMapping mapping;
			mapping.start = (u32)line["start"].to_int64();
			mapping.end = (u32)line["end"].to_int64();
			if (mapping.end > (u32)maxAddress)
				mapping.end = (u32)maxAddress;
			mapping.regionId = regionId;
			mapping.blockId = blockId;
			mapping.fileId = fileIdFor(line["source"].to_string());
			mapping.lineStart = (i32)line["line"].to_int64();
			mapping.lineEnd = mapping.lineStart;
			mapping.quality = DebugMappingQuality::Exact;
			if (mapping.start < mapping.end)
				database->AddMapping(mapping);
		}
	}

	Hjson::Value variables = root["variables"];
	for (size_t i = 0; i < variables.size(); i++)
	{
		const Hjson::Value &v = variables[(int)i];
		u32 start = (u32)v["start"].to_int64();
		if (start < (u32)maxAddress)
			labels.push_back(Label{ start, v["name"].to_string() });
	}

	for (auto &it : database->Files())
		LoadSourceTextFor(it.second, options);
	database->Finalize();

	guiMain->LockMutex();
	symbols->debugInterface->LockMutex();

	symbols->DeleteAllSymbols();
	CDebugSymbolsSegment *segment = symbols->CreateNewDebugSymbolsSegment(new CSlrString("Default"), 0);
	segment->debugRegionId = (int)regionId;
	symbols->segments.push_back(segment);
	for (const Label &label : labels)
	{
		if (!label.name.empty())
			segment->AddCodeLabel((int)label.address, (char *)label.name.c_str());	// copies the text
	}

	symbols->SetDebugDatabase(database);
	symbols->ActivateSelectedSegment();

	symbols->debugInterface->UnlockMutex();
	guiMain->UnlockMutex();

	LOGM("DebugInfoLoadOscar64: %d files, %d functions, %d mappings", (int)database->NumFiles(),
		 (int)database->Functions().size(), (int)database->NumMappings());
	return true;
}

// One cc65 .dbg record: "kind<TAB>key=value,key="quoted, value",...".
struct Cc65Record
{
	std::string kind;
	std::map<std::string, std::string> fields;

	bool Has(const char *key) const { return fields.find(key) != fields.end(); }
	std::string Get(const char *key) const
	{
		auto it = fields.find(key);
		return it == fields.end() ? std::string() : it->second;
	}
	long Num(const char *key, long fallback = -1) const
	{
		auto it = fields.find(key);
		return (it == fields.end() || it->second.empty()) ? fallback : strtol(it->second.c_str(), NULL, 0);
	}
	// "12+13+40" -> {12, 13, 40}
	std::vector<long> List(const char *key) const
	{
		std::vector<long> out;
		std::string value = Get(key);
		size_t pos = 0;
		while (pos < value.size())
		{
			size_t plus = value.find('+', pos);
			if (plus == std::string::npos)
				plus = value.size();
			out.push_back(strtol(value.substr(pos, plus - pos).c_str(), NULL, 0));
			pos = plus + 1;
		}
		return out;
	}
};

static bool ParseCc65Record(const std::string &line, Cc65Record &record)
{
	size_t tab = line.find('\t');
	if (tab == std::string::npos)
		return false;
	record.kind = line.substr(0, tab);
	record.fields.clear();
	size_t pos = tab + 1;
	while (pos < line.size())
	{
		size_t eq = line.find('=', pos);
		if (eq == std::string::npos)
			return false;
		std::string key = line.substr(pos, eq - pos);
		pos = eq + 1;
		std::string value;
		if (pos < line.size() && line[pos] == '"')
		{
			size_t close = line.find('"', pos + 1);
			if (close == std::string::npos)
				return false;
			value = line.substr(pos + 1, close - pos - 1);
			pos = close + 1;
		}
		else
		{
			size_t comma = line.find(',', pos);
			if (comma == std::string::npos)
				comma = line.size();
			value = line.substr(pos, comma - pos);
			pos = comma;
		}
		record.fields[key] = value;
		if (pos < line.size() && line[pos] == ',')
			pos++;
	}
	return true;
}

static bool IsCc65GeneratedLabel(const std::string &name)
{
	// cc65's code generator names its jump targets Lxxxx (hex)
	if (name.size() != 5 || name[0] != 'L')
		return false;
	for (size_t i = 1; i < name.size(); i++)
	{
		if (!isxdigit((unsigned char)name[i]))
			return false;
	}
	return true;
}

bool DebugInfoLoadCc65(CDebugSymbols *symbols, CByteBuffer *byteBuffer, const CDebugInfoLoadOptions &options)
{
	std::map<long, Cc65Record> files, segs, spans, scopes, syms;
	std::vector<Cc65Record> lines, csyms;

	std::string text((const char *)byteBuffer->data, (size_t)byteBuffer->length);
	size_t pos = 0;
	int lineNum = 0;
	while (pos < text.size())
	{
		size_t end = text.find('\n', pos);
		if (end == std::string::npos)
			end = text.size();
		std::string line = text.substr(pos, end - pos);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		pos = end + 1;
		lineNum++;

		Cc65Record record;
		if (line.empty() || !ParseCc65Record(line, record))
			continue;
		if (record.kind == "version" && record.Num("major") != 2)
		{
			LOGError("DebugInfoLoadCc65: unsupported version major=%ld", record.Num("major"));
			return false;
		}
		long id = record.Num("id");
		if (record.kind == "file")			files[id] = record;
		else if (record.kind == "seg")		segs[id] = record;
		else if (record.kind == "span")		spans[id] = record;
		else if (record.kind == "scope")	scopes[id] = record;
		else if (record.kind == "sym")		syms[id] = record;
		else if (record.kind == "line")		lines.push_back(record);
		else if (record.kind == "csym")		csyms.push_back(record);
	}

	std::shared_ptr<CDebugDatabase> database = std::make_shared<CDebugDatabase>("cc65");
	database->capabilities.lineMappings = true;
	database->capabilities.functions = true;
	u32 regionId = database->AddRegion("Default", "");
	u32 blockId = database->AddBlock("", regionId);
	int maxAddress = symbols->dataAdapter->AdapterGetDataLength();

	for (auto &it : files)
		database->AddFile((u32)it.first, it.second.Get("name"));

	// address range of a span: segment start + span start, size bytes
	auto spanRange = [&](long spanId, u32 &start, u32 &end) -> bool {
		auto span = spans.find(spanId);
		if (span == spans.end())
			return false;
		auto seg = segs.find(span->second.Num("seg"));
		if (seg == segs.end())
			return false;
		long size = span->second.Num("size", 0);
		if (size <= 0)
			return false;
		start = (u32)(seg->second.Num("start", 0) + span->second.Num("start", 0));
		end = start + (u32)size;
		return start < (u32)maxAddress;
	};

	// asm (type 0) and macro (type 2) lines first, C (type 1) last: later wins
	std::map<u32, std::pair<u32, i32>> cLineAtAddress;	// start -> (file, line), for function lines
	for (int pass = 0; pass < 2; pass++)
	{
		for (const Cc65Record &line : lines)
		{
			bool isC = line.Num("type", 0) == 1;
			if (isC != (pass == 1))
				continue;
			for (long spanId : line.List("span"))
			{
				CDebugSourceMapping mapping;
				if (!spanRange(spanId, mapping.start, mapping.end))
					continue;
				if (mapping.end > (u32)maxAddress)
					mapping.end = (u32)maxAddress;
				mapping.regionId = regionId;
				mapping.blockId = blockId;
				mapping.fileId = (u32)line.Num("file", 0);
				mapping.lineStart = (i32)line.Num("line", 0);
				mapping.lineEnd = mapping.lineStart;
				mapping.quality = DebugMappingQuality::Exact;
				database->AddMapping(mapping);
				if (isC)
					cLineAtAddress[mapping.start] = std::make_pair(mapping.fileId, mapping.lineStart);
			}
		}
	}

	// Functions: a C symbol with external linkage names an asm symbol, whose
	// scope ("_add") spans the function's code.
	for (const Cc65Record &csym : csyms)
	{
		if (csym.Get("sc") != "ext" || !csym.Has("sym"))
			continue;
		long symId = csym.Num("sym");
		for (auto &it : scopes)
		{
			const Cc65Record &scope = it.second;
			if (scope.Num("sym") != symId || scope.Get("type") != "scope")
				continue;
			CDebugFunction function;
			function.name = csym.Get("name");
			function.signature = scope.Get("name");
			function.regionId = regionId;
			bool any = false;
			for (long spanId : scope.List("span"))
			{
				u32 start, end;
				if (!spanRange(spanId, start, end))
					continue;
				if (!any || start < function.start)
					function.start = start;
				if (!any || end > function.end)
					function.end = end;
				any = true;
			}
			if (!any)
				continue;
			// the function's line: the C line mapped at its entry, when there is one
			auto entry = cLineAtAddress.find(function.start);
			if (entry != cLineAtAddress.end())
			{
				function.fileId = entry->second.first;
				function.line = entry->second.second;
			}
			database->AddFunction(function);
		}
	}

	struct Label { u32 address; std::string name; };
	std::vector<Label> labels;
	for (auto &it : syms)
	{
		const Cc65Record &sym = it.second;
		std::string name = sym.Get("name");
		if (sym.Get("type") != "lab" || name.empty() || IsCc65GeneratedLabel(name))
			continue;
		long value = sym.Num("val");
		if (value >= 0 && value < maxAddress)
			labels.push_back(Label{ (u32)value, name });
	}

	for (auto &it : database->Files())
		LoadSourceTextFor(it.second, options);
	database->Finalize();

	guiMain->LockMutex();
	symbols->debugInterface->LockMutex();

	symbols->DeleteAllSymbols();
	CDebugSymbolsSegment *segment = symbols->CreateNewDebugSymbolsSegment(new CSlrString("Default"), 0);
	segment->debugRegionId = (int)regionId;
	symbols->segments.push_back(segment);
	for (const Label &label : labels)
		segment->AddCodeLabel((int)label.address, (char *)label.name.c_str());	// copies the text

	symbols->SetDebugDatabase(database);
	symbols->ActivateSelectedSegment();

	symbols->debugInterface->UnlockMutex();
	guiMain->UnlockMutex();

	LOGM("DebugInfoLoadCc65: %d files, %d functions, %d mappings", (int)database->NumFiles(),
		 (int)database->Functions().size(), (int)database->NumMappings());
	return true;
}

// RD-014: stale debug info, by file time (see the header).
static bool FileTime(const std::string &path, std::filesystem::file_time_type *time)
{
	std::error_code error;
	*time = std::filesystem::last_write_time(std::filesystem::u8path(path), error);
	return !error;
}

static std::string FileNameOf(const std::string &path)
{
	size_t slash = path.find_last_of("/\\");
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::vector<std::string> DebugInfoCheckStale(const CDebugDatabase *database, const char *debugFilePath,
											 const char *programPath)
{
	std::vector<std::string> reasons;
	std::filesystem::file_time_type debugTime;
	if (!debugFilePath || !FileTime(debugFilePath, &debugTime))
		return reasons;
	std::string debugName = FileNameOf(debugFilePath);

	if (database)
	{
		for (auto &it : database->Files())
		{
			const CDebugSourceFile *file = it.second;
			std::filesystem::file_time_type sourceTime;
			if (file->loadedPath.empty() || !FileTime(file->loadedPath, &sourceTime))
				continue;
			if (sourceTime > debugTime + std::chrono::seconds(2))
				reasons.push_back(file->fileName + " changed after " + debugName + " was written: rebuild, lines may not match the code");
		}
	}

	std::filesystem::file_time_type programTime;
	if (programPath && FileTime(programPath, &programTime))
	{
		std::string programName = FileNameOf(programPath);
		if (programTime > debugTime + std::chrono::seconds(kDebugInfoBuildSkewSeconds))
			reasons.push_back(programName + " is newer than " + debugName + ": the debug info is from an older build");
		else if (debugTime > programTime + std::chrono::seconds(kDebugInfoBuildSkewSeconds))
			reasons.push_back(debugName + " is newer than " + programName + ": the program is from an older build");
	}
	return reasons;
}
