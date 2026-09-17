/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/*
 * vice310_stubs.c - Stub implementations for VICE 3.10 features not yet
 *                  integrated into c64d (profiler, ui_pause, callstack, etc.)
 *
 * These stubs allow the build to succeed while the full feature integration
 * remains deferred.
 */

#include "vice.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "vicetypes.h"
#include "montypes.h"
#include "mon_profile.h"
#include "monitor.h"

/* -------------------------------------------------------------------------
 * Profiler callstack variables (normally in profiler.c which we don't build).
 * mon_backtrace() in monitor.c references these as extern arrays.
 * Stubs: empty arrays / zero size => backtrace always shows nothing.
 * ------------------------------------------------------------------------- */
#define MAX_CALLSTACK_SIZE 512
uint16_t callstack_pc_dst[MAX_CALLSTACK_SIZE];
uint16_t callstack_pc_src[MAX_CALLSTACK_SIZE];
uint16_t callstack_memory_bank_config[MAX_CALLSTACK_SIZE];
uint8_t  callstack_sp[MAX_CALLSTACK_SIZE];
unsigned callstack_size = 0;

/* -------------------------------------------------------------------------
 * mon_profile functions (normally in mon_profile.c which depends on profiler.c)
 * Stubs: just do nothing / print a message.
 * ------------------------------------------------------------------------- */
void mon_profile(void)
{
    /* profiler not integrated */
}

void mon_profile_action(ACTION action)
{
    (void)action;
}

void mon_profile_flat(int num)
{
    (void)num;
}

void mon_profile_graph(int context_id, int depth)
{
    (void)context_id;
    (void)depth;
}

void mon_profile_func(MON_ADDR function)
{
    (void)function;
}

void mon_profile_disass(MON_ADDR function)
{
    (void)function;
}

void mon_profile_clear(MON_ADDR function)
{
    (void)function;
}

void mon_profile_disass_context(int context_id)
{
    (void)context_id;
}

/* -------------------------------------------------------------------------
 * monitor_force_import — removed in VICE 3.10 but still called by our
 * drivecpu.c (which was upgraded from VICE 3.1's 6510core.c pattern).
 * Returns 0 (no forced import needed).
 * ------------------------------------------------------------------------- */
int monitor_force_import(MEMSPACE mem)
{
    (void)mem;
    return 0;
}

/* -------------------------------------------------------------------------
 * monitor_is_binary — stub for platforms where monitor_binary.c is not
 * included in the build (e.g., Xcode).
 * ------------------------------------------------------------------------- */
#ifndef HAVE_MONITOR_BINARY
int monitor_is_binary(void)
{
    return 0;
}
#endif

/* -------------------------------------------------------------------------
 * mem_get_cursor_parameter / mem_read_screen
 * New VICE 3.10 functions normally provided by each machine's mem.c.
 * Used by monitor_vsync_hook for the "ready" detection feature.
 * Stubs return safe defaults.
 * ------------------------------------------------------------------------- */
void mem_get_cursor_parameter(uint16_t *screen_addr, uint8_t *cursor_column,
                               uint8_t *line_length, int *blinking)
{
    if (screen_addr)    *screen_addr    = 0x0400; /* C64 default screen */
    if (cursor_column)  *cursor_column  = 0;
    if (line_length)    *line_length    = 40;
    if (blinking)       *blinking       = 0;
}

uint8_t mem_read_screen(uint16_t addr)
{
    (void)addr;
    return 0x20; /* space character — never triggers "ready" detection */
}

/* -------------------------------------------------------------------------
 * ui_pause_* — VICE 3.10 UI pause API, provided by each arch's ui.c.
 * c64d doesn't use the VICE pause mechanism; stubs do nothing.
 * ------------------------------------------------------------------------- */
static int ui_is_paused = 0;

int ui_pause_active(void)
{
    return ui_is_paused;
}

void ui_pause_enable(void)
{
    ui_is_paused = 1;
}

void ui_pause_disable(void)
{
    ui_is_paused = 0;
}

bool ui_pause_loop_iteration(void)
{
    return false; /* never block in the pause loop */
}

/* -------------------------------------------------------------------------
 * vsync_get_warp_mode / vsync_set_warp_mode
 * VICE 3.10 API (was set_warp_mode resource callback in 3.1).
 * Delegate to c64d's existing warp flag.
 * ------------------------------------------------------------------------- */
extern int c64d_get_warp_mode(void);

int vsync_get_warp_mode(void)
{
    return c64d_get_warp_mode();
}

void vsync_set_warp_mode(int val)
{
    /* Delegate through the resource setter which also notifies sound. */
    extern int set_warp_mode(int val, void *param);
    set_warp_mode(val, NULL);
}

/* printerdrv upgrade stubs */
#include <stdio.h>
#include "coproc.h"

int fork_coproc(int *fd_wr, int *fd_rd, char *cmd, vice_pid_t *childpid)
{
    (void)fd_wr; (void)fd_rd; (void)cmd; (void)childpid;
    return -1; /* not supported in embedded build */
}

void kill_coproc(vice_pid_t pid)
{
    (void)pid;
}

FILE *archdep_fdopen(int fd, const char *mode) { (void)fd; (void)mode; return NULL; }
void archdep_close(int fd) { (void)fd; }

int printer_userport_init_cmdline_options(void) { return 0; }

/* gfxoutputdrv stubs for drivers we don't compile (no ffmpeg/zmbv libs) */
int gfxoutput_init_ffmpegexe(void) { return 0; }
int gfxoutput_init_zmbv(void) { return 0; }

/* rsuser_reset was made static in 3.10 (called via userport device),
   but c64.c still calls it directly */
void rsuser_reset(void) { /* handled by userport device reset now */ }

/* util functions removed in 3.10 but still called by our code */
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

long util_file_length(const char *name) {
    FILE *f = fopen(name, "rb");
    long len = -1;
    if (f) {
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fclose(f);
    }
    return len;
}

int util_string_to_long(const char *str, const char **endptr, int base, long *result) {
    char *end;
    if (str == NULL || *str == '\0') return -1;
    *result = strtol(str, &end, base);
    if (endptr) *endptr = end;
    return (end == str) ? -1 : 0;
}
