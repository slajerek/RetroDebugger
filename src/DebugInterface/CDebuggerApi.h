#ifndef _CDEBUGGERAPI_H_
#define _CDEBUGGERAPI_H_

#include "SYS_Defs.h"
#include "DebuggerDefs.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterface.h"
#include "SYS_Threading.h"
#include "CImageData.h"
#include <map>
#include <string>
#include <vector>

class CGuiView;
class CDebugInterface;
class CSourceStepController;

class CDebuggerApi
{
public:
	CDebuggerApi(CDebugInterface *debugInterface);
	virtual ~CDebuggerApi();
	
	CDebugInterface *debugInterface;
	
	static CDebuggerApi *GetDebuggerApi(u8 emulatorType);
	
	virtual void ResetMachine(bool isHardReset);
	virtual void PauseEmulation();
	virtual void UnPauseEmulation();
	
	virtual void StepOneCycle();
	virtual void StepOverInstruction();
	virtual void StepOverSubroutine();	// step over JSR
	virtual bool StepOutSubroutine();	// run until the current frame returns
	// RD-044: the CPU a source step or query is about. CPU "" or "main" is the
	// machine's CPU; the C64 also has "drive1541" (CDebuggerApiVice). False
	// for a CPU this machine does not have.
	struct SourceCpu
	{
		CDebugSymbols *symbols = NULL;
		CSourceStepController *stepper = NULL;
		int pc = -1;
		bool bankAware = true;		// its fetches have banks (RD-007)
	};
	virtual bool GetSourceCpu(const std::string &cpu, SourceCpu *out);

	// Source-level step (RD-013): KIND "into", "over" or "out"; false when not armed.
	virtual bool SourceStep(const std::string &kind, u64 budget = 0, const std::string &cpu = "");
	// The last source step: armed, kind, reason, pc and, when mapped, file and line.
	virtual nlohmann::json GetSourceStepStatusJson(const std::string &cpu = "");

	// RD-036: source queries on the loaded debug info (current segment's region)
	// ADDRESS (-1: the PC) -> file, line, function and every candidate mapping
	virtual nlohmann::json GetSourceLocationJson(int address, const std::string &cpu = "");
	// RD-007: the code in memory against the program file (default: the one
	// the debug info was loaded for)
	nlohmann::json VerifySourceCodeJson(const std::string &programPath);
	// RD-045: recording on/off, the capabilities, and the COUNT lines executed last
	virtual nlohmann::json GetSourceTraceJson(int count);
	// FILE:LINE -> the line actually used and its first addresses
	virtual nlohmann::json ResolveSourceLineJson(const std::string &file, int line, const std::string &cpu = "");
	// source breakpoints: add / remove by file + line, list with their resolution
	virtual nlohmann::json AddSourceBreakpointJson(const std::string &file, int line, const std::string &cpu = "");
	virtual bool RemoveSourceBreakpoint(const std::string &file, int line, const std::string &cpu = "");
	virtual nlohmann::json ListSourceBreakpointsJson(const std::string &cpu = "");
	// loads a symbols / debug info file by content (DebugInfoLoadFile)
	virtual nlohmann::json LoadDebugInfoJson(const std::string &path, const std::string &cpu = "");
	// RD-012: the frames on the stack now, innermost first, from the stack
	// annotation; each with its return address and, when mapped, function/line
	virtual nlohmann::json GetCallStackJson();

	virtual u64 GetMainCpuCycleCounter();
	virtual u64 GetMainCpuInstructionCycleCounter();
	virtual unsigned int GetEmulationFrameNumber();

	virtual void CreateNewPicture(u8 mode, u8 backgroundColor);
	virtual void StartThread(CSlrThread *run);
	
	// rgb reference image in vic editor
	virtual void ClearReferenceImage();
	virtual void LoadReferenceImage(char *filePath);
	virtual void LoadReferenceImage(CImageData *imageData);
	virtual void SetReferenceImageLayerVisible(bool isVisible);
	CImageData *GetReferenceImage();
	
	// emulated computer screen
	virtual void ClearScreen();
	virtual void ZoomDisplay(float newScale);
	
	// load from png file
	virtual bool ConvertImageToScreen(char *filePath);
	virtual bool ConvertImageToScreen(CImageData *imageData);
	virtual CImageData *GetScreenImage(int *width, int *height);
	
	// always returns 320x200 for C64:
	virtual CImageData *GetScreenImageWithoutBorders();
	
	//
	virtual u8 PaintPixel(int x, int y, u8 color);
	virtual u8 PaintReferenceImagePixel(int x, int y, u8 color);
	virtual u8 PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a);
	
	// data adapters
	CDataAdapter *GetDataAdapterMemoryWithIO();
	CDataAdapter *GetDataAdapterMemoryDirectRAM();

	// direct access
	virtual void SetByte(int addr, u8 v);	/// NOTE: this is generic poke, peek, in most cases including I/O, should not be used
	virtual void SetByteWithIo(int addr, u8 v);
	virtual void SetByteToRam(int addr, u8 v);
	virtual u8 GetByte(int addr);
	virtual u8 GetByteWithIo(int addr);
	virtual u8 GetByteFromRam(int addr);
	
	virtual void SetWord(int addr, u16 v);
	// Sets the CPU program counter. Returns true when the new PC has been committed
	// to the CPU, false when the backend could not commit it (still queued, or not
	// supported by this backend).
	virtual bool MakeJmp(int addr);
	
	//
	virtual void DetachEverything();
	// Detaches the disk image from a single drive WITHOUT resetting the machine.
	// This is the "Detach Disk Image" GUI action, as opposed to DetachEverything()
	// which power-cycles the machine. Returns false when the platform has no disk
	// drives or when deviceNumber is out of range.
	virtual bool DetachDriveDisk(int deviceNumber);
	// Attaches only a disk image, without loading or running any program and
	// without resetting the machine. Returns false when unsupported, invalid,
	// or the image could not be attached.
	virtual bool AttachDriveDisk(const char *filePath, int deviceNumber);
	// Drive/device number used when the caller does not specify one (C64: 8, Atari: 1)
	virtual int GetDefaultDiskDriveNumber();
	virtual void ClearRam(int startAddr, int endAddr, u8 value);
	
	//
	virtual int Assemble(int addr, char *assembleText);
	
	CByteBuffer *byteBufferAssembleText;
	virtual u8 *Assemble64Tass(char *assembleText, int *codeStartAddr, int *codeSize);
	virtual void Assemble64TassClearBuffer();
	virtual u8 *Assemble64Tass(int *codeStartAddr, int *codeSize);
	virtual u8 *Assemble64Tass(int *codeStartAddr, int *codeSize, const char *storeAsmFileName, bool quiet);
	virtual bool Assemble64TassToRam(int *codeStartAddr, int *codeSize);
	virtual bool Assemble64TassToRam(int *codeStartAddr, int *codeSize, char *storeAsmFileName, bool quiet);
	virtual void Assemble64TassAddLine(const char *format, ...);

	// RD-043: what the embedded 64tass reported for its last pass (its C
	// callbacks, c64debugger_assemble_* in CDebuggerApi.cpp)
	std::map<u32, i32> assembledLines;		// logical address -> source line
	struct AssembledLabel
	{
		std::string name;
		u32 value;
		bool isCode;	// a label, not a constant (name = value)
	};
	std::vector<AssembledLabel> assembledLabels;
	// one assembled byte for the machine (VICE: to the assemble target's RAM)
	virtual void AssembleResultToMemory(int addr, u8 value);
	// the symbols assembled code is described in; NULL when the bytes go
	// nowhere the debugger shows (VICE: no target)
	virtual CDebugSymbols *GetAssembleSymbols();
	// the last assembly's text (NAME, or "64tass-<start>.asm"), line map and
	// code labels into GetAssembleSymbols(): the source view shows the text
	// with the PC line, the disassembly the labels
	void RegisterAssembledSource(const char *text, size_t length, const char *name);
	// RD-043, remote: CODE, a whole 64tass source (origin "*=ADDRESS"
	// prepended when ADDRESS >= 0), assembled into memory; the result names
	// the address range, the labels and the line ranges
	nlohmann::json Assemble64TassJson(const std::string &code, int address);
	
	//
	virtual void AddWatch(CSlrString *segmentName, int address, CSlrString *watchName, uint8 representation, int numberOfValues);
	virtual void AddWatch(int address, char *watchName, uint8 representation, int numberOfValues);
	virtual void AddWatch(int address, char *watchName);
	
	//
	virtual void SaveBinary(int fromAddr, int toAddr, const char *fileName);
	virtual int LoadBinary(int fromAddr, const char *filePath);
	
	//
	virtual void LoadSnapshot(const char *fileName);
	
	virtual u8 *ExomizerMemoryRaw(u16 fromAddr, u16 toAddr, int *compressedSize);
	
	//
	virtual void ResetEmulationCounters();
	
	//
	virtual void SetWarpSpeed(bool isWarpSpeed);
	virtual bool GetWarpSpeed();

	// input
	virtual bool KeyboardDown(u32 mtKeyCode);
	virtual bool KeyboardUp(u32 mtKeyCode);
	
	virtual void JoystickDown(int port, u32 axis);
	virtual void JoystickUp(int port, u32 axis);
	
	// shortcuts to the Engine
	virtual void ShowMessage(const char *text);
	virtual void BlitText(const char *text, float posX, float posY, float fontSize);
	virtual void Sleep(long milliseconds);
	virtual long GetCurrentTimeInMilliseconds();
	virtual long GetCurrentFrameNumber();
	
	static u32 JoypadAxisNameToAxisCode(std::string axisName);

	// segments
	CSlrString *GetCurrentSegmentName();
	bool SetCurrentSegment(CSlrString *segmentName);
	
	// breakpoints
	u64 AddBreakpointPC(int addr);
	// RD-025: a PC breakpoint that fires only where the fetch at addr is one
	// of these physical bytes; replaces the breakpoint's previous identities
	u64 AddBreakpointPC(int addr, const std::vector<CDebugPhysicalAddress> &physicalIdentities);
	u64 RemoveBreakpointPC(int addr);
	u64 AddBreakpointMemory(int address, u32 memoryAccess, DataBreakpointComparison comparison, int value);
	u64 RemoveBreakpointMemory(int address);
	
	//
	virtual void AddView(CGuiView *view);

	//
	virtual nlohmann::json GetCpuStatusJson();

	// MCP analysis tools
	nlohmann::json DisassembleMemory(int startAddr, int instructionCount, bool includeBytes, bool includeLabels);
	nlohmann::json AssembleCode(int startAddr, const std::string &code);
	nlohmann::json GetCodeMap(int startAddr, int endAddr);
	nlohmann::json SearchOpcodePattern(const std::string &pattern, int startAddr, int endAddr, bool executedOnly);

private:
	// Helper: format one disassembled instruction as text line
	std::string FormatDisassemblyLine(u16 addr, u8 op, u8 lo, u8 hi, bool includeBytes, bool includeLabels, bool isExecuted);
};

#endif
