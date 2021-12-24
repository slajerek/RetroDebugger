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
