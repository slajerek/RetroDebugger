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

#ifndef BEOS_COMPILE
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
#endif

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE SDLK_UNKNOWN

/* Default sound output mode */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_SYSTEM

/* define if the platform supports the monitor in a seperate window */
/* #define ARCHDEP_SEPERATE_MONITOR_WINDOW */

/** \brief  Default state of mouse grab
 */
#define ARCHDEP_MOUSE_ENABLE_DEFAULT    0

/** \brief  Factory value of the CHIPShowStatusbar resource
 */
#define ARCHDEP_SHOW_STATUSBAR_FACTORY  0

/* FIXME: Ugly hack for preventing SDL crash using -help */
extern int sdl_help_shutdown;

/******************************************************************************/

#ifdef BEOS_COMPILE
#include "archdep_beos.h"
#endif

#if defined(UNIX_COMPILE) && !defined(CEGCC_COMPILE)
#include "archdep_unix.h"
/* Allow native monitor code (on host console) */
#define ALLOW_NATIVE_MONITOR
#endif

#ifdef WINDOWS_COMPILE
#include "archdep_win32.h"
/* This platform supports choosing drives. */
#define SDL_CHOOSE_DRIVES
#endif

#endif
