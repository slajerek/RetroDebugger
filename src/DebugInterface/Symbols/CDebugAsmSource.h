#ifndef _CDEBUGASMSOURCE_H_
#define _CDEBUGASMSOURCE_H_

#include "SYS_Defs.h"
#include "CSlrString.h"
#include "CDebugBreakpoints.h"
#include <map>
#include <vector>

class CByteBuffer;
class CDebugInterface;

class CDebugAsmSourceBlock;
class CDebugSymbolsSegment;
class CDebugAsmSourceLine;
class CDebugAsmSource;
class CDebugSymbolsCodeLabel;
class CDataAdapter;
class CDebugSymbolsDataWatch;
class CSlrFile;

class CDebugAsmSourceFile
{
public:
	~CDebugAsmSourceFile();
	
	CSlrString *sourceFilePath;
	CSlrString *sourceFileName;
	int sourceId;
	
	std::vector<CSlrString *> codeTextByLineNum;
	
	// for deletion purposes:
	std::list<CDebugAsmSourceLine *> asmSourceLines;
};

class CDebugAsmSourceLine
{
public:
	CDebugAsmSourceFile *codeFile;
	CDebugAsmSourceBlock *block;
	
	int codeLineNumberStart;
	int codeColumnNumberStart;
	int codeLineNumberEnd;
	int codeColumnNumberEnd;
	
	u32 memoryAddressStart;
	u32 memoryAddressEnd;
};

class CDebugAsmSourceBlock
{
public:
	CDebugAsmSourceBlock(CDebugSymbolsSegment *segment, CSlrString *name);
	~CDebugAsmSourceBlock();
	CSlrString *name;

	CDebugSymbolsSegment *segment;
//	C64AsmSourceLine **codeSourceLineByMemoryAddress;
};


class CDebugAsmSource
{
public:
	CDebugAsmSource(CByteBuffer *byteBuffer, CDebugInterface *debugInterface, CDataAdapter *dataAdapter);
	~CDebugAsmSource();
	
	CDebugInterface *debugInterface;
	CDataAdapter *dataAdapter;
	
	void ParseXML(CByteBuffer *byteBuffer, CDebugInterface *debugInterface);
	void ParseOldFormat(CByteBuffer *byteBuffer, CDebugInterface *debugInterface);
	
	std::map<u32, CDebugAsmSourceFile *> codeSourceFilesById;
	
	int maxMemoryAddress;
	
	void LoadSource(CDebugAsmSourceFile *asmSourceFile, CSlrFile *file);

};

#endif
//_CDEBUGASMSOURCE_H_

