#ifndef _CDebugInterfaceAtari_H_
#define _CDebugInterfaceAtari_H_

#include "SYS_Defs.h"
#include "CImageData.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDataAdapter.h"

extern "C"
{
#include "AtariWrapper.h"
};

#define NUM_ATARI_JOYSTICKS	4

class CAudioChannelAtari;

class CDebugInterfaceAtari : public CDebugInterface
{
	
public:
	CDebugInterfaceAtari(CViewC64 *viewC64); //, uint8 *memory);
	~CDebugInterfaceAtari();
	
	virtual int GetEmulatorType();
	virtual CSlrString *GetEmulatorVersionString();
	virtual const char *GetPlatformNameString();

	virtual float GetEmulationFPS();
	float numEmulationFPS;

	virtual void RunEmulationThread();

	CAudioChannelAtari *audioChannel;
	CDataAdapter *dataAdapter;

	void RestartEmulation();
	//	virtual void InitKeyMap(C64KeyMap *keyMap);
	

	virtual void DoFrame();
	virtual int GetScreenSizeX();
	virtual int GetScreenSizeY();

	// keyboard & joystick mapper
	virtual bool KeyboardDown(uint32 mtKeyCode);
	virtual bool KeyboardUp(uint32 mtKeyCode);
	
	volatile uint32 joystickState[NUM_ATARI_JOYSTICKS];
	virtual void JoystickDown(int port, uint32 axis);
	virtual void JoystickUp(int port, uint32 axis);
	
	//
	virtual void SetDebugMode(uint8 debugMode);
	virtual uint8 GetDebugMode();

	//
	virtual void Reset();
	virtual void HardReset();
	
	// this is main emulation cpu cycle counter
	virtual u64 GetMainCpuCycleCounter();
	virtual u64 GetPreviousCpuInstructionCycleCounter();
	
	// resettable counters for debug purposes
	virtual void ResetMainCpuDebugCycleCounter();
	virtual u64 GetMainCpuDebugCycleCounter();
	
	virtual void RefreshScreenNoCallback();

	virtual bool LoadExecutable(char *fullFilePath);
	virtual bool MountDisk(char *fullFilePath, int diskNo, bool readOnly);
	virtual bool InsertCartridge(char *fullFilePath, bool readOnly);
	virtual bool AttachTape(char *fullFilePath, bool readOnly);
	
	virtual void DetachEverything();
	virtual void DetachDriveDisk();
	virtual void DetachCartridge();

	//
	virtual bool GetSettingIsWarpSpeed();
	virtual void SetSettingIsWarpSpeed(bool isWarpSpeed);

	// TODO: this is not working for mzpokey now
	virtual void SetPokeyStereo(bool isStereo);
	
	//
	virtual bool LoadFullSnapshot(char *filePath);
	virtual void SaveFullSnapshot(char *filePath);

	// state
	virtual int GetCpuPC();
	
	virtual void GetWholeMemoryMap(uint8 *buffer);
	virtual void GetWholeMemoryMapFromRam(uint8 *buffer);

	//
	void GetCpuRegs(u16 *PC, u8 *A, u8 *X, u8 *Y, u8 *P, u8 *S, u8 *IRQ);
	
	int MapMTKeyToAKey(uint32 mtKeyCode, int shiftctrl, int key_control);
	
	void SetVideoSystem(u8 videoSystem);
	void SetMachineType(u8 machineType);
	void SetRamSizeOption(u8 ramSizeOption);
	
	virtual void SetByte(uint16 addr, uint8 val);
	//	virtual void SetByteToRamC64(uint16 addr, uint8 val);

	virtual uint8 GetByte(uint16 addr);

	virtual void GetMemory(uint8 *buffer, int addrStart, int addrEnd);

	//
	// make jmp without resetting CPU depending on dataAdapter
	virtual void MakeJmpNoReset(CDataAdapter *dataAdapter, uint16 addr);
	
	// make jmp and reset CPU
	virtual void MakeJmpAndReset(uint16 addr);

	virtual CDataAdapter *GetDataAdapter();

	// these calls should be synced with CPU IRQ so snapshot store or restore is allowed
	// store CHIPS only snapshot, not including DISK DATA
	virtual bool LoadChipsSnapshotSynced(CByteBuffer *byteBuffer);
	virtual bool SaveChipsSnapshotSynced(CByteBuffer *byteBuffer);
	// store DISK DATA only snapshot, without CHIPS
	virtual bool LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer);
	virtual bool SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer);
	
	// controls if we should also store disk drive snapshot when snapshot interval is hit
	// this is to allow only periodic disk drive snapshot storing, only when disk was changed or new data on disk was saved
	// when we restore snapshot, we will restore disk contents first
	virtual bool IsDriveDirtyForSnapshot();
	virtual void ClearDriveDirtyForSnapshotFlag();

	//
	void SetPokeyReceiveChannelsData(int pokeyNumber, bool isReceiving);

	virtual void StepOneCycle();

	
//	virtual uint8 GetByteFromRamC64(uint16 addr);
//	virtual void MakeJmpC64(uint16 addr);
//	virtual void MakeJmpNoResetC64(uint16 addr);
//	virtual void MakeJsrC64(uint16 addr);
//	
//	
//	virtual void MakeBasicRunC64();
//	
//	///
//	virtual void SetStackPointerC64(uint8 val);
//	virtual void SetRegisterAC64(uint8 val);
//	virtual void SetRegisterXC64(uint8 val);
//	virtual void SetRegisterYC64(uint8 val);
//	virtual void SetRegisterPC64(uint8 val);
//	
//	///
//	virtual void SetStackPointer1541(uint8 val);
//	virtual void SetRegisterA1541(uint8 val);
//	virtual void SetRegisterX1541(uint8 val);
//	virtual void SetRegisterY1541(uint8 val);
//	virtual void SetRegisterP1541(uint8 val);
//	
//	virtual void SetByte1541(uint16 addr, uint8 val);
//	virtual void SetByteToRam1541(uint16 addr, uint8 val);
//	virtual uint8 GetByte1541(uint16 addr);
//	virtual uint8 GetByteFromRam1541(uint16 addr);
//	virtual void MakeJmp1541(uint16 addr);
//	virtual void MakeJmpNoReset1541(uint16 addr);
//	
//	// memory access for memory map
//	virtual void GetWholeMemoryMapC64(uint8 *buffer);
//	virtual void GetWholeMemoryMapFromRamC64(uint8 *buffer);
//	virtual void GetWholeMemoryMap1541(uint8 *buffer);
//	virtual void GetWholeMemoryMapFromRam1541(uint8 *buffer);
//	
//	virtual void GetMemoryC64(uint8 *buffer, int addrStart, int addrEnd);
//	virtual void GetMemoryFromRamC64(uint8 *buffer, int addrStart, int addrEnd);
//	virtual void GetMemoryDrive1541(uint8 *buffer, int addrStart, int addrEnd);
//	virtual void GetMemoryFromRamDrive1541(uint8 *buffer, int addrStart, int addrEnd);
//	
//	virtual void FillC64Ram(uint16 addr, uint16 size, uint8 value);
//	
//	virtual void GetVICColors(uint8 *cD021, uint8 *cD022, uint8 *cD023, uint8 *cD025, uint8 *cD026, uint8 *cD027, uint8 *cD800);
//	virtual void GetVICSpriteColors(uint8 *cD021, uint8 *cD025, uint8 *cD026, uint8 *spriteColors);
//	virtual void GetCBMColor(uint8 colorNum, uint8 *r, uint8 *g, uint8 *b);
//	virtual void GetFloatCBMColor(uint8 colorNum, float *r, float *g, float *b);
//	
//	virtual bool LoadFullSnapshot(CByteBuffer *snapshotBuffer);
//	virtual void SaveFullSnapshot(CByteBuffer *snapshotBuffer);
//	
//	virtual bool LoadFullSnapshot(char *filePath);
//	virtual void SaveFullSnapshot(char *filePath);
//	
//	virtual void SetDebugMode(uint8 debugMode);
//	virtual uint8 GetDebugMode();
//	
//	virtual bool IsCpuJam();
//	virtual void ForceRunAndUnJamCpu();
//	
//	virtual void AttachCartridge(CSlrString *filePath);
//	virtual void DetachCartridge();
//	virtual void CartridgeFreezeButtonPressed();
//	virtual void GetC64CartridgeState(C64StateCartridge *cartridgeState);
//	
//	//
//	virtual void SetVicRegister(uint8 registerNum, uint8 value);
//	virtual u8 GetVicRegister(uint8 registerNum);
//	
//	virtual void SetVicRecordStateMode(uint8 recordMode);
//	
//	// render states
//	virtual void RenderStateVIC(vicii_cycle_state_t *viciiState,
//								float posX, float posY, float posZ, bool isVertical, bool showSprites, CSlrFont *fontBytes, float fontSize,
//								std::vector<CImageData *> *spritesImageData, std::vector<CSlrImage *> *spritesImages, bool renderDataWithColors);
//	void PrintVicInterrupts(uint8 flags, char *buf);
//	void UpdateVICSpritesImages(vicii_cycle_state_t *viciiState,
//								std::vector<CImageData *> *spritesImageData,
//								std::vector<CSlrImage *> *spritesImages, bool renderDataWithColors);
//	
//	virtual void RenderStateDrive1541(float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize,
//									  bool renderVia1, bool renderVia2, bool renderDriveLed, bool isVertical);
//	virtual void RenderStateCIA(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int ciaId);
//	virtual void RenderStateSID(uint16 sidBase, float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize);
//	void PrintSidWaveform(uint8 wave, char *buf);
//	
//	// SID
//	virtual void SetSIDMuteChannels(int sidNumber, bool mute1, bool mute2, bool mute3, bool muteExt);
//	virtual void SetSIDReceiveChannelsData(int sidNumber, bool isReceiving);
//	
//	// memory
//	uint8 *c64memory;
//	
//	virtual void SetPatchKernalFastBoot(bool isPatchKernal);
//	virtual void SetRunSIDWhenInWarp(bool isRunningSIDInWarp);
//	
//	//
//	virtual void SetRunSIDEmulation(bool isSIDEmulationOn);
//	virtual void SetAudioVolume(float volume);
};

extern CDebugInterfaceAtari *debugInterfaceAtari;

#endif
