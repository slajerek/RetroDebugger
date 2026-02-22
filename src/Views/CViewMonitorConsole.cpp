#include "CViewMonitorConsole.h"
#include "CGuiViewConsole.h"
#include "CSlrString.h"
#include "CDebugInterfaceC64.h"
#include "SYS_FileSystem.h"
#include "C64SettingsStorage.h"
#include "CSlrFileFromOS.h"
#include "CViewDataDump.h"
#include "CDebugMemory.h"
#include "CGuiMain.h"
#include "CViewDisassembly.h"
#include "CViewDataMap.h"
#include "CViewC64StateCPU.h"
#include "CViewDrive1541StateCPU.h"
#include "C64Tools.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugMemoryCell.h"
#include "CLayoutParameter.h"
#include "EmulatorsConfig.h"
#include "SYS_Funct.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsData.h"

#define C64DEBUGGER_MONITOR_HISTORY_FILE_VERSION	1

#define C64MONITOR_DEVICE_C64			1
#define C64MONITOR_DEVICE_DISK1541_8	2

#define C64MONITOR_DUMP_MEMORY_STEP		0x0100
#define C64MONITOR_DISASSEMBLY_STEP		0x0010

enum monitorSaveFileType
{
	MONITOR_SAVE_FILE_MEMORY_DUMP,
	MONITOR_SAVE_FILE_DISASSEMBLY
};

CViewMonitorConsole::CViewMonitorConsole(char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
//	this->name = "CViewMonitorConsole";
	this->name = name;
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->viewConsole = new CGuiViewConsole(posX, posY, posZ, sizeX, sizeY, viewC64->fontDefaultCBMShifted, 2.00f, 10, true, this);
	
	this->viewConsole->SetPrompt(".");

	this->viewConsole->textColorR = 0.23f;
	this->viewConsole->textColorG = 0.988f;
	this->viewConsole->textColorB = 0.203f;
	this->viewConsole->textColorA = 1.0f;

	addrStart = addrEnd = 0;
	assembleNextAddress = -1;

	this->device = C64MONITOR_DEVICE_C64;
	this->dataAdapter = debugInterface->GetDataAdapter();
	
	memoryExtensions.push_back(new CSlrString("bin"));
	prgExtensions.push_back(new CSlrString("prg"));
	disassemblyExtensions.push_back(new CSlrString("asm"));
	
	debugInterface->SetCodeMonitorCallback(this);
	
	fontScale = 1.90f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Scale", &fontScale));

	this->viewConsole->SetFontScale(fontScale);

	PrintInitPrompt();
	if (! (c64SettingsUseNativeEmulatorMonitor && debugInterface->IsCodeMonitorSupported()))
	{
		CommandHelp();
	}
	
	RestoreMonitorHistory();
}

void CViewMonitorConsole::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY, float fontScale)
{
	this->viewConsole->SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	this->fontScale = fontScale;
	this->viewConsole->SetFontScale(fontScale);
//	this->viewConsole->SetNumLines(floor((float)sizeY / (float)this->viewConsole->lineHeight) - 2);
	
//	this->viewConsole->lineHeight += 8;
	
	CGuiElement::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewMonitorConsole::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	this->viewConsole->SetPosition(posX, posY, posZ, sizeX, sizeY);
	CGuiElement::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewMonitorConsole::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	this->viewConsole->SetFontScale(fontScale);
}


void CViewMonitorConsole::ActivateView()
{
	UpdateDataAdapters();
}


bool CViewMonitorConsole::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewMonitorConsole::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return this->viewConsole->KeyDown(keyCode);
}

bool CViewMonitorConsole::KeyTextInput(const char *text)
{
	// NOTE: this is really bad hack and does not support UTF characters
	u32 key;
 
	//LOGD("commandLineCursorPos=%d", viewConsole->commandLineCursorPos);

	if (c64SettingsUseNativeEmulatorMonitor && debugInterface->IsCodeMonitorSupported())
	{
		key = text[0];
	}
	else
	{
		// hack for case-sensitive file names ;)
		if (viewConsole->commandLine[0] == 'S')
		{
			// 0123456789012345678
			// S XXXX YYYY filename
			// S PRG XXXX YYYY filename
			
			if (viewConsole->commandLine[2] != 'P' && viewConsole->commandLineCursorPos >= 11)
			{
				key = text[0];
			}
			else if (viewConsole->commandLine[2] == 'P' && viewConsole->commandLineCursorPos >= 15)
			{
				key = text[0];
			}
			else
			{
				key = toupper(text[0]);
			}
		}
		else if (viewConsole->commandLine[0] == 'L')
		{
			// 0123456789012345678
			// L XXXX filename
			// L PRG filename
			if (viewConsole->commandLine[2] != 'P' && viewConsole->commandLineCursorPos >= 6)
			{
				key = text[0];
			}
			else if (viewConsole->commandLine[2] == 'P' && viewConsole->commandLineCursorPos >= 5)
			{
				key = text[0];
			}
			else
			{
				key = toupper(text[0]);
			}
		}
		else
		{
			key = toupper(text[0]);
		}
	}

	// TODO: this is ugly, and not supported with UTF
	char buf[32] = {0};
	strncpy(buf, text, 31);
	buf[0] = text[0];
	return viewConsole->KeyTextInput(buf);
}

void CViewMonitorConsole::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewMonitorConsole::Render()
{
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, 0.15f, 0.15f, 0.15f, 1.0f);
	this->viewConsole->Render();
}

bool CViewMonitorConsole::DoScrollWheel(float deltaX, float deltaY)
{
	return this->viewConsole->DoScrollWheel(deltaX, deltaY);
}

void CViewMonitorConsole::PrintInitPrompt()
{
	if (c64SettingsUseNativeEmulatorMonitor && debugInterface->IsCodeMonitorSupported())
	{
		CSlrString *str = debugInterface->GetEmulatorVersionString();
		str->Concatenate(" monitor");
		this->viewConsole->PrintLine(str);
		
//		CSlrString *promptStr = debugInterface->GetCodeMonitorPrompt();
//		this->viewConsole->PrintLine(promptStr);
//		delete promptStr;
	}
	else
	{
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "RetroDebugger v%s monitor", RETRODEBUGGER_VERSION_STRING);
		this->viewConsole->PrintLine(buf);
	}
}

void CViewMonitorConsole::CodeMonitorCallbackPrintLine(CSlrString *printLine)
{
	this->viewConsole->PrintLine(printLine);
}

void CViewMonitorConsole::GuiViewConsoleExecuteCommand(char *commandText)
{
	UpdateDataAdapters();
	
	this->viewConsole->mutex->Lock();
	
	// empty command text?
	if (commandText[0] == 0x00)
	{
		// check previous command
		if (viewConsole->commandLineHistory.size() > 0
			&& viewConsole->commandLineHistoryIt == viewConsole->commandLineHistory.end())
		{
			std::list<char *>::iterator commandLineHistoryIt = viewConsole->commandLineHistory.end();
			commandLineHistoryIt--;
			if ((*commandLineHistoryIt)[0] == 'D' || (*commandLineHistoryIt)[0] == 'd')
			{
				addrStart = addrEnd;
				addrEnd = addrStart + C64MONITOR_DISASSEMBLY_STEP;
				DoDisassembleMemory(addrStart, addrEnd, false, NULL);

				this->viewConsole->mutex->Unlock();
				return;
			}
			else if ((*commandLineHistoryIt)[0] == 'M' || (*commandLineHistoryIt)[0] == 'm')
			{
				addrStart = addrEnd;
				addrEnd = addrStart + C64MONITOR_DUMP_MEMORY_STEP;
				DoMemoryDump(addrStart, addrEnd);
				
				this->viewConsole->mutex->Unlock();
				return;
			}
		}

	}
	
	
	// Echo the command line (skip for assemble command — it prints its own output)
	if (commandText[0] != 'A' && commandText[0] != 'a')
	{
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "%s%s", this->viewConsole->prompt, commandText);
		this->viewConsole->PrintLine(buf);
		SYS_ReleaseCharBuf(buf);
	}

	if (c64SettingsUseNativeEmulatorMonitor && debugInterface->IsCodeMonitorSupported())
	{
		this->viewConsole->mutex->Unlock();

		if (!strcmp(commandText, "C64D") || !strcmp(commandText, "c64d"))
		{
			c64SettingsUseNativeEmulatorMonitor = false;
			this->viewConsole->ResetCommandLine();
			this->PrintInitPrompt();
			C64DebuggerStoreSettings();
			return;
		}
		
		CSlrString *commandStr = new CSlrString(commandText);
		debugInterface->ExecuteCodeMonitorCommand(commandStr);
		delete commandStr;
		
		CSlrString *promptStr = debugInterface->GetCodeMonitorPrompt();
		this->viewConsole->PrintLine(promptStr);
		delete promptStr;

		this->viewConsole->ResetCommandLine();
		StoreMonitorHistory();
		
		return;
	}
	
	// tokenize command
	tokenIndex = 0;
	strCommandText = new CSlrString(commandText);
	tokens = strCommandText->Split(' ');
	
	// interpret
	if (tokens->size() > 0)
	{
		CSlrString *token = (*tokens)[tokenIndex];
		
		tokenIndex++;
		
		if (token->CompareWith("HELP") || token->CompareWith("help")
			|| token->CompareWith("?"))
		{
			CommandHelp();
		}
		else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE
				 && (token->CompareWith("DEVICE") || token->CompareWith("device")))
		{
			CommandDevice();
		}
		else if (token->CompareWith("F") || token->CompareWith("f"))
		{
			CommandFill();
		}
		else if (token->CompareWith("C") || token->CompareWith("c"))
		{
			CommandCompare();
		}
		else if (token->CompareWith("T") || token->CompareWith("t"))
		{
			CommandTransfer();
		}
		else if (token->CompareWith("H") || token->CompareWith("h"))
		{
			CommandHunt();
		}
		else if (token->CompareWith("HC") || token->CompareWith("hc"))
		{
			CommandHuntContinue();
		}
		else if (token->CompareWith("S") || token->CompareWith("s"))
		{
			CommandMemorySave();
		}
		else if (token->CompareWith("SPRG") || token->CompareWith("sprg"))
		{
			CommandMemorySavePRG();
		}
		else if (token->CompareWith("L") || token->CompareWith("l"))
		{
			CommandMemoryLoad();
		}
		else if (token->CompareWith("G") || token->CompareWith("g"))
		{
			CommandGoJMP();
		}
		else if (token->CompareWith("D") || token->CompareWith("d"))
		{
			CommandDisassemble();
		}
		else if (token->CompareWith("D?") || token->CompareWith("d?"))
		{
			this->viewConsole->PrintLine("Usage: D [NOHEX] <from address> <to address> [file name]");
		}
		else if (token->CompareWith("M") || token->CompareWith("m"))
		{
			CommandMemoryDump();
		}
		else if (token->CompareWith("A") || token->CompareWith("a"))
		{
			CommandAssemble();
		}
		else if (token->CompareWith("B") || token->CompareWith("b"))
		{
			CommandBreakpoint();
		}

		else if (token->CompareWith("VICE") || token->CompareWith("vice"))
		{
			c64SettingsUseNativeEmulatorMonitor = true;
			this->PrintInitPrompt();
			C64DebuggerStoreSettings();
		}
		else
		{
			this->viewConsole->PrintLine("Unknown command.");
			LOGD("commandText='%s'", commandText);
			token->DebugPrint("token=");
		}
	}
	
	// delete tokens
	while(!tokens->empty())
	{
		CSlrString *token = tokens->back();
		tokens->pop_back();
		delete token;
	}
	
	delete tokens; tokens = NULL;
	delete strCommandText; strCommandText = NULL;
	
	this->viewConsole->ResetCommandLine();

	if (assembleNextAddress >= 0)
	{
		sprintf(viewConsole->commandLine, "A %04X ", assembleNextAddress);
		viewConsole->commandLineCursorPos = (int)strlen(viewConsole->commandLine);
		assembleNextAddress = -1;
	}

	StoreMonitorHistory();

	this->viewConsole->mutex->Unlock();
}

bool CViewMonitorConsole::GetToken(CSlrString **token)
{
	if (tokenIndex >= tokens->size())
	{
		return false;
	}
	
	*token = (*tokens)[tokenIndex];

	return true;
}

bool CViewMonitorConsole::GetTokenValueHex(int *value)
{
	CSlrString *token;
	
	if (GetToken(&token) == false)
		return false;
	
	char *hexStr = token->GetStdASCII();
	
	// check chars
	for (int i = 0; i < strlen(hexStr); i++)
	{
		if ((hexStr[i] >= '0' && hexStr[i] <= '9')
			|| (hexStr[i] >= 'A' && hexStr[i] <= 'F')
			|| (hexStr[i] >= 'a' && hexStr[i] <= 'f'))
			continue;
		
		delete hexStr;
		return false;
	}
	
	sscanf(hexStr, "%x", value);

	delete [] hexStr;
	
	tokenIndex++;
	
	return true;
}

void CViewMonitorConsole::CommandHelp()
{
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		this->viewConsole->PrintLine("DEVICE C / D / 8");
		this->viewConsole->PrintLine("    set current device (C64/Disk/Disk)");
	}
	this->viewConsole->PrintLine("F <from address> <to address> <value>");
	this->viewConsole->PrintLine("    fill memory with value");
	this->viewConsole->PrintLine("C <from address> <to address> <destination address>");
	this->viewConsole->PrintLine("    compare memory with memory");
	this->viewConsole->PrintLine("H <from address> <to address> <value> [<value> ...]");
	this->viewConsole->PrintLine("    compare memory with values");
	this->viewConsole->PrintLine("HC <from address> <to address> <value> [<value> ...]");
	this->viewConsole->PrintLine("    find values that addresses are in previous hunt");
	this->viewConsole->PrintLine("T <from address> <to address> <destination address>");
	this->viewConsole->PrintLine("    copy memory");
	this->viewConsole->PrintLine("L [PRG] [to address] [file name]");
	this->viewConsole->PrintLine("    load memory");
	this->viewConsole->PrintLine("S [PRG] <from address> <to address> [file name]");
	this->viewConsole->PrintLine("    save memory (with option as PRG file)");
//	this->viewConsole->PrintLine("SPRG <from address> <to address> [file name]");
//	this->viewConsole->PrintLine("    save memory as PRG file");
	this->viewConsole->PrintLine("M [from address] [to address]");
	this->viewConsole->PrintLine("D [NH] <from address> <to address> [file name]");
	this->viewConsole->PrintLine("    disassemble memory (with option NH without hex)");
	this->viewConsole->PrintLine("G <address>");
	this->viewConsole->PrintLine("    jmp to address");
	this->viewConsole->PrintLine("A <address> <mnemonic> [operand]");
	this->viewConsole->PrintLine("    assemble instruction at address");
	this->viewConsole->PrintLine("B [<address>] [<address><op><value>]");
	this->viewConsole->PrintLine("    list, toggle PC, or set memory breakpoint");
	this->viewConsole->PrintLine("    ops: = == != < <= > >=");
}

void CViewMonitorConsole::CommandGoJMP()
{
	int addrStart;
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: G <address>");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad address value.");
		return;
	}
	
	// TODO: generalize me (add drive debug interface)
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		if (this->device == C64MONITOR_DEVICE_C64)
		{
			viewC64->debugInterfaceC64->MakeJmpNoResetC64(addrStart);
		}
		else
		{
			viewC64->debugInterfaceC64->MakeJmpNoReset1541(addrStart);
		}
	}
	else
	{
		debugInterface->MakeJmpNoReset(debugInterface->GetDataAdapter(), addrStart);
	}
}


void CViewMonitorConsole::CommandDevice()
{
	CSlrString *token;
	
	if (!GetToken(&token))
	{
		if (this->device == C64MONITOR_DEVICE_C64)
		{
			this->viewConsole->PrintLine("Current device: C64");
		}
		else
		{
			this->viewConsole->PrintLine("Current device: 1541 DISK (8)");
		}
		return;
	}
	
	if (token->CompareWith("C") || token->CompareWith("c"))
	{
		this->device = C64MONITOR_DEVICE_C64;
	}
	else if (token->CompareWith("D") || token->CompareWith("d") || token->CompareWith("8"))
	{
		this->device = C64MONITOR_DEVICE_DISK1541_8;
	}
	else
	{
		this->viewConsole->PrintLine("Usage: DEVICE C|D|8");
		return;
	}
	
	UpdateDataAdapters();
}

void CViewMonitorConsole::UpdateDataAdapters()
{
	// TODO: generalize me, add drive data adapters
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		bool v1, v2;
		if (device == C64MONITOR_DEVICE_C64)
		{
			this->dataAdapter = viewC64->viewC64MemoryDataDump->dataAdapter;
			
			v1 = true;
			v2 = false;
		}
		else if (device == C64MONITOR_DEVICE_DISK1541_8)
		{
			this->dataAdapter = viewC64->viewDrive1541MemoryDataDump->dataAdapter;
			
			v1 = false;
			v2 = true;
		}
		
		bool v;
		v = v1;
//		viewC64->viewC64StateCPU->SetVisible(v);
//		viewC64->viewC64Disassembly->SetVisible(v);
//		viewC64->viewC64MemoryDataDump->SetVisible(v);
//		viewC64->viewC64MemoryMap->SetVisible(v);
//		viewC64->debugInterfaceC64->isDebugOn = v;
		v = v2;
//		viewC64->viewDriveStateCPU->SetVisible(v);
//		viewC64->viewDrive1541Disassembly->SetVisible(v);
//		viewC64->viewDrive1541MemoryDataDump->SetVisible(v);
//		viewC64->viewDrive1541MemoryMap->SetVisible(v);
//		viewC64->debugInterfaceC64->debugOnDrive1541 = v;
	}
	else
	{
		this->dataAdapter = debugInterface->GetDataAdapter();
	}
}

void CViewMonitorConsole::CommandFill()
{
	int addrStart, addrEnd, value;
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: F <from address> <to address> <value>");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}

	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}

	//
	if (GetTokenValueHex(&value) == false)
	{
		this->viewConsole->PrintLine("Missing fill value.");
		return;
	}
	
	if (value < 0x00 || value > 0xFF)
	{
		this->viewConsole->PrintLine("Bad fill value.");
		return;
	}
	
	if (addrEnd < addrStart)
	{
		this->viewConsole->PrintLine("Usage: F <from address> <to address> <value>");
		return;
	}

	LOGD("Fill: %04x %04x %02x", addrStart, addrEnd, value);
	
	bool avail;
	
	int i = addrStart;
	
	do
	{
		dataAdapter->AdapterWriteByte(i, value, &avail);
		i++;
	}
	while (i < addrEnd);
}

void CViewMonitorConsole::CommandCompare()
{
	int addrStart, addrEnd, addrDestination;
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: C <from address> <to address> <destination address>");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrDestination) == false)
	{
		this->viewConsole->PrintLine("Missing 'destination' address value.");
		return;
	}
	
	if (addrDestination < 0x0000 || addrDestination > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'destination' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("From address must be less than to address.");
		this->viewConsole->PrintLine("Usage: C <from address> <to address> <destination address>");
		return;
	}
	
	LOGD("Compare: %04x %04x %04x", addrStart, addrEnd, addrDestination);
	
	bool a;
	
	char *buf = SYS_GetCharBuf();
	
	int len = addrEnd - addrStart;
	
	int addr1 = addrStart;
	int addr2 = addrDestination;
	
	for (int i = 0; i < len; i++)
	{
		uint8 v1, v2;
		dataAdapter->AdapterReadByte(addr1, &v1, &a);
		dataAdapter->AdapterReadByte(addr2, &v2, &a);
		
		if (v1 != v2)
		{
			sprintf (buf, " %04X %04X %02X %02X", addr1, addr2, v1, v2);
			viewConsole->PrintLine(buf);
		}
		
		addr1++;
		addr2++;
	}
	
	SYS_ReleaseCharBuf(buf);
}

void CViewMonitorConsole::CommandTransfer()
{
	int addrStart, addrEnd, addrDestination;
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: T <from address> <to address> <destination address>");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrDestination) == false)
	{
		this->viewConsole->PrintLine("Missing 'destination' address value.");
		return;
	}
	
	if (addrDestination < 0x0000 || addrDestination > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'destination' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("From address must be less than to address.");
		this->viewConsole->PrintLine("Usage: T <from address> <to address> <destination address>");
		return;
	}
	
	LOGD("Transfer: %04x %04x %04x", addrStart, addrEnd, addrDestination);
	
	bool a;
	
	int len = addrEnd - addrStart;
	
	uint8 *memoryBuffer = new uint8[0x10000];
	dataAdapter->AdapterReadBlockDirect(memoryBuffer, addrStart, addrEnd);
	
	uint8 *writeBuffer = new uint8[len];
	memcpy(writeBuffer, memoryBuffer + addrStart, len);

	int addr = addrDestination;
	for (int i = 0; i < len; i++)
	{
		dataAdapter->AdapterWriteByte(addr, writeBuffer[i], &a);
		addr++;
	}
	
	delete [] memoryBuffer;
	delete [] writeBuffer;
}

void CViewMonitorConsole::CommandHunt()
{
	int addrStart, addrEnd;
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: H <from address> <to address> <value> [<value> ...]");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("From address must be less than to address.");
		this->viewConsole->PrintLine("Usage: H <from address> <to address> <value> [<value> ...]");
		return;
	}

	std::list<int> values;

	int val;
	
	while (GetTokenValueHex(&val))
	{
		if (val >= 0x00 && val <= 0xFF)
		{
			values.push_back(val);
		}
		else if (val >= 0x0100 && val <= 0xFFFF)
		{
			u8 val1 = val & 0x00FF;
			u8 val2 = (val & 0xFF00) >> 8;
			
			values.push_back(val1);
			values.push_back(val2);
		}
		else
		{
			this->viewConsole->PrintLine("Bad hunt value.");
			return;
		}
	}
	
	if (values.size() == 0)
	{
		this->viewConsole->PrintLine("No values entered.");
		this->viewConsole->PrintLine("Usage: H <from address> <to address> <value> [<value> ...]");
		return;
	}
	
	bool a;
	
	char *buf = SYS_GetCharBuf();
	char *buf2 = SYS_GetCharBuf();
	
	int numAddresses = 0;
	previousHuntAddrs.clear();
	
	for (int i = addrStart; i < addrEnd; i++)
	{
		if (i + values.size() > 0xFFFF)
			break;
		
		bool found = true;

		int addr = i;
		
		for (std::list<int>::iterator it = values.begin(); it != values.end(); it++)
		{
			uint8 v;
			dataAdapter->AdapterReadByte(addr, &v, &a);
			
			if (a == false)
			{
				found = false;
				break;
			}
			
			if (v != *it)
			{
				found = false;
				break;
			}
			
			addr++;
		}
		
		if (found)
		{
			previousHuntAddrs[i] = i;
			
			sprintf(buf2, " %04X", i);
			strcat(buf, buf2);
			numAddresses++;
			
			if (numAddresses == 8)	
			{
				viewConsole->PrintLine(buf);
				buf[0] = 0x00;
				numAddresses = 0;
			}
		}
	}
	
	if (numAddresses != 0)
	{
		viewConsole->PrintLine(buf);
	}
	
	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(buf2);
}

void CViewMonitorConsole::CommandHuntContinue()
{
	int addrStart, addrEnd;
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: HC <from address> <to address> <value> [<value> ...]");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("From address must be less than to address.");
		this->viewConsole->PrintLine("Usage: HC <from address> <to address> <value> [<value> ...]");
		return;
	}
	
	std::list<int> values;
	
	int val;
	
	while (GetTokenValueHex(&val))
	{
		if (val >= 0x00 && val <= 0xFF)
		{
			values.push_back(val);
		}
		else if (val >= 0x0100 && val <= 0xFFFF)
		{
			u8 val1 = val & 0x00FF;
			u8 val2 = (val & 0xFF00) >> 8;
			
			values.push_back(val1);
			values.push_back(val2);
		}
		else
		{
			this->viewConsole->PrintLine("Bad hunt value.");
			return;
		}
	}
	
	if (values.size() == 0)
	{
		this->viewConsole->PrintLine("No values entered.");
		this->viewConsole->PrintLine("Usage: HC <from address> <to address> <value> [<value> ...]");
		return;
	}
	
	bool a;
	
	char *buf = SYS_GetCharBuf();
	char *buf2 = SYS_GetCharBuf();
	
	int numAddresses = 0;
	
	std::list<int> foundAddresses;
	
	for (int i = addrStart; i < addrEnd; i++)
	{
		if (i + values.size() > 0xFFFF)
			break;
		
		bool found = true;
		
		int addr = i;
		
		for (std::list<int>::iterator it = values.begin(); it != values.end(); it++)
		{
			uint8 v;
			dataAdapter->AdapterReadByte(addr, &v, &a);
			
			if (a == false)
			{
				found = false;
				break;
			}
			
			if (v != *it)
			{
				found = false;
				break;
			}
			
			addr++;
		}
		
		if (found)
		{
			// check if found addr is already in previous hunt results
			std::map<int, int>::iterator it = previousHuntAddrs.find(i);
			if (it != previousHuntAddrs.end())
			{
				// value found in previous hunt
				foundAddresses.push_back(i);
				
				sprintf(buf2, " %04X", i);
				strcat(buf, buf2);
				numAddresses++;
			
				if (numAddresses == 8)
				{
					viewConsole->PrintLine(buf);
					buf[0] = 0x00;
					numAddresses = 0;
				}
			}
		}
	}
	
	if (numAddresses != 0)
	{
		viewConsole->PrintLine(buf);
		
		// copy over previous hunt
		previousHuntAddrs.clear();
		
		for (std::list<int>::iterator it = foundAddresses.begin(); it != foundAddresses.end(); it++)
		{
			int addr = *it;
			previousHuntAddrs[addr] = addr;
		}
	}
	
	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(buf2);
}

void CViewMonitorConsole::CommandMemoryDump()
{
	int continueStep = C64MONITOR_DUMP_MEMORY_STEP;
	
	CSlrString *token;
	if (GetToken(&token) == false)
	{
		addrStart = addrEnd;
		addrEnd = addrStart + continueStep;
		goto viewMonitorConsoleParsedMemoryDump;
	}
	
	// get help
	if (token->CompareWith("?"))
	{
		this->viewConsole->PrintLine("Usage: M [from address] [to address]");
		return;
	}
	
	if (GetTokenValueHex(&addrStart) == false)
	{
//		this->viewConsole->PrintLine("Usage: M <from address> [to address]");
//		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		addrEnd = addrStart + continueStep;
	}
	
viewMonitorConsoleParsedMemoryDump:

	if (addrEnd < 0x0000)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("Usage: M [from address] [to address]");
		return;
	}
	
	if (addrEnd > 0xFFFF)
	{
		addrEnd = 0xFFFF;
	}
	
	DoMemoryDump(addrStart, addrEnd);
}

void CViewMonitorConsole::DoMemoryDump(int addrStart, int addrEnd)
{
	memoryLength = dataAdapter->AdapterGetDataLength();

	memory = new uint8[0x10000];
	dataAdapter->AdapterReadBlockDirect(memory, 0x0000, 0xFFFF);

	char *buf = SYS_GetCharBuf();
	
	int numHexCodesPerLine = 16;
	
	int renderAddr = addrStart;
	while(renderAddr < addrEnd)
	{
		int s = renderAddr;
		int p = 0;
		
		sprintfHexCode16(buf, renderAddr); p += 4;
		buf[p++] = ' ';

		for (int i = 0; i < numHexCodesPerLine; i++)
		{
			if (renderAddr > addrEnd)
				break;
			
			sprintfHexCode8(buf + p, memory[renderAddr]); p += 2;
			buf[p++] = ' ';
			
			renderAddr++;
		}
		
		// TODO: we need to change font here... for memory dump.
//		renderAddr = s;
//
//		for (int i = 0; i < numHexCodesPerLine; i++)
//		{
//			if (renderAddr > addrEnd)
//				break;
//			buf[p++] = memory[renderAddr];
//			renderAddr++;
//		}
//		buf[p] = 0x00;
//		LOGD("buf='%s'", buf);

		buf[p-1] = 0x00;
		
		viewConsole->PrintLine(buf);
	}
	
	addrEnd = renderAddr;
	
	delete [] memory;
	SYS_ReleaseCharBuf(buf);
}


void CViewMonitorConsole::CommandMemorySave()
{
	CSlrString *token;
	if (GetToken(&token) == false)
	{
		this->viewConsole->PrintLine("Usage: S [PRG] <from address> <to address> [file name]");
		return;
	}
	
	if (token->CompareWith("prg") || token->CompareWith("PRG"))
	{
		tokenIndex++;
		CommandMemorySavePRG();
		return;
	}
	
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: S [PRG] <from address> <to address> [file name]");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("'to' address must be lower that 'from' address>");
		return;
	}
	
	memoryDumpAsPRG = false;
	CommandMemorySaveDump();
}

void CViewMonitorConsole::CommandMemorySavePRG()
{
	if (GetTokenValueHex(&addrStart) == false)
	{
		this->viewConsole->PrintLine("Usage: S [PRG] <from address> <to address> [file name]");
		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		this->viewConsole->PrintLine("Missing 'to' address value.");
		return;
	}
	
	if (addrEnd < 0x0000 || addrEnd > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("'to' address must be lower that 'from' address>");
		return;
	}
	
	memoryDumpAsPRG = true;
	CommandMemorySaveDump();
}

void CViewMonitorConsole::CommandMemorySaveDump()
{
	CSlrString *fileName = NULL;
	if (GetToken(&fileName) == false)
	{
		// no file name supplied, open dialog
		CSlrString *defaultFileName = new CSlrString("c64memory");
		
		CSlrString *windowTitle = new CSlrString("Dump C64 memory");
		
		saveFileType = MONITOR_SAVE_FILE_MEMORY_DUMP;
		
		if (memoryDumpAsPRG == false)
		{
			viewC64->ShowDialogSaveFile(this, &memoryExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
		}
		else
		{
			viewC64->ShowDialogSaveFile(this, &prgExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
		}
		
		delete windowTitle;
		delete defaultFileName;
	}
	else
	{
		CSlrString *filePath = new CSlrString();

		if (c64SettingsDefaultMemoryDumpFolder != NULL)
		{
			if (SYS_FileDirExists(c64SettingsDefaultMemoryDumpFolder))
			{
				filePath->Concatenate(c64SettingsDefaultMemoryDumpFolder);
			}
		}
		filePath->Concatenate(fileName);
		
		if (DoMemoryDumpToFile(addrStart, addrEnd, memoryDumpAsPRG, filePath) == false)
		{
			char *cPath = filePath->GetStdASCII();
			this->viewConsole->PrintLine("Save memory dump failed to file %s", cPath);
			delete [] cPath;
		}
		
		delete filePath;
	}
}

void CViewMonitorConsole::SystemDialogFileSaveSelected(CSlrString *filePath)
{
	if (saveFileType == MONITOR_SAVE_FILE_MEMORY_DUMP)
	{
		if (DoMemoryDumpToFile(addrStart, addrEnd, memoryDumpAsPRG, filePath) == false)
		{
			char *cPath = filePath->GetStdASCII();
			this->viewConsole->PrintLine("Save memory dump failed to file %s", cPath);
			delete [] cPath;
		}
	}
	else if (saveFileType == MONITOR_SAVE_FILE_DISASSEMBLY)
	{
		
	}

}

void CViewMonitorConsole::SystemDialogFileSaveCancelled()
{
}


bool CViewMonitorConsole::DoMemoryDumpToFile(int addrStart, int addrEnd, bool isPRG, CSlrString *filePath)
{
	filePath->DebugPrint("DoMemoryDumpToFile: ");
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = filePath->GetFilePathWithoutFileNameComponentFromPath();
	C64DebuggerStoreSettings();
	
	// Correct addrEnd:
	// VICE monitor saves memory to file from addrStart to *addrEnd inclusive*
	addrEnd += 1;

	// get file path
	char *cFilePath = filePath->GetStdASCII();
	
	C64SaveMemory(addrStart, addrEnd, isPRG, dataAdapter, cFilePath);
	
	FILE *fp;
	fp = fopen(cFilePath, "wb");

	delete [] cFilePath;

	if (!fp)
	{
		return false;
	}
	
	int len = addrEnd - addrStart;
	uint8 *memoryBuffer = new uint8[0x10000];
	dataAdapter->AdapterReadBlockDirect(memoryBuffer, addrStart, addrEnd);
	
	uint8 *writeBuffer = new uint8[len];
	memcpy(writeBuffer, memoryBuffer + addrStart, len);
	
	if (isPRG)
	{
		// write header
		uint8 buf[2];
		buf[0] = addrStart & 0x00FF;
		buf[1] = (addrStart >> 8) & 0x00FF;
		
		fwrite(buf, 1, 2, fp);
	}
	
	
	int lenWritten = fwrite(writeBuffer, 1, len, fp);
	fclose(fp);
	
	delete [] writeBuffer;
	delete [] memoryBuffer;
	
	if (lenWritten != len)
	{
		return false;
	}
	
	char *buf = SYS_GetCharBuf();
	
	CSlrString *fileName = filePath->GetFileNameComponentFromPath();
	char *cFileName = fileName->GetStdASCII();
	
	sprintf(buf, "Stored %04X bytes to file %s", lenWritten, cFileName);
	viewConsole->PrintLine(buf);
	
	delete [] cFileName;
	delete fileName;
	
	SYS_ReleaseCharBuf(buf);
	
	return true;
}

void CViewMonitorConsole::CommandMemoryLoad()
{
	CSlrString *token;
	if (GetToken(&token) == false)
	{
		//this->viewConsole->PrintLine("Usage: L [PRG] [from addres] [file name]");
		// no tokens - just open LOAD PRG dialog
		viewC64->mainMenuHelper->OpenDialogOpenFile();
		return;
	}

	memoryDumpAsPRG = false;

	if (token->CompareWith("prg") || token->CompareWith("PRG"))
	{
		tokenIndex++;
		
		memoryDumpAsPRG = true;

		addrStart = -1;
	}
	
	if (memoryDumpAsPRG == false || tokens->size() == 4)
	{
		if (GetTokenValueHex(&addrStart) == false)
		{
			this->viewConsole->PrintLine("Usage: L [PRG] [from addres] [file name]");
			return;
		}
		
		if (addrStart < 0x0000 || addrStart > 0xFFFF)
		{
			this->viewConsole->PrintLine("Bad 'from' address value.");
			return;
		}
	}
	
	CommandMemoryLoadDump();
}

void CViewMonitorConsole::CommandMemoryLoadDump()
{
	CSlrString *fileName = NULL;
	if (GetToken(&fileName) == false)
	{
		// no file name supplied, open dialog
		CSlrString *defaultFileName = new CSlrString("c64memory");
		
		CSlrString *windowTitle = new CSlrString("Load C64 memory dump");
		
		if (memoryDumpAsPRG == false)
		{
			viewC64->ShowDialogOpenFile(this, NULL, c64SettingsDefaultMemoryDumpFolder, windowTitle);
		}
		else
		{
			viewC64->ShowDialogOpenFile(this, &prgExtensions, c64SettingsDefaultMemoryDumpFolder, windowTitle);
		}
		
		delete windowTitle;
		delete defaultFileName;
	}
	else
	{
		CSlrString *filePath = new CSlrString();
		
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
		{
			if (SYS_FileDirExists(c64SettingsDefaultMemoryDumpFolder))
			{
				filePath->Concatenate(c64SettingsDefaultMemoryDumpFolder);
			}
		}
		filePath->Concatenate(fileName);
		
		if (DoMemoryDumpFromFile(addrStart, memoryDumpAsPRG, filePath) == false)
		{
			char *cPath = filePath->GetStdASCII();
			this->viewConsole->PrintLine("Loading memory dump failed from file %s", cPath);
			delete [] cPath;
		}
		
		delete filePath;
	}
}

void CViewMonitorConsole::SystemDialogFileOpenSelected(CSlrString *filePath)
{
	if (DoMemoryDumpFromFile(addrStart, memoryDumpAsPRG, filePath) == false)
	{
		char *cPath = filePath->GetStdASCII();
		this->viewConsole->PrintLine("Loading memory dump failed from file %s", cPath);
		delete [] cPath;
	}
}

void CViewMonitorConsole::SystemDialogFileOpenCancelled()
{
}

// addrStart -1 means take start address from PRG
bool CViewMonitorConsole::DoMemoryDumpFromFile(int addrStart, bool isPRG, CSlrString *filePath)
{
	filePath->DebugPrint("DoMemoryDumpFromFile: ");
	
	if (c64SettingsDefaultMemoryDumpFolder != NULL)
		delete c64SettingsDefaultMemoryDumpFolder;
	c64SettingsDefaultMemoryDumpFolder = filePath->GetFilePathWithoutFileNameComponentFromPath();
	C64DebuggerStoreSettings();
	
	char *cFilePath = filePath->GetStdASCII();
	
	CSlrFileFromOS *file = new CSlrFileFromOS(cFilePath);
	
	if (!file->Exists())
	{
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "File does not exist at path: %s", cFilePath);
		viewConsole->PrintLine(buf);
		
		delete [] cFilePath;
		delete file;
		
		SYS_ReleaseCharBuf(buf);
		
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->readFromFileNoHeader(file);
	
	delete file;

	bool a;
	int addr = addrStart;
	int len = byteBuffer->length;
	
	if (isPRG)
	{
		u16 b1 = byteBuffer->GetByte();
		u16 b2 = byteBuffer->GetByte();
		
		u16 loadPoint = (b2 << 8) | b1;
		
		if (addrStart == -1)
		{
			addr = loadPoint;
		}
		
		len -= 2;
	}
	
	for (int i = 0; i < len; i++)
	{
		uint8 val = byteBuffer->GetU8();
		dataAdapter->AdapterWriteByte(addr, val, &a);
		
		if (a == false)
			break;
		
		addr++;
	}
	
	char *buf = SYS_GetCharBuf();
	CSlrString *fileName = filePath->GetFileNameComponentFromPath();
	char *cFileName = fileName->GetStdASCII();

	sprintf(buf, "Read %04X bytes from file %s", len, cFileName);
	viewConsole->PrintLine(buf);
	
	SYS_ReleaseCharBuf(buf);

	delete [] cFileName;
	delete fileName;
	
	return true;
}

void CViewMonitorConsole::StoreMonitorHistory()
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->PutU16(C64DEBUGGER_MONITOR_HISTORY_FILE_VERSION);
	
	byteBuffer->PutByte(this->viewConsole->commandLineHistory.size());
	for (std::list<char *>::iterator it = this->viewConsole->commandLineHistory.begin();
		 it != this->viewConsole->commandLineHistory.end(); it++)
	{
		byteBuffer->PutString(*it);
	}
	
	CSlrString *fileName = new CSlrString("monitor.dat");
	byteBuffer->storeToSettings(fileName);
	delete fileName;
	
	delete byteBuffer;
}

void CViewMonitorConsole::RestoreMonitorHistory()
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	CSlrString *fileName = new CSlrString("monitor.dat");
	byteBuffer->loadFromSettings(fileName);
	delete fileName;
	
	if (byteBuffer->length == 0)
	{
		LOGD("... no monitor settings found");
		delete byteBuffer;
		return;
	}
	
	u16 version = byteBuffer->GetU16();
	if (version != C64DEBUGGER_MONITOR_HISTORY_FILE_VERSION)
	{
		LOGError("C64DebuggerReadSettings: incompatible version %04x", version);
		delete byteBuffer;
		return;
	}
	
	int historySize = byteBuffer->GetByte();
	for (int i = 0; i < historySize; i++)
	{
		char *cmd = byteBuffer->GetString();
		viewConsole->commandLineHistory.push_back(cmd);
	}
	viewConsole->commandLineHistoryIt = viewConsole->commandLineHistory.end();
	
	delete byteBuffer;
}



////

void CViewMonitorConsole::CommandDisassemble()
{
	int continueStep = C64MONITOR_DISASSEMBLY_STEP;
	
	disassembleHexCodes = true;
	
	CSlrString *token;
	if (GetToken(&token) == false)
	{
		addrStart = addrEnd;
		addrEnd = addrStart + continueStep;
		goto viewMonitorConsoleParsedDisassembly;
	}

	// get help
	if (token->CompareWith("?"))
	{
		this->viewConsole->PrintLine("Usage: D [NOHEX] <from address> <to address> [file name]");
		return;
	}
	
	if (token->CompareWith("nohex") || token->CompareWith("NOHEX")
		|| token->CompareWith("nh") || token->CompareWith("NH"))
	{
		tokenIndex++;
		disassembleHexCodes = false;
	}
	
	if (GetTokenValueHex(&addrStart) == false)
	{
//		this->viewConsole->PrintLine("Usage: D [NOHEX] <from address> <to address> [file name]");
//		return;
	}
	
	if (addrStart < 0x0000 || addrStart > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad 'from' address value.");
		return;
	}
	
	//
	if (GetTokenValueHex(&addrEnd) == false)
	{
		addrEnd = addrStart + continueStep;
	}
	
viewMonitorConsoleParsedDisassembly:

	if (addrEnd < 0x0000)
	{
		this->viewConsole->PrintLine("Bad 'to' address value.");
		return;
	}
	
	if (addrEnd <= addrStart)
	{
		this->viewConsole->PrintLine("Usage: D [NOHEX] <from address> [to address] [file name]");
		return;
	}

	if (addrEnd > 0xFFFF)
	{
		addrEnd = 0xFFFF;
	}
	
	CommandDoDisassemble();
}

void CViewMonitorConsole::CommandDoDisassemble()
{
	CSlrString *fileName = NULL;
	if (GetToken(&fileName) == false)
	{
//		// no file name supplied, open dialog
//		CSlrString *defaultFileName = new CSlrString("c64disassembly");
//		
//		CSlrString *windowTitle = new CSlrString("Disassemble C64 memory");
//		
//		viewC64->ShowDialogSaveFile(this, &disassemblyExtensions, defaultFileName, c64SettingsDefaultMemoryDumpFolder, windowTitle);
//		
//		delete windowTitle;
//		delete defaultFileName;
		
		if (DoDisassembleMemory(addrStart, addrEnd, false, NULL) == false)
		{
			this->viewConsole->PrintLine("Disassembling memory failed.");
		}
	}
	else
	{
		CSlrString *filePath = new CSlrString();
		
		if (c64SettingsDefaultMemoryDumpFolder != NULL)
		{
			c64SettingsDefaultMemoryDumpFolder->DebugPrint("c64SettingsDefaultMemoryDumpFolder=");
			
			if (SYS_FileDirExists(c64SettingsDefaultMemoryDumpFolder))
			{
				filePath->Concatenate(c64SettingsDefaultMemoryDumpFolder);
			}
		}
		filePath->Concatenate(fileName);
		
		if (DoDisassembleMemory(addrStart, addrEnd, false, filePath) == false)
		{
			this->viewConsole->PrintLine("Disassembling memory failed.");
		}
	}
}

bool CViewMonitorConsole::DoDisassembleMemory(int startAddress, int endAddress, bool withLabels, CSlrString *filePath)
{
	FILE *fp = NULL;

	if (filePath != NULL)
	{
		filePath->DebugPrint("DoDisassembleMemory: filePath=");

		if (c64SettingsDefaultMemoryDumpFolder != NULL)
			delete c64SettingsDefaultMemoryDumpFolder;
		c64SettingsDefaultMemoryDumpFolder = filePath->GetFilePathWithoutFileNameComponentFromPath();
		C64DebuggerStoreSettings();
		
		// get file path
		char *cFilePath = filePath->GetStdASCII();
		LOGD("cFilePath='%s'", cFilePath);
		
		fp = fopen(cFilePath, "wb");
		
		delete [] cFilePath;
		
		if (!fp)
		{
			return false;
		}
	}
	
	UpdateDebugSymbols();


	memoryLength = dataAdapter->AdapterGetDataLength();

	memory = new uint8[0x10000];
	dataAdapter->AdapterReadBlockDirect(memory, 0x0000, 0xFFFF);
	
	CDebugMemory *debugMemory = debugSymbols->memory;
	
	//// perform disassemble
//	viewConsole->PrintLine("Disassemble %04X to %04X", addrStart, addrEnd);

	CByteBuffer *byteBuffer = new CByteBuffer();
	
	int addr = startAddress;
	int renderAddress = startAddress;
	int i;
	bool done = false;
	uint8 opcode;
	uint8 op[3];

	do
	{
		LOGD("renderAddress=%04x l=%04x endAddress=%04x", renderAddress, memoryLength, endAddress);
		if (addr >= memoryLength)
			break;
		
		if (addr == memoryLength-1)
		{
			DisassembleHexLine(addr, byteBuffer);
			break;
		}
		
		addr = renderAddress;
		
		for (i=0; i<3; i++, addr++)
		{
			if (addr == endAddress)
			{
				done = true;
			}
			
			op[i] = memory[addr];
		}
		
		{
			addr = renderAddress;
			
			// +0
			CDebugMemoryCell *cell0 = debugMemory->GetMemoryCell(addr);	//% memoryLength
			if (cell0->isExecuteCode)
			{
				opcode = memory[addr ];	//% memoryLength
				renderAddress += DisassembleLine(renderAddress, op[0], op[1], op[2], byteBuffer);
				byteBuffer->PutByte('\n');
			}
			else
			{
				// +1
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell( addr+1 );	//% memoryLength
				if (cell1->isExecuteCode)
				{
					// check if at addr is 1-length opcode
					opcode = memory[ (renderAddress) ];	//% memoryLength
					if (opcodes[opcode].addressingLength == 1)
					{
						DisassembleLine(renderAddress, op[0], op[1], op[2], byteBuffer);
						byteBuffer->PutByte('\n');
					}
					else
					{
						DisassembleHexLine(renderAddress, byteBuffer);
						byteBuffer->PutByte('\n');
					}
					
					renderAddress += 1;
					
					addr = renderAddress;
					for (i=0; i<3; i++, addr++)
					{
						if (addr == endAddress)
						{
							done = true;
						}
						
						op[i] = memory[addr];
					}
					
					opcode = memory[ (renderAddress) ];	//% memoryLength
					renderAddress += DisassembleLine(renderAddress, op[0], op[1], op[2], byteBuffer);
					byteBuffer->PutByte('\n');
				}
				else
				{
					// +2
					CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell( addr+2 );	//% memoryLength
					if (cell2->isExecuteCode)
					{
						// check if at addr is 2-length opcode
						opcode = memory[ (renderAddress) ];	//% memoryLength
						if (opcodes[opcode].addressingLength == 2)
						{
							renderAddress += DisassembleLine(renderAddress, op[0], op[1], op[2], byteBuffer);
							byteBuffer->PutByte('\n');
						}
						else
						{
							DisassembleHexLine(renderAddress, byteBuffer);
							byteBuffer->PutByte('\n');
							renderAddress += 1;
							
							DisassembleHexLine(renderAddress, byteBuffer);
							byteBuffer->PutByte('\n');
							renderAddress += 1;
						}
						
						addr = renderAddress;
						for (i=0; i<3; i++, addr++)
						{
							if (addr == endAddress)
							{
								done = true;
							}
							
							op[i] = memory[addr];
						}
						
						opcode = memory[ (renderAddress) ];	//% memoryLength
						renderAddress += DisassembleLine(renderAddress, op[0], op[1], op[2], byteBuffer);
						byteBuffer->PutByte('\n');
					}
					else
					{
						if (cell0->isExecuteArgument == false)
						{
							// execute not found, just render line
							renderAddress += DisassembleLine(renderAddress, op[0], op[1], op[2], byteBuffer);
							byteBuffer->PutByte('\n');
						}
						else
						{
							// it is argument
							DisassembleHexLine(renderAddress, byteBuffer);
							byteBuffer->PutByte('\n');
							renderAddress += 1;
						}
					}
				}
			}
		}
		
		if (renderAddress >= endAddress)
			break;
	}
	while (!done);
	
	LOGD("renderAddress=%04x l=%04x endAddress=%04x (completed)", renderAddress, memoryLength, endAddress);
	addrEnd = renderAddress;
	
	LOGD("new addrEnd=%04x (completed)", addrEnd);

	// end
	byteBuffer->PutByte(0x00);
	byteBuffer->PutByte(0x00);
	
	///
	if (fp != NULL)
	{
		fprintf(fp, "%s", byteBuffer->data);
		fclose(fp);
		
		char *buf = SYS_GetCharBuf();
		
		CSlrString *fileName = filePath->GetFileNameComponentFromPath();
		char *cFileName = fileName->GetStdASCII();
		
		sprintf(buf, "Disassembled %04X to %04X to file %s", addrStart, addrEnd, cFileName);
		viewConsole->PrintLine(buf);
		
		delete [] cFileName;
		delete fileName;
		
		SYS_ReleaseCharBuf(buf);
	}
	else
	{
		viewConsole->PrintString((char*)byteBuffer->data);
	}
	
	delete [] memory;
	
	return true;
}

//

// Disassemble one hex-only value (for disassemble up)
void CViewMonitorConsole::DisassembleHexLine(int addr, CByteBuffer *outBuffer)
{
	//	LOGD("addr=%4.4x op=%2.2x", addr, op);
	
	// check if this 1-length opcode
	uint8 op = memory[ (addr) % memoryLength];
	if (opcodes[op].addressingLength == 1)
	{
		DisassembleLine(addr, op, 0x00, 0x00, outBuffer);
		return;
	}
	
	char buf[128];
	char buf1[16];
	
//	CViewMemoryMapCell *cell = memoryMap->memoryCells[addr];
//	if (cell->isExecuteCode)
	
	
//	if (showLabels)
//	{
//		px += fontSize5*4.0f;
//		
//		std::map<u16, CDisassembleCodeLabel *>::iterator it = codeLabels.find(addr);
//		
//		if (it != codeLabels.end())
//		{
//			CDisassembleCodeLabel *label = it->second;
//			// found a label
//			float pxe = this->posX + fontSize*labelNumCharacters;
//			int l = strlen(label->labelText)+1;
//			float labelX = pxe - l*fontSize;
//			fontDisassemble->BlitTextColor(label->labelText, labelX, py, -1, fontSize, cr, cg, cb, ca);
//		}
//	}
	
	// addr
	sprintfHexCode16(buf, addr);
	DisassemblyPrint(outBuffer, buf);
	DisassemblyPrint(outBuffer, " ");

	if (disassembleHexCodes)
	{
		sprintfHexCode8(buf1, op);
		DisassemblyPrint(outBuffer, buf1);
		DisassemblyPrint(outBuffer, " ");
	}
	
	if (disassembleHexCodes)
	{
		DisassemblyPrint(outBuffer, "       ???");
	}
	else
	{
		sprintfHexCode8(buf1, op);
		DisassemblyPrint(outBuffer, buf1);
	}
	
//	if (addr == currentPC)
//	{
//		BlitFilledRectangle(posX, py, -1.0f,
//							markerSizeX, fontSize, cr, cg, cb, 0.3f);
//	}

}

// Disassemble one instruction, return length
int CViewMonitorConsole::DisassembleLine(int addr, uint8 op, uint8 lo, uint8 hi, CByteBuffer *outBuffer)
{
	//	LOGD("adr=%4.4x op=%2.2x", adr, op);
	
	char buf[128];
	char buf1[16];
	char buf2[2] = {0};
	char buf3[16];
	char buf4[16];
	char bufHexCodes[16];
	int length;
	
//	CViewMemoryMapCell *cell = memoryMap->memoryCells[addr];
//	if (cell->isExecuteCode)
	
	length = opcodes[op].addressingLength;
	
	
	// TODO:
//	if (disassemblyShowLabels)
//	{
//		for (int i = 0; i < length; i++)
//		{
//			std::map<u16, CDisassembleCodeLabel *>::iterator it = codeLabels.find(addr + i);
//			
//			if (it != codeLabels.end())
//			{
//				CDisassembleCodeLabel *label = it->second;
//				// found a label
//				float pxe = this->posX + fontSize*labelNumCharacters;
//				int l = strlen(label->labelText)+1;
//				float labelX = pxe - l*fontSize;
//				fontDisassemble->BlitTextColor(label->labelText, labelX, py, -1, fontSize, cr, cg, cb, ca);
//				
//				break;
//			}
//		}
//	}
	
	
	// addr
	sprintfHexCode16(buf, addr);
	DisassemblyPrint(outBuffer, buf);
	
	DisassemblyPrint(outBuffer, " ");
	
	if (disassembleHexCodes)
	{
		strcpy(buf1, "         ");
		
		switch (length)
		{
			case 1:
				//sprintf(buf1, "%2.2x       ", op);
				// "xx       "
				sprintfHexCode8WithoutZeroEnding(buf1, op);
				break;
				
			case 2:
				//sprintf(buf1, "%2.2x %2.2x    ", op, lo);
				// "xx xx    "
				sprintfHexCode8WithoutZeroEnding(buf1, op);
				sprintfHexCode8WithoutZeroEnding(buf1+3, lo);
				break;
				
			case 3:
				//sprintf(buf1, "%2.2x %2.2x %2.2x ", op, lo, hi);
				// "xx xx xx "
				sprintfHexCode8WithoutZeroEnding(buf1, op);
				sprintfHexCode8WithoutZeroEnding(buf1+3, lo);
				sprintfHexCode8WithoutZeroEnding(buf1+6, hi);
				break;
		}
		
		strcpy(bufHexCodes, buf1);
		strcat(bufHexCodes, buf2);
		DisassemblyPrint(outBuffer, bufHexCodes);
		
		// illegal opcode?
		if (opcodes[op].isIllegal == OP_ILLEGAL)
		{
			DisassemblyPrint(outBuffer, "*");
		}
		else
		{
			DisassemblyPrint(outBuffer, " ");
		}
	}
	
	// mnemonic
	strcpy(buf3, opcodes[op].name);
	strcat(buf3, " ");
	
	
	switch (opcodes[op].addressingMode)
	{
		case ADDR_IMP:
			sprintf(buf4, "");
			break;
			
		case ADDR_IMM:
			//sprintf(buf4, "#%2.2x", lo);
			buf4[0] = '#';
			sprintfHexCode8(buf4+1, lo);
			break;
			
		case ADDR_ZP:
			//sprintf(buf4, "%2.2x", lo);
			sprintfHexCode8(buf4, lo);
			break;
			
		case ADDR_ZPX:
			//sprintf(buf4, "%2.2x,x", lo);
			sprintfHexCode8WithoutZeroEnding(buf4, lo);
			buf4[2] = ',';
			buf4[3] = 'x';
			buf4[4] = 0x00;
			break;
			
		case ADDR_ZPY:
			//sprintf(buf4, "%2.2x,y", lo);
			sprintfHexCode8WithoutZeroEnding(buf4, lo);
			buf4[2] = ',';
			buf4[3] = 'y';
			buf4[4] = 0x00;
			break;
			
		case ADDR_IZX:
			//sprintf(buf4, "(%2.2x,x)", lo);
			buf4[0] = '(';
			sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
			buf4[3] = ',';
			buf4[4] = 'x';
			buf4[5] = ')';
			buf4[6] = 0x00;
			break;
			
		case ADDR_IZY:
			//sprintf(buf4, "(%2.2x),y", lo);
			buf4[0] = '(';
			sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
			buf4[3] = ')';
			buf4[4] = ',';
			buf4[5] = 'y';
			buf4[6] = 0x00;
			break;
			
		case ADDR_ABS:
			//sprintf(buf4, "%4.4x", (hi << 8) | lo);
			sprintfHexCode8WithoutZeroEnding(buf4, hi);
			sprintfHexCode8(buf4+2, lo);
			break;
			
		case ADDR_ABX:
			//sprintf(buf4, "%4.4x,x", (hi << 8) | lo);
			sprintfHexCode8WithoutZeroEnding(buf4, hi);
			sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
			buf4[4] = ',';
			buf4[5] = 'x';
			buf4[6] = 0x00;
			break;
			
		case ADDR_ABY:
			//sprintf(buf4, "%4.4x,y", (hi << 8) | lo);
			sprintfHexCode8WithoutZeroEnding(buf4, hi);
			sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
			buf4[4] = ',';
			buf4[5] = 'y';
			buf4[6] = 0x00;
			break;
			
		case ADDR_IND:
			//sprintf(buf4, "(%4.4x)", (hi << 8) | lo);
			buf4[0] = '(';
			sprintfHexCode8WithoutZeroEnding(buf4+1, hi);
			sprintfHexCode8WithoutZeroEnding(buf4+3, lo);
			buf4[5] = ')';
			buf4[6] = 0x00;
			break;
			
		case ADDR_REL:
			//sprintf(buf4, "%4.4x", ((addr + 2) + (int8)lo) & 0xFFFF);
			sprintfHexCode16(buf4, ((addr + 2) + (int8)lo) & 0xFFFF);
			break;
		default:
			break;
	}
	
	//sprintf(buf, "%s%s", buf3, buf4);
	strcpy(buf, buf3);
	strcat(buf, buf4);
	
	DisassemblyPrint(outBuffer, buf);
	
	int numBytesPerOp = opcodes[op].addressingLength;
	return numBytesPerOp;
}

void CViewMonitorConsole::DisassemblyPrint(CByteBuffer *byteBuffer, char *text)
{
	char *t = text;
	while (*t != '\0')
	{
		byteBuffer->PutByte(*t);
		t++;
	}
}

////
// Breakpoints

void CViewMonitorConsole::UpdateDebugSymbols()
{
	if (this->debugInterface == viewC64->debugInterfaceC64)
	{
		if (device == C64MONITOR_DEVICE_DISK1541_8)
		{
			debugSymbols = viewC64->debugInterfaceC64->symbolsDrive1541;
		}
		else
		{
			debugSymbols = viewC64->debugInterfaceC64->symbols;
		}
	}
	else
	{
		debugSymbols = debugInterface->symbols;
	}
}

static bool IsHexChar(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

void CViewMonitorConsole::CommandBreakpoint()
{
	UpdateDebugSymbols();

	if (debugSymbols == NULL || debugSymbols->currentSegment == NULL)
	{
		this->viewConsole->PrintLine("No debug symbols available.");
		return;
	}

	CDebugSymbolsSegment *segment = debugSymbols->currentSegment;

	CSlrString *token;

	// No arguments → list all breakpoints
	if (!GetToken(&token))
	{
		char *buf = SYS_GetCharBuf();
		bool anyFound = false;

		// iterate all breakpoint types
		for (std::map<int, CDebugBreakpointsAddr *>::iterator typeIt = segment->breakpointsByType.begin();
			 typeIt != segment->breakpointsByType.end(); typeIt++)
		{
			int type = typeIt->first;
			CDebugBreakpointsAddr *bps = typeIt->second;

			for (std::map<int, CDebugBreakpointAddr *>::iterator it = bps->breakpoints.begin();
				 it != bps->breakpoints.end(); it++)
			{
				int addr = it->first;
				CDebugBreakpointAddr *bp = it->second;

				if (type == BREAKPOINT_TYPE_DATA)
				{
					CDebugBreakpointData *dataBp = (CDebugBreakpointData *)bp;
					const char *compStr = CDebugBreakpointsData::DataBreakpointComparisonToStr(dataBp->comparison);
					sprintf(buf, " MEM %04X %s %02X", addr, compStr, dataBp->value);
				}
				else if (type == BREAKPOINT_TYPE_RASTER_LINE)
				{
					sprintf(buf, " RST %04X", addr);
				}
				else
				{
					sprintf(buf, " PC  %04X", addr);
				}

				viewConsole->PrintLine(buf);
				anyFound = true;
			}
		}

		if (!anyFound)
		{
			viewConsole->PrintLine("No breakpoints set.");
		}

		SYS_ReleaseCharBuf(buf);
		return;
	}

	// Join all remaining tokens into one string (no spaces)
	char *rawText = SYS_GetCharBuf();
	rawText[0] = 0x00;
	while (GetToken(&token))
	{
		tokenIndex++;
		char *tokenStr = token->GetStdASCII();
		strcat(rawText, tokenStr);
		delete [] tokenStr;
	}

	// Strip '$' characters
	char *cleanText = SYS_GetCharBuf();
	char *src = rawText;
	char *dst = cleanText;
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

	// Find the first non-hex character to split address from operator
	int i = 0;
	while (cleanText[i] && IsHexChar(cleanText[i]))
	{
		i++;
	}

	if (i == 0)
	{
		this->viewConsole->PrintLine("Bad address value.");
		SYS_ReleaseCharBuf(rawText);
		SYS_ReleaseCharBuf(cleanText);
		return;
	}

	// Parse address
	char addrStr[8];
	if (i > 6) i = 6;
	strncpy(addrStr, cleanText, i);
	addrStr[i] = 0x00;
	int addr;
	sscanf(addrStr, "%x", &addr);

	if (addr < 0x0000 || addr > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad address value.");
		SYS_ReleaseCharBuf(rawText);
		SYS_ReleaseCharBuf(cleanText);
		return;
	}

	// No operator → toggle PC breakpoint
	if (cleanText[i] == 0x00)
	{
		char *buf = SYS_GetCharBuf();

		debugInterface->LockMutex();

		CDebugBreakpointsAddr *breakpointsPC = segment->breakpointsPC;
		CDebugBreakpointAddr *existingBp = breakpointsPC->GetBreakpoint(addr);

		if (existingBp != NULL)
		{
			breakpointsPC->renderBreakpoints.erase(addr);
			breakpointsPC->DeleteBreakpoint(addr);
			sprintf(buf, "Breakpoint removed at %04X", addr);
		}
		else
		{
			CDebugBreakpointAddr *bp = new CDebugBreakpointAddr(debugSymbols, addr);
			bp->actions = ADDR_BREAKPOINT_ACTION_STOP;
			breakpointsPC->AddBreakpoint(bp);
			breakpointsPC->renderBreakpoints[addr] = bp;
			sprintf(buf, "Breakpoint set at %04X", addr);
		}

		debugInterface->UnlockMutex();

		viewConsole->PrintLine(buf);
		SYS_ReleaseCharBuf(buf);
		SYS_ReleaseCharBuf(rawText);
		SYS_ReleaseCharBuf(cleanText);
		return;
	}

	// Parse operator
	DataBreakpointComparison comparison;
	int opLen = 0;
	char opChar = cleanText[i];

	if (opChar == '!' && cleanText[i + 1] == '=')
	{
		comparison = MEMORY_BREAKPOINT_NOT_EQUAL;
		opLen = 2;
	}
	else if (opChar == '=' && cleanText[i + 1] == '=')
	{
		comparison = MEMORY_BREAKPOINT_EQUAL;
		opLen = 2;
	}
	else if (opChar == '=')
	{
		comparison = MEMORY_BREAKPOINT_EQUAL;
		opLen = 1;
	}
	else if (opChar == '<' && cleanText[i + 1] == '=')
	{
		comparison = MEMORY_BREAKPOINT_LESS_OR_EQUAL;
		opLen = 2;
	}
	else if (opChar == '<')
	{
		comparison = MEMORY_BREAKPOINT_LESS;
		opLen = 1;
	}
	else if (opChar == '>' && cleanText[i + 1] == '=')
	{
		comparison = MEMORY_BREAKPOINT_GREATER_OR_EQUAL;
		opLen = 2;
	}
	else if (opChar == '>')
	{
		comparison = MEMORY_BREAKPOINT_GREATER;
		opLen = 1;
	}
	else
	{
		this->viewConsole->PrintLine("Bad operator. Use = == != < <= > >=");
		SYS_ReleaseCharBuf(rawText);
		SYS_ReleaseCharBuf(cleanText);
		return;
	}

	// Parse value after operator
	char *valueStr = cleanText + i + opLen;
	if (*valueStr == 0x00)
	{
		this->viewConsole->PrintLine("Missing value after operator.");
		SYS_ReleaseCharBuf(rawText);
		SYS_ReleaseCharBuf(cleanText);
		return;
	}

	// Verify all chars are hex
	for (int j = 0; valueStr[j]; j++)
	{
		if (!IsHexChar(valueStr[j]))
		{
			this->viewConsole->PrintLine("Bad value.");
			SYS_ReleaseCharBuf(rawText);
			SYS_ReleaseCharBuf(cleanText);
			return;
		}
	}

	int value;
	sscanf(valueStr, "%x", &value);

	if (value < 0x00 || value > 0xFF)
	{
		this->viewConsole->PrintLine("Bad value (must be 00-FF).");
		SYS_ReleaseCharBuf(rawText);
		SYS_ReleaseCharBuf(cleanText);
		return;
	}

	// Add/toggle memory breakpoint
	char *buf = SYS_GetCharBuf();

	debugInterface->LockMutex();

	CDebugBreakpointsData *breakpointsData = segment->breakpointsData;
	CDebugBreakpointAddr *existingBp = breakpointsData->GetBreakpoint(addr);

	if (existingBp != NULL)
	{
		CDebugBreakpointData *existingDataBp = (CDebugBreakpointData *)existingBp;

		if (existingDataBp->comparison == comparison && existingDataBp->value == value)
		{
			// Exact match → remove
			breakpointsData->renderBreakpoints.erase(addr);
			breakpointsData->DeleteBreakpoint(addr);
			const char *compStr = CDebugBreakpointsData::DataBreakpointComparisonToStr(comparison);
			sprintf(buf, "Memory breakpoint removed: %04X %s %02X", addr, compStr, value);
		}
		else
		{
			// Different condition → replace
			breakpointsData->renderBreakpoints.erase(addr);
			breakpointsData->DeleteBreakpoint(addr);
			CDebugBreakpointData *bp = new CDebugBreakpointData(addr, MEMORY_BREAKPOINT_ACCESS_WRITE, comparison, value);
			breakpointsData->AddBreakpoint(bp);
			segment->breakOnMemory = true;
			const char *compStr = CDebugBreakpointsData::DataBreakpointComparisonToStr(comparison);
			sprintf(buf, "Memory breakpoint changed: %04X %s %02X", addr, compStr, value);
		}
	}
	else
	{
		segment->AddBreakpointMemory(addr, MEMORY_BREAKPOINT_ACCESS_WRITE, comparison, value);
		const char *compStr = CDebugBreakpointsData::DataBreakpointComparisonToStr(comparison);
		sprintf(buf, "Memory breakpoint set: %04X %s %02X", addr, compStr, value);
	}

	debugInterface->UnlockMutex();

	viewConsole->PrintLine(buf);
	SYS_ReleaseCharBuf(buf);

	SYS_ReleaseCharBuf(rawText);
	SYS_ReleaseCharBuf(cleanText);
}

////
// Assembler

void CViewMonitorConsole::CommandAssemble()
{
	int assembleAddress;

	if (GetTokenValueHex(&assembleAddress) == false)
	{
		this->viewConsole->PrintLine("Usage: A <address> <mnemonic> [operand]");
		return;
	}

	if (assembleAddress < 0x0000 || assembleAddress > 0xFFFF)
	{
		this->viewConsole->PrintLine("Bad address value.");
		return;
	}

	// Collect remaining tokens as mnemonic string
	char *mnemonicText = SYS_GetCharBuf();
	mnemonicText[0] = 0x00;

	CSlrString *token;
	bool first = true;
	while (GetToken(&token))
	{
		tokenIndex++;
		char *tokenStr = token->GetStdASCII();
		if (!first)
		{
			strcat(mnemonicText, " ");
		}
		strcat(mnemonicText, tokenStr);
		delete [] tokenStr;
		first = false;
	}

	// If no mnemonic, exit assembly mode
	if (mnemonicText[0] == 0x00)
	{
		SYS_ReleaseCharBuf(mnemonicText);
		return;
	}

	// Remove '$' characters (assembler is hex-only)
	char *cleanBuffer = SYS_GetCharBuf();
	char *src = mnemonicText;
	char *dst = cleanBuffer;
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

	// Assemble the instruction
	int instructionOpCode = -1;
	uint16 instructionValue = 0x0000;
	char *errorMessage = SYS_GetCharBuf();

	int ret = AssembleInstruction(assembleAddress, cleanBuffer, &instructionOpCode, &instructionValue, errorMessage);

	if (ret == -1)
	{
		this->viewConsole->PrintLine(errorMessage);
		// Stay in assembly mode at same address
		assembleNextAddress = assembleAddress;
		SYS_ReleaseCharBuf(errorMessage);
		SYS_ReleaseCharBuf(cleanBuffer);
		SYS_ReleaseCharBuf(mnemonicText);
		return;
	}

	// Write bytes to memory
	bool isDataAvailable;
	int instrLength = opcodes[instructionOpCode].addressingLength;

	switch (instrLength)
	{
		case 1:
			dataAdapter->AdapterWriteByte(assembleAddress, instructionOpCode, &isDataAvailable);
			break;
		case 2:
			dataAdapter->AdapterWriteByte(assembleAddress, instructionOpCode, &isDataAvailable);
			dataAdapter->AdapterWriteByte(assembleAddress + 1, (instructionValue & 0xFF), &isDataAvailable);
			break;
		case 3:
			dataAdapter->AdapterWriteByte(assembleAddress, instructionOpCode, &isDataAvailable);
			dataAdapter->AdapterWriteByte(assembleAddress + 1, (instructionValue & 0x00FF), &isDataAvailable);
			dataAdapter->AdapterWriteByte(assembleAddress + 2, ((instructionValue >> 8) & 0x00FF), &isDataAvailable);
			break;
	}

	// Print result line using DisassembleLine
	uint8 op, lo, hi;
	op = lo = hi = 0;
	dataAdapter->AdapterReadByte(assembleAddress, &op, &isDataAvailable);
	if (instrLength >= 2)
		dataAdapter->AdapterReadByte(assembleAddress + 1, &lo, &isDataAvailable);
	if (instrLength >= 3)
		dataAdapter->AdapterReadByte(assembleAddress + 2, &hi, &isDataAvailable);

	CByteBuffer *byteBuffer = new CByteBuffer();
	disassembleHexCodes = true;
	DisassembleLine(assembleAddress, op, lo, hi, byteBuffer);
	byteBuffer->PutByte(0x00);

	char *resultBuf = SYS_GetCharBuf();
	sprintf(resultBuf, ">  %s", (char*)byteBuffer->data);
	this->viewConsole->PrintLine(resultBuf);
	SYS_ReleaseCharBuf(resultBuf);
	delete byteBuffer;

	// Set next address for continuation
	assembleNextAddress = (assembleAddress + instrLength) & 0xFFFF;

	SYS_ReleaseCharBuf(errorMessage);
	SYS_ReleaseCharBuf(cleanBuffer);
	SYS_ReleaseCharBuf(mnemonicText);
}

#define ASSEMBLE_FAIL(ErrorMessage)  delete textParser; \
									strcpy(errorMessageBuf, (ErrorMessage)); \
									return -1;

int CViewMonitorConsole::AssembleInstruction(int assembleAddress, char *lineBuffer, int *instructionOpCode, uint16 *instructionValue, char *errorMessageBuf)
{
	CSlrTextParser *textParser = new CSlrTextParser(lineBuffer);
	textParser->ToUpper();

	char mnemonic[4] = {0x00};
	textParser->GetChars(mnemonic, 3);

	int baseOp = AssembleFindOp(mnemonic);

	if (baseOp < 0)
	{
		ASSEMBLE_FAIL("Unknown mnemonic");
	}

	AssembleToken token = AssembleGetToken(textParser);
	if (token == TOKEN_UNKNOWN)
	{
		ASSEMBLE_FAIL("Bad instruction");
	}

	OpcodeAddressingMode addressingMode = ADDR_UNKNOWN;

	// BRK
	if (token == TOKEN_EOF)
	{
		addressingMode = ADDR_IMP;
	}
	// LDA $00...
	else if (token == TOKEN_HEX_VALUE)
	{
		*instructionValue = textParser->GetHexNumber();

		token = AssembleGetToken(textParser);

		if (token == TOKEN_UNKNOWN)
		{
			ASSEMBLE_FAIL("Bad instruction");
		}
		// LDA $0000
		else if (token == TOKEN_EOF)
		{
			if (*instructionValue < 0x0100)
			{
				addressingMode = ADDR_ZP;
			}
			else
			{
				addressingMode = ADDR_ABS;
			}
		}
		// LDA $0000,
		else if (token == TOKEN_COMMA)
		{
			token = AssembleGetToken(textParser);

			// LDA $0000,X
			if (token == TOKEN_X)
			{
				if (*instructionValue < 0x0100)
				{
					addressingMode = ADDR_ZPX;
				}
				else
				{
					addressingMode = ADDR_ABX;
				}

				token = AssembleGetToken(textParser);
				if (token != TOKEN_EOF)
				{
					ASSEMBLE_FAIL("Extra tokens at end of line");
				}
			}
			// LDA $0000,Y
			else if (token == TOKEN_Y)
			{
				if (*instructionValue < 0x0100)
				{
					addressingMode = ADDR_ZPY;
				}
				else
				{
					addressingMode = ADDR_ABY;
				}

				token = AssembleGetToken(textParser);
				if (token != TOKEN_EOF)
				{
					ASSEMBLE_FAIL("Extra tokens at end of line");
				}
			}
			else
			{
				ASSEMBLE_FAIL("X or Y expected");
			}
		}
		else
		{
			ASSEMBLE_FAIL("Bad instruction");
		}
	}
	// LDA #$00
	else if (token == TOKEN_IMMEDIATE)
	{
		token = AssembleGetToken(textParser);

		if (token == TOKEN_HEX_VALUE)
		{
			*instructionValue = textParser->GetHexNumber();
			addressingMode = ADDR_IMM;

			token = AssembleGetToken(textParser);
			if (token != TOKEN_EOF)
			{
				ASSEMBLE_FAIL("Extra tokens at end of line");
			}
		}
		else
		{
			ASSEMBLE_FAIL("Not a number after #");
		}
	}
	// LDA (
	else if (token == TOKEN_LEFT_PARENTHESIS)
	{
		token = AssembleGetToken(textParser);

		// LDA ($00...
		if (token == TOKEN_HEX_VALUE)
		{
			*instructionValue = textParser->GetHexNumber();

			token = AssembleGetToken(textParser);

			// LDA ($00)...
			if (token == TOKEN_RIGHT_PARENTHESIS)
			{
				token = AssembleGetToken(textParser);
				if (token == TOKEN_EOF)
				{
					addressingMode = ADDR_IND;
				}
				else if (token == TOKEN_COMMA)
				{
					token = AssembleGetToken(textParser);

					// LDA ($00),Y
					if (token == TOKEN_Y)
					{
						addressingMode = ADDR_IZY;
						token = AssembleGetToken(textParser);
						if (token != TOKEN_EOF)
						{
							ASSEMBLE_FAIL("Extra tokens at end of line");
						}
					}
					else
					{
						ASSEMBLE_FAIL("Only Y allowed");
					}
				}
				else
				{
					ASSEMBLE_FAIL("Bad instruction");
				}
			}
			// LDA ($00,X)
			else if (token == TOKEN_COMMA)
			{
				token = AssembleGetToken(textParser);
				if (token == TOKEN_X)
				{
					token = AssembleGetToken(textParser);
					if (token == TOKEN_RIGHT_PARENTHESIS)
					{
						addressingMode = ADDR_IZX;
						token = AssembleGetToken(textParser);
						if (token != TOKEN_EOF)
						{
							ASSEMBLE_FAIL("Extra tokens at end of line");
						}
					}
					else
					{
						ASSEMBLE_FAIL(") expected");
					}
				}
				else
				{
					ASSEMBLE_FAIL("Only X allowed");
				}
			}
			else
			{
				ASSEMBLE_FAIL(") or , expected");
			}
		}
		else
		{
			ASSEMBLE_FAIL("Number expected");
		}
	}
	else
	{
		ASSEMBLE_FAIL("Bad instruction");
	}

	// check branching
	if (addressingMode == ADDR_ABS || addressingMode == ADDR_ZP)
	{
		*instructionOpCode = AssembleFindOp(mnemonic, ADDR_REL);
		if (*instructionOpCode != -1)
		{
			addressingMode = ADDR_REL;
			int16 branchValue = ((*instructionValue) - (assembleAddress + 2)) & 0xFFFF;
			if (branchValue < -0x80 || branchValue > 0x7F)
			{
				ASSEMBLE_FAIL("Branch address too far");
			}
			*instructionValue = branchValue & 0x00FF;
		}
	}

	if (*instructionOpCode == -1)
	{
		*instructionOpCode = AssembleFindOp(mnemonic, addressingMode);

		if (*instructionOpCode == -1)
		{
			*instructionOpCode = AssembleFindOp(mnemonic, ADDR_ABS);
		}
	}

	// found opcode?
	if (*instructionOpCode != -1)
	{
		delete textParser;
		return 0;
	}

	ASSEMBLE_FAIL("Instruction not found");
}

#undef ASSEMBLE_FAIL

int CViewMonitorConsole::AssembleFindOp(char *mnemonic)
{
	for (int i = 0; i < 256; i++)
	{
		const char *m = opcodes[i].name;
		if (!strcmp(mnemonic, m))
			return i;
	}

	return -1;
}

int CViewMonitorConsole::AssembleFindOp(char *mnemonic, OpcodeAddressingMode addressingMode)
{
	// try to find standard opcode first
	for (int i = 0; i < 256; i++)
	{
		const char *m = opcodes[i].name;
		if (!strcmp(mnemonic, m) && opcodes[i].addressingMode == addressingMode
			&& opcodes[i].isIllegal == false)
			return i;
	}

	// then illegals
	for (int i = 0; i < 256; i++)
	{
		const char *m = opcodes[i].name;
		if (!strcmp(mnemonic, m) && opcodes[i].addressingMode == addressingMode)
			return i;
	}

	return -1;
}

AssembleToken CViewMonitorConsole::AssembleGetToken(CSlrTextParser *textParser)
{
	textParser->ScrollWhiteChars();

	char chr = textParser->GetChar();

	if (chr == 0x00)
		return TOKEN_EOF;

	if (FUN_IsHexDigit(chr))
	{
		textParser->ScrollBack();
		return TOKEN_HEX_VALUE;
	}

	if (chr == '#')
		return TOKEN_IMMEDIATE;

	if (chr == '(')
		return TOKEN_LEFT_PARENTHESIS;

	if (chr == ')')
		return TOKEN_RIGHT_PARENTHESIS;

	if (chr == ',')
		return TOKEN_COMMA;

	if (chr == 'X')
		return TOKEN_X;

	if (chr == 'Y')
		return TOKEN_Y;

	return TOKEN_UNKNOWN;
}

// Layout
void CViewMonitorConsole::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewMonitorConsole::Deserialize(CByteBuffer *byteBuffer)
{
}


