#include "CDebuggerApi.h"
#include "CViewC64.h"
#include "CViewMonitorConsole.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicEditorCreateNewPicture.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerImage.h"
#include "CViewC64VicEditorLayers.h"
#include "CVicEditorLayerC64Screen.h"
#include "CViewC64Sprite.h"
#include "SYS_KeyCodes.h"
#include "C64Tools.h"
#include "CViewDisassembly.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CSlrFileFromOS.h"
#include "CViewDataMap.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CViewDataWatch.h"
#include "CGuiMain.h"
#include "CSlrFont.h"
#include "CDataAdapter.h"
#include "CDebuggerApiVice.h"
#include "CDebuggerApiNestopia.h"
#include "CDebuggerApiAtari.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "C64Opcodes.h"
#include "CSlrTextParser.h"
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdlib>

// static factory
CDebuggerApi *CDebuggerApi::GetDebuggerApi(u8 emulatorType)
{
	CDebugInterface *debugInterface = viewC64->GetDebugInterface(emulatorType);
	if (debugInterface)
	{
		return debugInterface->GetDebuggerApi();
	}
	
	LOGError("CDebuggerAPI::GetDebuggerApi: emulatorType=%d not supported", emulatorType);
	return NULL;
}

CDebuggerApi::CDebuggerApi(CDebugInterface *debugInterface)
{
	this->debugInterface = debugInterface;
	byteBufferAssembleText = new CByteBuffer();
}

CDebuggerApi::~CDebuggerApi()
{
}

void CDebuggerApi::PauseEmulation()
{
	LOGD("CDebuggerApi::PauseEmulation");
	debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
}

void CDebuggerApi::UnPauseEmulation()
{
	LOGD("CDebuggerApi::UnPauseEmulation");
	debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
}

void CDebuggerApi::StepOneCycle()
{
	debugInterface->StepOneCycle();
}

void CDebuggerApi::StepOverInstruction()
{
	debugInterface->StepOverInstruction();
}

void CDebuggerApi::StepOverSubroutine()
{
	debugInterface->StepOverSubroutine();
}

u64 CDebuggerApi::GetMainCpuCycleCounter()
{
	return debugInterface->GetMainCpuCycleCounter();
}

u64 CDebuggerApi::GetMainCpuInstructionCycleCounter()
{
	return debugInterface->GetCurrentCpuInstructionCycleCounter();
}

unsigned int CDebuggerApi::GetEmulationFrameNumber()
{
	return debugInterface->GetEmulationFrameNumber();
}


void CDebuggerApi::StartThread(CSlrThread *run)
{
	SYS_StartThread(run);
}

void CDebuggerApi::CreateNewPicture(u8 mode, u8 backgroundColor)
{
	SYS_FatalExit("CDebuggerApi::CreateNewPicture: not implemented");
}

void CDebuggerApi::ClearScreen()
{
	SYS_FatalExit("CDebuggerApi::ClearScreen: not implemented");
}

bool CDebuggerApi::ConvertImageToScreen(char *filePath)
{
	SYS_FatalExit("CDebuggerApi::ConvertImageToScreen: not implemented");
	return false;
}

bool CDebuggerApi::ConvertImageToScreen(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApi::ConvertImageToScreen: not implemented");
	return false;
}

void CDebuggerApi::ClearReferenceImage()
{
	SYS_FatalExit("CDebuggerApi::ClearReferenceImage: not implemented");
}

void CDebuggerApi::LoadReferenceImage(char *filePath)
{
	SYS_FatalExit("CDebuggerApi::LoadReferenceImage: not implemented");
}

void CDebuggerApi::LoadReferenceImage(CImageData *imageData)
{
	SYS_FatalExit("CDebuggerApi::LoadReferenceImage: not implemented");
}

void CDebuggerApi::SetReferenceImageLayerVisible(bool isVisible)
{
	SYS_FatalExit("CDebuggerApi::SetReferenceImageLayerVisible: not implemented");
}

CImageData *CDebuggerApi::GetReferenceImage()
{
	SYS_FatalExit("CDebuggerApi::GetReferenceImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApi::GetScreenImage(int *width, int *height)
{
	SYS_FatalExit("CDebuggerApi::GetScreenImage: not implemented");
	return NULL;
}

CImageData *CDebuggerApi::GetScreenImageWithoutBorders()
{
	SYS_FatalExit("CDebuggerApi::GetScreenImageWithoutBorders: not implemented");
	return NULL;
}

void CDebuggerApi::ZoomDisplay(float newScale)
{
	SYS_FatalExit("CDebuggerApi::ZoomDisplay: not implemented");
}

u8 CDebuggerApi::PaintPixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApi::PaintPixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApi::PaintReferenceImagePixel(int x, int y, u8 color)
{
	SYS_FatalExit("CDebuggerApi::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

u8 CDebuggerApi::PaintReferenceImagePixel(int x, int y, u8 r, u8 g, u8 b, u8 a)
{
	SYS_FatalExit("CDebuggerApi::PaintReferenceImagePixel: not implemented");
	return PAINT_RESULT_ERROR;
}

void CDebuggerApi::Sleep(long milliseconds)
{
	SYS_Sleep(milliseconds);
}

long CDebuggerApi::GetCurrentTimeInMilliseconds()
{
	return SYS_GetCurrentTimeInMillis();
}

long CDebuggerApi::GetCurrentFrameNumber()
{
	return debugInterface->GetEmulationFrameNumber();
}

void CDebuggerApi::ResetMachine(bool isHardReset)
{
	if (isHardReset)
	{
		debugInterface->ResetHard();
	}
	else
	{
		debugInterface->ResetSoft();
	}
}

void CDebuggerApi::MakeJmp(int addr)
{
	SYS_FatalExit("CDebuggerApi::MakeJMP: not implemented");
}

CDataAdapter *CDebuggerApi::GetDataAdapterMemoryWithIO()
{
	return debugInterface->GetDataAdapter();
}

CDataAdapter *CDebuggerApi::GetDataAdapterMemoryDirectRAM()
{
	return debugInterface->GetDataAdapterDirectRam();
}

void CDebuggerApi::SetByte(int addr, u8 v)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (addr >= 0 && addr < dataAdapter->AdapterGetDataLength())
	{
		dataAdapter->AdapterWriteByte(addr, v);
	}
}

void CDebuggerApi::SetByteWithIo(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApi::SetByteWithIo: not implemented");
}

void CDebuggerApi::SetByteToRam(int addr, u8 v)
{
	SYS_FatalExit("CDebuggerApi::SetByteToRam: not implemented");
}

u8 CDebuggerApi::GetByte(int addr)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (addr >= 0 && addr < dataAdapter->AdapterGetDataLength())
	{
		u8 val;
		dataAdapter->AdapterReadByte(addr, &val);
		return val;
	}
	
	return 0;
}

u8 CDebuggerApi::GetByteWithIo(int addr)
{
	SYS_FatalExit("CDebuggerApi::GetByteWithIo: not implemented");
	return 0;
}

u8 CDebuggerApi::GetByteFromRam(int addr)
{
	SYS_FatalExit("CDebuggerApi::GetByteFromRam: not implemented");
	return 0;
}


void CDebuggerApi::SetWord(int addr, u16 v)
{
	SetByte(addr+1, ( (v) &0xFF00)>>8);
	SetByte(addr  , ( (v) &0x00FF));
}

void CDebuggerApi::DetachEverything()
{
	SYS_FatalExit("CDebuggerApi::DetachEverything: not implemented");
}

void CDebuggerApi::ClearRam(int startAddr, int endAddr, u8 value)
{
	for (int i = startAddr; i < endAddr; i++)
	{
		SetByteToRam(i, value);
	}
}

extern "C" {
	unsigned char *assemble_64tass(void *userData, char *assembleText, int assembleTextSize, int *codeStartAddr, int *codeSize);
	void assemble_64tass_setquiet(int isQuiet);
}

u8 *CDebuggerApi::Assemble64Tass(char *assembleText, int *codeStartAddr, int *codeSize)
{
	u8 *buf = assemble_64tass((void*)this, assembleText, strlen(assembleText), codeStartAddr, codeSize);
	
	if (buf == NULL)
	{
		*codeStartAddr = 0;
		*codeSize = 0;
		LOGError("CDebuggerAPI::Assemble64Tass: assemble failed");
		return NULL;
	}

	return buf;
}

bool CDebuggerApi::Assemble64TassToRam(int *codeStartAddr, int *codeSize)
{
	return Assemble64TassToRam(codeStartAddr, codeSize, NULL, false);
}

bool CDebuggerApi::Assemble64TassToRam(int *codeStartAddr, int *codeSize, char *fileName, bool quiet)
{
	u8 *buf = Assemble64Tass(codeStartAddr, codeSize, fileName, quiet);
	
	if (buf == NULL)
	{
		*codeStartAddr = 0;
		*codeSize = 0;
		LOGError("CDebuggerAPI::Assemble64Tass: assemble failed");
		return false;
	}

	int addr = *codeStartAddr;
	for (int i = 0; i < *codeSize; i++)
	{
		SetByteToRam(addr, buf[i]);
		addr++;
	}
	free(buf);
	
	return true;
}


void CDebuggerApi::Assemble64TassClearBuffer()
{
	byteBufferAssembleText->Clear();
}

void CDebuggerApi::Assemble64TassAddLine(const char *format, ...)
{
	char *assembleText = SYS_GetCharBuf();

	va_list args;

	va_start(args, format);
	vsnprintf(assembleText, MAX_STRING_LENGTH, format, args);
	va_end(args);

	char *ptr = assembleText;
	while (*ptr != '\0')
	{
		byteBufferAssembleText->PutU8(*ptr);
		ptr++;
	}
	byteBufferAssembleText->PutU8('\n');
	
	SYS_ReleaseCharBuf(assembleText);
}

u8 *CDebuggerApi::Assemble64Tass(int *codeStartAddr, int *codeSize)
{
	return Assemble64Tass(codeStartAddr, codeSize, NULL, false);
}

u8 *CDebuggerApi::Assemble64Tass(int *codeStartAddr, int *codeSize, const char *storeAsmFileName, bool quiet)
{
	assemble_64tass_setquiet(quiet);
	
	byteBufferAssembleText->PutU8(0x00);
	
	char *assembleText = (char*)byteBufferAssembleText->data;
	if (storeAsmFileName != NULL)
	{
		FILE *fp = fopen(storeAsmFileName, "wb");
		if (fp != NULL)
		{
			fprintf(fp, "%s", assembleText);
			fclose(fp);
		}
		else
		{
			LOGError("CDebuggerAPI::Assemble64Tass: file not writable %s", storeAsmFileName);
		}
	}
	
	LOGD("assembleText='%s'", assembleText);
	
	u8 *buf = assemble_64tass((void*)this, assembleText, byteBufferAssembleText->length-1, codeStartAddr, codeSize);
	
	if (buf == NULL)
	{
		*codeStartAddr = 0;
		*codeSize = 0;
		LOGError("CDebuggerAPI::Assemble64Tass: assemble failed");
		return NULL;
	}
	
	byteBufferAssembleText->Reset();
	
	return buf;
}

int CDebuggerApi::Assemble(int addr, char *assembleText)
{
	SYS_FatalExit("CDebuggerApi::Assemble: not implemented");
	return -1;
}

//
CSlrString *CDebuggerApi::GetCurrentSegmentName()
{
	if (debugInterface->symbols->currentSegment == NULL)
		return NULL;
	return debugInterface->symbols->currentSegment->name;
}

bool CDebuggerApi::SetCurrentSegment(CSlrString *segmentName)
{
	return debugInterface->symbols->SetSegment(segmentName);
}

u64 CDebuggerApi::AddBreakpointPC(int addr)
{
	debugInterface->LockMutex();
	CDebugBreakpointAddr *breakpoint = debugInterface->viewDisassembly->AddPCBreakpoint(addr);
	debugInterface->UnlockMutex();
	return breakpoint->breakpointId;
}

u64 CDebuggerApi::RemoveBreakpointPC(int addr)
{
	debugInterface->LockMutex();
	u64 breakpointId = debugInterface->viewDisassembly->RemovePCBreakpoint(addr);
	debugInterface->UnlockMutex();
	return breakpointId;
}

u64 CDebuggerApi::AddBreakpointMemory(int address, u32 memoryAccess, DataBreakpointComparison comparison, int value)
{
	debugInterface->LockMutex();
	CDebugBreakpointData *breakpoint = debugInterface->symbols->currentSegment->AddBreakpointMemory(address, memoryAccess, comparison, value);
	debugInterface->UnlockMutex();
	return breakpoint->breakpointId;
}

u64 CDebuggerApi::RemoveBreakpointMemory(int address)
{
	debugInterface->LockMutex();
	u64 breakpointId = debugInterface->symbols->currentSegment->breakpointsData->DeleteBreakpoint(address);
	debugInterface->UnlockMutex();
	return breakpointId;
}

void CDebuggerApi::AddWatch(CSlrString *segmentName, int address, CSlrString *watchName, uint8 representation, int numberOfValues)
{
	if (debugInterface->symbols)
	{
		CDebugSymbolsSegment *segment = debugInterface->symbols->FindSegment(segmentName);
		if (segment == NULL)
		{
			segmentName->DebugPrint("segment=");
			LOGError("CDebuggerAPI::AddWatch: segment not found");
			return;
		}

		// TODO: convert watch name in symbols to CSlrString
		char *cWatchName = watchName->GetStdASCII();
		segment->AddWatch(address, cWatchName, representation, numberOfValues);
		delete [] cWatchName;
	}
	else
	{
		LOGError("CDebuggerAPI::AddWatch: no symbols");
	}
}

void CDebuggerApi::AddWatch(int address, char *watchName, uint8 representation, int numberOfValues)
{
	if (debugInterface->symbols)
	{
		CDebugSymbolsSegment *segment = debugInterface->symbols->currentSegment;
		if (segment == NULL)
		{
			LOGError("CDebuggerAPI::AddWatch: default segment not found");
			return;
		}
		
		segment->AddWatch(address, watchName, representation, numberOfValues);
	}
	else
	{
		LOGError("CDebuggerAPI::AddWatch: no symbols");
	}
}

void CDebuggerApi::AddWatch(int address, char *watchName)
{
	AddWatch(address, watchName, WATCH_REPRESENTATION_HEX_8, 1);
}

u8 *CDebuggerApi::ExomizerMemoryRaw(u16 fromAddr, u16 toAddr, int *compressedSize)
{
	return C64ExomizeMemoryRaw(fromAddr, toAddr, compressedSize);
}

void CDebuggerApi::SaveBinary(int fromAddr, int toAddr, const char *filePath)
{
	C64SaveMemory(fromAddr, toAddr, false, debugInterface->GetDataAdapterDirectRam(), filePath);
}

int CDebuggerApi::LoadBinary(int fromAddr, const char *filePath)
{
	return C64LoadMemory(fromAddr, debugInterface->GetDataAdapterDirectRam(), filePath);
}

void CDebuggerApi::LoadSnapshot(const char *fileName)
{
	debugInterface->LoadFullSnapshot((char*)fileName);
}

void CDebuggerApi::ResetEmulationCounters()
{
	debugInterface->ResetMainCpuDebugCycleCounter();
	debugInterface->ResetEmulationFrameCounter();
}

void CDebuggerApi::SetWarpSpeed(bool isWarpSpeed)
{
	debugInterface->SetSettingIsWarpSpeed(isWarpSpeed);
}

bool CDebuggerApi::GetWarpSpeed()
{
	return debugInterface->GetSettingIsWarpSpeed();
}

bool CDebuggerApi::KeyboardDown(u32 mtKeyCode)
{
	return debugInterface->KeyboardDown(mtKeyCode);
}

bool CDebuggerApi::KeyboardUp(u32 mtKeyCode)
{
	return debugInterface->KeyboardUp(mtKeyCode);
}

void CDebuggerApi::JoystickDown(int port, u32 axis)
{
	debugInterface->JoystickDown(port, axis);
}

void CDebuggerApi::JoystickUp(int port, u32 axis)
{
	debugInterface->JoystickUp(port, axis);
}

void CDebuggerApi::ShowMessage(const char *text)
{
	viewC64->ShowMessage((char*)text);
}

void CDebuggerApi::BlitText(const char *text, float posX, float posY, float fontSize)
{
	CSlrFont *font = viewC64->fontDisassembly;
	font->BlitText((char*)text, posX, posY, -1, fontSize);
}

void CDebuggerApi::AddView(CGuiView *view)
{
	guiMain->LockMutex();
	guiMain->AddViewSkippingLayout(view);
	debugInterface->AddView(view);
	guiMain->UnlockMutex();
}

nlohmann::json CDebuggerApi::GetCpuStatusJson()
{
	nlohmann::json empty;
	return empty;
}

u32 CDebuggerApi::JoypadAxisNameToAxisCode(std::string axisName)
{
	if (axisName == "select")
	{
		return JOYPAD_SELECT;
	}
	if (axisName == "start")
	{
		return JOYPAD_START;
	}
	if (axisName == "fireB")
	{
		return JOYPAD_FIRE_B;
	}
	if (axisName == "fire")
	{
		return JOYPAD_FIRE;
	}
	if (axisName == "e" || axisName == "east")
	{
		return JOYPAD_E;
	}
	if (axisName == "w" || axisName == "west")
	{
		return JOYPAD_W;
	}
	if (axisName == "s" || axisName == "south")
	{
		return JOYPAD_S;
	}
	if (axisName == "n" || axisName == "north")
	{
		return JOYPAD_N;
	}
	if (axisName == "sw" || axisName == "southwest")
	{
		return JOYPAD_SW;
	}
	if (axisName == "se" || axisName == "southeast")
	{
		return JOYPAD_SE;
	}
	if (axisName == "nw" || axisName == "northwest")
	{
		return JOYPAD_NW;
	}
	if (axisName == "ne" || axisName == "northeast")
	{
		return JOYPAD_NE;
	}
	return JOYPAD_IDLE;
}

// ============================================================================
// MCP analysis tools
// ============================================================================

using json = nlohmann::json;

std::string CDebuggerApi::FormatDisassemblyLine(u16 addr, u8 op, u8 lo, u8 hi, bool includeBytes, bool includeLabels, bool isExecuted)
{
	char buf[256];
	int len = opcodes[op].addressingLength;

	// Address
	int pos = sprintf(buf, ".%04X  ", addr);

	// Optional bytes column (fixed width: 10 chars)
	if (includeBytes)
	{
		switch (len)
		{
			case 1: pos += sprintf(buf + pos, "%02X        ", op); break;
			case 2: pos += sprintf(buf + pos, "%02X %02X     ", op, lo); break;
			case 3: pos += sprintf(buf + pos, "%02X %02X %02X  ", op, lo, hi); break;
			default: pos += sprintf(buf + pos, "          "); break;
		}
	}

	// Mnemonic
	pos += sprintf(buf + pos, "%s ", opcodes[op].name);

	// Argument
	switch (opcodes[op].addressingMode)
	{
		case ADDR_IMP:
			break;
		case ADDR_IMM:
			pos += sprintf(buf + pos, "#$%02X", lo);
			break;
		case ADDR_ZP:
		{
			char labelBuf[512];
			if (includeLabels && debugInterface->symbols && debugInterface->symbols->currentSegment
				&& debugInterface->symbols->currentSegment->FindLabelText(lo, labelBuf))
				pos += sprintf(buf + pos, "%s", labelBuf);
			else
				pos += sprintf(buf + pos, "$%02X", lo);
			break;
		}
		case ADDR_ZPX:
			pos += sprintf(buf + pos, "$%02X,X", lo);
			break;
		case ADDR_ZPY:
			pos += sprintf(buf + pos, "$%02X,Y", lo);
			break;
		case ADDR_IZX:
			pos += sprintf(buf + pos, "($%02X,X)", lo);
			break;
		case ADDR_IZY:
			pos += sprintf(buf + pos, "($%02X),Y", lo);
			break;
		case ADDR_ABS:
		{
			u16 absAddr = (hi << 8) | lo;
			char labelBuf[512];
			if (includeLabels && debugInterface->symbols && debugInterface->symbols->currentSegment
				&& debugInterface->symbols->currentSegment->FindLabelText(absAddr, labelBuf))
				pos += sprintf(buf + pos, "%s", labelBuf);
			else
				pos += sprintf(buf + pos, "$%04X", absAddr);
			break;
		}
		case ADDR_ABX:
			pos += sprintf(buf + pos, "$%04X,X", (hi << 8) | lo);
			break;
		case ADDR_ABY:
			pos += sprintf(buf + pos, "$%04X,Y", (hi << 8) | lo);
			break;
		case ADDR_IND:
			pos += sprintf(buf + pos, "($%04X)", (hi << 8) | lo);
			break;
		case ADDR_REL:
		{
			u16 target = (addr + 2 + (int8)lo) & 0xFFFF;
			char labelBuf[512];
			if (includeLabels && debugInterface->symbols && debugInterface->symbols->currentSegment
				&& debugInterface->symbols->currentSegment->FindLabelText(target, labelBuf))
				pos += sprintf(buf + pos, "%s", labelBuf);
			else
				pos += sprintf(buf + pos, "$%04X", target);
			break;
		}
		default:
			break;
	}

	// Execute marker
	if (isExecuted)
		pos += sprintf(buf + pos, "  ; [exec]");

	return std::string(buf, pos);
}

json CDebuggerApi::DisassembleMemory(int startAddr, int instructionCount, bool includeBytes, bool includeLabels)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (!dataAdapter)
		return json({{"error", "no_data_adapter"}});

	int memLength = dataAdapter->AdapterGetDataLength();
	CDebugMemory *debugMemory = (debugInterface->symbols) ? debugInterface->symbols->memory : NULL;

	json result;
	std::ostringstream text;
	json instructions = json::array();
	int addr = startAddr;

	for (int i = 0; i < instructionCount && addr < memLength; i++)
	{
		u8 op, lo = 0, hi = 0;
		dataAdapter->AdapterReadByte(addr, &op);

		int len = opcodes[op].addressingLength;
		if (len >= 2 && addr + 1 < memLength)
			dataAdapter->AdapterReadByte(addr + 1, &lo);
		if (len >= 3 && addr + 2 < memLength)
			dataAdapter->AdapterReadByte(addr + 2, &hi);

		bool isExec = false;
		if (debugMemory)
		{
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
			if (cell) isExec = cell->isExecuteCode;
		}

		std::string line = FormatDisassemblyLine(addr, op, lo, hi, includeBytes, includeLabels, isExec);
		text << line << "\n";

		// Also build JSON instruction array
		json instr;
		instr["addr"] = addr;
		char addrHex[8]; sprintf(addrHex, "%04X", addr);
		instr["addrHex"] = addrHex;
		instr["mnemonic"] = opcodes[op].name;
		instr["isExecuted"] = isExec;
		instr["length"] = len;
		if (opcodes[op].isIllegal) instr["illegal"] = true;
		instructions.push_back(instr);

		addr += len;
	}

	result["text"] = text.str();
	result["instructions"] = instructions;
	result["startAddress"] = startAddr;
	result["nextAddress"] = addr;
	result["count"] = (int)instructions.size();
	return result;
}

json CDebuggerApi::AssembleCode(int startAddr, const std::string &code)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (!dataAdapter)
		return json({{"error", "no_data_adapter"}});

	CDebugMemory *debugMemory = (debugInterface->symbols) ? debugInterface->symbols->memory : NULL;

	// Split code into lines
	std::vector<std::string> lines;
	std::istringstream stream(code);
	std::string line;
	while (std::getline(stream, line))
	{
		// Trim whitespace
		size_t start = line.find_first_not_of(" \t\r");
		if (start == std::string::npos) continue;
		size_t end = line.find_last_not_of(" \t\r");
		std::string trimmed = line.substr(start, end - start + 1);
		if (!trimmed.empty())
			lines.push_back(trimmed);
	}

	if (lines.empty())
		return json({{"error", "empty_code"}});

	// Phase 1: parse all lines, collect opcodes+values (don't write yet)
	struct AssembledInstr {
		int opcode;
		uint16 value;
		int length;
		std::string source;
	};
	std::vector<AssembledInstr> assembled;

	int addr = startAddr;
	for (size_t i = 0; i < lines.size(); i++)
	{
		// Use CViewDisassembly's assembler if available
		CViewDisassembly *viewDis = debugInterface->viewDisassembly;
		if (!viewDis)
			return json({{"error", "no_disassembly_view"}});

		char lineBuf[256];
		strncpy(lineBuf, lines[i].c_str(), sizeof(lineBuf) - 1);
		lineBuf[sizeof(lineBuf) - 1] = 0;

		// Remove '$' characters (assembler is hex-only), same as CViewMonitorConsole does.
		// '$' is the canonical 6502 notation, so clients send "lda #$07" and used to get
		// "Not a number after #" back. Stripping here keeps the shared assembler grammar
		// untouched, so a malformed "lda #$" still fails instead of silently assembling.
		{
			char *src = lineBuf;
			char *dst = lineBuf;
			while (*src)
			{
				if (*src != '$')
				{
					*dst = *src;
					dst++;
				}
				src++;
			}
			*dst = 0x00;
		}

		int instructionOpcode = -1;
		uint16 instructionValue = 0;
		char errorMsg[256] = {0};

		int ret = viewDis->Assemble(addr, lineBuf, &instructionOpcode, &instructionValue, errorMsg);

		if (ret == -1 || instructionOpcode == -1)
		{
			json err;
			err["error"] = "assemble_error";
			err["line"] = (int)(i + 1);
			err["input"] = lines[i];
			err["message"] = std::string(errorMsg);
			return err;
		}

		AssembledInstr ai;
		ai.opcode = instructionOpcode;
		ai.value = instructionValue;
		ai.length = opcodes[instructionOpcode].addressingLength;
		ai.source = lines[i];
		assembled.push_back(ai);

		addr += ai.length;
	}

	// Phase 2: all lines parsed successfully — write to memory atomically
	addr = startAddr;
	json instrArray = json::array();

	for (auto &ai : assembled)
	{
		bool isAvailable;
		dataAdapter->AdapterWriteByte(addr, ai.opcode, &isAvailable);
		if (debugMemory)
		{
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
			if (cell) cell->isExecuteCode = true;
		}

		if (ai.length >= 2)
			dataAdapter->AdapterWriteByte(addr + 1, ai.value & 0xFF, &isAvailable);
		if (ai.length >= 3)
			dataAdapter->AdapterWriteByte(addr + 2, (ai.value >> 8) & 0xFF, &isAvailable);

		json instr;
		char addrHex[8]; sprintf(addrHex, "%04X", addr);
		instr["addr"] = addrHex;

		// Format bytes
		char bytesBuf[16];
		switch (ai.length)
		{
			case 1: sprintf(bytesBuf, "%02x", ai.opcode); break;
			case 2: sprintf(bytesBuf, "%02x %02x", ai.opcode, ai.value & 0xFF); break;
			case 3: sprintf(bytesBuf, "%02x %02x %02x", ai.opcode, ai.value & 0xFF, (ai.value >> 8) & 0xFF); break;
			default: bytesBuf[0] = 0; break;
		}
		instr["bytes"] = bytesBuf;
		instr["mnemonic"] = ai.source;
		instrArray.push_back(instr);

		addr += ai.length;
	}

	json result;
	result["instructions"] = instrArray;
	result["totalBytes"] = addr - startAddr;
	char startHex[8]; sprintf(startHex, "%04X", startAddr);
	char endHex[8]; sprintf(endHex, "%04X", addr > startAddr ? addr - 1 : startAddr);
	result["startAddress"] = startHex;
	result["endAddress"] = endHex;
	result["written"] = true;
	return result;
}

json CDebuggerApi::GetCodeMap(int startAddr, int endAddr)
{
	CDebugMemory *debugMemory = (debugInterface->symbols) ? debugInterface->symbols->memory : NULL;
	if (!debugMemory)
		return json({{"error", "no_debug_memory"}});

	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	int memLength = dataAdapter ? dataAdapter->AdapterGetDataLength() : 65536;
	if (endAddr >= memLength) endAddr = memLength - 1;
	if (startAddr < 0) startAddr = 0;

	json regions = json::array();
	int regionStart = -1;
	int totalCodeBytes = 0;

	for (int addr = startAddr; addr <= endAddr; addr++)
	{
		CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
		bool isCode = (cell && cell->isExecuteCode);

		if (isCode)
		{
			if (regionStart == -1)
				regionStart = addr;
			totalCodeBytes++;
		}
		else
		{
			if (regionStart != -1)
			{
				json region;
				char startHex[8]; sprintf(startHex, "%04X", regionStart);
				char endHex[8]; sprintf(endHex, "%04X", addr - 1);
				region["start"] = startHex;
				region["end"] = endHex;
				region["bytes"] = addr - regionStart;
				regions.push_back(region);
				regionStart = -1;
			}
		}
	}

	// Close last region
	if (regionStart != -1)
	{
		json region;
		char startHex[8]; sprintf(startHex, "%04X", regionStart);
		char endHex[8]; sprintf(endHex, "%04X", endAddr);
		region["start"] = startHex;
		region["end"] = endHex;
		region["bytes"] = endAddr - regionStart + 1;
		regions.push_back(region);
	}

	json result;
	result["source"] = "runtime";
	result["codeRegions"] = regions;
	result["totalCodeBytes"] = totalCodeBytes;
	result["totalDataBytes"] = (endAddr - startAddr + 1) - totalCodeBytes;
	return result;
}

json CDebuggerApi::SearchOpcodePattern(const std::string &pattern, int startAddr, int endAddr, bool executedOnly)
{
	CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();
	if (!dataAdapter)
		return json({{"error", "no_data_adapter"}});

	CDebugMemory *debugMemory = (debugInterface->symbols) ? debugInterface->symbols->memory : NULL;
	int memLength = dataAdapter->AdapterGetDataLength();
	if (endAddr >= memLength) endAddr = memLength - 1;
	if (startAddr < 0) startAddr = 0;

	// Parse pattern: "DEC ??" or "LDA #??" or "STA $0340" etc.
	// Pattern format: MNEMONIC [argument]
	// "??" means wildcard for argument
	// We match by mnemonic name, and optionally by argument value

	// Parse mnemonic from pattern
	char mnemonicBuf[4] = {0};
	size_t i = 0;
	while (i < pattern.size() && i < 3 && pattern[i] != ' ')
	{
		mnemonicBuf[i] = toupper(pattern[i]);
		i++;
	}

	// Parse optional argument filter
	std::string argPart;
	if (i < pattern.size())
	{
		size_t argStart = pattern.find_first_not_of(" \t", i);
		if (argStart != std::string::npos)
			argPart = pattern.substr(argStart);
	}

	bool hasArgFilter = !argPart.empty() && argPart != "??" && argPart != "????";
	int filterValue = -1;
	if (hasArgFilter)
	{
		// Strip $ prefix
		std::string hexStr = argPart;
		if (!hexStr.empty() && hexStr[0] == '#') hexStr = hexStr.substr(1);
		if (!hexStr.empty() && hexStr[0] == '$') hexStr = hexStr.substr(1);
		// Remove surrounding () for indirect
		if (!hexStr.empty() && hexStr[0] == '(') hexStr = hexStr.substr(1);
		if (!hexStr.empty() && hexStr.back() == ')') hexStr.pop_back();
		// Remove ,X ,Y suffix
		size_t commaPos = hexStr.find(',');
		if (commaPos != std::string::npos) hexStr = hexStr.substr(0, commaPos);
		// Parse hex
		if (!hexStr.empty() && hexStr != "??" && hexStr != "????")
		{
			try { filterValue = std::stoi(hexStr, nullptr, 16); } catch (...) {}
		}
	}

	// Build set of matching opcodes
	std::vector<u8> matchingOpcodes;

	// A first token of exactly two hex digits is a raw opcode byte ("a9 ??"), not a
	// mnemonic. Hex bytes are the most common way to write a pattern by hand, and this
	// cannot collide with a mnemonic: every name in the opcode table is 3 characters
	// long (ADC, BCC and DEC look hex-ish but are 3 chars, not 2).
	bool isOpcodeByte = (strlen(mnemonicBuf) == 2
						 && isxdigit((unsigned char)mnemonicBuf[0])
						 && isxdigit((unsigned char)mnemonicBuf[1]));
	if (isOpcodeByte)
	{
		matchingOpcodes.push_back((u8)strtol(mnemonicBuf, NULL, 16));
	}
	else
	{
		for (int op = 0; op < 256; op++)
		{
			if (strcmp(opcodes[op].name, mnemonicBuf) == 0)
				matchingOpcodes.push_back(op);
		}
	}

	if (matchingOpcodes.empty())
		return json({{"error", "unknown_mnemonic"}, {"mnemonic", mnemonicBuf}});

	// Scan memory
	json matches = json::array();
	int maxResults = 100;

	int addr = startAddr;
	while (addr <= endAddr && (int)matches.size() < maxResults)
	{
		u8 op;
		dataAdapter->AdapterReadByte(addr, &op);

		// Check if this opcode matches
		bool opcodeMatch = false;
		for (u8 matchOp : matchingOpcodes)
		{
			if (op == matchOp) { opcodeMatch = true; break; }
		}

		if (opcodeMatch)
		{
			// Check execute filter
			bool isExec = false;
			if (debugMemory)
			{
				CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
				if (cell) isExec = cell->isExecuteCode;
			}

			if (!executedOnly || isExec)
			{
				// Read operand bytes
				u8 lo = 0, hi = 0;
				int len = opcodes[op].addressingLength;
				if (len >= 2 && addr + 1 < memLength)
					dataAdapter->AdapterReadByte(addr + 1, &lo);
				if (len >= 3 && addr + 2 < memLength)
					dataAdapter->AdapterReadByte(addr + 2, &hi);

				// Check argument filter
				bool argMatch = true;
				if (filterValue >= 0)
				{
					u16 operand;
					if (opcodes[op].addressingMode == ADDR_REL)
						operand = (addr + 2 + (int8)lo) & 0xFFFF;
					else if (len == 2)
						operand = lo;
					else if (len == 3)
						operand = (hi << 8) | lo;
					else
						operand = 0;

					argMatch = ((int)operand == filterValue);
				}

				if (argMatch)
				{
					json match;
					char addrHex[8]; sprintf(addrHex, "%04X", addr);
					match["addr"] = addrHex;
					match["isExecuted"] = isExec;

					// Build disassembly text for context
					match["disassembly"] = FormatDisassemblyLine(addr, op, lo, hi, true, false, isExec);
					matches.push_back(match);
				}
			}

			addr += opcodes[op].addressingLength;
		}
		else
		{
			// If we're in executed code, skip by instruction length; otherwise advance 1 byte
			bool isExec = false;
			if (debugMemory)
			{
				CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
				if (cell) isExec = cell->isExecuteCode;
			}

			if (isExec)
				addr += opcodes[op].addressingLength;
			else
				addr++;
		}
	}

	json result;
	result["pattern"] = pattern;
	result["mnemonic"] = mnemonicBuf;
	result["matches"] = matches;
	result["matchCount"] = (int)matches.size();
	result["truncated"] = ((int)matches.size() >= maxResults);
	return result;
}
