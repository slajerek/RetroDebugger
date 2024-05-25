#ifndef _VICEDEBUGINTERFACE_H_
#define _VICEDEBUGINTERFACE_H_

#include "SYS_Types.h"
#include "DebuggerDefs.h"

#define C64DEBUGGER_VICE_VERSION_STRING		"3.1"

#ifndef VICII_NUM_SPRITES
#define VICII_NUM_SPRITES      8
#endif

struct vicii_sprite_state_s {
	uint16 data;
	uint8 mc;
	uint8 mcbase;
	uint8 pointer;
	int exp_flop;
	int x;
};
typedef struct vicii_sprite_state_s vicii_sprite_state_t;

struct vicii_cycle_state_s
{
	uint8 regs[0x40];
	
	unsigned int raster_cycle;
	unsigned int raster_line;
	unsigned int raster_irq_line;
	
	int vbank_phi1;
	int vbank_phi2;
	
	int idle_state;
	int rc;
	int vc;
	int vcbase;
	int vmli;
	
	int bad_line;

	uint8 last_read_phi1;
	uint8 sprite_dma;
	unsigned int sprite_display_bits;
	
	vicii_sprite_state_t sprite[VICII_NUM_SPRITES];
	
	// additional vars
	uint8 exrom, game;
	uint8 export_ultimax_phi1, export_ultimax_phi2;
	uint16 vaddr_mask_phi1;
	uint16 vaddr_mask_phi2;
	uint16 vaddr_offset_phi1;
	uint16 vaddr_offset_phi2;
	
	uint16 vaddr_chargen_mask_phi1;
	uint16 vaddr_chargen_value_phi1;
	uint16 vaddr_chargen_mask_phi2;
	uint16 vaddr_chargen_value_phi2;
	
	// cpu
	uint8 a, x, y;
	uint8 processorFlags, sp;
	uint16 pc;
	//	uint8 intr[4];		// Interrupt state
	uint16 lastValidPC;
	uint8 instructionCycle;
	
	uint8 memory0001;
	
};

typedef struct vicii_cycle_state_s vicii_cycle_state_t;

//
#ifdef __cplusplus
extern "C" {
#endif

void c64d_sound_init();
void c64d_sound_pause();
void c64d_sound_resume();

extern uint16 viceCurrentC64PC;
extern uint16 viceCurrentDiskPC[4];

unsigned long mt_SYS_GetCurrentTimeInMillis();
void mt_SYS_Sleep(unsigned long milliseconds);
void mt_SYS_FatalExit(char *text);

void c64d_mark_c64_cell_read(uint16 addr);
void c64d_mark_c64_cell_write(uint16 addr, uint8 value);
void c64d_mark_c64_cell_execute(uint16 addr, uint8 opcode);

// TODO: add device num
void c64d_mark_drive1541_cell_read(uint16 addr);
void c64d_mark_drive1541_cell_write(uint16 addr, uint8 value);
void c64d_mark_drive1541_cell_execute(uint16 addr, uint8 opcode);

void c64d_mark_drive1541_contents_track_dirty(uint16 track);

void c64d_clear_screen();
void c64d_refresh_screen_no_callback();
void c64d_refresh_screen();
void c64d_refresh_previous_lines();
void c64d_refresh_dbuf();
void c64d_refresh_cia();

int c64d_set_vicii_border_mode(int borderMode);
int c64d_get_vicii_border_mode();

void c64d_c64_set_vicii_record_state_mode(uint8 recordMode);

void c64d_vicii_copy_state(vicii_cycle_state_t *viciiCopy);
void c64d_vicii_copy_state_data(vicii_cycle_state_t *viciiDest, vicii_cycle_state_t *viciiSrc);

vicii_cycle_state_t *c64d_get_vicii_state_for_raster_cycle(int rasterLine, int rasterCycle);
vicii_cycle_state_t *c64d_get_vicii_state_for_raster_line(int rasterLine);
void c64d_c64_vicii_cycle();

void c64d_c64_vicii_start_frame();
void c64d_c64_vicii_start_raster_line(uint16 rasterLine);

void c64d_display_speed(float speed, float frame_rate);
void c64d_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2);

extern uint8 c64d_palette_red[16];
extern uint8 c64d_palette_green[16];
extern uint8 c64d_palette_blue[16];

extern float c64d_float_palette_red[16];
extern float c64d_float_palette_green[16];
extern float c64d_float_palette_blue[16];

void c64d_set_palette(uint8 *palette);
void c64d_set_palette_vice(uint8 *palette);

int c64d_is_debug_on_c64();
int c64d_is_debug_on_drive1541();

extern volatile int c64d_debug_mode;

void c64d_c64_check_pc_breakpoint(uint16 pc);
void c64d_c64_check_raster_breakpoint(uint16 rasterLine);
int c64d_c64_is_checking_irq_breakpoints_enabled();
void c64d_drive1541_check_pc_breakpoint(uint16 pc);
int c64d_drive1541_is_checking_irq_breakpoints_enabled();
void c64d_drive1541_check_irqiec_breakpoint();
void c64d_drive1541_check_irqvia1_breakpoint();
void c64d_drive1541_check_irqvia2_breakpoint();

void c64d_c64_check_irqvic_breakpoint();
void c64d_c64_check_irqcia_breakpoint(int ciaNum);
void c64d_c64_check_irqnmi_breakpoint();
void c64d_debug_pause_check(int allowRestore);

void c64d_show_message(char *message);

// SID
extern int c64d_is_receive_channels_data[C64_MAX_NUM_SIDS];
void c64d_sid_receive_channels_data(int sidNum, int isOn);
void c64d_sid_channels_data(int sidNum, int v1, int v2, int v3, short mix);
void c64d_set_volume(float volume);
extern int c64d_skip_sound_run_sound_in_sound_store;

// VIC
void c64d_set_color_register(uint8 colorRegisterNum, uint8 value);

extern int c64d_skip_drawing_sprites;

// ROM patch
extern int c64d_patch_kernal_fast_boot_flag;

// run SID when in warp mode?
extern int c64d_setting_run_sid_when_in_warp;

// run SID emulation at all or always skip?
extern int c64d_setting_run_sid_emulation;

// Main CPU cycle of previous instruction
extern unsigned int c64d_maincpu_current_instruction_clk;
extern volatile unsigned int c64d_maincpu_previous_instruction_clk;
extern volatile unsigned int c64d_maincpu_previous2_instruction_clk;

//// render transparent c64 screen (for Vic Display), transparent color = $d021
//extern int c64d_setting_render_transparent_screen;

unsigned int c64d_get_frame_num();
void c64d_reset_counters();

int c64d_is_performing_snapshot_restore();
int c64d_check_cpu_snapshot_manager_restore();
void c64d_check_cpu_snapshot_manager_store();
void c64d_check_snapshot_interval();

// returns 1 if snapshot was restored
int c64d_check_snapshot_restore();

// check if disk image changed, required for snapshots manager
int c64d_is_drive_dirty_for_snapshot();
void c64d_clear_drive_dirty_for_snapshot();

// check if disk image changed, required for disk browser
int c64d_is_drive_dirty_and_needs_refresh(int driveNum);
void c64d_set_drive_dirty_needs_refresh_flag(int driveNum);
void c64d_clear_drive_dirty_needs_refresh_flag(int driveNum);

unsigned int c64d_get_drive_is_disk_attached(int driveId);

void c64d_uimon_print(char *p);
void c64d_uimon_print_line(char *p);

void c64d_lock_mutex();
void c64d_unlock_mutex();

#ifdef __cplusplus
}
#endif

#endif

