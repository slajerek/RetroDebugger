/** \file   archdep.h
 * \brief   Miscellaneous system-specific stuff - header
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * \note    Do NOT \#include stdbool.h here, that will lead to weird bugs in
 *          the monitor code.
 */

/*
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

/* XXX: do NOT include <stdbool.h>, causes bugs in monitor code */
#include "vice.h"
#include "sound.h"

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   1     /**< VICII double size */
#define ARCHDEP_VICII_DSCAN   1     /**< VICII double scan */
#define ARCHDEP_VDC_DSIZE     1     /**< VDC double size */
#define ARCHDEP_VDC_DSCAN     1     /**< VDC double scan */
#define ARCHDEP_VIC_DSIZE     1     /**< VIC double size */
#define ARCHDEP_VIC_DSCAN     1     /**< VIC double scan */
#define ARCHDEP_CRTC_DSIZE    1     /**< CRTC double size */
#define ARCHDEP_CRTC_DSCAN    1     /**< CRTC double scan */
#define ARCHDEP_TED_DSIZE     1     /**< TED double size */
#define ARCHDEP_TED_DSCAN     1     /**< TED double scan */

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE 0 /**< no keyboard symcode (?) */

/** \brief  Default sound output mode
 */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_SYSTEM

/** \brief  Define if the platform supports the monitor in a seperate window
 */
#define ARCHDEP_SEPERATE_MONITOR_WINDOW

/** \brief  Default state of mouse grab
 */
#define ARCHDEP_MOUSE_ENABLE_DEFAULT    0

/** \brief  Factory value of the CHIPShowStatusbar resource
 */
#define ARCHDEP_SHOW_STATUSBAR_FACTORY  1


/******************************************************************************/

#ifdef UNIX_COMPILE
#include "archdep_unix.h"
#endif

#ifdef WINDOWS_COMPILE
#include "archdep_win32.h"
#endif

#endif
