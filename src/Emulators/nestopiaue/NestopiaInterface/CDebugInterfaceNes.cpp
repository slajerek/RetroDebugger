#include "NstApiMachine.hpp"
#include "NstMachine.hpp"
#include "NstApiEmulator.hpp"
#include "NstApiVideo.hpp"
#include "NstApiCheats.hpp"
#include "NstApiSound.hpp"
#include "NstApiInput.hpp"
#include "NstApiCartridge.hpp"
#include "NstApiUser.hpp"
#include "NstApiFds.hpp"
#include "NstMachine.hpp"
#include "NstPpu.hpp"
#include "NstCpu.hpp"
#include "CWaveformData.h"

#include "CDataAdapterNesPpuNmt.h"

#include "EmulatorsConfig.h"
#include "CDebugInterfaceNes.h"
#include "CDebugInterfaceNesTasks.h"
#include "RES_ResourceManager.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "SYS_CommandLine.h"
#include "CGuiMain.h"
#include "SYS_KeyCodes.h"
#include "SND_SoundEngine.h"
#include "CSnapshotsManager.h"
#include "C64Tools.h"
#include "C64KeyMap.h"
#include "C64SettingsStorage.h"
#include "CViewC64.h"
#include "SND_Main.h"
#include "CSlrFileFromOS.h"
#include "CDebugSymbols.h"
#include "CDebuggerEmulatorPlugin.h"

#include "CDataAdapterNesRam.h"
#include "NesWrapper.h"
#include "CAudioChannelNes.h"

CDebugInterfaceNes *debugInterfaceNes;
extern Nes::Api::Emulator nesEmulator;

// 44100/50*5
#define NES_APU_WAVEFORM_LENGTH	4410

CDebugInterfaceNes::CDebugInterfaceNes(CViewC64 *viewC64) //, uint8 *memory)
: CDebugInterface(viewC64)
{
	LOGM("CDebugInterfaceNes: NestopiaUE v%s init", NST_VERSION);
	
	debugInterfaceNes = this;
	isInitialised = false;
	
	CreateScreenData();
	
	audioChannel = NULL;
	for (int apuNum = 0; apuNum < MAX_NUM_NES_APUS; apuNum++)
	{
		for (int chanNum = 0; chanNum < 6; chanNum++)
		{
			nesChannelWaveform[apuNum][chanNum] = new CWaveformData(NES_APU_WAVEFORM_LENGTH);
		}
		nesMixWaveform[apuNum] = new CWaveformData(NES_APU_WAVEFORM_LENGTH);
	}

	snapshotsManager = new CSnapshotsManager(this);

	dataAdapter = new CDataAdapterNesRam(this);
	dataAdapterPpuNmt = new CDataAdapterNesPpuNmt(this);
	
	// for loading breakpoints and symbols
	this->symbols = new CDebugSymbols(this, this->dataAdapter);
	this->symbols->CreateDefaultSegment();

	this->symbolsPpuNmt = new CDebugSymbols(this, this->dataAdapterPpuNmt);
	this->symbolsPpuNmt->CreateDefaultSegment();
	
	isDebugOn = true;
	
	if (NestopiaUE_Initialize())
	{
		isInitialised = true;
	}
}

void CDebugInterfaceNes::StepOneCycle()
{
	LOGTODO("CDebugInterfaceNes::StepOneCycle: not implemented");
}

CDebugInterfaceNes::~CDebugInterfaceNes()
{
	debugInterfaceNes = NULL;
	if (screenImage)
	{
		delete screenImage;
	}
	
	if (dataAdapter)
	{
		delete dataAdapter;
	}
	
	if (audioChannel)
	{
		SND_RemoveChannel(audioChannel);
		delete audioChannel;
	}
	
//	Atari800_Exit_Internal(0);
//	
//	SYS_Sleep(100);
}

void CDebugInterfaceNes::RestartEmulation()
{
//	NES_Exit_Internal(0);

	if (audioChannel)
	{
		SND_RemoveChannel(audioChannel);
		delete audioChannel;
	}
	
//	int ret = NES_Initialise(&sysArgc, sysArgv);
//	if (ret != 1)
//	{
//		SYS_FatalExit("NES restart failed, err=%d", ret);
//	}

}

int CDebugInterfaceNes::GetEmulatorType()
{
	return EMULATOR_TYPE_NESTOPIA;
}

CSlrString *CDebugInterfaceNes::GetEmulatorVersionString()
{
	return new CSlrString("NestopiaUE v" NST_VERSION);
}

const char *CDebugInterfaceNes::GetPlatformNameString()
{
	return "NES";
}

bool CDebugInterfaceNes::IsPal()
{
	return nesd_is_pal();
}

float CDebugInterfaceNes::GetEmulationFPS()
{
	if (IsPal())
		return 50.0f;
	
	return 60.0f;
}


double CDebugInterfaceNes::GetCpuClockFrequency()
{
	return nesd_get_cpu_clock_frquency();
}

void CDebugInterfaceNes::RunEmulationThread()
{
	LOGM("CDebugInterfaceNes::RunEmulationThread");
	CDebugInterface::RunEmulationThread();

	this->isRunning = true;

	while (isInitialised == false)
	{
		if (NestopiaUE_Initialize())
		{
			isInitialised = true;
			viewC64->ShowMessageError("disksys.rom missing");
		}
		else
		{
			SYS_Sleep(1000);
		}
	}
	
	NestopiaUE_PostInitialize();

	audioChannel->Start();
	NestopiaUE_Run();
	audioChannel->Stop();

}

void CDebugInterfaceNes::DoFrame()
{
	// perform async tasks
	
	this->LockMutex();
	this->ExecuteVSyncTasks();
	this->UnlockMutex();
	
	CDebugInterface::DoFrame();
}

void CDebugInterfaceNes::RestartAudio()
{
	audioChannel->Start();
	RefreshSync();
}

// reset a/v sync
void CDebugInterfaceNes::RefreshSync()
{
	nesd_reset_sync();
}

//	UBYTE MEMORY_mem[65536 + 2];

void CDebugInterfaceNes::SetByte(uint16 addr, uint8 val)
{
//	u8 *nesRam = nesd_get_ram();
//	nesRam[addr] = val;
	
	Nes::Core::Machine& machine = nesEmulator;
	machine.cpu.map.Poke8_NoMarking(addr, val);
}

uint8 CDebugInterfaceNes::GetByte(uint16 addr)
{
	u8 v;
	
//	u8 *nesRam = nesd_get_ram();
//	v = nesRam[addr];
	
//
//	v = nesd_peek_io(addr);
	
//	LockRenderScreenMutex();
//	LockMutex();
	v = nesd_peek_safe_io(addr);
//	UnlockMutex();
//	UnlockRenderScreenMutex();
	
	return v;
}

void CDebugInterfaceNes::GetMemory(uint8 *buffer, int addrStart, int addrEnd)
{
//	u8 *nesRam = nesd_get_ram();
//
	int addr;
	u8 *bufPtr = buffer + addrStart;
	for (addr = addrStart; addr < addrEnd; addr++)
	{
		*bufPtr++ = GetByte(addr);
	}
}

int CDebugInterfaceNes::GetCpuPC()
{
	return nesd_get_cpu_pc();
}

void CDebugInterfaceNes::GetWholeMemoryMap(uint8 *buffer)
{
//	u8 *nesRam = nesd_get_ram();
//	for (int addr = 0; addr < 0x10000; addr++)
//	{
//		buffer[addr] = nesRam[addr];
//	}

	LockMutex();
	int addr;
	u8 *bufPtr = buffer;
	for (addr = 0; addr < 0x10000; addr++)
	{
		*bufPtr++ = GetByte(addr);
	}
	UnlockMutex();
}

void CDebugInterfaceNes::GetWholeMemoryMapFromRam(uint8 *buffer)
{
	return GetWholeMemoryMap(buffer);
}

void CDebugInterfaceNes::GetCpuRegs(u16 *PC,
				u8 *A,
				u8 *X,
				u8 *Y,
				u8 *P,						/* Processor Status Byte (Partial) */
				u8 *S,
				u8 *IRQ)
{
	return nesd_get_cpu_regs(PC, A, X, Y, P, S, IRQ);
}

void CDebugInterfaceNes::GetPpuClocks(u32 *hClock, u32 *vClock, u32 *cycle)
{
	nesd_get_ppu_clocks(hClock, vClock, cycle);
}

//
int CDebugInterfaceNes::GetScreenSizeX()
{
	return 256;
}

int CDebugInterfaceNes::GetScreenSizeY()
{
	return 240;
}

//
void CDebugInterfaceNes::RefreshScreenNoCallback()
{
	nesd_update_screen(false);
}

//
void CDebugInterfaceNes::SetDebugMode(uint8 debugMode)
{
	LOGD("CDebugInterfaceNes::SetDebugMode: debugMode=%d", debugMode);
	nesd_debug_mode = debugMode;
	
	nesd_reset_sync();
	
	CDebugInterface::SetDebugMode(debugMode);
}

uint8 CDebugInterfaceNes::GetDebugMode()
{
	this->debugMode = nesd_debug_mode;
	return debugMode;
}

CDebugDataAdapter *CDebugInterfaceNes::GetDataAdapter()
{
	return this->dataAdapter;
}


// make jmp without resetting CPU depending on dataAdapter
void CDebugInterfaceNes::MakeJmpNoReset(CDataAdapter *dataAdapter, uint16 addr)
{
	this->LockMutex();
	
//	c64d_atari_set_cpu_pc(addr);
	
	this->UnlockMutex();
}

// make jmp and reset CPU
void CDebugInterfaceNes::MakeJmpAndReset(uint16 addr)
{
	LOGTODO("CDebugInterfaceNes::MakeJmpAndReset");
}

// keyboard & joystick mapper
bool CDebugInterfaceNes::KeyboardDown(uint32 mtKeyCode)
{
	LOGI("CDebugInterfaceNes::KeyboardDown: mtKeyCode=%04x", mtKeyCode);

	for (std::list<CDebuggerEmulatorPlugin *>::iterator it = this->plugins.begin(); it != this->plugins.end(); it++)
	{
		CDebuggerEmulatorPlugin *plugin = *it;
		mtKeyCode = plugin->KeyDown(mtKeyCode);
		
		if (mtKeyCode == 0)
			return false;
	}
	
	return false;
}

bool CDebugInterfaceNes::KeyboardUp(uint32 mtKeyCode)
{
	LOGI("CDebugInterfaceNes::KeyboardUp: mtKeyCode=%04x", mtKeyCode);
	
	for (std::list<CDebuggerEmulatorPlugin *>::iterator it = this->plugins.begin(); it != this->plugins.end(); it++)
	{
		CDebuggerEmulatorPlugin *plugin = *it;
		mtKeyCode = plugin->KeyUp(mtKeyCode);
		
		if (mtKeyCode == 0)
			return false;
	}
	
	return false;
}

void CDebugInterfaceNes::JoystickDown(int port, uint32 axis)
{
	LOGD("CDebugInterfaceNes::JoystickDown: %d %d", port, axis);
	CDebugInterfaceNesTaskJoystickEvent *joystickEventTask = new CDebugInterfaceNesTaskJoystickEvent(this, DEBUGGER_EVENT_BUTTON_DOWN, port, axis);
	AddCpuDebugInterruptTask(joystickEventTask);
}

void CDebugInterfaceNes::JoystickUp(int port, uint32 axis)
{
	LOGD("CDebugInterfaceNes::JoystickUp: %d %d", port, axis);
	CDebugInterfaceNesTaskJoystickEvent *joystickEventTask = new CDebugInterfaceNesTaskJoystickEvent(this, DEBUGGER_EVENT_BUTTON_UP, port, axis);
	AddCpuDebugInterruptTask(joystickEventTask);
}

void CDebugInterfaceNes::ProcessJoystickEventSynced(int port, u32 axis, u8 buttonState)
{
	LOGD("ProcessJoystickEventSynced: port=%d axis=%d buttonState=%d ", port, axis, buttonState);
	
	if (buttonState == DEBUGGER_EVENT_BUTTON_DOWN)
	{
		nesd_joystick_down(port, axis);
	}
	else if (buttonState == DEBUGGER_EVENT_BUTTON_UP)
	{
		nesd_joystick_up(port, axis);
	}
}

// this is called by CSnapshotManager to replay events at current cycle
void CDebugInterfaceNes::ReplayInputEventsFromSnapshotsManager(CByteBuffer *inputEventsBuffer)
{
	LOGD("ReplayInputEventsFromSnapshotsManager");

	while (inputEventsBuffer->IsEof() == false)
	{
		int port = inputEventsBuffer->GetI32();
		u32 axis = inputEventsBuffer->GetU32();
		u8 buttonState = inputEventsBuffer->GetU8();

		ProcessJoystickEventSynced(port, axis, buttonState);
	}
}

void CDebugInterfaceNes::Reset()
{
	LOGM("CDebugInterfaceNes::Reset");
	CDebugInterfaceNesTaskReset *task = new CDebugInterfaceNesTaskReset(this);
	this->AddCpuDebugInterruptTask(task);
}

void CDebugInterfaceNes::HardReset()
{
	LOGM("CDebugInterfaceNes::HardReset");
	CDebugInterfaceNesTaskHardReset *task = new CDebugInterfaceNesTaskHardReset(this);
	this->AddCpuDebugInterruptTask(task);
}

void CDebugInterfaceNes::ResetClockCounters()
{
	Nes::Core::Machine& machine = nesEmulator;
	machine.cpu.nesdMainCpuCycle = 0;
	machine.cpu.nesdMainCpuDebugCycle = 0;
	machine.cpu.nesdMainCpuPreviousInstructionCycle = 0;
}

bool CDebugInterfaceNes::LoadExecutable(char *fullFilePath)
{
	LOGM("CDebugInterfaceNes::LoadExecutable: %s", fullFilePath);
	return true;
}

bool CDebugInterfaceNes::MountDisk(char *fullFilePath, int diskNo, bool readOnly)
{
	return true;
}

bool CDebugInterfaceNes::InsertCartridge(char *fullFilePath)
{
	LOGM("CDebugInterfaceNes::InsertCartridge: %s", fullFilePath);
	
	this->LockMutex();
	
	CSlrString *str = new CSlrString(fullFilePath);
	CDebugInterfaceNesTaskInsertCartridge *task = new CDebugInterfaceNesTaskInsertCartridge(this, str);
	this->AddVSyncTask(task);
	delete str;

	this->UnlockMutex();
	
	return true;
}

bool CDebugInterfaceNes::AttachTape(char *fullFilePath, bool readOnly)
{
	return true;
}

// this is main emulation cpu cycle counter
u64 CDebugInterfaceNes::GetMainCpuCycleCounter()
{
	Nes::Core::Machine& machine = nesEmulator;
	return machine.cpu.nesdMainCpuCycle;
}

u64 CDebugInterfaceNes::GetPreviousCpuInstructionCycleCounter()
{
	Nes::Core::Machine& machine = nesEmulator;
	return machine.cpu.nesdMainCpuPreviousInstructionCycle;
}

// resettable counters for debug purposes
void CDebugInterfaceNes::ResetMainCpuDebugCycleCounter()
{
	Nes::Core::Machine& machine = nesEmulator;
	machine.cpu.nesdMainCpuDebugCycle = 0;
}

u64 CDebugInterfaceNes::GetMainCpuDebugCycleCounter()
{
	Nes::Core::Machine& machine = nesEmulator;
	return machine.cpu.nesdMainCpuDebugCycle;
}

bool CDebugInterfaceNes::LoadFullSnapshot(char *filePath)
{
	guiMain->LockMutex();
	this->LockMutex();
	
	bool ret = false;
	CSlrFileFromOS *file = new CSlrFileFromOS(filePath, SLR_FILE_MODE_READ);
	if (file->Exists())
	{
		CByteBuffer *byteBuffer = new CByteBuffer(file, false);
		if (nesd_restore_nesd_state_from_bytebuffer(byteBuffer))
		{
			ret = true;
		}
		
		delete byteBuffer;
	}
	
	delete file;
	
	this->UnlockMutex();
	guiMain->UnlockMutex();
	return ret;
}

void CDebugInterfaceNes::SaveFullSnapshot(char *filePath)
{
	LOGD("CDebugInterfaceNes::SaveFullSnapshot: %s", filePath);
	guiMain->LockMutex();
	this->LockMutex();

	CByteBuffer *byteBuffer = new CByteBuffer();
	nesd_store_nesd_state_to_bytebuffer(byteBuffer);
	byteBuffer->storeToFileNoHeader(filePath);
	
	delete byteBuffer;

	this->UnlockMutex();
	guiMain->UnlockMutex();
}

// these calls should be synced with CPU IRQ so snapshot store or restore is allowed
bool CDebugInterfaceNes::LoadChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	LOGD("CDebugInterfaceNes::LoadChipsSnapshotSynced");
	debugInterfaceNes->LockMutex();
	gSoundEngine->LockMutex("CDebugInterfaceNes::LoadChipsSnapshotSynced");
	
	bool ret = nesd_restore_nesd_state_from_bytebuffer(byteBuffer);
	
	if (ret == false)
	{
		LOGError("CDebugInterfaceNes::LoadChipsSnapshotSynced: failed");

		debugInterfaceNes->UnlockMutex();
		gSoundEngine->UnlockMutex("CDebugInterfaceNes::LoadChipsSnapshotSynced");
		return false;
	}
	
	debugInterfaceNes->UnlockMutex();
	gSoundEngine->UnlockMutex("CDebugInterfaceNes::LoadChipsSnapshotSynced");
	return true;
}

bool CDebugInterfaceNes::SaveChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	// TODO: check if data changed and store snapshot with data accordingly
	return nesd_store_nesd_state_to_bytebuffer(byteBuffer);
}

bool CDebugInterfaceNes::LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	return true;
}

bool CDebugInterfaceNes::SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	// TODO: check if data changed and store snapshot with data accordingly
	return true;
}

bool CDebugInterfaceNes::IsDriveDirtyForSnapshot()
{
	// TODO: check if data on disk/cart changed
	return false;
}

void CDebugInterfaceNes::ClearDriveDirtyForSnapshotFlag()
{
	//c64d_clear_drive_dirty_for_snapshot();
}


///

void CDebugInterfaceNes::SetVideoSystem(u8 videoSystem)
{
	LOGD("CDebugInterfaceNes::SetVideoSystem: %d", videoSystem);
}


void CDebugInterfaceNes::SetMachineType(u8 machineType)
{
}

void CDebugInterfaceNes::SetApuMuteChannels(int apuNumber, bool muteSquare1, bool muteSquare2, bool muteTriangle, bool muteNoise, bool muteDmc, bool muteExt)
{
	nesd_mute_channels(muteSquare1, muteSquare2, muteTriangle, muteNoise, muteDmc, muteExt);
}

void CDebugInterfaceNes::SetApuReceiveChannelsData(int apuNumber, bool isReceiving)
{
//	LOGD("SetApuReceiveChannelsData: isReceiving=%s", STRBOOL(isReceiving));
	nesd_isReceiveChannelsData = isReceiving;
}

void CDebugInterfaceNes::AddWaveformData(int apuNumber, int v1, int v2, int v3, int v4, int v5, int v6, short mix)
{
//	LOGD("CDebugInterfaceNes::AddWaveformData: #%d, %d %d %d %d %d %d | %d", apuNumber, v1, v2, v3, v4, v5, v6, mix);
	
	// apu channels
	nesChannelWaveform[apuNumber][0]->AddSample(v1);
	nesChannelWaveform[apuNumber][1]->AddSample(v2);
	nesChannelWaveform[apuNumber][2]->AddSample(v3);
	nesChannelWaveform[apuNumber][3]->AddSample(v4);
	nesChannelWaveform[apuNumber][4]->AddSample(v5);
	nesChannelWaveform[apuNumber][5]->AddSample(v6);

	// mix channel
	nesMixWaveform[apuNumber]->AddSample(mix);
}

void CDebugInterfaceNes::UpdateWaveforms()
{
	// copy waveform data as quickly as possible
	LockMutex();
//	for (int apuNumber = 0; apuNumber < MAX_NUM_NES_APUS; apuNumber++)
	{
		int apuNumber = 0;
		for (int chanNum = 0; chanNum < 6; chanNum++)
		{
			nesChannelWaveform[apuNumber][chanNum]->CopySampleData();
		}
		nesMixWaveform[apuNumber]->CopySampleData();
	}
	UnlockMutex();

//	for (int apuNumber = 0; apuNumber < MAX_NUM_NES_APUS; apuNumber++)
	{
		int apuNumber = 0;
		for (int chanNum = 0; chanNum < 6; chanNum++)
		{
			nesChannelWaveform[apuNumber][chanNum]->CalculateTriggerPos();
		}
		nesMixWaveform[apuNumber]->CalculateTriggerPos();
	}
}

void CDebugInterfaceNes::UpdateWaveformsMuteStatus()
{
	int apuNumber = 0;
	SetApuMuteChannels(apuNumber,
					   nesChannelWaveform[apuNumber][0]->isMuted,
					   nesChannelWaveform[apuNumber][1]->isMuted,
					   nesChannelWaveform[apuNumber][2]->isMuted,
					   nesChannelWaveform[apuNumber][3]->isMuted,
					   nesChannelWaveform[apuNumber][4]->isMuted,
					   nesChannelWaveform[apuNumber][5]->isMuted);

	if (nesChannelWaveform[apuNumber][0]->isMuted
		&& nesChannelWaveform[apuNumber][1]->isMuted
		&& nesChannelWaveform[apuNumber][2]->isMuted
		&& nesChannelWaveform[apuNumber][3]->isMuted
		&& nesChannelWaveform[apuNumber][4]->isMuted
		&& nesChannelWaveform[apuNumber][5]->isMuted)
	{
		nesMixWaveform[apuNumber]->isMuted = true;
	}
	else if (!nesChannelWaveform[apuNumber][0]->isMuted
			 || !nesChannelWaveform[apuNumber][1]->isMuted
			 || !nesChannelWaveform[apuNumber][2]->isMuted
			 || !nesChannelWaveform[apuNumber][3]->isMuted
			 || !nesChannelWaveform[apuNumber][4]->isMuted
			 || !nesChannelWaveform[apuNumber][5]->isMuted)
	{
		nesMixWaveform[apuNumber]->isMuted = false;
	}
}

u8 CDebugInterfaceNes::GetApuRegister(u16 addr)
{
	return nesd_get_apu_register(addr);
}

u8 CDebugInterfaceNes::GetPpuRegister(u16 addr)
{
	Nes::Core::Machine& machine = nesEmulator;
	return machine.ppu.registers[addr & 0x0F];
}

void CDebugInterfaceNes::SupportsBreakpoints(bool *writeBreakpoint, bool *readBreakpoint)
{
	*writeBreakpoint = true;
	*readBreakpoint = false;
}

