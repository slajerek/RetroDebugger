#ifndef _CDEBUGSYMBOLS_H_
#define _CDEBUGSYMBOLS_H_

#include "SYS_Defs.h"
#include "hjson.h"
#include <atomic>
#include <list>
#include <memory>
#include <string>
#include <vector>

class CSlrFile;
class CByteBuffer;
class CSlrString;
class CDebugInterface;
class CDebugDatabase;
class CDebugSymbolsSegment;
class CDebugSymbolsCodeLabel;
class CDebugDataAdapter;
class CDebugEventsHistory;
class CDebugMemoryCell;
class CDebugMemory;
class CDebugSourceBreakpoint;
class CDebugSourceWatch;

#define MAX_LABEL_TEXT_BUFFER_SIZE	512

#define C64_SYMBOL_DEVICE_COMMODORE	1
#define C64_SYMBOL_DEVICE_DRIVE1541	2

class CDebugSymbols
{
public:
	CDebugSymbols(CDebugInterface *debugInterface, bool supportBreakpoints);
	virtual ~CDebugSymbols();
	
	CDebugInterface *debugInterface;
	virtual void SetDataAdapter(CDebugDataAdapter *dataAdapter);
	
	bool supportBreakpoints;

	CDebugDataAdapter *dataAdapter;
	CDebugEventsHistory *debugEventsHistory;

	// Source mappings of the loaded debug info (CDebugDatabase.h): an
	// immutable snapshot, replaced whole on reload under guiMain and
	// debugInterface locks; NULL when none is loaded.
	std::shared_ptr<const CDebugDatabase> debugDatabase;

	// Swaps the database and re-resolves the source breakpoints against it
	// (NULL unloads). Loaders call this instead of assigning debugDatabase;
	// it drops the assembled sources below.
	void SetDebugDatabase(std::shared_ptr<const CDebugDatabase> database);
	void ApplyDebugDatabase(std::shared_ptr<const CDebugDatabase> database);	// keeps the base and assembled sources

	// RD-043: code the embedded 64tass assembled at run time, with its line
	// map and text (DebugInfoFromAssembly). debugDatabase is the loaded one
	// (baseDebugDatabase) with these merged into the current segment's
	// region; a newer one drops those whose code it overwrote, a load drops
	// them all. CODELABELS go to the current segment.
	struct AssembledSource
	{
		std::shared_ptr<const CDebugDatabase> database;
		u32 start, end;		// half-open, the assembled bytes
	};
	std::shared_ptr<const CDebugDatabase> baseDebugDatabase;
	std::vector<AssembledSource> assembledSources;
	void AddAssembledSource(std::shared_ptr<const CDebugDatabase> fragment, u32 start, u32 end,
							const std::vector<std::pair<std::string, u32>> &codeLabels);

	// The debug file the database came from, and why it may not match the
	// program or the sources (DebugInfoCheckStale); empty when it matches.
	std::string debugInfoPath;
	std::vector<std::string> debugInfoWarnings;
	// the program file the debug info was loaded for, when known (RD-007's
	// code check, DebugInfoCheckCode)
	std::string programPath;

	// Source breakpoints (RD-014, CDebugSourceBreakpoint.h): file + line,
	// resolved to address breakpoints in the segments of the loaded database.
	std::vector<CDebugSourceBreakpoint *> sourceBreakpoints;
	CDebugSourceBreakpoint *AddSourceBreakpoint(const char *path, i32 line);
	void RemoveSourceBreakpoint(CDebugSourceBreakpoint *breakpoint);
	CDebugSourceBreakpoint *FindSourceBreakpoint(const char *path, i32 line);
	
	// RD-053 (CDebugSourceWatch.h): watches kept by a C variable's name. Not
	// in a segment: a debug info reload keeps them. DeleteAllWatches (a
	// .watch file replacing the address watches) leaves them too.
	std::vector<CDebugSourceWatch *> sourceWatches;
	CDebugSourceWatch *AddSourceWatch(const std::string &name, const std::string &function, int representation, int numberOfValues);
	CDebugSourceWatch *FindSourceWatch(const std::string &name, const std::string &function);
	void RemoveSourceWatch(CDebugSourceWatch *watch);
	void DeleteAllSourceWatches();
	// resolves every source watch: at the PC when PAUSED, else the fixed globals only
	void UpdateSourceWatches(bool paused);

	std::vector<CDebugSymbolsSegment *> segments;
	CDebugSymbolsSegment *currentSegment;

	// RD-032: segments a reload replaced. The emulation thread reads
	// currentSegment and its breakpoints at every instruction without a lock,
	// so a retired segment is deleted only once that thread has passed a
	// frame boundary after the swap (a VSync task marks it), or at once when
	// the machine is not running. Deletion itself runs on the UI side
	// (DeleteRetiredSegments from the frame loop and from loads): the
	// segment's destructor takes the GUI mutex.
	std::vector<std::pair<CDebugSymbolsSegment *, u32>> retiredSegments;	// segment, generation
	u32 retireGeneration = 0;
	std::atomic<u32> safeGeneration { 0 };
	void RetireSegment(CDebugSymbolsSegment *segment);
	void DeleteRetiredSegments();
	u32 numDeletedSegments = 0;		// for tests
	CSlrString *previousSegmentName;
	int currentSegmentNum;
	CDebugSymbolsSegment *FindSegment(CSlrString *segmentName);
	
	void ActivateSelectedSegment();
	void ActivateSegment(CDebugSymbolsSegment *segment);
	void DeactivateSegment();
	void SetPreviousSegment(CSlrString* segmentName);
	
	//
	void SelectNextSegment();
	void SelectPreviousSegment();
	bool SetSegment(CSlrString *segmentName);
	
	virtual void ClearTemporaryBreakpoint();
	virtual void UpdateRenderBreakpoints();
	
	//
	
	void DeleteAllSymbols();
	void ParseSymbols(CSlrString *fileName);
	void ParseSymbols(CSlrFile *file);
	void ParseSymbols(CByteBuffer *byteBuffer);
	
	void DeleteAllBreakpoints();
	void ParseBreakpoints(CSlrString *fileName);
	void ParseBreakpoints(CByteBuffer *byteBuffer);
	
	void DeleteAllWatches();
	void ParseWatches(CSlrString *fileName);
	void ParseWatches(CSlrFile *file);
	void ParseWatches(CByteBuffer *byteBuffer);

	void DeleteSourceDebugInfo();
	void ParseSourceDebugInfo(CSlrString *fileName);
	void ParseSourceDebugInfo(CSlrFile *file);
	void ParseSourceDebugInfo(CByteBuffer *byteBuffer);
	
	//
	void LoadLabelsRetroDebuggerFormat(CSlrString *filePath);
	void SaveLabelsRetroDebuggerFormat(CSlrString *filePath);
	
	bool SerializeLabelsToHjson(Hjson::Value hjsonRoot);
	bool DeserializeLabelsFromHjson(Hjson::Value hjsonRoot);

	//
	void LoadWatchesRetroDebuggerFormat(CSlrString *filePath);
	void SaveWatchesRetroDebuggerFormat(CSlrString *filePath);
	bool SerializeWatchesToHjson(Hjson::Value hjsonRoot);
	bool DeserializeWatchesFromHjson(Hjson::Value hjsonRoot);
	
	//
	void LoadBreakpointsRetroDebuggerFormat(CSlrString *filePath);
	void SaveBreakpointsRetroDebuggerFormat(CSlrString *filePath);
	bool SerializeBreakpointsToHjson(Hjson::Value hjsonRoot);
	bool DeserializeBreakpointsFromHjson(Hjson::Value hjsonRoot);

	// The symbols that labels for DEVICEID (C64_SYMBOL_DEVICE_*, the "C:"/"D:"
	// prefix of VICE labels) belong to; NULL when this machine has no such device.
	virtual CDebugSymbols *GetSymbolsForDevice(int deviceId);

	// get currently executed addr (PC) to be checked against breakpoint, note it is virtual to have it different for C64 Drive CPU
	virtual int GetCurrentExecuteAddr();

	//
	virtual void LockMutex();
	virtual void UnlockMutex();
	
	//
	void CreateDefaultSegment();
	
	// virtual method to create specific symbols segment (for C64 including raster line, VIC, CIA, NMI irqs, for Drive including VIA irqs, etc.)
	virtual CDebugSymbolsSegment *CreateNewDebugSymbolsSegment(CSlrString *name);
	virtual CDebugSymbolsSegment *CreateNewDebugSymbolsSegment(CSlrString *name, int segmentNum);
	
	// memory cells for memory events and coloring
	virtual CDebugMemory *CreateNewDebugMemoryMap();
	CDebugMemory *memory;

private:
	void BindSourceBreakpoint(CDebugSourceBreakpoint *breakpoint);
	void UnbindSourceBreakpoint(CDebugSourceBreakpoint *breakpoint);
};

#endif
