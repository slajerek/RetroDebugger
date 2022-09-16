
#include "CDebugInterfaceAtari.h"
#include "CAudioChannelAtari.h"
#include "VID_Main.h"
#include "SYS_Main.h"
#include "SND_Main.h"
#include "SYS_Types.h"
#include "SYS_CommandLine.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CViewAtariStatePOKEY.h"
#include "CViewMemoryMap.h"
#include "SND_SoundEngine.h"
#include "CSnapshotsManager.h"
#include "C64D_Version.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include <string.h>

extern "C" {
#include "AtariWrapper.h"
#include "statesav.h"
#include "screen.h"
#include "memory.h"
	volatile int atrd_debug_mode;
}

volatile int atrd_start_frame_for_snapshots_manager = 0;

extern "C" {
	char *ATRD_GetPathForRoms()
	{
		// TODO: nasty implementation shortcut. needs to be addressed.
		char *buf = viewC64->ATRD_GetPathForRoms_IMPL();
		LOGD("ATRD_GetPathForRoms: %s", buf);
		return buf;
	}
}

extern "C" {
	int Atari800_GetPC();
}

void atrd_mark_atari_cell_read(uint16 addr)
{
	int pc = Atari800_GetPC();

	viewC64->viewAtariMemoryMap->CellRead(addr, pc, -1, -1);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceAtari->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	debugInterfaceAtari->LockMutex();
	
	CDebugSymbolsSegment *segment = debugInterfaceAtari->symbols->currentSegment;
	if (segment)
	{
		u8 value = MEMORY_SafeGetByte(addr);
		if (segment->breakpointsMemory->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_READ) != NULL)
		{
			debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_PAUSED);
		}
	}
	
	debugInterfaceAtari->UnlockMutex();
}

void atrd_mark_atari_cell_write(uint16 addr, uint8 value)
{
	int pc = Atari800_GetPC();
	viewC64->viewAtariMemoryMap->CellWrite(addr, value, pc, -1, -1); //viceCurrentC64PC, vicii.raster_line, vicii.raster_cycle);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceAtari->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	debugInterfaceAtari->LockMutex();
	
	CDebugSymbolsSegment *segment = debugInterfaceAtari->symbols->currentSegment;
	if (segment)
	{
		if (segment->breakpointsMemory->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_WRITE) != NULL)
		{
			debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_PAUSED);
		}
	}
	
	debugInterfaceAtari->UnlockMutex();
}

void atrd_mark_atari_cell_execute(uint16 addr, uint8 opcode)
{
//	LOGD("atrd_mark_atari_cell_execute: %04x %02x", addr, opcode);
	viewC64->viewAtariMemoryMap->CellExecute(addr, opcode);
}

int atrd_is_debug_on_atari()
{
	if (debugInterfaceAtari->isDebugOn)
		return 1;
	
	return 0;
}

void atrd_check_pc_breakpoint(uint16 pc)
{
//	LOGD("atrd_check_pc_breakpoint: pc=%04x", pc);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceAtari->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	CDebugInterfaceAtari *debugInterface = debugInterfaceAtari;

	CDebugSymbolsSegment *segment = debugInterface->symbols->currentSegment;
	
	if (!segment)
	{
		return;
	}
	
	if ((int)pc == segment->breakpointsPC->temporaryBreakpointPC)
	{
		debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
		segment->breakpointsPC->temporaryBreakpointPC = -1;
	}
	else
	{
		debugInterface->LockMutex();
		CBreakpointAddr *addrBreakpoint = segment->breakpointsPC->EvaluateBreakpoint(pc);
		if (addrBreakpoint != NULL)
		{
			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_SET_BACKGROUND))
			{
				// Not supported
			}
			
			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP))
			{
				debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
			}
		}
		debugInterface->UnlockMutex();
	}
}
//


extern "C" {
	void atrd_check_cpu_snapshot_manager_restore();
	void atrd_check_cpu_snapshot_manager_store();
	int atrd_check_maincpu_cycle();
}

#define ATRD_ASYNC_NO_COMMAND		0
#define ATRD_ASYNC_LOAD_SNAPSHOT	1
#define ATRD_ASYNC_SAVE_SNAPSHOT	2
#define ATRD_ASYNC_SET_PC			3
#define ATRD_ASYNC_SET_REG_A		4
#define ATRD_ASYNC_SET_REG_X		5
#define ATRD_ASYNC_SET_REG_Y		6
#define ATRD_ASYNC_SET_REG_P		7
#define ATRD_ASYNC_SET_REG_S		8

int atrd_async_command = ATRD_ASYNC_NO_COMMAND;
void *atrd_async_data;

void atrd_sync_load_snapshot(char *filePath);
void atrd_sync_save_snapshot(char *filePath);

extern "C" {
extern volatile UWORD CPU_regPC;
};

int atrd_debug_pause_check(int allowRestore)
{
//	LOGD("atrd_debug_pause_check, atrd_debug_mode=%d", atrd_debug_mode);
	
	int shouldSkipOneInstructionStep = atrd_check_maincpu_cycle();
	
	if (allowRestore)
	{
		atrd_check_cpu_snapshot_manager_restore();
	}
	else
	{
		if (atrd_is_performing_snapshot_restore())
			return FALSE;
	}
	
	if (atrd_debug_mode == DEBUGGER_MODE_PAUSED)
	{
		int frameNum = atrd_get_emulation_frame_num();
		LOGD("frame=%d atrdMainCpuCycle=%d PC=%x", frameNum, atrdMainCpuCycle, CPU_regPC);

		//		c64d_refresh_previous_lines();
		//		c64d_refresh_dbuf();
		//		c64d_refresh_cia();
		
		while (atrd_debug_mode == DEBUGGER_MODE_PAUSED)
		{
//			LOGD("atrd_debug_pause_check, waiting... PC=%04x atrd_debug_mode=%d", Atari800_GetPC(), atrd_debug_mode);
			mt_SYS_Sleep(10);
			//			vsync_do_vsync(vicii.raster.canvas, 0, 1);
			//mt_SYS_Sleep(50);
			
			if (debugInterfaceAtari->snapshotsManager->snapshotToRestore != NULL)
				break;
		}
		
//		LOGD("atrd_debug_pause_check: new mode is %d PC=%04x", atrd_debug_mode, Atari800_GetPC());
		
		return shouldSkipOneInstructionStep;
	}
	
	return FALSE;
}

int atrd_is_performing_snapshot_restore()
{
	if (debugInterfaceAtari->snapshotsManager->IsPerformingSnapshotRestore())
	{
		return 1;
	}
	return 0;
}

int atrd_check_maincpu_cycle()
{
//	LOGD("atrd_check_maincpu_cycle");
	if (debugInterfaceAtari->snapshotsManager->CheckMainCpuCycle())
	{
		return TRUE;
	}
	return FALSE;
}

int atrd_check_snapshot_restore()
{
//	LOGD("atrd_check_snapshot_restore");
	
	debugInterfaceAtari->snapshotsManager->CheckMainCpuCycle();
	
	if (debugInterfaceAtari->snapshotsManager->CheckSnapshotRestore())
	{
		return 1;
	}
	
	return 0;
}

void atrd_check_snapshot_interval()
{
	if (atrd_start_frame_for_snapshots_manager)
	{
		//		LOGD("atrd_check_snapshot_interval: %d", c64d_start_frame_for_snapshots_manager);
		atrd_start_frame_for_snapshots_manager = 0;
		debugInterfaceAtari->snapshotsManager->CheckSnapshotInterval();
	}
}

int atrd_get_emulation_frame_num()
{
	return debugInterfaceAtari->GetEmulationFrameNumber();
}

//
unsigned int atrd_get_joystick_state(int joystickNum)
{
	return debugInterfaceAtari->joystickState[joystickNum];
}

//
extern "C" {
	void atrd_atari_set_cpu_pc(u16 addr);
	void atrd_atari_set_cpu_reg_a(u8 val);
	void atrd_atari_set_cpu_reg_x(u8 val);
	void atrd_atari_set_cpu_reg_y(u8 val);
	void atrd_atari_set_cpu_reg_p(u8 val);
	void atrd_atari_set_cpu_reg_s(u8 val);
}

void atrd_async_check()
{
//	LOGD("atrd_async_check");
	
	atrd_mutex_lock();
	
	if (atrd_check_snapshot_restore())
	{
		atrd_mutex_unlock();
		return;
	}
	
	atrd_check_snapshot_interval();
	
	if (atrd_is_performing_snapshot_restore())
	{
		atrd_mutex_unlock();
		return;
	}
	
	if (atrd_async_command == ATRD_ASYNC_LOAD_SNAPSHOT)
	{
		char *fileName = (char *)atrd_async_data;
		atrd_sync_load_snapshot(fileName);
		free(fileName);
	}
	else if (atrd_async_command == ATRD_ASYNC_SAVE_SNAPSHOT)
	{
		char *fileName = (char *)atrd_async_data;
		atrd_sync_save_snapshot(fileName);
		free(fileName);
	}
	else if (atrd_async_command == ATRD_ASYNC_SET_PC)
	{
		int *newPC = (int *)atrd_async_data;
		atrd_atari_set_cpu_pc(*newPC);
		free(newPC);
	}
	else if (atrd_async_command == ATRD_ASYNC_SET_REG_A)
	{
		int *newRegValue = (int *)atrd_async_data;
		atrd_atari_set_cpu_reg_a(*newRegValue);
		free(newRegValue);
	}
	else if (atrd_async_command == ATRD_ASYNC_SET_REG_X)
	{
		int *newRegValue = (int *)atrd_async_data;
		atrd_atari_set_cpu_reg_x(*newRegValue);
		free(newRegValue);
	}
	else if (atrd_async_command == ATRD_ASYNC_SET_REG_Y)
	{
		int *newRegValue = (int *)atrd_async_data;
		atrd_atari_set_cpu_reg_y(*newRegValue);
		free(newRegValue);
	}
	else if (atrd_async_command == ATRD_ASYNC_SET_REG_P)
	{
		int *newRegValue = (int *)atrd_async_data;
		atrd_atari_set_cpu_reg_p(*newRegValue);
		free(newRegValue);
	}
	else if (atrd_async_command == ATRD_ASYNC_SET_REG_S)
	{
		int *newRegValue = (int *)atrd_async_data;
		atrd_atari_set_cpu_reg_s(*newRegValue);
		free(newRegValue);
	}

	atrd_async_data = NULL;
	atrd_async_command = ATRD_ASYNC_NO_COMMAND;
	
	atrd_mutex_unlock();
}

void atrd_async_load_snapshot(char *filePath)
{
	char *fc = strdup(filePath);
	atrd_async_data = (void*)fc;
	atrd_async_command = ATRD_ASYNC_LOAD_SNAPSHOT;
}

void atrd_async_save_snapshot(char *filePath)
{
	char *fc = strdup(filePath);
	atrd_async_data = (void*)fc;
	atrd_async_command = ATRD_ASYNC_SAVE_SNAPSHOT;
}

void atrd_async_set_cpu_pc(int newPC)
{
	int *pc = (int *)malloc(1 * sizeof(int));
	*pc = newPC;
	atrd_async_data = (void*)pc;
	atrd_async_command = ATRD_ASYNC_SET_PC;
	
	// if code is paused/going to be, then pc should be set immediately
	atrd_atari_set_cpu_pc(newPC);
}

void atrd_async_set_reg_a(int newRegValue)
{
	int *regValue = (int *)malloc(1 * sizeof(int));
	*regValue = newRegValue;
	atrd_async_data = (void*)regValue;
	atrd_async_command = ATRD_ASYNC_SET_REG_A;
	
	// if code is paused/going to be, then pc should be set now
	atrd_atari_set_cpu_reg_a(*regValue);
}

void atrd_async_set_reg_x(int newRegValue)
{
	int *regValue = (int *)malloc(1 * sizeof(int));
	*regValue = newRegValue;
	atrd_async_data = (void*)regValue;
	atrd_async_command = ATRD_ASYNC_SET_REG_X;
	
	// if code is paused/going to be, then pc should be set now
	atrd_atari_set_cpu_reg_x(*regValue);
}

void atrd_async_set_reg_y(int newRegValue)
{
	int *regValue = (int *)malloc(1 * sizeof(int));
	*regValue = newRegValue;
	atrd_async_data = (void*)regValue;
	atrd_async_command = ATRD_ASYNC_SET_REG_Y;
	
	// if code is paused/going to be, then pc should be set now
	atrd_atari_set_cpu_reg_y(*regValue);
}

void atrd_async_set_reg_p(int newRegValue)
{
	int *regValue = (int *)malloc(1 * sizeof(int));
	*regValue = newRegValue;
	atrd_async_data = (void*)regValue;
	atrd_async_command = ATRD_ASYNC_SET_REG_P;
	
	// if code is paused/going to be, then pc should be set now
	atrd_atari_set_cpu_reg_p(*regValue);
}


void atrd_async_set_reg_s(int newRegValue)
{
	int *regValue = (int *)malloc(1 * sizeof(int));
	*regValue = newRegValue;
	atrd_async_data = (void*)regValue;
	atrd_async_command = ATRD_ASYNC_SET_REG_S;
	
	// if code is paused/going to be, then pc should be set now
	atrd_atari_set_cpu_reg_s(*regValue);
}




void atrd_sync_load_snapshot(char *filePath)
{
	LOGD("atrd_sync_load_snapshot %s", filePath);
	if (StateSav_ReadAtariState(filePath, "rb") == FALSE)
	{
		LOGError("atrd_sync_load_snapshot: failed");
	}
	
	debugInterfaceAtari->snapshotsManager->ClearSnapshotsHistory();
	debugInterfaceAtari->ResetEmulationFrameCounter();
	debugInterfaceAtari->ResetMainCpuDebugCycleCounter();

}

void atrd_sync_save_snapshot(char *filePath)
{
	LOGD("atrd_sync_save_snapshot %s", filePath);
	if (StateSav_SaveAtariState(filePath, "wb", TRUE) == FALSE)
	{
		LOGError("atrd_sync_save_snapshot: failed");
	}
}

//
extern "C" {
	int StateSav_SaveAtariStateToByteBuffer(void *ByteBufferHook, UBYTE SaveVerbose);
	int StateSav_ReadAtariStateFromByteBuffer(void *ByteBufferHook);
}

#define ATRD_STATE_BUFFER_ONLY	0
#define ATRD_STATE_WRITE		1
#define ATRD_STATE_READ			2
CByteBuffer *atrdStateBuffer = NULL;
u8 atrdStateSaveMode = ATRD_STATE_BUFFER_ONLY;
char *atrdStateFileName;

bool atrd_store_snapshot_to_bytebuffer_synced(CByteBuffer *byteBuffer)
{
	debugInterfaceAtari->LockMutex();
	gSoundEngine->LockMutex("atrd_store_snapshot_to_bytebuffer_synced");
	
	byteBuffer->Rewind();
	atrdStateSaveMode = ATRD_STATE_BUFFER_ONLY;
	int ret = StateSav_SaveAtariStateToByteBuffer(byteBuffer, TRUE);
	atrdStateBuffer = NULL;
	
	byteBuffer->PutU32(debugInterfaceAtari->emulationFrameCounter);

	byteBuffer->PutU32(atrdMainCpuCycle);
	byteBuffer->PutU32(atrdMainCpuDebugCycle);
	byteBuffer->PutU32(atrdMainCpuPreviousInstructionCycle);
	
	// store screen
	Uint8 *screenBuffer = (Uint8 *)Screen_atari;
	int len = debugInterfaceAtari->GetScreenSizeX() * debugInterfaceAtari->GetScreenSizeY();
	byteBuffer->PutBytes(screenBuffer, len);
	
//	LOGD("atrd_store_snapshot_to_bytebuffer_synced: len=%d", byteBuffer->length);

	gSoundEngine->UnlockMutex("atrd_store_snapshot_to_bytebuffer_synced");
	debugInterfaceAtari->UnlockMutex();
	return (ret == TRUE) ? true : false;
}

extern "C" {
	extern UWORD *scrn_ptr;
}

bool atrd_restore_snapshot_from_bytebuffer_synced(CByteBuffer *byteBuffer)
{
	debugInterfaceAtari->LockMutex();
	gSoundEngine->LockMutex("atrd_restore_snapshot_from_bytebuffer_synced");

	byteBuffer->Rewind();
	LOGD("atrd_restore_snapshot_from_bytebuffer_synced: len=%d", byteBuffer->length);
	atrdStateSaveMode = ATRD_STATE_BUFFER_ONLY;
	int ret = StateSav_ReadAtariStateFromByteBuffer(byteBuffer);
	atrdStateBuffer = NULL;

	debugInterfaceAtari->emulationFrameCounter = byteBuffer->GetU32();
	
	atrdMainCpuCycle = byteBuffer->GetU32();
	atrdMainCpuDebugCycle = byteBuffer->GetU32();
	atrdMainCpuPreviousInstructionCycle = byteBuffer->GetU32();

	// restore screen
	Uint8 *screenBuffer = (Uint8 *)Screen_atari;
	int len = debugInterfaceAtari->GetScreenSizeX() * debugInterfaceAtari->GetScreenSizeY();
	byteBuffer->GetBytes(screenBuffer, len);

	scrn_ptr = (UWORD *) Screen_atari;

	gSoundEngine->UnlockMutex("atrd_restore_snapshot_from_bytebuffer_synced");
	debugInterfaceAtari->UnlockMutex();

	return (ret == TRUE) ? true : false;
}

void *atrd_state_buffer_open(const char *name, const char *mode)
{
	if (atrdStateBuffer != NULL)
	{
		delete atrdStateBuffer;
		atrdStateBuffer = NULL;
	}
	
	if (!strcmp(mode, "rb"))
	{
		if (!SYS_FileExists((char*)name))
		{
			return NULL;
		}
		
		atrdStateSaveMode = ATRD_STATE_READ;
		atrdStateBuffer = new CByteBuffer((char*)name, false);
	}
	else if (!strcmp(mode, "wb"))
	{
		atrdStateSaveMode = ATRD_STATE_WRITE;
		atrdStateFileName = STRALLOC(name);
		atrdStateBuffer = new CByteBuffer();
	}
	else
	{
		SYS_FatalExit("atrd_state_buffer_open: unknown mode %s", mode);
	}
	
	return atrdStateBuffer;
}

int atrd_state_buffer_close(void *stream)
{
	if (atrdStateSaveMode == ATRD_STATE_READ)
	{
		delete atrdStateBuffer;
		atrdStateBuffer = NULL;
	}
	else if (atrdStateSaveMode == ATRD_STATE_WRITE)
	{
		atrdStateBuffer->storeToFileNoHeader(atrdStateFileName);
		STRFREE(atrdStateFileName);
		atrdStateFileName = NULL;
		delete atrdStateBuffer;
		atrdStateBuffer = NULL;
	}
	else if (atrdStateSaveMode == ATRD_STATE_BUFFER_ONLY)
	{
		// do nothing
	}
	
	return 0;
}

size_t atrd_state_buffer_read(void *buf, size_t len, void *stream)
{
	CByteBuffer *byteBuffer = (CByteBuffer *)stream;
	
	byteBuffer->GetBytes((u8*)buf, (u32)len);
	return len;
}

size_t atrd_state_buffer_write(const void *buf, size_t len, void *stream)
{
	CByteBuffer *byteBuffer = (CByteBuffer *)stream;
	byteBuffer->PutBytes((u8*)buf, (u32)len);
	return len;
}

// sound
void atrd_sound_init()
{
	if (debugInterfaceAtari->audioChannel == NULL)
	{
		debugInterfaceAtari->audioChannel = new CAudioChannelAtari(debugInterfaceAtari);
		SND_AddChannel(debugInterfaceAtari->audioChannel);
	}
	
	// do not use it for now
	debugInterfaceAtari->audioChannel->Stop();
}

void atrd_sound_pause()
{
	debugInterfaceAtari->audioChannel->Stop();
}

void atrd_sound_resume()
{
	debugInterfaceAtari->audioChannel->Start();
}

void atrd_sound_lock()
{
	gSoundEngine->LockMutex("atrd_sound_lock");
}

void atrd_sound_unlock()
{
	gSoundEngine->UnlockMutex("atrd_sound_unlock");
}

int atrd_get_is_receive_channels_data(int pokeyNum)
{
	return viewC64->viewAtariStatePOKEY->IsVisible();
}

void atrd_pokey_channels_data(int pokeyNumber, int v1, int v2, int v3, int v4, short mix)
{
//		LOGD("atrd_pokey_channels_data: pokey#%d, %d %d %d %d | %d", pokeyNumber, v1, v2, v3, v4, mix);
	
//	if (pokeyNumber == 0)
	{
		viewC64->viewAtariStatePOKEY->AddWaveformData(pokeyNumber, v1, v2, v3, v4, mix);
	}
}


void atrd_mutex_lock()
{
	debugInterfaceAtari->LockMutex();
}

void atrd_mutex_unlock()
{
	debugInterfaceAtari->UnlockMutex();
}


