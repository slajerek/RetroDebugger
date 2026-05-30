/*
 * archdep.h - Miscellaneous system-specific stuff.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_ARCHDEP_H
#define VICE_ARCHDEP_H

#include "vice.h"

#include "vice_sdl.h"

#include "sound.h"
#include "archdep_tick.h"		/* VICE 3.10: tick_t + tick_now/tick_sleep/... */
#include "archdep_dir.h"		/* VICE 3.10: archdep_dir_t + archdep_opendir/readdir/closedir (palette/fileio dir scans expect this via archdep.h) */
#include "archdep_current_dir.h"	/* VICE 3.10: archdep_current_dir(). Without this prototype the -Wno-implicit-function-declaration silently truncates the returned char* to int -> sign-extended 64-bit garbage -> free() aborts (caught in sysfile.c set_system_path). */

/* VICE 3.10 archdep_*.h sweep: each declares one function. Without these prototypes -Wno-implicit-function-declaration silently truncates pointer returns to 32-bit ints -> sign-extended 64-bit garbage -> abort/SIGSEGV. Sweep added 2026-05-29 after archdep_current_dir + archdep_default_joymap_file_name caused exactly this. */
#include "archdep_access.h"
#include "archdep_boot_path.h"
#include "archdep_cbmfont.h"
#include "archdep_chdir.h"
#include "archdep_close.h"
#include "archdep_create_user_cache_dir.h"
#include "archdep_create_user_config_dir.h"
#include "archdep_create_user_state_dir.h"
#include "archdep_default_autostart_disk_image_file_name.h"
#include "archdep_default_fliplist_file_name.h"
#include "archdep_default_joymap_file_name.h"
#include "archdep_default_logfile.h"
#include "archdep_default_logger.h"
#include "archdep_default_portable_resource_file_name.h"
#include "archdep_default_resource_file_name.h"
#include "archdep_default_rtc_file_name.h"
#include "archdep_default_sysfile_pathlist.h"
#include "archdep_ethernet_available.h"
#include "archdep_exit.h"
#include "archdep_expand_path.h"
#include "archdep_extra_title_text.h"
#include "archdep_fdopen.h"
#include "archdep_file_exists.h"
#include "archdep_file_is_blockdev.h"
#include "archdep_file_is_chardev.h"
#include "archdep_file_size.h"
#include "archdep_filename_parameter.h"
#include "archdep_fix_permissions.h"
#include "archdep_fix_streams.h"
#include "archdep_fseeko.h"
#include "archdep_ftello.h"
#include "archdep_get_current_drive.h"
#include "archdep_get_hvsc_dir.h"
#include "archdep_get_runtime_info.h"
#include "archdep_get_vice_datadir.h"
#include "archdep_get_vice_docsdir.h"
#include "archdep_get_vice_drivesdir.h"
#include "archdep_get_vice_hotkeysdir.h"
#include "archdep_get_vice_machinedir.h"
#include "archdep_getcwd.h"
#include "archdep_glob.h"
#include "archdep_home_path.h"
#include "archdep_icon_path.h"
#include "archdep_is_haiku.h"
#include "archdep_is_macos_bindist.h"
#include "archdep_is_windows_nt.h"
#include "archdep_kbd_get_host_mapping.h"
#include "archdep_list_drives.h"
#include "archdep_make_backup_filename.h"
#include "archdep_mkdir.h"
#include "archdep_mkstemp_fd.h"
#include "archdep_network.h"
#include "archdep_open_default_log_file.h"
#include "archdep_path_is_relative.h"
#include "archdep_program_name.h"
#include "archdep_program_path.h"
#include "archdep_quote_parameter.h"
#include "archdep_quote_unzip.h"
#include "archdep_rawnet_capability.h"
#include "archdep_real_path.h"
#include "archdep_remove.h"
#include "archdep_rename.h"
#include "archdep_require_vkbd.h"
#include "archdep_rmdir.h"
#include "archdep_rtc_get_centisecond.h"
#include "archdep_sanitize_filename.h"
#include "archdep_set_current_drive.h"
#include "archdep_set_openmp_wait_policy.h"
#include "archdep_signals.h"
#include "archdep_sleep.h"
#include "archdep_socketpeek.h"
#include "archdep_sound.h"
#include "archdep_spawn.h"
#include "archdep_startup_log_error.h"
#include "archdep_stat.h"
#include "archdep_tmpnam.h"
#include "archdep_user_cache_path.h"
#include "archdep_user_config_path.h"
#include "archdep_user_state_path.h"
#include "archdep_usleep.h"
#include "archdep_xdg.h"


/* Extra functions for SDL UI */
//extern char *archdep_default_hotkey_file_name(void);
//extern char *archdep_default_joymap_file_name(void);

/* returns a NULL terminated list of strings. Both the list and the strings
 * must be freed by the caller using lib_free(void*) */
extern char **archdep_list_drives(void);

/* returns a string that corresponds to the current drive. The string must
 * be freed by the caller using lib_free(void*) */
extern char *archdep_get_current_drive(void);

/* sets the current drive to the given string */
extern void archdep_set_current_drive(const char *drive);

/* Virtual keyboard handling */
extern int archdep_require_vkbd(void);

/* VICE 3.10: archdep_access() replaced ioutil_access(). Mode constants match
   POSIX R_OK/W_OK/X_OK/F_OK so the values pass straight through to access(). */
#define ARCHDEP_ACCESS_R_OK 4
#define ARCHDEP_ACCESS_W_OK 2
#define ARCHDEP_ACCESS_X_OK 1
#define ARCHDEP_ACCESS_F_OK 0
int archdep_access(const char *pathname, int mode);

/* VICE 3.10: archdep_mkdir() mode constants (from arch/shared/archdep_mkdir.h).
   Octal POSIX permission bits passed straight to mkdir(). */
#define ARCHDEP_MKDIR_RWXU   0700
#define ARCHDEP_MKDIR_RWXUG  0770
#define ARCHDEP_MKDIR_RWXUGO 0777

/* VICE 3.10 UI factory defaults referenced by the (3.10) core resources. */
#define ARCHDEP_MOUSE_ENABLE_DEFAULT    0
#define ARCHDEP_SHOW_STATUSBAR_FACTORY  0

/* VICE 3.10 archdep macros normally from arch/shared/archdep_defs.h+archdep_unix.h
   (gated on configure-set *_COMPILE macros RD doesn't define). macOS values. */
#define PRI_SIZE_T          "zu"
#define ARCHDEP_DIR_SEP_STR "/"
#define ARCHDEP_DIR_SEP_CHR '/'
#define ARCHDEP_FSDEVICE_DEFAULT_DIR "."   /* CWD (archdep_unix.h value) */

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   1
#define ARCHDEP_VICII_DSCAN   1
#define ARCHDEP_VICII_HWSCALE 1
#define ARCHDEP_VDC_DSIZE     1
#define ARCHDEP_VDC_DSCAN     1
#define ARCHDEP_VDC_HWSCALE   1
#define ARCHDEP_VIC_DSIZE     1
#define ARCHDEP_VIC_DSCAN     1
#define ARCHDEP_VIC_HWSCALE   1
#define ARCHDEP_CRTC_DSIZE    1
#define ARCHDEP_CRTC_DSCAN    1
#define ARCHDEP_CRTC_HWSCALE  1
#define ARCHDEP_TED_DSIZE     1
#define ARCHDEP_TED_DSCAN     1
#define ARCHDEP_TED_HWSCALE   1

/* Video chip double buffering.  */
#define ARCHDEP_VICII_DBUF 0
#define ARCHDEP_VDC_DBUF   0
#define ARCHDEP_VIC_DBUF   0
#define ARCHDEP_CRTC_DBUF  0
#define ARCHDEP_TED_DBUF   0

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE 0x00

#define STATIC_PROTOTYPE static

/* Default sound output mode */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_SYSTEM

/* define if the platform supports the monitor in a seperate window */
/* #define ARCHDEP_SEPERATE_MONITOR_WINDOW */

#ifdef USE_SDLUI2
extern char *archdep_sdl2_default_renderers[];
#endif

#ifdef AMIGA_SUPPORT
#include "archdep_amiga.h"
#endif

#ifdef __BEOS__
#include "archdep_beos.h"
#endif

#if defined(UNIX_COMPILE) && !defined(CEGCC_COMPILE)
#include "archdep_unix.h"
#endif

#if defined(WIN32_COMPILE) && !defined(__XBOX__)
#include "archdep_win32.h"
#endif

#ifdef __XBOX__
#include "archdep_xbox.h"
#endif

#ifdef CEGCC_COMPILE
#include "archdep_cegcc.h"
#endif

#ifdef DINGOO_NATIVE
#include "archdep_dingoo.h"
#endif

#endif

/* VICE 3.10: socket error retrieval (unix uses errno). */
#include <errno.h>
#define ARCHDEP_SOCKET_ERROR errno
