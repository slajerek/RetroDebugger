#ifndef _CDebugSymbolsDataWatch_h_
#define _CDebugSymbolsDataWatch_h_

#include "SYS_Defs.h"
#include "CDebugSymbols.h"
#include "DebuggerDefs.h"
#include "CDebugMemoryMapping.h"
#include "hjson.h"

class CDebugSymbolsDataWatch
{
public:
	CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment);
	CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr);
	CDebugSymbolsDataWatch(CDebugSymbolsSegment *segment, char *name, int addr, int representation, int numberOfValues);
	virtual ~CDebugSymbolsDataWatch();
	
	CDebugSymbolsSegment *segment;
	
	void SetName(char *name);
	
	char *watchName;
	int address;
	
	int representation;
	int numberOfValues;

	// RD-059: the memory the watched bytes are in, captured when the watch is
	// added from the UI (BindToMemory): a watch on C64 RAM under the KERNAL,
	// on an Atari extended RAM bank or on a cartridge/PRG bank keeps reading
	// those bytes when the CPU has something else mapped at its address.
	// Without it (watch files, typed-in watches of older versions) the watch
	// reads what the CPU sees, as before.
	bool hasPhysical;
	CDebugPhysicalAddress physical;		// of the first byte; byte i at offset + i
	// Captures the memory at ADDRESS from the machine's resolver: where a
	// write lands when that is RAM (RAM under ROM included), else what a read
	// sees when it is ROM or cartridge memory; nothing for I/O. False when
	// there is no identity (no resolver, I/O, the 1541's symbols).
	bool BindToMemory();
	// Byte INDEX of the watch: from its memory when that is not mapped at
	// its CPU address now (*ELSEWHERE set), else through the CPU's view
	u8 ReadByte(int index, bool *elsewhere);
	// "c64-ram", "atari-xe-ram bank 2": where BindToMemory points
	std::string MemoryName() const;
	
	virtual void Serialize(Hjson::Value hjsonWatch);
	virtual void Deserialize(Hjson::Value hjsonWatch);

	static const char *RepresentationToStr(int representation);
	static const int StrToRepresentation(const char *str);
};


#endif
