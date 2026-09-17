/* [C64D-REFERENCE-ONLY]
 * Vendored from VICE as REFERENCE, not as live code. Kept for features the
 * original emulator has and RetroDebugger does not yet (printer / RS232, drive
 * and cartridge menus, virtual keyboard). Its SDL calls are already commented
 * out; RetroDebugger draws its own ImGui UI.
 * DO NOT delete, and do not "port" it -- see src/Emulators/REFERENCE_FILES.md.
 */
/** \file   archdep.h
 * \brief   Miscellaneous system-specific stuff - header (embedded version)
 *
 * Adapted from VICE 3.10 arch/headless/archdep.h for the embedded build.
 */

#ifndef VICE_ARCHDEP_H
#define VICE_ARCHDEP_H

#include "vice.h"
#include "sound.h"
#include "archdep_defs.h"
#include "archdep_tick.h"

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   1
#define ARCHDEP_VICII_DSCAN   1
#define ARCHDEP_VDC_DSIZE     1
#define ARCHDEP_VDC_DSCAN     1
#define ARCHDEP_VIC_DSIZE     1
#define ARCHDEP_VIC_DSCAN     1
#define ARCHDEP_CRTC_DSIZE    1
#define ARCHDEP_CRTC_DSCAN    1
#define ARCHDEP_TED_DSIZE     1
#define ARCHDEP_TED_DSCAN     1

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE 0

/* Default sound output mode */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_SYSTEM

/* Support for monitor in a separate window */
#define ARCHDEP_SEPERATE_MONITOR_WINDOW

/* Default state of mouse grab */
#define ARCHDEP_MOUSE_ENABLE_DEFAULT    0

/* Factory value of the CHIPShowStatusbar resource */
#define ARCHDEP_SHOW_STATUSBAR_FACTORY  0

#ifdef UNIX_COMPILE
#include "archdep_unix.h"
#endif

#ifdef WINDOWS_COMPILE
#include "archdep_win32.h"
#endif

/* Keyboard host mapping (stub for embedded build) */
static inline int archdep_kbd_get_host_mapping(void) { return 0; /* KBD_MAPPING_US */ }

/* Get user configuration directory */
void  archdep_user_config_path_free(void);
char *archdep_get_vice_datadir(void);
char *archdep_get_vice_docsdir(void);

/* Register CBM font with the OS without installing */
int archdep_register_cbmfont(void);
void archdep_unregister_cbmfont(void);

/* Joystick mapping file */
char *archdep_default_joymap_file_name(void);

/* File I/O helpers (VICE 3.10 arch layer) */
#include <stdio.h>
#include <sys/types.h>
off_t archdep_file_size(FILE *stream);
int archdep_fseeko(FILE *stream, off_t offset, int whence);
off_t archdep_ftello(FILE *stream);
FILE *archdep_fdopen(int fd, const char *mode);
void archdep_close(int fd);

#endif
