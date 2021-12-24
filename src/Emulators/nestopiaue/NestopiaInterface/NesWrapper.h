#ifndef _NES_INTERFACE_H_
#define _NES_INTERFACE_H_

#include "SYS_Types.h"
#include "DebuggerDefs.h"

class CByteBuffer;

extern volatile int nesd_debug_mode;

bool NestopiaUE_Initialize();
bool NestopiaUE_PostInitialize();
bool NestopiaUE_Run();

bool nesd_insert_cartridge(char *filePath);

void nesd_reset();
unsigned char *nesd_get_ram();
unsigned int nesd_get_cpu_pc();
unsigned char nesd_peek_io(unsigned short addr);
void nesd_get_cpu_regs(unsigned short *pc, unsigned char *a, unsigned char *x, unsigned char *y, unsigned char *p, unsigned char *s, unsigned char *irq);
void nesd_get_ppu_clocks(unsigned int *hClock, unsigned int *vClock, unsigned int *cycle);

void nesd_async_check();

CByteBuffer *nesd_store_state();
bool nesd_restore_state(CByteBuffer *byteBuffer);
bool nesd_store_nesd_state_to_bytebuffer(CByteBuffer *byteBuffer);
bool nesd_restore_nesd_state_from_bytebuffer(CByteBuffer *byteBuffer);
unsigned char nesd_peek_io(unsigned short addr);
unsigned char nesd_peek_safe_io(unsigned short addr);


bool nesd_is_debug_on();
void nesd_update_cpu_pc_by_emulator(uint16 cpuPC);
void nesd_mark_cell_read(uint16 addr);
void nesd_mark_cell_write(uint16 addr, uint8 value);
void nesd_mark_cell_execute(uint16 addr, uint8 opcode);
void nesd_check_pc_breakpoint(uint16 pc);

int nesd_is_performing_snapshot_restore();
int nesd_check_snapshot_restore();
void nesd_check_snapshot_interval();
void nesd_check_cpu_snapshot_manager_restore();
void nesd_check_cpu_snapshot_manager_store();
int nesd_debug_pause_check(int allowRestore);

void nesd_update_screen(bool lockRenderMutex);

//
//void nesd_async_check();
//
//void nesd_async_load_snapshot(char *filePath);
//void nesd_async_save_snapshot(char *filePath);
//
void nesd_sound_init();
void nesd_sound_pause();
void nesd_sound_resume();

void nesd_joystick_down(int port, uint32 axis);
void nesd_joystick_up(int port, uint32 axis);

void nesd_audio_callback(uint8 *stream, int numSamples);
void nesd_sound_lock(char *whoLocked);
void nesd_sound_unlock(char *whoLocked);

void nesd_reset_sync();

void nesd_mute_channels(bool muteSquare1, bool muteSquare2, bool muteTriangle, bool muteNoise, bool muteDmc, bool muteExt);
extern volatile bool nesd_isReceiveChannelsData;
void nesd_receive_channels_data(unsigned int valSquare1, unsigned int valSquare2, unsigned int valTriangle, unsigned int valNoise, unsigned int valDmc, unsigned int valExt, unsigned int valMix);

uint8 nesd_get_apu_register(uint16 addr);
bool nesd_is_pal();
double nesd_get_cpu_clock_frquency();


void nesd_mutex_lock();
void nesd_mutex_unlock();

//

//
//int nesd_get_joystick_state(int port);

#endif
