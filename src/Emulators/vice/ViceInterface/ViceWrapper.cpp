
extern "C" {
#include "ViceWrapper.h"
#include "vice.h"
#include "main.h"
#include "vicii.h"
#include "viciitypes.h"
#include "machine.h"
#include "vsync.h"
#include "raster.h"
#include "videoarch.h"
#include "drivetypes.h"
#include "gcr.h"
#include "c64.h"
#include "cia.h"
#include "maincpu.h"
#include "snapshot.h"
#include "vicii-resources.h"
}

#include "CDebugInterfaceVice.h"
#include "CAudioChannelVice.h"
#include "VID_Main.h"
#include "SND_Main.h"
#include "SYS_Main.h"
#include "SYS_Types.h"
#include "C64SettingsStorage.h"
#include "SYS_CommandLine.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "CViewC64StateSID.h"
#include "CSnapshotsManager.h"
#include "CByteBuffer.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegmentC64.h"
#include "CDebugSymbolsSegmentDrive1541.h"
#include "CDebugEventsHistory.h"
#include "SND_SoundEngine.h"

volatile int c64d_debug_mode = DEBUGGER_MODE_RUNNING;

int c64d_patch_kernal_fast_boot_flag = 0;
int c64d_setting_run_sid_when_in_warp = 1;

int c64d_setting_run_sid_emulation = 1;

int c64d_skip_drawing_sprites = 0;

volatile int c64d_start_frame_for_snapshots_manager = 0;
volatile unsigned int c64d_maincpu_previous_instruction_clk = 0;
volatile unsigned int c64d_maincpu_previous2_instruction_clk = 0;

uint16 viceCurrentC64PC;
uint16 viceCurrentDiskPC[4];

extern "C" {
extern int c64d_profiler_is_active;
extern FILE *c64d_profiler_file_out;

BYTE c64d_peek_c64(WORD addr);
void c64d_mem_write_c64_no_mark(unsigned int addr, unsigned char value);
void c64d_get_vic_simple_state(struct C64StateVIC *simpleStateVic);
}


void ViceWrapperInit(CDebugInterfaceVice *debugInterface)
{
	LOGM("ViceWrapperInit");
	
	debugInterfaceVice = debugInterface;
	
	viceCurrentC64PC = 0;
	viceCurrentDiskPC[0] = 0;
	viceCurrentDiskPC[1] = 0;
	viceCurrentDiskPC[2] = 0;
	viceCurrentDiskPC[3] = 0;
}

void c64d_sound_init()
{
	if (debugInterfaceVice->audioChannel == NULL)
	{
		debugInterfaceVice->audioChannel = new CAudioChannelVice(debugInterfaceVice);
		SND_AddChannel(debugInterfaceVice->audioChannel);
	}
	debugInterfaceVice->audioChannel->Start();
}

void c64d_sound_pause()
{
	debugInterfaceVice->audioChannel->Stop();
}

void c64d_sound_resume()
{
	debugInterfaceVice->audioChannel->Start();
}

void mt_SYS_FatalExit(char *text)
{
	SYS_FatalExit(text);
}

unsigned long mt_SYS_GetCurrentTimeInMillis()
{
	unsigned long t = SYS_GetCurrentTimeInMillis();
	return t;
}

void mt_SYS_Sleep(unsigned long milliseconds)
{
	//LOGD("mt_SYS_Sleep: %d", milliseconds);
	SYS_Sleep(milliseconds);
}

bool c64dSkipBogusPageOffsetReadOnSTA = false;

void c64d_mark_c64_cell_read(uint16 addr)
{
//	LOGD("c64d_mark_c64_cell_read=%04x", addr);
	
	bool isBogusPageOffsetRead = false;
	if (c64dSkipBogusPageOffsetReadOnSTA)
	{
		int pc = viceCurrentC64PC;
//		LOGD("c64d_mark_c64_cell_read pc=%04x", pc);
		u8 opcode = c64d_peek_c64(pc);
		if (opcode == 0x9D || opcode == 0x95 || opcode == 0x99 || opcode == 0x81 || opcode == 0x91
			|| opcode == 0x94 || opcode == 0x96)
		{
			isBogusPageOffsetRead = true;
//			LOGD("...bogus page read=true");
		}

	}
	
	if (!isBogusPageOffsetRead)
		viewC64->viewC64MemoryMap->CellRead(addr, viceCurrentC64PC, vicii.raster_line, vicii.raster_cycle);

	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	if (!isBogusPageOffsetRead)
	{
		debugInterfaceVice->LockMutex();
		
		CDebugSymbolsSegment *segment = debugInterfaceVice->symbols->currentSegment;
		if (segment)
		{
			u8 value = c64d_peek_c64(addr);
			CBreakpointMemory *breakpoint = segment->breakpointsMemory->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_READ);
			if (breakpoint != NULL)
			{
				debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
				segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_READ, segment);
			}
		}
		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_mark_c64_cell_write(uint16 addr, uint8 value)
{
	viewC64->viewC64MemoryMap->CellWrite(addr, value, viceCurrentC64PC, vicii.raster_line, vicii.raster_cycle);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	debugInterfaceVice->LockMutex();
	
	CDebugSymbolsSegment *segment = debugInterfaceVice->symbols->currentSegment;
	if (segment)
	{
		CBreakpointMemory *breakpoint = segment->breakpointsMemory->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_WRITE);
		if (breakpoint != NULL)
		{
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_WRITE, segment);
		}
	}
	debugInterfaceVice->UnlockMutex();
}

void c64d_mark_c64_cell_execute(uint16 addr, uint8 opcode)
{
	viewC64->viewC64MemoryMap->CellExecute(addr, opcode);
}

extern "C" {
	uint8 c64d_peek_drive(int driveNum, uint16 addr);
}

void c64d_mark_disk_cell_read(uint16 addr)
{
//	LOGD("c64d_mark_disk_cell_read: %04x", addr);
	viewC64->viewDrive1541MemoryMap->CellRead(addr, viceCurrentDiskPC[0], -1, -1);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	
	if (segment && segment->breakOnMemory)
	{
		debugInterfaceVice->LockMutex();
	
		u8 value = c64d_peek_drive(segment->driveNum, addr);

		CBreakpointMemory *breakpoint = segment->breakpointsMemory->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_READ);
		if (breakpoint)
		{
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_READ, segment);
		}
		
		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_mark_disk_cell_write(uint16 addr, uint8 value)
{
//	debugInterfaceVice->MarkDrive1541CellWrite(addr, value);
	viewC64->viewDrive1541MemoryMap->CellWrite(addr, value, viceCurrentDiskPC[0], -1, -1);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	
	if (segment && segment->breakOnMemory)
	{
		debugInterfaceVice->LockMutex();
		
		CBreakpointMemory *breakpoint = segment->breakpointsMemory->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_WRITE);
		if (breakpoint)
		{
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_WRITE, segment);
		}
		
		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_mark_disk_cell_execute(uint16 addr, uint8 opcode)
{
	//LOGD("c64d_mark_disk_cell_execute: %04x %02x", addr, opcode);
	viewC64->viewDrive1541MemoryMap->CellExecute(addr, opcode);
}


void c64d_display_speed(float speed, float frame_rate)
{
	debugInterfaceVice->emulationSpeed = speed;
	debugInterfaceVice->emulationFrameRate = frame_rate;
}

void c64d_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2)
{
	//LOGD("c64d_display_drive_led: %d: %d %d", drive_number, pwm1, led_pwm2);
	
	debugInterfaceVice->ledGreenPwm[drive_number] = (float)pwm1 / 1000.0f;
	debugInterfaceVice->ledRedPwm[drive_number] = (float)led_pwm2 / 1000.0f;
}

void c64d_show_message(char *message)
{
	viewC64->ShowMessageInfo(message);
}

// C64 frodo color palette (more realistic looking colors)
uint8 c64d_palette_red[16] = {
	0x00, 0xff, 0x99, 0x00, 0xcc, 0x44, 0x11, 0xff, 0xaa, 0x66, 0xff, 0x40, 0x80, 0x66, 0x77, 0xc0
};

uint8 c64d_palette_green[16] = {
	0x00, 0xff, 0x00, 0xff, 0x00, 0xcc, 0x00, 0xff, 0x55, 0x33, 0x66, 0x40, 0x80, 0xff, 0x77, 0xc0
};

uint8 c64d_palette_blue[16] = {
	0x00, 0xff, 0x00, 0xcc, 0xcc, 0x44, 0x99, 0x00, 0x00, 0x00, 0x66, 0x40, 0x80, 0x66, 0xff, 0xc0
};

float c64d_float_palette_red[16];
float c64d_float_palette_green[16];
float c64d_float_palette_blue[16];

void c64d_set_palette(uint8 *palette)
{
	int j = 0;
	for (int i = 0; i < 16; i++)
	{
		c64d_palette_red[i] = palette[j++];
		c64d_palette_green[i] = palette[j++];
		c64d_palette_blue[i] = palette[j++];
		
		c64d_float_palette_red[i] = (float)c64d_palette_red[i] / 255.0f;
		c64d_float_palette_green[i] = (float)c64d_palette_green[i] / 255.0f;
		c64d_float_palette_blue[i] = (float)c64d_palette_blue[i] / 255.0f;
	}
}

// set VICE-style palette
void c64d_set_palette_vice(uint8 *palette)
{
	int j = 0;
	for (int i = 0; i < 16; i++)
	{
		c64d_palette_red[i] = palette[j++];
		c64d_palette_green[i] = palette[j++];
		c64d_palette_blue[i] = palette[j++];
		j++; // just ignore intensity
		
		c64d_float_palette_red[i] = (float)c64d_palette_red[i] / 255.0f;
		c64d_float_palette_green[i] = (float)c64d_palette_green[i] / 255.0f;
		c64d_float_palette_blue[i] = (float)c64d_palette_blue[i] / 255.0f;
	}
}

//
extern "C" {
extern vicii_resources_t vicii_resources;
void vicii_change_timing(machine_timing_t *machine_timing, int border_mode);
}

int c64d_get_vicii_border_mode()
{
	return vicii_resources.border_mode;
}

int c64d_set_vicii_border_mode(int borderMode)
{
	if (vicii_resources.border_mode != borderMode) {
		vicii_resources.border_mode = borderMode;
		/* this works because vicii-timing.c only handles borders in
		   viciisc. */
		vicii_change_timing(0, vicii_resources.border_mode);
	}
	return 0;
}

//

void c64d_clear_screen()
{
	debugInterfaceVice->LockRenderScreenMutex();
	
	uint8 *destScreenPtr = (uint8 *)debugInterfaceVice->screenImage->resultData;
	
	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			*destScreenPtr++ = 0x00;
			*destScreenPtr++ = 0x00;
			*destScreenPtr++ = 0x00;
			*destScreenPtr++ = 0xFF;
		}
	}
	
	uint8 *screenBuffer = vicii.raster.canvas->draw_buffer->draw_buffer;
	uint8 *srcScreenPtr = screenBuffer;
	
	for (int y = 0; y < 100; y++)
	{
		for (int x = 0; x < 384; x++)
		{
			*srcScreenPtr++ = 0x00;
		}
	}

	debugInterfaceVice->UnlockRenderScreenMutex();

}

int c64d_screen_num_skip_top_lines()
{
	int borderMode = vicii_resources.border_mode;
	
	switch(borderMode)
	{
		case VICII_NORMAL_BORDERS:
			return 16;
		case VICII_FULL_BORDERS:
			return 8;
		case VICII_DEBUG_BORDERS:
			return 0;
		default:
		case VICII_NO_BORDERS:
			return 51;
	}
	return 16;
}

void c64d_refresh_screen_no_callback()
{
//	LOGD("c64d_refresh_screen_no_callback");
	//raster_t //vicii.raster
	//struct video_canvas_s //raster->canvas
	//canvas->draw_buffer->draw_buffer
	
	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;
	
	debugInterfaceVice->LockRenderScreenMutex();

	uint8 *screenBuffer = vicii.raster.canvas->draw_buffer->draw_buffer;
	
	volatile int superSample = debugInterfaceVice->screenSupersampleFactor;
	
	if (superSample == 1)
	{
		// dest screen width is 512
		// src  screen width is vicii.raster.canvas->draw_buffer->visible_width (normal borders=384)
		//
		// skip approx 16 black top lines
		int skipTopLines = c64d_screen_num_skip_top_lines();
		int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
		int screenHeight = vicii.raster.canvas->draw_buffer->visible_height; //-skipTopLines;
		
		uint8 *srcScreenPtr = screenBuffer + (skipTopLines*screenWidth);
		uint8 *destScreenPtr = (uint8 *)debugInterfaceVice->screenImage->resultData;
		
		for (int y = 0; y < screenHeight; y++)
		{
			for (int x = 0; x < screenWidth; x++)
			{
				u8 v = *srcScreenPtr++;
				*destScreenPtr++ = c64d_palette_red[v];
				*destScreenPtr++ = c64d_palette_green[v];
				*destScreenPtr++ = c64d_palette_blue[v];
				*destScreenPtr++ = 255;
			}
			
			destScreenPtr += (512-screenWidth)*4;
		}
	}
	else
	{
		//	// dest screen width is 512
		//	// src  screen width is 384
		//
		// skip 16 top lines
		int skipTopLines = c64d_screen_num_skip_top_lines();
		int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
		int screenHeight = vicii.raster.canvas->draw_buffer->visible_height; //-skipTopLines;

		uint8 *srcScreenPtr = screenBuffer + (skipTopLines*screenWidth);
		uint8 *destScreenPtr = (uint8 *)debugInterfaceVice->screenImage->resultData;
		
		for (int y = 0; y < screenHeight; y++)
		{
			for (int j = 0; j < superSample; j++)
			{
				uint8 *pScreenPtrSrc = srcScreenPtr;
				uint8 *pScreenPtrDest = destScreenPtr;
				for (int x = 0; x < screenWidth; x++)
				{
					u8 v = *pScreenPtrSrc++;
					
					for (int i = 0; i < superSample; i++)
					{
						*pScreenPtrDest++ = c64d_palette_red[v];
						*pScreenPtrDest++ = c64d_palette_green[v];
						*pScreenPtrDest++ = c64d_palette_blue[v];
						*pScreenPtrDest++ = 255;
					}
				}
				
				destScreenPtr += (512)*superSample*4;
			}
			
			srcScreenPtr += screenWidth;
		}
	}
	
	debugInterfaceVice->UnlockRenderScreenMutex();
}

void c64d_refresh_screen()
{
//	LOGD("c64d_refresh_screen");
	c64d_refresh_screen_no_callback();
	debugInterfaceVice->DoFrame();
}

// this is called when debug is paused to refresh only part of screen
void c64d_refresh_previous_lines()
{
	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;

//	LOGD("c64d_refresh_previous_lines");
	debugInterfaceVice->LockRenderScreenMutex();
	
	int skipTopLines = c64d_screen_num_skip_top_lines();
	int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
	int screenHeight = vicii.raster.canvas->draw_buffer->visible_height; //-skipTopLines
	
	int rasterY = vicii.raster_line - skipTopLines;
	
	rasterY--;
	
	// draw previous completed raster lines
	uint8 *screenBuffer = vicii.raster.canvas->draw_buffer->draw_buffer;
	
//	LOGD("..... rasterY=%x", rasterY);
	
	for (int x = 0; x < screenWidth; x++)
	{
		for (int y = 0; y < rasterY; y++)
		{
			int offset = x + ((y+skipTopLines) * screenWidth);
			
			u8 v = screenBuffer[offset];
			
			//LOGD("r=%d g=%d b=%d", r, g, b);
			
			for (int i = 0; i < debugInterfaceVice->screenSupersampleFactor; i++)
			{
				for (int j = 0; j < debugInterfaceVice->screenSupersampleFactor; j++)
				{
					debugInterfaceVice->screenImage->SetPixelResultRGBA(x * debugInterfaceVice->screenSupersampleFactor + j,
																   y * debugInterfaceVice->screenSupersampleFactor + i,
																   c64d_palette_red[v], c64d_palette_green[v], c64d_palette_blue[v], 255);
				}
			}
		}
	}
	
	debugInterfaceVice->UnlockRenderScreenMutex();
}

void c64d_refresh_dbuf()
{
	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;

//	return;
//	LOGD("c64d_refresh_dbuf");
	int skipTopLines = c64d_screen_num_skip_top_lines();
	int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
	int screenHeight = vicii.raster.canvas->draw_buffer->visible_height; //-skipTopLines

	int rasterY = vicii.raster_line - skipTopLines;

	if (rasterY < 0 || rasterY > screenHeight)
	{
		return;
	}

	if (vicii.raster_cycle > 61)
		return;
	
	if (vicii.raster_cycle == 0 && vicii.dbuf_offset == 504)
		return;
	
	debugInterfaceVice->LockRenderScreenMutex();
	
//	LOGD(".... rasterY=%x vicii.dbuf_offset + 8=%d", rasterY, (vicii.dbuf_offset + 8));
//	LOGD(".... rasterX=%x raster_cycle=%d", vicii.raster_cycle*8, vicii.raster_cycle);

	int borderMode = debugInterfaceVice->GetViciiBorderMode();
	int lineOffset;
	
	switch(borderMode)
	{
		default:
		case VICII_NORMAL_BORDERS:
			lineOffset = 104;
			break;
		case VICII_FULL_BORDERS:
			lineOffset = 104-16;
			break;
		case VICII_DEBUG_BORDERS:
			lineOffset = 0;
			break;
		case VICII_NO_BORDERS:
			lineOffset = 104+32;
			break;
	}
	
	int maxX = 0;
	
	for (int l = 0; l < vicii.dbuf_offset; l++)	//+8
	{
		int x = l - lineOffset;
		if (x < 0 || x > screenWidth)
			continue;
		
		if (maxX < x)
			maxX = x;
		
		u8 v = vicii.dbuf[l];
		
		for (int i = 0; i < debugInterfaceVice->screenSupersampleFactor; i++)
		{
			for (int j = 0; j < debugInterfaceVice->screenSupersampleFactor; j++)
			{
				debugInterfaceVice->screenImage->SetPixelResultRGBA(x * debugInterfaceVice->screenSupersampleFactor + j,
															   rasterY * debugInterfaceVice->screenSupersampleFactor + i,
															   c64d_palette_red[v], c64d_palette_green[v], c64d_palette_blue[v], 255);
			}
		}
	}
	
//	LOGD("........ maxX=%d", maxX);

	
	debugInterfaceVice->UnlockRenderScreenMutex();
}

extern "C" {
	void cia_update_ta(cia_context_t *cia_context, CLOCK rclk);
	void cia_update_tb(cia_context_t *cia_context, CLOCK rclk);
}

void c64d_refresh_cia()
{
//	LOGD("c64d_refresh_cia");
	
	cia_update_ta(machine_context.cia1, *(machine_context.cia1->clk_ptr));
	cia_update_tb(machine_context.cia1, *(machine_context.cia1->clk_ptr));
	
	cia_update_ta(machine_context.cia2, *(machine_context.cia2->clk_ptr));
	cia_update_tb(machine_context.cia2, *(machine_context.cia2->clk_ptr));
}

void c64d_reset_counters()
{
	if (c64SettingsResetCountersOnAutoRun)
	{
		debugInterfaceVice->ResetMainCpuDebugCycleCounter();
		debugInterfaceVice->ResetEmulationFrameCounter();
	}
}

unsigned int c64d_get_frame_num()
{
	return debugInterfaceVice->GetEmulationFrameNumber();
}

//

int c64d_is_debug_on_c64()
{
	if (debugInterfaceVice->isDebugOn)
		return 1;
	
	return 0;
}

int c64d_is_debug_on_drive1541()
{
	if (debugInterfaceVice->debugOnDrive1541)
		return 1;
	
	return 0;
}

void c64d_c64_check_pc_breakpoint(uint16 pc)
{
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*) debugInterfaceVice->symbols->currentSegment;
	if (segment == NULL)
		return;
	
	if ((int)pc == segment->breakpointsPC->temporaryBreakpointPC)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
		segment->breakpointsPC->temporaryBreakpointPC = -1;
	}
	else
	{
		debugInterfaceVice->LockMutex();
		
		CBreakpointAddr *addrBreakpoint = segment->breakpointsPC->EvaluateBreakpoint(pc);
		if (addrBreakpoint != NULL)
		{
			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_SET_BACKGROUND))
			{
				// VIC can't modify two registers at once
				
				C64StateVIC vicState;
				c64d_get_vic_simple_state(&vicState);
				
				int rasterX = vicState.raster_cycle*8;
				int rasterY = vicState.raster_line;

				// outside screen (in borders)?
				if (rasterY < 0x32 || rasterY > 0xFA
					|| rasterX < 0x88 || rasterX > 0x1C7)
				{
					c64d_mem_write_c64_no_mark(0xD021, addrBreakpoint->data);
					
					// this will be the real write in this VIC cycle:
					c64d_mem_write_c64_no_mark(0xD020, addrBreakpoint->data);
				}
				else
				{
					c64d_mem_write_c64_no_mark(0xD020, addrBreakpoint->data);
					
					// this will be the real write in this VIC cycle:
					c64d_mem_write_c64_no_mark(0xD021, addrBreakpoint->data);
				}
				
				// alternatively
				// val = c64d_peek_c64(0xD021) + 1;
				// if (val == 0x10)
				//		val = 0x00;
			}

			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP))
			{
				debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
				segment->symbols->debugEventsHistory->CreateEventBreakpoint(addrBreakpoint, ADDR_BREAKPOINT_ACTION_STOP, segment);
			}
		}
		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_drive1541_check_pc_breakpoint(uint16 pc)
{
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;

	if ((int)pc == segment->breakpointsPC->temporaryBreakpointPC)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
		segment->breakpointsPC->temporaryBreakpointPC = -1;
		viceCurrentDiskPC[0] = pc;
	}
	else if (segment->breakOnPC)
	{
		debugInterfaceVice->LockMutex();
		CBreakpointAddr *addrBreakpoint = segment->breakpointsPC->EvaluateBreakpoint(pc);
		if (addrBreakpoint != NULL)
		{
			if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP))
			{
				debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
				segment->symbols->debugEventsHistory->CreateEventBreakpoint(addrBreakpoint, ADDR_BREAKPOINT_ACTION_STOP, segment);
			}
		}
		debugInterfaceVice->UnlockMutex();
	}
	
}

// copy of vic state registers for VIC Display
//vicii_cycle_state_t viciiStateForCycle[312];	//Lines: PAL 312, NTSC 263
vicii_cycle_state_t viciiStateForCycle[312][64];	//Cycles: PAL 19655, NTSC 17095

extern "C"
{
	void c64d_get_maincpu_regs(uint8 *a, uint8 *x, uint8 *y, uint8 *p, uint8 *sp, uint16 *pc,
							   uint8 *instructionCycle);
	void c64d_get_exrom_game(BYTE *exrom, BYTE *game);
	void c64d_get_ultimax_phi(BYTE *ultimax_phi1, BYTE *ultimax_phi2);
};

vicii_cycle_state_t *c64d_get_vicii_state_for_raster_cycle(int rasterLine, int rasterCycle)
{
	return &(viciiStateForCycle[rasterLine][rasterCycle]);
}

vicii_cycle_state_t *c64d_get_vicii_state_for_raster_line(int rasterLine)
{
	return &(viciiStateForCycle[rasterLine][0]);
}

extern "C" {
	BYTE c64d_peek_memory0001();
};

/* Profiler call */
void c64d_vicii_copy_state(vicii_cycle_state_t *viciiCopy)
{
	memcpy(viciiCopy->regs, vicii.regs, 64);
	
	viciiCopy->raster_line = vicii.raster_line;
	viciiCopy->raster_cycle = vicii.raster_cycle;
	
	viciiCopy->raster_irq_line = vicii.raster_irq_line;
	
	viciiCopy->vbank_phi1 = vicii.vbank_phi1;
	viciiCopy->vbank_phi2 = vicii.vbank_phi2;
	
	viciiCopy->idle_state = vicii.idle_state;
	viciiCopy->rc = vicii.rc;
	viciiCopy->vc = vicii.vc;
	viciiCopy->vcbase = vicii.vcbase;
	viciiCopy->vmli = vicii.vmli;
	
	viciiCopy->bad_line = vicii.bad_line;
	
	viciiCopy->last_read_phi1 = vicii.last_read_phi1;
	viciiCopy->sprite_dma = vicii.sprite_dma;
	viciiCopy->sprite_display_bits = vicii.sprite_display_bits;
	
	for (int i = 0; i < VICII_NUM_SPRITES; i++)
	{
		viciiCopy->sprite[i].data = vicii.sprite[i].data;
		viciiCopy->sprite[i].mc = vicii.sprite[i].mc;
		viciiCopy->sprite[i].mcbase = vicii.sprite[i].mcbase;
		viciiCopy->sprite[i].pointer = vicii.sprite[i].pointer;
		viciiCopy->sprite[i].exp_flop = vicii.sprite[i].exp_flop;
		viciiCopy->sprite[i].x = vicii.sprite[i].x;
	}
	
	
	// additional vars
	c64d_get_exrom_game(&(viciiCopy->exrom), &(viciiCopy)->game);
	c64d_get_ultimax_phi(&(viciiCopy->export_ultimax_phi1), &(viciiCopy->export_ultimax_phi2));

	viciiCopy->vaddr_mask_phi1 = vicii.vaddr_mask_phi1;
	viciiCopy->vaddr_mask_phi2 = vicii.vaddr_mask_phi2;
	viciiCopy->vaddr_offset_phi1 = vicii.vaddr_offset_phi1;
	viciiCopy->vaddr_offset_phi2 = vicii.vaddr_offset_phi2;
	viciiCopy->vaddr_chargen_mask_phi1 = vicii.vaddr_chargen_mask_phi1;
	viciiCopy->vaddr_chargen_value_phi1 = vicii.vaddr_chargen_value_phi1;
	viciiCopy->vaddr_chargen_mask_phi2 = vicii.vaddr_chargen_mask_phi2;
	viciiCopy->vaddr_chargen_value_phi2 = vicii.vaddr_chargen_value_phi2;
	
	// CPU
	c64d_get_maincpu_regs(&(viciiCopy->a), &(viciiCopy->x), &(viciiCopy->y), &(viciiCopy->processorFlags), &(viciiCopy->sp), &(viciiCopy->pc),
						  &(viciiCopy->instructionCycle));

	// TODO: DO WE STILL NEED THIS?
	viciiCopy->lastValidPC = viciiCopy->pc;
	
	//LOGD("mem01=%02x", c64d_peek_memory0001());
	viciiCopy->memory0001 = c64d_peek_memory0001();
}

void c64d_vicii_copy_state_data(vicii_cycle_state_t *viciiDest, vicii_cycle_state_t *viciiSrc)
{
	memcpy(viciiDest->regs, viciiSrc->regs, 64);
	
	viciiDest->raster_line = viciiSrc->raster_line;
	viciiDest->raster_cycle = viciiSrc->raster_cycle;
	
	viciiDest->raster_irq_line = viciiSrc->raster_irq_line;
	
	viciiDest->vbank_phi1 = viciiSrc->vbank_phi1;
	viciiDest->vbank_phi2 = viciiSrc->vbank_phi2;
	
	viciiDest->idle_state = viciiSrc->idle_state;
	viciiDest->rc = viciiSrc->rc;
	viciiDest->vc = viciiSrc->vc;
	viciiDest->vcbase = viciiSrc->vcbase;
	viciiDest->vmli = viciiSrc->vmli;
	
	viciiDest->bad_line = viciiSrc->bad_line;
	
	viciiDest->last_read_phi1 = viciiSrc->last_read_phi1;
	viciiDest->sprite_dma = viciiSrc->sprite_dma;
	viciiDest->sprite_display_bits = viciiSrc->sprite_display_bits;
	
	for (int i = 0; i < VICII_NUM_SPRITES; i++)
	{
		viciiDest->sprite[i].data = viciiSrc->sprite[i].data;
		viciiDest->sprite[i].mc = viciiSrc->sprite[i].mc;
		viciiDest->sprite[i].mcbase = viciiSrc->sprite[i].mcbase;
		viciiDest->sprite[i].pointer = viciiSrc->sprite[i].pointer;
		viciiDest->sprite[i].exp_flop = viciiSrc->sprite[i].exp_flop;
		viciiDest->sprite[i].x = viciiSrc->sprite[i].x;
	}
	
	viciiDest->exrom = viciiSrc->exrom;
	viciiDest->game = viciiSrc->game;
	
	viciiDest->export_ultimax_phi1 = viciiSrc->export_ultimax_phi1;
	viciiDest->export_ultimax_phi2 = viciiSrc->export_ultimax_phi2;
	
	
	viciiDest->vaddr_mask_phi1 = viciiSrc->vaddr_mask_phi1;
	viciiDest->vaddr_mask_phi2 = viciiSrc->vaddr_mask_phi2;
	viciiDest->vaddr_offset_phi1 = viciiSrc->vaddr_offset_phi1;
	viciiDest->vaddr_offset_phi2 = viciiSrc->vaddr_offset_phi2;
	viciiDest->vaddr_chargen_mask_phi1 = viciiSrc->vaddr_chargen_mask_phi1;
	viciiDest->vaddr_chargen_value_phi1 = viciiSrc->vaddr_chargen_value_phi1;
	viciiDest->vaddr_chargen_mask_phi2 = viciiSrc->vaddr_chargen_mask_phi2;
	viciiDest->vaddr_chargen_value_phi2 = viciiSrc->vaddr_chargen_value_phi2;
	
	// CPU
	viciiDest->a = viciiSrc->a;
	viciiDest->x = viciiSrc->x;
	viciiDest->y = viciiSrc->y;
	viciiDest->processorFlags = viciiSrc->processorFlags;
	viciiDest->sp = viciiSrc->sp;
	viciiDest->pc = viciiSrc->pc;
	viciiDest->instructionCycle = viciiSrc->instructionCycle;
	
	
	// TODO: DO WE STILL NEED THIS?
	viciiDest->lastValidPC = viciiSrc->lastValidPC;
	
	// TODO: ???
	viciiDest->memory0001 = viciiSrc->memory0001;
	
}


//uint32 viciiFrameCycleNum = 0;

// TODO: add setting in settings
uint8 c64d_vicii_record_state_mode = C64D_VICII_RECORD_MODE_EVERY_CYCLE; //C64D_VICII_RECORD_MODE_NONE;

void c64d_c64_set_vicii_record_state_mode(uint8 recordMode)
{
	c64d_vicii_record_state_mode = recordMode;
}

//unsigned int viciiFrameCycleNum = 0;

void c64d_c64_vicii_start_frame()
{
//	LOGD("c64d_c64_vicii_start_frame, viciiFrameCycleNum=%d", viciiFrameCycleNum);
	
	//viciiFrameCycleNum = 0;
	
//	LOGD("c64d_c64_vicii_start_frame: %d", vicii.start_of_frame);
	//		LOGD("*** line=%04x / cycle=%04x  start=%d", vicii.raster_line, vicii.raster_cycle, vicii.start_of_frame);
	
	// TODO: frame counter + breakpoint on defined frame
//
//
//
	c64d_start_frame_for_snapshots_manager = 1;
	debugInterfaceVice->DoVSync();
}

void c64d_c64_vicii_cycle()
{
//	LOGD("line=%04x / cycle=%04x  start=%d", vicii.raster_line, vicii.raster_cycle, vicii.start_of_frame);
//	LOGD("viciiFrameCycleNum=%5d line=%04x / cycle=%04x  start=%d", viciiFrameCycleNum, vicii.raster_line, vicii.raster_cycle, vicii.start_of_frame);
	//viciiFrameCycleNum++;

	if (c64d_vicii_record_state_mode == C64D_VICII_RECORD_MODE_EVERY_CYCLE)
	{
		// correct the raster line on start frame
		unsigned int rasterLine = vicii.raster_line;
		unsigned int rasterCycle = vicii.raster_cycle;
		
		if (vicii.start_of_frame == 1)
		{
			rasterLine = 0;
		}

		vicii_cycle_state_t *viciiCopy = &viciiStateForCycle[rasterLine][rasterCycle];
		c64d_vicii_copy_state(viciiCopy);
	}
	
	// profiler
	if (c64SettingsC64ProfilerDoVicProfile)
	{
//		LOGD("c64SettingsC64ProfilerDoVicProfile");
		if (c64d_profiler_is_active && c64d_profiler_file_out)
		{
			unsigned int frameNum = c64d_get_frame_num();
			unsigned int raster_line = vicii.raster_line;
			unsigned int raster_cycle = vicii.raster_cycle;
			unsigned int bad_line = vicii.bad_line;
			unsigned int sprite_dma = vicii.sprite_dma;
			unsigned int raster_irq_line = vicii.raster_irq_line;
			
			fprintf(c64d_profiler_file_out, "vic %u %u %d %d %d %d %d\n",
					c64d_maincpu_clk, frameNum, raster_line, raster_cycle, bad_line,
					sprite_dma, raster_irq_line);
		}
	}
	
}

void c64d_c64_vicii_start_raster_line(uint16 rasterLine)
{
	// copy VIC state
//	LOGD("c64d_c64_vicii_start_raster_line: rasterLine=%d cycle=%d", rasterLine, vicii.raster_cycle);

	if (c64d_vicii_record_state_mode == C64D_VICII_RECORD_MODE_EVERY_LINE)
	{
		vicii_cycle_state_t *viciiCopy = &viciiStateForCycle[rasterLine][0];
		c64d_vicii_copy_state(viciiCopy);
	}
	
	c64d_c64_check_raster_breakpoint(rasterLine);
}

// note that changes in this will require changes in c64d_snapshot_write_module: snapshot version and hardcoded buffer length
#define VICII_STATE_BUFFER_LENGTH	251

void c64d_c64_vicii_store_state_to_bytebuffer(vicii_cycle_state_t *viciiState, CByteBuffer *byteBuffer)
{
	byteBuffer->PutBytes(viciiState->regs, 64);
	byteBuffer->PutU16(viciiState->raster_line);
	byteBuffer->PutU16(viciiState->raster_cycle);
	byteBuffer->PutU16(viciiState->raster_irq_line);
	byteBuffer->putInt(viciiState->vbank_phi1);
	byteBuffer->putInt(viciiState->vbank_phi2);
	byteBuffer->putInt(viciiState->idle_state);
	byteBuffer->putInt(viciiState->rc);
	byteBuffer->putInt(viciiState->vc);
	byteBuffer->putInt(viciiState->vcbase);
	byteBuffer->putInt(viciiState->vmli);
	byteBuffer->putInt(viciiState->bad_line);
	
	byteBuffer->PutU8(viciiState->last_read_phi1);
	byteBuffer->PutU8(viciiState->sprite_dma);
	byteBuffer->putInt(viciiState->sprite_display_bits);
	
	for (int i = 0; i < VICII_NUM_SPRITES; i++)
	{
		byteBuffer->PutU16(viciiState->sprite[i].data);
		byteBuffer->PutU8(viciiState->sprite[i].mc);
		byteBuffer->PutU8(viciiState->sprite[i].mcbase);
		byteBuffer->PutU8(viciiState->sprite[i].pointer);
		byteBuffer->PutU8(viciiState->sprite[i].mc);
		byteBuffer->putInt(viciiState->sprite[i].exp_flop);
		byteBuffer->putInt(viciiState->sprite[i].x);
	}
	
	byteBuffer->PutU8(viciiState->exrom);
	byteBuffer->PutU8(viciiState->game);
	byteBuffer->PutU8(viciiState->export_ultimax_phi1);
	byteBuffer->PutU8(viciiState->export_ultimax_phi2);
	
	byteBuffer->PutU16(viciiState->vaddr_mask_phi1);
	byteBuffer->PutU16(viciiState->vaddr_mask_phi2);
	byteBuffer->PutU16(viciiState->vaddr_offset_phi1);
	byteBuffer->PutU16(viciiState->vaddr_offset_phi2);

	byteBuffer->PutU16(viciiState->vaddr_chargen_mask_phi1);
	byteBuffer->PutU16(viciiState->vaddr_chargen_value_phi1);
	byteBuffer->PutU16(viciiState->vaddr_chargen_mask_phi2);
	byteBuffer->PutU16(viciiState->vaddr_chargen_value_phi2);

	byteBuffer->PutU8(viciiState->a);
	byteBuffer->PutU8(viciiState->x);
	byteBuffer->PutU8(viciiState->y);
	byteBuffer->PutU8(viciiState->processorFlags);
	byteBuffer->PutU8(viciiState->sp);

	byteBuffer->PutU16(viciiState->pc);
	byteBuffer->PutU16(viciiState->lastValidPC);

	byteBuffer->PutU8(viciiState->instructionCycle);
	byteBuffer->PutU8(viciiState->memory0001);
}

void c64d_c64_vicii_restore_state_from_bytebuffer(vicii_cycle_state_t *viciiState, CByteBuffer *byteBuffer)
{
	byteBuffer->GetBytes(viciiState->regs, 64);
	viciiState->raster_line = byteBuffer->GetU16();
	viciiState->raster_cycle = byteBuffer->GetU16();
	viciiState->raster_irq_line = byteBuffer->GetU16();
	viciiState->vbank_phi1 = byteBuffer->getInt();
	viciiState->vbank_phi2 = byteBuffer->getInt();
	viciiState->idle_state = byteBuffer->getInt();
	viciiState->rc = byteBuffer->getInt();
	viciiState->vc = byteBuffer->getInt();
	viciiState->vcbase = byteBuffer->getInt();
	viciiState->vmli = byteBuffer->getInt();
	viciiState->bad_line = byteBuffer->getInt();
	
	viciiState->last_read_phi1 = byteBuffer->GetU8();
	viciiState->sprite_dma = byteBuffer->GetU8();
	viciiState->sprite_display_bits = byteBuffer->getInt();
	
	for (int i = 0; i < VICII_NUM_SPRITES; i++)
	{
		viciiState->sprite[i].data = byteBuffer->GetU16();
		viciiState->sprite[i].mc = byteBuffer->GetU8();
		viciiState->sprite[i].mcbase = byteBuffer->GetU8();
		viciiState->sprite[i].pointer = byteBuffer->GetU8();
		viciiState->sprite[i].mc = byteBuffer->GetU8();
		viciiState->sprite[i].exp_flop = byteBuffer->getInt();
		viciiState->sprite[i].x = byteBuffer->getInt();
	}
	
	viciiState->exrom = byteBuffer->GetU8();
	viciiState->game = byteBuffer->GetU8();
	viciiState->export_ultimax_phi1 = byteBuffer->GetU8();
	viciiState->export_ultimax_phi2 = byteBuffer->GetU8();
	
	viciiState->vaddr_mask_phi1 = byteBuffer->GetU16();
	viciiState->vaddr_mask_phi2 = byteBuffer->GetU16();
	viciiState->vaddr_offset_phi1 = byteBuffer->GetU16();
	viciiState->vaddr_offset_phi2 = byteBuffer->GetU16();
	
	viciiState->vaddr_chargen_mask_phi1 = byteBuffer->GetU16();
	viciiState->vaddr_chargen_value_phi1 = byteBuffer->GetU16();
	viciiState->vaddr_chargen_mask_phi2 = byteBuffer->GetU16();
	viciiState->vaddr_chargen_value_phi2 = byteBuffer->GetU16();
	
	viciiState->a = byteBuffer->GetU8();
	viciiState->x = byteBuffer->GetU8();
	viciiState->y = byteBuffer->GetU8();
	viciiState->processorFlags = byteBuffer->GetU8();
	viciiState->sp = byteBuffer->GetU8();
	
	viciiState->pc = byteBuffer->GetU16();
	viciiState->lastValidPC = byteBuffer->GetU16();
	
	viciiState->instructionCycle = byteBuffer->GetU8();
	viciiState->memory0001 = byteBuffer->GetU8();
}
//


void c64d_c64_check_raster_breakpoint(uint16 rasterLine)
{
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

//	LOGD("c64d_c64_check_raster_breakpoint rasterLine=%d", rasterLine);
	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64 *) debugInterfaceVice->symbols->currentSegment;
	debugInterfaceVice->LockMutex();
	if (segment && segment->breakOnRaster)
	{
		CBreakpointAddr *breakpoint = segment->breakpointsRasterLine->EvaluateBreakpoint(rasterLine);
		if (breakpoint != NULL)
		{
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, ADDR_BREAKPOINT_ACTION_STOP_ON_RASTER, segment);
			//			TheCPU->lastValidPC = TheCPU->pc;
		}
	}
	debugInterfaceVice->UnlockMutex();
}

int c64d_drive1541_is_checking_irq_breakpoints_enabled()
{
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return 0;

	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	if (segment->breakOnDrive1541IrqIEC || segment->breakOnDrive1541IrqVIA1 || segment->breakOnDrive1541IrqVIA2)
		return 1;
	
	return 0;
}

void c64d_drive1541_check_irqiec_breakpoint()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	if (segment && segment->breakOnDrive1541IrqIEC)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}

void c64d_drive1541_check_irqvia1_breakpoint()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	if (segment && segment->breakOnDrive1541IrqVIA1)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}


void c64d_drive1541_check_irqvia2_breakpoint()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	if (segment && segment->breakOnDrive1541IrqVIA2)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}


int c64d_c64_is_checking_irq_breakpoints_enabled()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return 0;

	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*) debugInterfaceVice->symbols->currentSegment;
	if (segment)
	{
		if (segment->breakOnC64IrqVIC || segment->breakOnC64IrqCIA || segment->breakOnC64IrqNMI)
			return 1;
	}
	
	return 0;
}

void c64d_c64_check_irqvic_breakpoint()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*) debugInterfaceVice->symbols->currentSegment;
	if (segment && segment->breakOnC64IrqVIC)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED); //C64_DEBUG_RUN_ONE_INSTRUCTION);
	}
}

void c64d_c64_check_irqcia_breakpoint(int ciaNum)
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*) debugInterfaceVice->symbols->currentSegment;
	if (segment && segment->breakOnC64IrqCIA)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}

void c64d_c64_check_irqnmi_breakpoint()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*) debugInterfaceVice->symbols->currentSegment;
	if (segment && segment->breakOnC64IrqNMI)
	{
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}

void c64d_debug_pause_check(int allowRestore)
{
	if (allowRestore)
	{
		if (c64d_check_cpu_snapshot_manager_restore() == 0)
		{
			if (debugInterfaceVice->sidDataToRestore)
			{
				debugInterfaceVice->sidDataToRestore->RestoreSids();
				debugInterfaceVice->sidDataToRestore = NULL;
			}
		}
	}
	else
	{
		if (c64d_is_performing_snapshot_restore())
			return;
	}
	
	if (c64d_debug_mode == DEBUGGER_MODE_PAUSED)
	{		
		c64d_refresh_previous_lines();
		c64d_refresh_dbuf();
		c64d_refresh_cia();
		
		while (c64d_debug_mode == DEBUGGER_MODE_PAUSED)
		{
			if (allowRestore)
			{
				c64d_check_cpu_snapshot_manager_restore();
			}
			else
			{
				if (c64d_is_performing_snapshot_restore())
				{
					vsync_do_vsync(vicii.raster.canvas, 0, 1);
					return;
				}
			}
			
			if (debugInterfaceVice->sidDataToRestore)
			{
				debugInterfaceVice->sidDataToRestore->RestoreSids();
				debugInterfaceVice->sidDataToRestore = NULL;
			}

			vsync_do_vsync(vicii.raster.canvas, 0, 1);
			//mt_SYS_Sleep(50);
		}
	}
}

int c64d_is_performing_snapshot_restore()
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
	{
		return 1;
	}
	return 0;
}
		
int c64d_check_snapshot_restore()
{
	debugInterfaceVice->snapshotsManager->CheckMainCpuCycle();
	
	if (debugInterfaceVice->snapshotsManager->CheckSnapshotRestore())
	{
		return 1;
	}
	
	return 0;
}

void c64d_check_snapshot_interval()
{
	if (c64d_start_frame_for_snapshots_manager)
	{
//		LOGD("c64d_check_snapshot_interval: %d", c64d_start_frame_for_snapshots_manager);
		c64d_start_frame_for_snapshots_manager = 0;
		debugInterfaceVice->snapshotsManager->CheckSnapshotInterval();
	}
}

char c64d_uimon_buf[1024] = { 0 };
int c64d_uimon_bufpos = 0;

void c64d_uimon_print(char *p)
{
	char *c = (char*)p;
	for (int i = 0; i < strlen(p); i++)
	{
		if (*c == '\n')
		{
			c64d_uimon_buf[c64d_uimon_bufpos] = 0;
			
			if (debugInterfaceVice->codeMonitorCallback != NULL)
			{
				CSlrString *str = new CSlrString(c64d_uimon_buf);
				debugInterfaceVice->codeMonitorCallback->CodeMonitorCallbackPrintLine(str);
			}
			else
			{
				LOGError("c64d_uimon_print_line: codeMonitorCallback is NULL, line=%s", p);
			}
			
			c64d_uimon_bufpos = 0;
			continue;
		}
		c64d_uimon_buf[c64d_uimon_bufpos] = *c;
		c64d_uimon_bufpos++;
		c++;
	}
}

void c64d_uimon_print_line(char *p)
{
	LOGD("c64d_uimon_print_line: p=%s", p);
	
	c64d_uimon_print(p);
	
	if (c64d_uimon_bufpos != 0)
	{
		if (debugInterfaceVice->codeMonitorCallback != NULL)
		{
			c64d_uimon_buf[c64d_uimon_bufpos] = 0;

			CSlrString *str = new CSlrString(c64d_uimon_buf);
			debugInterfaceVice->codeMonitorCallback->CodeMonitorCallbackPrintLine(str);
		}
		else
		{
			LOGError("c64d_uimon_print_line: codeMonitorCallback is NULL, line=%s", p);
		}
		
		c64d_uimon_bufpos = 0;
	}
}

void c64d_lock_mutex()
{
	debugInterfaceVice->LockIoMutex();
}

void c64d_unlock_mutex()
{
	debugInterfaceVice->UnlockIoMutex();
}

extern "C" {
	void c64d_lock_sound_mutex(char *whoLocked)
	{
		gSoundEngine->LockMutex(whoLocked);
	}

	void c64d_unlock_sound_mutex(char *whoLocked)
	{
		gSoundEngine->UnlockMutex(whoLocked);
	}
}

////////////

// sid
int c64d_is_receive_channels_data[C64_MAX_NUM_SIDS] = { 0, 0, 0 };

void c64d_sid_receive_channels_data(int sidNum, int isOn)
{
	c64d_is_receive_channels_data[sidNum] = isOn;
}

void c64d_sid_channels_data(int sidNumber, int v1, int v2, int v3, short mix)
{
//	LOGD("c64d_sid_channels_data: sid#%d, %d %d %d %d", sidNumber, v1, v2, v3, mix);
	
	debugInterfaceVice->AddWaveformData(sidNumber, v1, v2, v3, mix);
}

// is drive dirty for snapshot interval?
int c64d_is_drive_dirty_for_snapshot()
{
//	LOGD("c64d_is_drive_dirty_for_snapshot:");
	for (int dnr = 0; dnr < DRIVE_NUM; dnr++)
	{
		drive_s *drive = drive_context[dnr]->drive;
//		LOGD(".... dnr=%d drive=%x GCR=%d P64=%d", dnr, drive, drive->GCR_dirty_track_for_snapshot, drive->P64_dirty_for_snapshot);
		if (drive->GCR_dirty_track_for_snapshot)
		{
			return 1;
		}
		if (drive->P64_dirty_for_snapshot)
		{
			return 1;
		}
	}
	
	return 0;
}

void c64d_clear_drive_dirty_for_snapshot()
{
	for (int dnr = 0; dnr < DRIVE_NUM; dnr++)
	{
		drive_s *drive = drive_context[dnr]->drive;
		drive->GCR_dirty_track_for_snapshot = 0;
		drive->P64_dirty_for_snapshot = 0;
	}
}

int c64d_is_drive_dirty_and_needs_refresh(int driveNum)
{
	drive_s *drive = drive_context[driveNum]->drive;
	if (drive->GCR_dirty_track_needs_refresh)
	{
		return 1;
	}
	if (drive->P64_dirty_needs_refresh)
	{
		return 1;
	}
	return 0;
}

void c64d_clear_drive_dirty_needs_refresh_flag(int driveNum)
{
	drive_s *drive = drive_context[driveNum]->drive;
	drive->GCR_dirty_track_needs_refresh = 0;
	drive->P64_dirty_needs_refresh = 0;
}

//
#define C64D_SNAPSHOT_VER_MAJOR   0
#define C64D_SNAPSHOT_VER_MINOR   1

bool c64d_store_vicii_state_with_snapshot = 0;

extern "C" {
	
int c64d_snapshot_write_module(snapshot_t *s, int store_screen)
{
	snapshot_module_t *m;
	
	m = snapshot_module_create(s, "DEBUGGER", C64D_SNAPSHOT_VER_MAJOR, C64D_SNAPSHOT_VER_MINOR);
 
	if (m == NULL) {
		return -1;
	}
	
	if (SMW_DW(m, c64d_maincpu_clk) < 0) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (SMW_DW(m, debugInterfaceVice->emulationFrameCounter) < 0) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (SMW_B(m, store_screen) < 0) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (store_screen)
	{
		// store screen data
		WORD screenHeight = debugInterfaceVice->GetScreenSizeY();
		if (SMW_W(m, screenHeight) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		
		if (SMW_BA(m, vicii.raster.canvas->draw_buffer->draw_buffer, 384 * screenHeight) < 0) {
			snapshot_module_close(m);
			return -1;
		}
	}
	
	// 5MB...
	if (c64d_store_vicii_state_with_snapshot)
	{
		const int s = 1;
		if (SMW_B(m, s) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		
		LOGD("sizeof(vicii_cycle_state_t)=%d total %d", sizeof(vicii_cycle_state_t), sizeof(vicii_cycle_state_t) * 312*64);
		// store vicii_cycle_state_t viciiStateForCycle[312][64];	//Cycles: PAL 19655, NTSC 17095
		u8 *viciiStateForCycleBuffer = (u8 *)viciiStateForCycle;
		if (SMW_BA(m, viciiStateForCycleBuffer, sizeof(vicii_cycle_state_t) * 312*64) < 0) {
			snapshot_module_close(m);
			return -1;
		}
	}
	else
	{
		const int s = 0;
		if (SMW_B(m, s) < 0) {
			snapshot_module_close(m);
			return -1;
		}
	}

	snapshot_module_close(m);
	
	return 0;
}

int c64d_snapshot_read_module(snapshot_t *s)
{
	LOGD("c64d_snapshot_read_module");
	BYTE major_version, minor_version;
	snapshot_module_t *m;
	
	m = snapshot_module_open(s, "DEBUGGER", &major_version, &minor_version);
	if (m == NULL) {
		return 0;	// no debugger module, skip it
	}
	
	if (major_version != C64D_SNAPSHOT_VER_MAJOR || minor_version > C64D_SNAPSHOT_VER_MINOR) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (SMR_DW_UINT(m, (unsigned int *)&c64d_maincpu_clk) < 0) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (SMR_DW_UINT(m, &(debugInterfaceVice->emulationFrameCounter)) < 0) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (minor_version > 0)
	{
		// restore screen data
		BYTE restore_screen;
		if (SMR_B(m, &restore_screen) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		
		if (restore_screen)
		{
			WORD screenHeight = -1;
			if (SMR_W(m, &screenHeight) < 0)
			{
				snapshot_module_close(m);
				return -1;
			}
			
			if (SMR_BA(m, vicii.raster.canvas->draw_buffer->draw_buffer, 384 * screenHeight) < 0)
			{
				snapshot_module_close(m);
				return -1;
			}
			
			debugInterfaceVice->LockRenderScreenMutex();
			c64d_refresh_screen_no_callback();
			debugInterfaceVice->UnlockRenderScreenMutex();
		}
		
		// restore vicii states?
		BYTE restore_vicii_states;
		if (SMR_B(m, &restore_vicii_states) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		
		if (restore_vicii_states)
		{
			// store vicii_cycle_state_t viciiStateForCycle[312][64];	//Cycles: PAL 19655, NTSC 17095
			u8 *viciiStateForCycleBuffer = (u8 *)viciiStateForCycle;
			if (SMR_BA(m, viciiStateForCycleBuffer, sizeof(vicii_cycle_state_t) * 312*64) < 0) {
				snapshot_module_close(m);
				return -1;
			}
		}
	}

	snapshot_module_close(m);
	return 0;
}

unsigned int c64d_get_vice_maincpu_clk()
{
//	LOGD("c64d_get_vice_maincpu_clk: %d", maincpu_clk);
	return maincpu_clk;
}

unsigned int c64d_get_vice_maincpu_current_instruction_clk()
{
	return c64d_maincpu_current_instruction_clk;
}


// extern "C"
}
