
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
#include "drive.h"
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
#include <atomic>
#include "C64SettingsStorage.h"
#include "SYS_CommandLine.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "CViewC64StateSID.h"
#include "CSnapshotsManager.h"
#include "CByteBuffer.h"
#include "CDebugSymbolsC64.h"
#include "CDebugSymbolsSegmentC64.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsSegmentDrive1541.h"
#include "CDataAdapterViceDrive1541DiskContents.h"
#include "CDebugMemory.h"
#include "CDebugEventsHistory.h"
#include "SND_SoundEngine.h"

std::atomic<int> c64d_debug_mode{DEBUGGER_MODE_RUNNING};
static int pauseRefreshDone = 0;  // Flag to ensure refresh happens only once when pausing

// Diagnostic: track who last changed c64d_debug_mode and detect unexpected transitions
const char *c64d_debug_mode_last_setter = "init";
static int c64d_debug_mode_prev_value = DEBUGGER_MODE_RUNNING;

void c64d_debug_mode_trace(int newMode, const char *setter)
{
	int prev = c64d_debug_mode_prev_value;
	c64d_debug_mode_prev_value = newMode;
	c64d_debug_mode_last_setter = setter;

	// Log all transitions to help diagnose stepping hangs
	if (prev != newMode)
	{
		LOGD("c64d_debug_mode: %d -> %d [%s] (clk=%d)", prev, newMode, setter, (int)c64d_maincpu_clk);
	}
}

// C-compatible accessors for c64d_debug_mode (std::atomic<int>)
// These must be used from C code instead of direct variable access
extern "C" int c64d_debug_mode_get(void)
{
	return c64d_debug_mode.load(std::memory_order_acquire);
}

extern "C" void c64d_debug_mode_store(int newMode)
{
	c64d_debug_mode.store(newMode, std::memory_order_release);
}

int c64d_patch_kernal_fast_boot_flag = 0;
int c64d_setting_run_sid_when_in_warp = 1;

int c64d_setting_run_sid_emulation = 1;

int c64d_skip_drawing_sprites = 0;

// I/O access tracking globals
int c64d_cia1_register_written = -1;
int c64d_cia1_register_read = -1;
uint8 c64d_cia1_write_value = 0;
uint8 c64d_cia1_read_value = 0;
int c64d_cia2_register_written = -1;
int c64d_cia2_register_read = -1;
uint8 c64d_cia2_write_value = 0;
uint8 c64d_cia2_read_value = 0;
int c64d_sid_register_written = -1;
int c64d_sid_register_read = -1;
uint8 c64d_sid_write_value = 0;
uint8 c64d_sid_read_value = 0;

// Memory access tracking globals
uint8 c64d_mem_access_watch[65536] = { 0 };
int c64d_mem_access_addr = -1;
uint8 c64d_mem_access_value = 0;
uint8 c64d_mem_access_is_write = 0;

volatile int c64d_start_frame_for_snapshots_manager = 0;
volatile CLOCK c64d_maincpu_previous_instruction_clk = 0;
volatile CLOCK c64d_maincpu_previous2_instruction_clk = 0;

uint16 viceCurrentC64PC;
uint16 viceCurrentDiskPC[4];

// Stack annotation pointers (set by CDebugInterfaceVice to point into CStackAnnotationData arrays)
uint8  *c64d_main_cpu_stack_entry_types = NULL;
uint8  *c64d_main_cpu_stack_irq_sources = NULL;
uint16 *c64d_main_cpu_stack_origin_pc = NULL;
uint8  *c64d_drive_cpu_stack_entry_types = NULL;
uint8  *c64d_drive_cpu_stack_irq_sources = NULL;
uint16 *c64d_drive_cpu_stack_origin_pc = NULL;

extern "C" {
extern int c64d_profiler_is_active;
extern FILE *c64d_profiler_file_out;

uint8_t c64d_peek_c64(uint16_t addr);
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

	// Detect bogus page-offset reads (dummy reads on indexed store instructions)
	// Always check this for mem_access watch filtering, regardless of the SkipBogus setting
	bool isBogusPageOffsetRead = false;
	{
		int pc = viceCurrentC64PC;
		u8 opcode = c64d_peek_c64(pc);
		if (opcode == 0x9D || opcode == 0x95 || opcode == 0x99 || opcode == 0x81 || opcode == 0x91
			|| opcode == 0x94 || opcode == 0x96)
		{
			isBogusPageOffsetRead = true;
		}
	}

	// Cache the peek result for addr so it can be reused between the watch block
	// and the breakpoint block without calling c64d_peek_c64(addr) twice.
	u8 readValue = 0;
	bool valueRead = false;

	// Memory access watch tracking: always skip bogus reads so the VIC editor
	// layer only marks cycles with real data accesses, not dummy page-offset reads
	if (c64d_mem_access_watch[addr] && !isBogusPageOffsetRead)
	{
		c64d_mem_access_addr = addr;
		readValue = c64d_peek_c64(addr); valueRead = true;
		c64d_mem_access_value = readValue;
		c64d_mem_access_is_write = 0;
	}

	// CellRead and breakpoint behavior: unchanged, gated by c64dSkipBogusPageOffsetReadOnSTA
	if (!isBogusPageOffsetRead || !c64dSkipBogusPageOffsetReadOnSTA)
	{
		debugInterfaceVice->symbols->memory->CellRead(addr, viceCurrentC64PC, vicii.raster_line, vicii.raster_cycle);
	}

	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	// Skip mutex + breakpoint evaluation when no data breakpoints exist (common case).
	// The map check is O(1) and avoids the mutex lock/unlock overhead on every memory access.
	CDebugSymbolsSegment *segment = debugInterfaceVice->symbols->currentSegment;
	if (segment && !segment->breakpointsData->breakpoints.empty())
	{
		if (!isBogusPageOffsetRead || !c64dSkipBogusPageOffsetReadOnSTA)
		{
			debugInterfaceVice->LockMutex();

			if (!valueRead) { readValue = c64d_peek_c64(addr); }
			CDebugBreakpointData *breakpoint = segment->breakpointsData->EvaluateBreakpoint(addr, readValue, MEMORY_BREAKPOINT_ACCESS_READ);
			if (breakpoint != NULL)
			{
				LOGD("DIAG: mem READ breakpoint fired at addr=%04x during mode=%d", addr, c64d_debug_mode.load(std::memory_order_relaxed));
				c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "mem-read-bp");
				debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
				segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_READ, segment);
			}

			debugInterfaceVice->UnlockMutex();
		}
	}
}

void c64d_mark_c64_cell_write(uint16 addr, uint8 value)
{
	if (c64d_mem_access_watch[addr])
	{
		c64d_mem_access_addr = addr;
		c64d_mem_access_value = value;
		c64d_mem_access_is_write = 1;
	}

	debugInterfaceVice->symbols->memory->CellWrite(addr, value, viceCurrentC64PC, vicii.raster_line, vicii.raster_cycle);

	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	// Skip mutex + breakpoint evaluation when no data breakpoints exist (common case)
	CDebugSymbolsSegment *segment = debugInterfaceVice->symbols->currentSegment;
	if (segment && !segment->breakpointsData->breakpoints.empty())
	{
		debugInterfaceVice->LockMutex();

		CDebugBreakpointData *breakpoint = segment->breakpointsData->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_WRITE);
		if (breakpoint != NULL)
		{
			LOGD("DIAG: mem WRITE breakpoint fired at addr=%04x during mode=%d", addr, c64d_debug_mode.load(std::memory_order_relaxed));
			c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "mem-write-bp");
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_WRITE, segment);
		}

		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_mark_c64_cell_execute(uint16 addr, uint8 opcode)
{
	debugInterfaceVice->symbols->memory->CellExecute(addr, opcode);
}

extern "C" {
	uint8 c64d_peek_drive(int driveNum, uint16 addr);
}

void c64d_mark_drive1541_cell_read(uint16 addr)
{
//	LOGD("c64d_mark_drive1541_cell_read: %04x", addr);
	debugInterfaceVice->symbolsDrive1541->memory->CellRead(addr, viceCurrentDiskPC[0], -1, -1);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	
	if (segment && segment->breakOnMemory
		&& !segment->breakpointsData->breakpoints.empty())
	{
		debugInterfaceVice->LockMutex();

		u8 value = c64d_peek_drive(segment->driveNum, addr);

		CDebugBreakpointData *breakpoint = segment->breakpointsData->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_READ);
		if (breakpoint)
		{
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_READ, segment);
		}
		
		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_mark_drive1541_cell_write(uint16 addr, uint8 value)
{
//	debugInterfaceVice->MarkDrive1541CellWrite(addr, value);
	debugInterfaceVice->symbolsDrive1541->memory->CellWrite(addr, value, viceCurrentDiskPC[0], -1, -1);
	
	// skip checking breakpoints when quick fast-forward/restoring snapshot
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;
	
	CDebugSymbolsSegmentDrive1541 *segment = (CDebugSymbolsSegmentDrive1541*) debugInterfaceVice->symbolsDrive1541->currentSegment;
	
	if (segment && segment->breakOnMemory
		&& !segment->breakpointsData->breakpoints.empty())
	{
		debugInterfaceVice->LockMutex();

		CDebugBreakpointData *breakpoint = segment->breakpointsData->EvaluateBreakpoint(addr, value, MEMORY_BREAKPOINT_ACCESS_WRITE);
		if (breakpoint)
		{
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, MEMORY_BREAKPOINT_ACCESS_WRITE, segment);
		}
		
		debugInterfaceVice->UnlockMutex();
	}
}

void c64d_mark_drive1541_cell_execute(uint16 addr, uint8 opcode)
{
	//LOGD("c64d_mark_drive1541_cell_execute: %04x %02x", addr, opcode);
	
	debugInterfaceVice->symbolsDrive1541->memory->CellExecute(addr, opcode);
}

// TODO: add driveId
void c64d_mark_drive1541_contents_track_dirty(uint16 track)
{
	((CDataAdapterViceDrive1541DiskContents*)(debugInterfaceVice->dataAdapterDrive1541DiskContents))->MarkTrackDirty(track);
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

uint32_t c64d_palette_rgba[16];

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

		c64d_palette_rgba[i] = (uint32_t)c64d_palette_red[i]
			| ((uint32_t)c64d_palette_green[i] << 8)
			| ((uint32_t)c64d_palette_blue[i] << 16)
			| 0xFF000000;
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

		c64d_palette_rgba[i] = (uint32_t)c64d_palette_red[i]
			| ((uint32_t)c64d_palette_green[i] << 8)
			| ((uint32_t)c64d_palette_blue[i] << 16)
			| 0xFF000000;
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
	uint8 *destScreenPtr = (uint8 *)debugInterfaceVice->screenImageData->resultData;

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

	debugInterfaceVice->PublishScreenImage();
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

extern "C" int c64d_vic_count_nonzero_in_drawbuffer()
{
	if (vicii.raster.canvas == NULL || vicii.raster.canvas->draw_buffer == NULL)
		return -1;
	uint8 *buf = vicii.raster.canvas->draw_buffer->draw_buffer;
	if (buf == NULL)
		return -1;
	int width  = vicii.raster.canvas->draw_buffer->visible_width;
	int height = vicii.raster.canvas->draw_buffer->visible_height;
	int count = 0;
	for (int i = 0; i < width * height; i++)
		if (buf[i] != 0) count++;
	return count;
}

extern "C" int c64d_vic_read_interior_pixel(int x, int y)
{
	// Interior screen (320×200) → draw_buffer offset.
	//
	//   screenImage path: copy starts at row `skipTopLines` of
	//     draw_buffer into screenImageData row 0 (full width 384).
	//   GetInteriorScreenImage: reads screenImageData(x+32, y+35) for
	//     interior (x, y).
	//
	// So interior (x, y) → draw_buffer[(y + 35 + skipTopLines) * 384
	//   + (x + 32)]. With NORMAL_BORDERS skipTopLines = 16, that's
	//   draw_buffer[(y + 51) * 384 + (x + 32)].
	if (x < 0 || x >= 320 || y < 0 || y >= 200)
		return -1;
	if (vicii.raster.canvas == NULL || vicii.raster.canvas->draw_buffer == NULL)
		return -1;
	uint8 *buf = vicii.raster.canvas->draw_buffer->draw_buffer;
	if (buf == NULL)
		return -1;
	int width   = vicii.raster.canvas->draw_buffer->visible_width;
	int skipTop = c64d_screen_num_skip_top_lines();
	const int offsetX = 32;
	const int offsetY = 35;
	int dbx = x + offsetX;
	int dby = y + offsetY + skipTop;
	return buf[dby * width + dbx];
}

void c64d_refresh_screen_no_callback()
{
	// During debug stepping, the atomic refresh in c64d_debug_pause_check()
	// handles screen updates (partial lines + completed lines). A full-frame
	// copy from draw_buffer would overwrite the current raster line with stale
	// data from the previous frame, causing a visible blink between steps.
	// Use acquire to ensure we see unpause notifications from other threads
	if (c64d_debug_mode.load(std::memory_order_acquire) != DEBUGGER_MODE_RUNNING)
	{
		return;
	}
	//raster_t //vicii.raster
	//struct video_canvas_s //raster->canvas
	//canvas->draw_buffer->draw_buffer

	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;

	// In running mode, screenImageData points to the current producer buffer
	// which only the emulation thread writes to — no lock needed.

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
		uint32_t *destScreenPtr32 = (uint32_t *)debugInterfaceVice->screenImageData->resultData;

		for (int y = 0; y < screenHeight; y++)
		{
			for (int x = 0; x < screenWidth; x++)
			{
				*destScreenPtr32++ = c64d_palette_rgba[*srcScreenPtr++];
			}

			destScreenPtr32 += (512-screenWidth);
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
		uint8 *destScreenPtr = (uint8 *)debugInterfaceVice->screenImageData->resultData;

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

	// Publish the completed frame: swap producer with ready buffer
	debugInterfaceVice->PublishScreenImage();
}

void c64d_refresh_screen()
{
//	LOGD("c64d_refresh_screen: raster_line=%d debug_mode=%d", vicii.raster_line, c64d_debug_mode);
	c64d_refresh_screen_no_callback();
	debugInterfaceVice->DoFrame();
}

// Internal: fast row-by-row refresh of completed lines from draw_buffer into screenImageData.
// Paints screen lines 0..numLines-1 from draw_buffer, starting at draw_buffer line skipTopLines.
// Called from emulator thread only — writes to the producer buffer (no mutex needed).
static void c64d_refresh_lines_fast_locked(int numLines, int skipTopLines, int screenWidth)
{
	if (numLines <= 0)
		return;

	uint8 *screenBuffer = vicii.raster.canvas->draw_buffer->draw_buffer;
	volatile int superSample = debugInterfaceVice->screenSupersampleFactor;

	if (superSample == 1)
	{
		// Fast path: direct pointer arithmetic, row-by-row, sequential memory access
		uint8 *srcScreenPtr = screenBuffer + (skipTopLines * screenWidth);
		uint32_t *destScreenPtr32 = (uint32_t *)debugInterfaceVice->screenImageData->resultData;

		for (int y = 0; y < numLines; y++)
		{
			for (int x = 0; x < screenWidth; x++)
			{
				*destScreenPtr32++ = c64d_palette_rgba[*srcScreenPtr++];
			}
			destScreenPtr32 += (512 - screenWidth);
		}
	}
	else
	{
		// Supersample path: row-by-row with pixel replication
		uint8 *srcScreenPtr = screenBuffer + (skipTopLines * screenWidth);
		uint8 *destScreenPtr = (uint8 *)debugInterfaceVice->screenImageData->resultData;

		for (int y = 0; y < numLines; y++)
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
				destScreenPtr += 512 * superSample * 4;
			}
			srcScreenPtr += screenWidth;
		}
	}
}

// this is called when debug is paused to refresh only part of screen
void c64d_refresh_previous_lines()
{
	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;

	int skipTopLines = c64d_screen_num_skip_top_lines();
	int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
	int numLines = vicii.raster_line - skipTopLines - 1;

	c64d_refresh_lines_fast_locked(numLines, skipTopLines, screenWidth);
	debugInterfaceVice->PublishScreenImage();
}

// Full-screen refresh into producer buffer + publish, works regardless of pause state.
// Used after snapshot restore to update screen with new VIC state.
void c64d_refresh_screen_paused()
{
	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;

	int skipTopLines = c64d_screen_num_skip_top_lines();
	int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
	int screenHeight = vicii.raster.canvas->draw_buffer->visible_height;

	c64d_refresh_lines_fast_locked(screenHeight, skipTopLines, screenWidth);
	debugInterfaceVice->PublishScreenImage();
}

// Paint the current partial raster line from dbuf into screenImageData.
// Called from emulator thread only — writes to the producer buffer (no mutex needed).
static void c64d_refresh_dbuf_line_locked(int rasterY, int screenWidth)
{
	if (rasterY < 0)
		return;

	int borderMode = debugInterfaceVice->GetViciiBorderMode();
	int lineOffset;

	switch(borderMode)
	{
		default:
		case VICII_NORMAL_BORDERS:  lineOffset = 104;     break;
		case VICII_FULL_BORDERS:    lineOffset = 104-16;  break;
		case VICII_DEBUG_BORDERS:   lineOffset = 0;       break;
		case VICII_NO_BORDERS:      lineOffset = 104+32;  break;
	}

	volatile int superSample = debugInterfaceVice->screenSupersampleFactor;

	for (int l = 0; l < vicii.dbuf_offset; l++)
	{
		int x = l - lineOffset;
		if (x < 0 || x > screenWidth)
			continue;

		u8 v = vicii.dbuf[l];

		for (int i = 0; i < superSample; i++)
		{
			for (int j = 0; j < superSample; j++)
			{
				debugInterfaceVice->screenImageData->SetPixelResultRGBA(
					x * superSample + j,
					rasterY * superSample + i,
					c64d_palette_red[v], c64d_palette_green[v], c64d_palette_blue[v], 255);
			}
		}
	}
}

void c64d_refresh_dbuf()
{
	if (debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
		return;

	int skipTopLines = c64d_screen_num_skip_top_lines();
	int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
	int rasterY = vicii.raster_line - skipTopLines;

	if (rasterY < 0 || rasterY > vicii.raster.canvas->draw_buffer->visible_height)
		return;

	c64d_refresh_dbuf_line_locked(rasterY, screenWidth);
	debugInterfaceVice->PublishScreenImage();
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
		LOGD("DIAG: temp PC breakpoint hit at pc=%04x during mode=%d", pc, c64d_debug_mode.load(std::memory_order_relaxed));
		c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "temp-pc-bp");
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
		segment->breakpointsPC->temporaryBreakpointPC = -1;
	}
	else if (!segment->breakpointsPC->breakpoints.empty())
	{
		debugInterfaceVice->LockMutex();

		CDebugBreakpointAddr *addrBreakpoint = segment->breakpointsPC->EvaluateBreakpoint(pc);
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
				if (rasterY < 0x33 || rasterY > 0xFB
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
				LOGD("DIAG: PC breakpoint hit at pc=%04x during mode=%d",
					 pc, c64d_debug_mode.load(std::memory_order_relaxed));
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
	else if (segment->breakOnPC
		&& !segment->breakpointsPC->breakpoints.empty())
	{
		debugInterfaceVice->LockMutex();
		CDebugBreakpointAddr *addrBreakpoint = segment->breakpointsPC->EvaluateBreakpoint(pc);
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
	void c64d_get_exrom_game(uint8_t *exrom, uint8_t *game);
	void c64d_get_ultimax_phi(uint8_t *ultimax_phi1, uint8_t *ultimax_phi2);
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
	uint8_t c64d_peek_memory0001();
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
	
	viciiCopy->registerWritten = vicii.prev_register_written;
	viciiCopy->registerRead = vicii.prev_register_read;

	// CIA1
	viciiCopy->cia1RegisterWritten = c64d_cia1_register_written;
	viciiCopy->cia1RegisterRead = c64d_cia1_register_read;
	viciiCopy->cia1WriteValue = c64d_cia1_write_value;
	viciiCopy->cia1ReadValue = c64d_cia1_read_value;
	c64d_cia1_register_written = -1;
	c64d_cia1_register_read = -1;

	// CIA2
	viciiCopy->cia2RegisterWritten = c64d_cia2_register_written;
	viciiCopy->cia2RegisterRead = c64d_cia2_register_read;
	viciiCopy->cia2WriteValue = c64d_cia2_write_value;
	viciiCopy->cia2ReadValue = c64d_cia2_read_value;
	c64d_cia2_register_written = -1;
	c64d_cia2_register_read = -1;

	// SID
	viciiCopy->sidRegisterWritten = c64d_sid_register_written;
	viciiCopy->sidRegisterRead = c64d_sid_register_read;
	viciiCopy->sidWriteValue = c64d_sid_write_value;
	viciiCopy->sidReadValue = c64d_sid_read_value;
	c64d_sid_register_written = -1;
	c64d_sid_register_read = -1;

	// Memory access tracking
	viciiCopy->memAccessAddr = c64d_mem_access_addr;
	viciiCopy->memAccessValue = c64d_mem_access_value;
	viciiCopy->memAccessIsWrite = c64d_mem_access_is_write;
	c64d_mem_access_addr = -1;

	// ghostbyte: 0x3FFF normally, 0x39FF when ECM (bit 6 of $D011)
	uint16 ghostLocalAddr = (vicii.regs[0x11] & 0x40) ? 0x39FF : 0x3FFF;
	int ghostPhysAddr = ((ghostLocalAddr + vicii.vbank_phi1) & vicii.vaddr_mask_phi1) | vicii.vaddr_offset_phi1;
	viciiCopy->ghostbyteAddr = ghostLocalAddr;
	viciiCopy->ghostbyteValue = vicii.ram_base_phi1[ghostPhysAddr];
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
	
	viciiDest->registerWritten = viciiSrc->registerWritten;
	viciiDest->registerRead = viciiSrc->registerRead;

	viciiDest->cia1RegisterWritten = viciiSrc->cia1RegisterWritten;
	viciiDest->cia1RegisterRead = viciiSrc->cia1RegisterRead;
	viciiDest->cia1WriteValue = viciiSrc->cia1WriteValue;
	viciiDest->cia1ReadValue = viciiSrc->cia1ReadValue;
	viciiDest->cia2RegisterWritten = viciiSrc->cia2RegisterWritten;
	viciiDest->cia2RegisterRead = viciiSrc->cia2RegisterRead;
	viciiDest->cia2WriteValue = viciiSrc->cia2WriteValue;
	viciiDest->cia2ReadValue = viciiSrc->cia2ReadValue;
	viciiDest->sidRegisterWritten = viciiSrc->sidRegisterWritten;
	viciiDest->sidRegisterRead = viciiSrc->sidRegisterRead;
	viciiDest->sidWriteValue = viciiSrc->sidWriteValue;
	viciiDest->sidReadValue = viciiSrc->sidReadValue;

	viciiDest->memAccessAddr = viciiSrc->memAccessAddr;
	viciiDest->memAccessValue = viciiSrc->memAccessValue;
	viciiDest->memAccessIsWrite = viciiSrc->memAccessIsWrite;

	viciiDest->ghostbyteAddr = viciiSrc->ghostbyteAddr;
	viciiDest->ghostbyteValue = viciiSrc->ghostbyteValue;
}


//uint32 viciiFrameCycleNum = 0;

// TODO: add setting in settings
uint8 c64d_vicii_record_state_mode = C64D_VICII_RECORD_MODE_EVERY_CYCLE; //C64D_VICII_RECORD_MODE_NONE;

void c64d_c64_set_vicii_record_state_mode(uint8 recordMode)
{
	c64d_vicii_record_state_mode = recordMode;
}

// ---- Multi-frame VIC state recorder ----
// Diagnostic ring of full per-cycle frame snapshots. Allocated on
// _start, freed on _stop. Hooked into c64d_c64_vicii_start_frame
// below: every time VICE begins a new frame, we copy the just-
// completed viciiStateForCycle[312][64] grid into the next ring slot.
typedef vicii_cycle_state_t viciiFrameGrid_t[312][64];
static viciiFrameGrid_t *gViciiMultiFrame = NULL;
static int gViciiMultiFrameCapacity = 0;
static int gViciiMultiFrameCount    = 0;
static bool gViciiMultiFrameSkipFirst = false;

void c64d_vicii_multiframe_start(int numFrames)
{
	c64d_vicii_multiframe_stop();
	if (numFrames <= 0) return;
	if (numFrames > 8) numFrames = 8;
	gViciiMultiFrame = (viciiFrameGrid_t *)malloc(sizeof(viciiFrameGrid_t) * numFrames);
	if (gViciiMultiFrame == NULL) return;
	gViciiMultiFrameCapacity = numFrames;
	gViciiMultiFrameCount    = 0;
	// Skip the first start_of_frame after _start because viciiStateForCycle
	// at that point holds a partial / mid-frame state from when we were
	// called. Wait one frame so the NEXT start_of_frame sees a complete
	// frame's worth of recorded cycles.
	gViciiMultiFrameSkipFirst = true;
}

void c64d_vicii_multiframe_stop()
{
	if (gViciiMultiFrame) free(gViciiMultiFrame);
	gViciiMultiFrame = NULL;
	gViciiMultiFrameCapacity = 0;
	gViciiMultiFrameCount    = 0;
	gViciiMultiFrameSkipFirst = false;
}

int c64d_vicii_multiframe_get_count()    { return gViciiMultiFrameCount; }
int c64d_vicii_multiframe_get_capacity() { return gViciiMultiFrameCapacity; }

vicii_cycle_state_t *c64d_vicii_multiframe_get(int frameIdx, int rasterLine, int rasterCycle)
{
	if (gViciiMultiFrame == NULL) return NULL;
	if (frameIdx < 0 || frameIdx >= gViciiMultiFrameCount) return NULL;
	if (rasterLine < 0 || rasterLine >= 312) return NULL;
	if (rasterCycle < 0 || rasterCycle >= 64) return NULL;
	return &(gViciiMultiFrame[frameIdx][rasterLine][rasterCycle]);
}

static void c64d_vicii_multiframe_capture_completed_frame()
{
	if (gViciiMultiFrame == NULL) return;
	if (gViciiMultiFrameCount >= gViciiMultiFrameCapacity) return;
	if (gViciiMultiFrameSkipFirst)
	{
		gViciiMultiFrameSkipFirst = false;
		return;
	}
	memcpy(&(gViciiMultiFrame[gViciiMultiFrameCount]),
	       viciiStateForCycle,
	       sizeof(viciiStateForCycle));
	gViciiMultiFrameCount++;
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

	// Multi-frame VIC state recorder hook — must run AFTER DoVSync so
	// the captured frame matches what the rest of the c64d pipeline
	// sees as "the previous frame".
	c64d_vicii_multiframe_capture_completed_frame();
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
			
			fprintf(c64d_profiler_file_out, "vic %llu %u %d %d %d %d %d\n",
					(unsigned long long)c64d_maincpu_clk, frameNum, raster_line, raster_cycle, bad_line,
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
	if (segment && segment->breakOnRaster
		&& !segment->breakpointsRasterLine->breakpoints.empty())
	{
		debugInterfaceVice->LockMutex();
		CDebugBreakpointAddr *breakpoint = segment->breakpointsRasterLine->EvaluateBreakpoint(rasterLine);
		if (breakpoint != NULL)
		{
			LOGD("DIAG: raster breakpoint fired at line=%d during mode=%d", rasterLine, c64d_debug_mode.load(std::memory_order_relaxed));
			c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "raster-bp");
			debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
			segment->symbols->debugEventsHistory->CreateEventBreakpoint(breakpoint, ADDR_BREAKPOINT_ACTION_STOP_ON_RASTER, segment);
		}
		debugInterfaceVice->UnlockMutex();
	}
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
		LOGD("DIAG: VIC IRQ breakpoint fired during mode=%d", c64d_debug_mode.load(std::memory_order_relaxed));
		c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "irq-vic-bp");
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}

void c64d_c64_check_irqcia_breakpoint(int ciaNum)
{
	if (debugInterfaceVice->snapshotsManager->IsPerformingSnapshotRestore())
		return;

	CDebugSymbolsSegmentC64 *segment = (CDebugSymbolsSegmentC64*) debugInterfaceVice->symbols->currentSegment;
	if (segment && segment->breakOnC64IrqCIA)
	{
		LOGD("DIAG: CIA%d IRQ breakpoint fired during mode=%d", ciaNum, c64d_debug_mode.load(std::memory_order_relaxed));
		c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "irq-cia-bp");
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
		LOGD("DIAG: NMI breakpoint fired during mode=%d", c64d_debug_mode.load(std::memory_order_relaxed));
		c64d_debug_mode_trace(DEBUGGER_MODE_PAUSED, "irq-nmi-bp");
		debugInterfaceVice->SetDebugMode(DEBUGGER_MODE_PAUSED);
	}
}

// Set while the emulation thread is parked in the pause loop below, i.e. while it
// holds no debugger locks and can service a debugger request (such as a deferred PC
// change) on the next wake-up. MakeJmpC64 uses this to decide whether it may wait
// for the request to be committed: when the CPU is NOT parked it is still on its way
// to a pause point and may need a lock the calling thread is holding, so waiting
// there could stall the caller for no reason.
static std::atomic<int> c64d_cpu_parked_in_pause_loop(0);

int c64d_is_cpu_parked_in_pause_loop(void)
{
	return c64d_cpu_parked_in_pause_loop.load(std::memory_order_acquire);
}

// Clears the parked flag on every exit path out of the pause loop, including the
// early return taken while a snapshot restore is in progress.
class CParkedInPauseLoopGuard
{
public:
	CParkedInPauseLoopGuard()  { c64d_cpu_parked_in_pause_loop.store(1, std::memory_order_release); }
	~CParkedInPauseLoopGuard() { c64d_cpu_parked_in_pause_loop.store(0, std::memory_order_release); }
};

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
		if (c64d_is_performing_snapshot_restore() || c64d_is_external_snapshot_request_active()
			|| c64d_is_snapshot_operation_in_progress())
			return;
	}

	// Use relaxed load for initial check - we're in emulation thread, no inter-thread visibility needed yet
	int currentMode = c64d_debug_mode.load(std::memory_order_acquire);
	if (currentMode == DEBUGGER_MODE_PAUSED)
	{
		LOGD("pause_check: entering pause loop (set by: %s, allowRestore=%d, clk=%d)",
			 c64d_debug_mode_last_setter, allowRestore, (int)c64d_maincpu_clk);
		// Only refresh once when entering pause mode, not on every pause check
		if (!pauseRefreshDone)
		{
			pauseRefreshDone = 1;

			// Refresh all completed lines + current partial line into the producer
			// buffer, then publish for the render thread to pick up via triple-buffer.
			// No mutex needed — the producer buffer is only written by this thread.
			if (!debugInterfaceVice->snapshotsManager->SkipRefreshOfVideoFrame())
			{
				int skipTopLines = c64d_screen_num_skip_top_lines();
				int screenWidth = vicii.raster.canvas->draw_buffer->visible_width;
				int screenHeight = vicii.raster.canvas->draw_buffer->visible_height;
				int rasterY = vicii.raster_line - skipTopLines;

				// Refresh the FULL screen from draw_buffer. With triple-buffer,
				// the producer buffer may contain stale data from 2 publishes ago,
				// so we must repaint all lines (not just 0..rasterY) to avoid
				// a visible flash of old screen content below the raster cursor.
				// draw_buffer has correct data: current frame above raster,
				// previous frame below raster.
				c64d_refresh_lines_fast_locked(screenHeight, skipTopLines, screenWidth);

				// Render current partial line from dbuf on top of the full refresh.
				if (rasterY >= 0 && rasterY <= screenHeight && vicii.raster_cycle >= 1)
				{
					c64d_refresh_dbuf_line_locked(rasterY, screenWidth);
				}

				debugInterfaceVice->PublishScreenImage();
			}
			c64d_refresh_cia();
		}

		{
			CParkedInPauseLoopGuard parkedGuard;
			std::unique_lock<std::mutex> lock(debugInterfaceVice->pauseMutex);
			while (c64d_debug_mode.load(std::memory_order_acquire) == DEBUGGER_MODE_PAUSED)
			{
				// Use 16ms timeout (~60fps) to keep audio processing frequent while paused.
				// The predicate also wakes on a debugger PC change requested while paused
				// (MakeJmpC64) so it commits here, on the emulation thread, instead of only
				// at the pause exit — this is what lets MakeJmpC64 report a committed jump.
				bool signaled = debugInterfaceVice->pauseCV.wait_for(lock, std::chrono::milliseconds(16),
					[]{ return c64d_debug_mode.load(std::memory_order_acquire) != DEBUGGER_MODE_PAUSED
							|| c64d_is_pc_change_pending(); });

				if (signaled && c64d_debug_mode.load(std::memory_order_acquire) == DEBUGGER_MODE_PAUSED)
				{
					// Woken only to apply a pending debugger PC change; stay paused.
					lock.unlock();
					c64d_apply_pending_debugger_pc();
					lock.lock();
					continue;
				}

				if (!signaled)
				{
					// Timeout: process audio and handle snapshot/SID restore while paused
					lock.unlock();

					// Backstop in case the notify above was missed
					c64d_apply_pending_debugger_pc();

					if (allowRestore)
					{
						c64d_check_cpu_snapshot_manager_restore();
					}
					else
					{
						if (c64d_is_performing_snapshot_restore() || c64d_is_external_snapshot_request_active()
			|| c64d_is_snapshot_operation_in_progress())
						{
							if (debugInterfaceVice->ShouldProcessPausedVSync())
							{
								vsync_do_vsync(vicii.raster.canvas, 0, 1);
							}
							return;
						}
					}

					if (debugInterfaceVice->sidDataToRestore)
					{
						debugInterfaceVice->sidDataToRestore->RestoreSids();
						debugInterfaceVice->sidDataToRestore = NULL;
					}


					if (debugInterfaceVice->ShouldProcessPausedVSync())
					{
						// Process audio while debugger-paused. When the C64 emulator is
						// disabled from the menu, audio is bypassed and must not feed VSync.
						vsync_do_vsync(vicii.raster.canvas, 0, 1);
					}

					lock.lock();
				}
				// signaled=true: predicate passed, outer while exits
			}
		}

		// Reset flag when exiting pause mode
		pauseRefreshDone = 0;
		debugInterfaceVice->RefreshSync();
		LOGD("pause_check: exited pause loop, mode now=%d (set by: %s, clk=%d)",
			 c64d_debug_mode.load(std::memory_order_relaxed), c64d_debug_mode_last_setter, (int)c64d_maincpu_clk);
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

int c64d_is_external_snapshot_request_active()
{
	return debugInterfaceVice->snapshotsManager->IsExternalSnapshotRequestActive() ? 1 : 0;
}

// True while a snapshot is being written or read by anyone -- not only through
// PerformExternalSnapshotRequest(). The drive CPUs are run forward as part of
// writing one and must unwind out of the pause loop rather than park, or the
// thread doing the snapshot waits for an unpause only it could deliver.
int c64d_is_snapshot_operation_in_progress()
{
	return debugInterfaceVice->snapshotsManager->IsSnapshotOperationInProgress() ? 1 : 0;
}
		
// Fast C-level flag for the CPU hot loop — avoids C++ pointer dereferences per cycle.
// Set by C++ code when replay/recording is toggled or tasks are added.
volatile int c64d_vice_input_tasks_flag = 0;
volatile int c64d_input_latch_immediate = 0;

int c64d_has_pending_input_tasks()
{
	return c64d_vice_input_tasks_flag;
}

int c64d_check_snapshot_restore(int allowSnapshotRestore)
{
	debugInterfaceVice->snapshotsManager->CheckMainCpuCycle();

	// Execute pending tasks and replay input events at current cycle
	debugInterfaceVice->ExecuteDebugInterruptTasks();

	// Clear the fast flag if no more work pending
	if (!debugInterfaceVice->hasPendingCpuDebugInterruptTasks.load(std::memory_order_acquire)
		&& !debugInterfaceVice->snapshotsManager->isReplayInputEventsEnabled
		&& !debugInterfaceVice->snapshotsManager->isStoreInputEventsEnabled)
	{
		c64d_vice_input_tasks_flag = 0;
	}

	// Only execute the actual snapshot restore at an instruction boundary
	// (allowSnapshotRestore=1). Doing it from the per-cycle hook would restore
	// mid-instruction, leaving reg_pc stale (no IMPORT_REGISTERS here) and
	// jamming the CPU. The boundary path (c64d_check_cpu_snapshot_manager_restore)
	// will pick up the pending restore on the next instruction.
	if (allowSnapshotRestore && debugInterfaceVice->snapshotsManager->CheckSnapshotRestore())
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
	for (int dnr = 0; dnr < NUM_DISK_UNITS; dnr++)
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
	for (int dnr = 0; dnr < NUM_DISK_UNITS; dnr++)
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

void c64d_set_drive_dirty_needs_refresh_flag(int driveNum)
{
	drive_s *drive = drive_context[driveNum]->drive;
	drive->GCR_dirty_track_needs_refresh = 1;
	drive->P64_dirty_needs_refresh = 1;
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
	
	if (SMW_DW(m, (uint32_t)c64d_maincpu_clk) < 0) {
		snapshot_module_close(m);
		return -1;
	}
	
	if (SMW_DW(m, debugInterfaceVice->emulationFrameCounter.load()) < 0) {
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
		uint16_t screenHeight = debugInterfaceVice->GetScreenSizeY();
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
	uint8_t major_version, minor_version;
	snapshot_module_t *m;
	
	m = snapshot_module_open(s, "DEBUGGER", &major_version, &minor_version);
	if (m == NULL) {
		return 0;	// no debugger module, skip it
	}
	
	if (major_version != C64D_SNAPSHOT_VER_MAJOR || minor_version > C64D_SNAPSHOT_VER_MINOR) {
		snapshot_module_close(m);
		return -1;
	}
	
	{
		unsigned int clk_lo32;
		if (SMR_DW_UINT(m, &clk_lo32) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		c64d_maincpu_clk = (CLOCK)clk_lo32;
	}
	
	{
		unsigned int emulationFrameCounter;
		if (SMR_DW_UINT(m, &emulationFrameCounter) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		debugInterfaceVice->emulationFrameCounter.store(emulationFrameCounter);
	}
	
	if (minor_version > 0)
	{
		// restore screen data
		uint8_t restore_screen;
		if (SMR_B(m, &restore_screen) < 0) {
			snapshot_module_close(m);
			return -1;
		}
		
		if (restore_screen)
		{
			uint16_t screenHeight = -1;
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
			
			c64d_refresh_screen_no_callback();
		}
		
		// restore vicii states?
		uint8_t restore_vicii_states;
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

CLOCK c64d_get_vice_maincpu_clk()
{
	return maincpu_clk;
}

CLOCK c64d_get_vice_drivecpu_clk(int driveNum)
{
	if (driveNum < 0 || driveNum >= NUM_DISK_UNITS) return 0;
	return drive_clk[driveNum];
}

CLOCK c64d_get_vice_maincpu_current_instruction_clk()
{
	return c64d_maincpu_current_instruction_clk;
}


// extern "C"
}
