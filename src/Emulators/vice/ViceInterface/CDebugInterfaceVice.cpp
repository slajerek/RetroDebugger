extern "C" {
#include "vice.h"
#include "main.h"
#include "vicetypes.h"
#include "mos6510.h"
#include "montypes.h"
#include "attach.h"
#include "keyboard.h"
#include "drivecpu.h"
#include "machine.h"
#include "vsync.h"
#include "interrupt.h"
#include "c64-snapshot.h"
#include "viciitypes.h"
#include "vicii.h"
#include "vicii-mem.h"
#include "drivetypes.h"
#include "drive.h"
#include "cia.h"
#include "c64.h"
#include "sid.h"
#include "sid-resources.h"
#include "drive.h"
#include "datasette.h"
#include "c64mem.h"
#include "c64model.h"
#include "driverom.h"
#include "ui.h"
#include "resources.h"
#include "ViceWrapper.h"
}

#include "RES_ResourceManager.h"
#include "CDebugInterfaceVice.h"
#include "CDebuggerApiVice.h"
#include "CDebugMemory.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "CDataAdaptersVice.h"
#include "SYS_CommandLine.h"
#include "CGuiMain.h"
#include "SYS_Main.h"
#include "SYS_KeyCodes.h"
#include "SND_SoundEngine.h"
#include "SYS_FileSystem.h"
#include "C64Tools.h"
#include "C64KeyMap.h"
#include "C64SettingsStorage.h"
#include "C64SIDFrequencies.h"
#include "CViewC64.h"
#include "CViewC64StateSID.h"
#include "CViewC64SidTrackerHistory.h"
#include "CViewDrive1541Browser.h"
#include "CAudioChannelVice.h"
#include "CDebuggerEmulatorPlugin.h"
#include "CSnapshotsManager.h"

#include "CDataAdapterViceDrive1541.h"
#include "CDataAdapterDrive1541Minimal.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsDrive1541.h"
#include "CWaveformData.h"
#include "CViewDataMap.h"
#include "CDebugEventsHistory.h"
#include "CDebuggerServerApiVice.h"

// 44100/50*5
#define SID_WAVEFORM_LENGTH 4410*8

extern "C" {
void vsync_suspend_speed_eval(void);
void c64d_reset_sound_clk();
void sound_suspend(void);
void sound_resume(void);
int c64d_sound_run_sound_when_paused(void);
int set_suspend_time(int val, void *param);

void c64d_set_debug_mode(int newMode);
void c64d_patch_kernal_fast_boot();
void c64d_init_memory(uint8 *c64memory);

int resources_get_int(const char *name, int *value_return);

disk_image_t *c64d_get_drive_disk_image(int driveId);
}

void c64d_update_c64_model();
void c64d_update_c64_machine_from_model_type(int modelType);
void c64d_update_c64_screen_height_from_model_type(int modelType);

void ViceWrapperInit(CDebugInterfaceVice *debugInterface);

CDebugInterfaceVice *debugInterfaceVice = NULL;

CDebugInterfaceVice::CDebugInterfaceVice(CViewC64 *viewC64, uint8 *c64memory, bool patchKernalFastBoot)
: CDebugInterfaceC64(viewC64)
{
	LOGM("CDebugInterfaceVice: VICE %s init", VERSION);

	CreateScreenData();

	audioChannel = NULL;
	snapshotsManager = new CSnapshotsManager(this);

	for (int sidNum = 0; sidNum < C64_MAX_NUM_SIDS; sidNum++)
	{
		for (int i = 0; i < 3; i++)
		{
			sidChannelWaveform[sidNum][i] = new CWaveformData(SID_WAVEFORM_LENGTH);
		}
		sidMixWaveform[sidNum] = new CWaveformData(SID_WAVEFORM_LENGTH);
	}
	
	ViceWrapperInit(this);
	
	ReadEmbeddedRoms();
	
	// set patch kernal flag
	//	SetPatchKernalFastBoot(patchKernalFastBoot);
	if (patchKernalFastBoot)
	{
		c64d_patch_kernal_fast_boot_flag = 1;
	}
	else
	{
		c64d_patch_kernal_fast_boot_flag = 0;
	}

	numSids = 1;
	
	// PAL
	machineType = MACHINE_TYPE_PAL;
	debugInterfaceVice->numEmulationFPS = 50;

	// init C64 memory, will be attached to a memmaped file if needed
	if (c64memory == NULL)
	{
		this->c64memory = (uint8 *)malloc(C64_RAM_SIZE);
	}
	else
	{
		this->c64memory = c64memory;
	}

	c64d_init_memory(this->c64memory);
	
	mutexSidDataHistory = new CSlrMutex("mutexSidDataHistory");
	sidDataHistoryCurrentStep = 0;
	sidDataHistorySteps = 1;
	sidDataToRestore = NULL;
	
	driveFlushThread = NULL;

	InitViceMainProgram();

	// create default symbols and data adapters
	
	// C64
	symbolsC64 = new CDebugSymbolsC64(this);
	symbols = symbolsC64;
	
	dataAdapterViceC64 = new CDataAdapterViceC64(symbols);
	dataAdapterC64 = dataAdapterViceC64;
	symbols->SetDataAdapter(dataAdapterC64);
	symbols->CreateDefaultSegment();
	
	dataAdapterViceC64DirectRam = new CDataAdapterViceC64DirectRam(symbols);
	dataAdapterC64DirectRam = dataAdapterViceC64DirectRam;

	// Drive1541
	symbolsDrive1541 = new CDebugSymbolsDrive1541(this);

	dataAdapterViceDrive1541 = new CDataAdapterViceDrive1541(symbolsDrive1541);
	dataAdapterDrive1541 = dataAdapterViceDrive1541;

	symbolsDrive1541->SetDataAdapter(dataAdapterDrive1541);
	symbolsDrive1541->CreateDefaultSegment();

	dataAdapterViceDrive1541DirectRam = new CDataAdapterViceDrive1541DirectRam(symbolsDrive1541);
	dataAdapterDrive1541DirectRam = dataAdapterViceDrive1541DirectRam;

	//
	dataAdapterDrive1541MinimalRam = new CDataAdapterDrive1541Minimal(symbolsDrive1541, dataAdapterViceDrive1541);

	// Drive1541DiskContents
	symbolsDrive1541DiskContents = new CDebugSymbols(this, false);
	dataAdapterViceDrive1541DiskContents = new CDataAdapterViceDrive1541DiskContents(symbolsDrive1541DiskContents, 0);
	dataAdapterDrive1541DiskContents = dataAdapterViceDrive1541DiskContents;
	
	symbolsDrive1541DiskContents->SetDataAdapter(dataAdapterViceDrive1541DiskContents);
	symbolsDrive1541DiskContents->CreateDefaultSegment();

	// C64Cartridge
	symbolsCartridgeC64 = new CDebugSymbols(this, false);
	dataAdapterViceC64Cartridge = new CDataAdapterViceC64Cartridge(symbolsCartridgeC64, CCartridgeDataAdapterViceType::C64CartridgeDataAdapterViceTypeRamL);
	dataAdapterCartridgeC64 = dataAdapterViceC64Cartridge;

	symbolsCartridgeC64->SetDataAdapter(dataAdapterViceC64Cartridge);
	symbolsCartridgeC64->CreateDefaultSegment();

	C64KeyMap *keyMap = C64KeyMapGetDefault();
	InitKeyMap(keyMap);
	
	isCodeMonitorOpened = false;
}

CDebugInterfaceVice::~CDebugInterfaceVice()
{
}

void CDebugInterfaceVice::InitViceMainProgram()
{
	int ret = vice_main_program(sysArgc, sysArgv, c64SettingsC64Model);
	if (ret != 0)
	{
		LOGError("Vice failed, err=%d", ret);
	}
	
	// note, vice changes and strips argv, we need to update the argc to be properly later parsed by other emulators
	for (int i = 1; i < sysArgc; i++)
	{
		if (sysArgv[i] == NULL)
		{
			sysArgc = i;
			break;
		}
	}
}

extern "C" {
	void c64d_patch_kernal_fast_boot();
	void c64d_un_patch_kernal_fast_boot();
	void c64d_update_rom();
};

float CDebugInterfaceVice::GetEmulationFPS()
{
	return numEmulationFPS;
}

void CDebugInterfaceVice::SetPatchKernalFastBoot(bool isPatchKernal)
{
	LOGM("CDebugInterfaceVice::SetPatchKernalFastBoot: %d", isPatchKernal);

	c64d_un_patch_kernal_fast_boot();
	
	if (isPatchKernal)
	{
		c64d_patch_kernal_fast_boot_flag = 1;
		c64d_patch_kernal_fast_boot();
	}
	else
	{
		c64d_patch_kernal_fast_boot_flag = 0;
	}
	
	c64d_update_rom();
}

void CDebugInterfaceVice::SetSkipDrawingSprites(bool isSkipDrawingSprites)
{
	LOGM("CDebugInterfaceVice::SetSkipDrawingSprites: %s", STRBOOL(isSkipDrawingSprites));
	
	if (isSkipDrawingSprites)
	{
		c64d_skip_drawing_sprites = 1;
	}
	else
	{
		c64d_skip_drawing_sprites = 0;
	}
}

void CDebugInterfaceVice::SetRunSIDWhenInWarp(bool isRunningSIDInWarp)
{
	c64d_setting_run_sid_when_in_warp = isRunningSIDInWarp ? 1 : 0;
}

void CDebugInterfaceVice::SetRunSIDEmulation(bool isSIDEmulationOn)
{
	// this does not stop sound via playback_enable, but just skips the SID emulation
	// thus, the CPU emulation will be in correct sync
	
	LOGD("CDebugInterfaceVice::SetRunSIDEmulation: %s", STRBOOL(isSIDEmulationOn));
	c64d_setting_run_sid_emulation = isSIDEmulationOn ? 1 : 0;
}

void CDebugInterfaceVice::SetAudioVolume(float volume)
{
	LOGD("CDebugInterfaceVice::SetAudioVolume: %f", volume);
	c64d_set_volume(volume);
}

int CDebugInterfaceVice::GetEmulatorType()
{
	return EMULATOR_TYPE_C64_VICE;
}

CSlrString *CDebugInterfaceVice::GetEmulatorVersionString()
{
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "Vice %s", VERSION);	// by The VICE Team
	CSlrString *versionString = new CSlrString(buf);
	SYS_ReleaseCharBuf(buf);
	
	return versionString;
}

#if defined(WIN32)
extern "C" {
	int uilib_cpu_is_smp(void);
	int set_single_cpu(int val, void *param);	// 1=set to first CPU, 0=set to all CPUs
}
#endif

void CDebugInterfaceVice::RunEmulationThread()
{
	LOGM("CDebugInterfaceVice::RunEmulationThread");
	CDebugInterface::RunEmulationThread();

	isRunning = true;

#if defined(WIN32)
	if (c64SettingsUseOnlyFirstCPU)
	{
		if (uilib_cpu_is_smp() == 1)
		{
			LOGD("CDebugInterfaceVice: set UseOnlyFirstCPU");
			set_single_cpu(1, NULL);
		}
	}
#endif
	
	// vice blocks d64 for read when mounted and does the flush only on disk unmount or quit. this leads to not saved data immediately.
	// thus, we do not block d64 for read and avoid that data is not flushed we check periodically if there's a need to flush data

	if (driveFlushThread == NULL)
	{
		driveFlushThread = new CThreadViceDriveFlush(this, 2500); // every 2.5s
		SYS_StartThread(driveFlushThread);
	}
	
	// update sid type
	SetSidTypeAsync(c64SettingsSIDEngineModel);
	
//	audioChannel->Start();
	
	vice_main_loop_run();
	
//	audioChannel->Stop();

	isRunning = false;
	LOGM("CDebugInterfaceVice::RunEmulationThread: finished");
}

void CDebugInterfaceVice::SetSidDataHistorySteps(int numSteps)
{
	sidDataHistorySteps = numSteps;
}

void CDebugInterfaceVice::UpdateSidDataHistory()
{
	mutexSidDataHistory->Lock();
	
	CSidData *sidData;
	if (sidDataHistory.size() > c64SettingsSidDataHistoryMaxSize)
	{
		sidData = sidDataHistory.back();
		sidDataHistory.pop_back();
	}
	else
	{
		sidData = new CSidData();
	}
	
	if (sidDataToRestore)
	{
		sidData->CopyFrom(sidDataToRestore);
	}
	else
	{
		sidData->PeekFromSids();
	}
	
	sidDataHistory.push_front(sidData);
	
	sidDataHistoryCurrentStep++;
	
	if (sidDataHistoryCurrentStep >= sidDataHistorySteps)
	{
		sidDataHistoryCurrentStep = 0;
		
		// TODO: make a proper callback for this:
		viewC64->viewC64SidTrackerHistory->VSyncStepsAdded();
	}
	mutexSidDataHistory->Unlock();
}

extern "C" {
	void c64d_joystick_latch_matrix_workaround();
	void vsync_sync_reset(void);
};

void CDebugInterfaceVice::DoVSync()
{
//	LOGD("CDebugInterfaceVice::DoVSync: store sid history for sid tracker view");
	UpdateSidDataHistory();
	CDebugInterfaceC64::DoVSync();
}

void CDebugInterfaceVice::DoFrame()
{
	CDebugInterfaceC64::DoFrame();
}

void CDebugInterfaceVice::RefreshSync()
{
	vsync_sync_reset();
}

void CDebugInterfaceVice::RestartAudio()
{
	audioChannel->Start();
	RefreshSync();
}

CThreadViceDriveFlush::CThreadViceDriveFlush(CDebugInterfaceVice *debugInterface, int flushCheckIntervalInMS)
{
	this->debugInterface = debugInterface;
	this->flushCheckIntervalInMS = flushCheckIntervalInMS;
}

void CThreadViceDriveFlush::ThreadRun(void *data)
{
//	LOGD("CViceDriveFlushThread started");
	while(true)
	{
		if (isRunning)
		{
			SYS_Sleep(flushCheckIntervalInMS);

			if (!isRunning)
				continue;

			debugInterface->LockMutex();
			if (debugInterface->snapshotsManager->snapshotToRestore
				|| debugInterface->snapshotsManager->pauseNumFrame != -1)
			{
				debugInterface->UnlockMutex();
				SYS_Sleep(flushCheckIntervalInMS * 4);
				continue;
			}

	//		LOGD("CViceDriveFlushThread: flushing drive");
			drive_gcr_data_writeback_all();
			debugInterface->UnlockMutex();
	//		LOGD("CViceDriveFlushThread: flushing drive finished");
			
		}
		
	}
}

extern "C" {
	void c64d_shutdown_vice();
}

void CDebugInterfaceVice::Shutdown()
{
	LOGD("CDebugInterfaceVice::Shutdown");
	
	this->LockMutex();
	drive_gcr_data_writeback_all();
	this->UnlockMutex();
	
	c64d_shutdown_vice();
	
	while(isRunning)
	{
		SYS_Sleep(50);
	}
	
	CDebugInterfaceC64::Shutdown();
	
	LOGD("Vice Shutdown finished");
}

void CDebugInterfaceVice::InitKeyMap(C64KeyMap *keyMap)
{
	LOGD("CDebugInterfaceVice::InitKeyMap");
	c64d_keyboard_keymap_clear();
	
	for (std::map<u32, C64KeyCode *>::iterator it = keyMap->keyCodes.begin();
		 it != keyMap->keyCodes.end(); it++)
	{
		C64KeyCode *key = it->second;
		
		if (key->matrixRow < 0)
		{
			// restore, caps lock, ...
			keyboard_parse_set_neg_row(key->keyCode, key->matrixRow, key->matrixCol);
		}
		else
		{
			
			//LOGD("... %04x %3d %3d %d", key->keyCode, key->matrixRow, key->matrixCol, key->shift);
			keyboard_parse_set_pos_row(key->keyCode, key->matrixRow, key->matrixCol, key->shift);
		}
	}
	
	
}

uint8 *CDebugInterfaceVice::GetCharRom()
{
	return mem_chargen_rom;
}

int CDebugInterfaceVice::GetScreenSizeX()
{
	int screenWidth, screenHeight, gfxPosX, gfxPosY;
	GetViciiGeometry(&screenWidth, &screenHeight, &gfxPosX, &gfxPosY);
	return screenWidth;
}

int CDebugInterfaceVice::GetScreenSizeY()
{
	int screenWidth, screenHeight, gfxPosX, gfxPosY;
	GetViciiGeometry(&screenWidth, &screenHeight, &gfxPosX, &gfxPosY);
	return screenHeight; //-16;
}

bool CDebugInterfaceVice::IsCpuJam()
{
	if (c64d_is_cpu_in_jam_state == 1)
	{
		return true;
	}
	
	return false;
}

void CDebugInterfaceVice::ForceRunAndUnJamCpu()
{
	c64d_is_cpu_in_jam_state = 0;
	this->SetDebugMode(DEBUGGER_MODE_RUNNING);
}

void CDebugInterfaceVice::ClearDebugMarkers()
{
	symbols->memory->ClearDebugMarkers();
	symbolsDrive1541->memory->ClearDebugMarkers();
	symbolsCartridgeC64->memory->ClearDebugMarkers();
}

void CDebugInterfaceVice::ResetSoft()
{
	vsync_suspend_speed_eval();
	
	keyboard_clear_keymatrix();

	machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
	this->ResetEmulationFrameCounter();
	c64d_maincpu_clk = 6;

	c64d_update_c64_model();

	if (c64d_is_cpu_in_jam_state == 1)
	{
		this->SetDebugMode(DEBUGGER_MODE_RUNNING);
		c64d_is_cpu_in_jam_state = 0;
	}
}

void CDebugInterfaceVice::ResetHard()
{
	LOGD("CDebugInterfaceVice::ResetHard");
	vsync_suspend_speed_eval();
	
	keyboard_clear_keymatrix();

	machine_trigger_reset(MACHINE_RESET_MODE_HARD);
	this->ResetEmulationFrameCounter();
	this->ClearHistory();
	
	c64d_maincpu_clk = 6;

	c64d_update_c64_model();

	if (c64d_is_cpu_in_jam_state == 1)
	{
		this->SetDebugMode(DEBUGGER_MODE_RUNNING);
		c64d_is_cpu_in_jam_state = 0;
	}
}

void CDebugInterfaceVice::DiskDriveReset()
{
	LOGM("CDebugInterfaceVice::DiskDriveReset()");
	
	drivecpu_reset(drive_context[0]);
}

extern "C" {
	unsigned int c64d_get_vice_maincpu_clk();
	unsigned int c64d_get_vice_maincpu_current_instruction_clk();
	void c64d_refresh_screen_no_callback();
}

u64 CDebugInterfaceVice::GetMainCpuCycleCounter()
{
	return c64d_get_vice_maincpu_clk();
}

u64 CDebugInterfaceVice::GetCurrentCpuInstructionCycleCounter()
{
	return c64d_get_vice_maincpu_current_instruction_clk();
}

u64 CDebugInterfaceVice::GetPreviousCpuInstructionCycleCounter()
{
	u64 viceMainCpuClk = c64d_get_vice_maincpu_clk();
	u64 previousInstructionClk = c64d_maincpu_previous_instruction_clk;
	u64 previous2InstructionClk = c64d_maincpu_previous2_instruction_clk;
	LOGD(">>>>>>>>>................ previous_inst_clk=%d previous2InstructionClk=%d | mainclk=%d", previousInstructionClk, previous2InstructionClk, viceMainCpuClk);
	
	if (previousInstructionClk == viceMainCpuClk)
	{
		LOGWarning("previousInstructionClk=%d == viceMainCpuClk=%d, previousInstructionClk will be previous2InstructionClk=%d", previousInstructionClk, viceMainCpuClk, previous2InstructionClk);
		
		// snapshot was recently restored and debug pause was moved forward or we have no data (i.e. snapshot restored etc)
		previousInstructionClk = previous2InstructionClk;
	}
	
	return previousInstructionClk;
}

void CDebugInterfaceVice::ResetMainCpuDebugCycleCounter()
{
	c64d_maincpu_clk = 0;
}

extern "C" {
	unsigned int c64d_get_vice_maincpu_clk();
};

u64 CDebugInterfaceVice::GetMainCpuDebugCycleCounter()
{
	return c64d_maincpu_clk;
}

extern "C" {
	void c64d_refresh_screen_no_callback();
}

void CDebugInterfaceVice::ResetEmulationFrameCounter()
{
	this->ClearHistory();
	CDebugInterfaceC64::ResetEmulationFrameCounter();
}

unsigned int CDebugInterfaceVice::GetEmulationFrameNumber()
{
	return CDebugInterfaceC64::GetEmulationFrameNumber();
}

void CDebugInterfaceVice::RefreshScreenNoCallback()
{
	c64d_refresh_screen_no_callback();
}

extern "C" {
	void c64d_joystick_key_down(int key, unsigned int joyport);
	void c64d_joystick_key_up(int key, unsigned int joyport);
}

bool CDebugInterfaceVice::KeyboardDown(uint32 mtKeyCode)
{
	for (std::list<CDebuggerEmulatorPlugin *>::iterator it = this->plugins.begin(); it != this->plugins.end(); it++)
	{
		CDebuggerEmulatorPlugin *plugin = *it;
		mtKeyCode = plugin->KeyDown(mtKeyCode);
		
		if (mtKeyCode == 0)
			return false;
	}
	
	if (keyboard_key_pressed((unsigned long)mtKeyCode) == 1)
		return true;
	
	return false;
}

bool CDebugInterfaceVice::KeyboardUp(uint32 mtKeyCode)
{
	for (std::list<CDebuggerEmulatorPlugin *>::iterator it = this->plugins.begin(); it != this->plugins.end(); it++)
	{
		CDebuggerEmulatorPlugin *plugin = *it;
		mtKeyCode = plugin->KeyUp(mtKeyCode);
		
		if (mtKeyCode == 0)
			return false;
	}
	
	if (keyboard_key_released((unsigned long)mtKeyCode) == 1)
		return true;
	
	return false;
}

void CDebugInterfaceVice::JoystickDown(int port, uint32 axis)
{
//	LOGD("CDebugInterfaceVice::JoystickDown %d %d", port, axis);
	c64d_joystick_key_down(axis, port+1);
}

void CDebugInterfaceVice::JoystickUp(int port, uint32 axis)
{
//	LOGD("CDebugInterfaceVice::JoystickUp %d %d", port, axis);
	c64d_joystick_key_up(axis, port+1);
}

extern "C" {
int set_mouse_enabled(int val, void *param);
int joyport_set_device(int port, int id);
void c64d_mouse_set_type(int mt);
int c64d_mouse_type_to_joyportid(int mouseType);
void mousedrv_button_left(int pressed);
void mousedrv_button_right(int pressed);
void mousedrv_button_middle(int pressed);
void mousedrv_button_up(int pressed);
void mousedrv_button_down(int pressed);
void mouse_move(int x, int y);
void c64d_mouse_set_position(int x, int y);
};

#define JOYPORT_ID_NONE                0
#define JOYPORT_ID_JOYSTICK            1

void CDebugInterfaceVice::EmulatedMouseUpdateSettings()
{
	// reset mouse
	set_mouse_enabled(0, NULL);
	
	// set mouse enabled as per settings
	if (c64SettingsEmulatedMouseC64Enabled)
	{
		if (set_mouse_enabled((c64SettingsEmulatedMouseC64Enabled ? 1:0), NULL) < 0)
		{
			return;
		}
		c64d_mouse_set_type(c64SettingsEmulatedMouseC64Type);
		
		// set ports
		joyport_set_device(0, JOYPORT_ID_JOYSTICK);
		joyport_set_device(1, JOYPORT_ID_JOYSTICK);

		if (c64SettingsEmulatedMouseC64Enabled)
		{
			int joyportId = c64d_mouse_type_to_joyportid(c64SettingsEmulatedMouseC64Type);
			joyport_set_device(c64SettingsEmulatedMouseC64Port, joyportId);
		}
	}
}

bool CDebugInterfaceVice::EmulatedMouseEnable(bool enable)
{
	c64SettingsEmulatedMouseC64Enabled = enable;
	EmulatedMouseUpdateSettings();
	return true;
}

void CDebugInterfaceVice::EmulatedMouseSetType(int mouseType)
{
	c64SettingsEmulatedMouseC64Type = mouseType;
	EmulatedMouseUpdateSettings();
}

void CDebugInterfaceVice::EmulatedMouseSetPort(int port)
{
	c64SettingsEmulatedMouseC64Port = port;
	EmulatedMouseUpdateSettings();
}

void CDebugInterfaceVice::EmulatedMouseSetPosition(int newX, int newY)
{
	c64d_mouse_set_position(newX, newY);
}

void CDebugInterfaceVice::EmulatedMouseButtonLeft(bool isPressed)
{
	mousedrv_button_left(isPressed ? 1:0);
}

void CDebugInterfaceVice::EmulatedMouseButtonMiddle(bool isPressed)
{
	mousedrv_button_middle(isPressed ? 1:0);
}

void CDebugInterfaceVice::EmulatedMouseButtonRight(bool isPressed)
{
	mousedrv_button_right(isPressed ? 1:0);
}

int CDebugInterfaceVice::GetCpuPC()
{
	return viceCurrentC64PC;
}

int CDebugInterfaceVice::GetDrive1541PC()
{
	return viceCurrentDiskPC[0];
}

extern "C" {
// from c64cpu.c
	void c64d_get_maincpu_regs(uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp, uint16 *pc,
							   uint8 *instructionCycle);
	void c64d_get_drivecpu_regs(int driveNum, uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp, uint16 *pc);
}

void CDebugInterfaceVice::GetC64CpuState(C64StateCPU *state)
{
	c64d_get_maincpu_regs(&(state->a), &(state->x), &(state->y), &(state->processorFlags), &(state->sp), &(state->pc),
						  &(state->instructionCycle));
	
	state->lastValidPC = state->pc;
}

void CDebugInterfaceVice::GetDrive1541CpuState(C64StateCPU *state)
{
	c64d_get_drivecpu_regs(0, &(state->a), &(state->x), &(state->y), &(state->processorFlags), &(state->sp), &(state->pc));
	
	state->lastValidPC = viceCurrentDiskPC[0];
	
	//LOGD("drive pc: %04x", state->pc);
}

extern "C" {
	void c64d_get_vic_simple_state(struct C64StateVIC *simpleStateVic);
}

void CDebugInterfaceVice::GetVICState(C64StateVIC *state)
{
	c64d_get_vic_simple_state(state);
}

void CDebugInterfaceVice::GetDrive1541State(C64StateDrive1541 *state)
{
	drive_t *drive = drive_context[0]->drive;
	state->headTrackPosition = drive->current_half_track + drive->side * 70;

}

void CDebugInterfaceVice::InsertD64(CSlrString *path)
{
	int diskId = 0;
	
	LockIoMutex();
	char *asciiPath = path->GetStdASCII();
	
	SYS_FixFileNameSlashes(asciiPath);

	int rc = file_system_attach_disk(8, asciiPath);
	
	if (rc == -1)
	{
		viewC64->ShowMessageError("Inserting disk failed");
	}
	
	delete [] asciiPath;

	// TODO: add drive ID
	((CDataAdapterViceDrive1541DiskContents*)dataAdapterDrive1541DiskContents)->DiskAttached();
	
	UnlockIoMutex();
}

void CDebugInterfaceVice::DetachDriveDisk()
{
	file_system_detach_disk(8);
	((CDataAdapterViceDrive1541DiskContents*)debugInterfaceVice->dataAdapterDrive1541DiskContents)->DiskDetached();
}

// REU
extern "C" {
	int set_reu_enabled(int value, void *param);
	int set_reu_size(int val, void *param);
	int set_reu_filename(const char *name, void *param);
	int reu_bin_save(const char *filename);
};

void CDebugInterfaceVice::SetReuEnabled(bool isEnabled)
{
	LOGD("CDebugInterfaceVice::SetReuEnabled: %s", STRBOOL(isEnabled));
	set_reu_enabled((isEnabled ? 1:0), NULL);
}

void CDebugInterfaceVice::SetReuSize(int reuSize)
{
	snapshotsManager->LockMutex();
	set_reu_size(reuSize, NULL);
	snapshotsManager->UnlockMutex();
}

bool CDebugInterfaceVice::LoadReu(char *filePath)
{
	resources_set_string("REUfilename", filePath);

//	if (set_reu_filename(filePath, NULL) == 0)
//		return true;
//	return false;
	
	return true;
}

bool CDebugInterfaceVice::SaveReu(char *filePath)
{
	snapshotsManager->LockMutex();
	if (reu_bin_save(filePath) == 0)
	{
		snapshotsManager->UnlockMutex();
		return true;
	}
	
	snapshotsManager->UnlockMutex();
	return false;
}


extern "C" {
	int c64d_get_warp_mode();
	int set_warp_mode(int val, void *param);
}

bool CDebugInterfaceVice::GetSettingIsWarpSpeed()
{
	return c64d_get_warp_mode() == 0 ? false : true;
}

void CDebugInterfaceVice::SetSettingIsWarpSpeed(bool isWarpSpeed)
{
	set_warp_mode(isWarpSpeed ? 1 : 0, NULL);
}

///

void CDebugInterfaceVice::GetSidTypes(std::vector<CSlrString *> *sidTypes)
{
	// 0-2
	sidTypes->push_back(new CSlrString("6581 (ReSID)"));
	sidTypes->push_back(new CSlrString("8580 (ReSID)"));
	sidTypes->push_back(new CSlrString("8580 + digi boost (ReSID)"));

	// 3-4
	sidTypes->push_back(new CSlrString("6581 (FastSID)"));
	sidTypes->push_back(new CSlrString("8580 (FastSID)"));

	// 5-14
	sidTypes->push_back(new CSlrString("6581R3 4885 (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("6581R3 0486S (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("6581R3 3984 (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("6581R4AR 3789 (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("6581R3 4485 (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("6581R4 1986S (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("8580R5 3691 (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("8580R5 3691 + digi (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("8580R5 1489 (ReSID-fp)"));
	sidTypes->push_back(new CSlrString("8580R5 1489 + digi (ReSID-fp)"));
}


void CDebugInterfaceVice::GetSidTypes(std::vector<const char *> *sidTypes)
{
	// 0-2
	sidTypes->push_back("6581 (ReSID)");
	sidTypes->push_back("8580 (ReSID)");
	sidTypes->push_back("8580 + digi boost (ReSID)");
	
	// 3-4
	sidTypes->push_back("6581 (FastSID)");
	sidTypes->push_back("8580 (FastSID)");
	
	// 5-14
	sidTypes->push_back("6581R3 4885 (ReSID-fp)");
	sidTypes->push_back("6581R3 0486S (ReSID-fp)");
	sidTypes->push_back("6581R3 3984 (ReSID-fp)");
	sidTypes->push_back("6581R4AR 3789 (ReSID-fp)");
	sidTypes->push_back("6581R3 4485 (ReSID-fp)");
	sidTypes->push_back("6581R4 1986S (ReSID-fp)");
	sidTypes->push_back("8580R5 3691 (ReSID-fp)");
	sidTypes->push_back("8580R5 3691 + digi (ReSID-fp)");
	sidTypes->push_back("8580R5 1489 (ReSID-fp)");
	sidTypes->push_back("8580R5 1489 + digi (ReSID-fp)");
	
#if defined(WIN32)
	// 15 hardsid
	sidTypes->push_back("HardSID");
#endif
	
}

int c64_change_sid_type_value_to_set = 0;
static void c64_change_sid_type_trap(WORD addr, void *v)
{
	gSoundEngine->LockMutex("c64_change_sid_type_trap");
	
	LOGD("c64_change_sid_type_trap: sidType=%d", c64_change_sid_type_value_to_set);
	debugInterfaceVice->SetSidTypeAsync(c64_change_sid_type_value_to_set);
	
	gSoundEngine->UnlockMutex("c64_change_sid_type_trap");
}

void CDebugInterfaceVice::SetSidType(int sidType)
{
	LOGM("CDebugInterfaceVice::SetSidType: %d", sidType);
	
	c64_change_sid_type_value_to_set = sidType;
	interrupt_maincpu_trigger_trap(c64_change_sid_type_trap, NULL);
}

void CDebugInterfaceVice::SetSidTypeAsync(int sidType)
{
	LOGD("CDebugInterfaceVice::SetSidTypeAsync: sidType=%d", sidType);
	
	snapshotsManager->LockMutex();
	
	switch(sidType)
	{
		default:
		case 0:
			sid_set_engine_model(SID_ENGINE_RESID, SID_MODEL_6581);
			break;
		case 1:
			sid_set_engine_model(SID_ENGINE_RESID, SID_MODEL_8580);
			break;
		case 2:
			sid_set_engine_model(SID_ENGINE_RESID, SID_MODEL_8580D);
			break;
		case 3:
			sid_set_engine_model(SID_ENGINE_FASTSID, SID_MODEL_6581);
			break;
		case 4:
			sid_set_engine_model(SID_ENGINE_FASTSID, SID_MODEL_8580);
			break;
		case 5:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_6581R3_4885);
			break;
		case 6:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_6581R3_0486S);
			break;
		case 7:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_6581R3_3984);
			break;
		case 8:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_6581R4AR_3789);
			break;
		case 9:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_6581R3_4485);
			break;
		case 10:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_6581R4_1986S);
			break;
		case 11:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_8580R5_3691);
			break;
		case 12:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_8580R5_3691D);
			break;
		case 13:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_8580R5_1489);
			break;
		case 14:
			sid_set_engine_model(SID_ENGINE_RESID_FP, SID_MODEL_8580R5_1489D);
			break;
		case 15:
			sid_set_engine_model(SID_ENGINE_HARDSID, SID_MODEL_DEFAULT);
			break;
			
	}
	snapshotsManager->UnlockMutex();
}

//
void CDebugInterfaceVice::SetViciiBorderMode(int borderMode)
{
	LockMutex();
	c64d_set_vicii_border_mode(borderMode);
	UnlockMutex();
}

int CDebugInterfaceVice::GetViciiBorderMode()
{
	return c64d_get_vicii_border_mode();
}

extern "C" {
void c64d_vicii_get_geometry(int *canvasWidth, int *canvasHeight, int *gfxPosX, int *gfxPosY);
}

void CDebugInterfaceVice::GetViciiGeometry(int *canvasWidth, int *canvasHeight, int *gfxPosX, int *gfxPosY)
{
	c64d_vicii_get_geometry(canvasWidth, canvasHeight, gfxPosX, gfxPosY);
}


// samplingMethod: Fast=0, Interpolating=1, Resampling=2, Fast Resampling=3
void CDebugInterfaceVice::SetSidSamplingMethod(int samplingMethod)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_sampling_method(samplingMethod);
	snapshotsManager->UnlockMutex();
}

// emulateFilters: no=0, yes=1
void CDebugInterfaceVice::SetSidEmulateFilters(int emulateFilters)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_emulate_filters(emulateFilters);
	snapshotsManager->UnlockMutex();
}

// passband: 0-90
void CDebugInterfaceVice::SetSidPassBand(int passband)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_passband(passband);
	snapshotsManager->UnlockMutex();
}

// filterBias: -500 500
void CDebugInterfaceVice::SetSidFilterBias(int filterBias)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_filter_bias(filterBias);
	snapshotsManager->UnlockMutex();
}

int CDebugInterfaceVice::GetNumSids()
{
	return this->numSids;
}

// 0=none, 1=stereo, 2=triple
void CDebugInterfaceVice::SetSidStereo(int stereoMode)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_stereo(stereoMode);
	
	// stereo: 0=none, 1=stereo, 2=triple
	this->numSids = stereoMode + 1;

	snapshotsManager->UnlockMutex();
}

void CDebugInterfaceVice::SetSidStereoAddress(uint16 sidAddress)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_stereo_address(sidAddress);
	snapshotsManager->UnlockMutex();
}

void CDebugInterfaceVice::SetSidTripleAddress(uint16 sidAddress)
{
	snapshotsManager->LockMutex();
	c64d_sid_set_triple_address(sidAddress);
	snapshotsManager->UnlockMutex();
}



///// c64model.c
//#define C64MODEL_C64_PAL 0
//#define C64MODEL_C64C_PAL 1
//#define C64MODEL_C64_OLD_PAL 2
//
//#define C64MODEL_C64_NTSC 3
//#define C64MODEL_C64C_NTSC 4
//#define C64MODEL_C64_OLD_NTSC 5
//
//#define C64MODEL_C64_PAL_N 6
//
///* SX-64 */
//#define C64MODEL_C64SX_PAL 7
//#define C64MODEL_C64SX_NTSC 8
//
//#define C64MODEL_C64_JAP 9
//#define C64MODEL_C64_GS 10
//
//#define C64MODEL_PET64_PAL 11
//#define C64MODEL_PET64_NTSC 12
///* max machine */
//#define C64MODEL_ULTIMAX 13

void CDebugInterfaceVice::GetC64ModelTypes(std::vector<CSlrString *> *modelTypeNames, std::vector<int> *modelTypeIds)
{
	modelTypeNames->push_back(new CSlrString("C64 PAL"));
	modelTypeIds->push_back(0);
	modelTypeNames->push_back(new CSlrString("C64C PAL"));
	modelTypeIds->push_back(1);
	modelTypeNames->push_back(new CSlrString("C64 old PAL"));
	modelTypeIds->push_back(2);
	modelTypeNames->push_back(new CSlrString("C64 NTSC"));
	modelTypeIds->push_back(3);
	modelTypeNames->push_back(new CSlrString("C64C NTSC"));
	modelTypeIds->push_back(4);
	modelTypeNames->push_back(new CSlrString("C64 old NTSC"));
	modelTypeIds->push_back(5);
	// crashes: modelTypeNames->push_back(new CSlrString("C64 PAL N (Drean)"));
	// crashes: modelTypeIds->push_back(6);
	modelTypeNames->push_back(new CSlrString("C64 SX PAL"));
	modelTypeIds->push_back(7);
	modelTypeNames->push_back(new CSlrString("C64 SX NTSC"));
	modelTypeIds->push_back(8);
	// no ROM: modelTypeNames->push_back(new CSlrString("Japanese"));
	// no ROM: modelTypeIds->push_back(9);
	// no ROM: modelTypeNames->push_back(new CSlrString("C64 GS"));
	// no ROM: modelTypeIds->push_back(10);
	modelTypeNames->push_back(new CSlrString("PET64 PAL"));
	modelTypeIds->push_back(11);
	modelTypeNames->push_back(new CSlrString("PET64 NTSC"));
	modelTypeIds->push_back(12);
	// no ROM: modelTypeNames->push_back(new CSlrString("MAX Machine"));
	// no ROM: modelTypeIds->push_back(13);
}

void CDebugInterfaceVice::GetC64ModelTypes(std::vector<const char *> *modelTypeNames, std::vector<int> *modelTypeIds)
{
	modelTypeNames->push_back("C64 PAL");
	modelTypeIds->push_back(0);
	modelTypeNames->push_back("C64C PAL");
	modelTypeIds->push_back(1);
	modelTypeNames->push_back("C64 old PAL");
	modelTypeIds->push_back(2);
	modelTypeNames->push_back("C64 NTSC");
	modelTypeIds->push_back(3);
	modelTypeNames->push_back("C64C NTSC");
	modelTypeIds->push_back(4);
	modelTypeNames->push_back("C64 old NTSC");
	modelTypeIds->push_back(5);
	// crashes: modelTypeNames->push_back("C64 PAL N (Drean)");
	// crashes: modelTypeIds->push_back(6);
	modelTypeNames->push_back("C64 SX PAL");
	modelTypeIds->push_back(7);
	modelTypeNames->push_back("C64 SX NTSC");
	modelTypeIds->push_back(8);
	// no ROM: modelTypeNames->push_back("Japanese");
	// no ROM: modelTypeIds->push_back(9);
	// no ROM: modelTypeNames->push_back("C64 GS");
	// no ROM: modelTypeIds->push_back(10);
	modelTypeNames->push_back("PET64 PAL");
	modelTypeIds->push_back(11);
	modelTypeNames->push_back("PET64 NTSC");
	modelTypeIds->push_back(12);
	// no ROM: modelTypeNames->push_back("MAX Machine");
	// no ROM: modelTypeIds->push_back(13);
}
int c64_change_model_type;

static void c64_change_model_trap(WORD addr, void *v)
{
	guiMain->LockMutex();
	debugInterfaceVice->LockRenderScreenMutex();
	
	LOGD("c64_change_model_trap: model=%d", c64_change_model_type);
	
	//c64_change_model_type = 0;
	
	c64model_set(c64_change_model_type);
	
	debugInterfaceVice->modelType = c64_change_model_type;
	
	c64d_update_c64_machine_from_model_type(c64_change_model_type);
	c64d_update_c64_screen_height_from_model_type(c64_change_model_type);
	
	c64d_clear_screen();
	
	SYS_Sleep(100);
	
	c64d_clear_screen();
	
	debugInterfaceVice->UnlockRenderScreenMutex();
	
	debugInterfaceVice->ResetEmulationFrameCounter();

	guiMain->UnlockMutex();
}

void CDebugInterfaceVice::SetC64ModelType(int modelType)
{
	LOGM("CDebugInterfaceVice::SetC64ModelType: %d", modelType);
	
	// blank screen when machine type is changed
	c64d_clear_screen();
	
	c64d_update_c64_machine_from_model_type(c64_change_model_type);

	c64_change_model_type = modelType;
	interrupt_maincpu_trigger_trap(c64_change_model_trap, NULL);
}

uint8 CDebugInterfaceVice::GetC64MachineType()
{
	return machineType;
}

int CDebugInterfaceVice::GetC64ModelType()
{
	snapshotsManager->LockMutex();
	int model = c64model_get();
	snapshotsManager->UnlockMutex();
	return model;
}

void CDebugInterfaceVice::SetEmulationMaximumSpeed(int maximumSpeed)
{
	resources_set_int("Speed", maximumSpeed);
	
	if (maximumSpeed < 20)
	{
		resources_set_int("Sound", 0);
	}
	else
	{
		resources_set_int("Sound", 1);
	}
}

extern "C" {
	int set_vsp_bug_enabled(int val, void *param);
}

void CDebugInterfaceVice::SetVSPBugEmulation(bool isVSPBugEmulation)
{
	if (isVSPBugEmulation)
	{
		set_vsp_bug_enabled(1, NULL);
	}
	else
	{
		set_vsp_bug_enabled(0, NULL);
	}
}


///
///

extern "C" {
	void c64d_mem_write_c64(unsigned int addr, unsigned char value);
	void c64d_mem_write_c64_no_mark(unsigned int addr, unsigned char value);
	void c64d_mem_ram_write_c64(WORD addr, BYTE value);
	void c64d_mem_ram_fill_c64(WORD addr, WORD size, BYTE value);
}

void CDebugInterfaceVice::SetByteC64(uint16 addr, uint8 val)
{
	c64d_mem_write_c64(addr, val);
}

void CDebugInterfaceVice::SetByteToRamC64(uint16 addr, uint8 val)
{
	c64d_mem_ram_write_c64(addr, val);
}

///////

extern "C" {
	//BYTE mem_bank_peek(int bank, WORD addr, void *context); // can't be used, because reading affects/changes state
	BYTE c64d_peek_c64(WORD addr);
	BYTE c64d_mem_ram_read_c64(WORD addr);
	void c64d_peek_memory_c64(BYTE *buffer, int addrStart, int addrEnd);
	void c64d_copy_ram_memory_c64(BYTE *buffer, int addrStart, int addrEnd);
	void c64d_copy_whole_mem_ram_c64(BYTE *destBuf);
	void c64d_peek_whole_map_c64(BYTE *memoryBuffer);
}

/***********/
uint8 CDebugInterfaceVice::GetByteC64(uint16 addr)
{
	return c64d_peek_c64(addr);
}


uint8 CDebugInterfaceVice::GetByteFromRamC64(uint16 addr)
{
	return c64d_mem_ram_read_c64(addr);
}


///

extern "C" {
	void mon_jump(MON_ADDR addr);
	void c64d_set_c64_pc(uint16 pc);
	void c64d_mem_ram_write_drive(int driveNum, uint16 addr, uint8 value);
	void c64d_drive_poke(int driveNum, uint16 addr, uint8 value);
	void c64d_set_drive_pc(int driveNr, uint16 pc);

}

void CDebugInterfaceVice::SetVicRegister(uint8 registerNum, uint8 value)
{
	vicii_store(registerNum, value);
	
	if (registerNum >= 0x20 && registerNum <= 0x2E)
	{
		c64d_set_color_register(registerNum, value);
	}
}

u8 CDebugInterfaceVice::GetVicRegister(uint8 registerNum)
{
	BYTE v = vicii_peek(registerNum);
	return v;
}

//
extern "C" {
	cia_context_t *c64d_get_cia_context(int ciaId);
	BYTE c64d_ciacore_peek(cia_context_t *cia_context, WORD addr);
	void ciacore_store(cia_context_t *cia_context, WORD addr, BYTE value);
}

void CDebugInterfaceVice::SetCiaRegister(uint8 ciaId, uint8 registerNum, uint8 value)
{
	cia_context_t *cia_context = c64d_get_cia_context(ciaId);
	ciacore_store(cia_context, registerNum, value);
}

u8 CDebugInterfaceVice::GetCiaRegister(uint8 ciaId, uint8 registerNum)
{
	cia_context_t *cia_context = c64d_get_cia_context(ciaId);	
	return c64d_ciacore_peek(cia_context, registerNum);
}

extern "C" {
	BYTE sid_peek_chip(WORD addr, int chipno);
	void sid_store_chip(WORD addr, BYTE value, int chipno);
}

struct SetSidRegisterData {
	WORD registerNum;
	BYTE value;
	int sidId;
};

static void c64_set_sid_register_trap(WORD addr, void *v)
{
	guiMain->LockMutex();
	debugInterfaceVice->LockMutex();
	gSoundEngine->LockMutex("CDebugInterfaceVice::c64_set_sid_register_trap");
	
	SetSidRegisterData *setSidRegisterData = (SetSidRegisterData *)v;
	
	sid_store_chip(setSidRegisterData->registerNum, setSidRegisterData->value, setSidRegisterData->sidId);
	delete setSidRegisterData;
	
	gSoundEngine->UnlockMutex("CDebugInterfaceVice::c64_set_sid_register_trap");
	debugInterfaceVice->UnlockMutex();
	guiMain->UnlockMutex();
}

void CDebugInterfaceVice::SetSidRegister(uint8 sidId, uint8 registerNum, uint8 value)
{
	snapshotsManager->LockMutex();

	this->LockMutex();
	
	if (this->GetDebugMode() == DEBUGGER_MODE_PAUSED)
	{ 
		this->LockIoMutex();
		sid_store_chip(registerNum, value, sidId);
		this->UnlockIoMutex();
	}
	else
	{
		SetSidRegisterData *setSidRegisterData = new SetSidRegisterData();
		setSidRegisterData->sidId = sidId;
		setSidRegisterData->registerNum = registerNum;
		setSidRegisterData->value = value;
		interrupt_maincpu_trigger_trap(c64_set_sid_register_trap, setSidRegisterData);
	}

	this->UnlockMutex();
	snapshotsManager->UnlockMutex();
}

void c64_set_sid_data(void *v)
{
	guiMain->LockMutex();
	debugInterfaceVice->LockMutex();
	gSoundEngine->LockMutex("CDebugInterfaceVice::c64_set_sid_data");
	
	CSidData *sidData = (CSidData *)v;
	sidData->RestoreSids();
	
	gSoundEngine->UnlockMutex("CDebugInterfaceVice::c64_set_sid_data");
	debugInterfaceVice->UnlockMutex();
	guiMain->UnlockMutex();
}


void CDebugInterfaceVice::SetSid(CSidData *sidData)
{
	this->LockMutex();
	snapshotsManager->LockMutex();

	this->sidDataToRestore = sidData;

	snapshotsManager->UnlockMutex();
	this->UnlockMutex();
}

u8 CDebugInterfaceVice::GetSidRegister(uint8 sidId, uint8 registerNum)
{
	return sid_peek_chip(registerNum, sidId);
}

extern "C" {
	void via1d1541_store(drive_context_t *ctxptr, WORD addr, BYTE data);
	BYTE c64d_via1d1541_peek(drive_context_t *ctxptr, WORD addr);
	void via2d_store(drive_context_t *ctxptr, WORD addr, BYTE data);
	BYTE c64d_via2d_peek(drive_context_t *ctxptr, WORD addr);
}
void CDebugInterfaceVice::SetViaRegister(uint8 driveId, uint8 viaId, uint8 registerNum, uint8 value)
{
	drive_context_t *drivectx = drive_context[driveId];
	
	if (viaId == 1)
	{
		via1d1541_store(drivectx, registerNum, value);
	}
	else
	{
		via2d_store(drivectx, registerNum, value);
	}

}

u8 CDebugInterfaceVice::GetViaRegister(uint8 driveId, uint8 viaId, uint8 registerNum)
{
	drive_context_t *drivectx = drive_context[driveId];
	
	if (viaId == 1)
	{
		return c64d_via1d1541_peek(drivectx, registerNum);
	}
	
	return c64d_via2d_peek(drivectx, registerNum);
}



void CDebugInterfaceVice::MakeJmpC64(uint16 addr)
{
	LOGD("CDebugInterfaceVice::MakeJmpC64: %04x", addr);
	
	if (c64d_debug_mode == DEBUGGER_MODE_PAUSED)
	{
		c64d_set_c64_pc(addr);
		c64d_set_debug_mode(DEBUGGER_MODE_PAUSED);
	}
	else
	{
		c64d_set_c64_pc(addr);
	}
}

void CDebugInterfaceVice::MakeJmpNoResetC64(uint16 addr)
{
	LOGTODO("CDebugInterfaceVice::MakeJmpNoResetC64");
	// TODO:
	this->MakeJmpC64(addr);
}

void CDebugInterfaceVice::MakeJsrC64(uint16 addr)
{
	LOGTODO("CDebugInterfaceVice::MakeJsrC64");
	// TODO:
	this->MakeJmpC64(addr);
}

extern "C" {
	void c64d_maincpu_make_basic_run();
};

void CDebugInterfaceVice::MakeJMPToBasicRunC64()
{
	LOGD("CDebugInterfaceVice::MakeJMPToBasicRunC64");
	
	c64d_maincpu_make_basic_run();
}


extern "C" {
	void c64d_set_maincpu_regs(uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp);
	void c64d_set_maincpu_set_sp(uint8 *sp);
	void c64d_set_maincpu_set_a(uint8 *a);
	void c64d_set_maincpu_set_x(uint8 *x);
	void c64d_set_maincpu_set_y(uint8 *y);
	void c64d_set_maincpu_set_p(uint8 *p);

}

void CDebugInterfaceVice::SetStackPointerC64(uint8 val)
{
	LOGD("CDebugInterfaceVice::SetStackPointerC64: val=%x", val);
	
	this->LockMutex();
	
	uint8 sp = val;
	c64d_set_maincpu_set_sp(&sp);

	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterAC64(uint8 val)
{
	LOGD("CDebugInterfaceVice::SetRegisterAC64: val=%x", val);
	
	this->LockMutex();
	
	uint8 a = val;
	c64d_set_maincpu_set_a(&a);

	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterXC64(uint8 val)
{
	LOGD("CDebugInterfaceVice::SetRegisterXC64: val=%x", val);
	
	this->LockMutex();
	
	uint8 x = val;
	c64d_set_maincpu_set_x(&x);
	
	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterYC64(uint8 val)
{
	LOGD("CDebugInterfaceVice::SetRegisterYC64: val=%x", val);
	
	this->LockMutex();
	
	uint8 y = val;
	c64d_set_maincpu_set_y(&y);
	
	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterPC64(uint8 val)
{
	LOGD("CDebugInterfaceVice::SetRegisterPC64: val=%x", val);
	
	this->LockMutex();
	
	uint8 p = val;
	c64d_set_maincpu_set_p(&p);
	
	this->UnlockMutex();
}

extern "C" {
	void c64d_set_drive_register_a(int driveNr, uint8 a);
	void c64d_set_drive_register_x(int driveNr, uint8 x);
	void c64d_set_drive_register_y(int driveNr, uint8 y);
	void c64d_set_drive_register_p(int driveNr, uint8 p);
	void c64d_set_drive_register_sp(int driveNr, uint8 sp);
}

void CDebugInterfaceVice::SetRegisterA1541(uint8 val)
{
	this->LockMutex();
	
	c64d_set_drive_register_a(0, val);
	
	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterX1541(uint8 val)
{
	this->LockMutex();
	
	c64d_set_drive_register_x(0, val);
	
	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterY1541(uint8 val)
{
	this->LockMutex();
	
	c64d_set_drive_register_y(0, val);
	
	this->UnlockMutex();
}

void CDebugInterfaceVice::SetRegisterP1541(uint8 val)
{
	this->LockMutex();
	
	c64d_set_drive_register_p(0, val);
	
	this->UnlockMutex();
}

void CDebugInterfaceVice::SetStackPointer1541(uint8 val)
{
	this->LockMutex();
	
	c64d_set_drive_register_sp(0, val);
	
	this->UnlockMutex();
}


void CDebugInterfaceVice::SetByte1541(uint16 addr, uint8 val)
{
	c64d_drive_poke(0, addr, val);
}

void CDebugInterfaceVice::SetByteToRam1541(uint16 addr, uint8 val)
{
	c64d_mem_ram_write_drive(0, addr, val);
}

extern "C" {
	uint8 c64d_peek_drive(int driveNum, uint16 addr);
	uint8 c64d_mem_ram_read_drive(int driveNum, uint16 addr);
	void c64d_peek_memory_drive(int driveNum, BYTE *buffer, int addrStart, int addrEnd);
	void c64d_copy_ram_memory_drive(int driveNum, BYTE *buffer, int addrStart, int addrEnd);
	void c64d_peek_whole_map_drive(int driveNum, uint8 *memoryBuffer);
	void c64d_copy_mem_ram_drive(int driveNum, uint8 *memoryBuffer);
}

uint8 CDebugInterfaceVice::GetByte1541(uint16 addr)
{
	return c64d_peek_drive(0, addr); 
}

uint8 CDebugInterfaceVice::GetByteFromRam1541(uint16 addr)
{
	return c64d_mem_ram_read_drive(0, addr);
}

void CDebugInterfaceVice::MakeJmp1541(uint16 addr)
{
	if (c64d_debug_mode == DEBUGGER_MODE_PAUSED)
	{
		c64d_set_drive_pc(0, addr);
		//c64d_set_debug_mode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
	}
	else
	{
		c64d_set_drive_pc(0, addr);
	}
}

void CDebugInterfaceVice::MakeJmpNoReset1541(uint16 addr)
{
	this->MakeJmp1541(addr);
}


void CDebugInterfaceVice::GetWholeMemoryMap(uint8 *buffer)
{
	c64d_peek_whole_map_c64(buffer);
}

void CDebugInterfaceVice::GetWholeMemoryMapFromRam(uint8 *buffer)
{
	c64d_copy_whole_mem_ram_c64(buffer);
}

void CDebugInterfaceVice::GetWholeMemoryMap1541(uint8 *buffer)
{
	c64d_peek_whole_map_drive(0, buffer);
}

void CDebugInterfaceVice::GetWholeMemoryMapFromRam1541(uint8 *buffer)
{
	c64d_copy_mem_ram_drive(0, buffer);
}


void CDebugInterfaceVice::GetMemoryC64(uint8 *buffer, int addrStart, int addrEnd)
{
	c64d_peek_memory_c64(buffer, addrStart, addrEnd);
}

void CDebugInterfaceVice::GetMemoryFromRam(uint8 *buffer, int addrStart, int addrEnd)
{
	c64d_copy_ram_memory_c64(buffer, addrStart, addrEnd);
}

void CDebugInterfaceVice::GetMemoryFromRamC64(uint8 *buffer, int addrStart, int addrEnd)
{
	c64d_copy_ram_memory_c64(buffer, addrStart, addrEnd);
}

void CDebugInterfaceVice::GetMemoryDrive1541(uint8 *buffer, int addrStart, int addrEnd)
{
	c64d_peek_memory_drive(0, buffer, addrStart, addrEnd);
}

void CDebugInterfaceVice::GetMemoryFromRamDrive1541(uint8 *buffer, int addrStart, int addrEnd)
{
	c64d_copy_ram_memory_drive(0, buffer, addrStart, addrEnd);
}

void CDebugInterfaceVice::FillC64Ram(uint16 addr, uint16 size, uint8 value)
{
	c64d_mem_ram_fill_c64(addr, size, value);
}


///

extern "C" {
	void c64d_get_vic_colors(uint8 *cD021, uint8 *cD022, uint8 *cD023, uint8 *cD025, uint8 *cD026, uint8 *cD027, uint8 *cD800);
}

void CDebugInterfaceVice::GetVICColors(uint8 *cD021, uint8 *cD022, uint8 *cD023, uint8 *cD025, uint8 *cD026, uint8 *cD027, uint8 *cD800)
{
	c64d_get_vic_colors(cD021, cD022, cD023, cD025, cD026, cD027, cD800);
}


void CDebugInterfaceVice::GetVICSpriteColors(uint8 *cD021, uint8 *cD025, uint8 *cD026, uint8 *spriteColors)
{
	LOGTODO("CDebugInterfaceVice::GetVICSpriteColors: not implemented");
}

void CDebugInterfaceVice::GetCBMColor(uint8 colorNum, uint8 *r, uint8 *g, uint8 *b)
{
	*r = c64d_palette_red[colorNum & 0x0F];
	*g = c64d_palette_green[colorNum & 0x0F];
	*b = c64d_palette_blue[colorNum & 0x0F];
}

void CDebugInterfaceVice::GetFloatCBMColor(uint8 colorNum, float *r, float *g, float *b)
{
	*r = c64d_float_palette_red[colorNum & 0x0F];
	*g = c64d_float_palette_green[colorNum & 0x0F];
	*b = c64d_float_palette_blue[colorNum & 0x0F];
}



void CDebugInterfaceVice::SetDebugMode(uint8 debugMode)
{
	LOGD("CDebugInterfaceVice::SetDebugMode: %d (cycle=%d)", debugMode, GetMainCpuCycleCounter());
	
	c64d_set_debug_mode(debugMode);
	
}

uint8 CDebugInterfaceVice::GetDebugMode()
{
	return c64d_debug_mode;
}

// tape
extern "C" {
	int tape_image_attach(unsigned int unit, const char *name);
	int tape_image_detach(unsigned int unit);
	void datasette_control(int command);
}

static void tape_attach_trap(WORD addr, void *v)
{
	char *filePath = (char*)v;
	tape_image_attach(1, filePath);

	SYS_ReleaseCharBuf(filePath);
}

static void tape_detach_trap(WORD addr, void *v)
{
	tape_image_detach(1);
}

void CDebugInterfaceVice::AttachTape(CSlrString *filePath)
{
	char *asciiPath = filePath->GetStdASCII();
	
	SYS_FixFileNameSlashes(asciiPath);
	
	char *buf = SYS_GetCharBuf();
	strcpy(buf, asciiPath);

	interrupt_maincpu_trigger_trap(tape_attach_trap, asciiPath);
}

void CDebugInterfaceVice::DetachTape()
{
	interrupt_maincpu_trigger_trap(tape_detach_trap, NULL);
}

void CDebugInterfaceVice::DatasettePlay()
{
	datasette_control(DATASETTE_CONTROL_START);
}

void CDebugInterfaceVice::DatasetteStop()
{
	datasette_control(DATASETTE_CONTROL_STOP);
}

void CDebugInterfaceVice::DatasetteForward()
{
	datasette_control(DATASETTE_CONTROL_FORWARD);
}

void CDebugInterfaceVice::DatasetteRewind()
{
	datasette_control(DATASETTE_CONTROL_REWIND);
}

void CDebugInterfaceVice::DatasetteRecord()
{
	datasette_control(DATASETTE_CONTROL_RECORD);
}

void CDebugInterfaceVice::DatasetteReset()
{
	datasette_control(DATASETTE_CONTROL_RESET);
}

void CDebugInterfaceVice::DatasetteSetSpeedTuning(int speedTuning)
{
	resources_set_int("DatasetteSpeedTuning", speedTuning);
}

void CDebugInterfaceVice::DatasetteSetZeroGapDelay(int zeroGapDelay)
{
	resources_set_int("DatasetteZeroGapDelay", zeroGapDelay);
}

void CDebugInterfaceVice::DatasetteSetResetWithCPU(bool resetWithCPU)
{
	resources_set_int("DatasetteResetWithCPU", resetWithCPU ? 1:0);
}

void CDebugInterfaceVice::DatasetteSetTapeWobble(int tapeWobble)
{
	resources_set_int("DatasetteTapeWobble", tapeWobble);
}


// http://www.lemon64.com/?mainurl=http%3A//www.lemon64.com/apps/list.php%3FGenre%3Dcarts

extern "C" {
	int cartridge_attach_image(int type, const char *filename);
	void cartridge_detach_image(int type);
	void cartridge_trigger_freeze(void);
}

static void cartridge_attach_trap(WORD addr, void *v)
{
	char *filePath = (char*)v;
	cartridge_attach_image(0, filePath);
	
	SYS_ReleaseCharBuf(filePath);

	debugInterfaceVice->ResetEmulationFrameCounter();
}

static void cartridge_detach_trap(WORD addr, void *v)
{
	// -1 means all slots
	cartridge_detach_image(-1);
	machine_trigger_reset(MACHINE_RESET_MODE_HARD);
	debugInterfaceVice->ResetEmulationFrameCounter();
	c64d_maincpu_clk = 6;
}

void CDebugInterfaceVice::AttachCartridge(CSlrString *filePath)
{
	char *asciiPath = filePath->GetStdASCII();
	
	SYS_FixFileNameSlashes(asciiPath);

//	this->SetDebugMode(C64_DEBUG_RUN_ONE_INSTRUCTION);
//	SYS_Sleep(5000);
	
//	gSoundEngine->LockMutex("CDebugInterfaceVice::CartridgeAttach");
//	debugInterfaceVice->LockMutex();
//	guiMain->LockMutex();

	
	cartridge_attach_image(0, asciiPath);

	
//	guiMain->UnlockMutex();
//	debugInterfaceVice->UnlockMutex();
//	gSoundEngine->UnlockMutex("CDebugInterfaceVice::CartridgeAttach");


//	char *buf = SYS_GetCharBuf();
//	strcpy(buf, filePath);
//	interrupt_maincpu_trigger_trap(cartridge_attach_trap, buf);
	
//	SYS_Sleep(1000);
//	this->SetDebugMode(C64_DEBUG_RUNNING);
	
	debugInterfaceVice->ResetEmulationFrameCounter();
}

void CDebugInterfaceVice::DetachCartridge()
{
	interrupt_maincpu_trigger_trap(cartridge_detach_trap, NULL);
}

void CDebugInterfaceVice::CartridgeFreezeButtonPressed()
{
	keyboard_clear_keymatrix();
	cartridge_trigger_freeze();
}

extern "C" {
	void c64d_get_exrom_game(BYTE *exrom, BYTE *game);
}

void CDebugInterfaceVice::GetC64CartridgeState(C64StateCartridge *cartridgeState)
{
	c64d_get_exrom_game(&(cartridgeState->exrom), &(cartridgeState)->game);
}

static void trap_detach_everything(WORD addr, void *v)
{
	// -1 means all slots
	cartridge_detach_image(-1);
	machine_trigger_reset(MACHINE_RESET_MODE_HARD);
	debugInterfaceVice->ResetEmulationFrameCounter();
	c64d_maincpu_clk = 6;

	tape_image_detach(1);
	
	file_system_detach_disk(8);
	
	((CDataAdapterViceDrive1541DiskContents*)debugInterfaceVice->dataAdapterDrive1541DiskContents)->DiskDetached();
}


void CDebugInterfaceVice::DetachEverything()
{
	interrupt_maincpu_trigger_trap(trap_detach_everything, NULL);
}


extern "C" {
	void c64d_c64_set_vicii_record_state_mode(uint8 recordMode);
}

void CDebugInterfaceVice::SetVicRecordStateMode(uint8 recordMode)
{
	c64d_c64_set_vicii_record_state_mode(recordMode);
}


void CDebugInterfaceVice::SetSIDMuteChannels(int sidNumber, bool mute1, bool mute2, bool mute3, bool muteExt)
{
	uint8 sidVoiceMask = 0xF0;
	
	if (mute1 == false)
	{
		sidVoiceMask |= 0x01;
	}
	if (mute2 == false)
	{
		sidVoiceMask |= 0x02;
	}
	if (mute3 == false)
	{
		sidVoiceMask |= 0x04;
	}
	if (muteExt == false)
	{
		sidVoiceMask |= 0x08;
	}

	sid_set_voice_mask(sidNumber, sidVoiceMask);

}

void CDebugInterfaceVice::SetSIDReceiveChannelsData(int sidNumber, bool isReceiving)
{
	if (isReceiving)
	{
		c64d_sid_receive_channels_data(sidNumber, 1);
	}
	else
	{
		c64d_sid_receive_channels_data(sidNumber, 0);
	}
}

void c64d_update_c64_model()
{
	int modelType = c64model_get();
	c64d_update_c64_machine_from_model_type(modelType);
	c64d_update_c64_screen_height_from_model_type(modelType);

}

void c64d_update_c64_machine_from_model_type(int modelType)
{
	switch(c64_change_model_type)
	{
		default:
		case 0:
		case 1:
		case 2:
		case 6:
		case 7:
		case 11:
			// PAL, 312 lines
			debugInterfaceVice->machineType = MACHINE_TYPE_PAL;
			debugInterfaceVice->numEmulationFPS = 50;
			break;
		case 3:
		case 4:
		case 5:
		case 8:
		case 12:
			// NTSC, 275 lines
			debugInterfaceVice->machineType = MACHINE_TYPE_NTSC;
			debugInterfaceVice->numEmulationFPS = 60;
			break;
	}
}

void c64d_update_c64_screen_height_from_model_type(int modelType)
{
	switch(c64_change_model_type)
	{
		default:
		case 0:
		case 1:
		case 2:
		case 6:
		case 7:
		case 11:
			// PAL, 312 lines
//			debugInterfaceVice->screenHeight = 272;
			break;
		case 3:
		case 4:
		case 5:
		case 8:
		case 12:
			// NTSC, 275 lines
//			debugInterfaceVice->screenHeight = 259;
			break;
	}
}


static void load_snapshot_trap(WORD addr, void *v)
{
	LOGD("load_snapshot_trap");
	
	guiMain->LockMutex();
	debugInterfaceVice->LockMutex();
	
	char *filePath = (char*)v;
	//int ret =

	FILE *fp = fopen(filePath, "rb");
	if (!fp)
	{
		viewC64->ShowMessageError("Snapshot not found");
		debugInterfaceVice->UnlockMutex();
		guiMain->UnlockMutex();
		return;
	}
	fclose(fp);
	
	gSoundEngine->LockMutex("load_snapshot_trap");

	if (c64_snapshot_read(filePath, 0, 1, 1, 1, 1) < 0)
	{
		viewC64->ShowMessageError("Snapshot loading failed");
		
		debugInterfaceVice->machineType = MACHINE_TYPE_UNKNOWN;
		debugInterfaceVice->numEmulationFPS = 1;
		
		c64d_clear_screen();
	}
	else
	{
		// if CPU is in JAM then un-jam and continue
		if (c64d_is_cpu_in_jam_state == 1)
		{
			c64d_is_cpu_in_jam_state = 0;
			c64d_set_debug_mode(DEBUGGER_MODE_RUNNING);
		}
	}
	
	c64d_update_c64_model();
	
	debugInterfaceVice->SetSidTypeAsync(c64SettingsSIDEngineModel);
	debugInterfaceVice->SetSidSamplingMethod(c64SettingsRESIDSamplingMethod);
	debugInterfaceVice->SetSidEmulateFilters(c64SettingsRESIDEmulateFilters);
	debugInterfaceVice->SetSidPassBand(c64SettingsRESIDPassBand);
	debugInterfaceVice->SetSidFilterBias(c64SettingsRESIDFilterBias);
	
	int val;
	resources_get_int("SidStereo", &val);
	c64SettingsSIDStereo = val;

	resources_get_int("SidStereoAddressStart", &val);
	c64SettingsSIDStereoAddress = val;

	resources_get_int("SidTripleAddressStart", &val);
	c64SettingsSIDTripleAddress = val;

	gSoundEngine->UnlockMutex("load_snapshot_trap");
	
	SYS_ReleaseCharBuf(filePath);
	
	debugInterfaceVice->ClearHistory();
	((CDataAdapterViceDrive1541DiskContents*)debugInterfaceVice->dataAdapterDrive1541DiskContents)->DiskAttached();

	debugInterfaceVice->UnlockMutex();
	guiMain->UnlockMutex();
}


bool CDebugInterfaceVice::LoadFullSnapshot(char *filePath)
{
	char *buf = SYS_GetCharBuf();
	strcpy(buf, filePath);
	
	this->machineType = MACHINE_TYPE_LOADING_SNAPSHOT;
	debugInterfaceVice->numEmulationFPS = 1;

	interrupt_maincpu_trigger_trap(load_snapshot_trap, buf);
	
	if (c64d_debug_mode == DEBUGGER_MODE_PAUSED)
	{
		c64d_set_debug_mode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
	}
	
	return true;
}

static void save_snapshot_trap(WORD addr, void *v)
{
	LOGD("save_snapshot_trap");
	
	debugInterfaceVice->LockMutex();

	char *filePath = (char*)v;
	
	gSoundEngine->LockMutex("save_snapshot_trap");
	
	c64_snapshot_write(filePath, 0, 1, 0, 1, 1, 1);
	
	gSoundEngine->UnlockMutex("save_snapshot_trap");
	
	SYS_ReleaseCharBuf(filePath);
	
	debugInterfaceVice->UnlockMutex();	
}

void CDebugInterfaceVice::SaveFullSnapshot(char *filePath)
{
	//	if (c64d_debug_mode == C64_DEBUG_PAUSED)
	//	{
	//		// can we?
	//		c64_snapshot_write(filePath, 0, 1, 0);
	//	}
	//	else
	{
		char *buf = SYS_GetCharBuf();
		strcpy(buf, filePath);
		interrupt_maincpu_trigger_trap(save_snapshot_trap, buf);
	}
	
	if (c64d_debug_mode == DEBUGGER_MODE_PAUSED)
	{
		c64d_set_debug_mode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
	}
}

// snapshots
bool CDebugInterfaceVice::LoadFullSnapshot(CByteBuffer *byteBuffer)
{
	LOGD("LoadFullSnapshot");
	debugInterfaceVice->LockMutex();
	gSoundEngine->LockMutex("LoadFullSnapshot");

	int ret = c64_snapshot_read_from_memory(1, 1, 1, 1, 1, 1, byteBuffer->data, byteBuffer->length);
	if (ret != 0)
	{
		LOGError("CDebugInterfaceVice::LoadFullSnapshot: failed");

		gSoundEngine->UnlockMutex("LoadFullSnapshot");
		debugInterfaceVice->UnlockMutex();
		return false;
	}
	
	gSoundEngine->UnlockMutex("LoadFullSnapshot");
	debugInterfaceVice->UnlockMutex();
	return true;
}

void CDebugInterfaceVice::SaveFullSnapshot(CByteBuffer *snapshotBuffer)
{
	LOGTODO("CDebugInterfaceVice::LoadFullSnapshot: not implemented");
}


// these calls should be synced with CPU IRQ so snapshot store or restore is allowed
bool CDebugInterfaceVice::LoadChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
//	extern int c64_snapshot_read_from_memory(int event_mode, int read_roms, int read_disks, int read_reu_data,
//											 unsigned char *snapshot_data, int snapshot_size);

	LOGD("LoadChipsSnapshotSynced");
	debugInterfaceVice->LockMutex();
	gSoundEngine->LockMutex("LoadChipsSnapshotSynced");

	int ret = c64_snapshot_read_from_memory(1, 0, 0, 0, 0, 0, byteBuffer->data, byteBuffer->length);
	if (ret != 0)
	{
		LOGError("CDebugInterfaceVice::LoadFullSnapshotSynced: failed");

		gSoundEngine->UnlockMutex("LoadChipsSnapshotSynced");
		debugInterfaceVice->UnlockMutex();
		return false;
	}
	
	gSoundEngine->UnlockMutex("LoadChipsSnapshotSynced");
	debugInterfaceVice->UnlockMutex();
	return true;
}

bool CDebugInterfaceVice::SaveChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	// TODO: check if data changed and store snapshot with data accordingly
	return this->SaveFullSnapshotSynced(byteBuffer, true, false, false, false, false, false, true);
}

bool CDebugInterfaceVice::LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	//	extern int c64_snapshot_read_from_memory(int event_mode, int read_roms, int read_disks, int read_reu_data,
	//											 unsigned char *snapshot_data, int snapshot_size);
	
	LOGD("LoadDiskDataSnapshotSynced");
	debugInterfaceVice->LockMutex();
	gSoundEngine->LockMutex("LoadDiskDataSnapshotSynced");
	
//	int ret = c64_snapshot_read_from_memory(0, 0, 1, 0, 0, byteBuffer->data, byteBuffer->length);
	int ret = c64_snapshot_read_from_memory(0, 0, 1, 0, 0, 1, byteBuffer->data, byteBuffer->length);
	if (ret != 0)
	{
		LOGError("CDebugInterfaceVice::LoadFullSnapshotSynced: failed");
		
		gSoundEngine->UnlockMutex("LoadDiskDataSnapshotSynced");
		debugInterfaceVice->UnlockMutex();
		return false;
	}
	
	gSoundEngine->UnlockMutex("LoadDiskDataSnapshotSynced");
	debugInterfaceVice->UnlockMutex();
	
	debugInterfaceVice->dataAdapterViceDrive1541DiskContents->DiskAttached();
	
//	viewC64->viewDrive1541Browser->RefreshInsertedDiskImageAsync();
	return true;
}

bool CDebugInterfaceVice::SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	// TODO: check if data changed and store snapshot with data accordingly
	return this->SaveFullSnapshotSynced(byteBuffer,
										true, false, true, false, false, true, false);
}

bool CDebugInterfaceVice::SaveFullSnapshotSynced(CByteBuffer *byteBuffer,
												   bool saveChips, bool saveRoms, bool saveDisks, bool eventMode,
												   bool saveReuData, bool saveCartRoms, bool saveScreen)
{
//	LOGD("SaveFullSnapshotSynced: saveDisks=%d data=%x", saveDisks, byteBuffer->data);
	int snapshotSize = 0;
	u8 *snapshotData = NULL;

	debugInterfaceVice->LockMutex();
	gSoundEngine->LockMutex("SaveFullSnapshotSynced");

	// TODO: reuse byteBuffer->data
	int ret = c64_snapshot_write_in_memory(saveChips ? 1:0, saveRoms ? 1:0, saveDisks ? 1:0, eventMode ? 1:0,
										   saveReuData ? 1:0, saveCartRoms ? 1:0, saveScreen ? 1:0,
										   &snapshotSize, &snapshotData);

//	LOGD("SaveFullSnapshotSynced: saveDisks=%d snapshotData=%x", saveDisks, snapshotData);

	gSoundEngine->UnlockMutex("SaveFullSnapshotSynced");
	debugInterfaceVice->UnlockMutex();

//	LOGD("CDebugInterfaceVice::SaveFullSnapshotSynced: snapshotData=%x snapshotSize=%d", snapshotData, snapshotSize);
	
	if (ret == 0)
	{
		byteBuffer->SetData(snapshotData, snapshotSize);
		return true;
	}
	
	if (snapshotData != NULL)
	{
		lib_free(snapshotData);
	}
	
	LOGError("CDebugInterfaceVice::SaveFullSnapshotSynced: failed");
	return false;
}

// this sets up drive internals from disk image so it is ready for the c64 basic run
void CDebugInterfaceVice::PrepareDriveForBasicRun()
{
	viewC64->viewDrive1541Browser->UpdateDriveDiskID();

	u8 buf[256];
	dataAdapterViceDrive1541DiskContents->ReadSector(D64_BAM_TRACK-1, 0, buf);
	
	for (int i = 0; i < 256; i++)
	{
		dataAdapterDrive1541DirectRam->AdapterWriteByte(0x400+i, buf[i]);
		dataAdapterDrive1541DirectRam->AdapterWriteByte(0x700+i, buf[i]);
	}
	
}

bool CDebugInterfaceVice::IsDriveDirtyForSnapshot()
{
	return c64d_is_drive_dirty_for_snapshot() == 0 ? false : true;
}

void CDebugInterfaceVice::ClearDriveDirtyForSnapshotFlag()
{
	c64d_clear_drive_dirty_for_snapshot();
}

bool CDebugInterfaceVice::IsDriveDirtyForRefresh(int driveNum)
{
	return c64d_is_drive_dirty_and_needs_refresh(driveNum) == 0 ? false : true;
}

void CDebugInterfaceVice::SetDriveDirtyForRefreshFlag(int driveNum)
{
	c64d_set_drive_dirty_needs_refresh_flag(driveNum);
}

void CDebugInterfaceVice::ClearDriveDirtyForRefreshFlag(int driveNum)
{
	c64d_clear_drive_dirty_needs_refresh_flag(driveNum);
}


// Profiler
extern "C"
{
	extern int c64d_profiler_is_active;
	void c64d_profiler_activate(char *fileName, int runForNumCycles, int pauseCpuWhenFinished);
	void c64d_profiler_deactivate();	
}

// if fileName is NULL no file will be created, if runForNumCycles is -1 it will run till ProfilerDeactivate
// TODO: select c64 cpu or disk drive cpu
void CDebugInterfaceVice::ProfilerActivate(char *fileName, int runForNumCycles, bool pauseCpuWhenFinished)
{
	c64d_profiler_activate(fileName, runForNumCycles, pauseCpuWhenFinished ? 1:0);
}

void CDebugInterfaceVice::ProfilerDeactivate()
{
	c64d_profiler_deactivate();
}

bool CDebugInterfaceVice::IsProfilerActive()
{
	return c64d_profiler_is_active == 1 ? true : false;
}

//void CDebugInterfaceVice::UpdateDriveDiskID(int driveId)
//{
//	if (diskImage[driveId])
//	{
//		// set correct disk ID to let 1541 ROM not throw 29, 'disk id mismatch'
//		// see $F3F6 in 1541 ROM: http://unusedino.de/ec64/technical/misc/c1541/romlisting.html#FDD3
//		//
//		LOGD("...diskId= %02x %02x", diskImage->diskId[2], diskImage[driveId]->diskId[3]);
//
//		viewC64->debugInterfaceC64->SetByte1541(0x0012, diskImage[driveId]->diskId[2]);
//		viewC64->debugInterfaceC64->SetByte1541(0x0013, diskImage[driveId]->diskId[3]);
//
//		viewC64->debugInterfaceC64->SetByte1541(0x0016, diskImage[driveId]->diskId[2]);
//		viewC64->debugInterfaceC64->SetByte1541(0x0017, diskImage[driveId]->diskId[3]);
//	}
//}

// code monitor
extern "C" {
	void monitor_open(void);
	void make_prompt(char *str);
	int monitor_process(char *cmd);
	void monitor_close(int check);
	int mon_buffer_flush(void);
}

bool CDebugInterfaceVice::IsCodeMonitorSupported()
{
	return true;
}

CSlrString *CDebugInterfaceVice::GetCodeMonitorPrompt()
{
	if (!isCodeMonitorOpened)
	{
		monitor_open();
		isCodeMonitorOpened = true;
	}
	
	char *prompt = SYS_GetCharBuf();
	make_prompt(prompt);
	
	CSlrString *str = new CSlrString(prompt);
	
	SYS_ReleaseCharBuf(prompt);
	
	return str;
}

extern "C" {
	char *lib_stralloc(const char *str);
}

static void execute_monitor_command_trap(WORD addr, void *v)
{
	char *monitorCmdStr = (char*)v;
	int exit_mon = monitor_process(monitorCmdStr);
	mon_buffer_flush();
	
	LOGD("exit_mon=%d", exit_mon);
}

bool CDebugInterfaceVice::ExecuteCodeMonitorCommand(CSlrString *commandStr)
{
	LOGD("CDebugInterfaceVice::ExecuteCodeMonitorCommand");
	//if (!isCodeMonitorOpened)
	{
		monitor_open();
		isCodeMonitorOpened = true;
	}
	
	if (commandStr->IsEmpty())
	{
		return true;
	}
	
	commandStr->ConvertToLowerCase();

	char *cmdStr = commandStr->GetStdASCII();
	char *monitorCmdStr = lib_stralloc(cmdStr);
	delete [] cmdStr;

//	// we need to move to next instruction on these commands
//	if (commandStr->CompareWith("step") || commandStr->CompareWith("next") || commandStr->CompareWith("return"))
//	{
//		SetDebugMode(DEBUGGER_MODE_RUNNING);
//		interrupt_maincpu_trigger_trap(execute_monitor_command_trap, monitorCmdStr);
//	}
//	else
	{
		int exit_mon = monitor_process(monitorCmdStr);
		mon_buffer_flush();
		LOGD("exit_mon=%d", exit_mon);
	}

	return true;
}

// d1541II

void CDebugInterfaceVice::ScanFolderForRoms(const char *folderPath)
{
	char *buf = SYS_GetCharBuf();
	
	bool foundKernal = false;
	bool foundBasic = false;
	bool foundChargen = false;
	bool foundDos1541 = false;
	bool foundDos1541ii = false;

	// Filenames moved into constants
	
	// Define the variables for the strings
	const char *KERNAL_ROM_NAME = "kernal";
	const char *BASIC_ROM_NAME = "basic";
	const char *CHARGEN_ROM_NAME = "chargen";
	const char *DOS1541_ROM_NAME = "dos1541";
	const char *DOS1541II_ROM_NAME = "dos1541ii";
	const char *DOS1541II_ROM_NAME_2 = "d1541ii";
	const char *DOS1541II_ROM_NAME_3 = "dos1541II";

	// kernal
	sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, KERNAL_ROM_NAME);
	if (SYS_FileExists(buf))
	{
		if (c64SettingsPathToRomC64Kernal)
			delete c64SettingsPathToRomC64Kernal;
		c64SettingsPathToRomC64Kernal = new CSlrString(buf);
		foundKernal = true;
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Kernal))
	{
		delete c64SettingsPathToRomC64Kernal;
		c64SettingsPathToRomC64Kernal = NULL;
		foundKernal = false;
	}
	
	// basic
	sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, BASIC_ROM_NAME);
	if (SYS_FileExists(buf))
	{
		if (c64SettingsPathToRomC64Basic)
			delete c64SettingsPathToRomC64Basic;
		c64SettingsPathToRomC64Basic = new CSlrString(buf);
		foundBasic = true;
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Basic))
	{
		delete c64SettingsPathToRomC64Basic;
		c64SettingsPathToRomC64Basic = NULL;
		foundBasic = false;
	}
	
	// chargen
	sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, CHARGEN_ROM_NAME);
	if (SYS_FileExists(buf))
	{
		if (c64SettingsPathToRomC64Chargen)
			delete c64SettingsPathToRomC64Chargen;
		c64SettingsPathToRomC64Chargen = new CSlrString(buf);
		foundChargen = true;
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Chargen))
	{
		delete c64SettingsPathToRomC64Chargen;
		c64SettingsPathToRomC64Chargen = NULL;
		foundChargen = false;
	}
	
	// drive 1541
	sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, DOS1541_ROM_NAME);
	if (SYS_FileExists(buf))
	{
		if (c64SettingsPathToRomC64Drive1541)
			delete c64SettingsPathToRomC64Drive1541;
		c64SettingsPathToRomC64Drive1541 = new CSlrString(buf);
		foundDos1541 = true;
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Drive1541))
	{
		delete c64SettingsPathToRomC64Drive1541;
		c64SettingsPathToRomC64Drive1541 = NULL;
		foundDos1541 = false;
	}
	
	// drive 1541-II
	sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, DOS1541II_ROM_NAME);
	if (SYS_FileExists(buf))
	{
		if (c64SettingsPathToRomC64Drive1541ii)
			delete c64SettingsPathToRomC64Drive1541ii;
		c64SettingsPathToRomC64Drive1541ii = new CSlrString(buf);
		foundDos1541ii = true;
	}
	else
	{
		sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, DOS1541II_ROM_NAME_2);
		if (SYS_FileExists(buf))
		{
			if (c64SettingsPathToRomC64Drive1541ii)
				delete c64SettingsPathToRomC64Drive1541ii;
			c64SettingsPathToRomC64Drive1541ii = new CSlrString(buf);
			foundDos1541ii = true;
		}
		else
		{
			sprintf(buf, "%s%c%s", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR, DOS1541II_ROM_NAME_3);
			if (SYS_FileExists(buf))
			{
				if (c64SettingsPathToRomC64Drive1541ii)
					delete c64SettingsPathToRomC64Drive1541ii;
				c64SettingsPathToRomC64Drive1541ii = new CSlrString(buf);
				foundDos1541ii = true;
			}
		}
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Drive1541ii))
	{
		delete c64SettingsPathToRomC64Drive1541ii;
		c64SettingsPathToRomC64Drive1541ii = NULL;
		foundDos1541ii = false;
	}

	// Made more user-friendly: show filenames and path
	// prepare message box
	sprintf(buf, "C64 ROMs Status:\n\n"
			  "Folder: '%s'\n\n"
			  "Kernal ROM (File: '%s'): %s\n"
			  "Basic ROM (File: '%s'): %s\n"
			  "Chargen ROM (File: '%s'): %s\n"
			  "Drive 1541 ROM (File: '%s'): %s\n"
			  "Drive 1541-II ROM (File: '%s'): %s\n\n"
			  "Please ensure that all required ROM files are in the specified folder.",
		folderPath,
		KERNAL_ROM_NAME, foundKernal ? "FOUND" : (c64SettingsPathToRomC64Kernal ? "MISSING, using previous version" : "MISSING, no version available"),
		BASIC_ROM_NAME, foundBasic ? "FOUND" : (c64SettingsPathToRomC64Basic ? "MISSING, using previous version" : "MISSING, no version available"),
		CHARGEN_ROM_NAME, foundChargen ? "FOUND" : (c64SettingsPathToRomC64Chargen ? "MISSING, using previous version" : "MISSING, no version available"),
		DOS1541_ROM_NAME, foundDos1541 ? "FOUND" : (c64SettingsPathToRomC64Drive1541 ? "MISSING, using previous version" : "MISSING, no version available"),
		DOS1541II_ROM_NAME, foundDos1541ii ? "FOUND" : (c64SettingsPathToRomC64Drive1541ii ? "MISSING, using previous version" : "MISSING, no version available"));

	ShowMessageBox("C64 ROMs folder scan", buf);
	
	SYS_ReleaseCharBuf(buf);
	
	UpdateRoms();
	
	if (GetDebugMode() == DEBUGGER_MODE_PAUSED
		&& foundKernal && foundBasic && foundChargen)
	{
		SetDebugMode(DEBUGGER_MODE_RUNNING);
	}
}

extern "C" {
	void c64d_trigger_update_roms();

	void _c64d_update_roms_trap(WORD addr, void *data)
	{
		LOGD("_c64d_update_roms_trap");
		debugInterfaceVice->UpdateRomsTrap();
	}

}

extern "C" {
	int driverom_load_images(void);
	extern int rom_loaded;

}

void CDebugInterfaceVice::UpdateRomsTrap()
{
	// Note: for some reason resources_set_string("DosName1541", str); or DosName1541ii crashes drive when drive was not initialised before (i.e. in situation that there were no roms at start). Thus we can not set resources_set_string here. As a workaround we are reading roms into embedded space and re-read them as defaults, plus init the drive from scratch

	ReadEmbeddedRoms();
	
	this->LockMutex();
	
	rom_loaded = 0;
	resources_set_defaults();
	drive_init();

	this->UnlockMutex();
	
	ResetHard();
	DiskDriveReset();
	SetDebugMode(DEBUGGER_MODE_RUNNING);
}

void CDebugInterfaceVice::UpdateRoms()
{
	c64d_trigger_update_roms();
}

extern "C" {
extern BYTE c64memrom_kernal64_rom[C64_KERNAL_ROM_SIZE];
extern BYTE c64memrom_basic64_rom[C64_BASIC_ROM_SIZE];
extern BYTE mem_chargen_rom[C64_CHARGEN_ROM_SIZE];
extern BYTE drive_rom1541_embedded[DRIVE_ROM1541_SIZE];
extern BYTE drive_rom1541ii_embedded[DRIVE_ROM1541II_SIZE];
}

void CDebugInterfaceVice::DumpRomsToFolder(const char *folderPath)
{
	char *buf = SYS_GetCharBuf();

	// kernal
	sprintf(buf, "%s%ckernal", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR);
	CByteBuffer::WriteBufferToFile(buf, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
	
	// basic
	sprintf(buf, "%s%cbasic", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR);
	CByteBuffer::WriteBufferToFile(buf, c64memrom_basic64_rom, C64_BASIC_ROM_SIZE);

	// chargen
	sprintf(buf, "%s%cchargen", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR);
	CByteBuffer::WriteBufferToFile(buf, mem_chargen_rom, C64_CHARGEN_ROM_SIZE);

	// drive 1541
	sprintf(buf, "%s%cdos1541", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR);
	CByteBuffer::WriteBufferToFile(buf, drive_rom1541_embedded, DRIVE_ROM1541_SIZE);

	// drive 1541-II
	sprintf(buf, "%s%cdos1541II", folderPath, SYS_FILE_SYSTEM_PATH_SEPARATOR);
	CByteBuffer::WriteBufferToFile(buf, drive_rom1541ii_embedded, DRIVE_ROM1541II_SIZE);

	SYS_ReleaseCharBuf(buf);
}

void CDebugInterfaceVice::ReadEmbeddedRoms()
{
	CByteBuffer::ReadFromFileOrClearBuffer(c64SettingsPathToRomC64Kernal, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
	CByteBuffer::ReadFromFileOrClearBuffer(c64SettingsPathToRomC64Basic, c64memrom_basic64_rom, C64_BASIC_ROM_SIZE);
	CByteBuffer::ReadBufferFromFile(c64SettingsPathToRomC64Chargen, mem_chargen_rom, C64_CHARGEN_ROM_SIZE);
	CByteBuffer::ReadFromFileOrClearBuffer(c64SettingsPathToRomC64Drive1541, drive_rom1541_embedded, DRIVE_ROM1541_SIZE);
	CByteBuffer::ReadFromFileOrClearBuffer(c64SettingsPathToRomC64Drive1541ii, drive_rom1541ii_embedded, DRIVE_ROM1541II_SIZE);
}

void CDebugInterfaceVice::CheckLoadedRoms()
{
	const char *sep = ""; const char *sep2 = "\n";
	char *buf = SYS_GetCharBuf();
	char *buf2 = SYS_GetCharBuf();
	int n = 0;
	
	if (!SYS_FileExists(c64SettingsPathToRomC64Kernal))
	{
		strcat(buf, "kernal"); sep = sep2; n++;
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Basic))
	{
		sprintf(buf2, "%sbasic", sep); sep = sep2; n++;
		strcat(buf, buf2);
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Chargen))
	{
		sprintf(buf2, "%schargen", sep); sep = sep2; n++;
		strcat(buf, buf2);
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Drive1541))
	{
		sprintf(buf2, "%sdos1541", sep); sep = sep2; n++;
		strcat(buf, buf2);
	}
	if (!SYS_FileExists(c64SettingsPathToRomC64Drive1541ii))
	{
		sprintf(buf2, "%sdos1541II", sep); n++;
		strcat(buf, buf2);
	}
	
	if (buf[0] != 0)
	{
		if (n != 5)
		{
			sprintf(buf2, "Failed to load C64 ROM files:\n%s", buf);
			viewC64->ShowMessageError(buf2);
		}
		else
		{
			viewC64->ShowMessageError("C64 ROM files are undefined. Please navigate to Settings and select the appropriate C64 ROMs folder to proceed.");
		}
	}
	
	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(buf2);

}

//
void CDebugInterfaceVice::SupportsBreakpoints(bool *writeBreakpoint, bool *readBreakpoint)
{
	*writeBreakpoint = true;
	*readBreakpoint = true;
}

CDebuggerApi *CDebugInterfaceVice::GetDebuggerApi()
{
	return new CDebuggerApiVice(this);
}

CDebuggerServerApi *CDebugInterfaceVice::GetDebuggerServerApi()
{
	if (debuggerServerApi)
		return debuggerServerApi;
	
	debuggerServerApi = new CDebuggerServerApiVice(this);
	return debuggerServerApi;
}

// SID poke/peek
// Note: SID is specific, as setting one register may cause strange effects. So we push all registers at once.
CPool CSidData::poolSidData(6000, sizeof(CSidData));

CSidData::CSidData()
{
	for (int i = 0; i < SOUND_SIDS_MAX; i++)
	{
		for (int reg = 0; reg < C64_NUM_SID_REGISTERS; reg++)
		{
			sidRegs[i][reg] = 0x00;
			shouldSetSidReg[i][reg] = true;
		}
	}
}

extern "C" {
void c64d_store_sid_data(BYTE *sidDataStore, int sidNum);
}

void CSidData::PeekFromSids()
{
	for (int sidNum = 0; sidNum < debugInterfaceVice->numSids; sidNum++)
	{
		c64d_store_sid_data(sidRegs[sidNum], sidNum);
	}
}

void CSidData::RestoreSids()
{
//	LOGD("CSidData::RestoreSids");
	gSoundEngine->LockMutex("CSidData::RestoreSids");
	c64d_skip_sound_run_sound_in_sound_store = TRUE;
	for (int sidNum = 0; sidNum < debugInterfaceVice->numSids; sidNum++)
	{
		for (int registerNum = 0; registerNum < C64_NUM_SID_REGISTERS; registerNum++)
		{
			if (shouldSetSidReg[sidNum][registerNum])
			{
				sid_store_chip(registerNum, sidRegs[sidNum][registerNum], sidNum);
			}
		}
	}
	c64d_skip_sound_run_sound_in_sound_store = FALSE;
	gSoundEngine->UnlockMutex("CSidData::RestoreSids");
//	LOGD("CSidData::RestoreSids done");
}

void CSidData::CopyFrom(CSidData *sidData)
{
	for (int sidNum = 0; sidNum < SOUND_SIDS_MAX; sidNum++)
	{
		for (int regNum = 0; regNum < C64_NUM_SID_REGISTERS; regNum++)
		{
			this->sidRegs[sidNum][regNum] = sidData->sidRegs[sidNum][regNum];
		}
	}
}

void CSidData::Serialize(CByteBuffer *byteBuffer)
{
	for (int sidNum = 0; sidNum < SOUND_SIDS_MAX; sidNum++)
	{
		for (int regNum = 0; regNum < C64_NUM_SID_REGISTERS; regNum++)
		{
			byteBuffer->PutU8(sidRegs[sidNum][regNum]);
		}
	}
}

bool CSidData::Deserialize(CByteBuffer *byteBuffer)
{
	for (int sidNum = 0; sidNum < SOUND_SIDS_MAX; sidNum++)
	{
		for (int regNum = 0; regNum < C64_NUM_SID_REGISTERS; regNum++)
		{
			if (byteBuffer->IsEof())
				return false;
			
			u8 v = byteBuffer->GetU8();
			sidRegs[sidNum][regNum] = v;
		}
	}
	return true;
}

bool CSidData::SaveRegs(const char *fileName)
{
	FILE *fp = fopen(fileName, "wb");
	if (!fp)
	{
		return false;
	}
	
	fwrite(sidRegs, 1, SOUND_SIDS_MAX*C64_NUM_SID_REGISTERS, fp);
	fclose(fp);
	return true;
}

bool CSidData::LoadRegs(const char *fileName)
{
	FILE *fp = fopen(fileName, "rb");
	if (!fp)
	{
		return false;
	}
	fread(sidRegs, 1, SOUND_SIDS_MAX*C64_NUM_SID_REGISTERS, fp);
	fclose(fp);
	return true;
}

/// default keymap
void ViceKeyMapInitDefault()
{
	SYS_FatalExit("ViceKeyMapInitDefault not implemented");
	
	
//	 C64 keyboard matrix:
//	 
//	 Bit   7   6   5   4   3   2   1   0
//	 0    CUD  F5  F3  F1  F7 CLR RET DEL
//	 1    SHL  E   S   Z   4   A   W   3
//	 2     X   T   F   C   6   D   R   5
//	 3     V   U   H   B   8   G   Y   7
//	 4     N   O   K   M   0   J   I   9
//	 5     ,   @   :   .   -   L   P   +
//	 6     /   ^   =  SHR HOM  ;   *   £
//	 7    R/S  Q   C= SPC  2  CTL  <-  1
	
	// MATRIX (row, column)
	
	// http://classiccmp.org/dunfield/c64/h/front.jpg
	
	//	keyboard_parse_set_pos_row('a', int row, int col, int shift);
	
	/*
	
	keyboard_parse_set_pos_row(MTKEY_F5, 0, 6, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F6, 0, 6, LEFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F3, 0, 5, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F4, 0, 5, LEFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F1, 0, 4, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F2, 0, 4, LEFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F7, 0, 3, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_F8, 0, 3, LEFT_SHIFT);
	
	keyboard_parse_set_pos_row(MTKEY_ENTER, 0, 1, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_BACKSPACE, 0, 0, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_LSHIFT, 1, 7, NO_SHIFT);
	keyboard_parse_set_pos_row('e', 1, 6, NO_SHIFT);
	keyboard_parse_set_pos_row('s', 1, 5, NO_SHIFT);
	keyboard_parse_set_pos_row('z', 1, 4, NO_SHIFT);
	keyboard_parse_set_pos_row('4', 1, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('a', 1, 2, NO_SHIFT);
	keyboard_parse_set_pos_row('w', 1, 1, NO_SHIFT);
	keyboard_parse_set_pos_row('3', 1, 0, NO_SHIFT);
	keyboard_parse_set_pos_row('x', 2, 7, NO_SHIFT);
	keyboard_parse_set_pos_row('t', 2, 6, NO_SHIFT);
	keyboard_parse_set_pos_row('f', 2, 5, NO_SHIFT);
	keyboard_parse_set_pos_row('c', 2, 4, NO_SHIFT);
	keyboard_parse_set_pos_row('6', 2, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('d', 2, 2, NO_SHIFT);
	keyboard_parse_set_pos_row('r', 2, 1, NO_SHIFT);
	keyboard_parse_set_pos_row('5', 2, 0, NO_SHIFT);
	keyboard_parse_set_pos_row('v', 3, 7, NO_SHIFT);
	keyboard_parse_set_pos_row('u', 3, 6, NO_SHIFT);
	keyboard_parse_set_pos_row('h', 3, 5, NO_SHIFT);
	keyboard_parse_set_pos_row('b', 3, 4, NO_SHIFT);
	keyboard_parse_set_pos_row('8', 3, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('g', 3, 2, NO_SHIFT);
	keyboard_parse_set_pos_row('y', 3, 1, NO_SHIFT);
	keyboard_parse_set_pos_row('7', 3, 0, NO_SHIFT);
	keyboard_parse_set_pos_row('n', 4, 7, NO_SHIFT);
	keyboard_parse_set_pos_row('o', 4, 6, NO_SHIFT);
	keyboard_parse_set_pos_row('k', 4, 5, NO_SHIFT);
	keyboard_parse_set_pos_row('m', 4, 4, NO_SHIFT);
	keyboard_parse_set_pos_row('0', 4, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('j', 4, 2, NO_SHIFT);
	keyboard_parse_set_pos_row('i', 4, 1, NO_SHIFT);
	keyboard_parse_set_pos_row('9', 4, 0, NO_SHIFT);
	keyboard_parse_set_pos_row(',', 5, 7, NO_SHIFT);
	keyboard_parse_set_pos_row('[', 5, 6, NO_SHIFT);
	keyboard_parse_set_pos_row(';', 5, 5, NO_SHIFT);
	keyboard_parse_set_pos_row('.', 5, 4, NO_SHIFT);
	keyboard_parse_set_pos_row('-', 5, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('l', 5, 2, NO_SHIFT);
	keyboard_parse_set_pos_row('p', 5, 1, NO_SHIFT);
	keyboard_parse_set_pos_row('=', 5, 0, NO_SHIFT);
	keyboard_parse_set_pos_row('/', 6, 7, NO_SHIFT);
	//	keyboard_parse_set_pos_row('^', 6, 6, NO_SHIFT);
	//	keyboard_parse_set_pos_row('@', 6, 5, DESHIFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_RSHIFT, 6, 4, NO_SHIFT);
	//	keyboard_parse_set_pos_row('', 6, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('\'', 6, 2, NO_SHIFT);
	keyboard_parse_set_pos_row(']', 6, 1, NO_SHIFT);
	//	keyboard_parse_set_pos_row('', 6, 0, NO_SHIFT);
	
	keyboard_parse_set_pos_row('`', 7, 7, NO_SHIFT);
	keyboard_parse_set_pos_row('q', 7, 6, NO_SHIFT);
	//	keyboard_parse_set_pos_row('', 7, 5, NO_SHIFT);
	keyboard_parse_set_pos_row(' ', 7, 4, NO_SHIFT);
	keyboard_parse_set_pos_row('2', 7, 3, NO_SHIFT);
	keyboard_parse_set_pos_row('@', 7, 3, LEFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_LCONTROL, 7, 2, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_LALT, 7, 5, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_ESC, 7, 1, NO_SHIFT);
	keyboard_parse_set_pos_row('1', 7, 0, NO_SHIFT);
	
	keyboard_parse_set_pos_row(MTKEY_ARROW_UP, 0, 7, LEFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_ARROW_DOWN, 0, 7, NO_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_ARROW_LEFT, 0, 2, LEFT_SHIFT);
	keyboard_parse_set_pos_row(MTKEY_ARROW_RIGHT, 0, 2, NO_SHIFT);
	
	*/

	
//	 C64 keyboard matrix:
//	 
//	 Bit   7   6   5   4   3   2   1   0
//	 0    CUD  F5  F3  F1  F7 CLR RET DEL
//	 1    SHL  E   S   Z   4   A   W   3
//	 2     X   T   F   C   6   D   R   5
//	 3     V   U   H   B   8   G   Y   7
//	 4     N   O   K   M   0   J   I   9
//	 5     ,   @   :   .   -   L   P   +
//	 6     /   ^   =  SHR HOM  ;   *   £
//	 7    R/S  Q   C= SPC  2  CTL  <-  1

}

