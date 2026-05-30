/** \file   archdep_sleep.c
 * \brief   sleep replacements for OS'es that don't support sleep
 *
 * \author  groepaz <groepaz@gmx.net>
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

#include "vice.h"

#ifdef WINDOWS_COMPILE
# include <windows.h>
#endif

#ifdef UNIX_COMPILE
# include <unistd.h>
#endif

#include "archdep_sleep.h"

#ifdef WINDOWS_COMPILE

/* Provide a sleep replacement */
void archdep_sleep(unsigned int seconds)
{
    Sleep((seconds) * 1000U);
}

#endif


#ifdef UNIX_COMPILE

void archdep_sleep(unsigned int seconds)
{
    sleep(seconds);
}

#endif
