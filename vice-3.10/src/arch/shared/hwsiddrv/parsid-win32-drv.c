/*
 * parsid-win32-drv.c - PARallel port SID support for WIN32.
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

#include "vice.h"

#ifdef WINDOWS_COMPILE

#ifdef HAVE_PARSID
#ifdef HAVE_LIBIEEE1284

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "archdep.h"
#include "parsid.h"
#include "ps-win32.h"
#include "types.h"

static int use_ieee1284 = 0;

void parsid_drv_out_ctr(uint8_t parsid_ctrport, int chipno)
{
    if (use_ieee1284) {
        ps_ieee1284_out_ctr(parsid_ctrport, chipno);
    }
}

uint8_t parsid_drv_in_ctr(int chipno)
{
    if (use_ieee1284) {
        return ps_ieee1284_in_ctr(chipno);
    }

    return 0;
}

int parsid_drv_close(void)
{
    if (use_ieee1284) {
        ps_ieee1284_close();
        use_ieee1284 = 0;
    }

    return 0;
}

uint8_t parsid_drv_in_data(int chipno)
{
    if (use_ieee1284) {
        return ps_ieee1284_in_data(chipno);
    }

    return 0;
}

void parsid_drv_out_data(uint8_t addr, int chipno)
{
    if (use_ieee1284) {
        ps_ieee1284_out_data(addr, chipno);
    }
}


void parsid_drv_sleep(int amount)
{
    archdep_usleep(amount);
}

int parsid_drv_available(void)
{
    if (use_ieee1284) {
        return ps_ieee1284_available();
    }

    return 0;
}

int parsid_drv_open(void)
{
    int i;

    i = ps_ieee1284_open();
    if (!i) {
        use_ieee1284 = 1;
        return 0;
    }

    return -1;
}
#endif
#endif
#endif
