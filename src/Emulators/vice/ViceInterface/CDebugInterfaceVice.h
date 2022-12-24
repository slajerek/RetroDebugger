#ifndef _CDebugInterfaceVice_H_
#define _CDebugInterfaceVice_H_

#include "CDebugInterfaceC64.h"
#include "ViceWrapper.h"
#include "SYS_Threading.h"
#include "CPool.h"

#ifndef SOUND_SIDS_MAX
#define SOUND_SIDS_MAX C64_MAX_NUM_SIDS
#endif

//HAVE_NETWORK  ?

enum shift_type {
	NO_SHIFT = 0,             /* Key is not shifted. */
	VIRTUAL_SHIFT = (1 << 0), /* The key needs a shift on the real machine. */
	LEFT_SHIFT = (1 << 1),    /* Key is left shift. */
	RIGHT_SHIFT = (1 << 2),   /* Key is right shift. */
	ALLOW_SHIFT = (1 << 3),   /* Allow key to be shifted. */
	DESHIFT_SHIFT = (1 << 4), /* Although SHIFT might be pressed, do not
							   press shift on the real machine. */
	ALLOW_OTHER = (1 << 5),   /* Allow another key code to be assigned if
							   SHIFT is pressed. */
	SHIFT_LOCK = (1 << 6),    /* Key is shift lock. */
	
	ALT_MAP  = (1 << 8)       /* Key is used for an alternative keyboard
							   mapping */
};


class CAudioChannelVice;
class CSidData;
class CDebugInterfaceVice;
class CViewBreakpoints;
class CWaveformData;

class CThreadViceDriveFlush : public CSlrThread
{
public:
	CThreadViceDriveFlush(CDebugInterfaceVice *debugInterface, int flushCheckIntervalInMS);
	int flushCheckIntervalInMS;
	CDebugInterfaceVice *debugInterface;
	virtual void ThreadRun(void *data);
};

class CDebugInterfaceVice : public CDebugInterfaceC64
{
public:
	CDebugInterfaceVice(CViewC64 *viewC64, uint8 *c64memory, bool patchKernalFastBoot);
	virtual ~CDebugInterfaceVice();
	
	void InitViceMainProgram();
	
	virtual void InitKeyMap(C64KeyMap *keyMap);

	CAudioChannelVice *audioChannel;
	
	int modelType;
	virtual int GetC64ModelType();

	virtual int GetEmulatorType();
	virtual CSlrString *GetEmulatorVersionString();
	
	virtual float GetEmulationFPS();
	float numEmulationFPS;

	virtual void RunEmulationThread();
	virtual void DoVSync();
	virtual void DoFrame();
	virtual void RefreshSync();
	virtual void RestartAudio();

	virtual uint8 *GetCharRom();

	volatile uint8 machineType;
	virtual uint8 GetC64MachineType();

	virtual int GetScreenSizeX();
	virtual int GetScreenSizeY();
	
	virtual void Reset();
	virtual void HardReset();
	virtual void DiskDriveReset();

	// this is main emulation cpu cycle counter
	virtual u64 GetMainCpuCycleCounter();
	virtual u64 GetCurrentCpuInstructionCycleCounter();
	virtual u64 GetPreviousCpuInstructionCycleCounter();
	
	// resettable counters for debug purposes
	virtual void ResetMainCpuDebugCycleCounter();
	virtual u64 GetMainCpuDebugCycleCounter();
	virtual void ResetEmulationFrameCounter();
	virtual unsigned int GetEmulationFrameNumber();
	
	virtual void RefreshScreenNoCallback();
	
	//
	virtual bool KeyboardDown(uint32 mtKeyCode);
	virtual bool KeyboardUp(uint32 mtKeyCode);
	
	virtual void JoystickDown(int port, uint32 axis);
	virtual void JoystickUp(int port, uint32 axis);
	
	//
	virtual int GetCpuPC();
	virtual int GetDrive1541PC();
	virtual void GetC64CpuState(C64StateCPU *state);
	virtual void GetDrive1541CpuState(C64StateCPU *state);
	virtual void GetVICState(C64StateVIC *state);

	virtual void GetDrive1541State(C64StateDrive1541 *state);

	virtual void InsertD64(CSlrString *path);
	virtual void DetachDriveDisk();

	virtual bool GetSettingIsWarpSpeed();
	virtual void SetSettingIsWarpSpeed(bool isWarpSpeed);

	virtual void SetViciiBorderMode(int borderMode);
	virtual int  GetViciiBorderMode();
	virtual void GetViciiGeometry(int *canvasWidth, int *canvasHeight, int *gfxPosX, int *gfxPosY);
	virtual void GetSidTypes(std::vector<CSlrString *> *sidTypes);
	virtual void GetSidTypes(std::vector<const char *> *sidTypes);
	virtual void SetSidType(int sidType);
	virtual void SetSidTypeAsync(int sidType);
	
	// samplingMethod: Fast=0, Interpolating=1, Resampling=2, Fast Resampling=3
	virtual void SetSidSamplingMethod(int samplingMethod);
	// emulateFilters: no=0, yes=1
	virtual void SetSidEmulateFilters(int emulateFilters);
	// passband: 0-90
	virtual void SetSidPassBand(int passband);
	// filterBias: -500 500
	virtual void SetSidFilterBias(int filterBias);

	// 0=none, 1=stereo, 2=triple
	int numSids;
	virtual int GetNumSids();
	virtual void SetSidStereo(int stereoMode);
	virtual void SetSidStereoAddress(uint16 sidAddress);
	virtual void SetSidTripleAddress(uint16 sidAddress);

	virtual void GetC64ModelTypes(std::vector<CSlrString *> *modelTypeNames, std::vector<int> *modelTypeIds);
	virtual void GetC64ModelTypes(std::vector<const char *> *modelTypeNames, std::vector<int> *modelTypeIds);
	virtual void SetC64ModelType(int modelType);
	virtual void SetEmulationMaximumSpeed(int maximumSpeed);

	virtual void SetVSPBugEmulation(bool isVSPBugEmulation);

	virtual void SetSkipDrawingSprites(bool isSkipDrawingSprites);
	
	virtual void SetByteC64(uint16 addr, uint8 val);
	virtual void SetByteToRamC64(uint16 addr, uint8 val);
	virtual uint8 GetByteC64(uint16 addr);
	virtual uint8 GetByteFromRamC64(uint16 addr);
	virtual void MakeJmpC64(uint16 addr);
	virtual void MakeJmpNoResetC64(uint16 addr);
	virtual void MakeJsrC64(uint16 addr);
	
	
	virtual void MakeBasicRunC64();
	
	///
	virtual void SetStackPointerC64(uint8 val);
	virtual void SetRegisterAC64(uint8 val);
	virtual void SetRegisterXC64(uint8 val);
	virtual void SetRegisterYC64(uint8 val);
	virtual void SetRegisterPC64(uint8 val);

	///
	virtual void SetStackPointer1541(uint8 val);
	virtual void SetRegisterA1541(uint8 val);
	virtual void SetRegisterX1541(uint8 val);
	virtual void SetRegisterY1541(uint8 val);
	virtual void SetRegisterP1541(uint8 val);

	virtual void SetByte1541(uint16 addr, uint8 val);
	virtual void SetByteToRam1541(uint16 addr, uint8 val);
	virtual uint8 GetByte1541(uint16 addr);
	virtual uint8 GetByteFromRam1541(uint16 addr);
	virtual void MakeJmp1541(uint16 addr);
	virtual void MakeJmpNoReset1541(uint16 addr);

	// memory access for memory map
	virtual void GetWholeMemoryMap(uint8 *buffer);
	virtual void GetWholeMemoryMapFromRam(uint8 *buffer);
	virtual void GetWholeMemoryMap1541(uint8 *buffer);
	virtual void GetWholeMemoryMapFromRam1541(uint8 *buffer);

	virtual void GetMemoryC64(uint8 *buffer, int addrStart, int addrEnd);
	virtual void GetMemoryFromRam(uint8 *buffer, int addrStart, int addrEnd);
	virtual void GetMemoryFromRamC64(uint8 *buffer, int addrStart, int addrEnd);
	virtual void GetMemoryDrive1541(uint8 *buffer, int addrStart, int addrEnd);
	virtual void GetMemoryFromRamDrive1541(uint8 *buffer, int addrStart, int addrEnd);

	virtual void FillC64Ram(uint16 addr, uint16 size, uint8 value);

	virtual void GetVICColors(uint8 *cD021, uint8 *cD022, uint8 *cD023, uint8 *cD025, uint8 *cD026, uint8 *cD027, uint8 *cD800);
	virtual void GetVICSpriteColors(uint8 *cD021, uint8 *cD025, uint8 *cD026, uint8 *spriteColors);
	virtual void GetCBMColor(uint8 colorNum, uint8 *r, uint8 *g, uint8 *b);
	virtual void GetFloatCBMColor(uint8 colorNum, float *r, float *g, float *b);

	virtual bool LoadFullSnapshot(CByteBuffer *snapshotBuffer);
	virtual void SaveFullSnapshot(CByteBuffer *snapshotBuffer);
	virtual bool LoadFullSnapshot(char *filePath);
	virtual void SaveFullSnapshot(char *filePath);
	
	// these calls should be synced with CPU IRQ so snapshot store or restore is allowed
	// these calls should be synced with CPU IRQ so snapshot store or restore is allowed
	// store CHIPS only snapshot, not including DISK DATA
	virtual bool SaveFullSnapshotSynced(CByteBuffer *byteBuffer,
										bool saveChips, bool saveRoms, bool saveDisks, bool eventMode,
										bool saveReuData, bool saveCartRoms, bool saveScreen);
	virtual bool LoadChipsSnapshotSynced(CByteBuffer *byteBuffer);
	virtual bool SaveChipsSnapshotSynced(CByteBuffer *byteBuffer);
	// store DISK DATA only snapshot, without CHIPS
	virtual bool LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer);
	virtual bool SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer);

	virtual bool IsDriveDirtyForSnapshot();
	virtual void ClearDriveDirtyForSnapshotFlag();
	
	virtual void SetDebugMode(uint8 debugMode);
	virtual uint8 GetDebugMode();
	
	virtual bool IsCpuJam();
	virtual void ForceRunAndUnJamCpu();

	// tape
	virtual void AttachTape(CSlrString *filePath);
	virtual void DetachTape();
	virtual void DatasettePlay();
	virtual void DatasetteStop();
	virtual void DatasetteForward();
	virtual void DatasetteRewind();
	virtual void DatasetteRecord();
	virtual void DatasetteReset();
	virtual void DatasetteSetSpeedTuning(int speedTuning);
	virtual void DatasetteSetZeroGapDelay(int zeroGapDelay);
	virtual void DatasetteSetResetWithCPU(bool resetWithCPU);
	virtual void DatasetteSetTapeWobble(int tapeWobble);

	virtual void AttachCartridge(CSlrString *filePath);
	virtual void DetachCartridge();
	virtual void CartridgeFreezeButtonPressed();
	virtual void GetC64CartridgeState(C64StateCartridge *cartridgeState);

	virtual void DetachEverything();
	
	// reu
	virtual void SetReuEnabled(bool isEnabled);
	virtual void SetReuSize(int reuSize);
	virtual bool LoadReu(char *filePath);
	virtual bool SaveReu(char *filePath);

	//
	virtual void SetVicRegister(uint8 registerNum, uint8 value);
	virtual u8 GetVicRegister(uint8 registerNum);
	

	// CIA
	virtual void SetCiaRegister(uint8 ciaId, uint8 registerNum, uint8 value);
	virtual u8 GetCiaRegister(uint8 ciaId, uint8 registerNum);
	
	// SID
	CSidData *sidDataToRestore;
	virtual void SetSid(CSidData *sidData);
	virtual void SetSidRegister(uint8 sidId, uint8 registerNum, uint8 value);
	virtual u8 GetSidRegister(uint8 sidId, uint8 registerNum);

	// VIA
	virtual void SetViaRegister(uint8 driveId, uint8 viaId, uint8 registerNum, uint8 value);
	virtual u8 GetViaRegister(uint8 driveId, uint8 viaId, uint8 registerNum);
	
	virtual void SetVicRecordStateMode(uint8 recordMode);

	// SID
	virtual void SetSIDMuteChannels(int sidNumber, bool mute1, bool mute2, bool mute3, bool muteExt);
	virtual void SetSIDReceiveChannelsData(int sidNumber, bool isReceiving);

	// memory
	uint8 *c64memory;

	//
	virtual void SetPatchKernalFastBoot(bool isPatchKernal);
	virtual void SetRunSIDWhenInWarp(bool isRunningSIDInWarp);
	
	//
	virtual void SetRunSIDEmulation(bool isSIDEmulationOn);
	virtual void SetAudioVolume(float volume);
		
	// sid data history
	CSlrMutex *mutexSidDataHistory;
	int sidDataHistorySteps;
	void SetSidDataHistorySteps(int numSteps);
	std::list<CSidData *> sidDataHistory;
	volatile int sidDataHistoryCurrentStep;
	virtual void UpdateSidDataHistory();
	
	// profiler
	// if fileName is NULL no file will be created, if runForNumCycles is -1 it will run till ProfilerDeactivate
	virtual void ProfilerActivate(char *fileName, int runForNumCycles, bool pauseCpuWhenFinished);
	virtual void ProfilerDeactivate();
	virtual bool IsProfilerActive();

	// drive flush thread
	CThreadViceDriveFlush *driveFlushThread;
	
//	// set correct disk ID to let 1541 ROM not throw 29, 'disk id mismatch'
//	// see $F3F6 in 1541 ROM: http://unusedino.de/ec64/technical/misc/c1541/romlisting.html#FDD3
//	virtual void UpdateDriveDiskID(int driveId);

	// interface to vice monitor
	virtual bool IsCodeMonitorSupported();
	bool isCodeMonitorOpened;
	virtual CSlrString *GetCodeMonitorPrompt();
	virtual bool ExecuteCodeMonitorCommand(CSlrString *commandStr);
	
	// ROMs
	virtual void ScanFolderForRoms(const char *folderPath);
	virtual void UpdateRoms();
	virtual void UpdateRomsTrap();
	virtual void ReadEmbeddedRoms();
	virtual void DumpRomsToFolder(const char *folderPath);
	virtual void CheckLoadedRoms();
	
	//
	virtual void SupportsBreakpoints(bool *writeBreakpoint, bool *readBreakpoint);
	
	//
	virtual void Shutdown();
};

#define C64_NUM_SID_REGISTERS 0x20

class CSidData
{
public:
	CSidData();
	
	void PeekFromSids();
	void RestoreSids();
	void CopyFrom(CSidData *sidData);
	u8 sidRegs[SOUND_SIDS_MAX][C64_NUM_SID_REGISTERS];
	
	void Serialize(CByteBuffer *byteBuffer);
	bool Deserialize(CByteBuffer *byteBuffer);
	bool SaveRegs(const char *fileName);
	bool LoadRegs(const char *fileName);

private:
	static CPool poolSidData;
public:
	static void* operator new(const size_t size) { return poolSidData.New(size); }
	static void operator delete(void* pObject) { poolSidData.Delete(pObject); }
};

extern CDebugInterfaceVice *debugInterfaceVice;

void ViceKeyMapInitDefault();

#endif

