
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
#include "CViewDataMap.h"
#include "SND_SoundEngine.h"
#include "CSnapshotsManager.h"
#include "EmulatorsConfig.h"
#include "CDebugSymbols.h"
#include "CDebugMemory.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugEventsHistory.h"
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

	debugInterfaceAtari->symbols->memory->CellRead(addr, pc, -1, -1);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceAtari->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	debugInterfaceAtari->LockMutex();
	
	CDebugSymbolsSegment *segment = debugInterfaceAtari->symbols->currentSegment;
	if (segment)
	{
		u8 value = MEMORY_SafeGetByte(addr);
		CDebugBreakpointData *breakpoint = segment->breakpointsData->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_READ);
		if (breakpoint != NULL)
		{
			debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_READ, segment);
		}
	}
	
	debugInterfaceAtari->UnlockMutex();
}

void atrd_mark_atari_cell_write(uint16 addr, uint8 value)
{
	int pc = Atari800_GetPC();
	viewC64->debugInterfaceAtari->symbols->memory->CellWrite(addr, value, pc, -1, -1); //viceCurrentC64PC, vicii.raster_line, vicii.raster_cycle);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceAtari->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	debugInterfaceAtari->LockMutex();
	
	CDebugSymbolsSegment *segment = debugInterfaceAtari->symbols->currentSegment;
	if (segment)
	{
		CDebugBreakpointData *breakpoint = segment->breakpointsData->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_WRITE);
		if (breakpoint != NULL)
		{
			debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_WRITE, segment);
		}
	}
	
	debugInterfaceAtari->UnlockMutex();
}

void atrd_mark_atari_cell_execute(uint16 addr, uint8 opcode)
{
//	LOGD("atrd_mark_atari_cell_execute: %04x %02x", addr, opcode);
	viewC64->debugInterfaceAtari->symbols->memory->CellExecute(addr, opcode);
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
		CDebugBreakpointAddr *addrBreakpoint = segment->breakpointsPC->EvaluateBreakpoint(pc);
		if (addrBreakpoint != NULL)
		{
			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_SET_BACKGROUND))
			{
				// Not supported
			}
			
			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP))
			{
				debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
				segment->symbols->debugEventsHistory->CreateEventBreakpoint(addrBreakpoint, ADDR_BREAKPOINT_ACTION_STOP, segment);
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

// The async queue defers work to the emulation thread, which drains it once per
// frame in atrd_async_check(). Only the snapshot commands use it.
//
// The CPU register setters used to queue as well AND write the register straight
// away, which meant every debugger register write landed TWICE: once on the
// calling thread, and again at the next frame boundary. For a PC write that is a
// second jump the user never asked for -- set the PC while paused, resume, and one
// frame later the CPU is yanked back to the same address and re-runs whatever the
// first jump started. That is what made CTestStackAnnotation flaky: its Atari
// program had reached its final state at $1011 when the queued copy fired and
// re-ran it from $1000, so a single step landed mid-program with the wrong SP.
// (Instrumented 2026-08-27: "queued ATRD_ASYNC_SET_PC firing: PC was 1011,
// forcing 1000".)
//
// The queue was also the wrong half to keep: it holds ONE command, so setting
// several registers in a row -- exactly what CViewAtariStateCPU does -- silently
// overwrote and leaked all but the last. The setters now write once, directly.
#define ATRD_ASYNC_NO_COMMAND		0
#define ATRD_ASYNC_LOAD_SNAPSHOT	1
#define ATRD_ASYNC_SAVE_SNAPSHOT	2

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

	// Always execute pending tasks and replay input events (needed for input replay
	// and task-based joystick injection even when allowRestore=0)
	debugInterfaceAtari->ExecuteDebugInterruptTasks();

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

// Fast C-level flag for the CPU hot loop
volatile int atrd_input_tasks_flag = 0;

int atrd_has_pending_input_tasks()
{
	return atrd_input_tasks_flag;
}

int atrd_check_snapshot_restore()
{
	debugInterfaceAtari->snapshotsManager->CheckMainCpuCycle();
	debugInterfaceAtari->ExecuteDebugInterruptTasks();

	// Clear fast flag if no more work pending
	if (!debugInterfaceAtari->hasPendingCpuDebugInterruptTasks.load(std::memory_order_acquire)
		&& !debugInterfaceAtari->snapshotsManager->isReplayInputEventsEnabled
		&& !debugInterfaceAtari->snapshotsManager->isStoreInputEventsEnabled)
	{
		atrd_input_tasks_flag = 0;
	}

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

// 1 while a command is queued for the emulation thread to run at the next frame
// boundary. Exists so a test can assert that a debugger write left NOTHING
// deferred behind it: a deferred duplicate of a register write is invisible until
// it fires, up to a frame later, and by then it looks like the emulator moved on
// its own. See CTestStackAnnotation.
int atrd_is_async_command_pending(void)
{
	return atrd_async_command != ATRD_ASYNC_NO_COMMAND;
}

// The CPU register setters below write the register ONCE, here on the calling
// thread. See the note at ATRD_ASYNC_NO_COMMAND for why they no longer also queue
// the same write for the emulation thread to repeat a frame later.
void atrd_async_set_cpu_pc(int newPC)
{
	atrd_atari_set_cpu_pc(newPC);
}

void atrd_async_set_reg_a(int newRegValue)
{
	atrd_atari_set_cpu_reg_a(newRegValue);
}

void atrd_async_set_reg_x(int newRegValue)
{
	atrd_atari_set_cpu_reg_x(newRegValue);
}

void atrd_async_set_reg_y(int newRegValue)
{
	atrd_atari_set_cpu_reg_y(newRegValue);
}

void atrd_async_set_reg_p(int newRegValue)
{
	atrd_atari_set_cpu_reg_p(newRegValue);
}


void atrd_async_set_reg_s(int newRegValue)
{
	atrd_atari_set_cpu_reg_s(newRegValue);
}




void atrd_sync_load_snapshot(char *filePath)
{
	LOGD("atrd_sync_load_snapshot %s", filePath);
	if (StateSav_ReadAtariState(filePath, "rb") == FALSE)
	{
		LOGError("atrd_sync_load_snapshot: failed");
	}
	
	debugInterfaceAtari->ClearHistory();
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
	
	byteBuffer->PutU32(debugInterfaceAtari->emulationFrameCounter.load());

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
		debugInterfaceAtari->AddWaveformData(pokeyNumber, v1, v2, v3, v4, mix);
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

