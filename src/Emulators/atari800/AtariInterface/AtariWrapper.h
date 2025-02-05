#ifndef _ATARI_INTERFACE_H_
#define _ATARI_INTERFACE_H_

#include "SYS_Types.h"
#include "DebuggerDefs.h"
#include <stdio.h>

#define C64DEBUGGER_ATARI800_VERSION_STRING		"5.0.0"

#define ATARI_AUDIO_BUFFER_FRAMES	4096
#define MAX_NUM_POKEYS	2

void mt_SYS_FatalExit(char *text);
unsigned long mt_SYS_GetCurrentTimeInMillis();
void mt_SYS_Sleep(unsigned long milliseconds);

char *ATRD_GetPathForRoms();

extern volatile int atrd_debug_mode;

void atrd_mark_atari_cell_read(uint16 addr);
void atrd_mark_atari_cell_write(uint16 addr, uint8 value);
void atrd_mark_atari_cell_execute(uint16 addr, uint8 opcode);
void atrd_check_pc_breakpoint(uint16 pc);
int atrd_debug_pause_check(int allowRestore);

int atrd_is_performing_snapshot_restore();
int atrd_check_snapshot_restore();
void atrd_check_snapshot_interval();

void atrd_async_load_snapshot(char *filePath);
void atrd_async_save_snapshot(char *filePath);
void atrd_async_set_cpu_pc(int newPC);
void atrd_async_set_reg_a(int newRegValue);
void atrd_async_set_reg_x(int newRegValue);
void atrd_async_set_reg_y(int newRegValue);
void atrd_async_set_reg_p(int newRegValue);
void atrd_async_set_reg_s(int newRegValue);

void atrd_async_check();

void atrd_sound_init();
void atrd_sound_pause();
void atrd_sound_resume();

void atrd_sound_lock();
void atrd_sound_unlock();

void atrd_mutex_lock();
void atrd_mutex_unlock();

int atrd_get_is_receive_channels_data(int pokeyNum);
void atrd_pokey_receive_channels_data(int pokeyNum, int isOn);
void atrd_pokey_channels_data(int pokeyNumber, int v1, int v2, int v3, int v4, short mix);

int atrd_is_debug_on_atari();

unsigned int atrd_get_joystick_state(int port);

unsigned int atrd_get_emulation_frame_number();
void atrd_set_emulation_frame_number(unsigned int frameNum);

// read and write state to CByteBuffer
void *atrd_state_buffer_open(const char *name, const char *mode);
int atrd_state_buffer_close(void *stream);
size_t atrd_state_buffer_read(void *buf, size_t len, void *stream);
size_t atrd_state_buffer_write(const void *buf, size_t len, void *stream);

//
extern volatile int atrd_start_frame_for_snapshots_manager;
extern volatile unsigned int atrd_previous_instruction_maincpu_clk;
extern volatile unsigned int atrd_previous2_instruction_maincpu_clk;

int atrd_get_emulation_frame_num();

extern volatile unsigned int atrdMainCpuDebugCycle;
extern volatile unsigned int atrdMainCpuCycle;
extern volatile unsigned int atrdMainCpuPreviousInstructionCycle;

void c64d_show_message(char *message);

#endif
