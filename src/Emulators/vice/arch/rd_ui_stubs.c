/*
 * rd_ui_stubs.c - Stubs for VICE 3.10 arch UI entry points that RD does not
 *                 implement (RD uses MTEngineSDL for its own UI). Each stub is
 *                 a no-op with the 3.10 signature so the link resolves.
 */

#include "vice.h"
#include <stdlib.h>

/* --- generic UI / arch lifecycle --- */
void arch_ui_activate(void) {}
int  c64scui_init_early(void) { return 0; }
int  default_settings_requested(void) { return 0; }
void main_exit(void) {}

/* --- joystick (SDL arch) --- */
int  joy_sdl_cmdline_options_init(void) { return 0; }
int  joy_sdl_resources_init(void) { return 0; }
void joystick_arch_init(void) {}
void joystick_arch_shutdown(void) {}

/* --- 3.10 UI action system / hotkeys --- */
void ui_action_trigger(int action) { (void)action; }
void ui_actions_shutdown(void) {}
void ui_hotkeys_shutdown(void) {}

/* --- video canvas / display settings (RD's canvas is MTEngineSDL-driven) --- */
void *ui_get_active_canvas(void) { return NULL; }
void  ui_display_reset(int device, int mode) { (void)device; (void)mode; }
int   ui_set_aspect_mode(int mode, void *canvas) { (void)mode; (void)canvas; return 0; }
int   ui_set_aspect_ratio(double aspect, void *canvas) { (void)aspect; (void)canvas; return 0; }
int   ui_set_flipx(int val, void *canvas) { (void)val; (void)canvas; return 0; }
int   ui_set_flipy(int val, void *canvas) { (void)val; (void)canvas; return 0; }
int   ui_set_fullscreen_custom_height(int h, void *canvas) { (void)h; (void)canvas; return 0; }
int   ui_set_fullscreen_custom_width(int w, void *canvas) { (void)w; (void)canvas; return 0; }
int   ui_set_glfilter(int val, void *canvas) { (void)val; (void)canvas; return 0; }
int   ui_set_rotate(int val, void *canvas) { (void)val; (void)canvas; return 0; }
int   ui_set_vsync(int val, void *canvas) { (void)val; (void)canvas; return 0; }
/* Stock VICE's set_warp_mode lives in arch/<ui>/menu_speed.c (returns int, takes
   the resource setter's void *param). RD doesn't link those UI files, so we
   forward to vsync_set_warp_mode here. Without this, RD's
   CDebugInterfaceVice::SetSettingIsWarpSpeed calls a no-op and warp_enabled
   never flips -- the C64 thread stays throttled at ~1 MHz even with warp ON
   (caught by test_warp_on_increases_cycle_rate). */
extern void vsync_set_warp_mode(int val);
int set_warp_mode(int value, void *param) { (void)param; vsync_set_warp_mode(value); return 0; }

/* --- monitor PETSCII/screencode output (echo back, RD shows debugger console) --- */
int uimon_petscii_out(const char *buffer, int len)         { (void)buffer; return len; }
int uimon_petscii_upper_out(const char *buffer, int len)   { (void)buffer; return len; }
int uimon_scrcode_out(const char *buffer, int len)         { (void)buffer; return len; }
int uimon_scrcode_upper_out(const char *buffer, int len)   { (void)buffer; return len; }

/* --- VSID (SID tune player) UI --- */
void vsid_ui_display_nr_of_tunes(int count) { (void)count; }
void vsid_ui_set_default_tune(int nr) { (void)nr; }

/* --- vsync --- */
void vsync_sync_reset(void) {}

/* Additional stubs surfaced after dropping arch/joy.c. */
int  archdep_default_logger_is_terminal(void) { return 0; }
void joystick_arch_resources_shutdown(void) {}
void sdljoy_autorepeat(void) {}
void sdljoy_swap_ports(void) {}
int  ui_action_get_id(const char *name) { (void)name; return -1; }
const char *ui_action_get_name(int action) { (void)action; return ""; }
