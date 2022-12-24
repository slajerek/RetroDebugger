#include "CDebugInterfaceC64.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "CViewMainMenu.h"
#include "CViewC64StateSID.h"
#include "CByteBuffer.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegmentC64.h"
#include "CDebugMemoryMapCell.h"
#include "CWaveformData.h"

CDebugInterfaceC64::CDebugInterfaceC64(CViewC64 *viewC64)
: CDebugInterface(viewC64)
{
	this->viewC64 = viewC64;
	
	emulationSpeed = 0;
	emulationFrameRate = 0;	

	debugMode = DEBUGGER_MODE_RUNNING;
	isDebugOn = true;
	debugOnDrive1541 = false;
		
	for (int i = 0; i < C64_NUM_DRIVES; i++)
	{
		ledGreenPwm[i] = 0.0f;
		ledRedPwm[i] = 0.0f;
//		this->diskImage[i] = NULL;
	}
}

CDebugInterfaceC64::~CDebugInterfaceC64()
{
}

void CDebugInterfaceC64::Shutdown()
{
	CDebugInterface::Shutdown();
}

int CDebugInterfaceC64::GetEmulatorType()
{
	SYS_FatalExit("CDebugInterfaceC64::GetEmulatorType");
	return -1;
}

CSlrString *CDebugInterfaceC64::GetEmulatorVersionString()
{
	SYS_FatalExit("CDebugInterfaceC64::GetEmulatorVersionString");
	return NULL;
}

const char *CDebugInterfaceC64::GetPlatformNameString()
{
	return "C64";
}

float CDebugInterfaceC64::GetEmulationFPS()
{
	return -1;
}

u64 CDebugInterfaceC64::GetMainCpuCycleCounter()
{
	return CDebugInterface::GetMainCpuCycleCounter();
}

u64 CDebugInterfaceC64::GetCurrentCpuInstructionCycleCounter()
{
	return GetMainCpuCycleCounter();
}

void CDebugInterfaceC64::ResetMainCpuDebugCycleCounter()
{
	CDebugInterface::ResetMainCpuDebugCycleCounter();
}

u64 CDebugInterfaceC64::GetMainCpuDebugCycleCounter()
{
	return CDebugInterface::GetMainCpuDebugCycleCounter();
}

u64 CDebugInterfaceC64::GetPreviousCpuInstructionCycleCounter()
{
	return CDebugInterface::GetPreviousCpuInstructionCycleCounter();
}

void CDebugInterfaceC64::ResetEmulationFrameCounter()
{
	CDebugInterface::ResetEmulationFrameCounter();
}

unsigned int CDebugInterfaceC64::GetEmulationFrameNumber()
{
	return CDebugInterface::GetEmulationFrameNumber();
}

void CDebugInterfaceC64::RefreshScreenNoCallback()
{
	SYS_FatalExit("CDebugInterfaceC64::RefreshScreenNoCallback");
}

void CDebugInterfaceC64::InitKeyMap(C64KeyMap *keyMap)
{
	SYS_FatalExit("CDebugInterfaceC64::InitKeyMap");
}

uint8 *CDebugInterfaceC64::GetCharRom()
{
	SYS_FatalExit("CDebugInterfaceC64::GetCharRom");
	return NULL;
}


void CDebugInterfaceC64::RunEmulationThread()
{
	CDebugInterface::RunEmulationThread();
	SYS_FatalExit("CDebugInterfaceC64::RunEmulationThread");
}

void CDebugInterfaceC64::CheckLoadedRoms()
{
}

void CDebugInterfaceC64::RestartAudio()
{
}

void CDebugInterfaceC64::RefreshSync()
{
}

//
int CDebugInterfaceC64::GetC64ModelType()
{
	return 0;
}

uint8 CDebugInterfaceC64::GetC64MachineType()
{
	return 0;
}


// video

int CDebugInterfaceC64::GetScreenSizeX()
{
	return -1;
}

int CDebugInterfaceC64::GetScreenSizeY()
{
	return -1;
}

void CDebugInterfaceC64::SetDebugOnC64(bool debugOnC64)
{
	this->isDebugOn = debugOnC64;
}

void CDebugInterfaceC64::SetDebugOnDrive1541(bool debugOnDrive1541)
{
	this->debugOnDrive1541 = debugOnDrive1541;
}

void CDebugInterfaceC64::Reset()
{
	SYS_FatalExit("CDebugInterfaceC64::Reset");
}

void CDebugInterfaceC64::HardReset()
{
	SYS_FatalExit("CDebugInterfaceC64::HardReset");
}

void CDebugInterfaceC64::DiskDriveReset()
{
	SYS_FatalExit("CDebugInterfaceC64::DiskDriveReset");
}

bool CDebugInterfaceC64::KeyboardDown(uint32 mtKeyCode)
{
	SYS_FatalExit("CDebugInterfaceC64::KeyboardDown");
	return false;
}

bool CDebugInterfaceC64::KeyboardUp(uint32 mtKeyCode)
{
	SYS_FatalExit("CDebugInterfaceC64::KeyboardUp");
	return false;
}

void CDebugInterfaceC64::JoystickDown(int port, uint32 axis)
{
	SYS_FatalExit("CDebugInterfaceC64::JoystickDown");
}

void CDebugInterfaceC64::JoystickUp(int port, uint32 axis)
{
	SYS_FatalExit("CDebugInterfaceC64::JoystickUp");
}

int CDebugInterfaceC64::GetCpuPC()
{
	SYS_FatalExit("CDebugInterfaceC64::GetCpuPC");
	return 0;
}

int CDebugInterfaceC64::GetDrive1541PC()
{
	SYS_FatalExit("CDebugInterfaceC64::GetDrive1541PC");
	return 0;
}

void CDebugInterfaceC64::GetC64CpuState(C64StateCPU *state)
{
	SYS_FatalExit("CDebugInterfaceC64::GetC64CpuState");
}

void CDebugInterfaceC64::GetDrive1541CpuState(C64StateCPU *state)
{
	SYS_FatalExit("CDebugInterfaceC64::GetDrive1541CpuState");
}

void CDebugInterfaceC64::GetVICState(C64StateVIC *state)
{
	SYS_FatalExit("CDebugInterfaceC64::GetVICState");
}

void CDebugInterfaceC64::GetDrive1541State(C64StateDrive1541 *state)
{
	SYS_FatalExit("CDebugInterfaceC64::GetDrive1541State");
}

//
void CDebugInterfaceC64::SetStackPointerC64(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetStackPointerC64");
}

void CDebugInterfaceC64::SetRegisterAC64(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterAC64");
}

void CDebugInterfaceC64::SetRegisterXC64(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterXC64");
}

void CDebugInterfaceC64::SetRegisterYC64(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterYC64");
}

void CDebugInterfaceC64::SetRegisterPC64(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterPC64");
}

void CDebugInterfaceC64::SetStackPointer1541(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetStackPointer1541");
}

void CDebugInterfaceC64::SetRegisterA1541(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterA1541");
}

void CDebugInterfaceC64::SetRegisterX1541(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterX1541");
}

void CDebugInterfaceC64::SetRegisterY1541(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterY1541");
}

void CDebugInterfaceC64::SetRegisterP1541(uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRegisterP1541");
}

void CDebugInterfaceC64::InsertD64(CSlrString *path)
{
	SYS_FatalExit("CDebugInterfaceC64::InsertD64");
}

void CDebugInterfaceC64::DetachDriveDisk()
{
	SYS_FatalExit("CDebugInterfaceC64::DetachDriveDisk");
}

void CDebugInterfaceC64::MakeBasicRunC64()
{
	SYS_FatalExit("CDebugInterfaceC64::MakeBasicRunC64");
}

bool CDebugInterfaceC64::GetSettingIsWarpSpeed()
{
	SYS_FatalExit("CDebugInterfaceC64::GetSettingIsWarpSpeed");
	return false;
}

void CDebugInterfaceC64::SetSettingIsWarpSpeed(bool isWarpSpeed)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSettingIsWarpSpeed");
}

void CDebugInterfaceC64::GetSidTypes(std::vector<CSlrString *> *sidTypes)
{
	SYS_FatalExit("CDebugInterfaceC64::GetSidTypes");
}

void CDebugInterfaceC64::GetSidTypes(std::vector<const char *> *sidTypes)
{
	SYS_FatalExit("CDebugInterfaceC64::GetSidTypes");
}

void CDebugInterfaceC64::SetSidType(int sidType)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidType");
}

// samplingMethod: Fast=0, Interpolating=1, Resampling=2, Fast Resampling=3
void CDebugInterfaceC64::SetSidSamplingMethod(int samplingMethod)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidSamplingMethod");
}

// emulateFilters: no=0, yes=1
void CDebugInterfaceC64::SetSidEmulateFilters(int emulateFilters)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidEmulateFilters");
}

// passband: 0-90
void CDebugInterfaceC64::SetSidPassBand(int passband)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidPassBand");
}

// filterBias: -500 500
void CDebugInterfaceC64::SetSidFilterBias(int filterBias)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidFilterBias");
}

int CDebugInterfaceC64::GetNumSids()
{
	SYS_FatalExit("CDebugInterfaceC64::GetNumSids");
	return 0;
}

void CDebugInterfaceC64::SetSidStereo(int stereoMode)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidStereo");
}

void CDebugInterfaceC64::SetSidStereoAddress(uint16 sidAddress)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidStereoAddress");
}

void CDebugInterfaceC64::SetSidTripleAddress(uint16 sidAddress)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidTripleAddress");
}

void CDebugInterfaceC64::GetC64ModelTypes(std::vector<CSlrString *> *modelTypeNames, std::vector<int> *modelTypeIds)
{
	SYS_FatalExit("CDebugInterfaceC64::GetC64ModelTypes");
}

void CDebugInterfaceC64::GetC64ModelTypes(std::vector<const char *> *modelTypeNames, std::vector<int> *modelTypeIds)
{
	SYS_FatalExit("CDebugInterfaceC64::GetC64ModelTypes");
}

void CDebugInterfaceC64::SetC64ModelType(int modelType)
{
	SYS_FatalExit("CDebugInterfaceC64::SetC64ModelType");
}

void CDebugInterfaceC64::SetPatchKernalFastBoot(bool isPatchKernal)
{
	SYS_FatalExit("CDebugInterfaceC64::SetPatchKernalFastBoot");
}

void CDebugInterfaceC64::SetRunSIDWhenInWarp(bool isRunningSIDInWarp)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRunSIDWhenInWarp");
}

void CDebugInterfaceC64::SetEmulationMaximumSpeed(int maximumSpeed)
{
	SYS_FatalExit("CDebugInterfaceC64::SetEmulationMaximumSpeed");
}

void CDebugInterfaceC64::SetVSPBugEmulation(bool isVSPBugEmulation)
{
	SYS_FatalExit("CDebugInterfaceC64::SetVSPBugEmulation");
}

void CDebugInterfaceC64::SetSkipDrawingSprites(bool isSkipDrawingSprites)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSkipDrawingSprites");
}

void CDebugInterfaceC64::SetByteC64(uint16 addr, uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetByteC64");
}

void CDebugInterfaceC64::SetByteToRamC64(uint16 addr, uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetByteToRamC64");
}

uint8 CDebugInterfaceC64::GetByteC64(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::GetByteC64");
	return 0;
}

uint8 CDebugInterfaceC64::GetByteFromRamC64(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::GetByteFromRamC64");
	return 0;
}

void CDebugInterfaceC64::MakeJmpNoReset(CDataAdapter *dataAdapter, uint16 addr)
{
	if (dataAdapter == this->dataAdapterC64 || dataAdapter == this->dataAdapterC64DirectRam)
	{
		this->MakeJmpNoResetC64(addr);
	}
	else
	{
		this->MakeJmpNoReset1541(addr);
	}
}

void CDebugInterfaceC64::MakeJmpAndReset(uint16 addr)
{
	this->MakeJmpC64(addr);
}

void CDebugInterfaceC64::MakeJmpC64(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::MakeJmpC64");
}

void CDebugInterfaceC64::MakeJmpNoResetC64(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::MakeJmpNoResetC64");
}

void CDebugInterfaceC64::MakeJsrC64(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::MakeJsrC64");
}

void CDebugInterfaceC64::MakeJmpNoReset1541(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::MakeJmpNoReset1541");
}

void CDebugInterfaceC64::SupportsBreakpoints(bool *writeBreakpoint, bool *readBreakpoint)
{
	CDebugInterface::SupportsBreakpoints(writeBreakpoint, readBreakpoint);
}

void CDebugInterfaceC64::ClearTemporaryBreakpoint()
{
	if (this->GetDebugMode() == DEBUGGER_MODE_RUNNING)
	{
		symbolsDrive1541->ClearTemporaryBreakpoint();
	}
	
	CDebugInterface::ClearTemporaryBreakpoint();
}

void CDebugInterfaceC64::SetByte1541(uint16 addr, uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetByte1541");
}

void CDebugInterfaceC64::SetByteToRam1541(uint16 addr, uint8 val)
{
	SYS_FatalExit("CDebugInterfaceC64::SetByteToRam1541");
}

uint8 CDebugInterfaceC64::GetByte1541(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::GetByte1541");
	return 0;
}

uint8 CDebugInterfaceC64::GetByteFromRam1541(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::GetByteFromRam1541");
	return 0;
}

void CDebugInterfaceC64::MakeJmp1541(uint16 addr)
{
	SYS_FatalExit("CDebugInterfaceC64::MakeJmp1541");
}

void CDebugInterfaceC64::GetWholeMemoryMap(uint8 *buffer)
{
	SYS_FatalExit("CDebugInterfaceC64::GetWholeMemoryMap");
}

void CDebugInterfaceC64::GetWholeMemoryMapFromRam(uint8 *buffer)
{
	SYS_FatalExit("CDebugInterfaceC64::GetWholeMemoryMapFromRam");
}

void CDebugInterfaceC64::GetWholeMemoryMap1541(uint8 *buffer)
{	
	SYS_FatalExit("CDebugInterfaceC64::GetWholeMemoryMap1541");
}

void CDebugInterfaceC64::GetWholeMemoryMapFromRam1541(uint8 *buffer)
{
	SYS_FatalExit("CDebugInterfaceC64::GetWholeMemoryMapFromRam1541");
}

void CDebugInterfaceC64::GetMemory(uint8 *buffer, int addrStart, int addrEnd)
{
	SYS_FatalExit("CDebugInterfaceC64::GetMemory");
}

void CDebugInterfaceC64::GetMemoryFromRam(uint8 *buffer, int addrStart, int addrEnd)
{
	SYS_FatalExit("CDebugInterfaceC64::GetMemoryFromRam");
}

void CDebugInterfaceC64::GetMemoryDrive1541(uint8 *buffer, int addrStart, int addrEnd)
{
	SYS_FatalExit("CDebugInterfaceC64::GetMemoryDrive1541");
}

void CDebugInterfaceC64::GetMemoryFromRamDrive1541(uint8 *buffer, int addrStart, int addrEnd)
{	
	SYS_FatalExit("CDebugInterfaceC64::GetMemoryFromRamDrive1541");
}

void CDebugInterfaceC64::FillC64Ram(uint16 addr, uint16 size, uint8 value)
{
	SYS_FatalExit("CDebugInterfaceC64::FillC64Ram");
}

bool CDebugInterfaceC64::LoadFullSnapshot(CByteBuffer *snapshotBuffer)
{
	SYS_FatalExit("CDebugInterfaceC64::LoadFullSnapshot");
	return false;
}

void CDebugInterfaceC64::SaveFullSnapshot(CByteBuffer *snapshotBuffer)
{
	SYS_FatalExit("CDebugInterfaceC64::SaveFullSnapshot");
}

bool CDebugInterfaceC64::LoadFullSnapshot(char *filePath)
{
	SYS_FatalExit("CDebugInterfaceC64::LoadFullSnapshot");
	return false;
}

void CDebugInterfaceC64::SaveFullSnapshot(char *filePath)
{
	SYS_FatalExit("CDebugInterfaceC64::SaveFullSnapshot");
}

// this call should be synced with CPU IRQ so snapshot store or restore is allowed
bool CDebugInterfaceC64::LoadChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	SYS_FatalExit("CDebugInterfaceC64::LoadChipsSnapshotSynced");
	return false;
}

bool CDebugInterfaceC64::SaveChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	SYS_FatalExit("CDebugInterfaceC64::SaveChipsSnapshotSynced");
	return false;
}

bool CDebugInterfaceC64::LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	SYS_FatalExit("CDebugInterfaceC64::LoadDiskDataSnapshotSynced");
	return false;
}

bool CDebugInterfaceC64::SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	SYS_FatalExit("CDebugInterfaceC64::SaveDiskDataSnapshotSynced");
	return false;
}

bool CDebugInterfaceC64::IsDriveDirtyForSnapshot()
{
	SYS_FatalExit("CDebugInterfaceC64::IsDriveDirtyForSnapshot");
	return false;
}

void CDebugInterfaceC64::ClearDriveDirtyForSnapshotFlag()
{
	SYS_FatalExit("CDebugInterfaceC64::ClearDriveDirtyForSnapshotFlag");
}

void CDebugInterfaceC64::GetVICColors(uint8 *cD021, uint8 *cD022, uint8 *cD023, uint8 *cD025, uint8 *cD026, uint8 *cD027, uint8 *cD800)
{
	SYS_FatalExit("CDebugInterfaceC64::GetVICColors");
}

void CDebugInterfaceC64::GetVICSpriteColors(uint8 *cD021, uint8 *cD025, uint8 *cD026, uint8 *spriteColors)
{
	SYS_FatalExit("CDebugInterfaceC64::GetVICSpriteColors");
}

void CDebugInterfaceC64::GetCBMColor(uint8 colorNum, uint8 *r, uint8 *g, uint8 *b)
{
	SYS_FatalExit("CDebugInterfaceC64::GetCBMColor");
}

void CDebugInterfaceC64::GetFloatCBMColor(uint8 colorNum, float *r, float *g, float *b)
{
	SYS_FatalExit("CDebugInterfaceC64::GetFloatCBMColor");
}

//

//void CDebugInterfaceC64::UiInsertD64(CSlrString *path)
//{
//	SYS_FatalExit("CDebugInterfaceC64::UiInsertD64");
////	LOGTODO("CDebugInterfaceC64::UiInsertD64: shall we update Folder path to D64?");
////	viewC64->viewC64MainMenu->InsertD64(path, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0);
//}

//
void CDebugInterfaceC64::SetVicRegister(uint8 registerNum, uint8 value)
{
	SYS_FatalExit("CDebugInterfaceC64::SetVicRegister");
}

u8 CDebugInterfaceC64::GetVicRegister(uint8 registerNum)
{
	SYS_FatalExit("CDebugInterfaceC64::GetVicRegister");
	return 0;
}

u8 CDebugInterfaceC64::GetVicRegister(vicii_cycle_state_t *viciiState, uint8 registerNum)
{
	return viciiState->regs[registerNum];
}


void CDebugInterfaceC64::SetCiaRegister(uint8 ciaId, uint8 registerNum, uint8 value)
{
	SYS_FatalExit("CDebugInterfaceC64::SetCiaRegister");
}

u8 CDebugInterfaceC64::GetCiaRegister(uint8 ciaId, uint8 registerNum)
{
	SYS_FatalExit("CDebugInterfaceC64::GetCiaRegister");
	return 0;
}

void CDebugInterfaceC64::SetSidRegister(uint8 sidId, uint8 registerNum, uint8 value)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSidRegister");
}

u8 CDebugInterfaceC64::GetSidRegister(uint8 sidId, uint8 registerNum)
{
	SYS_FatalExit("CDebugInterfaceC64::GetSidRegister");
	return 0;
}

void CDebugInterfaceC64::UpdateSidDataHistory()
{
	SYS_FatalExit("CDebugInterfaceC64::UpdateSidDataHistory");
}

void CDebugInterfaceC64::SetViaRegister(uint8 driveId, uint8 viaId, uint8 registerNum, uint8 value)
{
	SYS_FatalExit("CDebugInterfaceC64::SetViaRegister");
}

u8 CDebugInterfaceC64::GetViaRegister(uint8 driveId, uint8 viaId, uint8 registerNum)
{
	SYS_FatalExit("CDebugInterfaceC64::GetViaRegister");
	return 0;
}


void CDebugInterfaceC64::SetSIDMuteChannels(int sidNumber, bool mute1, bool mute2, bool mute3, bool muteExt)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSIDMuteChannels");
}

void CDebugInterfaceC64::SetSIDReceiveChannelsData(int sidNumber, bool isReceiving)
{
	SYS_FatalExit("CDebugInterfaceC64::SetSIDReceiveChannelsData");
}

//
void CDebugInterfaceC64::AddWaveformData(int sidNumber, int voice1, int voice2, int voice3, short mix)
{
	//	LOGD("CDebugInterfaceC64::AddWaveformData: sid#%d, %d %d %d %d", sidNumber, v1, v2, v3, mix);
	
	// sid channels
	sidChannelWaveform[sidNumber][0]->AddSample(voice1);
	sidChannelWaveform[sidNumber][1]->AddSample(voice2);
	sidChannelWaveform[sidNumber][2]->AddSample(voice3);
	
	// mix channel
	sidMixWaveform[sidNumber]->AddSample(mix);
}

void CDebugInterfaceC64::UpdateWaveforms()
{
	// copy waveform data as quickly as possible
	LockMutex();
	int numSids = GetNumSids();
	for (int sidNum = 0; sidNum < numSids; sidNum++)
	{
		for (int i = 0; i < 3; i++)
		{
			sidChannelWaveform[sidNum][i]->CopySampleData();
		}

		sidMixWaveform[sidNum]->CopySampleData();
	}
	UnlockMutex();
	
	// calculate trigger pos
	for (int sidNum = 0; sidNum < numSids; sidNum++)
	{
		for (int i = 0; i < 3; i++)
		{
			sidChannelWaveform[sidNum][i]->CalculateTriggerPos();
		}

		sidMixWaveform[sidNum]->CalculateTriggerPos();
	}
}

void CDebugInterfaceC64::UpdateWaveformsMuteStatus()
{
	for (int sidNum = 0; sidNum < GetNumSids(); sidNum++)
	{
		SetSIDMuteChannels(sidNum,
						   sidChannelWaveform[sidNum][0]->isMuted,
						   sidChannelWaveform[sidNum][1]->isMuted,
						   sidChannelWaveform[sidNum][2]->isMuted, false);

		if (sidChannelWaveform[sidNum][0]->isMuted
			&& sidChannelWaveform[sidNum][1]->isMuted
			&& sidChannelWaveform[sidNum][2]->isMuted)
		{
			sidMixWaveform[sidNum]->isMuted = true;
		}
		else if (!sidChannelWaveform[sidNum][0]->isMuted
				 || !sidChannelWaveform[sidNum][1]->isMuted
				 || !sidChannelWaveform[sidNum][2]->isMuted)
		{
			sidMixWaveform[sidNum]->isMuted = false;
		}
	}
}

void CDebugInterfaceC64::SetVicRecordStateMode(uint8 recordMode)
{
	SYS_FatalExit("CDebugInterfaceC64::SetVicRecordStateMode");
}

bool CDebugInterfaceC64::IsCpuJam()
{
	SYS_FatalExit("CDebugInterfaceC64::IsCpuJam");
	return false;
}

void CDebugInterfaceC64::ForceRunAndUnJamCpu()
{
	SYS_FatalExit("CDebugInterfaceC64::ForceRunAndUnJamCpu");
}


//
void CDebugInterfaceC64::AttachTape(CSlrString *filePath)
{
	SYS_FatalExit("CDebugInterfaceC64::AttachTape");
}

void CDebugInterfaceC64::DetachTape()
{
	SYS_FatalExit("CDebugInterfaceC64::DetachTape");
}

void CDebugInterfaceC64::DatasettePlay()
{
	SYS_FatalExit("CDebugInterfaceC64::DatasettePlay");
}

void CDebugInterfaceC64::DatasetteStop()
{
	SYS_FatalExit("CDebugInterfaceC64::DatasetteStop");
}

void CDebugInterfaceC64::DatasetteForward()
{
	SYS_FatalExit("CDebugInterfaceC64::DatasetteForward");
}

void CDebugInterfaceC64::DatasetteRewind()
{
	SYS_FatalExit("CDebugInterfaceC64::DatasetteRewind");
}

void CDebugInterfaceC64::DatasetteRecord()
{
	SYS_FatalExit("CDebugInterfaceC64::DatasetteRecord");
}

void CDebugInterfaceC64::DatasetteReset()
{
	SYS_FatalExit("CDebugInterfaceC64::DatasetteReset");
}

void CDebugInterfaceC64::DatasetteSetSpeedTuning(int speedTuning)
{
}

void CDebugInterfaceC64::DatasetteSetZeroGapDelay(int zeroGapDelay)
{
}

void CDebugInterfaceC64::DatasetteSetResetWithCPU(bool resetWithCPU)
{
}

void CDebugInterfaceC64::DatasetteSetTapeWobble(int tapeWobble)
{
}

void CDebugInterfaceC64::AttachCartridge(CSlrString *filePath)
{
	SYS_FatalExit("CDebugInterfaceC64::AttachCartridge");
}

void CDebugInterfaceC64::DetachCartridge()
{
	SYS_FatalExit("CDebugInterfaceC64::DetachCartridge");
}

void CDebugInterfaceC64::CartridgeFreezeButtonPressed()
{
	SYS_FatalExit("CDebugInterfaceC64::CartridgeFreezeButtonPressed");
}

void CDebugInterfaceC64::GetC64CartridgeState(C64StateCartridge *cartridgeState)
{
	SYS_FatalExit("CDebugInterfaceC64::GetC64CartridgeState");
}

// REU
void CDebugInterfaceC64::SetReuEnabled(bool isEnabled)
{
	SYS_FatalExit("CDebugInterfaceC64::SetReuEnabled");
}

void CDebugInterfaceC64::SetReuSize(int reuSize)
{
	SYS_FatalExit("CDebugInterfaceC64::SetReuSize");
}

bool CDebugInterfaceC64::LoadReu(char *filePath)
{
	SYS_FatalExit("CDebugInterfaceC64::LoadReu");
	return false;
}

bool CDebugInterfaceC64::SaveReu(char *filePath)
{
	SYS_FatalExit("CDebugInterfaceC64::SaveReu");
	return false;
}

//
void CDebugInterfaceC64::DetachEverything()
{
	SYS_FatalExit("CDebugInterfaceC64::DetachEverything");
}

void CDebugInterfaceC64::SetPalette(uint8 *palette)
{
	SYS_FatalExit("CDebugInterfaceC64::SetPalette");
}

void CDebugInterfaceC64::SetRunSIDEmulation(bool isSIDEmulationOn)
{
	SYS_FatalExit("CDebugInterfaceC64::SetRunSIDEmulation");
}

void CDebugInterfaceC64::SetAudioVolume(float volume)
{
	SYS_FatalExit("CDebugInterfaceC64::SetAudioVolume");
}

void CDebugInterfaceC64::ProfilerActivate(char *fileName, int runForNumCycles, bool pauseCpuWhenFinished)
{
	SYS_FatalExit("CDebugInterfaceC64::ProfilerActivate");
}

void CDebugInterfaceC64::ProfilerDeactivate()
{
	SYS_FatalExit("CDebugInterfaceC64::ProfilerDeactivate");
}

bool CDebugInterfaceC64::IsProfilerActive()
{
	SYS_FatalExit("CDebugInterfaceC64::IsProfilerActive");
	return false;
}

CDebugDataAdapter *CDebugInterfaceC64::GetDataAdapter()
{
	return this->dataAdapterC64;
}

//void CDebugInterfaceC64::UpdateDriveDiskID(int driveId)
//{
//	SYS_FatalExit("CDebugInterfaceC64::UpdateDriveDiskID");
//}

//
void CDebugInterfaceC64::DumpC64Memory(CSlrString *path)
{
	//path->DebugPrint("CDebugInterfaceC64::DumpC64Memory, path=");

	char *asciiPath = path->GetStdASCII();
	
	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		GetWholeMemoryMapFromRam(memoryBuffer);
	}
	else
	{
		GetWholeMemoryMap(memoryBuffer);
	}

	// BUG: this below will read value from RAM, but $0000 and $0001 are a special case which is already handled by GetWholeMemoryMapFromRam
//	memoryBuffer[0x0000] = GetByteFromRamC64(0x0000);
//	memoryBuffer[0x0001] = GetByteFromRamC64(0x0001);

	FILE *fp = fopen(asciiPath, "wb");
	if (fp == NULL)
	{
		viewC64->ShowMessageError("Saving memory dump failed");
		return;
	}
	
	fwrite(memoryBuffer, 0x10000, 1, fp);
	fclose(fp);
	
	delete [] memoryBuffer;
	delete [] asciiPath;
	
	viewC64->ShowMessageSuccess("C64 memory dumped");
}

void CDebugInterfaceC64::DumpC64MemoryMarkers(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpC64MemoryMarkers, path=");
	
	char *asciiPath = path->GetStdASCII();
	
	FILE *fp = fopen(asciiPath, "wb");
	delete [] asciiPath;

	if (fp == NULL)
	{
		viewC64->ShowMessageError("Saving memory markers failed");
		return;
	}
	
	viewC64->debugInterfaceC64->LockMutex();
	
	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		GetWholeMemoryMapFromRam(memoryBuffer);
	}
	else
	{
		GetWholeMemoryMap(memoryBuffer);
	}

	// BUG: this below will read value from RAM, but $0000 and $0001 are a special case which is already handled by GetWholeMemoryMapFromRam
//	memoryBuffer[0x0000] = GetByteFromRamC64(0x0000);
//	memoryBuffer[0x0001] = GetByteFromRamC64(0x0001);

	fprintf(fp, "Address,Value,Read,Write,Execute,Argument\n");
	
	for (int i = 0; i < 0x10000; i++)
	{
		CDebugMemoryMapCell *cell = viewC64->viewC64MemoryMap->memoryCells[i];
		
		fprintf(fp, "%04x,%02x,%s,%s,%s,%s\n", i, memoryBuffer[i],
				cell->isRead ? "read" : "",
				cell->isWrite ? "write" : "",
				cell->isExecuteCode ? "execute" : "",
				cell->isExecuteArgument ? "argument" : "");
	}
	
	fclose(fp);

	delete [] memoryBuffer;

	viewC64->debugInterfaceC64->UnlockMutex();

	viewC64->ShowMessageSuccess("C64 memory markers saved");
}

void CDebugInterfaceC64::DumpDisk1541Memory(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpDisk1541Memory, path=");
	
	char *asciiPath = path->GetStdASCII();

	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		GetWholeMemoryMapFromRam1541(memoryBuffer);
	}
	else
	{
		GetWholeMemoryMap1541(memoryBuffer);
	}
	
	memoryBuffer[0x0000] = GetByteFromRam1541(0x0000);
	memoryBuffer[0x0001] = GetByteFromRam1541(0x0001);
	
	FILE *fp = fopen(asciiPath, "wb");
	if (fp == NULL)
	{
		viewC64->ShowMessageError("Saving memory dump failed");
		return;
	}
	
//	fwrite(memoryBuffer, 0x10000, 1, fp);
	fwrite(memoryBuffer, 0x0800, 1, fp);
	
	fclose(fp);

	delete [] memoryBuffer;
	delete [] asciiPath;
	
	viewC64->ShowMessageSuccess("Drive 1541 memory dumped");
}

void CDebugInterfaceC64::DumpDisk1541MemoryMarkers(CSlrString *path)
{
	//path->DebugPrint("CViewSettingsMenu::DumpDisk1541MemoryMarkers, path=");
	
	char *asciiPath = path->GetStdASCII();
	
	FILE *fp = fopen(asciiPath, "wb");
	delete [] asciiPath;
	
	if (fp == NULL)
	{
		viewC64->ShowMessageError("Saving memory markers failed");
		return;
	}
	
	viewC64->debugInterfaceC64->LockMutex();
	
	// local copy of memory
	uint8 *memoryBuffer = new uint8[0x10000];
	
	if (viewC64->isDataDirectlyFromRAM)
	{
		for (int addr = 0; addr < 0x10000; addr++)
		{
			memoryBuffer[addr] = GetByteFromRam1541(addr);
		}
	}
	else
	{
		for (int addr = 0; addr < 0x10000; addr++)
		{
			memoryBuffer[addr] = GetByte1541(addr);
		}
	}
	
	memoryBuffer[0x0000] = GetByteFromRam1541(0x0000);
	memoryBuffer[0x0001] = GetByteFromRam1541(0x0001);
	
	fprintf(fp, "Address,Value,Read,Write,Execute,Argument\n");
	
	for (int i = 0; i < 0x10000; i++)
	{
		CDebugMemoryMapCell *cell = viewC64->viewDrive1541MemoryMap->memoryCells[i];
		
		fprintf(fp, "%04x,%02x,%s,%s,%s,%s\n", i, memoryBuffer[i],
				cell->isRead ? "read" : "",
				cell->isWrite ? "write" : "",
				cell->isExecuteCode ? "execute" : "",
				cell->isExecuteArgument ? "argument" : "");
	}
	
	fclose(fp);
	
	delete [] memoryBuffer;
	
	viewC64->debugInterfaceC64->UnlockMutex();
	
	viewC64->ShowMessageSuccess("Drive 1541 memory markers saved");
}



//
bool CDebugInterfaceC64::IsCodeMonitorSupported()
{
	return false;
}

CSlrString *CDebugInterfaceC64::GetCodeMonitorPrompt()
{
	// monitor is not supported
	return NULL;
}

bool CDebugInterfaceC64::ExecuteCodeMonitorCommand(CSlrString *commandStr)
{
	// monitor is not supported
	return false;
}

